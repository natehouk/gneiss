// Gneiss — hover tips and the in-plugin help pages. License: MIT.
#pragma once
#include "GneissEngine.hpp"

namespace gneiss {

static const char* const kManualURL = "https://github.com/natehouk/gneiss/blob/main/docs/MANUAL.md";

inline const char* paramTip(uint32_t p) {
    switch (p) {
    case kStyle: return "CLEAN: precise feed-forward compression that follows peaks, good for transparent control. CLASSIC: a feedback design that eases into compression and releases musically; glues buses and drums. OPTO: slow, smooth and level-dependent; the release lengthens the harder it works, ideal for vocals and bass.";
    case kThreshold: return "Level above which compression starts. Lower it to compress more of the signal. You can also drag horizontally in the transfer curve or vertically in the level display.";
    case kRatio: return "How strongly level above the threshold is reduced. 2:1 is gentle, 4:1 is firm, 10:1 and up behaves like a limiter.";
    case kKnee: return "Width of the transition around the threshold. 0 dB is a hard knee (compression starts abruptly); wider knees start gently below the threshold for a more transparent sound.";
    case kAttack: return "How fast compression reacts once the level goes over the threshold. Short attack tames peaks; longer attack lets the transient through for punch.";
    case kRelease: return "How fast compression lets go after the level drops. Short release is loud and lively (and can pump); long release is smooth and even. In Classic and Opto it also adapts to the material.";
    case kGain: return "Make-up gain after compression, to bring the level back up. The meters and transfer curve help you match the loudness of the bypassed signal.";
    case kAutoGain: return "Adds make-up gain automatically from the threshold and ratio, so you can change them without the level jumping. Gain adds on top.";
    case kMix: return "Blend between the dry signal (0%) and the compressed signal (100%). In between gives parallel ('New York') compression: dense but with natural transients.";
    case kChannelMode: return "L/R compresses left and right. M/S compresses the mid (centre) and side (stereo) parts instead, for example to control a wide mix without touching the centre.";
    case kLink: return "How much the two channels share one gain reduction. 100% keeps the stereo image steady; lower values let each side (or mid and side) compress on its own.";
    case kSidechain: return "INTERNAL: the compressor listens to its own input. EXTERNAL: it listens to the sidechain input instead, for ducking (route a kick or a voice to Gneiss's sidechain in your DAW).";
    case kScHpOn: return "Filters low frequencies out of the detector, so bass and kick drum do not trigger as much compression. The audio itself is not filtered.";
    case kScHpFreq: return "Cutoff of the sidechain high pass (48 dB/oct). Raise it to make the compressor ignore more of the low end.";
    case kScLpOn: return "Filters high frequencies out of the detector, so cymbals and sibilance trigger less compression (or, with only highs left by the high pass, more).";
    case kScLpFreq: return "Cutoff of the sidechain low pass (48 dB/oct).";
    case kScListen: return "Hear the detector signal (after the sidechain filters) instead of the output, to tune the filters. Remember to switch it off!";
    case kExpert: return "Shows the stereo and sidechain options: channel mode, link, internal/external sidechain, sidechain filters and listen.";
    case kMidiOn: return "Turns MIDI control on or off. When off, learned controllers are ignored.";
    default: return "";
    }
}

enum TipTarget { kTipUndo, kTipRedo, kTipLearn, kTipMidi, kTipHelp, kTipA, kTipB, kTipCopy, kTipPrev, kTipPresets, kTipNext,
                 kTipLevelDisplay, kTipCurve, kTipMeters, kNumTipTargets };
inline const char* targetTitle(int t) {
    static const char* const n[kNumTipTargets] = { "Undo", "Redo", "MIDI Learn", "MIDI", "Help", "Setting A", "Setting B", "Copy A/B",
                                                   "Previous preset", "Presets", "Next preset", "Level display", "Transfer curve", "Meters" };
    return n[t];
}
inline const char* targetTip(int t) {
    switch (t) {
    case kTipUndo: return "Steps back through your edits, including preset loads and A/B switches. Cmd/Ctrl-Z.";
    case kTipRedo: return "Re-applies an edit you undid. Cmd/Ctrl-Shift-Z or Cmd/Ctrl-Y.";
    case kTipLearn: return "Click, then click any control and move a knob or fader on your MIDI controller to assign it. Right-click a control to unassign. Assignments are saved with your project.";
    case kTipMidi: return paramTip(kMidiOn);
    case kTipHelp: return "Opens the help pages: compression basics, styles, the displays, stereo and sidechain, A/B, MIDI Learn and shortcuts.";
    case kTipA: case kTipB: return "Gneiss keeps two complete settings, A and B. Click the other letter to switch and compare by ear; each keeps its own changes.";
    case kTipCopy: return "Copies the current setting to the other slot, so you can try a variation from the same starting point.";
    case kTipPrev: return "Load the previous preset. The mouse wheel over the preset name also steps through presets.";
    case kTipPresets: return "Open the preset browser: factory presets by category, your own presets, and Save Current As. A * after the name means you have changed it.";
    case kTipNext: return "Load the next preset.";
    case kTipLevelDisplay: return "The last few seconds: input level (grey), output level (colour) and gain reduction (red, from the top). The dashed line is the threshold; drag up and down to move it.";
    case kTipCurve: return "Input level (horizontal) against output level (vertical), before make-up gain. The dot shows the current input level. Drag left and right to move the threshold.";
    case kTipMeters: return "Peak input level, gain reduction (red, downwards) and output level. In M/S mode the gain reduction bars show mid and side.";
    default: return "";
    }
}

struct HelpPage { const char* title; const char* body; };
static const HelpPage kHelpPages[] = {
    { "Overview",
      "Gneiss is a stereo compressor: it turns down the parts of a signal that are louder than the threshold, by an amount set by the ratio. "
      "That evens out the level, adds density and punch, and lets you bring the whole signal up with the make-up gain.\n\n"
      "Pick a STYLE, set THRESHOLD and RATIO until the red gain reduction looks right, shape the response with ATTACK and RELEASE, "
      "then use GAIN (or AUTO GAIN) to match the level. Use A/B to compare settings.\n\n"
      "Hover over any control for a moment to see what it does. Turn the tips off at the bottom of this window." },
    { "Styles",
      "CLEAN follows the signal's peaks (feed-forward detection) for precise, transparent control with very fast attack available.\n\n"
      "CLASSIC is a feedback design: its detector hears the already-compressed signal, so it eases into compression and its release "
      "lengthens after sustained compression. It glues buses and drums.\n\n"
      "OPTO reacts to the average level with a wide, soft knee, and its release slows down the harder it works, like an optical cell. "
      "Smooth and forgiving on vocals, bass and anything that needs levelling." },
    { "The displays",
      "LEVEL DISPLAY (left): the last few seconds of input level (grey), output level (colour) and gain reduction (red, hanging from the top). "
      "The dashed line is the threshold; drag in the display to move it.\n\n"
      "TRANSFER CURVE (centre): input level against output level. Above the threshold the line bends by the ratio; the knee rounds the corner. "
      "The dot shows where the signal is right now. Drag left and right to move the threshold.\n\n"
      "METERS (right): input peaks, gain reduction and output peaks." },
    { "Stereo & sidechain",
      "Turn on EXPERT to see these options.\n\n"
      "CHANNEL MODE: L/R works on left and right; M/S works on mid and side instead.\n\n"
      "LINK: at 100% both channels get the same gain reduction (stable image); lower values let them compress independently.\n\n"
      "SIDECHAIN: EXTERNAL listens to the sidechain input instead of the audio, for ducking. In your DAW, send the trigger track "
      "(a kick, a voice-over) to Gneiss's sidechain input.\n\n"
      "SC HIGH PASS / LOW PASS: 48 dB/oct filters on the detector only. A high pass at 80-150 Hz stops bass from pumping a mix.\n\n"
      "LISTEN: hear the filtered detector signal while you tune the filters." },
    { "A/B & undo",
      "Gneiss holds two complete settings, A and B. Click B to switch to it (the first time, it starts as a copy of A); click A to go back. "
      "Each slot keeps its own changes. COPY copies the current setting into the other slot.\n\n"
      "Every edit (knob, button, preset load, A/B switch) is an undo step. Use UNDO/REDO in the header, or Cmd/Ctrl-Z and Cmd/Ctrl-Shift-Z." },
    { "MIDI Learn",
      "1. Click LEARN in the header.\n2. Click the control you want to play.\n3. Move a knob, fader or wheel on your MIDI controller. "
      "The control shows its CC number.\n\n"
      "Right-click a control in Learn mode to unassign it, or use CLEAR ALL ASSIGNMENTS. Click LEARN again (or press Esc) to finish.\n\n"
      "Assignments are saved with your project. Route MIDI from a track or controller to Gneiss in your DAW." },
    { "Shortcuts",
      "Double-click a knob: type a value (for example -24, 4, 0.5, 150).\n"
      "Cmd/Ctrl-click a knob: reset it to its default.\n"
      "Shift-drag: fine adjustment.   Grab a knob's rim: turn it in a circle.\n"
      "Mouse wheel over a knob: small steps (Shift for finer).\n"
      "Cmd/Ctrl-Z: undo.   Cmd/Ctrl-Shift-Z or Cmd/Ctrl-Y: redo.\n"
      "Esc: leave MIDI Learn, close this window or a text field.\n\n"
      "Some hosts keep keyboard shortcuts for themselves; the UNDO and REDO buttons always work." },
};
static constexpr int kNumHelpPages = sizeof(kHelpPages) / sizeof(kHelpPages[0]);

} // namespace gneiss
