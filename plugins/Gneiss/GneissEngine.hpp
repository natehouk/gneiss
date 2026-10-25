// Gneiss — stereo compressor engine: fourteen styles (feed-forward, feedback, opto-like, vari-mu, upward, ...),
// variable knee to 72 dB, hold, range, gain-change mix to 200%, lookahead to 20 ms, auto threshold, auto release,
// oversampling to 32x, analog character (pre or post), left/right, mid/side and mid-to-side / side-to-mid modes,
// channel linking, external sidechain with 48 dB/oct detector filters plus a 6-band sidechain EQ, MIDI and
// tempo-synced pulse triggering, audition of the trigger and of the gain change, EBU R128 loudness metering.
// Clean-room implementation from public, generic techniques (log-domain gain computer with quadratic soft knee,
// branching attack/release smoothing, TPT state-variable filters, halfband oversampling, BS.1770 metering).
// No third-party product code, assets or disassembly were used. License: MIT (see LICENSE).

#pragma once
#include "common/Character.hpp"
#include "common/EqBand.hpp"
#include "common/Halfband.hpp"
#include "common/Loudness.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

namespace gneiss {

namespace eq = epsilon::eq;
static constexpr float kPi = 3.14159265358979f;
inline float clampf(float x, float lo, float hi) { return x < lo ? lo : (x > hi ? hi : x); }
inline int clampi(int x, int lo, int hi) { return x < lo ? lo : (x > hi ? hi : x); }
inline float dbToGain(float db) { return std::exp(db * 0.11512925465f); }            // 10^(db/20)
inline float gainToDb(float g) { return 8.68588963807f * std::log(std::max(g, 1e-9f)); }

static constexpr int kScBands = 6;
enum ScField { kSbUsed, kSbOn, kSbShape, kSbFreq, kSbGain, kSbQ, kSbSlope, kSbPlace, kScFields };

// ------------------------------------------------------------------ params (append-only)
enum Param : uint32_t {
    kStyle, kThreshold, kRatio, kKnee, kAttack, kRelease, kGain, kAutoGain, kMix,
    kChannelMode, kLink, kSidechain, kScHpOn, kScHpFreq, kScLpOn, kScLpFreq, kScListen,
    kExpert, kMidiOn,
    // outputs (read-only, for the UI)
    kOutLearn, kOutInL, kOutInR, kOutOutL, kOutOutR, kOutGrL, kOutGrR,
    // ---- 2.0
    kHold, kRange, kGrMix, kLookahead, kOversampling, kAutoThreshold, kAutoRelease,
    kCharacter, kDrive, kCharPos,
    kMidiTrigger, kPulse, kPulseSync, kPulseOffset, kPulseLength, kDelta, kScEqOn,
    kScFirst,
    kScLast = kScFirst + kScBands * kScFields - 1,
    kOutLoudM, kOutLoudS, kOutLoudI, kOutTruePeak, kOutTrigger,
    kParamCount
};
inline uint32_t sp(int band, int field) { return kScFirst + band * kScFields + field; }

// style numbers are stored in sessions: append only
enum Style { kClean, kClassic, kOpto, kVocal, kMastering, kBus, kPunch, kPumping, kVersatile, kSmooth, kUpward, kTTM, kVariMu, kElOp, kNumStyles };
enum ChannelMode { kLeftRight, kMidSide, kMidOnly, kSideOnly, kMidToSide, kSideToMid, kNumChannelModes };
enum CharPos { kCharPost, kCharPre };
static constexpr int kNumOversampling = 6;                       // 1x 2x 4x 8x 16x 32x
inline int osFactor(float v) { return 1 << clampi((int)(v + 0.5f), 0, kNumOversampling - 1); }
static constexpr int kNumPulseSyncs = 10;
inline float pulseBeats(int i) {                                  // length of one pulse period in quarter notes
    static const float t[kNumPulseSyncs] = { 4.f, 2.f, 1.f, 0.5f, 0.25f, 0.125f, 1.5f, 0.75f, 2.f / 3.f, 1.f / 3.f };
    return t[clampi(i, 0, kNumPulseSyncs - 1)];
}
static constexpr float kRatioInf = 99.95f;
inline bool ratioIsInf(float r) { return r >= kRatioInf; }

// how each style detects, times and shapes its gain change (1.x styles keep their exact 1.x behaviour)
struct StyleDef {
    float rmsMs;          // detector RMS window (0 = peak)
    bool feedback;        // detector sees the compressed signal
    float kneeExtra;      // added to the knee
    float attackScale, releaseScale;
    float relBase, relAmt, relRef;     // program-dependent release: rel *= relBase + relAmt * min(1, slowGR / relRef)
    int kind;             // 0 normal, 1 vocal, 2 upward, 3 TTM (up + down), 4 vari-mu, 5 pumping (linear release)
};
inline const StyleDef& styleDef(int s) {
    static const StyleDef t[kNumStyles] = {
        { 0.f,  false, 0.f,  1.f,  1.f,  1.f,  0.f, 1.f, 0 },     // Clean
        { 4.f,  true,  2.f,  1.f,  1.f,  1.f,  1.2f, 10.f, 0 },   // Classic
        { 15.f, false, 6.f,  1.5f, 1.f,  0.6f, 2.2f, 8.f, 0 },    // Opto
        { 8.f,  false, 9.f,  1.f,  1.f,  0.8f, 0.8f, 8.f, 1 },    // Vocal
        { 30.f, false, 4.f,  2.f,  1.2f, 0.8f, 0.6f, 6.f, 0 },    // Mastering
        { 10.f, true,  3.f,  1.2f, 1.f,  0.7f, 1.5f, 6.f, 0 },    // Bus
        { 1.f,  false, 1.f,  2.5f, 0.8f, 1.f,  0.3f, 8.f, 0 },    // Punch
        { 0.f,  false, 0.f,  1.f,  1.f,  1.f,  0.f, 1.f, 5 },     // Pumping
        { 2.f,  false, 3.f,  1.f,  1.f,  0.8f, 0.8f, 8.f, 0 },    // Versatile
        { 20.f, false, 8.f,  1.5f, 1.3f, 0.8f, 1.f, 8.f, 0 },     // Smooth
        { 5.f,  false, 4.f,  1.f,  1.f,  1.f,  0.f, 1.f, 2 },     // Upward
        { 2.f,  false, 6.f,  0.5f, 0.5f, 1.f,  0.f, 1.f, 3 },     // TTM
        { 10.f, false, 12.f, 2.f,  1.f,  1.f,  1.5f, 8.f, 4 },    // Vari-Mu
        { 10.f, false, 10.f, 1.f,  1.f,  0.5f, 3.f, 10.f, 0 },    // El-Op
    };
    return t[clampi(s, 0, kNumStyles - 1)];
}

// ------------------------------------------------------------------ helpers
// 48 dB/oct Butterworth: four cascaded TPT SVF stages (1.x sidechain filters)
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
    const bool inf = ratioIsInf(ratio);
    const float slope = feedback ? std::min(ratio - 1.f, 30.f) : (inf ? 1.f : 1.f - 1.f / ratio);
    const float over = levelDb - thr;
    if (knee > 0.01f) {
        if (2.f * over <= -knee) return 0.f;
        if (2.f * over < knee) { const float t = over + knee * 0.5f; return slope * t * t / (2.f * knee); }
    } else if (over <= 0.f) return 0.f;
    return slope * over;
}
// upward: dB of gain *increase* (>= 0) below the threshold, fading out far below it (so noise isn't raised)
inline float upwardBoost(float levelDb, float thr, float ratio, float knee) {
    const float slope = ratioIsInf(ratio) ? 1.f : 1.f - 1.f / ratio;
    const float under = thr - levelDb;
    float b;
    if (knee > 0.01f) { if (2.f * under <= -knee) return 0.f; b = 2.f * under < knee ? slope * (under + knee * 0.5f) * (under + knee * 0.5f) / (2.f * knee) : slope * under; }
    else { if (under <= 0.f) return 0.f; b = slope * under; }
    const float fade = clampf((thr - 40.f - levelDb) / 20.f, 0.f, 1.f);       // 40-60 dB below threshold: back to 0
    return b * (1.f - fade);
}

inline float variMuRatio(float lvl, float thr, float ratio) { return 1.f + (ratio - 1.f) * clampf((lvl - thr) / 18.f, 0.15f, 1.f); }

// the static input -> output curve (dB in -> dB out, before make-up) for a parameter set; the UI draws it
inline float transferCurve(const float* p, float inDb) {
    const StyleDef& sd = styleDef(clampi((int)(p[kStyle] + 0.5f), 0, kNumStyles - 1));
    const float knee = p[kKnee] + sd.kneeExtra, ratio = clampf(p[kRatio], 1.f, 100.f), thr = p[kThreshold];
    const float range = clampf(p[kRange], 0.f, 60.f), mix = clampf(p[kGrMix], 0.f, 200.f) * 0.01f;
    float gc;
    if (sd.kind == 2) gc = -std::min(upwardBoost(inDb, thr, ratio, knee), range < 59.95f ? 60.f : 24.f);
    else if (sd.kind == 3) gc = staticGr(inDb, thr, ratio, knee, false) - std::min(upwardBoost(inDb, thr - 12.f, ratio, knee), 12.f);
    else if (sd.kind == 4) gc = staticGr(inDb, thr, variMuRatio(inDb, thr, ratio), knee, false);
    else gc = staticGr(inDb, thr, ratio, knee, false);
    if (range < 59.95f) gc = clampf(gc, -range, range);
    return inDb - gc * mix;
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
        p[kHold] = 0; p[kRange] = 60; p[kGrMix] = 100; p[kLookahead] = 0; p[kOversampling] = 0; p[kAutoThreshold] = 0; p[kAutoRelease] = 0;
        p[kCharacter] = 0; p[kDrive] = 30; p[kCharPos] = kCharPost;
        p[kMidiTrigger] = 0; p[kPulse] = 0; p[kPulseSync] = 2; p[kPulseOffset] = 0; p[kPulseLength] = 25; p[kDelta] = 0; p[kScEqOn] = 1;
        static const float f0[kScBands] = { 60, 150, 400, 1200, 3500, 9000 };
        for (int b = 0; b < kScBands; ++b) {
            p[sp(b, kSbUsed)] = 0; p[sp(b, kSbOn)] = 1; p[sp(b, kSbShape)] = eq::kBell; p[sp(b, kSbFreq)] = f0[b];
            p[sp(b, kSbGain)] = 0; p[sp(b, kSbQ)] = 1; p[sp(b, kSbSlope)] = 1; p[sp(b, kSbPlace)] = 0;
        }
        p[kOutLoudM] = p[kOutLoudS] = p[kOutLoudI] = -70; p[kOutTruePeak] = -70;
        snap();
    }
    void setSampleRate(float sr) {
        fs = sr;
        smooth = 1.f - std::exp(-1.f / (0.015f * sr));
        meterDecayPerSample = std::exp(-1.f / (0.300f * sr));
        loud.setSampleRate(sr);
        maxLook = (int)std::ceil(0.020f * sr) + 2;
        reset();
    }
    void reset() {
        for (int c = 0; c < 2; ++c) { gr[c] = 0; grSlow[c] = 0; rms[c] = 0; hp[c].reset(); lp[c].reset(); holdLeft[c] = 0; relStart[c] = 0; lastGr[c] = 0; os[c].reset(); }
        for (int c = 0; c < 2; ++c) { mIn[c] = mOut[c] = 0; mGr[c] = 0; }
        resetScEq(); for (auto& k : scKey) k = -1;
        const int len = maxLook + 2 * 64 + 4;
        for (int c = 0; c < 2; ++c) { look[c].assign(len, 0.f); dryLine[c].assign(len, 0.f); }
        lookPos = 0; ltLevel = -18.f; ltMs = 0.00792f; noteVel = 0; notesHeld = 0; freeBeats = 0; trigNow = 0;
        chr.reset(); chrKey = -1;
        loud.reset();
        snap();
    }

    // latency (samples) the host must compensate: lookahead plus oversampling
    uint32_t latency() const { return (uint32_t)(lookSamples() + epsilon::OversamplerN::latency(osFactor(p[kOversampling]))); }

    // read-back for the UI meters (peak dBFS with 300 ms decay; gain change in dB, negative when boosting)
    float meterIn(int c) const { return gainToDb(mIn[c]); }
    float meterOut(int c) const { return gainToDb(mOut[c]); }
    float meterGr(int c) const { return mGr[c]; }
    const epsilon::LoudnessMeter& loudness() const { return loud; }
    void resetLoudness() { loud.resetIntegrated(); }
    float triggerLevel() const { return trigNow; }

    // MIDI triggering (velocity 1-127 while held) and the host transport for pulse triggering
    void noteOn(int vel) { ++notesHeld; noteVel = std::max(noteVel, vel); }
    void noteOff() { if (notesHeld > 0 && --notesHeld == 0) noteVel = 0; }
    void allNotesOff() { notesHeld = 0; noteVel = 0; }
    void setTransport(bool playing, double ppq, double bpm) { tPlaying = playing; tPpq = ppq; tBpm = bpm > 1 ? bpm : 120.0; tValid = true; }

    // sc may be null when the host gives no sidechain
    void process(const float* inL, const float* inR, const float* scL, const float* scR, float* outL, float* outR, uint32_t n) {
        const int style = clampi((int)(p[kStyle] + 0.5f), 0, kNumStyles - 1);
        const StyleDef& sd = styleDef(style);
        const int chm = clampi((int)(p[kChannelMode] + 0.5f), 0, kNumChannelModes - 1);
        const bool ms = chm != kLeftRight;
        const bool external = p[kSidechain] > 0.5f && scL && scR;
        const bool feedback = sd.feedback;
        const bool hpOn = p[kScHpOn] > 0.5f, lpOn = p[kScLpOn] > 0.5f, listen = p[kScListen] > 0.5f, delta = p[kDelta] > 0.5f;
        const float gHp = std::tan(kPi * clampf(p[kScHpFreq], 10.f, 0.45f * fs) / fs);
        const float gLp = std::tan(kPi * clampf(p[kScLpFreq], 10.f, 0.45f * fs) / fs);
        const float rmsMs = sd.rmsMs;
        const float rmsCoef = rmsMs > 0 ? 1.f - std::exp(-1.f / (rmsMs * 0.001f * fs)) : 1.f;
        const float slowCoef = 1.f - std::exp(-1.f / (0.4f * fs));
        const float tA = clampf(p[kAttack], 0.005f, 500.f) * sd.attackScale, tR = clampf(p[kRelease], 1.f, 5000.f) * sd.releaseScale;
        const float aCoef = 1.f - std::exp(-1.f / (tA * 0.001f * fs));
        const int holdSamples = (int)(clampf(p[kHold], 0.f, 500.f) * 0.001f * fs);
        const float range = clampf(p[kRange], 0.f, 60.f);
        const float grMix = clampf(p[kGrMix], 0.f, 200.f) * 0.01f;
        const bool autoThr = p[kAutoThreshold] > 0.5f, autoRel = p[kAutoRelease] > 0.5f;
        const int L = lookSamples();
        const int osf = osFactor(p[kOversampling]);
        const int osLat = epsilon::OversamplerN::latency(osf);
        if (osf != lastOs) { for (int c = 0; c < 2; ++c) os[c].reset(); lastOs = osf; }
        const int chMode = clampi((int)(p[kCharacter] + 0.5f), 0, epsilon::Character::kNumModes - 1);
        const float chKey = (float)chMode * 1000.f + p[kDrive] + (float)osf * 1e6f;
        if (chKey != chrKey) { chrKey = chKey; chr.setup(fs * osf, chMode, clampf(p[kDrive], 0.f, 100.f) * 0.01f, fs); }   // runs inside the oversampling
        const bool chPre = p[kCharPos] > 0.5f;
        const bool useEq = p[kScEqOn] > 0.5f && updateScEq();
        const float ltCoef = 1.f - std::exp(-1.f / (1.0f * fs));         // auto threshold follows the level over about a second
        // triggers
        const bool midiTrig = p[kMidiTrigger] > 0.5f, pulse = p[kPulse] > 0.5f;
        const double period = pulseBeats((int)(p[kPulseSync] + 0.5f));
        const double pOff = clampf(p[kPulseOffset], 0.f, 100.f) * 0.01, pLen = clampf(p[kPulseLength], 0.f, 100.f) * 0.01;
        const double beatsPerSample = (tValid ? tBpm : 120.0) / 60.0 / fs;
        double ppq = (tValid && tPlaying) ? tPpq : freeBeats;
        const uint32_t rd = (uint32_t)look[0].size();
        float trigMax = 0;

        for (uint32_t i = 0; i < n; ++i) {
            // smoothed controls
            sThr += (p[kThreshold] - sThr) * smooth; sRatio += (clampf(p[kRatio], 1.f, 100.f) - sRatio) * smooth;
            sKnee += (p[kKnee] - sKnee) * smooth; sMix += (clampf(p[kMix], 0.f, 1.f) - sMix) * smooth; sLink += (clampf(p[kLink], 0.f, 1.f) - sLink) * smooth;
            const float autoDb = p[kAutoGain] > 0.5f ? std::max(0.f, -sThr) * (1.f - 1.f / sRatio) * 0.5f : 0.f;
            sMake += (dbToGain(p[kGain] + autoDb) - sMake) * smooth;
            chr.tick();

            float x[2] = { inL[i], inR[i] };
            if (ms) { const float m = 0.5f * (x[0] + x[1]), s = 0.5f * (x[0] - x[1]); x[0] = m; x[1] = s; }
            float sc[2];
            if (external) { sc[0] = scL[i]; sc[1] = scR[i]; if (ms) { const float m = 0.5f * (sc[0] + sc[1]), s = 0.5f * (sc[0] - sc[1]); sc[0] = m; sc[1] = s; } }
            else { sc[0] = x[0]; sc[1] = x[1]; }
            if (useEq) applyScEq(sc, ms);

            // trigger level: MIDI notes and tempo pulses act like a signal at velocity / 0 dBFS
            float trig = 0;
            if (midiTrig && noteVel > 0) trig = noteVel / 127.f;
            if (pulse) { double ph = std::fmod(ppq / period - pOff, 1.0); if (ph < 0) ph += 1.0; if (ph < pLen) trig = 1.f; }
            ppq += beatsPerSample;
            trigMax = std::max(trigMax, trig);
            const float trigDb = trig > 0 ? gainToDb(trig) : -200.f;

            // auto threshold: follow the long-term level so the threshold stays relative to the program
            if (autoThr) {   // mean square of the key (silence doesn't pull it down), read as the level of a sine
                const float e2 = 0.5f * (sc[0] * sc[0] + sc[1] * sc[1]);
                if (e2 > 1e-8f) ltMs += (e2 - ltMs) * ltCoef;
                if ((i & 15) == 0) ltLevel = clampf(10.f * std::log10(std::max(2.f * ltMs, 1e-12f)), -60.f, 0.f);
            }
            const float thrNow = sThr + (autoThr ? clampf(ltLevel + 18.f, -24.f, 24.f) : 0.f) + (chMode == epsilon::Character::kVintage ? 0.4f * chr.drift(0) : 0.f);

            float target[2];
            for (int c = 0; c < 2; ++c) {
                float s = sc[c];
                if (hpOn) s = hp[c].process(s, gHp, true);
                if (lpOn) s = lp[c].process(s, gLp, false);
                scOut[c] = s;
                rms[c] += (s * s - rms[c]) * rmsCoef;
                float lvl = rmsMs > 0 ? 10.f * std::log10(std::max(rms[c] * 2.f, 1e-18f)) : gainToDb(std::fabs(s));   // RMS scaled so a sine reads its peak
                lvl = std::max(lvl, trigDb);
                target[c] = gainFor(sd, lvl, c, thrNow, feedback);
            }
            if (chm >= kMidOnly) {     // one part detects, one part is compressed
                const int det = (chm == kMidOnly || chm == kMidToSide) ? 0 : 1;
                target[0] = target[1] = target[det];
            }
            for (int c = 0; c < 2; ++c) {
                // program-dependent release (1.x Classic and Opto, and the newer styles)
                float rel = tR * (sd.relAmt > 0 ? sd.relBase + sd.relAmt * std::min(1.f, grSlow[c] / sd.relRef) : 1.f);
                if (autoRel) rel *= 0.35f + 1.6f * std::min(1.f, grSlow[c] / (std::fabs(gr[c]) + 1.f));
                const float rCoef = 1.f - std::exp(-1.f / (rel * 0.001f * fs));
                const bool attacking = std::fabs(target[c]) > std::fabs(gr[c]) && target[c] * gr[c] >= 0.f;
                if (attacking) { gr[c] += (target[c] - gr[c]) * aCoef; holdLeft[c] = holdSamples; relStart[c] = gr[c]; }
                else if (holdLeft[c] > 0) { --holdLeft[c]; relStart[c] = gr[c]; }
                else if (sd.kind == 5) {   // Pumping: release at a constant dB rate, so the level swells back audibly
                    const float step = std::max(std::fabs(relStart[c]), 1.f) / (rel * 0.001f * fs);
                    gr[c] = gr[c] > target[c] ? std::max(target[c], gr[c] - step) : std::min(target[c], gr[c] + step);
                } else gr[c] += (target[c] - gr[c]) * rCoef;
                grSlow[c] += (gr[c] - grSlow[c]) * slowCoef;
            }
            const float gmax = std::fabs(gr[0]) >= std::fabs(gr[1]) ? gr[0] : gr[1];

            // the delayed (lookahead) signal the gain is applied to, and the dry signal for the mix
            float xd[2], dry[2];
            const uint32_t w = lookPos;
            for (int c = 0; c < 2; ++c) {
                look[c][w] = x[c];
                dryLine[c][w] = c == 0 ? inL[i] : inR[i];
                xd[c] = look[c][(w + rd - (uint32_t)L) % rd];
                dry[c] = dryLine[c][(w + rd - (uint32_t)(L + osLat)) % rd];
            }
            lookPos = (lookPos + 1) % rd;

            float y[2], g[2];
            for (int c = 0; c < 2; ++c) {
                float gc = (1.f - sLink) * gr[c] + sLink * gmax;
                if (chm >= kMidOnly) gc = gr[c];
                if (range < 59.95f) gc = clampf(gc, -range, range);        // 60 dB = no limit
                gc *= grMix;
                if (chm == kMidOnly || chm == kSideToMid) { if (c == 1) gc = 0; }
                if (chm == kSideOnly || chm == kMidToSide) { if (c == 0) gc = 0; }
                g[c] = gc;
                mGr[c] = std::fabs(gc) >= std::fabs(mGr[c] * meterDecayPerSample) ? gc : mGr[c] * meterDecayPerSample;
            }
            for (int c = 0; c < 2; ++c) {
                const float s0 = lastGr[c], s1 = g[c];
                lastGr[c] = s1;
                float wet;
                if (osf == 1 && chMode == epsilon::Character::kOff) wet = delta ? xd[c] - xd[c] * dbToGain(-s1) : xd[c] * dbToGain(-s1);
                else {
                    float up[32]; os[c].up(xd[c], osf, up);
                    const float g0 = dbToGain(-s0), g1 = dbToGain(-s1), gStep = (g1 - g0) / osf;
                    for (int k = 0; k < osf; ++k) {
                        const float gg = g0 + gStep * (float)(k + 1);                          // gain interpolated across the sub-samples
                        float v = up[k];
                        if (chPre) v = chr.process(v, c);
                        float o = v * gg;
                        if (delta) o = v - o;
                        else if (!chPre) o = chr.process(o, c);
                        up[k] = o;
                    }
                    wet = chr.tone(os[c].down(up, osf), c);       // Vintage's linear tone runs at the base rate
                }
                y[c] = listen ? scOut[c] : wet * (delta ? 1.f : sMake);
            }
            if (ms) { const float m = y[0], s = y[1]; y[0] = m + s; y[1] = m - s; }
            float o[2];
            for (int c = 0; c < 2; ++c) {
                o[c] = (listen || delta) ? y[c] : dry[c] * (1.f - sMix) + y[c] * sMix;
                if (!(std::fabs(o[c]) < 1e6f)) o[c] = 0.f;
                mIn[c] = std::max(std::fabs(c == 0 ? inL[i] : inR[i]), mIn[c] * meterDecayPerSample);
                mOut[c] = std::max(std::fabs(o[c]), mOut[c] * meterDecayPerSample);
            }
            outL[i] = o[0]; outR[i] = o[1];
        }
        if (!(tValid && tPlaying)) freeBeats = ppq;
        trigNow = trigMax;
        loud.process(outL, outR, n);
        for (int c = 0; c < 2; ++c) if (!(std::fabs(gr[c]) < 1e4f) || !(std::fabs(rms[c]) < 1e12f)) { gr[c] = 0; rms[c] = 0; hp[c].reset(); lp[c].reset(); resetScEq(); }
    }

    // the static input -> output curve the UI draws (dB in -> dB out, before makeup)
    float curve(float inDb) const { return transferCurve(p, inDb); }
    // the sidechain EQ's response (for the UI)
    eq::cpx scEqResponse(float hz) const {
        eq::cpx h(1.f, 0.f);
        for (int b = 0; b < kScBands; ++b) {
            if (p[sp(b, kSbUsed)] < 0.5f || p[sp(b, kSbOn)] < 0.5f) continue;
            eq::Design d; eq::design(scSpec(b), fs, d); h *= eq::response(d, hz);
        }
        return h;
    }
    eq::Spec scSpec(int b) const {
        eq::Spec s; s.shape = clampi((int)(p[sp(b, kSbShape)] + 0.5f), 0, eq::kNumShapes - 1); s.freq = clampf(p[sp(b, kSbFreq)], 10.f, 30000.f);
        s.gainDb = p[sp(b, kSbGain)]; s.q = clampf(p[sp(b, kSbQ)], 0.025f, 40.f);
        static const int sl[17] = { 6, 12, 24, 48, 18, 30, 36, 42, 54, 60, 66, 72, 78, 84, 90, 96, eq::kBrickwall };
        const int idx = clampi((int)(p[sp(b, kSbSlope)] + 0.5f), 0, 16);
        s.slopeDb = eq::isCut(s.shape) ? sl[idx] : (sl[idx] == eq::kBrickwall ? 96 : sl[idx]);
        return s;
    }
    float sampleRate() const { return fs; }

private:
    float gainFor(const StyleDef& sd, float lvl, int c, float thr, bool feedback) const {
        switch (sd.kind) {
        case 1: {   // Vocal: gentle at first, firmer on loud words
            const float r = 1.f + (sRatio - 1.f) * clampf(0.4f + (lvl - thr) / 15.f, 0.4f, 1.f);
            return staticGr(lvl, thr, r, sKnee + sd.kneeExtra, false); }
        case 2: return -std::min(upwardBoost(lvl, thr, sRatio, sKnee + sd.kneeExtra), clampf(p[kRange], 0.f, 60.f) < 59.95f ? 60.f : 24.f);   // with no range set, upward boosts at most 24 dB
        case 3: return staticGr(lvl, thr, sRatio, sKnee + sd.kneeExtra, false) - std::min(upwardBoost(lvl, thr - 12.f, sRatio, sKnee + sd.kneeExtra), 12.f);
        case 4: return staticGr(lvl, thr, variMuRatio(lvl, thr, sRatio), sKnee + sd.kneeExtra, false);
        default: return feedback ? staticGr(lvl - gr[c], thr, sRatio, sKnee + sd.kneeExtra, true)
                                 : staticGr(lvl, thr, sRatio, sKnee + sd.kneeExtra, false);
        }
    }
    int lookSamples() const { return std::min(maxLook - 1, (int)std::lround(clampf(p[kLookahead], 0.f, 20.f) * 0.001f * fs)); }
    // sidechain EQ: rebuilt when its settings change; returns whether any band is active
    bool updateScEq() {
        bool any = false;
        for (int b = 0; b < kScBands; ++b) {
            const bool on = p[sp(b, kSbUsed)] > 0.5f && p[sp(b, kSbOn)] > 0.5f;
            float key = on ? 1.f : 0.f; for (int f = kSbShape; f < kScFields; ++f) key = key * 1.000123f + p[sp(b, f)] * (float)(f + 3);
            if (key != scKey[b]) {
                scKey[b] = key;
                if (on) { eq::Design d; eq::design(scSpec(b), fs, d); scBand[b].set(d, fs, scSpec(b).shape == eq::kFlatTilt ? 0.f : scSpec(b).freq); }
                scActive[b] = on; scPlace[b] = clampi((int)(p[sp(b, kSbPlace)] + 0.5f), 0, 2);
            }
            any |= scActive[b];
        }
        return any;
    }
    // bands may work on the sidechain's mid or side only; the sidechain is converted as needed
    void applyScEq(float* sc, bool ms) {
        float m = ms ? sc[0] : 0.5f * (sc[0] + sc[1]), s = ms ? sc[1] : 0.5f * (sc[0] - sc[1]);
        float l = sc[0], r = sc[1];
        bool needMs = false; for (int b = 0; b < kScBands; ++b) if (scActive[b] && scPlace[b] != 0) needMs = true;
        if (!needMs && !ms) { for (int b = 0; b < kScBands; ++b) if (scActive[b]) { l = scBand[b].process(l, 0); r = scBand[b].process(r, 1); } sc[0] = l; sc[1] = r; return; }
        for (int b = 0; b < kScBands; ++b) {
            if (!scActive[b]) continue;
            if (scPlace[b] != 2) m = scBand[b].process(m, 0);
            if (scPlace[b] != 1) s = scBand[b].process(s, 1);
        }
        if (ms) { sc[0] = m; sc[1] = s; } else { sc[0] = m + s; sc[1] = m - s; }
    }
    void resetScEq() { for (auto& b : scBand) b.reset(); }
    void snap() {
        sThr = p[kThreshold]; sRatio = clampf(p[kRatio], 1.f, 100.f); sKnee = p[kKnee]; sMix = p[kMix]; sLink = p[kLink];
        sMake = dbToGain(p[kGain] + (p[kAutoGain] > 0.5f ? std::max(0.f, -sThr) * (1.f - 1.f / sRatio) * 0.5f : 0.f));
    }
    float fs = 48000.f, smooth = 0.01f, meterDecayPerSample = 0.9999f;
    float gr[2] = {}, grSlow[2] = {}, rms[2] = {}, scOut[2] = {}, relStart[2] = {}, lastGr[2] = {};
    int holdLeft[2] = {};
    float mIn[2] = {}, mOut[2] = {}, mGr[2] = {};
    float sThr = -18, sRatio = 4, sKnee = 6, sMix = 1, sLink = 1, sMake = 1;
    Filter48 hp[2], lp[2];
    eq::Cascade<2> scBand[kScBands]; float scKey[kScBands] = { -1, -1, -1, -1, -1, -1 }; bool scActive[kScBands] = {}; int scPlace[kScBands] = {};
    std::vector<float> look[2], dryLine[2]; uint32_t lookPos = 0; int maxLook = 962;
    epsilon::OversamplerN os[2]; int lastOs = 1;
    epsilon::Character chr; float chrKey = -1;
    epsilon::LoudnessMeter loud;
    float ltLevel = -18.f, ltMs = 0.00792f, trigNow = 0;      // ltMs starts at the mean square of a -18 dB sine
    int noteVel = 0, notesHeld = 0;
    bool tPlaying = false, tValid = false; double tPpq = 0, tBpm = 120, freeBeats = 0;
};

} // namespace gneiss
