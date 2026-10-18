// Random parameters (with and without an external sidechain) at 44.1-192 kHz, loud noise: output finite.
#include "../plugins/Gneiss/ParamDefs.hpp"
#include <cstdio>
#include <random>
using namespace gneiss;
int main() {
    std::mt19937 g(5); std::uniform_real_distribution<float> u(0, 1); int bad = 0; float pk = 0;
    for (int sr : { 44100, 48000, 96000, 192000 }) {
        Engine e; e.setSampleRate((float)sr);
        for (int it = 0; it < 3000; ++it) {
            for (uint32_t i = 0; i < kParamCount; ++i) {
                const ParamDef& d = paramDef(i); if ((d.hints & kHintOutput) || u(g) < 0.5f) continue;
                float v = (u(g) < .3f) ? (u(g) < .5f ? d.min : d.max) : d.min + (d.max - d.min) * u(g);
                if (d.hints & kHintInteger) v = std::round(v); e.p[i] = v;
            }
            float in[300], sc[300], L[300], R[300]; const float amp = u(g) < .2f ? 0 : (u(g) < .5f ? 4.f : 0.5f);
            for (int k = 0; k < 300; ++k) { in[k] = amp * (2 * u(g) - 1); sc[k] = 2 * u(g) - 1; }
            e.process(in, in, u(g) < .5f ? sc : nullptr, sc, L, R, 300);
            for (int k = 0; k < 300; ++k) { if (!std::isfinite(L[k]) || !std::isfinite(R[k])) bad++; pk = std::max(pk, std::max(std::fabs(L[k]), std::fabs(R[k]))); }
        }
    }
    printf("fuzz nonfinite=%d peak=%.1f (gain up to +36 dB allowed)\n", bad, pk); return bad;
}
