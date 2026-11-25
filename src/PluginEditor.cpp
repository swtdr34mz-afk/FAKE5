/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (FAKE5) */
#include "PluginEditor.h"

using namespace f5ui;
using namespace f5;
using Panel = Fake5Panel;

namespace
{
constexpr float KD = 70.0f;                      /* a knob's size (with its scale) */
constexpr float BW = 34.0f, BH = 25.0f;          /* a button's */
constexpr float LS = 11.0f;                      /* the legends' size */
constexpr double SHOW_FOR = 1.6;                 /* a touched control's value on the display: seconds */

/* the destination a panel control is, for DEST ASSIGN (-1: not one) */
int destOf(int p)
{
    switch (p) {
    case P_O1_OCT: return MD_OSC1_OCT;
    case P_O2_OCT: return MD_OSC2_OCT;
    case P_O1_PITCH: return MD_OSC1_FINE;
    case P_O2_PITCH: return MD_OSC2_FINE;
    case P_O1_SHAPE: return MD_OSC1_SHAPE;
    case P_O2_SHAPE: return MD_OSC2_SHAPE;
    case P_O1_LEVEL: return MD_OSC1_LEVEL;
    case P_O2_LEVEL: return MD_OSC2_LEVEL;
    case P_SUB: return MD_SUB;
    case P_NOISE: return MD_NOISE;
    case P_DRIVE: return MD_DRIVE;
    case P_CUTOFF: return MD_CUTOFF;
    case P_RESO: return MD_RESO;
    case P_VINTAGE: return MD_VINTAGE;
    case P_OVERDRIVE: return MD_OVERDRIVE;
    case P_FX_MIX: return MD_FX_MIX;
    case P_FX_TIME: return MD_FX_TIME;
    case P_FX_MISC: return MD_FX_MISC;
    case P_REV_MIX: return MD_REV_MIX;
    case P_REV_DAMP: return MD_REV_SIZE;
    case P_REV_PREDELAY: return MD_REV_PREDELAY;
    case P_REV_DECAY: return MD_REV_DECAY;
    case P_REV_TONE: return MD_REV_TONE;
    case P_LFO1 + LFO_FREQ: return MD_LFO1_FREQ;
    case P_LFO1 + LFO_N + LFO_FREQ: return MD_LFO2_FREQ;
    case P_LFO1 + LFO_AMT: return MD_LFO1_AMT;
    case P_LFO1 + LFO_N + LFO_AMT: return MD_LFO2_AMT;
    case P_E1_AMT: return MD_ENV1_AMT;
    case P_E2_AMT: return MD_ENV2_AMT;
    case P_E1_ATT: return MD_ENV1_ATT;
    case P_E2_ATT: return MD_ENV2_ATT;
    case P_E1_DEC: return MD_ENV1_DEC;
    case P_E2_DEC: return MD_ENV2_DEC;
    case P_E1_REL: return MD_ENV1_REL;
    case P_E2_REL: return MD_ENV2_REL;
    default: return -1;
    }
}

/* ------------------------------------------------------------------------------- a knob --- */
/* a parameter's knob (target: which, now: the LFO chosen), or an endless encoder (onTurn, or a parameter stepped) */
class Knob : public juce::Component, public Panel::Live
{
public:
    using Target = std::function<juce::RangedAudioParameter *()>;
    Knob(Panel &p, Target t, std::function<void(juce::RangedAudioParameter *)> touched, KnobKind k = KnobKind::pot)
        : panel(p), target(std::move(t)), onTouch(std::move(touched)), kind(k), shownKind(k), bodyKind(k)
    {
    }
    Knob(Panel &p, std::function<void(int)> turn)
        : panel(p), onTurn(std::move(turn)), kind(KnobKind::encoder), shownKind(KnobKind::encoder), bodyKind(KnobKind::pot)
    {
    }
    bool stepped = false;                        /* an encoder bound to a parameter: each detent one step */

    void paint(juce::Graphics &g) override
    {
        /* the body drawn once at the screen's density and kept; only the line drawn each time */
        float sc = g.getInternalContext().getPhysicalPixelScaleFactor();
        auto r = getLocalBounds().toFloat();
        if (body.isNull() || sc != bodyScale || bodyKind != shownKind) {
            bodyScale = sc;
            bodyKind = shownKind;
            body = juce::Image(juce::Image::ARGB, juce::jmax(1, (int)std::ceil(r.getWidth() * sc)), juce::jmax(1, (int)std::ceil(r.getHeight() * sc)), true);
            juce::Graphics bg(body);
            bg.addTransform(juce::AffineTransform::scale(sc));
            drawKnob(bg, r, shownKind, true);
        }
        g.drawImage(body, r, juce::RectanglePlacement::stretchToFit);
        drawKnobPointer(g, r, shownPos, shownKind);
    }
    void poll() override
    {
        if (!target)
            return;
        auto *p = target();
        float pos = p ? p->getValue() : 0.0f;
        KnobKind k = kind;
        if (kind == KnobKind::pot)
            if (auto *ip = dynamic_cast<juce::AudioParameterInt *>(p))
                k = ip->getRange().getStart() < 0 ? KnobKind::bipolar : KnobKind::pot;
        if (pos != shownPos || k != shownKind) {
            shownPos = pos;
            shownKind = k;
            repaint();
        }
    }
    void mouseDown(const juce::MouseEvent &e) override
    {
        startY = e.position.y;
        acc = 0.0f;
        if (!target)
            return;
        if (auto *p = target()) {
            startV = p->getValue();
            p->beginChangeGesture();
            if (onTouch)
                onTouch(p);
        }
    }
    void mouseDrag(const juce::MouseEvent &e) override
    {
        float dy = startY - e.position.y;
        bool fine = e.mods.isShiftDown();
        if (!target || stepped) {
            float step = fine ? 14.0f : 7.0f;
            int n = (int)((dy - acc) / step);
            if (n != 0) {
                acc += (float)n * step;
                turn(n);
            }
            return;
        }
        auto *p = target();
        if (!p)
            return;
        if (auto *ip = dynamic_cast<juce::AudioParameterInt *>(p)) {
            int lo = ip->getRange().getStart(), hi = ip->getRange().getEnd(), range = hi - lo;
            if (range <= 40) {                       /* a few values: a step every so many pixels */
                float px = juce::jlimit(3.0f, 18.0f, 200.0f / (float)juce::jmax(1, range)) * (fine ? 3.0f : 1.0f);
                int v0 = (int)std::lround(p->convertFrom0to1(startV));
                set(p, juce::jlimit(lo, hi, v0 + (int)std::lround(dy / px)));
            } else {                                 /* many: the travel in 220 pixels (fine: 900) */
                float v = juce::jlimit(0.0f, 1.0f, startV + dy / (fine ? 900.0f : 220.0f));
                set(p, (int)std::lround(p->convertFrom0to1(v)));
            }
        }
    }
    void mouseUp(const juce::MouseEvent &) override
    {
        if (target && !stepped)
            if (auto *p = target())
                p->endChangeGesture();
    }
    void mouseWheelMove(const juce::MouseEvent &, const juce::MouseWheelDetails &w) override
    {
        int d = w.deltaY > 0 ? 1 : w.deltaY < 0 ? -1 : 0;
        if (d == 0)
            return;
        if (!target || stepped) {
            turn(d);
            return;
        }
        if (auto *p = target()) {
            if (onTouch)
                onTouch(p);
            if (auto *ip = dynamic_cast<juce::AudioParameterInt *>(p)) {
                int range = ip->getRange().getEnd() - ip->getRange().getStart();
                int step = range > 300 ? 8 : 1;
                p->beginChangeGesture();
                set(p, juce::jlimit(ip->getRange().getStart(), ip->getRange().getEnd(), ip->get() + d * step));
                p->endChangeGesture();
            }
        }
    }
    void mouseDoubleClick(const juce::MouseEvent &) override
    {
        if (!target || stepped)
            return;
        if (auto *p = target()) {
            p->beginChangeGesture();
            p->setValueNotifyingHost(p->getDefaultValue());
            p->endChangeGesture();
        }
    }

private:
    void turn(int n)
    {
        if (onTurn) {
            onTurn(n);
            return;
        }
        if (auto *p = target())
            if (auto *ip = dynamic_cast<juce::AudioParameterInt *>(p)) {
                if (onTouch)
                    onTouch(p);
                p->beginChangeGesture();
                set(p, juce::jlimit(ip->getRange().getStart(), ip->getRange().getEnd(), ip->get() + n));
                p->endChangeGesture();
            }
    }
    void set(juce::RangedAudioParameter *p, int v) { p->setValueNotifyingHost(p->convertTo0to1((float)v)); }
    Panel &panel;
    Target target;
    std::function<void(juce::RangedAudioParameter *)> onTouch;
    std::function<void(int)> onTurn;
    KnobKind kind, shownKind, bodyKind;
    float shownPos = -1.0f, startY = 0.0f, acc = 0.0f, bodyScale = 0.0f, startV = 0.0f;
    juce::Image body;
};

/* ------------------------------------------------------------------------------ a button --- */
class Btn : public juce::Component, public Panel::Live
{
public:
    Btn(std::function<void()> press, std::function<bool()> on) : onPress(std::move(press)), isOn(std::move(on))
    {
        setRepaintsOnMouseActivity(true);
    }
    void paint(juce::Graphics &g) override
    {
        float sc = g.getInternalContext().getPhysicalPixelScaleFactor();
        int k = (lit ? 1 : 0) | (down ? 2 : 0) | (isMouseOver() ? 4 : 0);
        auto r = getLocalBounds().toFloat();
        auto big = r;
        auto face = r.withSizeKeepingCentre(BW, BH);
        if (sc != cacheScale) {
            for (auto &c : cache)
                c = juce::Image();
            cacheScale = sc;
        }
        if (cache[k].isNull()) {
            cache[k] = juce::Image(juce::Image::ARGB, juce::jmax(1, (int)std::ceil(big.getWidth() * sc)), juce::jmax(1, (int)std::ceil(big.getHeight() * sc)), true);
            juce::Graphics cg(cache[k]);
            cg.addTransform(juce::AffineTransform::scale(sc));
            drawButton(cg, face, lit, down, isMouseOver());
        }
        g.drawImage(cache[k], big, juce::RectanglePlacement::stretchToFit);
    }
    bool hitTest(int x, int y) override
    {
        return getLocalBounds().toFloat().withSizeKeepingCentre(BW + 8.0f, BH + 8.0f).contains((float)x, (float)y);
    }
    void poll() override
    {
        bool l = isOn ? isOn() : false;
        if (l != lit) {
            lit = l;
            repaint();
        }
    }
    void mouseDown(const juce::MouseEvent &) override
    {
        down = true;
        repaint();
        if (onPress)
            onPress();
        poll();
    }
    void mouseUp(const juce::MouseEvent &) override
    {
        down = false;
        repaint();
    }

private:
    std::function<void()> onPress;
    std::function<bool()> isOn;
    bool lit = false, down = false;
    juce::Image cache[8];
    float cacheScale = 0.0f;
};

/* ----------------------------------------------------------------------------- LEDs --- */
class Leds : public juce::Component, public Panel::Live
{
public:
    Leds(std::vector<juce::Point<float>> at, std::function<float(int)> level, std::function<void(int)> pick = {})
        : pts(std::move(at)), level(std::move(level)), pick(std::move(pick)), shown(pts.size(), -1.0f)
    {
    }
    void paint(juce::Graphics &g) override
    {
        for (size_t i = 0; i < pts.size(); i++)
            drawLed(g, pts[i] - getPosition().toFloat(), 7.0f, shown[i]);
    }
    void poll() override
    {
        bool ch = false;
        for (size_t i = 0; i < pts.size(); i++) {
            float v = std::round(juce::jlimit(0.0f, 1.0f, level((int)i)) * 16.0f) / 16.0f;
            if (v != shown[i]) {
                shown[i] = v;
                ch = true;
            }
        }
        if (ch)
            repaint();
    }
    void mouseDown(const juce::MouseEvent &e) override
    {
        if (!pick)
            return;
        float best = 1e9f;
        int bi = 0;
        for (size_t i = 0; i < pts.size(); i++) {
            float d = (pts[i] - getPosition().toFloat()).getDistanceFrom(e.position);
            if (d < best) {
                best = d;
                bi = (int)i;
            }
        }
        pick(bi);
    }
    void fit()
    {
        float x0 = 1e9f, y0 = 1e9f, x1 = -1e9f, y1 = -1e9f;   /* (a union of empty rectangles stays empty: by hand) */
        for (auto &p : pts) {
            x0 = juce::jmin(x0, p.x);
            y0 = juce::jmin(y0, p.y);
            x1 = juce::jmax(x1, p.x);
            y1 = juce::jmax(y1, p.y);
        }
        setBounds(juce::Rectangle<float>(x0, y0, x1 - x0, y1 - y0).expanded(16.0f).toNearestInt());
    }

private:
    std::vector<juce::Point<float>> pts;
    std::function<float(int)> level;
    std::function<void(int)> pick;
    std::vector<float> shown;
};

/* --------------------------------------------------------------------- the program display --- */
class Seg : public juce::Component, public Panel::Live
{
public:
    Seg(Panel &p) : panel(p) {}
    void poll() override
    {
        juce::String s = text();
        if (s != last) {
            last = s;
            repaint();
        }
    }
    void paint(juce::Graphics &g) override
    {
        bool dots[3] = {false, false, last.endsWith(".")};
        drawSevenSeg(g, getLocalBounds().toFloat(), last.upToFirstOccurrenceOf(".", false, false), dots);
    }
    void mouseDown(const juce::MouseEvent &) override { panel.programMenu(getScreenBounds()); }

private:
    juce::String text() const
    {
        int pr = panel.proc.getCurrentProgram();
        int b = panel.bankOf(pr);
        bool fac = pr < NFACTORY;
        int idx = fac ? pr : pr - NFACTORY;
        juce::String s = juce::String(b + 1) + juce::String(idx % 16 + 1).paddedLeft('0', 2);
        if (!fac)
            s = "U" + juce::String(idx % 16 + 1).paddedLeft('0', 2);
        return s + (panel.proc.edited() ? "." : "");
    }
    Panel &panel;
    juce::String last;
};
} // namespace

/* ---------------------------------------------------------------------------------- OLED --- */
class Oled : public juce::Component, public Panel::Live
{
public:
    explicit Oled(Panel &p) : panel(p) {}
    void poll() override
    {
        juce::String s = signature();
        if (s != last) {
            last = s;
            repaint();
        }
    }
    void paint(juce::Graphics &g) override
    {
        using namespace juce;
        auto r = getLocalBounds().toFloat();
        g.setColour(Colour(0xff000000));
        g.fillRoundedRectangle(r, 2.0f);
        auto in = r.reduced(7.0f, 5.0f);
        auto &proc = panel.proc;
        auto mono = [&](float h) { return font(Face::mono, h); };
        auto sans = [&](float h) { return font(Face::label, h); };
        bool transient = Time::getMillisecondCounterHiRes() * 0.001 - panel.shownAt < SHOW_FOR;
        auto page = panel.page;
        if (page == Panel::P_HOME && panel.msg.isNotEmpty() && transient) {
            g.setColour(col::oled);
            g.setFont(sans(15.0f));
            g.drawFittedText(panel.msg, in.toNearestInt(), Justification::centred, 3, 0.8f);
            return;
        }
        if (page == Panel::P_HOME) {
            /* the corners: BPM, CLK DIVIDE; ARP, EFFECT; the program large between */
            g.setColour(col::oledDim);
            g.setFont(sans(10.5f));
            g.drawText("BPM", in.withHeight(12.0f), Justification::centredLeft, false);
            g.drawText("CLK DIVIDE", in.withHeight(12.0f), Justification::centredRight, false);
            g.setColour(col::oled);
            g.setFont(mono(14.0f));
            auto row2 = in.withTrimmedTop(12.0f).withHeight(15.0f);
            g.drawText(String((int)std::lround(proc.bpmNow.load())), row2, Justification::centredLeft, false);
            g.drawText(DIVIDE_NAME[jlimit(0, NDIVIDE - 1, proc.value(P_DIVIDE))], row2, Justification::centredRight, false);
            auto mid = in.withTrimmedTop(30.0f).withTrimmedBottom(30.0f);
            if (panel.shownParam >= 0 || panel.shownGlobal) {
                String n, v;
                what(n, v);
                if (transient) {
                    g.setFont(sans(12.5f));
                    g.drawText(n, mid.removeFromTop(14.0f), Justification::centred, true);
                    g.setFont(sans(v.length() > 11 ? 16.0f : 21.0f));
                    g.drawFittedText(v, mid.toNearestInt(), Justification::centred, 1, 0.6f);
                } else
                    name(g, mid);
            } else
                name(g, mid);
            auto bot = in.withTrimmedTop(in.getHeight() - 28.0f);
            g.setFont(mono(13.0f));
            g.setColour(proc.value(P_ARP_ON) ? col::oled : col::oledDim);
            g.drawText(PARAMS[P_ARP_MODE].choice[jlimit(0, NARPMODE - 1, proc.value(P_ARP_MODE))], bot.withHeight(15.0f), Justification::centredLeft, false);
            g.setColour(proc.value(P_FX_ON) ? col::oled : col::oledDim);
            g.drawText(FXTYPE_SHORT[jlimit(0, NFXTYPE - 1, proc.value(P_FX_TYPE))], bot.withHeight(15.0f), Justification::centredRight, false);
            g.setColour(col::oledDim);
            g.setFont(sans(10.5f));
            g.drawText("ARP", bot.withTrimmedTop(15.0f), Justification::centredLeft, false);
            g.drawText("EFFECT", bot.withTrimmedTop(15.0f), Justification::centredRight, false);
            return;
        }
        if (page == Panel::P_SEQ) {
            auto s = proc.seqCopy();
            bool rec = proc.recording.load();
            int st = rec ? proc.recStepNow.load() : jmax(0, proc.seqStepNow.load());
            g.setColour(col::oled);
            g.setFont(sans(13.5f));
            String head = rec ? (proc.value(P_SEQ_PLAY) ? "TRANSPOSE: play a key" : "RECORD") : "SEQUENCER";
            g.drawText(head, in.removeFromTop(16.0f), Justification::centredLeft, true);
            g.setFont(mono(13.0f));
            g.drawText("Step " + String(st + 1) + " / " + String(jmax<int>(s.length, rec ? st + 1 : 1)) +
                           (proc.seqTransNow.load() ? "  T" + String(proc.seqTransNow.load() > 0 ? "+" : "") + String(proc.seqTransNow.load()) : String()),
                       in.removeFromTop(16.0f), Justification::centredLeft, false);
            static const char *const N[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
            int show = rec && st > 0 ? st - 1 : st;
            String notes;
            for (int k = 0; k < SEQ_NOTES; k++) {
                int n = s.note[show % SEQ_STEPS][k];
                if (n == PN_TIE && k == 0)
                    notes = "tie";
                else if (n >= 2)
                    notes += String(N[(n - 2) % 12]) + String((n - 2) / 12 - 2) + " ";
            }
            g.drawFittedText(notes.isEmpty() ? String("rest") : notes, in.removeFromTop(30.0f).toNearestInt(), Justification::centredTop, 2, 0.7f);
            g.setColour(col::oledDim);
            g.setFont(sans(10.5f));
            g.drawText(rec ? "program: rest   write: tie" : "PLAY starts, RECORD records", in, Justification::centredLeft, true);
            return;
        }
        /* a page of lines: the one in hand large */
        auto es = panel.entries();
        if (es.empty())
            return;
        int it = jlimit(0, (int)es.size() - 1, panel.item);
        String title = page == Panel::P_PROGRAM ? "PROGRAM" : page == Panel::P_GLOBAL ? "GLOBAL" : page == Panel::P_MOD
                                                                                       ? "MOD " + String(panel.modSlot + 1)
                                                                                       : "LFO " + String(panel.lfoSel + 1) + " DEST";
        g.setColour(col::oledDim);
        g.setFont(sans(11.0f));
        g.drawText(title, in.removeFromTop(13.0f), Justification::centredLeft, false);
        g.setColour(col::oled);
        if (page == Panel::P_MOD) {
            /* the slot as the Take 5 shows it: source, amount, destination, each M or P */
            for (int i = 1; i < (int)es.size(); i++) {
                auto row = in.removeFromTop(i == 2 ? 26.0f : 20.0f);
                bool sel = i == it;
                if (sel) {
                    g.setColour(col::oled.withAlpha(0.16f));
                    g.fillRoundedRectangle(row, 2.0f);
                }
                g.setColour(col::oled);
                g.setFont(i == 2 ? mono(19.0f) : sans(14.0f));
                g.drawText(es[(size_t)i].text(es[(size_t)i].get()), row.reduced(4.0f, 0.0f), i == 2 ? Justification::centred : Justification::centredLeft, true);
                if (i != 2) {
                    int v = es[(size_t)i].get();
                    bool poly = i == 1 ? MODSRC_POLY[jlimit(0, NMODSRC - 1, v)] : MODDST_POLY[jlimit(0, NMODDST - 1, v)];
                    g.setFont(mono(12.0f));
                    g.drawText(poly ? "P" : "M", row.reduced(4.0f, 0.0f), Justification::centredRight, false);
                }
            }
            if (it == 0) {
                g.setColour(col::oledDim);
                g.setFont(sans(10.5f));
                g.drawText("SELECT: slot / line", r.reduced(7.0f, 4.0f), Justification::bottomRight, false);
            }
            return;
        }
        g.setFont(sans(14.0f));
        g.drawText(String(it + 1) + ". " + es[(size_t)it].label, in.removeFromTop(18.0f), Justification::centredLeft, true);
        auto &e = es[(size_t)it];
        int v = e.get();
        g.setFont(sans(22.0f));
        g.drawFittedText(e.text ? e.text(v) : String(v), in.removeFromTop(32.0f).toNearestInt(), Justification::centred, 1, 0.6f);
        if (it + 1 < (int)es.size()) {
            g.setColour(col::oledDim);
            g.setFont(sans(11.5f));
            g.drawText(String(it + 2) + ". " + es[(size_t)it + 1].label, in, Justification::centredLeft, true);
        }
    }
    void mouseWheelMove(const juce::MouseEvent &, const juce::MouseWheelDetails &w) override
    {
        int d = w.deltaY > 0 ? 1 : w.deltaY < 0 ? -1 : 0;
        if (d)
            panel.valueTurn(d);
    }
    void mouseDown(const juce::MouseEvent &) override
    {
        if (panel.page == Panel::P_HOME)
            panel.programMenu(getScreenBounds());
        else if (panel.page != Panel::P_SEQ)
            panel.entryMenu(getScreenBounds());
    }

private:
    void name(juce::Graphics &g, juce::Rectangle<float> mid)
    {
        g.setColour(col::oled);
        g.setFont(font(Face::label, 20.0f));
        g.drawFittedText(panel.proc.programName(), mid.toNearestInt(), juce::Justification::centred, 1, 0.65f);
    }
    void what(juce::String &name, juce::String &val) const
    {
        if (panel.shownGlobal) {
            name = panel.shownGlobal->getName(30);
            val = panel.shownGlobal->getCurrentValueAsText();
            return;
        }
        int p = panel.shownParam;
        name = PARAMS[p].name;
        if (p == P_FX_TIME || p == P_FX_MIX || p == P_FX_MISC)
            name = juce::String(FXTYPE_SHORT[panel.proc.value(P_FX_TYPE)]) + " " + fxKnobName(panel.proc.value(P_FX_TYPE), p - P_FX_TIME);
        val = panel.valueText(p, panel.proc.value(p));
    }
    juce::String signature() const
    {
        auto &proc = panel.proc;
        bool transient = juce::Time::getMillisecondCounterHiRes() * 0.001 - panel.shownAt < SHOW_FOR;
        juce::String s;
        s << (int)panel.page << "|" << panel.item << "|" << proc.getCurrentProgram() << "|" << (int)std::lround(proc.bpmNow.load()) << "|"
          << (int)transient << "|" << panel.shownParam << "|" << panel.msg << "|" << panel.modSlot << "|" << panel.lfoSel << "|"
          << proc.value(P_DIVIDE) << proc.value(P_ARP_MODE) << proc.value(P_ARP_ON) << proc.value(P_FX_TYPE) << proc.value(P_FX_ON);
        juce::String n, v;
        if (panel.page == Panel::P_HOME) {
            if (panel.shownParam >= 0 || panel.shownGlobal)
                what(n, v);
            s << n << v << proc.programName();
        } else if (panel.page == Panel::P_SEQ) {
            auto sd = proc.seqCopy();
            uint64_t h = 1469598103934665603ull;
            for (size_t i = 0; i < sizeof sd; i++)
                h = (h ^ reinterpret_cast<const uint8_t *>(&sd)[i]) * 1099511628211ull;
            s << (juce::int64)h << proc.seqStepNow.load() << "|" << proc.recStepNow.load() << (int)proc.recording.load()
              << proc.seqTransNow.load() << proc.value(P_SEQ_PLAY);
        } else {
            for (auto &e : panel.entries())
                s << e.get() << ",";
        }
        return s;
    }
    Panel &panel;
    juce::String last;
};

/* ============================================================================ the panel === */
template <class T, class... A> T *Panel::add(A &&...a)
{
    auto c = std::make_unique<T>(std::forward<A>(a)...);
    T *raw = c.get();
    addAndMakeVisible(*raw);
    if (auto *lv = dynamic_cast<Live *>(raw))
        live.push_back(lv);
    parts.push_back(std::move(c));
    return raw;
}

Panel::Fake5Panel(Fake5Processor &p) : proc(p)
{
    setLookAndFeel(&look);
    setSize(W, H);
    bank = bankOf(proc.getCurrentProgram());
    build();
    startTimerHz(30);
}
Panel::~Fake5Panel()
{
    stopTimer();
    if (dialog != nullptr)                       /* (an open WRITE dialog goes with the editor: it uses its look) */
        delete dialog.getComponent();
    setLookAndFeel(nullptr);
}

juce::String Panel::valueText(int p, int v) const
{
    char t[40];
    bool synced = false;
    int fxType = proc.value(P_FX_TYPE);
    if (p == P_LFO1 + LFO_FREQ || p == P_LFO1 + LFO_N + LFO_FREQ)
        synced = proc.value(p - LFO_FREQ + LFO_SYNC) != 0;
    if (p == P_FX_TIME)
        synced = proc.value(P_FX_SYNC) != 0;
    paramText(p, v, t, sizeof t, synced, fxType);
    return juce::String(t);
}

void Panel::message(const juce::String &m)
{
    msg = m;
    shownParam = -1;
    shownGlobal = nullptr;
    shownAt = juce::Time::getMillisecondCounterHiRes() * 0.001;
    page = P_HOME;
}

void Panel::touch(int p)
{
    /* DEST ASSIGN armed: this control becomes the slot's destination (or the LFO's) */
    if (armDest || armLfoDest) {
        int d = destOf(p);
        if (d >= 0) {
            if (armDest) {
                proc.setValue(P_MOD1 + modSlot * MOD_N + MOD_DST, d);
                armDest = false;
                openPage(P_MOD, 3);
            } else {
                proc.setValue(P_LFO1 + lfoSel * LFO_N + LFO_DEST, d);
                armLfoDest = false;
                openPage(P_LFODEST, 0);
            }
            return;
        }
    }
    shownParam = p;
    shownGlobal = nullptr;
    msg.clear();
    shownAt = juce::Time::getMillisecondCounterHiRes() * 0.001;
    if (page != P_SEQ)
        page = P_HOME;
}
void Panel::touchGlobal(juce::RangedAudioParameter *p)
{
    shownGlobal = p;
    shownParam = -1;
    msg.clear();
    shownAt = juce::Time::getMillisecondCounterHiRes() * 0.001;
    if (page != P_SEQ)
        page = P_HOME;
}

int Panel::bankOf(int pr) const { return pr < NFACTORY ? pr / 16 : (pr - NFACTORY) / 16; }

void Panel::gotoProgram(int index)
{
    if (index < 0 || index >= proc.getNumPrograms())
        return;
    proc.setCurrentProgram(index);
    bank = bankOf(index);
    shownParam = -1;
    shownGlobal = nullptr;
    msg.clear();
    page = P_HOME;
}

/* the pages' lines */
std::vector<Panel::Entry> Panel::entries() const
{
    std::vector<Entry> es;
    auto *self = const_cast<Panel *>(this);
    auto pe = [&](int p, const char *label = nullptr) {
        Entry e;
        e.label = label ? label : PARAMS[p].name;
        e.lo = PARAMS[p].lo;
        e.hi = PARAMS[p].hi;
        e.get = [self, p] { return self->proc.value(p); };
        e.set = [self, p](int v) { self->proc.setValue(p, v); };
        e.text = [self, p](int v) { return self->valueText(p, v); };
        es.push_back(e);
    };
    switch (page) {
    case P_PROGRAM:
        /* the Take 5's program menu, numbered as its display does */
        for (int p : std::initializer_list<int>{P_O1_KEY, P_O2_KEY, P_O1_GLIDE, P_O2_GLIDE, P_O1_PITCH, P_O2_PITCH, P_KEYAMT, P_E1_DEL, P_E2_DEL,
                      P_UNI_KEYMODE, P_UNI_DETUNE, P_UNI_RETRIG, P_GLIDE_MODE, P_ARP_RELATCH, P_PGM_VOL, P_PB_UP, P_PB_DOWN,
                      P_LFO1 + LFO_RESET, P_LFO1 + LFO_N + LFO_RESET, P_LFO1 + LFO_SLEW, P_LFO1 + LFO_N + LFO_SLEW,
                      P_ENV_REPEAT, P_O2_BYPASS, P_E1_DEST, P_UNI_VOICES, P_SPLIT_POINT})
            pe(p);
        break;
    case P_MOD: {
        Entry e;
        e.label = "Mod Slot";
        e.lo = 0;
        e.hi = NMOD - 1;
        e.get = [self] { return self->modSlot; };
        e.set = [self](int v) { self->modSlot = juce::jlimit(0, NMOD - 1, v); };
        e.text = [](int v) { return "MOD " + juce::String(v + 1); };
        es.push_back(e);
        int b = P_MOD1 + modSlot * MOD_N;
        pe(b + MOD_SRC, "Source");
        pe(b + MOD_AMT, "Amount");
        pe(b + MOD_DST, "Destination");
        break;
    }
    case P_LFODEST: pe(P_LFO1 + lfoSel * LFO_N + LFO_DEST, lfoSel ? "LFO 2 Destination" : "LFO 1 Destination"); break;
    case P_GLOBAL: {
        auto gp = [&](const char *id, const char *label) {
            auto *q = proc.apvts.getParameter(id);
            Entry e;
            e.label = label;
            auto *ip = dynamic_cast<juce::AudioParameterInt *>(q);
            auto *cp = dynamic_cast<juce::AudioParameterChoice *>(q);
            e.lo = ip ? ip->getRange().getStart() : 0;
            e.hi = ip ? ip->getRange().getEnd() : cp ? cp->choices.size() - 1 : 1;
            e.get = [q, ip, cp] { return ip ? ip->get() : cp ? cp->getIndex() : (q->getValue() > 0.5f ? 1 : 0); };
            e.set = [q, ip, cp](int v) {
                q->beginChangeGesture();
                if (ip)
                    q->setValueNotifyingHost(q->convertTo0to1((float)v));
                else if (cp)
                    q->setValueNotifyingHost(q->convertTo0to1((float)v));
                else
                    q->setValueNotifyingHost(v ? 1.0f : 0.0f);
                q->endChangeGesture();
            };
            e.text = [q, ip, cp](int v) { return (ip || cp) ? q->getText(q->convertTo0to1((float)v), 20) : q->getText(v ? 1.0f : 0.0f, 20); };
            es.push_back(e);
        };
        gp("voices", "Voices");
        gp("quality", "Quality (CPU)");
        gp("host_sync", "Host Sync");
        gp("transpose", "Transpose");
        gp("master_coarse", "Master Coarse");
        gp("master_fine", "Master Fine");
        {
            Entry e;
            e.label = "Window Size";
            e.lo = 5;
            e.hi = 16;
            e.get = [self] { return (int)std::lround(self->proc.uiScale.load() * 10.0f); };
            e.set = [self](int v) {
                if (auto *ed = self->findParentComponentOfClass<Fake5Editor>())
                    ed->setSize((int)std::lround(W * v / 10.0), (int)std::lround(H * v / 10.0));
            };
            e.text = [](int v) { return juce::String(v * 10) + " %"; };
            es.push_back(e);
        }
        {
            Entry e;
            e.label = "Panic (all notes off)";
            e.lo = 0;
            e.hi = 1;
            e.get = [] { return 0; };
            e.set = [self](int) { self->proc.command(Fake5Processor::C_PANIC); };
            e.text = [](int) { return juce::String("turn VALUE"); };
            es.push_back(e);
        }
        {
            Entry e;
            e.label = "Basic Program";
            e.lo = 0;
            e.hi = 1;
            e.get = [] { return 0; };
            e.set = [self](int) { self->gotoProgram(NFACTORY - 1); };
            e.text = [](int) { return juce::String("turn VALUE"); };
            es.push_back(e);
        }
        break;
    }
    default: break;
    }
    return es;
}

void Panel::openPage(Page p, int at)
{
    page = p;
    item = at;
    msg.clear();
}

void Panel::selectTurn(int d)
{
    if (page == P_HOME) {
        openPage(P_PROGRAM);
        return;
    }
    if (page == P_SEQ) {
        if (d < 0)
            proc.command(Fake5Processor::C_BACK);
        return;
    }
    if (page == P_MOD) {                         /* through the lines, and on to the next slot */
        int it = item + d;
        if (it > 3 && modSlot < NMOD - 1) {
            modSlot++;
            it = 1;
        } else if (it < 0 && modSlot > 0) {
            modSlot--;
            it = 3;
        }
        item = juce::jlimit(0, 3, it);
        return;
    }
    auto es = entries();
    item = juce::jlimit(0, juce::jmax(0, (int)es.size() - 1), item + d);
}

void Panel::valueTurn(int d)
{
    if (page == P_HOME) {
        if (shownGlobal) {
            shownGlobal->beginChangeGesture();
            if (auto *ip = dynamic_cast<juce::AudioParameterInt *>(shownGlobal))
                shownGlobal->setValueNotifyingHost(shownGlobal->convertTo0to1((float)(ip->get() + d)));
            else
                shownGlobal->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, shownGlobal->getValue() + 0.01f * (float)d));
            shownGlobal->endChangeGesture();
            shownAt = juce::Time::getMillisecondCounterHiRes() * 0.001;
        } else if (shownParam >= 0) {
            int range = PARAMS[shownParam].hi - PARAMS[shownParam].lo;
            proc.setValue(shownParam, proc.value(shownParam) + d * (range > 300 ? 4 : 1));
            shownAt = juce::Time::getMillisecondCounterHiRes() * 0.001;
        } else
            gotoProgram(((proc.getCurrentProgram() + d) % proc.getNumPrograms() + proc.getNumPrograms()) % proc.getNumPrograms());
        return;
    }
    if (page == P_SEQ)
        return;
    auto es = entries();
    if (es.empty())
        return;
    auto &e = es[(size_t)juce::jlimit(0, (int)es.size() - 1, item)];
    e.set(juce::jlimit(e.lo, e.hi, e.get() + d));
}

void Panel::entryMenu(juce::Rectangle<int> screen)
{
    auto es = entries();
    if (es.empty())
        return;
    int it = juce::jlimit(0, (int)es.size() - 1, item);
    auto e = es[(size_t)it];
    juce::PopupMenu m;
    m.addSectionHeader(e.label);
    int cur = e.get();
    if (e.hi - e.lo > 64) {                      /* a long range: in groups of 32 */
        for (int g0 = e.lo; g0 <= e.hi; g0 += 32) {
            juce::PopupMenu sub;
            int g1 = juce::jmin(e.hi, g0 + 31);
            for (int v = g0; v <= g1; v++)
                sub.addItem(v - e.lo + 1, e.text ? e.text(v) : juce::String(v), true, v == cur);
            m.addSubMenu((e.text ? e.text(g0) : juce::String(g0)) + "  ..  " + (e.text ? e.text(g1) : juce::String(g1)), sub, true,
                         nullptr, cur >= g0 && cur <= g1);
        }
    } else {
        for (int v = e.lo; v <= e.hi; v++)
            m.addItem(v - e.lo + 1, e.text ? e.text(v) : juce::String(v), true, v == cur);
    }
    juce::Component::SafePointer<Panel> self(this);
    m.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea(screen).withMinimumWidth(200), [self, e](int r) {
        if (self && r > 0)
            e.set(e.lo + r - 1);
    });
}

void Panel::programMenu(juce::Rectangle<int> screen)
{
    juce::PopupMenu m;
    for (int b = 0; b * 16 < NFACTORY; b++) {
        juce::PopupMenu sub;
        for (int i = b * 16; i < juce::jmin(NFACTORY, b * 16 + 16); i++)
            sub.addItem(i + 1, juce::String(b + 1) + juce::String(i % 16 + 1).paddedLeft('0', 2) + "   " + proc.getProgramName(i), true,
                        i == proc.getCurrentProgram());
        m.addSubMenu("FACTORY BANK " + juce::String(b + 1), sub, true, nullptr, proc.getCurrentProgram() / 16 == b && proc.getCurrentProgram() < NFACTORY);
    }
    m.addSeparator();
    m.addSectionHeader("YOUR PROGRAMS");
    for (int i = NFACTORY; i < proc.getNumPrograms(); i++)
        m.addItem(i + 1, "U" + juce::String(i - NFACTORY + 1).paddedLeft('0', 2) + "   " + proc.getProgramName(i), true,
                  i == proc.getCurrentProgram());
    if (proc.userPrograms.empty())
        m.addItem(-1, "(WRITE keeps the sound as a program of your own)", false, false);
    juce::Component::SafePointer<Panel> self(this);
    m.showMenuAsync(juce::PopupMenu::Options().withTargetScreenArea(screen).withMinimumWidth(260), [self](int r) {
        if (self && r > 0)
            self->gotoProgram(r - 1);
    });
}

void Panel::writeDialog()
{
    auto *w = new juce::AlertWindow("WRITE", "Keep the sound as a program of your own:", juce::MessageBoxIconType::NoIcon, this);
    w->setLookAndFeel(&look);
    w->addTextEditor("name", proc.programName(), "Name");
    w->addButton("Write", 1, juce::KeyPress(juce::KeyPress::returnKey));
    w->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));
    juce::Component::SafePointer<Panel> self(this);
    dialog = w;
    w->enterModalState(true, juce::ModalCallbackFunction::create([self, w](int r) {
                           if (self && r == 1) {
                               int k = self->proc.writeProgram(w->getTextEditorContents("name"));
                               if (k >= 0) {
                                   self->bank = self->bankOf(k);
                                   self->message("Written: U" + juce::String(k - NFACTORY + 1).paddedLeft('0', 2));
                               }
                           }
                       }),
                       true);
}

/* ------------------------------------------------------------------------------- the build --- */
namespace L
{
/* the places (panel units). The top: LFOs, MOD, SETTINGS / MEMORY, CLOCK, ARPEGGIATOR, STEP SEQ, EFFECT / REVERB; the
 * plates: OSCILLATORS, MIXER, FILTER, ENVELOPES; the bottom row */
constexpr float R1 = 108, R2 = 222;              /* the top's two rows */
constexpr float O1 = 352, O2 = 458;              /* the plates' two rows */
constexpr float BOT = 588;                       /* the bottom row's buttons */
constexpr float LX = 96;                         /* the left column */
constexpr float ENVX[5] = {1236, 1336, 1436, 1536, 1636};
constexpr float ENV_VEL = 1730;
constexpr float FXX[6] = {1366, 1444, 1522, 1600, 1678, 1758};
constexpr float PROG0 = 896, PROGD = 41.5f;      /* PROGRAM SELECT */
} // namespace L

void Panel::build()
{
    using juce::Rectangle;
    using namespace L;
    auto P = [this](int p) { return [this, p]() -> juce::RangedAudioParameter * { return proc.param(p); }; };
    auto touchP = [this](int p) { return [this, p](juce::RangedAudioParameter *) { touch(p); }; };
    auto place = [](juce::Component *c, float x, float y, float w, float h) { c->setBounds(Rectangle<float>(w, h).withCentre({x, y}).toNearestInt()); };
    auto knob = [&](float x, float y, int p, KnobKind k = KnobKind::pot, float size = KD) {
        auto *c = add<Knob>(*this, P(p), touchP(p), k);
        place(c, x, y, size, size);
        return c;
    };
    auto lknob = [&](float x, float y, int off) {
        auto *c = add<Knob>(*this, [this, off]() -> juce::RangedAudioParameter * { return proc.param(P_LFO1 + lfoSel * LFO_N + off); },
                            [this, off](juce::RangedAudioParameter *) { touch(P_LFO1 + lfoSel * LFO_N + off); });
        place(c, x, y, KD, KD);
        return c;
    };
    auto enc = [&](float x, float y, int p) {         /* an encoder on a parameter: a step a detent */
        auto *c = add<Knob>(*this, P(p), touchP(p), KnobKind::encoder);
        c->stepped = true;
        place(c, x, y, KD * 0.92f, KD * 0.92f);
        return c;
    };
    auto encoder = [&](float x, float y, std::function<void(int)> f) {
        auto *c = add<Knob>(*this, std::move(f));
        place(c, x, y, KD * 0.92f, KD * 0.92f);
        return c;
    };
    auto button = [&](float x, float y, std::function<void()> press, std::function<bool()> on) {
        auto *b = add<Btn>(std::move(press), std::move(on));
        place(b, x, y, BW * 2.4f, BH * 2.6f);
        return b;
    };
    auto leds = [&](std::vector<juce::Point<float>> pts, std::function<float(int)> lv, std::function<void(int)> pick = {}) {
        auto *l = add<Leds>(std::move(pts), std::move(lv), std::move(pick));
        l->fit();
        return l;
    };
    auto toggle = [this](int p) {
        return [this, p] {
            proc.setValue(p, proc.value(p) ? 0 : 1);
            touch(p);
        };
    };
    auto isOn = [this](int p) { return [this, p] { return proc.value(p) != 0; }; };
    auto glob = [this](const char *id) { return proc.apvts.getParameter(id); };

    /* the left column: VOLUME, OVERDRIVE; VINTAGE, GLIDE */
    {
        auto *k = add<Knob>(*this, [glob] { return glob("volume"); }, [this](juce::RangedAudioParameter *q) { touchGlobal(q); });
        place(k, LX, R1 - 8, KD, KD);
        knob(LX, R2, P_OVERDRIVE);
        knob(LX, O1, P_VINTAGE);
        knob(LX, O2, P_GLIDE_RATE);
    }
    /* LFO 1: MONO | LFO 2: POLY */
    {
        auto lt = [this](int off) {
            return [this, off] {
                int p = P_LFO1 + lfoSel * LFO_N + off;
                proc.setValue(p, proc.value(p) ? 0 : 1);
                touch(p);
            };
        };
        button(196, R1 - 6, lt(LFO_SYNC), [this] { return proc.value(P_LFO1 + lfoSel * LFO_N + LFO_SYNC) != 0; });
        lknob(280, R1, LFO_FREQ);
        button(360, R1 - 6,
               [this] {
                   int p = P_LFO1 + lfoSel * LFO_N + LFO_SHAPE;
                   proc.setValue(p, (proc.value(p) + 1) % NLFOSHAPE);
                   touch(p);
               },
               [] { return false; });
        std::vector<juce::Point<float>> sp;
        for (int i = 0; i < NLFOSHAPE; i++)
            sp.push_back({398.0f, R1 - 42.0f + 17.0f * (float)i});
        leds(sp, [this](int i) { return proc.value(P_LFO1 + lfoSel * LFO_N + LFO_SHAPE) == i ? 1.0f : 0.0f; },
             [this](int i) {
                 proc.setValue(P_LFO1 + lfoSel * LFO_N + LFO_SHAPE, i);
                 touch(P_LFO1 + lfoSel * LFO_N + LFO_SHAPE);
             });
        button(196, R2 - 6,
               [this] {
                   armLfoDest = !armLfoDest;
                   armDest = false;
                   if (armLfoDest)
                       openPage(P_LFODEST, 0);
                   else if (page == P_LFODEST)
                       openPage(P_HOME);
               },
               [this] { return armLfoDest || page == P_LFODEST; });
        lknob(280, R2, LFO_AMT);
        button(360, R2 - 6,
               [this] {
                   lfoSel = 1 - lfoSel;
                   if (page == P_LFODEST)
                       openPage(P_LFODEST, 0);
               },
               [] { return false; });
        leds({{398.0f, R2 - 16.0f}, {398.0f, R2 + 4.0f}}, [this](int i) {
            float a = std::fabs(proc.lfoNow[i].load()) * (proc.value(P_LFO1 + i * LFO_N + LFO_AMT) != 0 ? 1.0f : 0.0f);
            return lfoSel == i ? 0.55f + 0.45f * a : 0.0f;
        },
             [this](int i) { lfoSel = i; });
    }
    /* MOD: SRC ASSIGN, DEST ASSIGN */
    {
        button(526, R1 - 6,
               [this] {
                   armDest = false;
                   armLfoDest = false;
                   if (page == P_MOD && item == 1)
                       openPage(P_HOME);
                   else
                       openPage(P_MOD, 1);
               },
               [this] { return page == P_MOD && item == 1; });
        button(526, R2 - 6,
               [this] {
                   armLfoDest = false;
                   armDest = !armDest;
                   openPage(armDest || page != P_MOD ? P_MOD : P_HOME, 3);
               },
               [this] { return armDest || (page == P_MOD && item == 3); });
    }
    /* SETTINGS, the display, MEMORY */
    {
        button(620, 96,
               [this] {
                   openPage(page == P_GLOBAL ? P_HOME : P_GLOBAL);
                   armDest = armLfoDest = false;
               },
               [this] { return page == P_GLOBAL; });
        button(620, 168,
               [this] {
                   if (proc.recording.load() && !proc.value(P_SEQ_PLAY)) {   /* (recording: PROGRAM puts a rest) */
                       proc.command(Fake5Processor::C_REST);
                       return;
                   }
                   openPage(page == P_PROGRAM ? P_HOME : P_PROGRAM);
                   armDest = armLfoDest = false;
               },
               [this] { return page == P_PROGRAM; });
        auto *o = add<Oled>(*this);
        o->setBounds(Rectangle<float>(666, 60, 200, 118).toNearestInt());
        oledView = o;
        encoder(712, 238, [this](int d) { selectTurn(d); });
        encoder(822, 238, [this](int d) { valueTurn(d); });
        button(912, 96, [this] { proc.setCompare(!proc.compare); }, [this] { return proc.compare; });
        button(912, 168,
               [this] {
                   if (proc.recording.load() && !proc.value(P_SEQ_PLAY)) {   /* (recording: WRITE ties) */
                       proc.command(Fake5Processor::C_TIE);
                       return;
                   }
                   writeDialog();
               },
               [] { return false; });
    }
    /* CLOCK */
    enc(978, R1 - 4, P_BPM);
    enc(1056, R1 - 4, P_DIVIDE);
    button(1017, R2 - 6,
           [this] {
               double now = juce::Time::getMillisecondCounterHiRes() * 0.001;
               if (!taps.empty() && now - taps.back() > 2.0)
                   taps.clear();
               taps.push_back(now);
               if (taps.size() > 4)
                   taps.erase(taps.begin());
               if (taps.size() >= 2) {
                   double avg = (taps.back() - taps.front()) / (double)(taps.size() - 1);
                   proc.setValue(P_BPM, (int)std::lround(60.0 / avg));
                   touch(P_BPM);
               }
           },
           [this] { return proc.beatLed.load(); });
    /* ARPEGGIATOR */
    {
        button(1126, R1 - 6, toggle(P_ARP_ON), isOn(P_ARP_ON));
        enc(1222, R1 - 4, P_ARP_OCT);
        leds({{1182.0f, R1 - 30.0f}, {1182.0f, R1 - 13.0f}, {1182.0f, R1 + 4.0f}}, [this](int i) { return proc.value(P_ARP_OCT) == 3 - i ? 1.0f : 0.0f; },
             [this](int i) { proc.setValue(P_ARP_OCT, 3 - i); });
        enc(1140, R2 - 4, P_ARP_MODE);
        enc(1222, R2 - 4, P_ARP_REPEAT);
        leds({{1182.0f, R2 - 30.0f}, {1182.0f, R2 - 13.0f}, {1182.0f, R2 + 4.0f}}, [this](int i) { return proc.value(P_ARP_REPEAT) == 3 - i ? 1.0f : 0.0f; },
             [this](int i) { proc.setValue(P_ARP_REPEAT, proc.value(P_ARP_REPEAT) == 3 - i ? 0 : 3 - i); });
    }
    /* STEP SEQ */
    button(1295, R1 - 6,
           [this] {
               bool rec = proc.recording.load();
               proc.command(Fake5Processor::C_RECORD, rec ? 0 : 1);
               openPage(rec && page == P_SEQ ? P_HOME : P_SEQ);
           },
           [this] { return proc.recording.load(); });
    button(1295, R2 - 6,
           [this] {
               proc.setValue(P_SEQ_PLAY, proc.value(P_SEQ_PLAY) ? 0 : 1);
               openPage(P_SEQ);
           },
           isOn(P_SEQ_PLAY));
    /* EFFECT, REVERB */
    {
        button(FXX[0], R1 - 6, toggle(P_FX_ON), isOn(P_FX_ON));
        enc(FXX[1], R1 - 4, P_FX_TYPE);
        knob(FXX[2], R1, P_FX_TIME);
        knob(FXX[3], R1, P_FX_MIX);
        knob(FXX[4], R1, P_FX_MISC);
        button(FXX[5], R1 - 26, toggle(P_FX_SYNC), isOn(P_FX_SYNC));
        button(FXX[0], R2 - 6, toggle(P_REV_ON), isOn(P_REV_ON));
        knob(FXX[1], R2, P_REV_MIX);
        knob(FXX[2], R2, P_REV_DAMP);
        knob(FXX[3], R2, P_REV_PREDELAY);
        knob(FXX[4], R2, P_REV_DECAY);
        knob(FXX[5], R2 - 8, P_REV_TONE);
    }
    /* OSCILLATORS */
    {
        for (int o = 0; o < 2; o++) {
            float y = o ? O2 : O1;
            int oct = o ? P_O2_OCT : P_O1_OCT;
            knob(262, y, oct, KnobKind::plain);
            std::vector<juce::Point<float>> pts;
            for (int i = 0; i < 5; i++)                   /* the octave's LEDs in an arc over its upper left */
                pts.push_back(juce::Point<float>(262.0f, y).getPointOnCircumference(40.0f, -1.45f + 0.36f * (float)i));
            leds(pts, [this, oct](int i) { return proc.value(oct) == i - 2 ? 1.0f : 0.0f; },
                 [this, oct](int i) {
                     proc.setValue(oct, i - 2);
                     touch(oct);
                 });
            knob(382, y, o ? P_O2_PITCH : P_O1_PITCH);
            knob(552, y, o ? P_O2_SHAPE : P_O1_SHAPE);
        }
        button(470, (O1 + O2) * 0.5f - 6.0f, toggle(P_SYNC), isOn(P_SYNC));
    }
    /* MIXER */
    knob(672, O1, P_O1_LEVEL);
    button(722, O1 + 30, toggle(P_FM), isOn(P_FM));
    knob(774, O1, P_SUB);
    knob(672, O2, P_O2_LEVEL);
    knob(774, O2, P_NOISE);
    /* FILTER */
    knob(874, O1, P_RESO);
    knob(874, O2, P_DRIVE);
    knob(990, (O1 + O2) * 0.5f - 4.0f, P_CUTOFF, KnobKind::big, 150.0f);
    /* ENVELOPES */
    {
        leds({{1092.0f, 338.0f}, {1092.0f, 355.0f}}, [this](int i) {
            int r = proc.value(P_ENV_ROUTING);
            return (i == 0) == (r == ER_FILTER_AMP) ? 1.0f : 0.0f;
        });
        leds({{1092.0f, 410.0f}, {1092.0f, 427.0f}, {1092.0f, 444.0f}}, [this](int i) { return proc.value(P_ENV_ROUTING) == i ? 1.0f : 0.0f; },
             [this](int i) {
                 proc.setValue(P_ENV_ROUTING, i);
                 touch(P_ENV_ROUTING);
             });
        button(1100, 494,
               [this] {
                   proc.setValue(P_ENV_ROUTING, (proc.value(P_ENV_ROUTING) + 1) % NENVROUTING);
                   touch(P_ENV_ROUTING);
               },
               [] { return false; });
        const int e1[5] = {P_E1_ATT, P_E1_DEC, P_E1_SUS, P_E1_REL, P_E1_AMT}, e2[5] = {P_E2_ATT, P_E2_DEC, P_E2_SUS, P_E2_REL, P_E2_AMT};
        for (int i = 0; i < 5; i++) {
            knob(ENVX[i], O1, e1[i]);
            knob(ENVX[i], O2, e2[i]);
        }
        button(ENV_VEL, O1 - 6, toggle(P_E1_VEL), isOn(P_E1_VEL));
        button(ENV_VEL, O2 - 6, toggle(P_E2_VEL), isOn(P_E2_VEL));
    }
    /* the bottom row: GLIDE, UNISON, HOLD, FACTORY; TRANSPOSE, LOW SPLIT; BANK and the display, PROGRAM SELECT,
     * PRESET; VOICES, HOST SYNC */
    {
        button(LX, BOT, toggle(P_GLIDE), isOn(P_GLIDE));
        button(184, BOT,
               [this] {
                   if (proc.keysHeld.load() > 0) {   /* keys held: CHORD MEMORY (one key: cleared) */
                       proc.command(Fake5Processor::C_CHORD);
                       if (proc.keysHeld.load() > 1) {
                           proc.setValue(P_UNISON, 1);
                           message("Chord stored");
                       } else
                           message("Chord memory cleared");
                       return;
                   }
                   proc.setValue(P_UNISON, proc.value(P_UNISON) ? 0 : 1);
                   touch(P_UNI_VOICES);
               },
               isOn(P_UNISON));
        button(268, BOT, [this] { proc.command(Fake5Processor::C_HOLD, proc.holdNow.load() ? 0 : 1); }, [this] { return proc.holdNow.load(); });
        button(352, BOT,
               [this] {
                   bool fac = proc.getCurrentProgram() < NFACTORY;
                   if (fac) {
                       if (proc.userPrograms.empty())
                           message("No programs of your own yet:\nWRITE keeps one");
                       else
                           gotoProgram(NFACTORY);
                   } else
                       gotoProgram(0);
               },
               [this] { return proc.getCurrentProgram() < NFACTORY; });
        auto tp = [this, glob](int d) {
            return [this, glob, d] {
                auto *q = glob("transpose");
                int v = juce::jlimit(-2, 2, (int)std::lround(q->convertFrom0to1(q->getValue())) + d);
                q->beginChangeGesture();
                q->setValueNotifyingHost(q->convertTo0to1((float)v));
                q->endChangeGesture();
                touchGlobal(q);
            };
        };
        button(436, BOT + 4, tp(-1), [] { return false; });
        button(540, BOT + 4, tp(1), [] { return false; });
        std::vector<juce::Point<float>> tpp;
        for (int i = 0; i < 5; i++)
            tpp.push_back({458.0f + 15.0f * (float)i, BOT - 18.0f});
        leds(tpp, [this, glob](int i) {
            auto *q = glob("transpose");
            return (int)std::lround(q->convertFrom0to1(q->getValue())) == i - 2 ? 1.0f : 0.0f;
        });
        auto split = [this](int p, int other) {
            return [this, p, other] {
                bool on = proc.value(p) == 0;
                proc.setValue(p, on ? 1 : 0);
                if (on)
                    proc.setValue(other, 0);
                touch(p);
            };
        };
        button(612, BOT + 4, split(P_SPLIT_2, P_SPLIT_1), isOn(P_SPLIT_2));
        button(676, BOT + 4, split(P_SPLIT_1, P_SPLIT_2), isOn(P_SPLIT_1));
        button(752, BOT - 6,
               [this] {
                   bool fac = proc.getCurrentProgram() < NFACTORY;
                   int n = fac ? NFACTORY : (int)proc.userPrograms.size(), banks = juce::jmax(1, (n + 15) / 16);
                   bank = (bank + 1) % banks;
                   message(juce::String(fac ? "FACTORY" : "YOUR") + " BANK " + juce::String(bank + 1) + ":\npick a program 1-16");
               },
               [] { return false; });
        auto *seg = add<Seg>(*this);
        seg->setBounds(Rectangle<float>(806, BOT - 26, 66, 38).toNearestInt());
        for (int i = 0; i < 16; i++) {
            button(PROG0 + PROGD * (float)i, BOT - 6,
                   [this, i] {
                       bool fac = proc.getCurrentProgram() < NFACTORY;
                       int base = fac ? 0 : NFACTORY, n = fac ? NFACTORY : (int)proc.userPrograms.size();
                       int k = bank * 16 + i;
                       if (k < n)
                           gotoProgram(base + k);
                   },
                   [this, i] {
                       int pr = proc.getCurrentProgram();
                       return bankOf(pr) == bank && (pr < NFACTORY ? pr : pr - NFACTORY) % 16 == i;
                   });
        }
        button(1592, BOT - 6,
               [this] {
                   gotoProgram(proc.getCurrentProgram());   /* (PRESET: the stored program again, the edit let go) */
                   message("Program reloaded");
               },
               [] { return true; });
        button(1664, BOT - 6,
               [this, glob] {
                   auto *q = glob("voices");
                   q->beginChangeGesture();
                   q->setValueNotifyingHost(q->getValue() > 0.5f ? 0.0f : 1.0f);
                   q->endChangeGesture();
                   touchGlobal(q);
               },
               [] { return false; });
        leds({{1692.0f, BOT - 16.0f}, {1692.0f, BOT + 2.0f}}, [this, glob](int i) { return (glob("voices")->getValue() > 0.5f) == (i == 1) ? 1.0f : 0.0f; },
             [this, glob](int i) {
                 auto *q = glob("voices");
                 q->beginChangeGesture();
                 q->setValueNotifyingHost(i ? 1.0f : 0.0f);
                 q->endChangeGesture();
                 touchGlobal(q);
             });
        button(1762, BOT - 6,
               [this, glob] {
                   auto *q = glob("host_sync");
                   q->beginChangeGesture();
                   q->setValueNotifyingHost(q->getValue() > 0.5f ? 0.0f : 1.0f);
                   q->endChangeGesture();
                   touchGlobal(q);
               },
               [glob] { return glob("host_sync")->getValue() > 0.5f; });
    }
}

/* ------------------------------------------------------------------------------ painting --- */
void Panel::paintBackground(juce::Graphics &g, float sc)
{
    using namespace juce;
    using namespace L;
    const float cap = 36.0f, lipY = 640.0f;
    auto face = Rectangle<float>(cap, 0.0f, (float)W - 2.0f * cap, lipY);
    drawPanelFace(g, face, sc);
    drawEndCap(g, Rectangle<float>(0.0f, 0.0f, cap, (float)H), true);
    drawEndCap(g, Rectangle<float>((float)W - cap, 0.0f, cap, (float)H), false);
    drawLip(g, Rectangle<float>(cap, lipY, (float)W - 2.0f * cap, (float)H - lipY));
    for (float x : {cap, (float)W - cap}) {           /* the caps' shadow on the panel */
        bool left = x < 100.0f;
        g.setGradientFill(ColourGradient(Colours::black.withAlpha(0.5f), x, 0.0f, Colours::transparentBlack, x + (left ? 10.0f : -10.0f), 0.0f, false));
        g.fillRect(left ? Rectangle<float>(x, 0.0f, 10.0f, lipY) : Rectangle<float>(x - 10.0f, 0.0f, 10.0f, lipY));
    }
    /* the lip: the maker's tag, FAKE 5 */
    drawTag(g, Rectangle<float>(64.0f, lipY + 13.0f, 230.0f, 26.0f), Colour(0xffe8e8e4));
    drawLogo(g, Rectangle<float>(1560.0f, lipY + 9.0f, 220.0f, 34.0f), Colour(0xfff0f0ec));

    auto C = [](float x, float y, float w, float h) { return Rectangle<float>(w, h).withCentre({x, y}); };
    auto under = [&](float x, float y, const String &t, float dy = 33.0f, float size = LS) { drawLegend(g, t, C(x, y + dy + 6.0f, 120.0f, 14.0f), size); };
    auto underB = [&](float x, float y, const String &t, float size = LS - 0.5f) { drawLegend(g, t, C(x, y + 24.0f, 110.0f, 14.0f), size); };
    auto small = [&](float x, float y, const String &t, float size = 9.5f, Justification j = Justification::centred) {
        if (j == Justification::centredLeft)          /* (from x on) */
            drawLegend(g, t, Rectangle<float>(x, y - 6.0f, 90.0f, 12.0f), size, j);
        else
            drawLegend(g, t, C(x, y, 90.0f, 12.0f), size, j);
    };
    auto title = [&](float x, float y, const String &t, float size = 13.5f) {
        g.setColour(col::legend);
        g.setFont(font(Face::title, size).withExtraKerningFactor(0.06f));
        g.drawText(t, C(x, y, 300.0f, 18.0f), Justification::centred, false);
    };
    auto vline = [&](float x, float y0, float y1) {
        g.setColour(col::line.withAlpha(0.85f));
        g.fillRect(Rectangle<float>(x - 0.7f, y0, 1.4f, y1 - y0));
    };

    /* the top band's dividers */
    for (float x : {150.0f, 470.0f, 578.0f, 948.0f, 1098.0f, 1262.0f, 1328.0f})
        vline(x, 22.0f, 278.0f);
    /* the left column */
    under(LX, R1 - 8, "VOLUME");
    under(LX, R2, "OVERDRIVE");
    under(LX, O1, "VINTAGE");
    under(LX, O2, "GLIDE");
    underB(LX, BOT, "GLIDE");
    /* LFOs */
    title(310, 36, "LFO 1: MONO  |  LFO 2: POLY");
    underB(196, R1 - 6, "SYNC");
    under(280, R1, "FREQUENCY");
    underB(360, R1 - 6, "SHAPE");
    g.setColour(col::legend);
    for (int i = 0; i < NLFOSHAPE; i++)
        drawWaveIcon(g, Rectangle<float>(410.0f, R1 - 47.0f + 17.0f * (float)i, 22.0f, 9.0f), i, 1.2f);
    small(436, R1 + 26.0f, "S&H", 8.0f, Justification::centredLeft);
    underB(196, R2 - 6, "DEST");
    under(280, R2, "AMOUNT");
    underB(360, R2 - 6, "LFO");
    small(414, R2 - 16.0f, "1", 10.0f);
    small(414, R2 + 4.0f, "2", 10.0f);
    /* MOD */
    title(524, 36, "MOD");
    underB(526, R1 - 6, "SRC ASSIGN");
    underB(526, R2 - 6, "DEST ASSIGN");
    /* SETTINGS, MEMORY: the display's frame and its corners' names */
    title(630, 36, "SETTINGS");
    title(904, 36, "MEMORY");
    underB(620, 96, "global");
    underB(620, 168, "program");
    underB(912, 96, "compare");
    underB(912, 168, "write");
    {
        auto o = Rectangle<float>(666, 60, 200, 118).expanded(5.0f);
        g.setColour(Colour(0xff111214));
        g.fillRoundedRectangle(o, 4.0f);
        g.setColour(Colours::black);
        g.drawRoundedRectangle(o, 4.0f, 1.2f);
        g.setColour(Colours::white.withAlpha(0.08f));
        g.drawLine(o.getX() + 4.0f, o.getBottom() + 0.6f, o.getRight() - 4.0f, o.getBottom() + 0.6f, 1.0f);
    }
    under(712, 238, "select", 31.0f);
    under(822, 238, "value", 31.0f);
    /* CLOCK */
    title(1023, 36, "CLOCK");
    under(978, R1 - 4, "BPM", 31.0f);
    under(1056, R1 - 4, "CLK DIVIDE", 31.0f);
    underB(1017, R2 - 6, "TAP TEMPO");
    /* ARPEGGIATOR */
    title(1180, 36, "ARPEGGIATOR");
    underB(1126, R1 - 6, "ON");
    under(1222, R1 - 4, "OCTAVE", 31.0f);
    under(1140, R2 - 4, "MODE", 31.0f);
    under(1222, R2 - 4, "REPEAT", 31.0f);
    for (int i = 0; i < 3; i++) {
        small(1168, R1 - 30.0f + 17.0f * (float)i, String(3 - i), 9.0f);
        small(1168, R2 - 30.0f + 17.0f * (float)i, String(3 - i), 9.0f);
    }
    /* STEP SEQ */
    title(1295, 36, "STEP SEQ", 12.0f);
    underB(1295, R1 - 6, "RECORD");
    underB(1295, R2 - 6, "PLAY");
    /* EFFECT, REVERB: the line between with REVERB in it */
    title(1560, 30, "EFFECT");
    underB(FXX[0], R1 - 6, "ON");
    under(FXX[1], R1 - 4, "TYPE", 31.0f);
    under(FXX[2], R1, "TIME");
    under(FXX[3], R1, "DEPTH/MIX");
    under(FXX[4], R1, "FBACK/MISC");
    underB(FXX[5], R1 - 26, "CLK SYNC");
    {
        float y = 160.0f;
        auto f = font(Face::title, 13.5f).withExtraKerningFactor(0.06f);
        float tw = textWidth(f, "REVERB"), cx = 1556.0f;
        g.setColour(col::line.withAlpha(0.85f));
        g.fillRect(Rectangle<float>(1340.0f, y - 0.7f, cx - tw * 0.5f - 10.0f - 1340.0f, 1.4f));
        g.fillRect(Rectangle<float>(cx + tw * 0.5f + 10.0f, y - 0.7f, 1730.0f - (cx + tw * 0.5f + 10.0f), 1.4f));
        title(cx, y, "REVERB");
    }
    underB(FXX[0], R2 - 6, "ON");
    under(FXX[1], R2, "MIX");
    small(FXX[1] - 30.0f, R2 + 26.0f, "dry", 9.0f);
    small(FXX[1] + 31.0f, R2 + 26.0f, "wet", 9.0f);
    under(FXX[2], R2, "DAMPING");
    under(FXX[3], R2, "PREDELAY");
    under(FXX[4], R2, "DECAY");
    under(FXX[5], R2 - 8, "TONE");
    small(FXX[5] - 31.0f, R2 + 18.0f, "lpf", 9.0f);
    small(FXX[5] + 31.0f, R2 + 18.0f, "hpf", 9.0f);

    /* the plates */
    const float py0 = 296.0f, py1 = 546.0f;
    drawPlate(g, Rectangle<float>(160.0f, py0, 452.0f, py1 - py0), 16.0f, sc);
    drawPlate(g, Rectangle<float>(620.0f, py0, 200.0f, py1 - py0), 12.0f, sc);
    drawPlate(g, Rectangle<float>(828.0f, py0, 236.0f, py1 - py0), 12.0f, sc);
    drawPlate(g, Rectangle<float>(1072.0f, py0, 722.0f, py1 - py0), 16.0f, sc);
    drawWide(g, "OSCILLATORS", Rectangle<float>(300.0f, 518.0f, 300.0f, 24.0f), 21.0f, 1.18f, col::legend, Justification::centred);
    drawWide(g, "MIXER", Rectangle<float>(620.0f, 518.0f, 200.0f, 24.0f), 21.0f);
    drawWide(g, "FILTER", Rectangle<float>(940.0f, 518.0f, 120.0f, 24.0f), 21.0f);
    drawWide(g, "ENVELOPES", Rectangle<float>(1340.0f, 518.0f, 300.0f, 24.0f), 21.0f);
    /* OSCILLATORS */
    {
        for (int o = 0; o < 2; o++) {
            float y = o ? O2 : O1;
            {   /* "OSC 1" / "OSC 2", turned on its side */
                Graphics::ScopedSaveState ss(g);
                g.addTransform(AffineTransform::rotation(-MathConstants<float>::halfPi, 182.0f, y));
                drawLegend(g, o ? "OSC 2" : "OSC 1", C(182.0f, y, 70.0f, 16.0f), 12.5f);
            }
            under(262, y, "OCTAVE");
            static const char *const OCT[5] = {"-2", "-1", "0", "+1", "+2"};
            for (int i = 0; i < 5; i++) {
                auto p = Point<float>(262.0f, y).getPointOnCircumference(55.0f, -1.45f + 0.36f * (float)i);
                small(p.x, p.y, OCT[i], 8.5f);
            }
            under(382, y, "PITCH");
            small(382, y - 41.0f, "0", 9.5f);
            under(552, y, "SHAPE");
            g.setColour(col::legend);
            drawWaveIcon(g, Rectangle<float>(510.0f, y + 14.0f, 13.0f, 8.0f), 5, 1.1f);
            drawWaveIcon(g, Rectangle<float>(522.0f, y - 40.0f, 13.0f, 8.0f), 1, 1.1f);
            drawWaveIcon(g, Rectangle<float>(572.0f, y - 40.0f, 13.0f, 8.0f), 3, 1.1f);
            drawWaveIcon(g, Rectangle<float>(584.0f, y + 14.0f, 13.0f, 8.0f), 6, 1.1f);
        }
        drawLegend(g, "SYNC 2>1", C(470.0f, (O1 + O2) * 0.5f + 18.0f, 80.0f, 14.0f), 10.0f);
    }
    /* MIXER */
    {
        under(672, O1, "OSC 1");
        under(774, O1, "OSC 1 SUB");
        under(672, O2, "OSC 2");
        under(774, O2, "NOISE");
        small(745, O2 + 26.0f, "pink", 9.0f);
        small(805, O2 + 26.0f, "white", 9.0f);
        small(774, O2 - 40.0f, "0", 9.5f);
        /* FM 2>1: from OSC 2 up to the button */
        g.setColour(col::legend);
        Path p;
        p.startNewSubPath(700.0f, O2 - 4.0f);
        p.lineTo(722.0f, O2 - 4.0f);
        p.lineTo(722.0f, O1 + 62.0f);
        g.strokePath(p, PathStrokeType(1.3f));
        g.drawArrow(Line<float>(722.0f, O1 + 70.0f, 722.0f, O1 + 46.0f), 1.3f, 6.0f, 6.0f);
        small(728, O1 + 60.0f, "FM 2>1", 8.5f, Justification::centredLeft);
    }
    /* FILTER */
    under(874, O1, "RESONANCE");
    under(874, O2, "DRIVE");
    under(990, (O1 + O2) * 0.5f - 4.0f, "CUTOFF", 70.0f, 11.5f);
    /* ENVELOPES */
    {
        g.setColour(col::line);
        auto bracket = [&](float y0, float y1) {
            g.fillRect(Rectangle<float>(1080.0f, y0, 1.3f, y1 - y0));
            g.fillRect(Rectangle<float>(1080.0f, y0, 8.0f, 1.3f));
        };
        bracket(322.0f, 362.0f);
        bracket(394.0f, 452.0f);
        drawLegend(g, "ENV 1", Rectangle<float>(1090.0f, 314.0f, 60.0f, 14.0f), 11.0f, Justification::centredLeft);
        small(1103, 338, "Filter", 9.0f, Justification::centredLeft);
        small(1103, 355, "Aux", 9.0f, Justification::centredLeft);
        drawLegend(g, "ENV 2", Rectangle<float>(1090.0f, 386.0f, 60.0f, 14.0f), 11.0f, Justification::centredLeft);
        small(1103, 410, "Amp", 9.0f, Justification::centredLeft);
        small(1103, 427, "Filter+Amp", 9.0f, Justification::centredLeft);
        small(1103, 444, "Filter+Gate", 9.0f, Justification::centredLeft);
        drawLegend(g, "ENV ROUTING", Rectangle<float>(1126.0f, 487.0f, 100.0f, 14.0f), 10.0f, Justification::centredLeft);
        const char *nm[5] = {"ATTACK", "DECAY", "SUSTAIN", "RELEASE", "AMOUNT"};
        for (int i = 0; i < 5; i++) {
            under(ENVX[i], O1, nm[i]);
            under(ENVX[i], O2, nm[i]);
        }
        small(ENVX[4], O1 - 41.0f, "0", 9.5f);
        small(ENVX[4], O2 - 41.0f, "0", 9.5f);
        underB(ENV_VEL, O1 - 6, "VELOCITY");
        underB(ENV_VEL, O2 - 6, "VELOCITY");
    }
    /* the bottom row */
    {
        underB(184, BOT, "UNISON");
        underB(268, BOT, "HOLD");
        underB(352, BOT, "FACTORY");
        static const char *const TP[5] = {"-2", "-1", "0", "+1", "+2"};
        for (int i = 0; i < 5; i++)
            small(458.0f + 15.0f * (float)i, BOT - 31.0f, TP[i], 8.0f);
        underB(436, BOT + 4, "down");
        underB(540, BOT + 4, "up");
        drawLegend(g, "TRANSPOSE", C(488.0f, BOT + 44.0f, 100.0f, 13.0f), 10.0f);
        underB(612, BOT + 4, "-2 oct");
        underB(676, BOT + 4, "-1 oct");
        drawLegend(g, "LOW SPLIT", C(644.0f, BOT + 44.0f, 100.0f, 13.0f), 10.0f);
        g.setColour(col::line);
        g.drawLine(600.0f, BOT + 37.0f, 690.0f, BOT + 37.0f, 1.0f);
        g.drawLine(424.0f, BOT + 37.0f, 552.0f, BOT + 37.0f, 1.0f);
        /* BANK, in its outline, and the line along the numbers to PROGRAM SELECT */
        auto bb = C(752.0f, BOT + 22.0f, 54.0f, 17.0f);
        g.drawRoundedRectangle(bb, 8.5f, 1.2f);
        drawLegend(g, "BANK", bb, 10.0f);
        g.drawLine(bb.getRight(), bb.getCentreY(), PROG0 - 14.0f, bb.getCentreY(), 1.2f);
        for (int i = 0; i < 16; i++) {
            float x = PROG0 + PROGD * (float)i;
            auto cc = C(x, BOT + 22.0f, 17.0f, 17.0f);
            g.setColour(col::line);
            if (i < 8)
                g.drawEllipse(cc, 1.2f);
            drawLegend(g, String(i + 1), cc, 9.5f);
            if (i < 15) {
                g.setColour(col::line);
                if (i < 7)
                    g.drawLine(cc.getRight(), cc.getCentreY(), cc.getRight() + PROGD - 17.0f, cc.getCentreY(), 1.2f);
            }
        }
        float x0 = PROG0 + PROGD * 8.0f - 18.0f, x1 = PROG0 + PROGD * 15.0f + 12.0f;
        g.setColour(col::line);
        g.drawLine(PROG0 - 10.0f, BOT + 42.0f, x0 - 6.0f, BOT + 42.0f, 1.2f);
        g.drawLine(x0 + 150.0f, BOT + 42.0f, x1, BOT + 42.0f, 1.2f);
        drawLegend(g, "PROGRAM SELECT", Rectangle<float>(x0, BOT + 35.0f, 150.0f, 14.0f), 10.0f);
        underB(1592, BOT - 6, "PRESET");
        underB(1664, BOT - 6, "VOICES");
        small(1712, BOT - 16.0f, "5", 9.5f, Justification::centredLeft);
        small(1712, BOT + 2.0f, "10", 9.5f, Justification::centredLeft);
        underB(1762, BOT - 6, "HOST SYNC");
    }
    /* the screws, and a few years of use over it all (worn round the knobs played most) */
    drawWear(g, face, 1.0f / sc, {Point<float>(990.0f, 402.0f), Point<float>(874.0f, O1), Point<float>(LX, R1 - 8.0f), Point<float>(822.0f, 238.0f),
                                  Point<float>(FXX[1], R2), Point<float>(1236.0f, O2)});
}

void Panel::paint(juce::Graphics &g)
{
    float sc = g.getInternalContext().getPhysicalPixelScaleFactor();
    if (sc != backgroundScale || background.isNull()) {
        backgroundScale = sc;
        background = juce::Image(juce::Image::RGB, juce::jmax(1, (int)std::ceil(W * sc)), juce::jmax(1, (int)std::ceil(H * sc)), true);
        juce::Graphics bg(background);
        bg.addTransform(juce::AffineTransform::scale(sc));
        paintBackground(bg, sc);
    }
    g.drawImage(background, getLocalBounds().toFloat(), juce::RectanglePlacement::stretchToFit);
}

void Panel::timerCallback()
{
    for (auto *l : live)
        l->poll();
    /* (a moment's message gone: home again) */
    if (msg.isNotEmpty() && juce::Time::getMillisecondCounterHiRes() * 0.001 - shownAt > SHOW_FOR + 0.8)
        msg.clear();
}

/* ============================================================================== the editor === */
Fake5Editor::Fake5Editor(Fake5Processor &p) : AudioProcessorEditor(&p), proc(p), panel(p)
{
    addAndMakeVisible(panel);
    panel.setBounds(0, 0, Fake5Panel::W, Fake5Panel::H);
    const double W = Fake5Panel::W, H = Fake5Panel::H;
    /* the window: never larger than the screen it opens on (a MacBook Air too); the first time as large as the screen
     * comfortably allows, after that as it was left */
    auto area = juce::Rectangle<int>(0, 0, 1920, 1080);
    if (auto *d = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
        area = d->userArea;
    double fit = juce::jmin((area.getWidth() - 40) / W, (area.getHeight() - 90) / H);
    fit = juce::jlimit(0.4, 1.6, fit);
    setResizable(true, true);
    setResizeLimits((int)(W * 0.4), (int)(H * 0.4), (int)(W * juce::jmax(0.5, fit)), (int)(H * juce::jmax(0.5, fit)));
    getConstrainer()->setFixedAspectRatio(W / H);
    double s = proc.uiScaleSet ? (double)proc.uiScale.load() : juce::jmin(fit, 1.0);
    s = juce::jlimit(0.4, fit, s);
    setSize((int)std::lround(W * s), (int)std::lround(H * s));
}

void Fake5Editor::resized()
{
    float s = (float)getWidth() / (float)Fake5Panel::W;
    panel.setTransform(juce::AffineTransform::scale(s));
    proc.uiScale = s;
    proc.uiScaleSet = true;
}
