// Gneiss — hover tips and the in-plugin help pages. License: MIT.
#pragma once
#include "GneissEngine.hpp"
#include "common/UiKit.hpp"

namespace gneiss {

static const char* const kManualURL = "https://github.com/natehouk/gneiss/blob/main/docs/MANUAL.md";

inline const char* styleTip(int s) {
    switch (s) {
    case kClean: return "CLEAN: precise feed-forward compression that follows peaks. Transparent; very fast attack available.";
    case kClassic: return "CLASSIC: a feedback design. It eases into compression, and its release lengthens after sustained compression. Glues buses and drums.";
    case kOpto: return "OPTO: reacts to the average level with a wide, soft knee. Its release slows down the harder it works, like an optical cell. Smooth on vocals and bass.";
    case kVocal: return "VOCAL: gentle on quiet words and firmer on loud ones, with a wide knee. Keeps a voice even without squashing it.";
    case kMastering: return "MASTERING: slow, averaged detection, a soft knee and longer times. Transparent control of a whole mix.";
    case kBus: return "BUS: a feedback design with a medium knee and a release that adapts to the material. Glue for groups and mix buses.";
    case kPunch: return "PUNCH: lets the start of each hit through before clamping down, so drums get punchier, not flatter.";
    case kPumping: return "PUMPING: releases at a steady rate, so the level swells back audibly after each hit. The classic pumping and breathing effect.";
    case kVersatile: return "VERSATILE: a balanced all-rounder between Clean and Classic. A good first choice for anything.";
    case kSmooth: return "SMOOTH: averaged detection, a soft knee and gentle timing. Levels without drawing attention to itself.";
    case kUpward: return "UPWARD: brings quiet parts up toward the threshold instead of pushing loud parts down; loud parts are untouched. Brings out detail and room sound. RANGE limits the boost (24 dB at most when RANGE is at its maximum).";
    case kTTM: return "TTM (To The Max): compresses loud parts down and brings quiet parts up at the same time, fast. A dense, in-your-face sound.";
    case kVariMu: return "VARI-MU: the ratio grows with the level: barely compressing just over the threshold, firm on peaks. Warm and gentle, like a variable-mu tube compressor.";
    case kElOp: return "EL-OP: an electro-optical style with a very soft knee and a two-stage release that slows down after sustained compression. Smooth and natural.";
    default: return "";
    }
}

inline const char* paramTip(uint32_t p) {
    switch (p) {
    case kStyle: return "The compressor's character: how it detects, how its knee and ratio behave and how it times attack and release. Hover a style for details.";
    case kThreshold: return "Level above which compression starts (below which, in Upward style, the boost starts). Lower it to compress more. You can also drag in the level display or the transfer curve.";
    case kRatio: return "How strongly level above the threshold is reduced. 2:1 is gentle, 4:1 firm, 10:1 and up is limiting; all the way up is infinity (a hard limit at the threshold).";
    case kKnee: return "Width of the transition around the threshold, from 0 dB (hard) to 72 dB. Very wide knees with a fast attack start to saturate.";
    case kAttack: return "How fast compression reacts once the level goes over the threshold. Short attack tames peaks; longer attack lets the transient through for punch.";
    case kRelease: return "How fast compression lets go after the level drops. Short is loud and lively (and can pump); long is smooth. Several styles also adapt it to the material.";
    case kGain: return "Make-up gain after compression, to bring the level back up.";
    case kAutoGain: return "Adds make-up gain automatically from the threshold and ratio, so you can change them without the level jumping. Gain adds on top.";
    case kMix: return "DRY/WET: blend between the dry signal (0%) and the compressed signal (100%), for parallel ('New York') compression.";
    case kChannelMode: return "L/R: left and right. M/S: mid and side, each compressed. MID or SIDE only: compress one, leave the other. MID>SIDE: the mid's level compresses the side. SIDE>MID: the side's level compresses the mid.";
    case kLink: return "How much the two channels share one gain reduction. 100% keeps the stereo image steady; lower values let each side compress on its own.";
    case kSidechain: return "INTERNAL: the compressor listens to its own input. EXTERNAL: it listens to the sidechain input instead, for ducking (route a kick or a voice to Gneiss's sidechain).";
    case kScHpOn: return "A 48 dB/oct high pass on the detector, so bass and kick trigger less compression. The audio itself is not filtered.";
    case kScHpFreq: return "Cutoff of the 48 dB/oct sidechain high pass.";
    case kScLpOn: return "A 48 dB/oct low pass on the detector, so cymbals and sibilance trigger less compression.";
    case kScLpFreq: return "Cutoff of the 48 dB/oct sidechain low pass.";
    case kScListen: return "AUDITION TRIGGER: hear what the compressor listens to (after the sidechain EQ and filters) instead of the output. Remember to switch it off!";
    case kExpert: return "Shows the stereo, sidechain, trigger and character options.";
    case kMidiOn: return "Turns MIDI on or off: learned controllers and MIDI triggering.";
    case kHold: return "Keeps the gain reduction for this long after the level drops, before the release starts. Stops fast releases from fluttering on low notes.";
    case kRange: return "The most the compressor may change the gain, in dB. At its maximum (60 dB) there is no limit. Lower values keep some of the dynamics however hard it works.";
    case kGrMix: return "Scales the gain change: 50% applies half the compression, 200% doubles it. Unlike dry/wet, it keeps everything in phase and works the same at any setting.";
    case kLookahead: return "Lets the compressor see up to 20 ms ahead, so it is already working when a peak arrives. Adds that much latency, which your DAW compensates.";
    case kOversampling: return "Runs the gain stage and the character at up to 32 times the sample rate, which avoids aliasing from very fast gain changes and saturation. Adds a little latency and CPU.";
    case kAutoThreshold: return "Makes the threshold relative to the material's level: the compressor keeps doing the same amount of work whether the input is loud or quiet. Great for dialogue and varying sources.";
    case kAutoRelease: return "Adapts the release to the material: fast after short peaks, slower during sustained compression. The RELEASE knob sets the overall speed.";
    case kCharacter: return "Analog-style colour. WARM: tube-like, mostly even harmonics. PUNCHY: rounds peaks, odd harmonics. VINTAGE: warm with a gentle tone and slow analog drift. OFF is perfectly clean.";
    case kDrive: return "How hard the character stage is pushed. It changes the colour, not the level.";
    case kCharPos: return "POST colours the compressed signal (smooth, controlled). PRE colours the signal before compression (the compressor then tames the extra harmonics).";
    case kMidiTrigger: return "MIDI notes trigger compression: while a note is held, the compressor acts as if the key were at the note's velocity (127 = 0 dBFS).";
    case kPulse: return "Triggers compression in time with your song, without a sidechain track: a pulse every bar, beat or division. Classic sidechain pumping, locked to the tempo.";
    case kPulseSync: return "How often the pulse repeats.";
    case kPulseOffset: return "Moves the pulse later within each period (as a percentage of the period), to line it up with your kick or to make off-beat pumping.";
    case kPulseLength: return "How long each pulse lasts, as a percentage of the period. Longer pulses hold the compression down for longer.";
    case kDelta: return "AUDITION GAIN CHANGE: hear only what the compressor takes away. A quick way to hear where and how much it works.";
    case kScEqOn: return "Switches the 6-band sidechain EQ on or off. It shapes what the compressor listens to, never the audio itself.";
    default: break;
    }
    switch (scField(p)) {
    case kSbShape: return "Shape of this sidechain EQ band: bell, shelves, cuts, notch, band pass, tilts or all pass, as in Mica.";
    case kSbFreq: return "Frequency of this sidechain EQ band. Drag its dot in the sidechain EQ display.";
    case kSbGain: return "Boost to make the compressor react more to these frequencies, cut to make it react less.";
    case kSbQ: return "Width of this sidechain EQ band. Mouse wheel over its dot.";
    case kSbSlope: return "Steepness of this sidechain EQ band (shelves, cuts, band pass).";
    case kSbPlace: return "Whether this band shapes the whole sidechain, only its mid or only its side.";
    case kSbOn: return "Switches this sidechain EQ band in or out.";
    default: return "";
    }
}

enum TipTarget { kTipUndo, kTipRedo, kTipLearn, kTipMidi, kTipHelp, kTipA, kTipB, kTipCopy, kTipPrev, kTipPresets, kTipNext,
                 kTipLevelDisplay, kTipCurve, kTipMeters, kTipLoudness, kTipTabs, kTipScEq, kTipScDot, kTipTrigger, kNumTipTargets };
inline const char* targetTitle(int t) {
    static const char* const n[kNumTipTargets] = { "Undo", "Redo", "MIDI Learn", "MIDI", "Help", "Setting A", "Setting B", "Copy A/B",
                                                   "Previous preset", "Presets", "Next preset", "Level display", "Transfer curve", "Meters",
                                                   "Loudness", "Panels", "Sidechain EQ", "Sidechain EQ band", "Trigger" };
    return n[t];
}
inline const char* targetTip(int t) {
    switch (t) {
    case kTipUndo: return "Steps back through your edits, including preset loads and A/B switches. Cmd/Ctrl-Z.";
    case kTipRedo: return "Re-applies an edit you undid. Cmd/Ctrl-Shift-Z or Cmd/Ctrl-Y.";
    case kTipLearn: return "Click, then click any control and move a knob or fader on your MIDI controller to assign it. Right-click a control to unassign. Assignments are saved with your project.";
    case kTipMidi: return paramTip(kMidiOn);
    case kTipHelp: return "Opens the help pages: basics, styles, displays and meters, stereo, sidechain, triggering, character, A/B, MIDI Learn and shortcuts.";
    case kTipA: case kTipB: return "Gneiss keeps two complete settings, A and B. Click the other letter to switch and compare by ear.";
    case kTipCopy: return "Copies the current setting to the other slot.";
    case kTipPrev: return "Load the previous preset. The mouse wheel over the preset name also steps through presets.";
    case kTipPresets: return "Open the preset browser. A * after the name means you have changed it.";
    case kTipNext: return "Load the next preset.";
    case kTipLevelDisplay: return "The last few seconds: input level (grey), output level (colour), gain reduction (red, from the top) and upward boost (green). The dashed line is the threshold; drag to move it.";
    case kTipCurve: return "Input level (horizontal) against output level (vertical), before make-up gain. The dot shows the current input level. Drag left and right to move the threshold.";
    case kTipMeters: return "Peak input, gain change (red when reducing, green when boosting) and output.";
    case kTipLoudness: return "Output loudness (EBU R128): M momentary (400 ms), S short-term (3 s), I integrated since the last reset, and TP the highest true peak. Click to reset I and TP.";
    case kTipTabs: return "STEREO: channel modes, link, dry/wet and oversampling. SIDECHAIN: external input, the sidechain EQ and filters, auditions. TRIGGER: MIDI and tempo pulses. CHARACTER: analog colour.";
    case kTipScEq: return "What the compressor listens to. Double-click to add a band (up to 6), drag a dot for frequency and gain, mouse wheel for Q, right-click to remove. The audio is never filtered.";
    case kTipScDot: return "Drag for frequency and gain, mouse wheel for Q. Right-click removes it. Its shape, slope and channel are below the display.";
    case kTipTrigger: return "Lights while a MIDI note or a tempo pulse is triggering the compressor.";
    default: return "";
    }
}

static const epsilon::HelpPage kHelpPages[] = {
    { "Overview",
      "Gneiss is a stereo compressor: it turns down the parts of a signal that are louder than the threshold, by an amount set by the ratio. "
      "That evens out the level, adds density and punch, and lets you bring the whole signal up with the make-up gain.\n\n"
      "Pick a STYLE, set THRESHOLD and RATIO until the red gain reduction looks right, shape the response with ATTACK and RELEASE, "
      "then use GAIN (or AUTO GAIN) to match the level.\n\n"
      "The panels on the right hold the stereo, sidechain, trigger and character options. Hover over anything for a moment to see what it does." },
    { "Styles",
      "Fourteen styles: CLEAN, CLASSIC, OPTO, VOCAL, MASTERING, BUS, PUNCH, PUMPING, VERSATILE, SMOOTH, UPWARD, TTM, VARI-MU and EL-OP. "
      "They differ in how they detect level, how their knee and ratio behave and how they time attack and release. Hover a style for a description.\n\n"
      "UPWARD brings quiet parts up instead of loud parts down; TTM does both at once. VARI-MU's ratio grows with level." },
    { "Controls",
      "KNEE: 0 to 72 dB. RANGE: the most the gain may change (60 dB = no limit). HOLD: keep the gain reduction before releasing.\n\n"
      "MIX scales the gain change from 0 to 200%; DRY/WET (STEREO panel) blends the dry signal for parallel compression.\n\n"
      "LOOKAHEAD: see up to 20 ms ahead (adds latency). AUTO THRESHOLD makes the threshold follow the material's level; AUTO RELEASE adapts the release." },
    { "Displays & meters",
      "LEVEL DISPLAY: input (grey), output (colour), gain reduction (red) and upward boost (green) over the last few seconds; drag to set the threshold.\n\n"
      "TRANSFER CURVE: input against output; the dot is the current level.\n\n"
      "METERS: input, gain change and output peaks, and output LOUDNESS (EBU R128): momentary, short-term, integrated and true peak. Click the loudness readout to reset it." },
    { "Stereo & character",
      "CHANNELS: L/R, M/S, MID or SIDE only, MID>SIDE (the mid's level compresses the side) and SIDE>MID. LINK shares the gain between channels.\n\n"
      "OVERSAMPLING up to 32x avoids aliasing from fast gain changes and saturation.\n\n"
      "CHARACTER: WARM, PUNCHY or VINTAGE analog colour, with DRIVE, before (PRE) or after (POST) compression." },
    { "Sidechain & triggers",
      "SIDECHAIN EXTERNAL listens to the sidechain input instead of the audio, for ducking.\n\n"
      "SIDECHAIN EQ: up to 6 bands shape what the compressor listens to: double-click to add, drag to move, wheel for Q. Each band can work on the sidechain's mid or side only. "
      "The 48 dB/oct high and low pass filters are there too.\n\n"
      "AUDITION: TRIGGER lets you hear what the compressor listens to; GAIN CHANGE lets you hear what it takes away.\n\n"
      "TRIGGER: MIDI notes, or tempo-synced PULSES (sync, offset and length), can drive the compressor without any sidechain audio." },
    { "A/B, MIDI & shortcuts",
      "A/B: two complete settings; COPY copies the current one to the other. Every edit is an undo step (Cmd/Ctrl-Z, Cmd/Ctrl-Shift-Z).\n\n"
      "MIDI LEARN: click LEARN, click a control, move a controller. Right-click unassigns.\n\n"
      "Double-click a knob to type a value. Cmd/Ctrl-click resets it. Shift-drag is fine adjustment; grab a knob's rim to turn it in a circle. Esc closes this window." },
};
static constexpr int kNumHelpPages = sizeof(kHelpPages) / sizeof(kHelpPages[0]);

} // namespace gneiss
