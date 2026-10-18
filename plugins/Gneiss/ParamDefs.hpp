// Parameter metadata shared by the DSP wrapper and the UI. License: MIT.
#pragma once
#include "GneissEngine.hpp"
#include <cstdio>

namespace gneiss {

static constexpr uint32_t kHintInteger = 1, kHintBoolean = 2, kHintLog = 4, kHintOutput = 8;
struct ParamDef { const char* name; const char* sym; const char* unit; float min, max; uint32_t hints; };

inline const ParamDef& paramDef(uint32_t i) {
    static ParamDef t[kParamCount];
    static bool done = false;
    if (!done) {
        done = true;
        auto set = [](uint32_t i, const char* n, const char* s, const char* u, float lo, float hi, uint32_t h) { t[i] = { n, s, u, lo, hi, h }; };
        const uint32_t I = kHintInteger, B = kHintBoolean | kHintInteger, O = kHintOutput;
        set(kStyle, "Style", "style", "", 0, kNumStyles - 1, I);
        set(kThreshold, "Threshold", "thresh", "dB", -60, 0, 0);
        set(kRatio, "Ratio", "ratio", ":1", 1, 20, kHintLog);
        set(kKnee, "Knee", "knee", "dB", 0, 24, 0);
        set(kAttack, "Attack", "attack", "ms", 0.01f, 250, kHintLog);
        set(kRelease, "Release", "release", "ms", 5, 2500, kHintLog);
        set(kGain, "Gain", "gain", "dB", -12, 36, 0);
        set(kAutoGain, "Auto Gain", "autogain", "", 0, 1, B);
        set(kMix, "Mix", "mix", "", 0, 1, 0);
        set(kChannelMode, "Channel Mode", "chmode", "", 0, 1, I);
        set(kLink, "Stereo Link", "link", "", 0, 1, 0);
        set(kSidechain, "Sidechain", "sc", "", 0, 1, I);
        set(kScHpOn, "Sidechain High Pass", "schpon", "", 0, 1, B);
        set(kScHpFreq, "Sidechain HP Freq", "schp", "Hz", 20, 20000, kHintLog);
        set(kScLpOn, "Sidechain Low Pass", "sclpon", "", 0, 1, B);
        set(kScLpFreq, "Sidechain LP Freq", "sclp", "Hz", 20, 20000, kHintLog);
        set(kScListen, "Sidechain Listen", "sclisten", "", 0, 1, B);
        set(kExpert, "Expert Mode", "expert", "", 0, 1, B);
        set(kMidiOn, "MIDI", "midion", "", 0, 1, B);
        set(kOutLearn, "MIDI Learn", "o_learn", "", 0, 100000, O);
        set(kOutInL, "Input Level L", "o_inl", "dB", -120, 12, O);   set(kOutInR, "Input Level R", "o_inr", "dB", -120, 12, O);
        set(kOutOutL, "Output Level L", "o_outl", "dB", -120, 12, O); set(kOutOutR, "Output Level R", "o_outr", "dB", -120, 12, O);
        set(kOutGrL, "Gain Reduction 1", "o_grl", "dB", 0, 60, O);    set(kOutGrR, "Gain Reduction 2", "o_grr", "dB", 0, 60, O);
    }
    return t[i];
}

inline const char* const* enumLabels(uint32_t p, int& count) {
    static const char* const style[] = { "Clean", "Classic", "Opto" };
    static const char* const chm[] = { "Left/Right", "Mid/Side" };
    static const char* const sc[] = { "Internal", "External" };
    switch (p) {
    case kStyle: count = kNumStyles; return style;
    case kChannelMode: count = 2; return chm;
    case kSidechain: count = 2; return sc;
    default: count = 0; return nullptr;
    }
}

inline float toNorm(uint32_t p, float v) {
    const ParamDef& d = paramDef(p);
    if (d.hints & kHintLog) return std::log(v / d.min) / std::log(d.max / d.min);
    return (v - d.min) / (d.max - d.min);
}
inline float fromNorm(uint32_t p, float n) {
    const ParamDef& d = paramDef(p);
    n = clampf(n, 0.f, 1.f);
    float v = (d.hints & kHintLog) ? d.min * std::pow(d.max / d.min, n) : d.min + n * (d.max - d.min);
    if (d.hints & kHintInteger) v = std::round(v);
    return v;
}

} // namespace gneiss
