// CPU: stereo compressor on noise at 48 kHz: 1.x settings, then the heavier 2.0 options.
#include "../plugins/Gneiss/ParamDefs.hpp"
#include <chrono>
#include <cstdio>
using namespace gneiss;
int main() {
    const int SR = 48000; static float in[SR], sc[SR], L[SR], R[SR]; uint32_t r = 1;
    for (int i = 0; i < SR; ++i) { r = r * 1664525u + 1013904223u; in[i] = 0.5f * ((int32_t)r * (1.f / 2147483648.f)); sc[i] = in[(i * 7) % SR]; }
    struct Cfg { const char* name; int os; int chr; bool eq; float look; } cfg[] = {
        { "Classic, 1.x settings", 0, 0, false, 0 }, { "+ lookahead 5 ms, 6-band sidechain EQ", 0, 0, true, 5 },
        { "+ Warm character, 4x oversampling", 2, 1, true, 5 }, { "+ Vintage character, 32x oversampling", 5, 3, true, 5 } };
    for (const Cfg& c : cfg) {
        Engine e; e.setSampleRate(SR); e.p[kStyle] = kClassic; e.p[kThreshold] = -24; e.p[kOversampling] = c.os; e.p[kCharacter] = c.chr; e.p[kLookahead] = c.look;
        if (c.eq) for (int b = 0; b < kScBands; ++b) { e.p[sp(b, kSbUsed)] = 1; e.p[sp(b, kSbGain)] = 3; }
        e.reset();
        const int secs = 5; auto t0 = std::chrono::steady_clock::now();
        for (int k = 0; k < secs; ++k) for (int b = 0; b < SR; b += 256) e.process(in + b, in + b, sc + b, sc + b, L + b, R + b, 256);
        const double el = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        printf("%-42s %.2f%% of one core\n", c.name, 100 * el / secs);
    }
    return 0;
}
