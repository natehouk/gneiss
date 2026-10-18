// Gneiss — stereo compressor engine: three styles (feed-forward / feedback / opto-like), soft knee,
// left/right or mid/side, channel linking, external sidechain with 48 dB/oct detector filters.
// Clean-room implementation from public, generic techniques (log-domain gain computer with quadratic
// soft knee, branching attack/release smoothing, TPT state-variable filters). No third-party product
// code, assets or disassembly were used. License: MIT (see LICENSE).

#pragma once
#include <cmath>
#include <cstdint>
#include <algorithm>

namespace gneiss {

static constexpr float kPi = 3.14159265358979f;
inline float clampf(float x, float lo, float hi) { return x < lo ? lo : (x > hi ? hi : x); }
inline float dbToGain(float db) { return std::exp(db * 0.11512925465f); }            // 10^(db/20)
inline float gainToDb(float g) { return 8.68588963807f * std::log(std::max(g, 1e-9f)); }

// ------------------------------------------------------------------ params (append-only)
enum Param : uint32_t {
    kStyle, kThreshold, kRatio, kKnee, kAttack, kRelease, kGain, kAutoGain, kMix,
    kChannelMode, kLink, kSidechain, kScHpOn, kScHpFreq, kScLpOn, kScLpFreq, kScListen,
    kExpert, kMidiOn,
    // outputs (read-only, for the UI)
    kOutLearn, kOutInL, kOutInR, kOutOutL, kOutOutR, kOutGrL, kOutGrR,
    kParamCount
};
enum Style { kClean, kClassic, kOpto, kNumStyles };

// ------------------------------------------------------------------ helpers
// 48 dB/oct Butterworth: four cascaded TPT SVF stages
struct Filter48 {
    float ic1[4] = {}, ic2[4] = {};
    void reset() { for (int i = 0; i < 4; ++i) ic1[i] = ic2[i] = 0; }
    inline float process(float x, float g, bool highpass) {
        static const float k[4] = { 1.96157056f, 1.66293922f, 1.11114047f, 0.39018064f };
        for (int i = 0; i < 4; ++i) {
            const float a1 = 1.f / (1.f + g * (g + k[i]));
            const float v3 = x - ic2[i], v1 = a1 * ic1[i] + g * a1 * v3, v2 = ic2[i] + g * v1;
            ic1[i] = 2.f * v1 - ic1[i]; ic2[i] = 2.f * v2 - ic2[i];
            x = highpass ? x - k[i] * v1 - v2 : v2;
        }
        return x;
    }
};

// static gain computer: dB of gain reduction (>= 0) for a detector level in dB.
// Feed-forward: over*(1 - 1/R). Feedback (detector sees the output): over*(R - 1).
inline float staticGr(float levelDb, float thr, float ratio, float knee, bool feedback) {
    const float slope = feedback ? std::min(ratio - 1.f, 30.f) : 1.f - 1.f / ratio;
    const float over = levelDb - thr;
    if (knee > 0.01f) {
        if (2.f * over <= -knee) return 0.f;
        if (2.f * over < knee) { const float t = over + knee * 0.5f; return slope * t * t / (2.f * knee); }
    } else if (over <= 0.f) return 0.f;
    return slope * over;
}

// ------------------------------------------------------------------ engine
class Engine {
public:
    float p[kParamCount];

    Engine() {
        std::fill(p, p + kParamCount, 0.f);
        p[kStyle] = kClean; p[kThreshold] = -18; p[kRatio] = 4; p[kKnee] = 6; p[kAttack] = 10; p[kRelease] = 150;
        p[kGain] = 0; p[kAutoGain] = 0; p[kMix] = 1; p[kChannelMode] = 0; p[kLink] = 1; p[kSidechain] = 0;
        p[kScHpOn] = 0; p[kScHpFreq] = 80; p[kScLpOn] = 0; p[kScLpFreq] = 8000; p[kScListen] = 0; p[kExpert] = 0; p[kMidiOn] = 1;
        snap();
    }
    void setSampleRate(float sr) {
        fs = sr;
        smooth = 1.f - std::exp(-1.f / (0.015f * sr));
        meterDecayPerSample = std::exp(-1.f / (0.300f * sr));
        reset();
    }
    void reset() {
        for (int c = 0; c < 2; ++c) { gr[c] = 0; grSlow[c] = 0; rms[c] = 0; hp[c].reset(); lp[c].reset(); }
        for (int c = 0; c < 2; ++c) { mIn[c] = mOut[c] = 0; mGr[c] = 0; }
        snap();
    }

    // read-back for the UI meters (peak dBFS with 300 ms decay; gain reduction in dB)
    float meterIn(int c) const { return gainToDb(mIn[c]); }
    float meterOut(int c) const { return gainToDb(mOut[c]); }
    float meterGr(int c) const { return mGr[c]; }

    // sc may be null when the host gives no sidechain
    void process(const float* inL, const float* inR, const float* scL, const float* scR, float* outL, float* outR, uint32_t n) {
        const int style = (int)(p[kStyle] + 0.5f);
        const bool ms = p[kChannelMode] > 0.5f;
        const bool external = p[kSidechain] > 0.5f && scL && scR;
        // Classic is a feedback design: its detector sees the compressed signal (detector level minus the gain
        // reduction already applied), so it settles towards the target instead of computing it directly
        const bool feedback = style == kClassic;
        const bool hpOn = p[kScHpOn] > 0.5f, lpOn = p[kScLpOn] > 0.5f, listen = p[kScListen] > 0.5f;
        const float gHp = std::tan(kPi * clampf(p[kScHpFreq], 10.f, 0.45f * fs) / fs);
        const float gLp = std::tan(kPi * clampf(p[kScLpFreq], 10.f, 0.45f * fs) / fs);
        // per-style detector and timing character
        const float rmsMs = style == kClean ? 0.f : (style == kClassic ? 4.f : 15.f);
        const float rmsCoef = rmsMs > 0 ? 1.f - std::exp(-1.f / (rmsMs * 0.001f * fs)) : 1.f;
        const float kneeExtra = style == kOpto ? 6.f : (style == kClassic ? 2.f : 0.f);
        const float slowCoef = 1.f - std::exp(-1.f / (0.4f * fs));
        const float tA = clampf(p[kAttack], 0.005f, 500.f) * (style == kOpto ? 1.5f : 1.f), tR = clampf(p[kRelease], 1.f, 5000.f);
        const float aCoef = 1.f - std::exp(-1.f / (tA * 0.001f * fs));

        for (uint32_t i = 0; i < n; ++i) {
            // smoothed controls
            sThr += (p[kThreshold] - sThr) * smooth; sRatio += (clampf(p[kRatio], 1.f, 100.f) - sRatio) * smooth;
            sKnee += (p[kKnee] - sKnee) * smooth; sMix += (clampf(p[kMix], 0.f, 1.f) - sMix) * smooth; sLink += (clampf(p[kLink], 0.f, 1.f) - sLink) * smooth;
            const float autoDb = p[kAutoGain] > 0.5f ? std::max(0.f, -sThr) * (1.f - 1.f / sRatio) * 0.5f : 0.f;
            sMake += (dbToGain(p[kGain] + autoDb) - sMake) * smooth;

            float x[2] = { inL[i], inR[i] };
            if (ms) { const float m = 0.5f * (x[0] + x[1]), s = 0.5f * (x[0] - x[1]); x[0] = m; x[1] = s; }
            float sc[2];
            if (external) { sc[0] = scL[i]; sc[1] = scR[i]; if (ms) { const float m = 0.5f * (sc[0] + sc[1]), s = 0.5f * (sc[0] - sc[1]); sc[0] = m; sc[1] = s; } }
            else { sc[0] = x[0]; sc[1] = x[1]; }

            float target[2];
            for (int c = 0; c < 2; ++c) {
                float s = sc[c];
                if (hpOn) s = hp[c].process(s, gHp, true);
                if (lpOn) s = lp[c].process(s, gLp, false);
                scOut[c] = s;
                rms[c] += (s * s - rms[c]) * rmsCoef;
                const float lvl = rmsMs > 0 ? 10.f * std::log10(std::max(rms[c] * 2.f, 1e-18f)) : gainToDb(std::fabs(s));   // RMS scaled so a sine reads its peak
                target[c] = feedback ? staticGr(lvl - gr[c], sThr, sRatio, sKnee + kneeExtra, true)
                                     : staticGr(lvl, sThr, sRatio, sKnee + kneeExtra, false);
            }
            for (int c = 0; c < 2; ++c) {
                // program-dependent release: Classic and Opto release more slowly after sustained compression
                float rel = tR;
                if (style == kClassic) rel *= 1.f + 1.2f * std::min(1.f, grSlow[c] / 10.f);
                if (style == kOpto)    rel *= 0.6f + 2.2f * std::min(1.f, grSlow[c] / 8.f);
                const float rCoef = 1.f - std::exp(-1.f / (rel * 0.001f * fs));
                gr[c] += (target[c] - gr[c]) * (target[c] > gr[c] ? aCoef : rCoef);
                grSlow[c] += (gr[c] - grSlow[c]) * slowCoef;
            }
            const float gmax = std::max(gr[0], gr[1]);
            float y[2];
            for (int c = 0; c < 2; ++c) {
                const float g = (1.f - sLink) * gr[c] + sLink * gmax;
                const float wet = x[c] * dbToGain(-g);
                mGr[c] = std::max(g, mGr[c] * meterDecayPerSample);
                y[c] = listen ? scOut[c] : wet * sMake;
            }
            if (ms) { const float m = y[0], s = y[1]; y[0] = m + s; y[1] = m - s; }
            const float dry[2] = { inL[i], inR[i] };
            float o[2];
            for (int c = 0; c < 2; ++c) {
                o[c] = listen ? y[c] : dry[c] * (1.f - sMix) + y[c] * sMix;
                if (!(std::fabs(o[c]) < 1e6f)) o[c] = 0.f;
                mIn[c] = std::max(std::fabs(dry[c]), mIn[c] * meterDecayPerSample);
                mOut[c] = std::max(std::fabs(o[c]), mOut[c] * meterDecayPerSample);
            }
            outL[i] = o[0]; outR[i] = o[1];
        }
        for (int c = 0; c < 2; ++c) if (!(std::fabs(gr[c]) < 1e4f) || !(std::fabs(rms[c]) < 1e12f)) { gr[c] = 0; rms[c] = 0; hp[c].reset(); lp[c].reset(); }
    }

    // the static input -> output curve the UI draws (dB in -> dB out, before makeup)
    float curve(float inDb) const {
        const int style = (int)(p[kStyle] + 0.5f);
        const float knee = p[kKnee] + (style == kOpto ? 6.f : (style == kClassic ? 2.f : 0.f));
        return inDb - staticGr(inDb, p[kThreshold], clampf(p[kRatio], 1.f, 100.f), knee, false);
    }

private:
    void snap() {
        sThr = p[kThreshold]; sRatio = clampf(p[kRatio], 1.f, 100.f); sKnee = p[kKnee]; sMix = p[kMix]; sLink = p[kLink];
        sMake = dbToGain(p[kGain] + (p[kAutoGain] > 0.5f ? std::max(0.f, -sThr) * (1.f - 1.f / sRatio) * 0.5f : 0.f));
    }
    float fs = 48000.f, smooth = 0.01f, meterDecayPerSample = 0.9999f;
    float gr[2] = {}, grSlow[2] = {}, rms[2] = {}, scOut[2] = {};
    float mIn[2] = {}, mOut[2] = {}, mGr[2] = {};
    float sThr = -18, sRatio = 4, sKnee = 6, sMix = 1, sLink = 1, sMake = 1;
    Filter48 hp[2], lp[2];
};

} // namespace gneiss
