// Gneiss 2.0 behaviour: the eleven new styles, upward/TTM/vari-mu shapes, hold, range, gain-change mix,
// lookahead, oversampling, auto threshold, auto release, character, sidechain EQ, MIDI and pulse triggering,
// gain-change audition, the four new stereo modes, knee 72 / ratio infinity, loudness metering.
#include "../plugins/Gneiss/ParamDefs.hpp"
#include <cstdio>
#include <vector>
using namespace gneiss;
static int fails = 0;
#define CHECK(c, ...) do { bool ok_ = (c); char m_[260]; snprintf(m_, sizeof m_, __VA_ARGS__); printf("%s  %s\n", ok_ ? "PASS" : "FAIL", m_); if (!ok_) fails++; } while (0)
static const int SR = 48000;
struct Buf { std::vector<float> l, r; };
static Buf run(Engine& e, const std::vector<float>& xl, const std::vector<float>& xr, const std::vector<float>* sc = nullptr, int block = 256) {
    Buf b; b.l.resize(xl.size()); b.r.resize(xl.size());
    for (size_t i = 0; i < xl.size(); i += block) { const uint32_t n = (uint32_t)std::min<size_t>(block, xl.size() - i);
        e.process(xl.data() + i, xr.data() + i, sc ? sc->data() + i : nullptr, sc ? sc->data() + i : nullptr, b.l.data() + i, b.r.data() + i, n); }
    return b;
}
static Buf run(Engine& e, const std::vector<float>& x) { return run(e, x, x); }
static float peakDb(const std::vector<float>& v, size_t a, size_t z) { float m = 0; for (size_t i = a; i < z; ++i) m = std::max(m, std::fabs(v[i])); return gainToDb(m); }
static std::vector<float> sine(int n, float f, float db) { std::vector<float> v(n); const float a = dbToGain(db); for (int i = 0; i < n; ++i) v[i] = a * std::sin(2 * kPi * f * i / SR); return v; }
static Engine comp(int style, float thr, float ratio, float knee = 0, float atk = 1, float rel = 50) {
    Engine e; e.setSampleRate(SR); e.p[kStyle] = style; e.p[kThreshold] = thr; e.p[kRatio] = ratio; e.p[kKnee] = knee; e.p[kAttack] = atk; e.p[kRelease] = rel; e.reset(); return e;
}
static float outFor(Engine e, float inDb, float f = 1000) { Buf b = run(e, sine(SR, f, inDb)); return peakDb(b.l, SR / 2, SR); }

int main() {
    const char* names[kNumStyles] = { "Clean", "Classic", "Opto", "Vocal", "Mastering", "Bus", "Punch", "Pumping", "Versatile", "Smooth", "Upward", "TTM", "Vari-Mu", "El-Op" };
    // every downward style compresses a loud tone by a sensible amount and leaves a quiet one alone
    for (int st = kVocal; st < kNumStyles; ++st) {
        if (st == kUpward) continue;
        const float loud = outFor(comp(st, -18, 4, 0, 1, 50), -6), quiet = outFor(comp(st, -18, 4, 0, 1, 50), -40);
        const bool ttm = st == kTTM;
        CHECK(loud < -10 && loud > -19 && (ttm ? quiet > -40 : std::fabs(quiet + 40) < 0.3f), "%-9s -6 dB in -> %.1f dB out; -40 dB in -> %.1f dB out", names[st], loud, quiet);
    }
    { // upward: quiet parts come up toward the threshold, loud ones are untouched; range limits the boost
        const float q = outFor(comp(kUpward, -20, 2, 0, 1, 50), -40), l = outFor(comp(kUpward, -20, 2, 0, 1, 50), -6);
        Engine r = comp(kUpward, -20, 2, 0, 1, 50); r.p[kRange] = 4; const float qr = outFor(r, -40);
        CHECK(std::fabs(q + 30) < 1.f && std::fabs(l + 6) < 0.2f && std::fabs(qr + 36) < 0.5f, "upward 2:1 thr -20: -40 dB in -> %.1f (want -30), -6 -> %.1f, range 4 dB -> %.1f", q, l, qr);
        const float noise = outFor(comp(kUpward, -20, 2, 0, 1, 50), -85);
        CHECK(std::fabs(noise + 85) < 0.5f, "upward leaves very quiet noise alone (-85 -> %.1f)", noise);
    }
    { // vari-mu: the ratio grows with level
        Engine v = comp(kVariMu, -30, 8, 0, 1, 50); v.p[kKnee] = 0;
        const float gr6 = -24 - outFor(v, -24), gr24 = -6 - outFor(v, -6);
        CHECK(gr6 / 6.f < 0.85f * gr24 / 24.f, "vari-mu: %.2f dB GR per dB over at +6, %.2f at +24", gr6 / 6.f, gr24 / 24.f);
    }
    { // knee 72 and ratio infinity
        Engine k = comp(kClean, -30, 4, 72); const float gr = -30 - outFor(k, -30);
        CHECK(std::fabs(gr - 0.75f * 72 / 8) < 0.5f, "72 dB knee at the threshold: %.2f dB GR (ideal %.2f)", gr, 0.75f * 72 / 8);
        Engine li = comp(kClean, -20, 100, 0, 0.01f, 50); const float o = outFor(li, 0);
        CHECK(std::fabs(o + 20) < 0.3f, "ratio infinity: 0 dB in -> %.2f dB out (threshold -20)", o);
    }
    { // range, gain-change mix
        Engine r = comp(kClean, -30, 10); r.p[kRange] = 6; const float o = outFor(r, -6);
        CHECK(std::fabs(o + 12) < 0.3f, "range 6 dB: -6 in -> %.2f out", o);
        Engine m0 = comp(kClean, -30, 10); m0.p[kGrMix] = 0; Engine m2 = comp(kClean, -30, 10); m2.p[kGrMix] = 200; Engine m1 = comp(kClean, -30, 10);
        const float g1 = -6 - outFor(m1, -6), g2 = -6 - outFor(m2, -6), g0 = -6 - outFor(m0, -6);
        CHECK(std::fabs(g0) < 0.05f && std::fabs(g2 - 2 * g1) < 0.3f, "gain-change mix: 0%% -> %.2f dB, 100%% -> %.2f dB, 200%% -> %.2f dB", g0, g1, g2);
    }
    { // hold keeps the gain reduction after the signal drops
        auto grAfter = [](float holdMs, float ms) {
            Engine e = comp(kClean, -30, 20, 0, 1, 20); e.p[kHold] = holdMs;
            std::vector<float> x(SR, 0.f); for (int i = 0; i < SR; ++i) x[i] = i < SR / 2 ? 0.5f : 0.01f;
            Buf b = run(e, x, x, nullptr, 48); const int at = SR / 2 + (int)(ms * 48);
            return gainToDb(x[at]) - gainToDb(std::fabs(b.l[at])); };
        CHECK(grAfter(100, 60) > 20 && grAfter(0, 60) < 2, "hold 100 ms: %.1f dB GR 60 ms after the drop (no hold: %.1f dB)", grAfter(100, 60), grAfter(0, 60));
    }
    { // lookahead: reported latency, and a step is caught before it passes (no overshoot)
        Engine a = comp(kClean, -20, 100, 0, 0.5f, 100); a.p[kLookahead] = 5; a.reset();
        Engine b = comp(kClean, -20, 100, 0, 0.5f, 100);
        CHECK(a.latency() == 240 && b.latency() == 0, "lookahead 5 ms: latency %u samples", a.latency());
        std::vector<float> x(SR, 0.f); for (int i = SR / 4; i < SR; ++i) x[i] = 0.9f * std::sin(2 * kPi * 200 * i / SR);
        Buf ba = run(a, x), bb = run(b, x);
        const float pa = peakDb(ba.l, 0, SR), pb = peakDb(bb.l, 0, SR);
        CHECK(pa < pb - 3 && pa < -15, "step into a limiter: peak %.1f dB with lookahead, %.1f dB without", pa, pb);
    }
    { // oversampling: latency per factor, transparent below threshold (aligned), compression the same
        for (int o = 1; o < kNumOversampling; ++o) {
            Engine e = comp(kClean, -18, 4); e.p[kOversampling] = o; e.reset();
            const int lat = (int)e.latency();
            auto x = sine(SR, 1000, -30); Buf b = run(e, x);
            float d = 0; for (int i = SR / 2; i < SR; ++i) d = std::max(d, std::fabs(b.l[i] - x[i - lat]));
            const float loud = outFor(e, -6);
            CHECK(d < 2e-3f && std::fabs(loud + 15) < 0.6f, "oversampling %dx: latency %d, below-threshold error %.5f, -6 dB in -> %.2f dB", 1 << o, lat, d, loud);
        }
    }
    { // auto threshold: the amount of compression no longer depends on the input level
        auto grAt = [](float lvl, bool autoThr) { Engine e = comp(kClean, -24, 4, 6, 5, 100); e.p[kAutoThreshold] = autoThr; e.reset();
            Buf b = run(e, sine(SR * 6, 1000, lvl)); return lvl - peakDb(b.l, SR * 5, SR * 6); };
        const float a1 = grAt(-6, true), a2 = grAt(-20, true), m1 = grAt(-6, false), m2 = grAt(-20, false);
        CHECK(std::fabs(a1 - a2) < 1.f && std::fabs(m1 - m2) > 8, "auto threshold: GR %.1f / %.1f dB at -6 / -20 dB in (fixed threshold: %.1f / %.1f)", a1, a2, m1, m2);
    }
    { // auto release: quicker after a short peak than after sustained compression
        auto relTime = [](bool autoRel, int burstMs) {
            Engine e = comp(kClean, -30, 10, 0, 1, 300); e.p[kAutoRelease] = autoRel; e.reset();
            const int on = burstMs * 48, N = on + SR; std::vector<float> x(N); for (int i = 0; i < N; ++i) x[i] = i < on ? 0.5f : 0.003f;
            Buf b = run(e, x, x, nullptr, 48);
            const float full = gainToDb(x[on - 1]) - gainToDb(std::fabs(b.l[on - 1]));
            for (int i = on; i < N; ++i) if (gainToDb(x[i]) - gainToDb(std::fabs(b.l[i])) < 0.368f * full) return (i - on) / 48.f;
            return 1000.f; };
        const float shortA = relTime(true, 20), longA = relTime(true, 3000), shortF = relTime(false, 20);
        CHECK(shortA < 0.6f * longA && shortA < shortF, "auto release: %.0f ms after a 20 ms peak, %.0f ms after 3 s (fixed: %.0f ms)", shortA, longA, shortF);
    }
    { // character: off is bit-transparent at ratio 1; warm adds harmonics; pre and post differ
        auto thd = [](Engine e) { auto x = sine(SR, 1000, -6); Buf b = run(e, x); const int M = SR / 2; double re = 0, im = 0, tot = 0; const int lat = (int)e.latency();
            for (int i = SR / 2; i < SR; ++i) { const double ph = 2 * 3.14159265358979 * 1000.0 * (i - lat) / SR; re += b.l[i] * std::cos(ph); im += b.l[i] * std::sin(ph); tot += (double)b.l[i] * b.l[i]; }
            const double a = 2 * std::sqrt(re * re + im * im) / M; return (float)(10 * std::log10(std::max(1e-20, tot / M - a * a / 2) / (a * a / 2))); };
        Engine off = comp(kClean, -18, 4); Engine warm = comp(kClean, -18, 4); warm.p[kCharacter] = 1; warm.p[kDrive] = 80;
        Engine pre = warm; pre.p[kCharPos] = kCharPre;
        const float t0 = thd(off), t1 = thd(warm), t2 = thd(pre);
        CHECK(t1 > t0 + 15 && std::fabs(t1 - t2) > 1, "character: THD off %.0f dB, warm post %.0f dB, warm pre %.0f dB", t0, t1, t2);
        Engine os = comp(kClean, -18, 4); os.p[kCharacter] = 2; os.p[kDrive] = 100; os.p[kOversampling] = 3; os.reset();
        Buf b = run(os, sine(SR, 9000, -3)); bool ok = true; for (float v : b.l) ok &= std::isfinite(v);
        CHECK(ok, "punchy character at 8x oversampling stays finite");
    }
    { // sidechain EQ: a +12 dB bell at 100 Hz makes a 100 Hz tone compress harder; a side-only band ignores the mid
        Engine flat = comp(kClean, -24, 4, 0); Engine eqd = comp(kClean, -24, 4, 0);
        eqd.p[sp(0, kSbUsed)] = 1; eqd.p[sp(0, kSbFreq)] = 100; eqd.p[sp(0, kSbGain)] = 12; eqd.p[sp(0, kSbQ)] = 1;
        const float g0 = -18 - outFor(flat, -18, 100), g1 = -18 - outFor(eqd, -18, 100);
        CHECK(g1 > g0 + 8, "sidechain EQ +12 dB at 100 Hz: GR %.1f dB (flat sidechain %.1f dB)", g1, g0);
        Engine side = comp(kClean, -24, 4, 0); side.p[sp(0, kSbUsed)] = 1; side.p[sp(0, kSbShape)] = eq::kHighCut; side.p[sp(0, kSbFreq)] = 20; side.p[sp(0, kSbSlope)] = 3; side.p[sp(0, kSbPlace)] = 2;
        const float gs = -18 - outFor(side, -18, 1000);     // mono signal: all mid; the band only cuts the side
        CHECK(std::fabs(gs - g0) < 0.3f, "side-only sidechain band leaves a mono key alone (GR %.1f vs %.1f dB)", gs, g0);
        Engine cut = comp(kClean, -24, 4, 0); cut.p[sp(0, kSbUsed)] = 1; cut.p[sp(0, kSbShape)] = eq::kHighCut; cut.p[sp(0, kSbFreq)] = 20; cut.p[sp(0, kSbSlope)] = 3;
        CHECK(-18 - outFor(cut, -18, 1000) < 0.3f, "a stereo high cut at 20 Hz on the sidechain stops the compression");
    }
    { // MIDI trigger: a held note compresses a quiet signal
        Engine e = comp(kClean, -20, 10, 0, 1, 50); e.p[kMidiTrigger] = 1; e.reset();
        auto x = sine(SR, 500, -30); e.noteOn(127); Buf b = run(e, x); const float held = -30 - peakDb(b.l, SR / 2, SR);
        e.noteOff(); Buf c = run(e, x); const float rel = -30 - peakDb(c.l, SR / 2, SR);
        CHECK(held > 15 && rel < 0.3f, "MIDI trigger: %.1f dB GR while a note is held, %.1f dB after", held, rel);
    }
    { // pulse trigger: 1/4 notes at 120 BPM, 25% long -> the level dips twice a second
        Engine e = comp(kClean, -20, 10, 0, 0.5f, 20); e.p[kPulse] = 1; e.p[kPulseSync] = 2; e.p[kPulseLength] = 25; e.reset();
        auto x = sine(SR * 2, 500, -30); Buf b; b.l.resize(x.size()); b.r.resize(x.size());
        for (int i = 0; i < SR * 2; i += 512) {     // the host reports the transport position with every block
            const int n = std::min(512, SR * 2 - i); e.setTransport(true, i / (double)SR * 2.0, 120.0);
            e.process(x.data() + i, x.data() + i, nullptr, nullptr, b.l.data() + i, b.r.data() + i, (uint32_t)n); }
        int dips = 0; bool in = false;
        for (int i = 0; i < SR * 2; i += 240) { float m = 0; for (int k = i; k < i + 240; ++k) m = std::max(m, std::fabs(b.l[k])); const bool d = gainToDb(m) < -40; if (d && !in) ++dips; in = d; }
        CHECK(dips == 4, "pulse trigger: %d dips in 2 s at 120 BPM, 1/4 (want 4)", dips);
    }
    { // audition of the gain change: you hear what is taken away
        Engine e = comp(kClean, -30, 10); e.p[kDelta] = 1; e.reset(); Engine f = comp(kClean, -30, 10);
        auto x = sine(SR, 1000, -6); Buf d = run(e, x), y = run(f, x);
        float err = 0; for (int i = SR / 2; i < SR; ++i) err = std::max(err, std::fabs(d.l[i] - (x[i] - y.l[i])));
        CHECK(err < 1e-4f, "audition: output = input - compressed (error %.6f)", err);
    }
    { // stereo modes on a mid tone + side tone
        auto msRun = [](int mode, float midDb, float sideDb) {
            Engine e = comp(kClean, -24, 10); e.p[kChannelMode] = mode; e.p[kLink] = 0; e.reset();
            auto m = sine(SR, 300, midDb), s = sine(SR, 2000, sideDb);
            std::vector<float> L(SR), R(SR); for (int i = 0; i < SR; ++i) { L[i] = m[i] + s[i]; R[i] = m[i] - s[i]; }
            Buf b = run(e, L, R); std::vector<float> mo(SR), so(SR);
            for (int i = 0; i < SR; ++i) { mo[i] = 0.5f * (b.l[i] + b.r[i]); so[i] = 0.5f * (b.l[i] - b.r[i]); }
            return std::make_pair(midDb - peakDb(mo, SR / 2, SR), sideDb - peakDb(so, SR / 2, SR)); };
        auto a = msRun(kMidOnly, -6, -6), b = msRun(kSideOnly, -6, -6), c = msRun(kMidToSide, -6, -40), d = msRun(kSideToMid, -40, -6);
        CHECK(a.first > 10 && a.second < 0.3f, "mid only: mid GR %.1f, side %.1f", a.first, a.second);
        CHECK(b.second > 10 && b.first < 0.3f, "side only: side GR %.1f, mid %.1f", b.second, b.first);
        CHECK(c.second > 10 && c.first < 0.3f, "mid to side (loud mid, quiet side): side GR %.1f, mid %.1f", c.second, c.first);
        CHECK(d.first > 10 && d.second < 0.3f, "side to mid (loud side, quiet mid): mid GR %.1f, side %.1f", d.first, d.second);
    }
    { // loudness of the output
        Engine e = comp(kClean, -18, 1); auto x = sine(SR * 5, 1000, -23); run(e, x);
        CHECK(std::fabs(e.loudness().integrated() + 23) < 0.1f && std::fabs(e.loudness().momentary() + 23) < 0.1f,
              "loudness: 1 kHz at -23 dBFS reads %.2f LUFS integrated, %.2f momentary", e.loudness().integrated(), e.loudness().momentary());
    }
    printf("%s (%d failures)\n", fails ? "FAILED" : "ALL PASSED", fails);
    return fails;
}
