// 1.x compatibility: random 1.x settings (2.0 parameters at their defaults) through the frozen 1.0.1 engine and
// the 2.0 engine, internal and external sidechain, 44.1-96 kHz: the outputs must match.
#include "../plugins/Gneiss/ParamDefs.hpp"
#include "legacy/GneissEngine1.hpp"
#include <cstdio>
#include <random>
#include <vector>
using namespace gneiss;
int main() {
    std::mt19937 g(5); std::uniform_real_distribution<float> u(0, 1);
    int bad = 0; float worstAll = -1000;
    for (int sr : { 44100, 48000, 96000 }) for (int trial = 0; trial < 30; ++trial) {
        Engine e; gneiss_old::Engine o;
        for (uint32_t i = 0; i <= kMidiOn; ++i) {
            float lo, hi; bool integer = false, logs = false;
            switch (i) {
            case kStyle: lo = 0; hi = 2; integer = true; break;
            case kThreshold: lo = -60; hi = 0; break;
            case kRatio: lo = 1; hi = 20; logs = true; break;
            case kKnee: lo = 0; hi = 24; break;
            case kAttack: lo = 0.01f; hi = 250; logs = true; break;
            case kRelease: lo = 5; hi = 2500; logs = true; break;
            case kGain: lo = -12; hi = 36; break;
            case kMix: case kLink: lo = 0; hi = 1; break;
            case kChannelMode: case kSidechain: case kAutoGain: case kScHpOn: case kScLpOn: case kScListen: case kExpert: case kMidiOn: lo = 0; hi = 1; integer = true; break;
            case kScHpFreq: case kScLpFreq: lo = 20; hi = 20000; logs = true; break;
            default: lo = 0; hi = 1;
            }
            float v = logs ? lo * std::pow(hi / lo, u(g)) : lo + (hi - lo) * u(g);
            if (integer) v = std::round(v);
            if (i == kScListen && u(g) < 0.8f) v = 0;
            e.p[i] = v; o.p[i] = v;
        }
        e.setSampleRate((float)sr); o.setSampleRate((float)sr);
        const int N = sr; std::vector<float> l(N), r(N), k(N), L1(N), R1(N), L2(N), R2(N);
        for (int i = 0; i < N; ++i) { const float env = (i / (sr / 4)) % 2 ? 0.8f : 0.05f; l[i] = env * (2 * u(g) - 1); r[i] = 0.7f * l[i] + 0.1f * (2 * u(g) - 1); k[i] = ((i / (sr / 8)) % 2) * 0.5f * std::sin(0.03f * i); }
        for (int i = 0; i < N; i += 333) {
            const int n = std::min(333, N - i);
            if (i >= N / 2 && i < N / 2 + 333) { e.p[kThreshold] -= 6; o.p[gneiss_old::kThreshold] -= 6; }
            o.process(l.data() + i, r.data() + i, k.data() + i, k.data() + i, L1.data() + i, R1.data() + i, n);
            e.process(l.data() + i, r.data() + i, k.data() + i, k.data() + i, L2.data() + i, R2.data() + i, n);
        }
        float worst = 0; for (int i = 0; i < N; ++i) worst = std::max(worst, std::max(std::fabs(L1[i] - L2[i]), std::fabs(R1[i] - R2[i])));
        const float rel = 20 * std::log10(worst / 0.8f + 1e-12f);
        worstAll = std::max(worstAll, rel);
        if (rel > -80) { ++bad; printf("FAIL sr %d trial %d style %.0f chm %.0f: difference %.1f dB below the input\n", sr, trial, e.p[kStyle], e.p[kChannelMode], -rel); }
    }
    printf("1.x compatibility: largest difference %.1f dB below the input level, %d failures\n", -worstAll, bad);
    return bad;
}
