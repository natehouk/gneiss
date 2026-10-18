// Run a drum-like loop through every factory preset (with a kick on the sidechain); report level and gain reduction.
#include "../plugins/Gneiss/ParamDefs.hpp"
#include "../plugins/Gneiss/Presets.hpp"
#include <cstdio>
#include <vector>
using namespace gneiss;
int main() {
    const int SR = 48000, N = SR * 3; int bad = 0;
    std::vector<float> x(N), kick(N); uint32_t r = 9;
    for (int i = 0; i < N; ++i) {
        r = r * 1664525u + 1013904223u; const float w = (int32_t)r * (1.f / 2147483648.f);
        const int beat = i % (SR / 2), off = (i + SR / 4) % (SR / 2);
        kick[i] = beat < 6000 ? 0.8f * std::exp(-beat / 1500.f) * std::sin(2 * kPi * (50 + 80 * std::exp(-beat / 400.f)) * beat / SR) : 0.f;
        const float snare = off < 4000 ? 0.4f * std::exp(-off / 900.f) * w : 0.f;
        x[i] = kick[i] + snare + 0.1f * std::sin(2 * kPi * 220 * i / (float)SR) + 0.03f * w;
    }
    double si = 0; for (float v : x) si += v * v; const float inR = (float)std::sqrt(si / N);
    for (uint32_t p = 0; p < kNumPresets; ++p) {
        Engine e; e.setSampleRate(SR); for (uint32_t k = 0; k < kParamCount; ++k) e.p[k] = presets()[p].v[k]; e.reset();
        std::vector<float> L(N), R(N); float grMax = 0;
        for (int i = 0; i < N; i += 512) { const int n = std::min(512, N - i); e.process(x.data() + i, x.data() + i, kick.data() + i, kick.data() + i, L.data() + i, R.data() + i, n); grMax = std::max(grMax, e.meterGr(0)); }
        double s = 0; bool nf = false; for (int i = 0; i < N; ++i) { s += L[i] * L[i]; nf |= !std::isfinite(L[i]); }
        const float rel = 20 * std::log10(std::sqrt(s / N) / inR + 1e-9f);
        const bool ok = !nf && rel > -9 && rel < 12 && (p == 0 || grMax > 1.f); bad += !ok;
        printf("%-18s %-13s level %+5.1f dB   max GR %5.1f dB %s\n", presets()[p].name, presets()[p].category, rel, grMax, ok ? "" : "<-- CHECK");
    }
    printf("%u presets, %d flagged\n", kNumPresets, bad); return bad;
}
