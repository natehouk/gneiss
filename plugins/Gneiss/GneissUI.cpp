// Gneiss — custom vector UI (DPF/NanoVG). Original design. License: MIT.
#include "DistrhoUI.hpp"
#include "ParamDefs.hpp"
#include "Presets.hpp"
#include "ResizeHandle.hpp"
#include "Help.hpp"
#include "common/Platform.hpp"
#include "common/UiKit.hpp"
#include "common/UndoHistory.hpp"
#include "common/UserPresets.hpp"
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

START_NAMESPACE_DISTRHO

using namespace gneiss;
using DGL_NAMESPACE::ResizeHandle;
using epsilon::nowSec;

static bool gTipsOn = true;
static constexpr float W = 1180.f, H = 760.f;

#define C_BG        Color(0x10, 0x10, 0x12)
#define C_PANEL     Color(0x19, 0x1a, 0x1d)
#define C_PANEL_HI  Color(0x24, 0x25, 0x29)
#define C_LINE      Color(0x2f, 0x31, 0x36)
#define C_TEXT      Color(0xe8, 0xe8, 0xe6)
#define C_DIM       Color(0x8a, 0x8c, 0x92)
#define C_FAINT     Color(0x4c, 0x4e, 0x55)
#define C_ACC       Color(0xd9, 0xc2, 0x7a)       // gneiss gold
#define C_OUT       Color(0x8f, 0xd0, 0xb8)       // output level
#define C_GR        Color(0xf0, 0x5a, 0x4a)       // gain reduction
#define C_UP        Color(0x7c, 0xd8, 0x6a)       // upward boost
#define C_DISPLAY   Color(0x0b, 0x0b, 0x0d)

static Color withAlpha(const Color& c, float a) { Color r(c); r.alpha = a; return r; }
static epsilon::Theme gneissTheme() { return { C_BG, C_PANEL, C_PANEL_HI, C_LINE, C_TEXT, C_DIM, C_FAINT, C_ACC, Color(0x1f, 0x20, 0x24), Color(0xc6, 0xc8, 0xcc) }; }
static Color scBandColor(int b) { static const Color c[6] = { Color(0xf0, 0x8a, 0x5a), Color(0xe8, 0xd0, 0x60), Color(0x7c, 0xd8, 0x6a), Color(0x60, 0xc8, 0xe0), Color(0x90, 0x9c, 0xf8), Color(0xe0, 0x88, 0xe0) }; return c[b % 6]; }

// ---------------------------------------------------------------- layout
static constexpr float LX = 16, LY = 64, LW = 650, LH = 320;          // level display
static constexpr float TX = 674, TY = 64, TW = 300, TH = 320;         // transfer curve
static constexpr float MX = 982, MY = 64, MW = 182, MH = 320;         // meters + loudness
static constexpr float PY = 394, PH = 352;                            // bottom panels
static constexpr float KX = 16, KW = 704, EX = 728, EW = 436;
static constexpr float SEG_H = 22;
static constexpr float TAB_Y = PY + 12, TAB_H = 24;
static constexpr float QX = EX + 16, QY = PY + 112, QW = EW - 32, QH = 128;    // sidechain EQ display
static constexpr float QFMIN = 20.f, QFMAX = 20000.f, QDB = 24.f;
enum Tab { TabStereo, TabSidechain, TabTrigger, TabCharacter, kNumTabs };

enum WType { Knob, Switch, Segment, Stepper, StyleGrid, ScStepper };
// tab: -1 always (main panel), else the expert tab it lives on
struct Ctl { WType type; uint32_t param; float x, y, r; const char* label; int tab; int seg0; int segN; };

static std::vector<Ctl> buildControls() {
    std::vector<Ctl> c;
    const int A = -1;
    c.push_back({ StyleGrid, kStyle, KX + 16, PY + 14, 96, "", A, 0, kNumStyles });
    c.push_back({ Knob, kThreshold, KX + 66,  PY + 146, 36, "THRESHOLD", A, 0, 0 });
    c.push_back({ Knob, kRatio,     KX + 170, PY + 146, 32, "RATIO", A, 0, 0 });
    c.push_back({ Knob, kAttack,    KX + 268, PY + 146, 30, "ATTACK", A, 0, 0 });
    c.push_back({ Knob, kRelease,   KX + 362, PY + 146, 30, "RELEASE", A, 0, 0 });
    c.push_back({ Knob, kGain,      KX + 456, PY + 146, 30, "GAIN", A, 0, 0 });
    c.push_back({ Knob, kGrMix,     KX + 550, PY + 146, 30, "MIX", A, 0, 0 });
    c.push_back({ Switch, kAutoGain, KX + 616, PY + 118, 0, "AUTO GAIN", A, 0, 0 });
    c.push_back({ Switch, kAutoThreshold, KX + 616, PY + 146, 0, "AUTO THRESH", A, 0, 0 });
    c.push_back({ Switch, kAutoRelease, KX + 616, PY + 174, 0, "AUTO RELEASE", A, 0, 0 });
    c.push_back({ Knob, kKnee,      KX + 66,  PY + 284, 22, "KNEE", A, 0, 0 });
    c.push_back({ Knob, kRange,     KX + 150, PY + 284, 22, "RANGE", A, 0, 0 });
    c.push_back({ Knob, kHold,      KX + 234, PY + 284, 22, "HOLD", A, 0, 0 });
    c.push_back({ Knob, kLookahead, KX + 318, PY + 284, 22, "LOOKAHEAD", A, 0, 0 });
    c.push_back({ Switch, kExpert, KX + 616, PY + 270, 0, "SHOW PANELS", A, 0, 0 });
    // STEREO tab
    c.push_back({ Segment, kChannelMode, EX + 160, PY + 76, 288, "CHANNELS", TabStereo, 0, 3 });
    c.push_back({ Segment, kChannelMode, EX + 160, PY + 104, 288, "", TabStereo, 3, 3 });
    c.push_back({ Knob, kLink, EX + 356, PY + 92, 22, "LINK", TabStereo, 0, 0 });
    c.push_back({ Segment, kOversampling, EX + 160, PY + 168, 288, "OVERSAMPLING", TabStereo, 0, kNumOversampling });
    c.push_back({ Knob, kMix, EX + 356, PY + 176, 22, "DRY/WET", TabStereo, 0, 0 });
    // SIDECHAIN tab
    c.push_back({ Segment, kSidechain, EX + 82, PY + 62, 132, "", TabSidechain, 0, 2 });
    c.push_back({ Switch, kScEqOn, EX + 172, PY + 62, 0, "SC EQ", TabSidechain, 0, 0 });
    c.push_back({ Switch, kScListen, EX + 266, PY + 62, 0, "AUDITION", TabSidechain, 0, 0 });
    c.push_back({ Switch, kDelta, EX + 266, PY + 88, 0, "GAIN CHANGE", TabSidechain, 0, 0 });
    c.push_back({ ScStepper, kSbShape, EX + 92, PY + 262, 150, "", TabSidechain, 0, 0 });
    c.push_back({ ScStepper, kSbSlope, EX + 232, PY + 262, 116, "", TabSidechain, 0, 0 });
    c.push_back({ ScStepper, kSbPlace, EX + 358, PY + 262, 92, "", TabSidechain, 0, 0 });
    c.push_back({ Switch, kScHpOn, EX + 36, PY + 306, 0, "HP 48", TabSidechain, 0, 0 });
    c.push_back({ Knob, kScHpFreq, EX + 150, PY + 306, 15, "", TabSidechain, 0, 0 });
    c.push_back({ Switch, kScLpOn, EX + 236, PY + 306, 0, "LP 48", TabSidechain, 0, 0 });
    c.push_back({ Knob, kScLpFreq, EX + 350, PY + 306, 15, "", TabSidechain, 0, 0 });
    // TRIGGER tab
    c.push_back({ Switch, kMidiTrigger, EX + 36, PY + 72, 0, "MIDI NOTES", TabTrigger, 0, 0 });
    c.push_back({ Switch, kPulse, EX + 36, PY + 132, 0, "TEMPO PULSE", TabTrigger, 0, 0 });
    c.push_back({ Stepper, kPulseSync, EX + 110, PY + 196, 140, "SYNC", TabTrigger, 0, 0 });
    c.push_back({ Knob, kPulseOffset, EX + 248, PY + 196, 22, "OFFSET", TabTrigger, 0, 0 });
    c.push_back({ Knob, kPulseLength, EX + 336, PY + 196, 22, "LENGTH", TabTrigger, 0, 0 });
    // CHARACTER tab
    c.push_back({ Segment, kCharacter, EX + 160, PY + 76, 288, "CHARACTER", TabCharacter, 0, 4 });
    c.push_back({ Knob, kDrive, EX + 362, PY + 92, 26, "DRIVE", TabCharacter, 0, 0 });
    c.push_back({ Segment, kCharPos, EX + 100, PY + 140, 168, "POSITION", TabCharacter, 0, 2 });
    return c;
}
static const std::vector<Ctl>& controls() { static std::vector<Ctl> c = buildControls(); return c; }

static const char* const* segLabels(uint32_t p, int& n) {
    static const char* const style[] = { "CLEAN", "CLASSIC", "OPTO", "VOCAL", "MASTERING", "BUS", "PUNCH", "PUMPING", "VERSATILE", "SMOOTH", "UPWARD", "TTM", "VARI-MU", "EL-OP" };
    static const char* const chm[] = { "L / R", "M / S", "MID", "SIDE", "MID > SIDE", "SIDE > MID" };
    static const char* const sc[] = { "INTERNAL", "EXTERNAL" };
    static const char* const ovs[] = { "OFF", "2x", "4x", "8x", "16x", "32x" };
    static const char* const chr[] = { "OFF", "WARM", "PUNCHY", "VINTAGE" };
    static const char* const pos[] = { "POST", "PRE" };
    switch (p) {
    case kStyle: n = kNumStyles; return style;
    case kChannelMode: n = kNumChannelModes; return chm;
    case kSidechain: n = 2; return sc;
    case kOversampling: n = kNumOversampling; return ovs;
    case kCharacter: n = 4; return chr;
    case kCharPos: n = 2; return pos;
    default: n = 0; return nullptr;
    }
}
static int scSlopeRank(int idx) { static const int r[17] = { 0, 1, 3, 7, 2, 4, 5, 6, 8, 9, 10, 11, 12, 13, 14, 15, 16 }; return r[std::max(0, std::min(16, idx))]; }
static int scSlopeByRank(int r) { static const int o[17] = { 0, 1, 4, 2, 5, 6, 7, 3, 8, 9, 10, 11, 12, 13, 14, 15, 16 }; return o[std::max(0, std::min(16, r))]; }

static constexpr float PS_X = 330.f, PS_Y = 20.f, PS_W = 230.f, PS_H = 28.f, PS_ARROW = 30.f;
static constexpr float A_X = 572.f, AB_W = 30.f, COPY_X = 636.f, COPY_W = 44.f;
static constexpr float UNDO_X = 692.f, REDO_X = 746.f, UR_W = 50.f, LEARN_X = 806.f, LEARN_W = 66.f, MIDI_X = 878.f, MIDI_W = 52.f, HELP_X = 936.f, HELP_W = 48.f;
static constexpr float PM_COLW = 150.f, PM_ROW = 23.f, PM_HEAD = 22.f, PM_PAD = 8.f;
static constexpr int HIST = 260;

class GneissUI : public UI {
public:
    GneissUI() : UI((uint)W, (uint)H), theme(gneissTheme()), help("GNEISS HELP", kHelpPages, kNumHelpPages, kManualURL), store("Gneiss", ".gneiss"), resizeHandle(this) {
        loadSharedResources();
        Engine def;
        for (uint32_t i = 0; i < kParamCount; ++i) { val[i] = def.p[i]; defv[i] = def.p[i]; const ParamDef& d = paramDef(i); pparams[i] = { d.sym, d.min, d.max, isSetting(i) }; }
        val[kOutInL] = val[kOutInR] = val[kOutOutL] = val[kOutOutR] = -120;
        for (int& m : paramCC) m = -1;
        const double sf = getScaleFactor();
        if (sf != 1.0) setSize((uint)(W * sf), (uint)(H * sf));
        setGeometryConstraints((uint)(W * 0.6 * sf), (uint)(H * 0.6 * sf), true);
        if (isResizable()) resizeHandle.hide();
        userPresets = store.list();
        history.init(kParamCount, [](uint32_t p) { return isSetting(p); });
        history.reset(val);
        help.w = 800; help.h = 540; help.centre(W, H);
        for (int i = 0; i < HIST; ++i) { hIn[i] = hOut[i] = -120; hGr[i] = 0; }
    }

protected:
    float uiScale() const { return (float)getWidth() / W; }

    // ---- values -------------------------------------------------------------
    void setValue(uint32_t p, float v) {
        if (p >= kParamCount) return;
        const ParamDef& d = paramDef(p);
        v = clampf(v, d.min, d.max);
        if (d.hints & kHintInteger) v = std::round(v);
        if (v == val[p]) return;
        val[p] = v; setParameterValue(p, v); repaint();
    }
    void commit(uint32_t p, float v) { if (p >= kParamCount) return; editParameter(p, true); setValue(p, v); editParameter(p, false); }
    void parameterChanged(uint32_t i, float v) override {
        if (i >= kParamCount) return;
        if (i == kOutLearn) {
            if (v != lastLearnEcho && v > 0) {
                const int code = (int)v % 1000000 - 1, prm = code / 128, cc = code % 128;
                for (int& m : paramCC) if (m == cc) m = -1;
                if (prm >= 0 && prm < (int)kParamCount) paramCC[prm] = cc;
                learnWaiting = -1;
            }
            lastLearnEcho = v; repaint(); return;
        }
        if (paramDef(i).hints & kHintOutput) { val[i] = v; return; }        // meters: drawn on the idle tick
        if (val[i] != v) { val[i] = v; history.hostChanged(i, v); repaint(); }
    }
    void stateChanged(const char* key, const char* value) override {
        if (std::strcmp(key, "midimap") != 0) return;
        for (int& m : paramCC) m = -1;
        const char* q = value;
        while (q && *q) {
            int cc = -1, prm = -1;
            if (std::sscanf(q, "%d:%d", &cc, &prm) == 2 && prm >= 0 && prm < (int)kParamCount) paramCC[prm] = cc;
            const char* s = std::strchr(q, ';'); if (!s) break; q = s + 1;
        }
        repaint();
    }

    // ---- undo / redo / A-B -----------------------------------------------------------
    void beginGesture() { gesture = true; history.begin(val); }
    void endGesture() { gesture = false; history.end(val); repaint(); }
    void applyAll(const float* s) { for (uint32_t i = 0; i < kParamCount; ++i) if (isSetting(i) && val[i] != s[i]) commit(i, s[i]); }
    void undo() { epsilon::UndoHistory::Snap s; if (history.undo(val, s)) { applyAll(s.data()); flash("Undo"); } }
    void redo() { epsilon::UndoHistory::Snap s; if (history.redo(val, s)) { applyAll(s.data()); flash("Redo"); } }
    void switchAB(int to) { epsilon::UndoHistory::Snap s; if (history.switchTo(to, val, s)) { applyAll(s.data()); flash(to == 0 ? "Setting A" : "Setting B"); } }
    void copyAB() { history.copyToOther(val); flash(history.slot() == 0 ? "Copied A to B" : "Copied B to A"); }

    // ---- formatting ---------------------------------------------------------
    void fmtVal(uint32_t p, float v, char* b, size_t n) const {
        int ne = 0;
        if (const char* const* l = enumLabels(p, ne)) { snprintf(b, n, "%s", l[std::max(0, std::min(ne - 1, (int)std::round(v)))]); return; }
        const ParamDef& d = paramDef(p);
        if (d.hints & kHintBoolean) { snprintf(b, n, "%s", v > 0.5f ? "On" : "Off"); return; }
        const int sf = scField(p);
        if (sf == kSbFreq || p == kScHpFreq || p == kScLpFreq) { if (v >= 1000) snprintf(b, n, "%.2f kHz", v / 1000); else snprintf(b, n, "%.0f Hz", v); return; }
        if (sf == kSbGain) { snprintf(b, n, "%+.1f dB", v); return; }
        if (sf == kSbQ) { snprintf(b, n, v < 10 ? "%.2f" : "%.1f", v); return; }
        switch (p) {
        case kThreshold: case kKnee: snprintf(b, n, "%.1f dB", v); return;
        case kRange: if (v >= 59.95f) snprintf(b, n, "No limit"); else snprintf(b, n, "%.1f dB", v); return;
        case kGain: snprintf(b, n, "%+.1f dB", v); return;
        case kRatio: if (ratioIsInf(v)) snprintf(b, n, "Inf:1"); else snprintf(b, n, v < 10 ? "%.2f:1" : "%.1f:1", v); return;
        case kAttack: snprintf(b, n, v < 1 ? "%.2f ms" : (v < 10 ? "%.1f ms" : "%.0f ms"), v); return;
        case kRelease: if (v >= 1000) snprintf(b, n, "%.2f s", v / 1000); else snprintf(b, n, "%.0f ms", v); return;
        case kHold: snprintf(b, n, "%.0f ms", v); return;
        case kLookahead: snprintf(b, n, v < 10 ? "%.1f ms" : "%.0f ms", v); return;
        case kGrMix: case kDrive: case kPulseOffset: case kPulseLength: snprintf(b, n, "%.0f%%", v); return;
        default: snprintf(b, n, "%.0f%%", v * 100);
        }
    }
    void fmt(uint32_t p, char* b, size_t n) const { if (p < kParamCount) fmtVal(p, val[p], b, n); else snprintf(b, n, "-"); }
    bool parse(uint32_t p, const std::string& in, float& out) const {
        std::string s; for (char ch : in) if (!std::isspace((unsigned char)ch)) s += (char)std::tolower((unsigned char)ch);
        if (s.empty()) return false;
        if (p == kRatio && (s == "inf" || s == "inf:1")) { out = 100.f; return true; }
        char* end = nullptr; double x = std::strtod(s.c_str(), &end);
        if (end == s.c_str()) return false;
        std::string suf(end);
        if (suf == "k" || suf == "khz") x *= 1000;
        if (p == kRelease && suf == "s") x *= 1000;
        if (p == kMix || p == kLink) { out = (float)(x / 100.0); return true; }
        out = (float)x; return true;
    }
    int selBand() const { return scSel >= 0 && val[sp(scSel, kSbUsed)] > 0.5f ? scSel : -1; }
    uint32_t effParam(const Ctl& c) const {
        if (c.type == ScStepper) { const int b = selBand(); return b >= 0 ? sp(b, (int)c.param) : kParamCount; }
        return c.param;
    }
    bool visible(const Ctl& c) const {
        if (c.tab < 0) return true;
        if (val[kExpert] < 0.5f || tab != c.tab) return false;
        if (c.type == ScStepper) return selBand() >= 0;
        return true;
    }
    bool dimmed(const Ctl& c) const {
        const uint32_t p = effParam(c);
        if (p == kScHpFreq && val[kScHpOn] < 0.5f) return true;
        if (p == kScLpFreq && val[kScLpOn] < 0.5f) return true;
        if ((p == kPulseSync || p == kPulseOffset || p == kPulseLength) && val[kPulse] < 0.5f) return true;
        if ((p == kDrive || p == kCharPos) && val[kCharacter] < 0.5f) return true;
        if (p == kLink && (int)(val[kChannelMode] + 0.5f) >= kMidOnly) return true;
        if (c.type == ScStepper && c.param == kSbSlope) { const int b = selBand(); return b < 0 || !eq::hasSlope((int)(val[sp(b, kSbShape)] + 0.5f)); }
        return false;
    }

    // ---- drawing ------------------------------------------------------------
    void onNanoDisplay() override {
        const float s = uiScale();
        save(); scale(s, s);
        beginPath(); rect(0, 0, W, H); fillColor(C_BG); fill();
        fontFace(NANOVG_DEJAVU_SANS_TTF);
        for (int i = 0; i < 4; ++i) {   // header: folded (gneissic) bands
            const float y0 = 20 + i * 7.f;
            beginPath(); moveTo(16, y0 + 3); bezierTo(22, y0 - 3, 28, y0 + 6, 38, y0); lineTo(38, y0 + 4.5f); bezierTo(28, y0 + 10.5f, 22, y0 + 1.5f, 16, y0 + 7.5f); closePath();
            fillColor(i % 2 == 0 ? C_ACC : Color(0x6c, 0x6e, 0x76)); fill();
        }
        fontSize(22); textLetterSpacing(9); fillColor(C_TEXT); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); text(50, 34, "GNEISS", nullptr);
        fontSize(10); textLetterSpacing(2.5f); fillColor(C_DIM); text(198, 30, "COMPRESSOR", nullptr);
        fontSize(7.5f); textLetterSpacing(1.8f); fillColor(C_FAINT); text(198, 42, "EPSILON AUDIO  2.0", nullptr);
        drawPresetBar();
        for (int k = 0; k < 2; ++k) {
            const float x = A_X + k * AB_W; const bool on = history.slot() == k, hot = hoverHeader == 6 + k;
            beginPath(); roundedRect(x + (k ? 0.5f : 0), PS_Y, AB_W - 0.5f, PS_H, 5); fillColor(on ? withAlpha(C_ACC, 0.85f) : (hot ? C_PANEL_HI : C_PANEL)); fill();
            strokeColor(on ? C_ACC : C_LINE); strokeWidth(1); stroke();
            fontSize(11); textLetterSpacing(0); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(on ? C_BG : C_DIM); text(x + AB_W / 2, PS_Y + PS_H / 2 + 0.5f, k ? "B" : "A", nullptr);
        }
        hdrButton(COPY_X, COPY_W, "COPY", false, hoverHeader == 8, false);
        hdrButton(UNDO_X, UR_W, "UNDO", false, hoverHeader == 3, !history.canUndo());
        hdrButton(REDO_X, UR_W, "REDO", false, hoverHeader == 4, !history.canRedo());
        hdrButton(LEARN_X, LEARN_W, learnMode ? "LEARNING" : "LEARN", learnMode, hoverHeader == 1, false);
        hdrButton(MIDI_X, MIDI_W, "MIDI", val[kMidiOn] > 0.5f, hoverHeader == 2, false);
        hdrButton(HELP_X, HELP_W, "HELP", help.open, hoverHeader == 5, false);
        {
            const double sr = getSampleRate() > 0 ? getSampleRate() : 48000.0;
            const double lat = std::round(clampf(val[kLookahead], 0, 20) * 0.001 * sr) + epsilon::OversamplerN::latency(osFactor(val[kOversampling]));
            if (lat > 0) { char b[48]; snprintf(b, sizeof b, "LATENCY  %.1f ms", lat * 1000.0 / sr); fontSize(8); textLetterSpacing(1.5f); textAlign(ALIGN_RIGHT | ALIGN_MIDDLE); fillColor(C_DIM); text(W - 16, 34, b, nullptr); }
        }
        drawLevelDisplay();
        drawTransfer();
        drawMeters();
        panel(KX, PY, KW, PH); panel(EX, PY, EW, PH);
        caption(KX + 192, PY + 236, "KNEE · RANGE · HOLD · LOOKAHEAD");
        if (val[kExpert] > 0.5f) drawTabs(); else drawExpertSummary();
        const std::vector<Ctl>& cs = controls();
        for (int i = 0; i < (int)cs.size(); ++i) if (visible(cs[i])) drawCtl(cs[i], i == active || i == hover);
        if (val[kExpert] > 0.5f && tab == TabSidechain) drawScEq();
        if (val[kExpert] > 0.5f && tab == TabTrigger) drawTriggerInfo();
        if (val[kExpert] > 0.5f && tab == TabCharacter) drawCharacterInfo();
        if (editing >= 0) drawEditBox();
        drawStatus();
        if (menuOpen) drawPresetMenu();
        if (saving) drawSaveBox();
        if (help.open) help.draw(*this, theme, W, H, gTipsOn);
        else if (tips.shown && gTipsOn) { std::string t, v, b; if (tipContent(t, v, b)) epsilon::drawTooltip(*this, theme, tips.x, tips.y, W, H, t, v, b); }
        restore();
    }
    void panel(float x, float y, float w, float h) { beginPath(); roundedRect(x, y, w, h, 6); fillColor(C_PANEL); fill(); strokeColor(C_LINE); strokeWidth(1); stroke(); }
    void displayBox(float x, float y, float w, float h) { beginPath(); roundedRect(x, y, w, h, 6); fillColor(C_DISPLAY); fill(); strokeColor(C_LINE); strokeWidth(1); stroke(); }
    void caption(float cx, float y, const char* t, int align = ALIGN_CENTER) { if (!t || !t[0]) return; fontSize(7.5f); textLetterSpacing(1.8f); textAlign(align | ALIGN_MIDDLE); fillColor(C_FAINT); text(cx, y, t, nullptr); }
    void hdrButton(float x, float w, const char* label, bool on, bool hot, bool disabled) { epsilon::drawHeaderButton(*this, theme, x, PS_Y, w, PS_H, label, on, hot, disabled); }

    void drawTabs() {
        static const char* const names[kNumTabs] = { "STEREO", "SIDECHAIN", "TRIGGER", "CHARACTER" };
        const float tw = (EW - 24) / kNumTabs;
        for (int t = 0; t < kNumTabs; ++t) {
            const float x = EX + 12 + t * tw; const bool on = t == tab, hot = hoverTab == t;
            beginPath(); roundedRect(x + 1, TAB_Y, tw - 2, TAB_H, 5); fillColor(on ? withAlpha(C_ACC, 0.22f) : (hot ? C_PANEL_HI : C_PANEL)); fill();
            strokeColor(on ? C_ACC : C_LINE); strokeWidth(1); stroke();
            fontSize(9); textLetterSpacing(1.8f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(on ? C_TEXT : C_DIM); text(x + tw / 2, TAB_Y + TAB_H / 2 + 0.5f, names[t], nullptr);
            // a dot when something on that tab is active
            bool act = false;
            if (t == TabStereo) act = val[kChannelMode] > 0.5f || val[kOversampling] > 0.5f || val[kMix] < 0.999f;
            if (t == TabSidechain) act = val[kSidechain] > 0.5f || val[kScHpOn] > 0.5f || val[kScLpOn] > 0.5f || anyScBand() || val[kScListen] > 0.5f || val[kDelta] > 0.5f;
            if (t == TabTrigger) act = val[kMidiTrigger] > 0.5f || val[kPulse] > 0.5f;
            if (t == TabCharacter) act = val[kCharacter] > 0.5f;
            if (act) { beginPath(); circle(x + tw - 10, TAB_Y + 7, 2.5f); fillColor(C_ACC); fill(); }
        }
    }
    bool anyScBand() const { for (int b = 0; b < kScBands; ++b) if (val[sp(b, kSbUsed)] > 0.5f) return true; return false; }
    void drawExpertSummary() {
        char b[96]; std::string l1, l2, l3;
        fmt(kChannelMode, b, sizeof b); l1 = std::string(b) + "   ·   link " + std::to_string((int)std::round(val[kLink] * 100)) + "%";
        fmt(kSidechain, b, sizeof b); l2 = std::string(b) + " sidechain"; if (anyScBand() && val[kScEqOn] > 0.5f) l2 += "   ·   SC EQ"; if (val[kScHpOn] > 0.5f || val[kScLpOn] > 0.5f) l2 += "   ·   filtered";
        fmt(kCharacter, b, sizeof b); l3 = std::string("character ") + b; fmt(kOversampling, b, sizeof b); l3 += std::string("   ·   oversampling ") + b;
        fontSize(11); textLetterSpacing(0.6f); textAlign(ALIGN_LEFT | ALIGN_TOP); fillColor(C_DIM);
        text(EX + 20, PY + 70, l1.c_str(), nullptr); text(EX + 20, PY + 92, l2.c_str(), nullptr); text(EX + 20, PY + 114, l3.c_str(), nullptr);
        fontSize(10); fillColor(C_FAINT); textBox(EX + 20, PY + 160, EW - 40, "Switch on SHOW PANELS for the stereo modes, the sidechain EQ and filters, MIDI and tempo triggering, and analog character.", nullptr);
    }

    // level display (dB 0 at top .. -60 at bottom)
    static float yOfDbL(float db) { return LY + 10 + (LH - 30) * clampf(-db / 60.f, 0.f, 1.f); }
    void drawLevelDisplay() {
        displayBox(LX, LY, LW, LH);
        fontSize(8); textLetterSpacing(1); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(C_FAINT);
        beginPath();
        for (int db = 0; db >= -60; db -= 12) { const float y = yOfDbL((float)db); moveTo(LX + 6, y); lineTo(LX + LW - 6, y); }
        strokeColor(Color(255, 255, 255, 8 / 255.f)); strokeWidth(1); stroke();
        for (int db = -12; db >= -60; db -= 12) { char b[8]; snprintf(b, sizeof b, "%d", db); text(LX + 8, yOfDbL((float)db) - 6, b, nullptr); }
        const float x0 = LX + 30, x1 = LX + LW - 8, dx = (x1 - x0) / (HIST - 1);
        auto idx = [&](int k) { return (hPos + k) % HIST; };
        beginPath(); moveTo(x0, yOfDbL(-60));
        for (int k = 0; k < HIST; ++k) lineTo(x0 + k * dx, yOfDbL(hIn[idx(k)]));
        lineTo(x1, yOfDbL(-60)); closePath(); fillColor(Color(0x3a, 0x3c, 0x42, 0.85f)); fill();
        beginPath();
        for (int k = 0; k < HIST; ++k) { const float x = x0 + k * dx, y = yOfDbL(hOut[idx(k)]); if (!k) moveTo(x, y); else lineTo(x, y); }
        strokeColor(C_OUT); strokeWidth(1.6f); stroke();
        // gain change hanging from the top: reduction red, upward boost green (0..30 dB over the top half)
        for (int pass = 0; pass < 2; ++pass) {
            const float sgn = pass == 0 ? 1.f : -1.f; bool any = false;
            for (int k = 0; k < HIST; ++k) if (hGr[idx(k)] * sgn > 0.05f) { any = true; break; }
            if (!any) continue;
            beginPath(); moveTo(x0, LY + 10);
            for (int k = 0; k < HIST; ++k) lineTo(x0 + k * dx, LY + 10 + (LH - 30) * 0.5f * clampf(sgn * hGr[idx(k)] / 30.f, 0.f, 1.f));
            lineTo(x1, LY + 10); closePath(); fillColor(withAlpha(pass == 0 ? C_GR : C_UP, 0.45f)); fill();
        }
        if (hTrig[(hPos + HIST - 1) % HIST] > 0.f || true) {   // trigger activity along the bottom
            for (int k = 0; k < HIST; ++k) if (hTrig[idx(k)] > 0.01f) { beginPath(); rect(x0 + k * dx, LY + LH - 22, dx + 0.5f, 4); fillColor(withAlpha(C_ACC, 0.3f + 0.6f * hTrig[idx(k)])); fill(); }
        }
        const float ty = yOfDbL(val[kThreshold]);
        for (float x = x0; x < x1; x += 10) { beginPath(); moveTo(x, ty); lineTo(std::min(x + 5, x1), ty); strokeColor(withAlpha(C_ACC, hoverThr || dragThr ? 1.f : 0.7f)); strokeWidth(hoverThr || dragThr ? 2.f : 1.2f); stroke(); }
        char b[40]; snprintf(b, sizeof b, val[kAutoThreshold] > 0.5f ? "%.1f dB (auto)" : "%.1f dB", val[kThreshold]);
        fontSize(9); textLetterSpacing(0.6f); textAlign(ALIGN_RIGHT | ALIGN_BOTTOM); fillColor(C_ACC); text(x1 - 2, ty - 3, b, nullptr);
        fontSize(8); textLetterSpacing(1.6f); textAlign(ALIGN_LEFT | ALIGN_BOTTOM);
        fillColor(C_DIM); text(LX + 30, LY + LH - 4, "INPUT", nullptr); fillColor(C_OUT); text(LX + 80, LY + LH - 4, "OUTPUT", nullptr); fillColor(C_GR); text(LX + 136, LY + LH - 4, "GAIN REDUCTION", nullptr);
        if ((int)(val[kStyle] + .5f) == kUpward || (int)(val[kStyle] + .5f) == kTTM) { fillColor(C_UP); text(LX + 244, LY + LH - 4, "BOOST", nullptr); }
    }
    static float xOfDbT(float db) { return TX + 26 + (TW - 40) * clampf((db + 60.f) / 60.f, 0.f, 1.f); }
    static float yOfDbT(float db) { return TY + 12 + (TH - 44) * clampf(-db / 60.f, 0.f, 1.f); }
    void drawTransfer() {
        displayBox(TX, TY, TW, TH);
        beginPath();
        for (int db = 0; db >= -60; db -= 12) { moveTo(xOfDbT((float)db), yOfDbT(0)); lineTo(xOfDbT((float)db), yOfDbT(-60)); moveTo(xOfDbT(-60), yOfDbT((float)db)); lineTo(xOfDbT(0), yOfDbT((float)db)); }
        strokeColor(Color(255, 255, 255, 8 / 255.f)); strokeWidth(1); stroke();
        beginPath(); moveTo(xOfDbT(-60), yOfDbT(-60)); lineTo(xOfDbT(0), yOfDbT(0)); strokeColor(C_FAINT); strokeWidth(1); stroke();
        scissor(TX + 1, TY + 1, TW - 2, TH - 2);
        beginPath();
        for (int i = 0; i <= 240; ++i) { const float in = -60.f + i * 0.25f, x = xOfDbT(in), y = yOfDbT(transferCurve(val, in)); if (!i) moveTo(x, y); else lineTo(x, y); }
        strokeColor(C_ACC); strokeWidth(2.2f); stroke();
        resetScissor();
        const float tx = xOfDbT(val[kThreshold]);
        beginPath(); moveTo(tx, yOfDbT(0)); lineTo(tx, yOfDbT(-60)); strokeColor(withAlpha(C_ACC, hoverCurve || dragCurve ? 0.6f : 0.25f)); strokeWidth(1); stroke();
        const float inNow = std::max(val[kOutInL], val[kOutInR]);
        if (inNow > -60) { beginPath(); circle(xOfDbT(inNow), yOfDbT(transferCurve(val, inNow)), 5); fillColor(C_OUT); fill(); }
        fontSize(8); textLetterSpacing(1.4f); textAlign(ALIGN_CENTER | ALIGN_BOTTOM); fillColor(C_FAINT); text(TX + TW / 2 + 6, TY + TH - 6, "INPUT  dB", nullptr);
        textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
        for (int db = -12; db >= -48; db -= 12) { char b[8]; snprintf(b, sizeof b, "%d", db); text(TX + 6, yOfDbT((float)db), b, nullptr); }
        char b[48]; fmt(kRatio, b, sizeof b); std::string st = std::string(b) + "   knee "; char kb[16]; snprintf(kb, sizeof kb, "%.0f dB", val[kKnee]); st += kb;
        int ne; st += "   "; st += enumLabels(kStyle, ne)[(int)(val[kStyle] + .5f)];
        fontSize(9); textLetterSpacing(0.6f); textAlign(ALIGN_LEFT | ALIGN_TOP); fillColor(C_DIM); text(TX + 30, TY + 14, st.c_str(), nullptr);
    }
    void drawMeters() {
        displayBox(MX, MY, MW, MH);
        const float top = MY + 26, bot = MY + MH - 22, h = bot - top;
        auto yDb = [&](float db) { return top + h * clampf(-db / 60.f, 0.f, 1.f); };
        auto bar = [&](float x, float w, float db, const Color& c) {
            beginPath(); rect(x, top, w, h); fillColor(C_PANEL_HI); fill();
            if (db > -60) { beginPath(); rect(x, yDb(db), w, bot - yDb(db)); fillColor(db > -0.5f ? C_GR : c); fill(); }
        };
        bar(MX + 10, 9, val[kOutInL], C_DIM); bar(MX + 21, 9, val[kOutInR], C_DIM);
        for (int k = 0; k < 2; ++k) {
            const float x = MX + 40 + k * 7.f; beginPath(); rect(x, top, 6, h); fillColor(C_PANEL_HI); fill();
            const float gv = val[k ? kOutGrR : kOutGrL], g = clampf(std::fabs(gv) / 30.f, 0.f, 1.f);
            if (g > 0.002f) { beginPath(); rect(x, top, 6, h * g); fillColor(gv >= 0 ? C_GR : C_UP); fill(); }
        }
        bar(MX + 61, 9, val[kOutOutL], C_OUT); bar(MX + 72, 9, val[kOutOutR], C_OUT);
        fontSize(7.5f); textLetterSpacing(1.2f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(C_FAINT);
        text(MX + 20, MY + 14, "IN", nullptr); text(MX + 46.5f, MY + 14, "GR", nullptr); text(MX + 71, MY + 14, "OUT", nullptr);
        const float gmax = std::fabs(val[kOutGrL]) >= std::fabs(val[kOutGrR]) ? val[kOutGrL] : val[kOutGrR];
        char b[24]; snprintf(b, sizeof b, gmax >= 0 ? "-%.1f" : "+%.1f", std::fabs(gmax)); fontSize(9); fillColor(gmax >= 0 ? C_GR : C_UP); text(MX + 46.5f, MY + MH - 10, b, nullptr);
        // loudness
        const float lx = MX + 96;
        beginPath(); moveTo(lx - 6, MY + 10); lineTo(lx - 6, MY + MH - 10); strokeColor(C_LINE); strokeWidth(1); stroke();
        fontSize(7.5f); textLetterSpacing(1.4f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(C_FAINT); text(lx, MY + 14, "LOUDNESS", nullptr);
        struct Row { const char* k; uint32_t p; const char* u; } rows[4] = { { "M", kOutLoudM, "LUFS" }, { "S", kOutLoudS, "LUFS" }, { "I", kOutLoudI, "LUFS" }, { "TP", kOutTruePeak, "dBTP" } };
        for (int r = 0; r < 4; ++r) {
            const float y = MY + 46 + r * 52;
            fontSize(8); textLetterSpacing(1.2f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(C_DIM); text(lx, y, rows[r].k, nullptr);
            const float v = val[rows[r].p];
            if (v <= -69.9f) snprintf(b, sizeof b, "--"); else snprintf(b, sizeof b, "%.1f", v);
            fontSize(17); textLetterSpacing(0.4f); fillColor(rows[r].p == kOutTruePeak && v > -1.f ? C_GR : (hoverLoud ? C_TEXT : Color(0xd8, 0xd8, 0xd6))); text(lx, y + 18, b, nullptr);
            fontSize(7); textLetterSpacing(1.f); fillColor(C_FAINT); text(lx + 52, y + 21, rows[r].u, nullptr);
        }
        fontSize(7.5f); textLetterSpacing(1.2f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(hoverLoud ? C_ACC : C_FAINT); text(lx, MY + MH - 12, "CLICK TO RESET", nullptr);
    }

    // sidechain EQ display
    static float qxOfHz(float hz) { return QX + QW * std::log(hz / QFMIN) / std::log(QFMAX / QFMIN); }
    static float qhzOfX(float x) { return QFMIN * std::pow(QFMAX / QFMIN, (x - QX) / QW); }
    static float qyOfDb(float db) { return QY + QH / 2 - db / QDB * (QH / 2 - 8); }
    static float qdbOfY(float y) { return (QY + QH / 2 - y) / (QH / 2 - 8) * QDB; }
    eq::Spec scSpecUI(int b) const {
        eq::Spec s; s.shape = clampi((int)(val[sp(b, kSbShape)] + 0.5f), 0, eq::kNumShapes - 1); s.freq = val[sp(b, kSbFreq)]; s.gainDb = val[sp(b, kSbGain)]; s.q = val[sp(b, kSbQ)];
        static const int sl[17] = { 6, 12, 24, 48, 18, 30, 36, 42, 54, 60, 66, 72, 78, 84, 90, 96, eq::kBrickwall };
        const int idx = clampi((int)(val[sp(b, kSbSlope)] + 0.5f), 0, 16); s.slopeDb = eq::isCut(s.shape) ? sl[idx] : (sl[idx] == eq::kBrickwall ? 96 : sl[idx]);
        return s;
    }
    float scDotDb(int b) const { const int sh = clampi((int)(val[sp(b, kSbShape)] + 0.5f), 0, 9); return eq::hasGain(sh) ? val[sp(b, kSbGain)] : 20.f * std::log10(val[sp(b, kSbQ)] / 0.70710678f); }
    void drawScEq() {
        displayBox(QX, QY, QW, QH);
        const float fs = (float)(getSampleRate() > 0 ? getSampleRate() : 48000.0);
        beginPath();
        for (float g : { 50.f, 100.f, 200.f, 500.f, 1000.f, 2000.f, 5000.f, 10000.f }) { const float x = qxOfHz(g); moveTo(x, QY + 4); lineTo(x, QY + QH - 4); }
        for (float db : { -12.f, 12.f }) { moveTo(QX + 4, qyOfDb(db)); lineTo(QX + QW - 4, qyOfDb(db)); }
        strokeColor(Color(255, 255, 255, 8 / 255.f)); strokeWidth(1); stroke();
        beginPath(); moveTo(QX + 4, qyOfDb(0)); lineTo(QX + QW - 4, qyOfDb(0)); strokeColor(Color(255, 255, 255, 20 / 255.f)); stroke();
        fontSize(7.5f); textLetterSpacing(0.6f); textAlign(ALIGN_CENTER | ALIGN_BOTTOM); fillColor(C_FAINT);
        const char* gl[] = { "100", "1k", "10k" }; const float gv[] = { 100, 1000, 10000 };
        for (int i = 0; i < 3; ++i) text(qxOfHz(gv[i]), QY + QH - 2, gl[i], nullptr);
        eq::Design d[kScBands]; bool on[kScBands];
        for (int b = 0; b < kScBands; ++b) { on[b] = val[sp(b, kSbUsed)] > 0.5f && val[sp(b, kSbOn)] > 0.5f; if (val[sp(b, kSbUsed)] > 0.5f) eq::design(scSpecUI(b), fs, d[b]); }
        scissor(QX + 1, QY + 1, QW - 2, QH - 2);
        const int N = 200;
        for (int b = 0; b < kScBands; ++b) {
            if (val[sp(b, kSbUsed)] < 0.5f) continue;
            beginPath();
            for (int i = 0; i <= N; ++i) { const float x = QX + QW * i / N, f = qhzOfX(x); const float y = qyOfDb(20.f * std::log10(std::max(1e-6f, std::abs(eq::response(d[b], f))))); if (!i) moveTo(x, y); else lineTo(x, y); }
            strokeColor(withAlpha(scBandColor(b), on[b] ? (b == scSel ? 0.9f : 0.4f) : 0.15f)); strokeWidth(1); stroke();
        }
        const bool eqOn = val[kScEqOn] > 0.5f;
        beginPath();
        for (int i = 0; i <= N; ++i) {
            const float x = QX + QW * i / N, f = qhzOfX(x); eq::cpx h(1, 0);
            if (eqOn) for (int b = 0; b < kScBands; ++b) if (on[b]) h *= eq::response(d[b], f);
            float db = 20.f * std::log10(std::max(1e-6f, std::abs(h)));
            if (val[kScHpOn] > 0.5f) { const float r = f / std::max(10.f, val[kScHpFreq]); db += 10.f * std::log10(std::pow(r, 8.f) / (1.f + std::pow(r, 8.f))); }
            if (val[kScLpOn] > 0.5f) { const float r = f / std::max(10.f, val[kScLpFreq]); db -= 10.f * std::log10(1.f + std::pow(r, 8.f)); }
            const float y = qyOfDb(std::max(db, -40.f)); if (!i) moveTo(x, y); else lineTo(x, y);
        }
        strokeColor(eqOn ? C_ACC : C_DIM); strokeWidth(2); stroke();
        resetScissor();
        for (int b = 0; b < kScBands; ++b) {
            if (val[sp(b, kSbUsed)] < 0.5f) continue;
            const float x = qxOfHz(val[sp(b, kSbFreq)]), y = qyOfDb(clampf(scDotDb(b), -QDB, QDB));
            const bool sel = b == scSel, hot = b == scHover;
            if (sel || hot) { beginPath(); circle(x, y, 12); fillColor(withAlpha(scBandColor(b), 0.2f)); fill(); }
            beginPath(); circle(x, y, 8); fillColor(on[b] ? scBandColor(b) : C_PANEL_HI); fill(); if (sel) { strokeColor(C_TEXT); strokeWidth(1.6f); stroke(); }
            char t[4]; snprintf(t, sizeof t, "%d", b + 1); fontSize(8.5f); textLetterSpacing(0); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(on[b] ? C_BG : scBandColor(b)); text(x, y + 0.5f, t, nullptr);
        }
        caption(QX, QY - 8, selBand() >= 0 ? "SIDECHAIN EQ" : "SIDECHAIN EQ  ·  DOUBLE-CLICK TO ADD A BAND", ALIGN_LEFT);
        const int sb = selBand();
        if (sb >= 0) {
            char f[24], g[24], q[24]; fmt(sp(sb, kSbFreq), f, sizeof f); fmt(sp(sb, kSbGain), g, sizeof g); fmt(sp(sb, kSbQ), q, sizeof q);
            std::string line = std::string("BAND ") + std::to_string(sb + 1) + "   " + f + "   " + g + "   Q " + q;
            fontSize(9); textLetterSpacing(0.6f); textAlign(ALIGN_RIGHT | ALIGN_MIDDLE); fillColor(scBandColor(sb)); text(QX + QW, QY - 8, line.c_str(), nullptr);
        } else { fontSize(9); textLetterSpacing(0.6f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(C_FAINT); text(EX + 20, PY + 262, "Click a band's dot to change its shape, slope and channel", nullptr); }
    }
    void drawTriggerInfo() {
        const float lv = val[kOutTrigger];
        beginPath(); circle(EX + 360, PY + 72, 7); fillColor(lv > 0.01f ? withAlpha(C_ACC, 0.4f + 0.6f * lv) : C_PANEL_HI); fill(); strokeColor(C_LINE); strokeWidth(1); stroke();
        fontSize(8); textLetterSpacing(1.4f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(C_FAINT); text(EX + 374, PY + 72, "TRIGGER", nullptr);
        fontSize(10); textLetterSpacing(0.3f); textLineHeight(1.3f); textAlign(ALIGN_LEFT | ALIGN_TOP); fillColor(C_FAINT);
        textBox(EX + 20, PY + 260, EW - 40, "MIDI NOTES: route a MIDI track to Gneiss; each held note drives the compressor at its velocity. "
                                           "TEMPO PULSE: pumps in time with your song, no sidechain needed. Both add to the audio trigger.", nullptr);
        textLineHeight(1.f);
    }
    void drawCharacterInfo() {
        const int m = (int)(val[kCharacter] + 0.5f);
        static const char* const d[4] = { "Off: perfectly clean.", "WARM: tube-like saturation, mostly even harmonics. Adds body and density.",
            "PUNCHY: rounds off peaks with odd harmonics, like a transformer pushed hard. Adds bite.", "VINTAGE: warm saturation with a little low-end weight, a softened top and slow analog drift (the threshold and colour wander very slightly)." };
        fontSize(10.5f); textLetterSpacing(0.3f); textLineHeight(1.35f); textAlign(ALIGN_LEFT | ALIGN_TOP); fillColor(C_DIM);
        textBox(EX + 20, PY + 186, EW - 40, d[clampi(m, 0, 3)], nullptr);
        fontSize(9.5f); fillColor(C_FAINT); textBox(EX + 20, PY + 262, EW - 40, "Drive changes the colour, not the level. Use oversampling (STEREO panel) to keep heavy drive free of aliasing.", nullptr);
        textLineHeight(1.f);
    }

    void drawKnobBody(float x, float y, float r, float n, bool hot, Color col, bool dim, bool bipolar = false) {
        const float a0 = 0.75f * 3.14159265f, a1 = 2.25f * 3.14159265f, ang = a0 + (a1 - a0) * n;
        const float tw = r > 30 ? 5.f : (r > 18 ? 4.f : 3.f);
        beginPath(); arc(x, y, r, a0, a1, NanoVG::CW); strokeColor(C_LINE); strokeWidth(tw); lineCap(ROUND); stroke();
        const float from = bipolar ? (a0 + a1) / 2 : a0;
        if (std::fabs(ang - from) > 0.001f) { beginPath(); arc(x, y, r, std::min(from, ang), std::max(from, ang), NanoVG::CW); strokeColor(dim ? C_FAINT : col); stroke(); }
        const float cr = r - (r > 30 ? 10 : (r > 18 ? 8 : 6));
        beginPath(); circle(x, y, cr);
        fillPaint(radialGradient(x - cr * 0.3f, y - cr * 0.4f, 0, cr * 1.4f, hot ? Color(0x3c, 0x3e, 0x44) : C_PANEL_HI, Color(0x14, 0x15, 0x17)));
        fill(); strokeColor(Color(255, 255, 255, 18 / 255.f)); strokeWidth(1); stroke();
        beginPath(); moveTo(x + std::cos(ang) * cr * 0.35f, y + std::sin(ang) * cr * 0.35f); lineTo(x + std::cos(ang) * cr * 0.9f, y + std::sin(ang) * cr * 0.9f);
        strokeColor(dim ? C_DIM : C_TEXT); strokeWidth(r > 30 ? 3.f : 2.f); lineCap(ROUND); stroke();
    }
    void drawSwitch(float x, float y, bool on, const char* label, bool dim, bool hot, Color tc) {
        const float w = 30, h = 16, x0 = x - w / 2, y0 = y - h / 2;
        beginPath(); roundedRect(x0, y0, w, h, h / 2); fillColor(on ? withAlpha(tc, dim ? 0.1f : 0.25f) : C_PANEL_HI); fill(); strokeColor(on && !dim ? tc : C_LINE); strokeWidth(1); stroke();
        beginPath(); circle(on ? x0 + w - h / 2 : x0 + h / 2, y, 5.5f); fillColor(dim ? C_FAINT : (on ? tc : C_DIM)); fill();
        if (label && *label) { fontSize(9); textLetterSpacing(1.6f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(dim ? C_FAINT : (on || hot ? C_TEXT : C_DIM)); text(x + w / 2 + 8, y + 0.5f, label, nullptr); }
    }
    void drawCtl(const Ctl& c, bool hot) {
        const uint32_t p = effParam(c); if (p >= kParamCount) return;
        const bool dim = dimmed(c); char buf[48];
        switch (c.type) {
        case Knob: {
            float n = toNorm(p, val[p]);
            drawKnobBody(c.x, c.y, c.r, n, hot, C_ACC, dim, p == kGain);
            const bool big = c.r >= 28;
            textAlign(ALIGN_CENTER | ALIGN_TOP);
            if (c.label[0]) { fontSize(big ? 9.f : 7.5f); textLetterSpacing(big ? 1.8f : 1.3f); fillColor(dim ? C_FAINT : C_DIM); text(c.x, c.y + c.r + (big ? 6 : 4), c.label, nullptr); }
            fmt(p, buf, sizeof buf); fontSize(big ? 11.5f : 9.5f); textLetterSpacing(0.4f); fillColor(dim ? C_FAINT : (hot ? C_ACC : C_TEXT));
            text(c.x, c.y + c.r + (c.label[0] ? (big ? 19 : 14) : 4), buf, nullptr);
            break; }
        case Switch: {
            const bool alert = (p == kScListen || p == kDelta) && val[p] > 0.5f;
            drawSwitch(c.x, c.y, val[p] > 0.5f, c.label, dim, hot, alert ? C_GR : C_ACC); break; }
        case Segment: case StyleGrid: {
            int n = 0; const char* const* names = segLabels(p, n);
            const int cur = (int)(val[p] + .5f);
            const int count = c.segN, first = c.seg0;
            const bool grid = c.type == StyleGrid;
            const float sw = grid ? c.r : c.r / count, x0 = grid ? c.x : c.x - c.r / 2;
            if (!grid && c.label[0]) caption(c.x, c.y - SEG_H / 2 - 8, c.label);
            for (int i = 0; i < count; ++i) {
                const int v = first + i;
                const float x = grid ? x0 + (i % 7) * sw : x0 + sw * i, y = grid ? c.y + (i / 7) * 28.f + SEG_H / 2 : c.y;
                const bool on = v == cur;
                beginPath(); roundedRect(x + 1.5f, y - SEG_H / 2, sw - 3, SEG_H, 5); fillColor(on ? (dim ? C_FAINT : C_ACC) : C_PANEL_HI); fill();
                if (!on && hot && hoverSeg == i) { strokeColor(C_DIM); strokeWidth(1); stroke(); }
                fontSize(8.5f); textLetterSpacing(1.2f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(on ? C_BG : (dim ? C_FAINT : C_DIM)); text(x + sw / 2, y + 0.5f, names[v], nullptr);
            }
            break; }
        case Stepper: case ScStepper: {
            const float x0 = c.x - c.r / 2, y0 = c.y - SEG_H / 2;
            if (c.label[0]) caption(c.x, c.y - SEG_H / 2 - 8, c.label);
            beginPath(); roundedRect(x0, y0, c.r, SEG_H, 5); fillColor(C_PANEL_HI); fill(); strokeColor(hot && !dim ? C_DIM : C_LINE); strokeWidth(1); stroke();
            for (int s2 = 0; s2 < 2; ++s2) {
                const float cx = s2 == 0 ? x0 + 10 : x0 + c.r - 10, d = s2 == 0 ? -1.f : 1.f;
                beginPath(); moveTo(cx - 3 * d, c.y - 5); lineTo(cx + 3 * d, c.y); lineTo(cx - 3 * d, c.y + 5); closePath(); fillColor(dim ? C_FAINT : (hot && hoverSeg == s2 ? C_ACC : C_DIM)); fill();
            }
            fmt(p, buf, sizeof buf);
            std::string t = buf; for (char& ch : t) ch = (char)std::toupper((unsigned char)ch);
            fontSize(8.5f); textLetterSpacing(1.f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(dim ? C_FAINT : (c.type == ScStepper ? scBandColor(selBand()) : C_TEXT)); text(c.x, c.y + 0.5f, t.c_str(), nullptr);
            break; }
        }
        if (learnMode) {
            const int cc = paramCC[p]; const bool waiting = learnWaiting == (int)p;
            if (cc >= 0 || waiting) {
                const float by = c.type == Knob ? c.y - c.r - 10 : c.y - SEG_H / 2 - 7;
                snprintf(buf, sizeof buf, waiting ? "?" : "CC%d", cc);
                fontSize(8.5f); textLetterSpacing(0.5f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
                const float bw = waiting ? 16 : 34, bx = c.type == StyleGrid ? KX + 360 : c.x;
                beginPath(); roundedRect(bx - bw / 2, by - 7, bw, 14, 3); fillColor(waiting ? C_ACC : C_PANEL_HI); fill(); strokeColor(C_ACC); strokeWidth(1); stroke();
                fillColor(waiting ? C_BG : C_ACC); text(bx, by + 0.5f, buf, nullptr);
            }
        }
    }
    void drawStatus() {
        std::string msg; bool alert = false;
        if (flashMsg.active()) msg = flashMsg.text;
        else if (learnMode) msg = learnWaiting >= 0 ? std::string("Move a MIDI controller to assign: ") + paramDef(learnWaiting).name : "Click a control, then move a MIDI controller.  Right-click unassigns.";
        else if (val[kScListen] > 0.5f) { msg = "AUDITION TRIGGER is on: you hear what the compressor listens to"; alert = true; }
        else if (val[kDelta] > 0.5f) { msg = "AUDITION GAIN CHANGE is on: you hear what the compressor takes away"; alert = true; }
        if (!msg.empty()) { fontSize(10); textLetterSpacing(0.8f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(alert ? C_GR : C_ACC); text(LX + 230, LY + 16, msg.c_str(), nullptr); }
        if (learnMode && learnWaiting < 0) { fontSize(9); textAlign(ALIGN_RIGHT | ALIGN_MIDDLE); fillColor(hoverClear ? C_TEXT : C_DIM); text(LX + LW - 12, LY + LH - 30, "CLEAR ALL ASSIGNMENTS", nullptr); }
    }
    void flash(const std::string& s) { flashMsg.show(s); repaint(); }
    void drawEditBox() {
        const Ctl& c = controls()[editing];
        const float w = 90, h = 22, x = c.x - w / 2, y = c.y + c.r + 4;
        beginPath(); roundedRect(x, y, w, h, 4); fillColor(C_DISPLAY); fill(); strokeColor(C_ACC); strokeWidth(1); stroke();
        fontSize(12); textLetterSpacing(0.5f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(C_TEXT); text(c.x, y + h / 2 + 0.5f, (editBuf + "|").c_str(), nullptr);
    }

    // ---- presets ------------------------------------------------------------------
    bool matchesValues(const float* v) const {
        for (uint32_t i = 0; i < kParamCount; ++i) {
            if (!isSetting(i)) continue;
            if (std::fabs(val[i] - v[i]) > 1e-4f * std::max(1.f, std::fabs(v[i]))) return false;
        }
        return true;
    }
    void applyValues(const float* v) { beginGesture(); applyAll(v); endGesture(); }
    void applyFactory(int idx) { idx = (idx % (int)kNumPresets + (int)kNumPresets) % (int)kNumPresets; applyValues(presets()[idx].v); curFactory = idx; curUser.clear(); repaint(); }
    void applyUser(const std::string& name) {
        float v[kParamCount]; std::copy(defv, defv + kParamCount, v);
        if (!store.load(name, pparams, kParamCount, v)) return;
        applyValues(v); std::copy(v, v + kParamCount, userVals); curUser = name; curFactory = -1; repaint();
    }
    void programLoaded(uint32_t index) override { curFactory = (int)index; curUser.clear(); repaint(); }
    void step(int d) {
        const int nf = (int)kNumPresets, nu = (int)userPresets.size(), total = nf + nu;
        int cur = curFactory; if (!curUser.empty()) for (int i = 0; i < nu; ++i) if (userPresets[i] == curUser) cur = nf + i;
        if (cur < 0) cur = 0;
        const int next = ((cur + d) % total + total) % total;
        if (next < nf) applyFactory(next); else applyUser(userPresets[next - nf]);
    }
    void drawPresetBar() {
        const float x = PS_X, y = PS_Y, w = PS_W, h = PS_H, a = PS_ARROW;
        beginPath(); roundedRect(x, y, w, h, 5); fillColor(C_PANEL); fill(); strokeColor(menuOpen ? C_ACC : C_LINE); strokeWidth(1); stroke();
        beginPath(); moveTo(x + a, y + 5); lineTo(x + a, y + h - 5); moveTo(x + w - a, y + 5); lineTo(x + w - a, y + h - 5); strokeColor(C_LINE); stroke();
        for (int side = 0; side < 2; ++side) {
            const bool hot = presetHover == (side == 0 ? 0 : 2);
            const float cx = side == 0 ? x + a / 2 : x + w - a / 2, cy = y + h / 2, d = side == 0 ? -1.f : 1.f;
            beginPath(); moveTo(cx - 3 * d, cy - 5); lineTo(cx + 3 * d, cy); lineTo(cx - 3 * d, cy + 5); closePath(); fillColor(hot ? C_ACC : C_DIM); fill();
        }
        std::string name; bool modified = false;
        if (!curUser.empty()) { name = curUser; modified = !matchesValues(userVals); }
        else if (curFactory >= 0) { name = presets()[curFactory].name; modified = !matchesValues(presets()[curFactory].v); }
        else name = "Custom";
        if (modified) name += " *";
        fontSize(12); textLetterSpacing(1.f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(presetHover == 1 || menuOpen ? C_TEXT : Color(0xd2, 0xd2, 0xd0));
        text(x + w / 2 - 6, y + h / 2 + 0.5f, name.c_str(), nullptr);
        const float cx = x + w - a - 12, cy = y + h / 2;
        beginPath(); moveTo(cx - 4, cy - 2); lineTo(cx + 4, cy - 2); lineTo(cx, cy + 3); closePath(); fillColor(C_DIM); fill();
    }
    struct MRect { float x, y, w, h; int kind; int idx; };
    void menuLayout(float& mx, float& my, float& mw, float& mh, std::vector<MRect>& items) const {
        const int cols = kNumCategories + 1;
        mw = PM_COLW * cols + 2 * PM_PAD; mx = std::min(W - mw - 8, std::max(8.f, PS_X + PS_W / 2 - mw / 2)); my = PS_Y + PS_H + 4;
        items.clear();
        const float top = my + PM_PAD + PM_ROW + 6 + PM_HEAD;
        int rows[kNumCategories + 1] = {}; int maxRows = 1;
        for (uint32_t i = 0; i < kNumPresets; ++i) {
            const char* cat = presets()[i].category;
            if (std::strcmp(cat, "Init") == 0) { items.push_back({ mx + PM_PAD, my + PM_PAD, PM_COLW - 4, PM_ROW, 0, (int)i }); continue; }
            for (int c = 0; c < kNumCategories; ++c)
                if (std::strcmp(cat, kCategories[c]) == 0) { items.push_back({ mx + PM_PAD + c * PM_COLW, top + rows[c] * PM_ROW, PM_COLW - 4, PM_ROW, 0, (int)i }); maxRows = std::max(maxRows, ++rows[c]); }
        }
        for (int u = 0; u < (int)userPresets.size() && u < 20; ++u) items.push_back({ mx + PM_PAD + kNumCategories * PM_COLW, top + u * PM_ROW, PM_COLW - 4, PM_ROW, 1, u });
        maxRows = std::max(maxRows, std::min(20, (int)userPresets.size()));
        items.push_back({ mx + mw - PM_PAD - 150, my + PM_PAD, 150, PM_ROW, 2, 0 });
        mh = PM_PAD + PM_ROW + 6 + PM_HEAD + maxRows * PM_ROW + PM_PAD;
    }
    void drawPresetMenu() {
        float mx, my, mw, mh; std::vector<MRect> items; menuLayout(mx, my, mw, mh, items);
        beginPath(); rect(mx - 12, my - 4, mw + 24, mh + 20); fillPaint(boxGradient(mx, my + 2, mw, mh, 6, 14, Color(0, 0, 0, 0.6f), Color(0, 0, 0, 0.f))); fill();
        beginPath(); roundedRect(mx, my, mw, mh, 6); fillColor(C_PANEL_HI); fill(); strokeColor(C_LINE); strokeWidth(1); stroke();
        beginPath(); moveTo(mx + PM_PAD, my + PM_PAD + PM_ROW + 3); lineTo(mx + mw - PM_PAD, my + PM_PAD + PM_ROW + 3); strokeColor(C_LINE); stroke();
        fontSize(8.5f); textLetterSpacing(2.f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
        for (int c = 0; c <= kNumCategories; ++c) {
            std::string up = c < kNumCategories ? kCategories[c] : "User"; for (char& ch : up) ch = (char)std::toupper((unsigned char)ch);
            fillColor(c < kNumCategories ? C_ACC : C_OUT); text(mx + PM_PAD + c * PM_COLW + 12, my + PM_PAD + PM_ROW + 6 + PM_HEAD / 2, up.c_str(), nullptr);
        }
        if (userPresets.empty()) { fontSize(10); textLetterSpacing(0.4f); fillColor(C_FAINT); text(mx + PM_PAD + kNumCategories * PM_COLW + 12, my + PM_PAD + PM_ROW + 6 + PM_HEAD + PM_ROW / 2, "(none saved yet)", nullptr); }
        for (int i = 0; i < (int)items.size(); ++i) {
            const MRect& r = items[i]; const bool hot = i == menuHover;
            if (r.kind == 2) {
                beginPath(); roundedRect(r.x, r.y + 1, r.w, r.h - 2, 4); fillColor(hot ? withAlpha(C_OUT, 0.25f) : C_PANEL); fill(); strokeColor(C_OUT); strokeWidth(1); stroke();
                fontSize(9.5f); textLetterSpacing(1.6f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(C_TEXT); text(r.x + r.w / 2, r.y + r.h / 2 + 0.5f, "SAVE CURRENT AS...", nullptr);
                continue;
            }
            const bool isCur = r.kind == 0 ? (curUser.empty() && r.idx == curFactory) : (curUser == userPresets[r.idx]);
            const bool del = r.kind == 1 && pendingDelete == r.idx;
            if (hot || del) { beginPath(); roundedRect(r.x, r.y, r.w, r.h, 4); fillColor(del ? Color(0xd0, 0x50, 0x40, 0.35f) : withAlpha(C_ACC, 0.18f)); fill(); }
            if (isCur) { beginPath(); circle(r.x + 6, r.y + r.h / 2, 2.5f); fillColor(r.kind == 0 ? C_ACC : C_OUT); fill(); }
            fontSize(11); textLetterSpacing(0.4f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(hot || isCur || del ? C_TEXT : C_DIM);
            const std::string label = del ? "Delete? click again" : (r.kind == 0 ? std::string(presets()[r.idx].name) : userPresets[r.idx]);
            text(r.x + 12, r.y + r.h / 2 + 0.5f, label.c_str(), nullptr);
        }
    }
    int menuHit(float x, float y) const { float mx, my, mw, mh; std::vector<MRect> it; menuLayout(mx, my, mw, mh, it);
        for (int i = 0; i < (int)it.size(); ++i) if (x >= it[i].x && x < it[i].x + it[i].w && y >= it[i].y && y < it[i].y + it[i].h) return i;
        return -1; }
    MRect menuItem(int i) const { float mx, my, mw, mh; std::vector<MRect> it; menuLayout(mx, my, mw, mh, it); return it[i]; }
    void drawSaveBox() {
        const float w = 360, h = 96, x = W / 2 - w / 2, y = 130;
        beginPath(); rect(0, 0, W, H); fillColor(Color(0, 0, 0, 0.45f)); fill();
        beginPath(); roundedRect(x, y, w, h, 8); fillColor(C_PANEL_HI); fill(); strokeColor(C_OUT); strokeWidth(1); stroke();
        fontSize(9.5f); textLetterSpacing(2.f); textAlign(ALIGN_LEFT | ALIGN_TOP); fillColor(C_DIM); text(x + 16, y + 14, "SAVE PRESET AS", nullptr);
        beginPath(); roundedRect(x + 16, y + 36, w - 32, 26, 4); fillColor(C_DISPLAY); fill(); strokeColor(C_LINE); stroke();
        fontSize(13); textLetterSpacing(0.4f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(C_TEXT); text(x + 26, y + 49.5f, (saveName + "|").c_str(), nullptr);
        fontSize(8.5f); textLetterSpacing(1.2f); fillColor(C_FAINT); textAlign(ALIGN_RIGHT | ALIGN_TOP);
        text(x + w - 16, y + 72, saveError ? "COULD NOT SAVE - CHECK THE NAME" : "ENTER TO SAVE   ·   ESC TO CANCEL", nullptr);
    }

    // ---- idle: level history, tips ----------------------------------------------------------
    void uiIdle() override {
        const double t = nowSec();
        if (t - lastHist > 0.04) {
            lastHist = t;
            hIn[hPos] = std::max(val[kOutInL], val[kOutInR]); hOut[hPos] = std::max(val[kOutOutL], val[kOutOutR]);
            hGr[hPos] = std::fabs(val[kOutGrL]) >= std::fabs(val[kOutGrR]) ? val[kOutGrL] : val[kOutGrR]; hTrig[hPos] = val[kOutTrigger];
            hPos = (hPos + 1) % HIST; repaint();
        }
        if (tips.due(gTipsOn, active >= 0 || dragThr || dragCurve || scDrag >= 0 || menuOpen || saving || help.open || editing >= 0)) repaint();
        if (flashMsg.until > 0 && !flashMsg.active()) { flashMsg.clear(); repaint(); }
    }
    bool tipContent(std::string& title, std::string& value, std::string& body) const {
        int k = tips.key, seg = -1;
        if (k < 0) return false;
        if (k >= 100000) { seg = (k - 100000) % 32; k = (k - 100000) / 32; }
        if (k < 1000) {
            const Ctl& c = controls()[k]; const uint32_t p = effParam(c); if (p >= kParamCount) return false;
            char b[40]; fmt(p, b, sizeof b);
            title = paramDef(p).name; value = b; body = paramTip(p);
            if (c.type == StyleGrid && seg >= 0) { body = styleTip(seg); title = "Style"; int ne; value = enumLabels(kStyle, ne)[seg]; }
            if (c.type == Knob) body += "\n\nDrag up/down or grab the rim and turn. Double-click to type a value, Cmd/Ctrl-click to reset.";
            if (paramCC[p] >= 0) { char cc[32]; snprintf(cc, sizeof cc, "   ·   CC%d", paramCC[p]); value += cc; }
            return true;
        }
        int tt = -1;
        if (k >= 6000) tt = k == 6000 ? kTipTabs : (k == 6001 ? kTipScEq : (k == 6002 ? kTipScDot : (k == 6003 ? kTipLoudness : kTipTrigger)));
        else if (k >= 4000) tt = kTipLevelDisplay + (k - 4000);
        else if (k >= 2000) tt = kTipPrev + (k - 2000);
        else { const int hh = k - 1000;
            tt = hh == 1 ? kTipLearn : hh == 2 ? kTipMidi : hh == 3 ? kTipUndo : hh == 4 ? kTipRedo : hh == 5 ? kTipHelp : hh == 6 ? kTipA : hh == 7 ? kTipB : kTipCopy; }
        title = targetTitle(tt); value.clear(); body = targetTip(tt);
        if (tt == kTipUndo || tt == kTipRedo) { const size_t n = tt == kTipUndo ? history.undoSteps() : history.redoSteps(); char b[32]; snprintf(b, sizeof b, "%d step%s", (int)n, n == 1 ? "" : "s"); value = b; }
        return true;
    }

    // ---- interaction --------------------------------------------------------------
    int hit(float x, float y, int* seg = nullptr) const {
        const std::vector<Ctl>& cs = controls();
        for (int i = 0; i < (int)cs.size(); ++i) {
            const Ctl& c = cs[i];
            if (!visible(c)) continue;
            switch (c.type) {
            case Knob: { const float rr = c.r + 6; if ((x - c.x) * (x - c.x) + (y - c.y) * (y - c.y) <= rr * rr) return i; break; }
            case Switch: if (x >= c.x - 16 && x <= c.x + 100 && std::fabs(y - c.y) <= 10) return i; break;
            case StyleGrid: for (int k = 0; k < c.segN; ++k) { const float sx = c.x + (k % 7) * c.r, sy = c.y + (k / 7) * 28.f; if (x >= sx && x < sx + c.r && y >= sy && y < sy + SEG_H) { if (seg) *seg = k; return i; } } break;
            case Segment: { const float x0 = c.x - c.r / 2;
                if (x >= x0 && x < x0 + c.r && std::fabs(y - c.y) <= SEG_H / 2 + 1) { if (seg) *seg = clampi((int)((x - x0) / (c.r / c.segN)), 0, c.segN - 1); return i; } break; }
            case Stepper: case ScStepper: { const float x0 = c.x - c.r / 2; if (x >= x0 && x < x0 + c.r && std::fabs(y - c.y) <= SEG_H / 2 + 1) { if (seg) *seg = x < c.x ? 0 : 1; return i; } break; }
            }
        }
        return -1;
    }
    static bool inRect(float x, float y, float rx, float ry, float rw, float rh) { return x >= rx && x < rx + rw && y >= ry && y < ry + rh; }
    int headerHit(float x, float y) const {
        if (y < PS_Y || y > PS_Y + PS_H) return 0;
        if (x >= LEARN_X && x <= LEARN_X + LEARN_W) return 1;
        if (x >= MIDI_X && x <= MIDI_X + MIDI_W) return 2;
        if (x >= UNDO_X && x <= UNDO_X + UR_W) return 3;
        if (x >= REDO_X && x <= REDO_X + UR_W) return 4;
        if (x >= HELP_X && x <= HELP_X + HELP_W) return 5;
        if (x >= A_X && x < A_X + AB_W) return 6;
        if (x >= A_X + AB_W && x < A_X + 2 * AB_W) return 7;
        if (x >= COPY_X && x <= COPY_X + COPY_W) return 8;
        return 0;
    }
    int presetBarHit(float x, float y) const {
        if (y < PS_Y || y > PS_Y + PS_H || x < PS_X || x > PS_X + PS_W) return -1;
        return x < PS_X + PS_ARROW ? 0 : (x > PS_X + PS_W - PS_ARROW ? 2 : 1);
    }
    int tabHit(float x, float y) const {
        if (val[kExpert] < 0.5f || y < TAB_Y || y > TAB_Y + TAB_H) return -1;
        const float tw = (EW - 24) / kNumTabs;
        for (int t = 0; t < kNumTabs; ++t) if (x >= EX + 12 + t * tw && x < EX + 12 + (t + 1) * tw) return t;
        return -1;
    }
    bool inScEq(float x, float y) const { return val[kExpert] > 0.5f && tab == TabSidechain && inRect(x, y, QX, QY, QW, QH); }
    int scDotHit(float x, float y) const {
        if (!inScEq(x, y)) return -1;
        int best = -1; float bd = 12 * 12;
        for (int b = 0; b < kScBands; ++b) { if (val[sp(b, kSbUsed)] < 0.5f) continue; const float dx = x - qxOfHz(val[sp(b, kSbFreq)]), dy = y - qyOfDb(clampf(scDotDb(b), -QDB, QDB)); if (dx * dx + dy * dy < bd) { bd = dx * dx + dy * dy; best = b; } }
        return best;
    }
    bool clearHit(float x, float y) const { return learnMode && learnWaiting < 0 && inRect(x, y, LX + LW - 190, LY + LH - 40, 180, 20); }
    bool loudHit(float x, float y) const { return inRect(x, y, MX + 90, MY, MW - 90, MH); }
    int areaHit(float x, float y) const {
        if (inRect(x, y, LX, LY, LW, LH)) return 0;
        if (inRect(x, y, TX, TY, TW, TH)) return 1;
        if (inRect(x, y, MX, MY, 90, MH)) return 2;
        return -1;
    }

    void setLearn(bool on) { learnMode = on; learnWaiting = -1; if (!on) setState("cmd", "cancel"); repaint(); }
    void endEdit(bool apply) {
        if (editing < 0) return;
        const uint32_t p = effParam(controls()[editing]); float v;
        if (apply && p < kParamCount && parse(p, editBuf, v)) { beginGesture(); commit(p, v); endGesture(); }
        editing = -1; editBuf.clear(); repaint();
    }
    static float angleNorm(const Ctl& c, float x, float y) {
        float a = std::atan2(y - c.y, x - c.x);
        const float a0 = 0.75f * 3.14159265f;
        float rel = a - a0; while (rel < 0) rel += 2 * 3.14159265f;
        if (rel > 1.5f * 3.14159265f) rel = rel > 1.75f * 3.14159265f ? 0.f : 1.5f * 3.14159265f;
        return rel / (1.5f * 3.14159265f);
    }
    void stepScField(int field, int dir) {
        const int b = selBand(); if (b < 0) return;
        const uint32_t p = sp(b, field);
        if (field == kSbSlope) { const bool cut = eq::isCut((int)(val[sp(b, kSbShape)] + 0.5f)); setValue(p, (float)scSlopeByRank(clampi(scSlopeRank((int)(val[p] + 0.5f)) + dir, 0, cut ? 16 : 15))); }
        else { const int n = field == kSbShape ? eq::kNumShapes : 3; setValue(p, (float)(((int)(val[p] + 0.5f) + dir + n) % n)); }
    }
    int addScBand(float hz, float db) {
        int b = -1; for (int i = 0; i < kScBands; ++i) if (val[sp(i, kSbUsed)] < 0.5f) { b = i; break; }
        if (b < 0) { flash("All 6 sidechain EQ bands are in use"); return -1; }
        commit(sp(b, kSbShape), eq::kBell); commit(sp(b, kSbFreq), clampf(hz, QFMIN, QFMAX)); commit(sp(b, kSbGain), clampf(db, -24, 24));
        commit(sp(b, kSbQ), 1); commit(sp(b, kSbSlope), 1); commit(sp(b, kSbPlace), 0); commit(sp(b, kSbOn), 1); commit(sp(b, kSbUsed), 1);
        if (val[kScEqOn] < 0.5f) commit(kScEqOn, 1);
        return b;
    }

    bool onKeyboard(const KeyboardEvent& ev) override {
        if (!ev.press) return false;
        std::string* buf = saving ? &saveName : (editing >= 0 ? &editBuf : nullptr);
        if (!buf) {
            const bool cmd = (ev.mod & (kModifierControl | kModifierSuper)) != 0;
            if (cmd && (ev.key == 'z' || ev.key == 'Z')) { if (ev.mod & kModifierShift) redo(); else undo(); return true; }
            if (cmd && (ev.key == 'y' || ev.key == 'Y')) { redo(); return true; }
            if ((ev.key == kKeyDelete || ev.key == kKeyBackspace) && selBand() >= 0 && val[kExpert] > 0.5f && tab == TabSidechain) { beginGesture(); commit(sp(selBand(), kSbUsed), 0); endGesture(); scSel = -1; return true; }
            if (ev.key == kKeyEscape && help.open) { help.open = false; repaint(); return true; }
            if (ev.key == kKeyEscape && learnMode) { setLearn(false); return true; }
            return false;
        }
        if (ev.key == kKeyEnter) {
            if (saving) {
                if (store.save(saveName, pparams, kParamCount, val)) { userPresets = store.list(); curUser = epsilon::PresetStore::sanitize(saveName); curFactory = -1; std::copy(val, val + kParamCount, userVals); saving = false; saveError = false; }
                else saveError = true;
            } else endEdit(true);
            repaint(); return true;
        }
        if (ev.key == kKeyEscape) { if (saving) saving = false; else endEdit(false); repaint(); return true; }
        if (ev.key == kKeyBackspace || ev.key == kKeyDelete) { if (!buf->empty()) buf->pop_back(); repaint(); return true; }
        const char ch = epsilon::typedChar(ev.key, (ev.mod & kModifierShift) != 0);
        if (ch && buf->size() < (saving ? 40u : 12u)) { *buf += ch; repaint(); return true; }
        return true;
    }

    bool onMouse(const MouseEvent& ev) override {
        const float s = uiScale(), x = ev.pos.getX() / s, y = ev.pos.getY() / s;
        if (saving) return true;
        if (ev.press) { flashMsg.clear(); tips.press(); }
        if (help.open) { if (ev.press && ev.button == 1) { help.click(x, y, gTipsOn); repaint(); } return true; }
        if (ev.button == 2 && ev.press) {
            if (menuOpen) { const int m = menuHit(x, y); if (m >= 0 && menuItem(m).kind == 1) { pendingDelete = menuItem(m).idx; repaint(); } return true; }
            if (learnMode) { const int i = hit(x, y); if (i >= 0) { const uint32_t p = effParam(controls()[i]); if (p < kParamCount) { char b[24]; snprintf(b, sizeof b, "unmap:%u", p); setState("cmd", b); paramCC[p] = -1; repaint(); } } return true; }
            const int d = scDotHit(x, y);
            if (d >= 0) { beginGesture(); commit(sp(d, kSbUsed), 0); endGesture(); if (scSel == d) scSel = -1; return true; }
            return false;
        }
        if (ev.button != 1) return false;
        if (!ev.press) {
            if (active >= 0) { const uint32_t p = effParam(controls()[active]); if (p < kParamCount) editParameter(p, false); active = -1; endGesture(); return true; }
            if (dragThr || dragCurve) { editParameter(kThreshold, false); dragThr = dragCurve = false; endGesture(); return true; }
            if (scDrag >= 0) { editParameter(sp(scDrag, kSbFreq), false); editParameter(sp(scDrag, kSbGain), false); editParameter(sp(scDrag, kSbQ), false); scDrag = -1; endGesture(); return true; }
            return false;
        }
        if (editing >= 0) endEdit(true);
        if (menuOpen) {
            const int m = menuHit(x, y);
            if (m >= 0) {
                const MRect r = menuItem(m);
                if (r.kind == 2) { menuOpen = false; saving = true; saveError = false; saveName = curUser; }
                else if (r.kind == 1 && pendingDelete == r.idx) { store.remove(userPresets[r.idx]); if (curUser == userPresets[r.idx]) curUser.clear(); userPresets = store.list(); pendingDelete = -1; repaint(); return true; }
                else { menuOpen = false; if (r.kind == 0) applyFactory(r.idx); else applyUser(userPresets[r.idx]); }
            } else menuOpen = false;
            pendingDelete = -1; menuHover = -1; repaint(); return true;
        }
        const int hh = headerHit(x, y);
        if (hh == 1) { setLearn(!learnMode); return true; }
        if (hh == 2) { beginGesture(); commit(kMidiOn, val[kMidiOn] > 0.5f ? 0.f : 1.f); endGesture(); return true; }
        if (hh == 3) { undo(); return true; }
        if (hh == 4) { redo(); return true; }
        if (hh == 5) { help.open = true; repaint(); return true; }
        if (hh == 6 || hh == 7) { switchAB(hh - 6); return true; }
        if (hh == 8) { copyAB(); return true; }
        const int pb = presetBarHit(x, y);
        if (pb == 0) { step(-1); return true; }
        if (pb == 2) { step(1); return true; }
        if (pb == 1) { userPresets = store.list(); menuOpen = true; menuHover = -1; pendingDelete = -1; repaint(); return true; }
        if (clearHit(x, y)) { setState("cmd", "clear"); for (int& m : paramCC) m = -1; repaint(); return true; }
        const int th = tabHit(x, y); if (th >= 0) { tab = th; repaint(); return true; }
        if (loudHit(x, y)) { setState("cmd", "loudreset"); flash("Loudness reset"); return true; }

        int seg = 0; const int i = hit(x, y, &seg);
        if (i >= 0) {
            const Ctl& c = controls()[i]; const uint32_t p = effParam(c);
            if (p >= kParamCount) return true;
            if (learnMode) { learnWaiting = (int)p; char b[24]; snprintf(b, sizeof b, "learn:%u", p); setState("cmd", b); repaint(); return true; }
            if (c.type == Switch) { beginGesture(); commit(p, val[p] > 0.5f ? 0.f : 1.f); endGesture(); return true; }
            if (c.type == Segment || c.type == StyleGrid) { beginGesture(); commit(p, (float)(c.seg0 + seg)); endGesture(); return true; }
            if (c.type == Stepper) { beginGesture(); commit(p, clampf(val[p] + (seg == 0 ? -1.f : 1.f), paramDef(p).min, paramDef(p).max)); endGesture(); return true; }
            if (c.type == ScStepper) { beginGesture(); editParameter(p, true); stepScField((int)c.param, seg == 0 ? -1 : 1); editParameter(p, false); endGesture(); return true; }
            if (ev.mod & (kModifierControl | kModifierSuper)) { beginGesture(); commit(p, defv[p]); endGesture(); return true; }
            if (i == lastClick && ev.time - lastClickTime < 350) {
                lastClick = -1; editing = i;
                char b[40]; fmt(p, b, sizeof b); editBuf = b;
                const size_t sp2 = editBuf.find_first_of(" :%"); if (sp2 != std::string::npos) editBuf.resize(sp2);
                repaint(); return true;
            }
            lastClick = i; lastClickTime = ev.time;
            beginGesture();
            active = i; dragY = y; dragNorm = toNorm(p, val[p]);
            circular = c.r > 19 && std::sqrt((x - c.x) * (x - c.x) + (y - c.y) * (y - c.y)) > c.r * 0.62f;
            editParameter(p, true);
            if (circular) setValue(p, fromNorm(p, angleNorm(c, x, y)));
            repaint(); return true;
        }
        if (inScEq(x, y) && !learnMode) {
            const int d = scDotHit(x, y);
            if (d >= 0) {
                if (ev.mod & kModifierAlt) { beginGesture(); commit(sp(d, kSbOn), val[sp(d, kSbOn)] > 0.5f ? 0.f : 1.f); endGesture(); return true; }
                scSel = d; beginGesture(); scDrag = d; scX0 = x; scY0 = y; scF0 = val[sp(d, kSbFreq)]; scG0 = val[sp(d, kSbGain)]; scQ0 = val[sp(d, kSbQ)];
                editParameter(sp(d, kSbFreq), true); editParameter(sp(d, kSbGain), true); editParameter(sp(d, kSbQ), true); repaint(); return true;
            }
            const bool dbl = ev.time - lastScClick < 350 && std::fabs(x - lastScX) < 6 && std::fabs(y - lastScY) < 6;
            lastScClick = ev.time; lastScX = x; lastScY = y;
            if (dbl) { beginGesture(); const int b = addScBand(qhzOfX(x), qdbOfY(y)); endGesture(); if (b >= 0) scSel = b; repaint(); return true; }
            scSel = -1; repaint(); return true;
        }
        if (!learnMode) {
            const int a = areaHit(x, y);
            if (a == 0 || a == 1) {
                beginGesture(); editParameter(kThreshold, true);
                if (a == 0) { dragThr = true; setValue(kThreshold, std::round(clampf(-(y - LY - 10) / (LH - 30) * 60.f, -60.f, 0.f) * 10) / 10); }
                else { dragCurve = true; setValue(kThreshold, std::round(clampf((x - TX - 26) / (TW - 40) * 60.f - 60.f, -60.f, 0.f) * 10) / 10); }
                return true;
            }
        }
        return false;
    }

    bool onMotion(const MotionEvent& ev) override {
        const float s = uiScale(), x = ev.pos.getX() / s, y = ev.pos.getY() / s;
        if (saving) return true;
        if (help.open) { if (help.motion(x, y)) repaint(); return true; }
        if (active >= 0) {
            const Ctl& c = controls()[active]; const uint32_t p = effParam(c);
            if (p >= kParamCount) return true;
            if (circular) { setValue(p, fromNorm(p, angleNorm(c, x, y))); return true; }
            float sens = (ev.mod & kModifierShift) ? 1500.f : 240.f;
            if (paramDef(p).hints & kHintInteger) sens = 80.f;
            dragNorm = clampf(dragNorm + (dragY - y) / sens, 0.f, 1.f); dragY = y;
            setValue(p, fromNorm(p, dragNorm));
            return true;
        }
        if (dragThr) { setValue(kThreshold, std::round(clampf(-(y - LY - 10) / (LH - 30) * 60.f, -60.f, 0.f) * 10) / 10); return true; }
        if (dragCurve) { setValue(kThreshold, std::round(clampf((x - TX - 26) / (TW - 40) * 60.f - 60.f, -60.f, 0.f) * 10) / 10); return true; }
        if (scDrag >= 0) {
            const float fine = (ev.mod & kModifierShift) ? 0.15f : 1.f;
            setValue(sp(scDrag, kSbFreq), clampf(scF0 * qhzOfX(scX0 + (x - scX0) * fine) / qhzOfX(scX0), 10.f, 30000.f));
            const float dDb = qdbOfY(scY0 + (y - scY0) * fine) - qdbOfY(scY0);
            if (eq::hasGain((int)(val[sp(scDrag, kSbShape)] + 0.5f))) setValue(sp(scDrag, kSbGain), clampf(scG0 + dDb, -30.f, 30.f));
            else setValue(sp(scDrag, kSbQ), clampf(scQ0 * std::pow(10.f, dDb / 20.f), 0.1f, 40.f));
            return true;
        }
        if (menuOpen) { const int m = menuHit(x, y); if (m != menuHover) { menuHover = m; repaint(); } return true; }
        const int hh = headerHit(x, y); if (hh != hoverHeader) { hoverHeader = hh; repaint(); }
        const int pb = presetBarHit(x, y); if (pb != presetHover) { presetHover = pb; repaint(); }
        const bool hc = clearHit(x, y); if (hc != hoverClear) { hoverClear = hc; repaint(); }
        const int tb = tabHit(x, y); if (tb != hoverTab) { hoverTab = tb; repaint(); }
        const bool hl = loudHit(x, y); if (hl != hoverLoud) { hoverLoud = hl; repaint(); }
        int seg = -1; const int h = hit(x, y, &seg); if (h != hover || seg != hoverSeg) { hover = h; hoverSeg = seg; repaint(); }
        const int sd = h < 0 ? scDotHit(x, y) : -1; if (sd != scHover) { scHover = sd; repaint(); }
        const int a = h >= 0 ? -1 : areaHit(x, y);
        const bool ht = a == 0 && std::fabs(y - yOfDbL(val[kThreshold])) < 8; if (ht != hoverThr) { hoverThr = ht; repaint(); }
        const bool hcv = a == 1; if (hcv != hoverCurve) { hoverCurve = hcv; repaint(); }
        int key = -1;
        if (h >= 0) key = (controls()[h].type == StyleGrid && seg >= 0) ? 100000 + h * 32 + seg : h;
        else if (hh > 0) key = 1000 + hh;
        else if (pb >= 0) key = 2000 + pb;
        else if (tb >= 0) key = 6000;
        else if (sd >= 0) key = 6002;
        else if (inScEq(x, y)) key = 6001;
        else if (hl) key = 6003;
        else if (a >= 0) key = 4000 + a;
        if (tips.hover(key, x, y)) repaint();
        return false;
    }

    bool onScroll(const ScrollEvent& ev) override {
        const float s = uiScale(), sx = ev.pos.getX() / s, sy = ev.pos.getY() / s;
        if (menuOpen || saving || help.open) return true;
        if (presetBarHit(sx, sy) >= 0) { step(ev.delta.getY() > 0 ? -1 : 1); return true; }
        const float dir = ev.delta.getY() > 0 ? 1.f : -1.f;
        const int d = scDotHit(sx, sy);
        if (d >= 0) { beginGesture(); commit(sp(d, kSbQ), clampf(val[sp(d, kSbQ)] * std::pow(2.f, dir * ((ev.mod & kModifierShift) ? 0.05f : 0.2f)), 0.1f, 40.f)); endGesture(); return true; }
        int seg = 0; const int i = hit(sx, sy, &seg);
        if (i < 0) return false;
        const Ctl& c = controls()[i]; const uint32_t p = effParam(c); if (p >= kParamCount) return false;
        if (c.type == ScStepper) { beginGesture(); stepScField((int)c.param, dir > 0 ? 1 : -1); endGesture(); return true; }
        if (c.type == Stepper) { beginGesture(); commit(p, clampf(val[p] + dir, paramDef(p).min, paramDef(p).max)); endGesture(); return true; }
        if (c.type != Knob) return false;
        const float st = (ev.mod & kModifierShift) ? 0.004f : 0.02f;
        beginGesture(); commit(p, fromNorm(p, toNorm(p, val[p]) + dir * st)); endGesture();
        return true;
    }

private:
    epsilon::Theme theme;
    epsilon::HelpWindow help;
    epsilon::PresetStore store;
    epsilon::PresetParam pparams[kParamCount];
    epsilon::UndoHistory history;
    epsilon::TipTimer tips;
    epsilon::Flash flashMsg;
    float val[kParamCount], defv[kParamCount], userVals[kParamCount] = {};
    int paramCC[kParamCount];
    int active = -1, hover = -1, hoverSeg = -1, lastClick = -1, editing = -1;
    uint lastClickTime = 0, lastScClick = 0; float lastScX = 0, lastScY = 0;
    float dragY = 0, dragNorm = 0; bool circular = false, gesture = false;
    bool dragThr = false, dragCurve = false, hoverThr = false, hoverCurve = false, hoverLoud = false;
    int tab = TabStereo, hoverTab = -1;
    int scSel = -1, scHover = -1, scDrag = -1; float scX0 = 0, scY0 = 0, scF0 = 0, scG0 = 0, scQ0 = 0;
    float hIn[HIST], hOut[HIST], hGr[HIST], hTrig[HIST] = {}; int hPos = 0; double lastHist = 0;
    std::string editBuf;
    int curFactory = 0, presetHover = -1, menuHover = -1, pendingDelete = -1, hoverHeader = 0;
    std::string curUser; std::vector<std::string> userPresets;
    bool menuOpen = false, saving = false, saveError = false; std::string saveName;
    bool learnMode = false, hoverClear = false; int learnWaiting = -1; float lastLearnEcho = 0;
    ResizeHandle resizeHandle;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GneissUI)
};

UI* createUI() { return new GneissUI(); }

END_NAMESPACE_DISTRHO
