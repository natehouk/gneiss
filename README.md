# Gneiss

**A stereo compressor plugin for macOS, Windows and Linux (VST3 · AU · CLAP · VST2).**

Part of **[Epsilon Audio](https://github.com/natehouk/epsilon-plugins)**, a family of free, open-source plugins: Monolith, Caldera, Obsidian, Strata, Basalt, Gneiss, Mica, Flint, Granite, Magma, Schist, Slate, Talc, Geode and Malachite.

![Gneiss](screenshot.png)

Gneiss is a clean-room design with the feature set of FabFilter Pro-C 3 (2025). Gneiss 1.x matched the original Pro-C (2007, 1.10). Version 2.0 adds:
- 14 styles, including upward, TTM, vari-mu and El-Op
- lookahead, hold, range and a gain-change mix up to 200%
- auto threshold and auto release
- oversampling up to 32x
- analog character
- a 6-band sidechain EQ
- MIDI and tempo triggering
- loudness metering

It was written from scratch, working only from FabFilter's public product pages and press releases. No FabFilter code, binaries, presets, artwork or trademarks were used.

It is part of a family of plugins:
- [Monolith](https://github.com/natehouk/monolith) (synth)
- [Caldera](https://github.com/natehouk/caldera) (filter)
- [Obsidian](https://github.com/natehouk/obsidian) (polysynth)
- [Strata](https://github.com/natehouk/strata) (tape delay)
- [Basalt](https://github.com/natehouk/basalt) (live dual filter)
- **Gneiss**, named after a rock formed under pressure

## Download

Get the latest build from [**Releases**](../../releases):

| Platform | File | Contains |
|---|---|---|
| macOS 10.13+ (Apple Silicon + Intel) | `Gneiss-macos-universal.zip` | VST3, AU, CLAP, manual |
| Windows x64 | `Gneiss-windows-x64.zip` | VST3, CLAP, VST2, manual |
| Linux x64 | `Gneiss-linux-x64.zip` | VST3, CLAP, VST2, manual |

### Install

| Platform | Copy | To |
|---|---|---|
| macOS | `Gneiss.vst3` / `Gneiss.component` / `Gneiss.clap` | `~/Library/Audio/Plug-Ins/VST3` / `Components` / `CLAP` |
| Windows | `Gneiss.vst3` (folder) / `Gneiss.clap` | `C:\Program Files\Common Files\VST3\` / `CLAP\` |
| Linux | `Gneiss.vst3` / `Gneiss.clap` | `~/.vst3/` / `~/.clap/` |

**macOS:** the plugins are ad-hoc signed, not notarized. After copying, run:

```
xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/*/Gneiss*
```

## Features

**Compression**
- **Fourteen styles:**

  | Style | Character |
  |---|---|
  | **Clean** | Feed-forward peak detection; precise and transparent |
  | **Classic** | Feedback design; release lengthens after sustained compression |
  | **Opto** | Averaged detection, wide knee, release slows the harder it works |
  | **Vocal** | Gentle on quiet words, firmer on loud ones |
  | **Mastering** | Slow averaged detection, soft knee |
  | **Bus** | Feedback glue with an adaptive release |
  | **Punch** | Lets the start of each hit through |
  | **Pumping** | Linear release for audible pumping |
  | **Versatile** | A balanced all-rounder |
  | **Smooth** | Gentle, transparent levelling |
  | **Upward** | Raises quiet parts toward the threshold |
  | **TTM** | Down and up at once: dense and loud |
  | **Vari-Mu** | Ratio grows with level |
  | **El-Op** | Electro-optical: very soft knee, two-stage release |

- **Controls:**
  - Threshold: −60 to 0 dB
  - Ratio: 1:1 to 100:1, with Inf:1 at the top
  - Knee: 0 to 72 dB
  - Attack: 0.01 to 250 ms
  - Release: 5 ms to 2.5 s
  - Make-up gain: −12 to +36 dB
- **Hold** (up to 500 ms) and **range**: the most the gain may change.
- **Gain-change mix** from 0 to 200%. It is in-phase parallel compression, and above 100% it exaggerates.
- **Dry/wet**: classic parallel compression.
- **Lookahead** up to 20 ms. Latency is reported to the host.
- **Automatic help**:
  - **auto gain**
  - **auto threshold**: compression stays steady while the input level moves
  - **auto release**
- **Oversampling** up to 32x, with linear-phase filters.
- **Analog character**:
  - Warm, Punchy or Vintage (with drift)
  - pre or post the compressor
  - level-neutral drive
- **Click-free automation**: every control is smoothed.

**Stereo, sidechain and triggering**
- **Stereo modes**: left/right, mid/side, mid only, side only, mid-to-side and side-to-mid.
- **Stereo link**, from fully independent to fully linked.
- **External sidechain input** (stereo).
- **6-band sidechain EQ** with Mica's shapes and slopes. Each band can work on the whole sidechain or on its mid or side. The 1.x 48 dB/oct high and low pass filters are still there.
- **Audition**: hear the sidechain, or only the gain change.
- **MIDI note triggering** and a **tempo-synced pulse**: 1/1 to 1/32, dotted and triplet values, with offset and length.

**Displays**
- **Level display**: input, output, gain reduction, upward boost and trigger activity. Drag it to set the threshold.
- **Transfer curve**: shows the current style, knee, range and mix.
- **Meters**: input, gain change and output.
- **EBU R128 / BS.1770 loudness**: momentary, short-term, integrated and true peak.

**Workflow**
- **A/B comparison**: two complete settings, with copy between them.
- **Undo/redo** for every edit.
- **MIDI Learn**, saved with your project.
- **Big knobs**:
  - vertical and circular dragging
  - double-click to type a value
  - Cmd/Ctrl-click to reset
- **46 factory presets**: the 31 from 1.x plus 15 new ones. You can also save your own user presets.
- **Hover tips** on every control. The **HELP** window has seven pages, and there is a **[full manual](docs/MANUAL.md)** (also in every release zip as `Gneiss-Manual.html`).
- **Resizable** window.

![Help window](screenshot-help.png)

### Compared with FabFilter Pro-C 3 (2025)

| Pro-C 3 | Gneiss |
|---|---|
| 14 compression styles, including Upward, TTM, Vari-Mu and El-Op | ✓ |
| Threshold, ratio up to Inf:1, knee, attack, release, hold, range, lookahead | ✓ |
| Auto gain, auto threshold, auto release | ✓ |
| Gain-change mix up to 200%, dry/wet | ✓ |
| Character modes (warm, punchy, vintage), pre or post | ✓ |
| Oversampling up to 32x | ✓ (linear phase) |
| Stereo, mid/side and cross-channel modes, stereo link | ✓ |
| Sidechain EQ with up to 6 bands, external sidechain | ✓ |
| Sidechain and gain-change audition | ✓ |
| MIDI triggering | ✓ (note velocity) |
| Level display, transfer curve, meters, loudness metering | ✓ |
| MIDI Learn, undo/redo, A/B, help hints | ✓ |
| Resizable interface | ✓ (resizes freely) |
| Instance List | Not supported (needs instances to talk to each other) |
| Surround and Dolby Atmos | Not supported (stereo only) |
| AAX, AudioSuite | Not supported (needs Avid's SDK and signing) |
| iPad (AUv3) | Not supported |
| — | Extra: a tempo-synced pulse trigger |

## Build from source

The plugin is built on [DPF](https://github.com/DISTRHO/DPF):

```
git clone --recursive https://github.com/DISTRHO/DPF.git dpf
make -C plugins/Gneiss                    # Linux / macOS (macOS also builds the AU)
make -C plugins/Gneiss WINDOWS=true CC=x86_64-w64-mingw32-gcc CXX=x86_64-w64-mingw32-g++
```

GitHub Actions builds all three platforms on every push. The macOS build is universal and ad-hoc signed. A `v*` tag, or a release published on GitHub, attaches the zips to that release.

## Tests

```
cd tests
g++ -O2 -std=c++17 -I../plugins/Gneiss features.cpp -o features && ./features    # 1.x behaviour: curves, timing, link, M/S, sidechain, mix, meters
g++ -O2 -std=c++17 -I../plugins/Gneiss features2.cpp -o features2 && ./features2 # 2.0: every style, hold, range, lookahead, oversampling, auto modes,
                                                                                 # character, stereo modes, SC EQ, MIDI + pulse triggers, delta, loudness
g++ -O2 -std=c++17 -I../plugins/Gneiss compat.cpp -o compat && ./compat          # random 1.x settings through the frozen 1.0.1 engine and 2.0: identical
g++ -O2 -std=c++17 -I../plugins/Gneiss fuzz.cpp -o fuzz && ./fuzz                # random parameters at 44.1-192 kHz: always finite
g++ -O2 -std=c++17 -I../plugins/Gneiss presets.cpp -o presets && ./presets       # a drum loop through every preset
g++ -O3 -std=c++17 -I../plugins/Gneiss bench.cpp -o bench && ./bench             # CPU per configuration
```

CI runs all of these except the benchmark on every push.

CPU use, as a share of one core at 48 kHz:

| Configuration | CPU |
|---|---|
| 1.x settings | about 1.3% |
| Warm character, 4x oversampling | about 3.8% |
| Vintage, 32x oversampling | about 22% |

Release checks:
- Steinberg's VST3 validator passes, including the sidechain bus.
- The interface is exercised with scripted input and live audio.

## Source layout

| File | Contents |
|---|---|
| `plugins/Gneiss/GneissEngine.hpp` | All DSP. Header-only |
| `plugins/Gneiss/ParamDefs.hpp` | Parameter table, labels, value mapping |
| `plugins/Gneiss/Presets.hpp` | Factory presets |
| `plugins/Gneiss/Help.hpp` | Hover tip text and in-plugin help pages |
| `plugins/Gneiss/GneissPlugin.cpp` | Plugin wrapper: parameters, sidechain ports, MIDI, transport, latency, programs, meters |
| `plugins/Gneiss/GneissUI.cpp` | Custom vector interface (NanoVG) |
| `plugins/Gneiss/common/` | Shared code vendored from [epsilon-common](https://github.com/natehouk/epsilon-common) (see its `VERSION` file). It covers: oversampling, EQ, loudness, character, UI kit, undo, user presets |
| `tests/legacy/` | Frozen 1.0.1 engine, used by the compatibility test |
| `docs/MANUAL.md` | User manual (`Gneiss-Manual.html` is generated from it with pandoc) |

New parameters, styles and presets are only ever appended, so saved sessions and automation keep working across versions.

## Changelog

- **2.0:** Brings Gneiss up to Pro-C 3. New in this version:
  - Eleven new styles, including Upward, TTM, Vari-Mu and El-Op.
  - Hold, range, gain-change mix up to 200%, and lookahead.
  - Auto threshold and auto release.
  - Oversampling up to 32x and analog character.
  - Four more stereo modes.
  - A 6-band sidechain EQ, sidechain and gain-change audition.
  - MIDI and tempo-pulse triggering.
  - Loudness metering.
  - Ratio up to Inf:1 and knee up to 72 dB.
  - Fifteen new presets; a larger, panelled interface.

  Sessions, automation and presets from 1.x load and sound the same. A test checks this against the frozen 1.0.1 engine.

  Ratio, knee and channel mode now cover wider ranges. In hosts that store automation as a 0–1 position, check those lanes after upgrading.
- **1.0.1:** Right-click actions respond to the right mouse button.
- **1.0:** First release.

## License

[MIT](LICENSE). Built with [DPF](https://github.com/DISTRHO/DPF) and its bundled NanoVG, which are under the ISC license. `plugins/Gneiss/ResizeHandle.hpp` comes from DPF's examples and keeps its ISC notice.

"FabFilter", "FabFilter Pro-C" and "Pro-C 3" are trademarks of FabFilter Software Instruments. Gneiss is not affiliated with or endorsed by FabFilter.
