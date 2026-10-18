# Gneiss 1.0 — User Manual

Gneiss is a stereo compressor with three styles (Clean, Classic and Opto), a soft knee, left/right or mid/side processing with channel linking, an external sidechain with 48 dB/oct detector filters, auto gain, parallel mix, A/B comparison, undo/redo and MIDI Learn. It runs as VST3, AU, CLAP and VST2 on macOS, Windows and Linux, with no added latency.

**Contents**

1. [Installing](#1-installing)
2. [A quick tour](#2-a-quick-tour)
3. [Compression in two minutes](#3-compression-in-two-minutes)
4. [The styles](#4-the-styles)
5. [The main controls](#5-the-main-controls)
6. [The displays and meters](#6-the-displays-and-meters)
7. [Expert: stereo and sidechain](#7-expert-stereo-and-sidechain)
8. [Using an external sidechain in your DAW](#8-using-an-external-sidechain-in-your-daw)
9. [A/B, undo and redo](#9-ab-undo-and-redo)
10. [MIDI Learn](#10-midi-learn)
11. [Presets](#11-presets)
12. [Help and hover tips](#12-help-and-hover-tips)
13. [Recipes](#13-recipes)
14. [Keyboard and mouse reference](#14-keyboard-and-mouse-reference)
15. [Parameter reference](#15-parameter-reference)
16. [Troubleshooting](#16-troubleshooting)

---

## 1. Installing

Download the zip for your system from the project's **Releases** page and copy the plugin files:

| System | Copy | To |
|---|---|---|
| macOS | `Gneiss.vst3`, `Gneiss.component` (AU), `Gneiss.clap` | `~/Library/Audio/Plug-Ins/VST3`, `.../Components`, `.../CLAP` |
| Windows | the `Gneiss.vst3` folder, `Gneiss.clap` | `C:\Program Files\Common Files\VST3\`, `...\CLAP\` |
| Linux | `Gneiss.vst3`, `Gneiss.clap` | `~/.vst3/`, `~/.clap/` |

**macOS:** the builds are signed ad hoc but not notarized. If macOS blocks them, open Terminal and run:

```
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/*/Gneiss*
```

Rescan plugins in your DAW. Gneiss appears under **Epsilon Audio** as a dynamics effect.

---

## 2. A quick tour

![Gneiss](../screenshot.png)

- **Header**: the preset browser, **A / B / COPY**, **UNDO / REDO**, **LEARN**, **MIDI** and **HELP**.
- **Level display** (left): the last few seconds of input, output and gain reduction, with the threshold as a dashed line.
- **Transfer curve** (centre): how input level maps to output level.
- **Meters** (right): input, gain reduction and output.
- **Main panel**: style, auto gain and the seven main knobs.
- **Expert panel**: stereo and sidechain options (turn on **EXPERT** to see them).

Hover over anything for a moment and a tip explains it.

---

## 3. Compression in two minutes

A compressor turns down whatever is louder than the **threshold**. The **ratio** says by how much: at 4:1, a signal that goes 8 dB over the threshold comes out only 2 dB over it, so the compressor has applied 6 dB of **gain reduction**. **Attack** and **release** set how quickly that reduction starts and stops. Because the loud parts are now quieter, you can raise everything with **make-up gain** — the result is more even, denser and often louder.

A good starting routine:

1. Pick a **style**.
2. Lower the **threshold** until the red gain reduction moves by a few dB on the loudest parts.
3. Adjust **ratio**: higher for firmer control.
4. Shape the feel with **attack** (longer = more punch) and **release** (shorter = livelier, longer = smoother).
5. Raise **gain** (or turn on **auto gain**) until the level matches the bypassed sound, and compare with **A/B**.

---

## 4. The styles

| Style | How it works | Character | Try it on |
|---|---|---|---|
| **Clean** | Feed-forward: the detector follows the signal's peaks directly. | Precise and transparent, attack down to 0.01 ms. | Peaks, transients, anything that needs control without colour. |
| **Classic** | Feedback: the detector hears the already-compressed signal, so it eases into compression. The release lengthens after sustained compression. | Smooth, musical, "glue". | Drum buses, mix buses, guitars. |
| **Opto** | Detects the average (RMS) level with a wider knee; the release slows the harder it works, like an optical cell. | Slow, gentle, forgiving levelling. | Vocals, bass, acoustic instruments. |

Classic adds 2 dB and Opto 6 dB to the knee you set, which the transfer curve shows.

---

## 5. The main controls

**Threshold** (−60 to 0 dB): level where compression begins. You can also drag the dashed line in the level display (up/down) or drag in the transfer curve (left/right).

**Ratio** (1:1 to 20:1): strength of compression above the threshold. 1.5–2:1 is gentle, 3–4:1 is firm, 8:1 and above approaches limiting.

**Knee** (0 to 24 dB): 0 is a hard knee (compression starts abruptly at the threshold); wider knees ease in below it.

**Attack** (0.01 to 250 ms): time to react once the level goes over the threshold. Under 1 ms catches the sharpest peaks; 10–30 ms lets drum transients through for punch.

**Release** (5 ms to 2.5 s): time to recover after the level drops. Short releases sound loud and lively but can pump; long ones are smooth. Classic and Opto adapt their release to the material.

**Gain** (−12 to +36 dB): make-up gain after compression.

**Auto Gain**: adds make-up gain from the threshold and ratio (half the gain reduction a full-scale signal would get), so changing them doesn't change the level as much. **Gain** adds on top.

**Mix** (0 to 100%): blend of the dry signal with the compressed signal. Values in between give parallel ("New York") compression — dense, but with natural transients.

---

## 6. The displays and meters

**Level display.** Time runs from left to right over about ten seconds.
- **Grey area**: input level.
- **Coloured line**: output level (after gain).
- **Red area hanging from the top**: gain reduction (0–30 dB over the top half).
- **Dashed line**: the threshold. Drag anywhere in the display to move it.

**Transfer curve.** Input level (horizontal) against output level (vertical), before make-up gain. The faint diagonal is "no change". Above the threshold the curve bends by the ratio; the knee rounds the corner. The dot shows the current input level on the curve. Drag left and right to set the threshold.

**Meters.** IN: input peaks (left, right). GR: gain reduction for each channel (or mid and side), hanging down, with the current maximum below. OUT: output peaks. Peaks turn red at 0 dBFS.

---

## 7. Expert: stereo and sidechain

Turn on **EXPERT** to show these options. With Expert off, the panel summarises the current settings.

**Channels: L/R or M/S.** In **L/R** the two channels are left and right. In **M/S** Gneiss compresses the **mid** (what is common to both sides — usually vocals, kick, bass) and the **side** (the stereo difference — reverb, wide synths, room) separately. The gain reduction meters then show mid and side.

**Link** (0–100%): how much the two channels share one gain reduction.
- **100%**: both get the same reduction, so the stereo image never shifts.
- **0%**: each compresses on its own — useful in M/S to treat mid and side independently.

**Sidechain: Internal or External.** Internal listens to the audio being compressed. External listens to the sidechain input instead (see section 8). If External is selected but nothing is routed to the sidechain, the compressor hears silence and does nothing.

**SC High Pass / SC Low Pass** (48 dB/oct, 20 Hz–20 kHz): filter the detector signal only — the audio is not filtered.
- A high pass at 80–150 Hz stops the kick and bass from pumping a whole mix.
- A high pass at 4–7 kHz makes Gneiss react mainly to sibilance (a simple de-esser, see Recipes).

**Listen**: hear the detector signal after the filters instead of the output, to tune them. The status line turns red while Listen is on; remember to switch it off.

---

## 8. Using an external sidechain in your DAW

Gneiss has a stereo sidechain input in VST3, AU and CLAP. To use it:

1. Put Gneiss on the track you want to duck or control (for example a bass or pad).
2. In Gneiss, turn on **EXPERT** and select **EXTERNAL**.
3. Route the trigger (for example a kick drum) to Gneiss's sidechain:
   - **Ableton Live**: in Gneiss's device, open the sidechain section or choose the kick track as Audio From in the plugin's input chooser.
   - **Logic Pro**: choose the trigger track in the plugin window's **Side Chain** menu.
   - **Bitwig**: use the plugin's sidechain input selector.
   - **Reaper**: send the trigger track to channels 3/4 of the Gneiss track and enable them as the plugin's sidechain pins.
   - **Cubase / Studio One**: activate the plugin's sidechain and add a send from the trigger track to it.
4. Lower the threshold until the bass ducks on each kick; set release to taste (100–300 ms for classic pumping).

---

## 9. A/B, undo and redo

**A/B**: Gneiss holds two complete settings. Click **B** to switch to it — the first time, B starts as a copy of A. Click **A** to go back. Each slot keeps its own changes, so you can tweak both and compare by ear. **COPY** copies the current slot into the other one.

**Undo / redo**: every edit made in Gneiss's window is one step — a knob drag, a button, a typed value, a preset load or an A/B switch. Use the header buttons (hover them to see how many steps are available) or **Cmd/Ctrl-Z** and **Cmd/Ctrl-Shift-Z** (or **Cmd/Ctrl-Y**). Changes from host automation or MIDI are not undo steps.

---

## 10. MIDI Learn

1. Route a MIDI track or controller to Gneiss in your DAW.
2. Click **LEARN**.
3. Click the control you want to play.
4. Move a knob or fader on your controller. The control shows its **CC** number.
5. Click **LEARN** again (or press Esc) when you are done.

Right-click a control in Learn mode to unassign it; **CLEAR ALL ASSIGNMENTS** (bottom right of the level display) removes them all. Assignments are saved with your project. The **MIDI** button turns MIDI control off without forgetting them.

---

## 11. Presets

Click the preset name to open the browser.

| Category | Inside |
|---|---|
| **Vocals** | Gentle, present, levelling, aggressive, sibilance tamer, rap |
| **Drums** | Kick punch, snare snap, drum bus, smash, parallel, room pump |
| **Bass & Keys** | Even and tight bass, synth bass, piano, guitar sustain, acoustic |
| **Bus & Master** | Glue, gentle mix bus, master leveller, wide bus, mid/side control, low-end glue |
| **Creative** | Sidechain duck, kick pump, New York, squash, sustainer, lo-fi crush |

The *Sidechain Duck* and *Kick Pump* presets use the external sidechain: route a trigger to Gneiss (section 8).

- The arrows (or the mouse wheel over the name) step through presets.
- A **\*** after the name means you have changed it.
- **SAVE CURRENT AS...** saves your own preset; right-click a user preset, then click again, to delete it.

| System | User preset folder |
|---|---|
| macOS | `~/Library/Audio/Presets/Epsilon Audio/Gneiss/` |
| Windows | `%APPDATA%\Epsilon Audio\Gneiss\Presets\` |
| Linux | `~/.config/epsilon-audio/gneiss/presets/` |

---

## 12. Help and hover tips

- **Hover tips**: rest the pointer on any control, display, meter or header button and a tip explains it, with the current value and any MIDI CC.
- **HELP** opens short help pages. The **HOVER TIPS** switch turns tips on or off; **FULL MANUAL (WEB)** opens this manual.

---

## 13. Recipes

**Vocal leveller.** Opto, ratio 3:1, knee 12+ dB, attack 20–30 ms, release 400–600 ms. Lower the threshold until the meter shows 3–6 dB of reduction on loud phrases; add gain to match.

**Punchy drums.** Clean, ratio 4:1, attack 20–30 ms (transients get through), release 60–100 ms. 4–8 dB of reduction.

**Mix bus glue.** Classic, ratio 2:1, attack 30 ms, release 150–250 ms, SC high pass at 100 Hz. Aim for 1–3 dB of reduction.

**New York (parallel) compression.** Clean, ratio 10:1, threshold low, attack under 1 ms, release 100 ms, lots of gain, then Mix around 30–40%.

**Sidechain ducking.** Clean, ratio 6:1, attack 1 ms, release 150–250 ms, Expert → External, kick routed to the sidechain (section 8).

**Simple de-esser.** Clean, ratio 6:1, attack 0.5 ms, release 40 ms, SC high pass at 5–7 kHz. Use Listen to find the harshest band, then lower the threshold until only the "s" sounds trigger it.

**Mid/side bus control.** M/S, link 0%, ratio 3:1: the side channel can be held in check without touching the centre.

---

## 14. Keyboard and mouse reference

| Action | Result |
|---|---|
| Drag knob middle up/down | Adjust (Shift = fine) |
| Grab knob rim and turn | Set by angle |
| Double-click knob | Type a value |
| Cmd/Ctrl-click knob | Reset to default |
| Mouse wheel on knob | Small steps (Shift = smaller) |
| Drag in level display | Set threshold (vertical) |
| Drag in transfer curve | Set threshold (horizontal) |
| Cmd/Ctrl-Z | Undo |
| Cmd/Ctrl-Shift-Z, Cmd/Ctrl-Y | Redo |
| Mouse wheel on preset name | Previous / next preset |
| Right-click (Learn mode) | Remove MIDI assignment |
| Esc | Leave Learn mode, close Help, cancel typing |

---

## 15. Parameter reference

| Parameter | Range | Default |
|---|---|---|
| Style | Clean, Classic, Opto | Clean |
| Threshold | −60 – 0 dB | −18 dB |
| Ratio | 1:1 – 20:1 | 4:1 |
| Knee | 0 – 24 dB | 6 dB |
| Attack | 0.01 – 250 ms | 10 ms |
| Release | 5 – 2500 ms | 150 ms |
| Gain | −12 – +36 dB | 0 dB |
| Auto Gain | Off, On | Off |
| Mix | 0 – 100% | 100% |
| Channel Mode | Left/Right, Mid/Side | Left/Right |
| Stereo Link | 0 – 100% | 100% |
| Sidechain | Internal, External | Internal |
| Sidechain High Pass / Low Pass | Off, On | Off |
| Sidechain HP / LP Freq | 20 Hz – 20 kHz | 80 Hz / 8 kHz |
| Sidechain Listen | Off, On | Off |
| Expert Mode | Off, On | Off |
| MIDI | Off, On | On |

Gneiss also reports its input, output and gain reduction levels to the host as read-only output parameters.

---

## 16. Troubleshooting

**Nothing happens with External sidechain.** The sidechain input is empty: route a trigger to it (section 8), or switch back to Internal.

**I hear a thin, filtered sound.** Listen is on (the status line is red). Switch it off in the Expert panel.

**Cmd/Ctrl-Z undoes something in my DAW.** The host keeps the shortcut; use the UNDO / REDO buttons.

**The plugin doesn't appear.** Check the install folders and rescan; on macOS run the `xattr` command from section 1. Logic needs the `.component` file.

---

Gneiss is free software under the MIT license. "FabFilter" and "Pro-C" are trademarks of FabFilter Software Instruments; Gneiss is an independent design and is not affiliated with or endorsed by FabFilter.
