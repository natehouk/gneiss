// Gneiss — custom vector UI (DPF/NanoVG). Original design. License: MIT.
#include "DistrhoUI.hpp"
#include "ParamDefs.hpp"
#include "Presets.hpp"
#include "UserPresets.hpp"
#include "ResizeHandle.hpp"
#include "Help.hpp"
#include <chrono>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#if defined(_WIN32)
extern "C" __declspec(dllimport) void* __stdcall ShellExecuteA(void*, const char*, const char*, const char*, const char*, int);
#endif

START_NAMESPACE_DISTRHO

static void openURL(const char* url) {
#if defined(_WIN32)
    ShellExecuteA(nullptr, "open", url, nullptr, nullptr, 1);
#elif defined(__APPLE__)
    const std::string c = std::string("open '") + url + "' &"; if (std::system(c.c_str())) {}
#else
    const std::string c = std::string("xdg-open '") + url + "' >/dev/null 2>&1 &"; if (std::system(c.c_str())) {}
#endif
}
static bool gTipsOn = true;
static double nowSec() { return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); }

using namespace gneiss;
using DGL_NAMESPACE::ResizeHandle;

static constexpr float W = 1040.f, H = 660.f;

#define C_BG        Color(0x10, 0x10, 0x12)
#define C_PANEL     Color(0x19, 0x1a, 0x1d)
#define C_PANEL_HI  Color(0x24, 0x25, 0x29)
#define C_LINE      Color(0x2f, 0x31, 0x36)
#define C_TEXT      Color(0xe8, 0xe8, 0xe6)
#define C_DIM       Color(0x8a, 0x8c, 0x92)
#define C_FAINT     Color(0x4c, 0x4e, 0x55)
#define C_ACC       Color(0xd9, 0xc2, 0x7a)       // mica gold
#define C_OUT       Color(0x8f, 0xd0, 0xb8)       // output level
#define C_GR        Color(0xf0, 0x5a, 0x4a)       // gain reduction
#define C_DISPLAY   Color(0x0b, 0x0b, 0x0d)

static Color withAlpha(const Color& c, float a) { Color r(c); r.alpha = a; return r; }

// ---------------------------------------------------------------- layout
enum WType { Knob, Toggle, Segment };
struct Ctl { WType type; uint32_t param; float x, y, r; const char* label; bool expert; };

static constexpr float LX = 16, LY = 64, LW = 600, LH = 326;          // level display
static constexpr float TX = 624, TY = 64, TW = 300, TH = 326;         // transfer curve
static constexpr float MX = 932, MY = 64, MW = 92, MH = 326;          // meters
static constexpr float PY = 400, PH = 246;                            // bottom panels
static constexpr float KX = 16, KW = 648, EX = 672, EW = 352;
static constexpr float SEG_H = 24;

static std::vector<Ctl> buildControls() {
    std::vector<Ctl> c;
    c.push_back({ Segment, kStyle, KX + 140, PY + 30, 240, "", false });
    c.push_back({ Toggle, kAutoGain, KX + 470, PY + 30, 0, "AUTO GAIN", false });
    c.push_back({ Knob, kThreshold, KX + 72,  PY + 132, 40, "THRESHOLD", false });
    c.push_back({ Knob, kRatio,     KX + 178, PY + 132, 36, "RATIO", false });
    c.push_back({ Knob, kKnee,      KX + 266, PY + 132, 22, "KNEE", false });
    c.push_back({ Knob, kAttack,    KX + 352, PY + 132, 30, "ATTACK", false });
    c.push_back({ Knob, kRelease,   KX + 442, PY + 132, 30, "RELEASE", false });
    c.push_back({ Knob, kGain,      KX + 532, PY + 132, 30, "GAIN", false });
    c.push_back({ Knob, kMix,       KX + 612, PY + 132, 22, "MIX", false });
    c.push_back({ Toggle, kExpert, EX + 36, PY + 26, 0, "EXPERT", false });
    c.push_back({ Segment, kChannelMode, EX + 104, PY + 74, 168, "", true });
    c.push_back({ Knob, kLink, EX + 300, PY + 92, 24, "LINK", true });
    c.push_back({ Segment, kSidechain, EX + 104, PY + 116, 168, "", true });
    c.push_back({ Toggle, kScListen, EX + 226, PY + 116, 0, "LISTEN", true });
    c.push_back({ Toggle, kScHpOn, EX + 40,  PY + 168, 0, "SC HIGH PASS", true });
    c.push_back({ Knob, kScHpFreq, EX + 104, PY + 198, 20, "FREQ", true });
    c.push_back({ Toggle, kScLpOn, EX + 204, PY + 168, 0, "SC LOW PASS", true });
    c.push_back({ Knob, kScLpFreq, EX + 268, PY + 198, 20, "FREQ", true });
    return c;
}
static const std::vector<Ctl>& controls() { static std::vector<Ctl> c = buildControls(); return c; }

static const char* const* segLabels(uint32_t p, int& n) {
    static const char* const style[] = { "CLEAN", "CLASSIC", "OPTO" };
    static const char* const chm[] = { "L / R", "M / S" };
    static const char* const sc[] = { "INTERNAL", "EXTERNAL" };
    switch (p) {
    case kStyle: n = 3; return style;
    case kChannelMode: n = 2; return chm;
    case kSidechain: n = 2; return sc;
    default: n = 0; return nullptr;
    }
}

static constexpr float PS_X = 330.f, PS_Y = 20.f, PS_W = 230.f, PS_H = 28.f, PS_ARROW = 30.f;
static constexpr float A_X = 572.f, AB_W = 30.f, COPY_X = 636.f, COPY_W = 44.f;
static constexpr float UNDO_X = 692.f, REDO_X = 746.f, UR_W = 50.f, LEARN_X = 806.f, LEARN_W = 66.f, MIDI_X = 878.f, MIDI_W = 52.f, HELP_X = 936.f, HELP_W = 48.f;
static constexpr float PM_COLW = 150.f, PM_ROW = 23.f, PM_HEAD = 22.f, PM_PAD = 8.f;
static constexpr int HIST = 240;

class GneissUI : public UI {
public:
    GneissUI() : UI((uint)W, (uint)H), resizeHandle(this) {
        loadSharedResources();
        Engine def;
        for (uint32_t i = 0; i < kParamCount; ++i) { val[i] = def.p[i]; defv[i] = def.p[i]; }
        val[kOutInL] = val[kOutInR] = val[kOutOutL] = val[kOutOutR] = -120;
        for (int& m : paramCC) m = -1;
        const double sf = getScaleFactor();
        if (sf != 1.0) setSize((uint)(W * sf), (uint)(H * sf));
        setGeometryConstraints((uint)(W * 0.6 * sf), (uint)(H * 0.6 * sf), true);
        if (isResizable()) resizeHandle.hide();
        userPresets = listUserPresets();
        std::copy(val, val + kParamCount, before);
        for (int i = 0; i < HIST; ++i) { hIn[i] = hOut[i] = -120; hGr[i] = 0; }
    }

protected:
    float uiScale() const { return (float)getWidth() / W; }

    // ---- values -------------------------------------------------------------
    void setValue(uint32_t p, float v) {
        const ParamDef& d = paramDef(p);
        v = clampf(v, d.min, d.max);
        if (v == val[p]) return;
        val[p] = v; setParameterValue(p, v); repaint();
    }
    void commit(uint32_t p, float v) { editParameter(p, true); setValue(p, v); editParameter(p, false); }

    void parameterChanged(uint32_t i, float v) override {
        if (i >= kParamCount) return;
        if (i == kOutLearn) {
            if (v != lastLearnEcho && v > 0) {
                const int code = (int)v % 10000 - 1, prm = code / 128, cc = code % 128;
                for (int& m : paramCC) if (m == cc) m = -1;
                if (prm >= 0 && prm < (int)kParamCount) paramCC[prm] = cc;
                learnWaiting = -1;
            }
            lastLearnEcho = v; repaint(); return;
        }
        if (paramDef(i).hints & kHintOutput) { val[i] = v; return; }        // meters: drawn on the idle tick
        if (val[i] != v) { val[i] = v; if (!gesture) before[i] = v; repaint(); }
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

    // ---- undo / redo ----------------------------------------------------------------
    typedef std::vector<float> Snap;
    Snap snapshot() const { return Snap(val, val + kParamCount); }
    bool differs(const float* a, const float* b) const {
        for (uint32_t i = 0; i < kParamCount; ++i) if (!(paramDef(i).hints & kHintOutput) && a[i] != b[i]) return true;
        return false;
    }
    void beginGesture() { gesture = true; std::copy(val, val + kParamCount, before); }
    void endGesture() {
        gesture = false;
        if (differs(before, val)) {
            undoStack.push_back(Snap(before, before + kParamCount)); if (undoStack.size() > 200) undoStack.erase(undoStack.begin());
            redoStack.clear();
        }
        std::copy(val, val + kParamCount, before); repaint();
    }
    void applyAll(const float* s) { for (uint32_t i = 0; i < kParamCount; ++i) if (!(paramDef(i).hints & kHintOutput) && val[i] != s[i]) commit(i, s[i]); }
    void restoreSnap(const Snap& s) { applyAll(s.data()); std::copy(val, val + kParamCount, before); }
    void undo() { if (undoStack.empty()) return; redoStack.push_back(snapshot()); restoreSnap(undoStack.back()); undoStack.pop_back(); flash("Undo"); }
    void redo() { if (redoStack.empty()) return; undoStack.push_back(snapshot()); restoreSnap(redoStack.back()); redoStack.pop_back(); flash("Redo"); }

    // ---- A / B ----------------------------------------------------------------------------
    void switchAB(int to) {
        if (to == abSlot) return;
        abStore[abSlot] = snapshot();
        if (abStore[to].empty()) abStore[to] = snapshot();          // first visit: start from a copy
        beginGesture(); applyAll(abStore[to].data()); endGesture();
        abSlot = to; flash(to == 0 ? "Setting A" : "Setting B");
    }
    void copyAB() { abStore[1 - abSlot] = snapshot(); flash(abSlot == 0 ? "Copied A to B" : "Copied B to A"); }

    // ---- formatting ---------------------------------------------------------
    void fmt(uint32_t p, char* b, size_t n) const {
        const float v = val[p];
        int ne = 0;
        if (const char* const* l = enumLabels(p, ne)) { snprintf(b, n, "%s", l[std::max(0, std::min(ne - 1, (int)std::round(v)))]); return; }
        const ParamDef& d = paramDef(p);
        if (d.hints & kHintBoolean) { snprintf(b, n, "%s", v > 0.5f ? "On" : "Off"); return; }
        switch (p) {
        case kThreshold: case kKnee: snprintf(b, n, "%.1f dB", v); return;
        case kGain: snprintf(b, n, "%+.1f dB", v); return;
        case kRatio: snprintf(b, n, v < 10 ? "%.2f:1" : "%.1f:1", v); return;
        case kAttack: snprintf(b, n, v < 1 ? "%.2f ms" : (v < 10 ? "%.1f ms" : "%.0f ms"), v); return;
        case kRelease: if (v >= 1000) snprintf(b, n, "%.2f s", v / 1000); else snprintf(b, n, "%.0f ms", v); return;
        case kScHpFreq: case kScLpFreq: if (v >= 1000) snprintf(b, n, "%.2f kHz", v / 1000); else snprintf(b, n, "%.0f Hz", v); return;
        default: snprintf(b, n, "%.0f%%", v * 100);
        }
    }
    bool parse(uint32_t p, const std::string& in, float& out) const {
        std::string s; for (char ch : in) if (!std::isspace((unsigned char)ch)) s += (char)std::tolower((unsigned char)ch);
        if (s.empty()) return false;
        char* end = nullptr; double x = std::strtod(s.c_str(), &end);
        if (end == s.c_str()) return false;
        std::string suf(end);
        if (suf == "k" || suf == "khz") x *= 1000;
        if (p == kRelease && suf == "s") x *= 1000;
        if (p == kMix || p == kLink) { out = (float)(x / 100.0); return true; }
        out = (float)x; return true;
    }
    bool dimmed(uint32_t p) const {
        if (p == kScHpFreq && val[kScHpOn] < 0.5f) return true;
        if (p == kScLpFreq && val[kScLpOn] < 0.5f) return true;
        return false;
    }
    bool visible(const Ctl& c) const { return !c.expert || val[kExpert] > 0.5f; }

    // ---- drawing ------------------------------------------------------------
    void onNanoDisplay() override {
        const float s = uiScale();
        save(); scale(s, s);
        beginPath(); rect(0, 0, W, H); fillColor(C_BG); fill();
        fontFace(NANOVG_DEJAVU_SANS_TTF);

        // header: folded (gneissic) bands
        for (int i = 0; i < 4; ++i) {
            const float y0 = 20 + i * 7.f;
            beginPath(); moveTo(16, y0 + 3); bezierTo(22, y0 - 3, 28, y0 + 6, 38, y0); lineTo(38, y0 + 4.5f); bezierTo(28, y0 + 10.5f, 22, y0 + 1.5f, 16, y0 + 7.5f); closePath();
            fillColor(i % 2 == 0 ? C_ACC : Color(0x6c, 0x6e, 0x76)); fill();
        }
        fontSize(22); textLetterSpacing(9); fillColor(C_TEXT); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); text(50, 34, "GNEISS", nullptr);
        fontSize(10); textLetterSpacing(2.5f); fillColor(C_DIM); text(198, 30, "COMPRESSOR", nullptr);
        fontSize(7.5f); textLetterSpacing(1.8f); fillColor(C_FAINT); text(198, 42, "EPSILON AUDIO  1.0", nullptr);
        drawPresetBar();
        drawAB();
        drawHeaderButton(UNDO_X, UR_W, "UNDO", false, hoverHeader == 3, undoStack.empty());
        drawHeaderButton(REDO_X, UR_W, "REDO", false, hoverHeader == 4, redoStack.empty());
        drawHeaderButton(LEARN_X, LEARN_W, learnMode ? "LEARNING" : "LEARN", learnMode, hoverHeader == 1, false);
        drawHeaderButton(MIDI_X, MIDI_W, "MIDI", val[kMidiOn] > 0.5f, hoverHeader == 2, false);
        drawHeaderButton(HELP_X, HELP_W, "HELP", helpOpen, hoverHeader == 5, false);

        drawLevelDisplay();
        drawTransfer();
        drawMeters();
        panel(KX, PY, KW, PH); panel(EX, PY, EW, PH);
        if (val[kExpert] < 0.5f) drawExpertSummary();
        else { drawCaption(EX + 104, PY + 57, "CHANNELS"); drawCaption(EX + 104, PY + 99, "SIDECHAIN"); }

        const std::vector<Ctl>& cs = controls();
        for (int i = 0; i < (int)cs.size(); ++i) if (visible(cs[i])) drawCtl(cs[i], i == active || i == hover);
        if (editing >= 0) drawEditBox();
        drawStatus();
        if (menuOpen) drawPresetMenu();
        if (saving) drawSaveBox();
        if (helpOpen) drawHelp();
        else if (tipShown && gTipsOn) drawTooltip();
        restore();
    }
    void panel(float x, float y, float w, float h) { beginPath(); roundedRect(x, y, w, h, 6); fillColor(C_PANEL); fill(); strokeColor(C_LINE); strokeWidth(1); stroke(); }
    void displayBox(float x, float y, float w, float h) { beginPath(); roundedRect(x, y, w, h, 6); fillColor(C_DISPLAY); fill(); strokeColor(C_LINE); strokeWidth(1); stroke(); }
    void drawCaption(float cx, float y, const char* t) { if (!t[0]) return; fontSize(7.5f); textLetterSpacing(1.8f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(C_FAINT); text(cx, y, t, nullptr); }
    void drawHeaderButton(float x, float w, const char* label, bool on, bool hot, bool disabled) {
        beginPath(); roundedRect(x, PS_Y, w, PS_H, 5); fillColor(on ? withAlpha(C_ACC, 0.22f) : C_PANEL); fill();
        strokeColor(on ? C_ACC : (hot && !disabled ? C_DIM : C_LINE)); strokeWidth(1); stroke();
        fontSize(9.5f); textLetterSpacing(2.f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(disabled ? C_FAINT : (on ? C_TEXT : C_DIM));
        text(x + w / 2, PS_Y + PS_H / 2 + 0.5f, label, nullptr);
    }
    void drawAB() {
        for (int k = 0; k < 2; ++k) {
            const float x = A_X + k * AB_W; const bool on = abSlot == k, hot = hoverHeader == 6 + k;
            beginPath(); roundedRect(x + (k ? 0.5f : 0), PS_Y, AB_W - 0.5f, PS_H, 5); fillColor(on ? withAlpha(C_ACC, 0.85f) : (hot ? C_PANEL_HI : C_PANEL)); fill();
            strokeColor(on ? C_ACC : C_LINE); strokeWidth(1); stroke();
            fontSize(11); textLetterSpacing(0); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(on ? C_BG : C_DIM); text(x + AB_W / 2, PS_Y + PS_H / 2 + 0.5f, k ? "B" : "A", nullptr);
        }
        drawHeaderButton(COPY_X, COPY_W, "COPY", false, hoverHeader == 8, false);
    }

    // level display mapping (dB 0 at top .. -60 at bottom)
    static float yOfDbL(float db) { return LY + 10 + (LH - 30) * clampf(-db / 60.f, 0.f, 1.f); }
    void drawLevelDisplay() {
        displayBox(LX, LY, LW, LH);
        fontSize(8); textLetterSpacing(1); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(C_FAINT);
        beginPath();
        for (int db = 0; db >= -60; db -= 12) { const float y = yOfDbL((float)db); moveTo(LX + 6, y); lineTo(LX + LW - 6, y); }
        strokeColor(Color(255, 255, 255, 8 / 255.f)); strokeWidth(1); stroke();
        for (int db = -12; db >= -60; db -= 12) { char b[8]; snprintf(b, sizeof b, "%d", db); text(LX + 8, yOfDbL((float)db) - 6, b, nullptr); }
        const float x0 = LX + 30, x1 = LX + LW - 8, dx = (x1 - x0) / (HIST - 1);
        auto idx = [&](int k) { return (hPos + k) % HIST; };            // oldest first
        // input level (grey area)
        beginPath(); moveTo(x0, yOfDbL(-60));
        for (int k = 0; k < HIST; ++k) lineTo(x0 + k * dx, yOfDbL(hIn[idx(k)]));
        lineTo(x1, yOfDbL(-60)); closePath(); fillColor(Color(0x3a, 0x3c, 0x42, 0.85f)); fill();
        // output level (line)
        beginPath();
        for (int k = 0; k < HIST; ++k) { const float x = x0 + k * dx, y = yOfDbL(hOut[idx(k)]); if (!k) moveTo(x, y); else lineTo(x, y); }
        strokeColor(C_OUT); strokeWidth(1.6f); stroke();
        // gain reduction (hanging from the top, 0..30 dB over the top half)
        beginPath(); moveTo(x0, LY + 10);
        for (int k = 0; k < HIST; ++k) lineTo(x0 + k * dx, LY + 10 + (LH - 30) * 0.5f * clampf(hGr[idx(k)] / 30.f, 0.f, 1.f));
        lineTo(x1, LY + 10); closePath(); fillColor(withAlpha(C_GR, 0.45f)); fill();
        beginPath();
        for (int k = 0; k < HIST; ++k) { const float x = x0 + k * dx, y = LY + 10 + (LH - 30) * 0.5f * clampf(hGr[idx(k)] / 30.f, 0.f, 1.f); if (!k) moveTo(x, y); else lineTo(x, y); }
        strokeColor(C_GR); strokeWidth(1.4f); stroke();
        // threshold (dashed)
        const float ty = yOfDbL(val[kThreshold]);
        for (float x = x0; x < x1; x += 10) { beginPath(); moveTo(x, ty); lineTo(std::min(x + 5, x1), ty); strokeColor(withAlpha(C_ACC, hoverThr || dragThr ? 1.f : 0.7f)); strokeWidth(hoverThr || dragThr ? 2.f : 1.2f); stroke(); }
        char b[24]; snprintf(b, sizeof b, "%.1f dB", val[kThreshold]);
        fontSize(9); textLetterSpacing(0.6f); textAlign(ALIGN_RIGHT | ALIGN_BOTTOM); fillColor(C_ACC); text(x1 - 2, ty - 3, b, nullptr);
        fontSize(8); textLetterSpacing(1.6f); textAlign(ALIGN_LEFT | ALIGN_BOTTOM);
        fillColor(C_DIM); text(LX + 30, LY + LH - 4, "INPUT", nullptr); fillColor(C_OUT); text(LX + 80, LY + LH - 4, "OUTPUT", nullptr); fillColor(C_GR); text(LX + 136, LY + LH - 4, "GAIN REDUCTION", nullptr);
    }

    // transfer curve mapping: -60..0 dB both axes
    static float xOfDbT(float db) { return TX + 26 + (TW - 40) * clampf((db + 60.f) / 60.f, 0.f, 1.f); }
    static float yOfDbT(float db) { return TY + 12 + (TH - 44) * clampf(-db / 60.f, 0.f, 1.f); }
    float curveOut(float inDb) const {
        const int style = (int)(val[kStyle] + .5f);
        const float knee = val[kKnee] + (style == kOpto ? 6.f : (style == kClassic ? 2.f : 0.f));
        return inDb - staticGr(inDb, val[kThreshold], val[kRatio], knee, false);
    }
    void drawTransfer() {
        displayBox(TX, TY, TW, TH);
        beginPath();
        for (int db = 0; db >= -60; db -= 12) { moveTo(xOfDbT((float)db), yOfDbT(0)); lineTo(xOfDbT((float)db), yOfDbT(-60)); moveTo(xOfDbT(-60), yOfDbT((float)db)); lineTo(xOfDbT(0), yOfDbT((float)db)); }
        strokeColor(Color(255, 255, 255, 8 / 255.f)); strokeWidth(1); stroke();
        beginPath(); moveTo(xOfDbT(-60), yOfDbT(-60)); lineTo(xOfDbT(0), yOfDbT(0)); strokeColor(C_FAINT); strokeWidth(1); stroke();
        beginPath();
        for (int i = 0; i <= 120; ++i) { const float in = -60.f + i * 0.5f, x = xOfDbT(in), y = yOfDbT(curveOut(in)); if (!i) moveTo(x, y); else lineTo(x, y); }
        strokeColor(C_ACC); strokeWidth(2.2f); stroke();
        // threshold marker and live dot
        const float tx = xOfDbT(val[kThreshold]);
        beginPath(); moveTo(tx, yOfDbT(0)); lineTo(tx, yOfDbT(-60)); strokeColor(withAlpha(C_ACC, hoverCurve || dragCurve ? 0.6f : 0.25f)); strokeWidth(1); stroke();
        const float inNow = std::max(val[kOutInL], val[kOutInR]);
        if (inNow > -60) { beginPath(); circle(xOfDbT(inNow), yOfDbT(curveOut(inNow)), 5); fillColor(C_OUT); fill(); }
        fontSize(8); textLetterSpacing(1.4f); textAlign(ALIGN_CENTER | ALIGN_BOTTOM); fillColor(C_FAINT); text(TX + TW / 2 + 6, TY + TH - 6, "INPUT  dB", nullptr);
        textAlign(ALIGN_LEFT | ALIGN_MIDDLE);
        for (int db = -12; db >= -48; db -= 12) { char b[8]; snprintf(b, sizeof b, "%d", db); text(TX + 6, yOfDbT((float)db), b, nullptr); }
        char b[48]; fmt(kRatio, b, sizeof b); std::string st = std::string(b) + "   knee "; char kb[16]; snprintf(kb, sizeof kb, "%.0f dB", val[kKnee]); st += kb;
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
        // gain reduction hangs from the top (0..30 dB scale)
        for (int k = 0; k < 2; ++k) {
            const float x = MX + 40 + k * 7.f; beginPath(); rect(x, top, 6, h); fillColor(C_PANEL_HI); fill();
            const float g = clampf(val[k ? kOutGrR : kOutGrL] / 30.f, 0.f, 1.f); if (g > 0.002f) { beginPath(); rect(x, top, 6, h * g); fillColor(C_GR); fill(); }
        }
        bar(MX + 61, 9, val[kOutOutL], C_OUT); bar(MX + 72, 9, val[kOutOutR], C_OUT);
        fontSize(7.5f); textLetterSpacing(1.2f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(C_FAINT);
        text(MX + 20, MY + 14, "IN", nullptr); text(MX + 46.5f, MY + 14, "GR", nullptr); text(MX + 71, MY + 14, "OUT", nullptr);
        char b[16]; snprintf(b, sizeof b, "-%.1f", std::max(val[kOutGrL], val[kOutGrR])); fontSize(9); fillColor(C_GR); text(MX + 46.5f, MY + MH - 10, b, nullptr);
    }
    void drawExpertSummary() {
        char b[96]; fmt(kChannelMode, b, sizeof b); std::string l1 = std::string(b) + "   ·   link " + std::to_string((int)std::round(val[kLink] * 100)) + "%";
        fmt(kSidechain, b, sizeof b); std::string l2 = std::string(b) + " sidechain";
        if (val[kScHpOn] > 0.5f || val[kScLpOn] > 0.5f) l2 += "   ·   filtered";
        fontSize(11); textLetterSpacing(0.6f); textAlign(ALIGN_LEFT | ALIGN_TOP); fillColor(C_DIM);
        text(EX + 20, PY + 70, l1.c_str(), nullptr); text(EX + 20, PY + 92, l2.c_str(), nullptr);
        fontSize(10); fillColor(C_FAINT); textBox(EX + 20, PY + 140, EW - 40, "Turn on EXPERT for mid/side processing, stereo link, the external sidechain and the sidechain filters.", nullptr);
    }

    void drawKnobBody(float x, float y, float r, float n, bool hot, Color col, bool dim) {
        const float a0 = 0.75f * 3.14159265f, a1 = 2.25f * 3.14159265f, ang = a0 + (a1 - a0) * n;
        const float tw = r > 30 ? 5.f : (r > 18 ? 4.f : 3.f);
        beginPath(); arc(x, y, r, a0, a1, NanoVG::CW); strokeColor(C_LINE); strokeWidth(tw); lineCap(ROUND); stroke();
        if (ang - a0 > 0.001f) { beginPath(); arc(x, y, r, a0, ang, NanoVG::CW); strokeColor(dim ? C_FAINT : col); stroke(); }
        const float cr = r - (r > 30 ? 10 : (r > 18 ? 8 : 6));
        beginPath(); circle(x, y, cr);
        fillPaint(radialGradient(x - cr * 0.3f, y - cr * 0.4f, 0, cr * 1.4f, hot ? Color(0x3c, 0x3e, 0x44) : C_PANEL_HI, Color(0x14, 0x15, 0x17)));
        fill(); strokeColor(Color(255, 255, 255, 18 / 255.f)); strokeWidth(1); stroke();
        beginPath(); moveTo(x + std::cos(ang) * cr * 0.35f, y + std::sin(ang) * cr * 0.35f); lineTo(x + std::cos(ang) * cr * 0.9f, y + std::sin(ang) * cr * 0.9f);
        strokeColor(dim ? C_DIM : C_TEXT); strokeWidth(r > 30 ? 3.f : 2.f); lineCap(ROUND); stroke();
    }
    void drawCtl(const Ctl& c, bool hot) {
        const uint32_t p = c.param; const bool dim = dimmed(p); char buf[40];
        switch (c.type) {
        case Knob: {
            drawKnobBody(c.x, c.y, c.r, toNorm(p, val[p]), hot, C_ACC, dim);
            const bool big = c.r >= 30;
            fontSize(big ? 9.f : 8.f); textLetterSpacing(big ? 1.8f : 1.4f); textAlign(ALIGN_CENTER | ALIGN_TOP);
            if (big) {
                fillColor(dim ? C_FAINT : C_DIM); text(c.x, c.y + c.r + 6, c.label, nullptr);
                fmt(p, buf, sizeof buf); fontSize(11.5f); textLetterSpacing(0.5f); fillColor(hot ? C_ACC : C_TEXT); text(c.x, c.y + c.r + 19, buf, nullptr);
            } else if (hot && editing < 0 && !learnMode) { fmt(p, buf, sizeof buf); fillColor(C_ACC); text(c.x, c.y + c.r + 6, buf, nullptr); }
            else { fillColor(dim ? C_FAINT : C_DIM); text(c.x, c.y + c.r + 6, c.label, nullptr); }
            break; }
        case Toggle: {
            const bool on = val[p] > 0.5f, alert = p == kScListen && on;
            const Color tc = alert ? C_GR : C_ACC;
            const float w = 34, h = 18, x = c.x - w / 2, y = c.y - h / 2;
            beginPath(); roundedRect(x, y, w, h, h / 2); fillColor(on ? withAlpha(tc, 0.25f) : C_PANEL_HI); fill(); strokeColor(on ? tc : C_LINE); strokeWidth(1); stroke();
            beginPath(); circle(on ? x + w - h / 2 : x + h / 2, c.y, 6); fillColor(on ? tc : C_DIM); fill();
            fontSize(9.5f); textLetterSpacing(1.8f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(on ? C_TEXT : C_DIM); text(c.x + w / 2 + 8, c.y + 0.5f, c.label, nullptr);
            break; }
        case Segment: {
            int n = 0; const char* const* names = segLabels(p, n);
            const int cur = (int)(val[p] + .5f); const float sw = c.r / n, x0 = c.x - c.r / 2;
            for (int i = 0; i < n; ++i) {
                const float x = x0 + sw * i;
                beginPath(); roundedRect(x + 1.5f, c.y - SEG_H / 2, sw - 3, SEG_H, 5); fillColor(i == cur ? C_ACC : C_PANEL_HI); fill();
                fontSize(9.5f); textLetterSpacing(1.6f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(i == cur ? C_BG : C_DIM); text(x + sw / 2, c.y + 0.5f, names[i], nullptr);
            }
            break; }
        }
        if (learnMode) {
            const int cc = paramCC[p]; const bool waiting = learnWaiting == (int)p;
            if (cc >= 0 || waiting) {
                const float by = c.type == Knob ? c.y - c.r - 12 : c.y - SEG_H / 2 - 8;
                snprintf(buf, sizeof buf, waiting ? "?" : "CC%d", cc);
                fontSize(8.5f); textLetterSpacing(0.5f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE);
                const float bw = waiting ? 16 : 34;
                beginPath(); roundedRect(c.x - bw / 2, by - 7, bw, 14, 3); fillColor(waiting ? C_ACC : C_PANEL_HI); fill(); strokeColor(C_ACC); strokeWidth(1); stroke();
                fillColor(waiting ? C_BG : C_ACC); text(c.x, by + 0.5f, buf, nullptr);
            }
        }
    }
    void drawStatus() {
        std::string msg;
        if (statusFlash > 0) msg = statusText;
        else if (learnMode) msg = learnWaiting >= 0 ? std::string("Move a MIDI controller to assign: ") + paramDef(learnWaiting).name : "MIDI Learn: click a control, then move a controller.  Right-click a control to unassign.";
        else if (val[kScListen] > 0.5f) msg = "LISTEN is on: you are hearing the sidechain detector signal";
        if (!msg.empty()) { fontSize(10); textLetterSpacing(0.8f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(val[kScListen] > 0.5f && statusFlash <= 0 && !learnMode ? C_GR : C_ACC); text(LX + 200, LY + 16, msg.c_str(), nullptr); }
        if (learnMode && learnWaiting < 0) { textAlign(ALIGN_RIGHT | ALIGN_MIDDLE); fillColor(hoverClear ? C_TEXT : C_DIM); text(LX + LW - 12, LY + LH - 12, "CLEAR ALL ASSIGNMENTS", nullptr); }
        if (statusFlash > 0) { --statusFlash; repaint(); }
    }
    void flash(const std::string& s) { statusText = s; statusFlash = 60; repaint(); }
    void drawEditBox() {
        const Ctl& c = controls()[editing];
        const float w = 90, h = 22, x = c.x - w / 2, y = c.y + c.r + 4;
        beginPath(); roundedRect(x, y, w, h, 4); fillColor(C_DISPLAY); fill(); strokeColor(C_ACC); strokeWidth(1); stroke();
        fontSize(12); textLetterSpacing(0.5f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(C_TEXT); text(c.x, y + h / 2 + 0.5f, (editBuf + "|").c_str(), nullptr);
    }

    // ---- presets ------------------------------------------------------------------
    bool matchesValues(const float* v) const {
        for (uint32_t i = 0; i < kParamCount; ++i) {
            if ((paramDef(i).hints & kHintOutput) || i == kExpert) continue;
            if (std::fabs(val[i] - v[i]) > 1e-4f * std::max(1.f, std::fabs(v[i]))) return false;
        }
        return true;
    }
    void applyValues(const float* v) { beginGesture(); applyAll(v); endGesture(); }
    void applyFactory(int idx) { idx = (idx % (int)kNumPresets + (int)kNumPresets) % (int)kNumPresets; applyValues(presets()[idx].v); curFactory = idx; curUser.clear(); repaint(); }
    void applyUser(const std::string& name) { float v[kParamCount]; if (!loadUserPreset(name, v)) return; applyValues(v); std::copy(v, v + kParamCount, userVals); curUser = name; curFactory = -1; repaint(); }
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
        for (int u = 0; u < (int)userPresets.size() && u < 16; ++u) items.push_back({ mx + PM_PAD + kNumCategories * PM_COLW, top + u * PM_ROW, PM_COLW - 4, PM_ROW, 1, u });
        maxRows = std::max(maxRows, std::min(16, (int)userPresets.size()));
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

    // ---- hover tips -------------------------------------------------------------------
    void uiIdle() override {
        // level history (about 25 points per second)
        const double t = nowSec();
        if (t - lastHist > 0.04) {
            lastHist = t;
            hIn[hPos] = std::max(val[kOutInL], val[kOutInR]); hOut[hPos] = std::max(val[kOutOutL], val[kOutOutR]); hGr[hPos] = std::max(val[kOutGrL], val[kOutGrR]);
            hPos = (hPos + 1) % HIST; repaint();
        }
        if (gTipsOn && !tipShown && tipKey >= 0 && tipKey != tipSuppressed && active < 0 && !dragThr && !dragCurve && !menuOpen && !saving && !helpOpen && editing < 0
            && t - tipSince > 0.6) { tipShown = true; repaint(); }
    }
    bool tipContent(std::string& title, std::string& value, std::string& body) const {
        if (tipKey < 0) return false;
        if (tipKey < 1000) {
            const uint32_t p = controls()[tipKey].param; char b[40]; fmt(p, b, sizeof b);
            title = paramDef(p).name; value = b; body = paramTip(p);
            if (controls()[tipKey].type == Knob) body += "\n\nDrag the middle up/down or grab the rim and turn. Double-click to type a value, Cmd/Ctrl-click to reset.";
            if (paramCC[p] >= 0) { char cc[32]; snprintf(cc, sizeof cc, "   ·   CC%d", paramCC[p]); value += cc; }
            return true;
        }
        int t = -1;
        if (tipKey >= 4000) t = kTipLevelDisplay + (tipKey - 4000);
        else if (tipKey >= 2000) t = kTipPrev + (tipKey - 2000);
        else { const int hh = tipKey - 1000;
            t = hh == 1 ? kTipLearn : hh == 2 ? kTipMidi : hh == 3 ? kTipUndo : hh == 4 ? kTipRedo : hh == 5 ? kTipHelp : hh == 6 ? kTipA : hh == 7 ? kTipB : kTipCopy; }
        title = targetTitle(t); value.clear(); body = targetTip(t);
        if (t == kTipUndo || t == kTipRedo) { const size_t n = t == kTipUndo ? undoStack.size() : redoStack.size(); char b[32]; snprintf(b, sizeof b, "%d step%s", (int)n, n == 1 ? "" : "s"); value = b; }
        return true;
    }
    void drawTooltip() {
        std::string title, value, body;
        if (!tipContent(title, value, body)) return;
        const float w = 310, pad = 12;
        fontSize(11); textLetterSpacing(0.2f); textLineHeight(1.3f);
        float bb[4]; textBoxBounds(0, 0, w - 2 * pad, body.c_str(), nullptr, bb);
        const float h = pad + 16 + 8 + (bb[3] - bb[1]) + pad;
        float x = tipX + 16, y = tipY + 20;
        if (x + w > W - 8) x = tipX - w - 12;
        if (y + h > H - 8) y = tipY - h - 12;
        x = clampf(x, 8, W - w - 8); y = clampf(y, 8, H - h - 8);
        beginPath(); rect(x - 10, y - 4, w + 20, h + 18); fillPaint(boxGradient(x, y + 3, w, h, 6, 14, Color(0, 0, 0, 0.55f), Color(0, 0, 0, 0.f))); fill();
        beginPath(); roundedRect(x, y, w, h, 6); fillColor(Color(0x1f, 0x20, 0x24)); fill(); strokeColor(C_LINE); strokeWidth(1); stroke();
        fontSize(10); textLetterSpacing(1.6f); textAlign(ALIGN_LEFT | ALIGN_TOP); fillColor(C_TEXT);
        std::string up = title; for (char& ch : up) ch = (char)std::toupper((unsigned char)ch);
        text(x + pad, y + pad + 1, up.c_str(), nullptr);
        if (!value.empty()) { fontSize(10.5f); textLetterSpacing(0.4f); textAlign(ALIGN_RIGHT | ALIGN_TOP); fillColor(C_ACC); text(x + w - pad, y + pad + 1, value.c_str(), nullptr); }
        fontSize(11); textLetterSpacing(0.2f); textLineHeight(1.3f); textAlign(ALIGN_LEFT | ALIGN_TOP); fillColor(C_DIM);
        textBox(x + pad, y + pad + 24, w - 2 * pad, body.c_str(), nullptr);
        textLineHeight(1.f);
    }

    // ---- help window ---------------------------------------------------------------
    static constexpr float HX = 140, HY = 76, HW = 760, HH = 508, TAB_W = 170, TAB_H = 34;
    int helpHit(float x, float y) const {
        if (x < HX || x > HX + HW || y < HY || y > HY + HH) return -2;
        for (int i = 0; i < kNumHelpPages; ++i) if (x >= HX + 12 && x < HX + 12 + TAB_W && y >= HY + 56 + i * TAB_H && y < HY + 56 + (i + 1) * TAB_H) return i;
        const float by = HY + HH - 44;
        if (y >= by && y < by + 28) {
            if (x >= HX + 12 && x < HX + 12 + TAB_W) return 100;
            if (x >= HX + HW - 330 && x < HX + HW - 140) return 101;
            if (x >= HX + HW - 124 && x < HX + HW - 16) return 102;
        }
        if (x >= HX + HW - 40 && y < HY + 40) return 102;
        return -1;
    }
    void helpClick(float x, float y) {
        const int h = helpHit(x, y);
        if (h >= 0 && h < kNumHelpPages) helpPage = h;
        else if (h == 100) gTipsOn = !gTipsOn;
        else if (h == 101) openURL(kManualURL);
        else if (h == 102 || h == -2) helpOpen = false;
        repaint();
    }
    void drawHelp() {
        beginPath(); rect(0, 0, W, H); fillColor(Color(0, 0, 0, 0.55f)); fill();
        beginPath(); roundedRect(HX, HY, HW, HH, 10); fillColor(C_PANEL); fill(); strokeColor(C_LINE); strokeWidth(1); stroke();
        fontSize(13); textLetterSpacing(4.f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(C_TEXT); text(HX + 22, HY + 28, "GNEISS HELP", nullptr);
        const float cx = HX + HW - 24, cy = HY + 26;
        beginPath(); moveTo(cx - 6, cy - 6); lineTo(cx + 6, cy + 6); moveTo(cx + 6, cy - 6); lineTo(cx - 6, cy + 6); strokeColor(helpHover == 102 ? C_TEXT : C_DIM); strokeWidth(1.6f); stroke();
        for (int i = 0; i < kNumHelpPages; ++i) {
            const float ty = HY + 56 + i * TAB_H;
            if (i == helpPage || i == helpHover) { beginPath(); roundedRect(HX + 12, ty + 2, TAB_W, TAB_H - 4, 5); fillColor(i == helpPage ? withAlpha(C_ACC, 0.2f) : C_PANEL_HI); fill(); }
            if (i == helpPage) { beginPath(); roundedRect(HX + 12, ty + 8, 3, TAB_H - 16, 1.5f); fillColor(C_ACC); fill(); }
            fontSize(11.5f); textLetterSpacing(0.6f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(i == helpPage ? C_TEXT : C_DIM);
            text(HX + 28, ty + TAB_H / 2 + 0.5f, kHelpPages[i].title, nullptr);
        }
        beginPath(); moveTo(HX + TAB_W + 26, HY + 56); lineTo(HX + TAB_W + 26, HY + HH - 56); strokeColor(C_LINE); strokeWidth(1); stroke();
        const float bx = HX + TAB_W + 46, bw = HW - TAB_W - 70;
        fontSize(15); textLetterSpacing(0.8f); textAlign(ALIGN_LEFT | ALIGN_TOP); fillColor(C_ACC); text(bx, HY + 60, kHelpPages[helpPage].title, nullptr);
        fontSize(12); textLetterSpacing(0.2f); textLineHeight(1.38f); fillColor(Color(0xc6, 0xc8, 0xcc));
        textBox(bx, HY + 92, bw, kHelpPages[helpPage].body, nullptr);
        textLineHeight(1.f);
        const float by = HY + HH - 44;
        beginPath(); moveTo(HX + 12, by - 10); lineTo(HX + HW - 12, by - 10); strokeColor(C_LINE); stroke();
        const float tw = 34, th = 18, tx = HX + 16, tyy = by + 14 - th / 2;
        beginPath(); roundedRect(tx, tyy, tw, th, th / 2); fillColor(gTipsOn ? withAlpha(C_ACC, 0.25f) : C_PANEL_HI); fill(); strokeColor(gTipsOn ? C_ACC : C_LINE); stroke();
        beginPath(); circle(gTipsOn ? tx + tw - th / 2 : tx + th / 2, by + 14, 6); fillColor(gTipsOn ? C_ACC : C_DIM); fill();
        fontSize(10); textLetterSpacing(1.6f); textAlign(ALIGN_LEFT | ALIGN_MIDDLE); fillColor(helpHover == 100 ? C_TEXT : C_DIM); text(tx + tw + 10, by + 14.5f, "HOVER TIPS", nullptr);
        auto btn = [&](float x0, float w0, const char* label, bool hot, bool primary) {
            beginPath(); roundedRect(x0, by, w0, 28, 5); fillColor(primary ? (hot ? withAlpha(C_ACC, 0.35f) : withAlpha(C_ACC, 0.18f)) : (hot ? C_PANEL_HI : C_PANEL)); fill();
            strokeColor(primary ? C_ACC : C_LINE); strokeWidth(1); stroke();
            fontSize(9.5f); textLetterSpacing(1.8f); textAlign(ALIGN_CENTER | ALIGN_MIDDLE); fillColor(C_TEXT); text(x0 + w0 / 2, by + 14.5f, label, nullptr);
        };
        btn(HX + HW - 330, 190, "FULL MANUAL (WEB)", helpHover == 101, false);
        btn(HX + HW - 124, 108, "CLOSE", helpHover == 102, true);
    }

    // ---- interaction --------------------------------------------------------------
    int hit(float x, float y, int* seg = nullptr) const {
        const std::vector<Ctl>& cs = controls();
        for (int i = 0; i < (int)cs.size(); ++i) {
            const Ctl& c = cs[i];
            if (!visible(c)) continue;
            switch (c.type) {
            case Knob: { const float rr = c.r + 6; if ((x - c.x) * (x - c.x) + (y - c.y) * (y - c.y) <= rr * rr) return i; break; }
            case Toggle: if (x >= c.x - 20 && x <= c.x + 100 && std::fabs(y - c.y) <= 12) return i; break;
            case Segment: { int n = 0; segLabels(c.param, n); const float x0 = c.x - c.r / 2;
                if (x >= x0 && x < x0 + c.r && std::fabs(y - c.y) <= SEG_H / 2 + 1) { if (seg) *seg = (int)((x - x0) / (c.r / n)); return i; } break; }
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
    bool clearHit(float x, float y) const { return learnMode && learnWaiting < 0 && inRect(x, y, LX + LW - 190, LY + LH - 22, 180, 20); }
    int areaHit(float x, float y) const {   // 0 level display, 1 transfer curve, 2 meters
        if (inRect(x, y, LX, LY, LW, LH)) return 0;
        if (inRect(x, y, TX, TY, TW, TH)) return 1;
        if (inRect(x, y, MX, MY, MW, MH)) return 2;
        return -1;
    }

    void setLearn(bool on) { learnMode = on; learnWaiting = -1; if (!on) setState("cmd", "cancel"); repaint(); }
    void endEdit(bool apply) {
        if (editing < 0) return;
        const uint32_t p = controls()[editing].param; float v;
        if (apply && parse(p, editBuf, v)) { beginGesture(); commit(p, v); endGesture(); }
        editing = -1; editBuf.clear(); repaint();
    }
    static float angleNorm(const Ctl& c, float x, float y) {
        float a = std::atan2(y - c.y, x - c.x);
        const float a0 = 0.75f * 3.14159265f;
        float rel = a - a0; while (rel < 0) rel += 2 * 3.14159265f;
        if (rel > 1.5f * 3.14159265f) rel = rel > 1.75f * 3.14159265f ? 0.f : 1.5f * 3.14159265f;
        return rel / (1.5f * 3.14159265f);
    }

    bool onKeyboard(const KeyboardEvent& ev) override {
        if (!ev.press) return false;
        std::string* buf = saving ? &saveName : (editing >= 0 ? &editBuf : nullptr);
        if (!buf) {
            const bool cmd = (ev.mod & (kModifierControl | kModifierSuper)) != 0;
            if (cmd && (ev.key == 'z' || ev.key == 'Z')) { if (ev.mod & kModifierShift) redo(); else undo(); return true; }
            if (cmd && (ev.key == 'y' || ev.key == 'Y')) { redo(); return true; }
            if (ev.key == kKeyEscape && helpOpen) { helpOpen = false; repaint(); return true; }
            if (ev.key == kKeyEscape && learnMode) { setLearn(false); return true; }
            return false;
        }
        if (ev.key == kKeyEnter) {
            if (saving) {
                if (saveUserPreset(saveName, val)) { userPresets = listUserPresets(); curUser = sanitizeName(saveName); curFactory = -1; std::copy(val, val + kParamCount, userVals); saving = false; saveError = false; }
                else saveError = true;
            } else endEdit(true);
            repaint(); return true;
        }
        if (ev.key == kKeyEscape) { if (saving) saving = false; else endEdit(false); repaint(); return true; }
        if (ev.key == kKeyBackspace || ev.key == kKeyDelete) { if (!buf->empty()) buf->pop_back(); repaint(); return true; }
        if (ev.key >= 32 && ev.key < 127 && buf->size() < (saving ? 40u : 12u)) { *buf += (char)ev.key; repaint(); return true; }
        return true;
    }

    bool onMouse(const MouseEvent& ev) override {
        const float s = uiScale(), x = ev.pos.getX() / s, y = ev.pos.getY() / s;
        if (saving) return true;
        if (ev.press) { statusFlash = 0; tipShown = false; tipSuppressed = tipKey; }
        if (helpOpen) { if (ev.press && ev.button == 1) helpClick(x, y); return true; }
        if (ev.button == 3 && ev.press) {
            if (menuOpen) { const int m = menuHit(x, y); if (m >= 0 && menuItem(m).kind == 1) { pendingDelete = menuItem(m).idx; repaint(); } return true; }
            if (learnMode) { const int i = hit(x, y); if (i >= 0) { const uint32_t p = controls()[i].param; char b[24]; snprintf(b, sizeof b, "unmap:%u", p); setState("cmd", b); paramCC[p] = -1; repaint(); } return true; }
            return false;
        }
        if (ev.button != 1) return false;
        if (!ev.press) {
            if (active >= 0) { editParameter(controls()[active].param, false); active = -1; endGesture(); return true; }
            if (dragThr || dragCurve) { editParameter(kThreshold, false); dragThr = dragCurve = false; endGesture(); return true; }
            return false;
        }
        if (editing >= 0) endEdit(true);
        if (menuOpen) {
            const int m = menuHit(x, y);
            if (m >= 0) {
                const MRect r = menuItem(m);
                if (r.kind == 2) { menuOpen = false; saving = true; saveError = false; saveName = curUser; }
                else if (r.kind == 1 && pendingDelete == r.idx) { deleteUserPreset(userPresets[r.idx]); if (curUser == userPresets[r.idx]) curUser.clear(); userPresets = listUserPresets(); pendingDelete = -1; repaint(); return true; }
                else { menuOpen = false; if (r.kind == 0) applyFactory(r.idx); else applyUser(userPresets[r.idx]); }
            } else menuOpen = false;
            pendingDelete = -1; menuHover = -1; repaint(); return true;
        }
        const int hh = headerHit(x, y);
        if (hh == 1) { setLearn(!learnMode); return true; }
        if (hh == 2) { beginGesture(); commit(kMidiOn, val[kMidiOn] > 0.5f ? 0.f : 1.f); endGesture(); return true; }
        if (hh == 3) { undo(); return true; }
        if (hh == 4) { redo(); return true; }
        if (hh == 5) { helpOpen = true; repaint(); return true; }
        if (hh == 6 || hh == 7) { switchAB(hh - 6); return true; }
        if (hh == 8) { copyAB(); return true; }
        const int pb = presetBarHit(x, y);
        if (pb == 0) { step(-1); return true; }
        if (pb == 2) { step(1); return true; }
        if (pb == 1) { userPresets = listUserPresets(); menuOpen = true; menuHover = -1; pendingDelete = -1; repaint(); return true; }
        if (clearHit(x, y)) { setState("cmd", "clear"); for (int& m : paramCC) m = -1; repaint(); return true; }

        int seg = 0; const int i = hit(x, y, &seg);
        if (i >= 0) {
            const Ctl& c = controls()[i]; const uint32_t p = c.param;
            if (learnMode) { learnWaiting = (int)p; char b[24]; snprintf(b, sizeof b, "learn:%u", p); setState("cmd", b); repaint(); return true; }
            if (c.type == Toggle) { beginGesture(); commit(p, val[p] > 0.5f ? 0.f : 1.f); endGesture(); return true; }
            if (c.type == Segment) { beginGesture(); commit(p, (float)seg); endGesture(); return true; }
            if (ev.mod & (kModifierControl | kModifierSuper)) { beginGesture(); commit(p, defv[p]); endGesture(); return true; }
            if (i == lastClick && ev.time - lastClickTime < 350) {
                lastClick = -1; editing = i;
                char b[40]; fmt(p, b, sizeof b); editBuf = b;
                const size_t sp = editBuf.find_first_of(" :%"); if (sp != std::string::npos) editBuf.resize(sp);
                repaint(); return true;
            }
            lastClick = i; lastClickTime = ev.time;
            beginGesture();
            active = i; dragY = y; dragNorm = toNorm(p, val[p]);
            circular = std::sqrt((x - c.x) * (x - c.x) + (y - c.y) * (y - c.y)) > c.r * 0.62f;
            editParameter(p, true);
            if (circular) setValue(p, fromNorm(p, angleNorm(c, x, y)));
            repaint(); return true;
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
        if (helpOpen) { const int h = helpHit(x, y); if (h != helpHover) { helpHover = h; repaint(); } return true; }
        if (active >= 0) {
            const Ctl& c = controls()[active]; const uint32_t p = c.param;
            if (circular) { setValue(p, fromNorm(p, angleNorm(c, x, y))); return true; }
            float sens = (ev.mod & kModifierShift) ? 1500.f : 240.f;
            if (paramDef(p).hints & kHintInteger) sens = 80.f;
            dragNorm = clampf(dragNorm + (dragY - y) / sens, 0.f, 1.f); dragY = y;
            setValue(p, fromNorm(p, dragNorm));
            return true;
        }
        if (dragThr) { setValue(kThreshold, std::round(clampf(-(y - LY - 10) / (LH - 30) * 60.f, -60.f, 0.f) * 10) / 10); return true; }
        if (dragCurve) { setValue(kThreshold, std::round(clampf((x - TX - 26) / (TW - 40) * 60.f - 60.f, -60.f, 0.f) * 10) / 10); return true; }
        if (menuOpen) { const int m = menuHit(x, y); if (m != menuHover) { menuHover = m; repaint(); } return true; }
        const int hh = headerHit(x, y); if (hh != hoverHeader) { hoverHeader = hh; repaint(); }
        const int pb = presetBarHit(x, y); if (pb != presetHover) { presetHover = pb; repaint(); }
        const bool hc = clearHit(x, y); if (hc != hoverClear) { hoverClear = hc; repaint(); }
        const int h = hit(x, y); if (h != hover) { hover = h; repaint(); }
        const int a = h >= 0 ? -1 : areaHit(x, y);
        const bool ht = a == 0 && std::fabs(y - yOfDbL(val[kThreshold])) < 8; if (ht != hoverThr) { hoverThr = ht; repaint(); }
        const bool hcv = a == 1; if (hcv != hoverCurve) { hoverCurve = hcv; repaint(); }
        const int key = h >= 0 ? h : (hh > 0 ? 1000 + hh : (pb >= 0 ? 2000 + pb : (a >= 0 ? 4000 + a : -1)));
        tipX = x; tipY = y;
        if (key != tipKey) { const bool was = tipShown; tipKey = key; tipSince = nowSec(); tipShown = false; if (key != tipSuppressed) tipSuppressed = -2; if (was) repaint(); }
        return false;
    }

    bool onScroll(const ScrollEvent& ev) override {
        const float s = uiScale(), sx = ev.pos.getX() / s, sy = ev.pos.getY() / s;
        if (menuOpen || saving || helpOpen) return true;
        if (presetBarHit(sx, sy) >= 0) { step(ev.delta.getY() > 0 ? -1 : 1); return true; }
        const int i = hit(sx, sy);
        if (i < 0 || controls()[i].type != Knob) return false;
        const uint32_t p = controls()[i].param;
        const float st = (ev.mod & kModifierShift) ? 0.004f : 0.02f;
        beginGesture(); commit(p, fromNorm(p, toNorm(p, val[p]) + (ev.delta.getY() > 0 ? st : -st))); endGesture();
        return true;
    }

private:
    float val[kParamCount], defv[kParamCount], userVals[kParamCount] = {}, before[kParamCount];
    int paramCC[kParamCount];
    int active = -1, hover = -1, lastClick = -1, editing = -1;
    uint lastClickTime = 0;
    float dragY = 0, dragNorm = 0; bool circular = false, gesture = false;
    bool dragThr = false, dragCurve = false, hoverThr = false, hoverCurve = false;
    std::vector<Snap> undoStack, redoStack;
    Snap abStore[2]; int abSlot = 0;
    float hIn[HIST], hOut[HIST], hGr[HIST]; int hPos = 0; double lastHist = 0;
    std::string editBuf, statusText; int statusFlash = 0;
    int curFactory = 0, presetHover = -1, menuHover = -1, pendingDelete = -1, hoverHeader = 0;
    std::string curUser; std::vector<std::string> userPresets;
    bool menuOpen = false, saving = false, saveError = false; std::string saveName;
    bool learnMode = false, hoverClear = false; int learnWaiting = -1; float lastLearnEcho = 0;
    int tipKey = -1, tipSuppressed = -2; double tipSince = 0; bool tipShown = false; float tipX = 0, tipY = 0;
    bool helpOpen = false; int helpPage = 0, helpHover = -1;
    ResizeHandle resizeHandle;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GneissUI)
};

UI* createUI() { return new GneissUI(); }

END_NAMESPACE_DISTRHO
