// Behaviour checks: static curve per style, knee, attack/release timing, program-dependent release,
// stereo link, mid/side, external sidechain, sidechain filters and listen, mix, makeup and auto gain.
#include "../plugins/Gneiss/ParamDefs.hpp"
#include <cstdio>
#include <vector>
using namespace gneiss;
static int fails = 0;
#define CHECK(c, msg) do { bool ok_ = (c); printf("%s  %s\n", ok_ ? "PASS" : "FAIL", msg); if (!ok_) fails++; } while (0)
static const int SR = 48000;
struct Buf { std::vector<float> l, r; };
static Buf run(Engine& e, const std::vector<float>& xl, const std::vector<float>& xr, const std::vector<float>* sl = nullptr, const std::vector<float>* sr = nullptr) {
    Buf b; b.l.resize(xl.size()); b.r.resize(xl.size());
    for (size_t i = 0; i < xl.size(); i += 256) {
        const uint32_t n = (uint32_t)std::min<size_t>(256, xl.size() - i);
        e.process(xl.data() + i, xr.data() + i, sl ? sl->data() + i : nullptr, sr ? sr->data() + i : nullptr, b.l.data() + i, b.r.data() + i, n);
    }
    return b;
}
static Buf run(Engine& e, const std::vector<float>& x) { return run(e, x, x); }
static float peakDb(const std::vector<float>& v, size_t a, size_t z) { float m = 0; for (size_t i = a; i < z; ++i) m = std::max(m, std::fabs(v[i])); return gainToDb(m); }
static std::vector<float> sine(int n, float f, float db) { std::vector<float> v(n); const float a = dbToGain(db); for (int i = 0; i < n; ++i) v[i] = a * std::sin(2 * kPi * f * i / SR); return v; }
static Engine comp(int style, float thr, float ratio, float knee = 0, float atk = 1, float rel = 50) {
    Engine e; e.setSampleRate(SR); e.p[kStyle] = style; e.p[kThreshold] = thr; e.p[kRatio] = ratio; e.p[kKnee] = knee; e.p[kAttack] = atk; e.p[kRelease] = rel; e.reset(); return e;
}
int main() {
    char msg[160];
    // static curve: 1 kHz sine at -6 dBFS, threshold -18, ratio 4, hard knee -> -15 dBFS out
    const char* names[3] = { "Clean", "Classic", "Opto" };
    for (int st = 0; st < 3; ++st) {
        Engine e = comp(st, -18, 4, 0, 1, 50); if (st == kOpto) e.p[kKnee] = 0;
        Buf b = run(e, sine(SR, 1000, -6)); const float out = peakDb(b.l, SR / 2, SR);
        const float want = st == kOpto ? -15.f : -15.f;
        snprintf(msg, sizeof msg, "%-7s -6 dB in, thr -18, 4:1 -> %.2f dB out (ideal -15.0)", names[st], out);
        CHECK(std::fabs(out - want) < (st == kClean ? 0.5f : 1.5f), msg);
    }
    { // below threshold: untouched; ratio 1: untouched
        Engine e = comp(kClean, -18, 4); Buf b = run(e, sine(SR / 2, 1000, -30));
        CHECK(std::fabs(peakDb(b.l, SR / 4, SR / 2) + 30) < 0.05f, "below threshold: no gain change");
        Engine u = comp(kClean, -40, 1); Buf c = run(u, sine(SR / 2, 1000, -6));
        CHECK(std::fabs(peakDb(c.l, SR / 4, SR / 2) + 6) < 0.05f, "ratio 1:1: no gain change");
    }
    { // knee: at the threshold, a 12 dB knee gives (1-1/R)*W/8 = 1.13 dB of reduction
        Engine e = comp(kClean, -18, 4, 12); Buf b = run(e, sine(SR, 1000, -18)); const float gr = -18 - peakDb(b.l, SR / 2, SR);
        snprintf(msg, sizeof msg, "12 dB soft knee at threshold: %.2f dB reduction (ideal 1.13)", gr); CHECK(std::fabs(gr - 1.125f) < 0.2f, msg);
    }
    { // attack and release timing: 20 dB step over threshold; GR reaches 63% after about the attack time
        Engine e = comp(kClean, -30, 20, 0, 20, 200);
        std::vector<float> x(SR * 2, 0.f); for (int i = 0; i < SR * 2; ++i) x[i] = (i >= SR / 2 && i < SR) ? 0.316f : 0.01f;   // -10 dB burst, -40 dB rest
        std::vector<float> L(SR * 2), R(SR * 2), grT;
        for (int i = 0; i < SR * 2; i += 48) { e.process(x.data() + i, x.data() + i, nullptr, nullptr, L.data() + i, R.data() + i, 48); }
        // measure from the gain applied to the DC-like input
        auto grAt = [&](int i) { return gainToDb(x[i]) - gainToDb(std::fabs(L[i])); };
        const float full = grAt(SR - 10); int tA = -1, tR = -1;
        for (int i = SR / 2; i < SR; ++i) if (grAt(i) >= 0.632f * full) { tA = i - SR / 2; break; }
        for (int i = SR; i < 2 * SR; ++i) if (grAt(i) <= 0.368f * full) { tR = i - SR; break; }
        snprintf(msg, sizeof msg, "timing: full GR %.1f dB, attack 63%% after %.1f ms (set 20), release 63%% after %.0f ms (set 200)", full, tA / 48.f, tR / 48.f);
        CHECK(std::fabs(full - 19) < 0.6f && std::fabs(tA / 48.f - 20) < 3 && std::fabs(tR / 48.f - 200) < 25, msg);
    }
    { // program-dependent release: after 3 s of heavy compression, Opto releases much more slowly than Clean
        auto relTime = [](int style) {
            Engine e = comp(style, -30, 10, 0, 5, 100);
            std::vector<float> x(SR * 4); for (int i = 0; i < SR * 4; ++i) x[i] = i < SR * 3 ? 0.5f : 0.003f;
            std::vector<float> L(x.size()), R(x.size());
            for (size_t i = 0; i < x.size(); i += 64) e.process(x.data() + i, x.data() + i, nullptr, nullptr, L.data() + i, R.data() + i, 64);
            const float full = gainToDb(x[SR * 3 - 1]) - gainToDb(std::fabs(L[SR * 3 - 1]));
            for (int i = SR * 3; i < SR * 4; ++i) if (gainToDb(x[i]) - gainToDb(std::fabs(L[i])) < 0.368f * full) return (i - SR * 3) / 48.f;
            return 1000.f;
        };
        const float c = relTime(kClean), o = relTime(kOpto), k = relTime(kClassic);
        snprintf(msg, sizeof msg, "program-dependent release after sustained GR: Clean %.0f ms, Classic %.0f ms, Opto %.0f ms", c, k, o);
        CHECK(o > c * 1.8f && k > c * 1.3f, msg);
    }
    { // stereo link: a loud left channel ducks the right only when linked
        auto rightGr = [](float link) { Engine e = comp(kClean, -30, 10); e.p[kLink] = link; e.reset();
            Buf b = run(e, sine(SR, 1000, -6), sine(SR, 1000, -40)); return -40 - peakDb(b.r, SR / 2, SR); };
        const float g0 = rightGr(0), g1 = rightGr(1), gh = rightGr(0.5f);
        snprintf(msg, sizeof msg, "stereo link: right channel GR %.1f dB unlinked, %.1f at 50%%, %.1f linked", g0, gh, g1);
        CHECK(g0 < 0.1f && g1 > 20 && std::fabs(gh - g1 / 2) < 1.5f, msg);
    }
    { // mid/side, unlinked: a loud side signal is compressed while the mid passes untouched
        Engine e = comp(kClean, -24, 10); e.p[kChannelMode] = 1; e.p[kLink] = 0; e.reset();
        auto m = sine(SR, 300, -30), s = sine(SR, 2000, -6);
        std::vector<float> L(SR), R(SR); for (int i = 0; i < SR; ++i) { L[i] = m[i] + s[i]; R[i] = m[i] - s[i]; }
        Buf b = run(e, L, R);
        std::vector<float> mo(SR), so(SR); for (int i = 0; i < SR; ++i) { mo[i] = 0.5f * (b.l[i] + b.r[i]); so[i] = 0.5f * (b.l[i] - b.r[i]); }
        const float md = peakDb(mo, SR / 2, SR), sd = peakDb(so, SR / 2, SR);
        snprintf(msg, sizeof msg, "mid/side: mid %.1f dB (in -30), side %.1f dB (in -6)", md, sd);
        CHECK(std::fabs(md + 30) < 0.3f && sd < -18, msg);
    }
    { // external sidechain ducks the main signal; internal ignores the sidechain input
        auto mainIn = sine(SR, 500, -20), key = sine(SR, 100, -3);
        Engine e = comp(kClean, -30, 10); e.p[kSidechain] = 1; e.reset(); Buf b = run(e, mainIn, mainIn, &key, &key);
        Engine f = comp(kClean, -30, 10); Buf c = run(f, mainIn, mainIn, &key, &key);
        const float duck = -20 - peakDb(b.l, SR / 2, SR), self = -20 - peakDb(c.l, SR / 2, SR);
        snprintf(msg, sizeof msg, "external sidechain: %.1f dB ducking from the key; internal mode: %.1f dB (own level only)", duck, self);
        CHECK(duck > 20 && std::fabs(self - 9) < 1.f, msg);
    }
    { // sidechain filters: a 50 Hz tone above threshold triggers nothing with the 48 dB high pass at 500 Hz
        Engine e = comp(kClean, -30, 10); e.p[kScHpOn] = 1; e.p[kScHpFreq] = 500; e.reset();
        Buf b = run(e, sine(SR, 50, -10)); const float gr = -10 - peakDb(b.l, SR / 2, SR);
        Engine f = comp(kClean, -30, 10); e.p[kScLpOn] = 1; f.p[kScLpOn] = 1; f.p[kScLpFreq] = 300; f.reset();
        Buf c = run(f, sine(SR, 5000, -10)); const float gr2 = -10 - peakDb(c.l, SR / 2, SR);
        snprintf(msg, sizeof msg, "sidechain HP 500 Hz vs 50 Hz tone: %.2f dB GR; LP 300 Hz vs 5 kHz tone: %.2f dB GR", gr, gr2);
        CHECK(gr < 0.5f && gr2 < 0.5f, msg);
        Engine g = comp(kClean, -30, 10); g.p[kScHpOn] = 1; g.p[kScHpFreq] = 2000; g.p[kScListen] = 1; g.reset();
        Buf d = run(g, sine(SR, 100, -6)); CHECK(peakDb(d.l, SR / 2, SR) < -60, "sidechain listen: you hear the filtered detector signal");
    }
    { // mix, makeup gain and auto gain
        Engine e = comp(kClean, -30, 10); e.p[kMix] = 0; e.reset(); auto x = sine(SR, 1000, -6); Buf b = run(e, x);
        float d = 0; for (int i = SR / 2; i < SR; ++i) d = std::max(d, std::fabs(b.l[i] - x[i])); CHECK(d < 1e-4f, "mix 0%: output equals input");
        Engine g = comp(kClean, -18, 4); g.p[kGain] = 6; g.reset(); Buf c = run(g, sine(SR, 1000, -6));
        snprintf(msg, sizeof msg, "makeup +6 dB: %.2f dB out (expect -9)", peakDb(c.l, SR / 2, SR)); CHECK(std::fabs(peakDb(c.l, SR / 2, SR) + 9) < 0.5f, msg);
        Engine a = comp(kClean, -24, 4); a.p[kAutoGain] = 1; a.reset(); Buf ab = run(a, sine(SR, 1000, -30));
        snprintf(msg, sizeof msg, "auto gain (thr -24, 4:1) below threshold: %+.2f dB (expect +9)", peakDb(ab.l, SR / 2, SR) + 30); CHECK(std::fabs(peakDb(ab.l, SR / 2, SR) + 21) < 0.3f, msg);
    }
    { // meters
        Engine e = comp(kClean, -18, 4); run(e, sine(SR, 1000, -6));
        snprintf(msg, sizeof msg, "meters: in %.1f dB, out %.1f dB, GR %.1f dB", e.meterIn(0), e.meterOut(0), e.meterGr(0));
        CHECK(std::fabs(e.meterIn(0) + 6) < 0.3f && std::fabs(e.meterOut(0) + 15) < 0.6f && std::fabs(e.meterGr(0) - 9) < 0.6f, msg);
    }
    printf("%s (%d failures)\n", fails ? "FAILED" : "ALL PASSED", fails);
    return fails ? 1 : 0;
}
