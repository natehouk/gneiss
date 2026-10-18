// Factory presets (each lists only what it changes from the engine defaults). License: MIT.
// Engine defaults: Clean, threshold -18 dB, 4:1, 6 dB knee, 10 ms attack, 150 ms release, linked stereo.
#pragma once
#include "GneissEngine.hpp"
#include <algorithm>

namespace gneiss {

struct PV { uint32_t id; float v; };
struct PresetDef { const char* name; const char* category; const PV* pv; };
struct Preset { const char* name; const char* category; float v[kParamCount]; };

static const char* const kCategories[] = { "Vocals", "Drums", "Bass & Keys", "Bus & Master", "Creative" };
static constexpr int kNumCategories = 5;

#define C(st, th, ra, kn, at, re, ga) {kStyle,st},{kThreshold,th},{kRatio,ra},{kKnee,kn},{kAttack,at},{kRelease,re},{kGain,ga}
#define END { kParamCount, 0 }
namespace pv {
static const PV init[] = { END };
// ---------------------------------------------------------------- Vocals
static const PV vocalGentle[] = { C(2,-22,2.5f,12,15,250,3), END };
static const PV vocalPresent[]= { C(1,-24,4,8,5,120,5), END };
static const PV vocalLevel[]  = { C(2,-28,3,18,30,600,6), END };
static const PV vocalAggro[]  = { C(1,-30,8,4,1.5f,60,9), END };
static const PV deEssLike[]   = { C(0,-30,6,4,0.5f,40,0), {kScHpOn,1},{kScHpFreq,5000},{kExpert,1}, END };
static const PV rapVocal[]    = { C(0,-26,6,6,3,80,7), END };
// ---------------------------------------------------------------- Drums
static const PV kickPunch[]   = { C(0,-20,4,4,25,80,3), END };
static const PV snareSnap[]   = { C(0,-24,5,3,15,60,4), END };
static const PV drumBus[]     = { C(1,-18,3,6,20,150,3), END };
static const PV smash[]       = { C(0,-40,20,0,0.1f,40,18), END };
static const PV parallelDrums[]={ C(0,-40,12,2,1,60,14), {kMix,.4f}, END };
static const PV roomPump[]    = { C(1,-36,10,2,2,250,20), END };
// ---------------------------------------------------------------- Bass & Keys
static const PV bassEven[]    = { C(2,-24,4,10,20,200,4), END };
static const PV bassTight[]   = { C(0,-22,6,4,5,80,4), END };
static const PV synthBass[]   = { C(1,-20,4,6,10,100,3), {kScHpOn,1},{kScHpFreq,60},{kExpert,1}, END };
static const PV pianoSmooth[] = { C(2,-26,2.5f,12,25,400,3), END };
static const PV guitarSustain[]={ C(1,-34,8,6,8,600,18), END };
static const PV acoustic[]    = { C(2,-24,3,12,12,250,3), END };
// ---------------------------------------------------------------- Bus & Master
static const PV glue[]        = { C(1,-16,2,6,30,200,1.5f), END };
static const PV mixBusGentle[]= { C(2,-14,1.5f,12,30,300,1), END };
static const PV masterLevel[] = { C(0,-14,2,8,30,200,2), END };
static const PV wideBus[]     = { C(1,-18,2.5f,8,20,180,2), {kLink,.6f}, END };
static const PV msControl[]   = { C(0,-20,3,8,15,150,1), {kChannelMode,1},{kLink,0}, END };
static const PV lowGlue[]     = { C(1,-18,2,6,30,200,1.5f), {kScHpOn,1},{kScHpFreq,120},{kExpert,1}, END };
// ---------------------------------------------------------------- Creative
static const PV duck[]        = { C(0,-30,6,4,1,180,2), {kSidechain,1},{kExpert,1}, END };
static const PV pump[]        = { C(1,-36,20,0,0.5f,300,6), {kSidechain,1},{kScLpOn,1},{kScLpFreq,150},{kExpert,1}, END };
static const PV newYork[]     = { C(0,-36,10,2,0.5f,100,12), {kMix,.35f}, END };
static const PV squash[]      = { C(2,-45,20,6,10,800,20), END };
static const PV sustainer[]   = { C(2,-50,8,12,5,1500,22), END };
static const PV lofi[]        = { C(0,-38,20,0,0.05f,15,16), END };
} // namespace pv
#undef C
#undef END

static const PresetDef kPresetDefs[] = {
    {"Init", "Init", pv::init},
    {"Gentle Vocal", "Vocals", pv::vocalGentle}, {"Present Vocal", "Vocals", pv::vocalPresent}, {"Vocal Leveler", "Vocals", pv::vocalLevel},
    {"Aggressive Vocal", "Vocals", pv::vocalAggro}, {"Sibilance Tamer", "Vocals", pv::deEssLike}, {"Rap Vocal", "Vocals", pv::rapVocal},
    {"Kick Punch", "Drums", pv::kickPunch}, {"Snare Snap", "Drums", pv::snareSnap}, {"Drum Bus", "Drums", pv::drumBus},
    {"Smash", "Drums", pv::smash}, {"Parallel Drums", "Drums", pv::parallelDrums}, {"Room Pump", "Drums", pv::roomPump},
    {"Even Bass", "Bass & Keys", pv::bassEven}, {"Tight Bass", "Bass & Keys", pv::bassTight}, {"Synth Bass", "Bass & Keys", pv::synthBass},
    {"Smooth Piano", "Bass & Keys", pv::pianoSmooth}, {"Guitar Sustain", "Bass & Keys", pv::guitarSustain}, {"Acoustic", "Bass & Keys", pv::acoustic},
    {"Glue", "Bus & Master", pv::glue}, {"Gentle Mix Bus", "Bus & Master", pv::mixBusGentle}, {"Master Leveler", "Bus & Master", pv::masterLevel},
    {"Wide Bus", "Bus & Master", pv::wideBus}, {"Mid/Side Control", "Bus & Master", pv::msControl}, {"Low-End Glue", "Bus & Master", pv::lowGlue},
    {"Sidechain Duck", "Creative", pv::duck}, {"Kick Pump", "Creative", pv::pump}, {"New York", "Creative", pv::newYork},
    {"Squash", "Creative", pv::squash}, {"Sustainer", "Creative", pv::sustainer}, {"Lo-Fi Crush", "Creative", pv::lofi},
};
static constexpr uint32_t kNumPresets = sizeof(kPresetDefs) / sizeof(kPresetDefs[0]);

inline const Preset* presets() {
    static Preset table[kNumPresets];
    static bool done = false;
    if (done) return table;
    done = true;
    Engine e;
    for (uint32_t i = 0; i < kNumPresets; ++i) {
        table[i].name = kPresetDefs[i].name; table[i].category = kPresetDefs[i].category;
        std::copy(e.p, e.p + kParamCount, table[i].v);
        for (const PV* x = kPresetDefs[i].pv; x->id != kParamCount; ++x) table[i].v[x->id] = x->v;
    }
    return table;
}

} // namespace gneiss
