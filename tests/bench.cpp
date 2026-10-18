// CPU: Opto (RMS detector), mid/side, both 48 dB sidechain filters, external sidechain.
#include "../plugins/Gneiss/ParamDefs.hpp"
#include <chrono>
#include <cstdio>
using namespace gneiss;
int main() {
    const int SR = 48000; Engine e; e.setSampleRate(SR);
    e.p[kStyle] = kOpto; e.p[kChannelMode] = 1; e.p[kScHpOn] = 1; e.p[kScLpOn] = 1; e.p[kSidechain] = 1; e.p[kLink] = 0.5f;
    static float in[SR], sc[SR], L[SR], R[SR]; uint32_t r = 1;
    for (int i = 0; i < SR; ++i) { r = r * 1664525u + 1013904223u; in[i] = 0.3f * ((int32_t)r * (1.f / 2147483648.f)); sc[i] = in[(i * 7) % SR]; }
    const int secs = 20; auto t0 = std::chrono::steady_clock::now();
    for (int k = 0; k < secs; ++k) for (int b = 0; b < SR; b += 256) e.process(in + b, in + b, sc + b, sc + b, L + b, R + b, 256);
    const double el = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    printf("%d s of audio in %.3f s: %.1f%% of one core\n", secs, el, 100 * el / secs); return 0;
}
