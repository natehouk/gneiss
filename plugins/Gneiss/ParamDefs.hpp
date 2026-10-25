// Parameter metadata shared by the DSP wrapper and the UI. License: MIT.
#pragma once
#include "GneissEngine.hpp"
#include <cstdio>

namespace gneiss {

static constexpr uint32_t kHintInteger = 1, kHintBoolean = 2, kHintLog = 4, kHintOutput = 8, kHintView = 16;   // view: a display setting, not saved in presets/undo
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
        set(kRatio, "Ratio", "ratio", ":1", 1, 100, kHintLog);
        set(kKnee, "Knee", "knee", "dB", 0, 72, 0);
        set(kAttack, "Attack", "attack", "ms", 0.01f, 250, kHintLog);
        set(kRelease, "Release", "release", "ms", 5, 2500, kHintLog);
        set(kGain, "Gain", "gain", "dB", -12, 36, 0);
        set(kAutoGain, "Auto Gain", "autogain", "", 0, 1, B);
        set(kMix, "Mix", "mix", "", 0, 1, 0);
        set(kChannelMode, "Channel Mode", "chmode", "", 0, kNumChannelModes - 1, I);
        set(kLink, "Stereo Link", "link", "", 0, 1, 0);
        set(kSidechain, "Sidechain", "sc", "", 0, 1, I);
        set(kScHpOn, "Sidechain High Pass", "schpon", "", 0, 1, B);
        set(kScHpFreq, "Sidechain HP Freq", "schp", "Hz", 20, 20000, kHintLog);
        set(kScLpOn, "Sidechain Low Pass", "sclpon", "", 0, 1, B);
        set(kScLpFreq, "Sidechain LP Freq", "sclp", "Hz", 20, 20000, kHintLog);
        set(kScListen, "Sidechain Listen", "sclisten", "", 0, 1, B);
        set(kExpert, "Expert Mode", "expert", "", 0, 1, B | kHintView);
        set(kMidiOn, "MIDI", "midion", "", 0, 1, B);
        set(kOutLearn, "MIDI Learn", "o_learn", "", 0, 10000000, O);
        set(kOutInL, "Input Level L", "o_inl", "dB", -120, 12, O);   set(kOutInR, "Input Level R", "o_inr", "dB", -120, 12, O);
        set(kOutOutL, "Output Level L", "o_outl", "dB", -120, 12, O); set(kOutOutR, "Output Level R", "o_outr", "dB", -120, 12, O);
        set(kOutGrL, "Gain Reduction 1", "o_grl", "dB", -60, 60, O);   set(kOutGrR, "Gain Reduction 2", "o_grr", "dB", -60, 60, O);
        // ---- 2.0
        set(kHold, "Hold", "hold", "ms", 0, 500, 0);
        set(kRange, "Range", "range", "dB", 0, 60, 0);
        set(kGrMix, "Gain Change Mix", "grmix", "%", 0, 200, 0);
        set(kLookahead, "Lookahead", "lookahead", "ms", 0, 20, 0);
        set(kOversampling, "Oversampling", "os", "", 0, kNumOversampling - 1, I);
        set(kAutoThreshold, "Auto Threshold", "autothr", "", 0, 1, B);
        set(kAutoRelease, "Auto Release", "autorel", "", 0, 1, B);
        set(kCharacter, "Character", "character", "", 0, epsilon::Character::kNumModes - 1, I);
        set(kDrive, "Drive", "drive", "%", 0, 100, 0);
        set(kCharPos, "Character Position", "charpos", "", 0, 1, I);
        set(kMidiTrigger, "MIDI Trigger", "miditrig", "", 0, 1, B);
        set(kPulse, "Pulse Trigger", "pulse", "", 0, 1, B);
        set(kPulseSync, "Pulse Sync", "pulsesync", "", 0, kNumPulseSyncs - 1, I);
        set(kPulseOffset, "Pulse Offset", "pulseoff", "%", 0, 100, 0);
        set(kPulseLength, "Pulse Length", "pulselen", "%", 0, 100, 0);
        set(kDelta, "Audition Gain Change", "delta", "", 0, 1, B);
        set(kScEqOn, "Sidechain EQ", "sceq", "", 0, 1, B);
        static char nm[kScBands][kScFields][32], sy[kScBands][kScFields][16];
        static const char* fn[kScFields] = { "Used", "Enabled", "Shape", "Frequency", "Gain", "Q", "Slope", "Channel" };
        static const char* fsy[kScFields] = { "used", "on", "shape", "freq", "gain", "q", "slope", "chan" };
        for (int b = 0; b < kScBands; ++b)
            for (int f = 0; f < kScFields; ++f) {
                std::snprintf(nm[b][f], 32, "SC EQ %d %s", b + 1, fn[f]); std::snprintf(sy[b][f], 16, "sc%d%s", b + 1, fsy[f]);
                const uint32_t i = sp(b, f);
                switch (f) {
                case kSbUsed: case kSbOn: set(i, nm[b][f], sy[b][f], "", 0, 1, B); break;
                case kSbShape: set(i, nm[b][f], sy[b][f], "", 0, eq::kNumShapes - 1, I); break;
                case kSbFreq: set(i, nm[b][f], sy[b][f], "Hz", 10, 30000, kHintLog); break;
                case kSbGain: set(i, nm[b][f], sy[b][f], "dB", -30, 30, 0); break;
                case kSbQ: set(i, nm[b][f], sy[b][f], "", 0.025f, 40, kHintLog); break;
                case kSbSlope: set(i, nm[b][f], sy[b][f], "", 0, 16, I); break;
                default: set(i, nm[b][f], sy[b][f], "", 0, 2, I); break;
                }
            }
        set(kOutLoudM, "Loudness Momentary", "o_lufsm", "LUFS", -70, 10, O);
        set(kOutLoudS, "Loudness Short-Term", "o_lufss", "LUFS", -70, 10, O);
        set(kOutLoudI, "Loudness Integrated", "o_lufsi", "LUFS", -70, 10, O);
        set(kOutTruePeak, "True Peak", "o_tp", "dBTP", -70, 20, O);
        set(kOutTrigger, "Trigger Level", "o_trig", "", 0, 1, O);
    }
    return t[i];
}

inline int scField(uint32_t p) { return (p >= kScFirst && p <= kScLast) ? (int)((p - kScFirst) % kScFields) : -1; }
inline int scBandOf(uint32_t p) { return (p >= kScFirst && p <= kScLast) ? (int)((p - kScFirst) / kScFields) : -1; }
inline bool isSetting(uint32_t p) { return p < kParamCount && !(paramDef(p).hints & (kHintOutput | kHintView)); }

inline const char* const* enumLabels(uint32_t p, int& count) {
    static const char* const style[] = { "Clean", "Classic", "Opto", "Vocal", "Mastering", "Bus", "Punch", "Pumping", "Versatile", "Smooth", "Upward", "TTM", "Vari-Mu", "El-Op" };
    static const char* const chm[] = { "Left/Right", "Mid/Side", "Mid Only", "Side Only", "Mid to Side", "Side to Mid" };
    static const char* const sc[] = { "Internal", "External" };
    static const char* const ovs[] = { "Off", "2x", "4x", "8x", "16x", "32x" };
    static const char* const chr[] = { "Off", "Warm", "Punchy", "Vintage" };
    static const char* const pos[] = { "Post", "Pre" };
    static const char* const sync[] = { "1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/4 dotted", "1/8 dotted", "1/4 triplet", "1/8 triplet" };
    static const char* const shape[] = { "Bell", "Low Shelf", "High Shelf", "Low Cut", "High Cut", "Notch", "Band Pass", "Tilt Shelf", "Flat Tilt", "All Pass" };
    static const char* const slope[] = { "6 dB/oct", "12 dB/oct", "24 dB/oct", "48 dB/oct", "18 dB/oct", "30 dB/oct", "36 dB/oct", "42 dB/oct", "54 dB/oct",
                                         "60 dB/oct", "66 dB/oct", "72 dB/oct", "78 dB/oct", "84 dB/oct", "90 dB/oct", "96 dB/oct", "Brickwall" };
    static const char* const place[] = { "Stereo", "Mid", "Side" };
    switch (p) {
    case kStyle: count = kNumStyles; return style;
    case kChannelMode: count = kNumChannelModes; return chm;
    case kSidechain: count = 2; return sc;
    case kOversampling: count = kNumOversampling; return ovs;
    case kCharacter: count = 4; return chr;
    case kCharPos: count = 2; return pos;
    case kPulseSync: count = kNumPulseSyncs; return sync;
    default: break;
    }
    switch (scField(p)) {
    case kSbShape: count = 10; return shape;
    case kSbSlope: count = 17; return slope;
    case kSbPlace: count = 3; return place;
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
