/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (FAKE5) */
/* The MPC skin's pictures, drawn with the VST3's own art (src/ui/Look.cpp: the charcoal powder coat, the blue-grey
 * plates, the knobs with their grey caps, the glowing buttons, the LEDs, the legends, the maker's tag), on the MPC's
 * pages (its function keys; some have a second half: the key pressed again):
 *
 *   OSC      OSCILLATORS, MIXER  /  GLIDE, KEYS, UNISON
 *   FILTER   FILTER, ENV 1       /  ENV 2, ROUTING
 *   LFO      LFO 1, LFO 2
 *   MOD      slots 1-8           /  slots 9-16
 *   FX       EFFECT, REVERB, OVERDRIVE
 *   PLAY     ARPEGGIATOR, CLOCK, SEQUENCER, VOICES
 *
 * Each screen is drawn at the MPC's 1280 x 628 and cut up: its background (the face, the plates, the legends), and
 * every control at every value it can take (a knob's film strip, a button's look for each value with the value a press
 * gives, an LED list's rows lit and unlit). manifest.json says what each is, for make_skin.py.
 *
 *   fake5_skin_render <out folder> */
#include "Params.h"
#include "ui/Look.h"
#include <juce_gui_basics/juce_gui_basics.h>

using namespace juce;
using namespace f5ui;
using namespace f5;

namespace {
constexpr int TW = 1280, TH = 628;
constexpr float KS = 76.0f;              /* a knob */
constexpr float BW = 44.0f, BH = 30.0f;  /* a button */
constexpr float LR = 21.0f;              /* an LED row */

struct Ctl {
    enum Kind { knob, button, leds } kind;
    String id;                           /* a parameter's VST id ("cutoff"); or "vol", "voices", "quality", "hold" */
    float x, y;                          /* the centre (knob, button) or the top left (LEDs) */
    String legend;
    StringArray names;                   /* LEDs: each row's name */
    float w = 0.0f;                      /* LEDs: the rows' width */
    bool cycle = false;                  /* a button that steps through the values (else a switch) */
    float rh = LR;
    bool big = false;                    /* CUTOFF's knob */
};
struct Sec {
    Rectangle<float> r;
    String title;
    bool plate;                          /* a blue-grey plate (the synthesis section), else a white frame */
};
struct Screen {
    String name;                         /* the page's name and its half ("OSC", "OSC 2") */
    std::vector<Sec> secs;
    std::vector<Ctl> ctls;
    std::vector<std::pair<Point<float>, String>> notes;
};

int paramIndex(const String &id)
{
    for (int i = 0; i < NPARAM; i++)
        if (id == PARAMS[i].id)
            return i;
    return -1;
}
struct Range {
    int lo, hi;
    StringArray names;
};
Range rangeOf(const String &id)
{
    int p = paramIndex(id);
    if (p >= 0) {
        const ParamDef &d = PARAMS[p];
        Range r{d.lo, d.hi, {}};
        if (d.kind == K_CHOICE || d.kind == K_ONOFF)
            for (int k = d.lo; k <= d.hi; k++)
                r.names.add(d.choice[k]);
        return r;
    }
    if (id == "voices")
        return {0, 1, {"5 Voices", "10 Voices"}};
    if (id == "quality")
        return {0, 1, {"ECO", "HIGH"}};
    if (id == "hold")
        return {0, 1, {"Off", "On"}};
    if (id == "vol")
        return {0, 127, {}};
    std::fprintf(stderr, "unknown %s\n", id.toRawUTF8());
    std::exit(1);
}
bool bipolarOf(const String &id)
{
    int p = paramIndex(id);
    return p >= 0 && PARAMS[p].lo < 0;
}

/* ------------------------------------------------------------------------------- the pages --- */
struct Builder {
    Screen s;
    void sec(float x0, float y0, float x1, float y1, const char *t, bool plate = false) { s.secs.push_back({{x0, y0, x1 - x0, y1 - y0}, t, plate}); }
    void K(float x, float y, const char *id, const char *leg, bool big = false)
    {
        Ctl c{Ctl::knob, id, x, y, leg, {}, 0.0f, false};
        c.big = big;
        s.ctls.push_back(c);
    }
    void B(float x, float y, const char *id, const char *leg, bool cycle = false) { s.ctls.push_back({Ctl::button, id, x, y, leg, {}, 0.0f, cycle}); }
    void L(float x, float y, const char *id, float w, StringArray names = {}, float rh = LR)
    {
        auto r = rangeOf(id);
        s.ctls.push_back({Ctl::leds, id, x, y, "", names.isEmpty() ? r.names : names, w, false, rh});
    }
    void note(float x, float y, const char *t) { s.notes.push_back({{x, y}, t}); }
};

std::vector<Screen> screens()
{
    std::vector<Screen> ps;
    const float T1 = 84, T2 = 208, B1 = 372, B2 = 496;
    {
        Builder b;
        b.s.name = "OSC";
        b.sec(14, 18, 1266, 286, "OSCILLATORS", true);
        b.K(90, T1, "o1_oct", "OSC 1 OCTAVE");
        b.K(240, T1, "o1_pitch", "PITCH");
        b.K(390, T1, "o1_shape", "SHAPE");
        b.K(90, T2, "o2_oct", "OSC 2 OCTAVE");
        b.K(240, T2, "o2_pitch", "PITCH");
        b.K(390, T2, "o2_shape", "SHAPE");
        b.B(540, (T1 + T2) * 0.5f - 8, "sync", "SYNC 2>1");
        b.B(680, T1 - 8, "o1_key", "OSC 1 KEY");
        b.B(680, T2 - 8, "o2_key", "OSC 2 KEY");
        b.B(780, (T1 + T2) * 0.5f - 8, "o2_bypass", "OSC 2 BYPASS");
        b.note(1060, T1 - 10, "OCTAVE: -2 .. +2");
        b.note(1060, T1 + 14, "PITCH: +-7 semitones");
        b.note(1060, T2 - 10, "SYNC 2>1: osc 1 restarts");
        b.note(1060, T2 + 14, "with osc 2");
        b.sec(14, 306, 1266, 574, "MIXER", true);
        b.K(110, B1, "o1_level", "OSC 1");
        b.K(270, B1, "sub", "OSC 1 SUB");
        b.K(110, B2, "o2_level", "OSC 2");
        b.K(270, B2, "noise", "NOISE");
        b.B(430, (B1 + B2) * 0.5f - 8, "fm", "FM 2>1");
        b.note(270, B2 + 66, "pink  <  0  >  white");
        b.K(620, B1, "vintage", "VINTAGE");
        b.K(800, B1, "overdrive", "OVERDRIVE");
        b.K(620, B2, "drive", "FILTER DRIVE");
        b.K(800, B2, "pgm_vol", "PROGRAM VOL");
        b.K(980, B1, "spread", "VOICE SPREAD");
        b.note(1060, B2 - 10, "SHAPE: sine, saw,");
        b.note(1060, B2 + 14, "square, narrowing pulse");
        ps.push_back(b.s);
    }
    {
        Builder b;
        b.s.name = "OSC 2";
        b.sec(14, 18, 640, 286, "GLIDE");
        b.B(80, T1 - 8, "glide", "GLIDE");
        b.K(210, T1, "glide_rate", "RATE");
        b.K(350, T1, "o1_glide", "OSC 1 GLIDE");
        b.K(490, T1, "o2_glide", "OSC 2 GLIDE");
        b.K(210, T2, "glide_mode", "MODE");
        b.L(270, T2 - 46, "glide_mode", 140);
        b.sec(652, 18, 1266, 286, "PITCH BEND, LOW SPLIT");
        b.K(730, T1, "pb_up", "BEND UP");
        b.K(870, T1, "pb_down", "BEND DOWN");
        b.B(730, T2 - 8, "split2", "-2 OCT");
        b.B(870, T2 - 8, "split1", "-1 OCT");
        b.K(1030, T2, "split_point", "SPLIT POINT");
        b.sec(14, 306, 1266, 574, "UNISON");
        b.B(80, B1 - 8, "unison", "UNISON");
        b.K(220, B1, "uni_voices", "VOICES");
        b.K(370, B1, "uni_detune", "DETUNE");
        b.K(520, B1, "uni_keymode", "KEY MODE");
        b.L(580, B1 - 30, "uni_keymode", 80);
        b.B(80, B2 - 8, "uni_retrig", "ENV RETRIG");
        b.note(900, B1 - 10, "CHORD MEMORY: the program's own");
        b.note(900, B1 + 14, "chord (made on the VST3)");
        ps.push_back(b.s);
    }
    {
        Builder b;
        b.s.name = "FILTER";
        b.sec(14, 18, 1266, 286, "FILTER", true);
        b.K(150, (T1 + T2) * 0.5f, "cutoff", "CUTOFF", true);
        b.K(360, T1, "reso", "RESONANCE");
        b.K(360, T2, "drive", "DRIVE");
        b.K(520, T1, "keyamt", "KEY>LPF FREQ");
        b.K(700, T1, "e1_amt", "ENV 1 AMOUNT");
        b.B(700, T2 - 8, "e1_vel", "VELOCITY");
        b.L(880, T1 - 40, "env_routing", 200, {"Filter / Amp", "Aux / Filter+Amp", "Aux / Filter+Gate"}, 24.0f);
        b.note(980, T2 + 4, "ENV ROUTING");
        b.sec(14, 306, 1266, 574, "ENV 1", true);
        const char *e[5][2] = {{"e1_del", "DELAY"}, {"e1_att", "ATTACK"}, {"e1_dec", "DECAY"}, {"e1_sus", "SUSTAIN"}, {"e1_rel", "RELEASE"}};
        for (int i = 0; i < 5; i++)
            b.K(110.0f + 175.0f * (float)i, (B1 + B2) * 0.5f, e[i][0], e[i][1]);
        b.K(1000, (B1 + B2) * 0.5f, "e1_dest", "AUX DEST");
        ps.push_back(b.s);
    }
    {
        Builder b;
        b.s.name = "FILTER 2";
        b.sec(14, 18, 1266, 286, "ENV 2", true);
        const char *e[5][2] = {{"e2_del", "DELAY"}, {"e2_att", "ATTACK"}, {"e2_dec", "DECAY"}, {"e2_sus", "SUSTAIN"}, {"e2_rel", "RELEASE"}};
        for (int i = 0; i < 5; i++)
            b.K(110.0f + 175.0f * (float)i, (T1 + T2) * 0.5f, e[i][0], e[i][1]);
        b.K(1000, T1, "e2_amt", "AMOUNT");
        b.B(1000, T2 - 8, "e2_vel", "VELOCITY");
        b.sec(14, 306, 1266, 574, "ROUTING, REPEAT");
        b.L(60, B1 - 40, "env_routing", 220, {"Filter / Amp", "Aux / Filter+Amp", "Aux / Filter+Gate"}, 26.0f);
        b.L(420, B1 - 40, "env_repeat", 180, {}, 26.0f);
        b.note(220, B2 + 30, "ENV 1 / ENV 2");
        b.note(510, B2 + 30, "ENVELOPE REPEAT");
        b.K(850, (B1 + B2) * 0.5f, "e1_dest", "ENV 1 AUX DEST");
        ps.push_back(b.s);
    }
    {
        Builder b;
        b.s.name = "LFO";
        for (int k = 0; k < 2; k++) {
            float y0 = k ? 306.0f : 18.0f, y = k ? B1 : T1, y2 = k ? B2 : T2;
            b.sec(14, y0, 1266, y0 + 268.0f, k ? "LFO 2: PER VOICE" : "LFO 1: GLOBAL");
            static char ids[2][7][16];
            const char *suf[7] = {"freq", "amt", "shape", "sync", "dest", "reset", "slew"};
            for (int i = 0; i < 7; i++)
                std::snprintf(ids[k][i], 16, "lfo%d_%s", k + 1, suf[i]);
            b.K(110, (y + y2) * 0.5f, ids[k][0], "FREQUENCY");
            b.K(270, (y + y2) * 0.5f, ids[k][1], "AMOUNT");
            b.B(420, y - 8, ids[k][2], "SHAPE", true);
            b.L(470, y - 48, ids[k][2], 110);
            b.B(420, y2 - 8, ids[k][3], "CLK SYNC");
            b.K(760, (y + y2) * 0.5f, ids[k][4], "DESTINATION");
            b.B(920, (y + y2) * 0.5f - 8, ids[k][5], "RESET");
            b.K(1080, (y + y2) * 0.5f, ids[k][6], "SLEW");
        }
        ps.push_back(b.s);
    }
    for (int half = 0; half < 2; half++) {
        Builder b;
        b.s.name = half ? "MOD 2" : "MOD";
        b.sec(14, 18, 1266, 286, half ? "MODULATION 9-12" : "MODULATION 1-4");
        b.sec(14, 306, 1266, 574, half ? "MODULATION 13-16" : "MODULATION 5-8");
        for (int k = 0; k < 8; k++) {
            int slot = half * 8 + k;
            float x0 = 30.0f + 310.0f * (float)(k % 4), y = k < 4 ? T1 + 56.0f : B1 + 56.0f;
            static char ids[16][3][16], lbl[16][16];
            std::snprintf(ids[slot][0], 16, "mod%d_src", slot + 1);
            std::snprintf(ids[slot][1], 16, "mod%d_amt", slot + 1);
            std::snprintf(ids[slot][2], 16, "mod%d_dst", slot + 1);
            std::snprintf(lbl[slot], 16, "MOD %d", slot + 1);
            b.K(x0 + 50.0f, y, ids[slot][0], "SOURCE");
            b.K(x0 + 145.0f, y, ids[slot][1], "AMOUNT");
            b.K(x0 + 240.0f, y, ids[slot][2], "DEST");
            b.note(x0 + 145.0f, y - 74.0f, lbl[slot]);
        }
        ps.push_back(b.s);
    }
    {
        Builder b;
        b.s.name = "FX";
        b.sec(14, 18, 1266, 286, "EFFECT");
        b.B(80, T1 - 8, "fx_on", "ON");
        b.B(80, T2 - 8, "fx_sync", "CLK SYNC");
        b.K(230, (T1 + T2) * 0.5f, "fx_type", "TYPE");
        b.K(450, (T1 + T2) * 0.5f, "fx_time", "TIME");
        b.K(620, (T1 + T2) * 0.5f, "fx_mix", "DEPTH/MIX");
        b.K(790, (T1 + T2) * 0.5f, "fx_misc", "FBACK/MISC");
        b.K(1080, (T1 + T2) * 0.5f, "overdrive", "OVERDRIVE");
        b.sec(14, 306, 1266, 574, "REVERB");
        b.B(80, (B1 + B2) * 0.5f - 8, "rev_on", "ON");
        b.K(230, (B1 + B2) * 0.5f, "rev_mix", "MIX");
        b.K(430, (B1 + B2) * 0.5f, "rev_damp", "DAMPING");
        b.K(630, (B1 + B2) * 0.5f, "rev_predelay", "PREDELAY");
        b.K(830, (B1 + B2) * 0.5f, "rev_decay", "DECAY");
        b.K(1030, (B1 + B2) * 0.5f, "rev_tone", "TONE");
        b.note(1030, B2 + 34, "lpf  <  0  >  hpf");
        b.note(230, B2 + 34, "dry  >  wet");
        ps.push_back(b.s);
    }
    {
        Builder b;
        b.s.name = "PLAY";
        b.sec(14, 18, 760, 286, "ARPEGGIATOR");
        b.B(80, T1 - 8, "arp_on", "ON");
        b.K(220, T1, "arp_mode", "MODE");
        b.L(280, T1 - 50, "arp_mode", 100);
        b.K(470, T1, "arp_oct", "OCTAVE");
        b.K(630, T1, "arp_repeat", "REPEAT");
        b.B(80, T2 - 8, "hold", "HOLD");
        b.B(470, T2 - 8, "arp_relatch", "RELATCH");
        b.sec(772, 18, 1266, 286, "CLOCK, SEQUENCER");
        b.K(860, T1, "divide", "CLK DIVIDE");
        b.B(1060, T1 - 8, "seq_play", "SEQ PLAY");
        b.note(1019, T2 - 4, "on the MPC's tempo; PLAY:");
        b.note(1019, T2 + 20, "the program's own sequence");
        b.sec(14, 306, 640, 574, "VOICES");
        b.L(40, B1 - 30, "voices", 120);
        b.L(240, B1 - 30, "quality", 110, {"ECO (2x)", "HIGH (4x)"});
        b.note(327, B2 + 10, "HIGH: 4x inside (more CPU)");
        b.sec(652, 306, 1266, 574, "PROGRAM");
        b.K(740, B1, "pgm_vol", "PROGRAM VOL");
        b.K(900, B1, "vol", "VOLUME");
        b.K(1080, B1, "bpm", "BPM");
        b.note(1080, B2 - 6, "(off the MPC's clock)");
        ps.push_back(b.s);
    }
    return ps;
}

/* ----------------------------------------------------------------------------- drawing --- */
void drawScreen(Graphics &g, const Screen &pg)
{
    auto r = Rectangle<float>(0.0f, 0.0f, (float)TW, (float)TH);
    drawPanelFace(g, r, 1.0f);
    for (auto &s : pg.secs) {
        if (s.plate) {
            drawPlate(g, s.r, 14.0f, 1.0f);
            drawWide(g, s.title, Rectangle<float>(s.r.getRight() - 330.0f, s.r.getBottom() - 30.0f, 310.0f, 22.0f), 19.0f, 1.18f, col::legend,
                     Justification::right);
        } else
            drawSection(g, s.r, s.title);
    }
    for (auto &c : pg.ctls) {
        if (c.kind == Ctl::knob) {
            float ks = c.big ? KS * 1.75f : KS;
            drawLegend(g, c.legend, Rectangle<float>(c.x - 75.0f, c.y + ks * 0.5f + 2.0f, 150.0f, 20.0f), 15.5f);
        } else if (c.kind == Ctl::button)
            drawLegend(g, c.legend, Rectangle<float>(c.x - 75.0f, c.y + BH * 0.5f + 6.0f, 150.0f, 20.0f), 15.0f);
        else {
            g.setColour(col::legend);
            g.setFont(font(Face::title, 15.0f));
            for (int i = 0; i < c.names.size(); i++)
                g.drawText(c.names[i], Rectangle<float>(c.x + 22.0f, c.y + c.rh * (float)i, c.w, c.rh), Justification::centredLeft, false);
        }
    }
    g.setColour(col::legend);
    g.setFont(font(Face::label, 14.5f));
    for (auto &[p, t] : pg.notes)
        g.drawText(t, Rectangle<float>(p.x - 170.0f, p.y - 9.0f, 340.0f, 18.0f), Justification::centred, false);
    /* the bottom strip: the name, the page; the maker's tag */
    drawLogo(g, Rectangle<float>(18.0f, 588.0f, 100.0f, 26.0f));
    drawLegend(g, pg.name, Rectangle<float>(140.0f, 586.0f, 300.0f, 30.0f), 16.0f, Justification::centredLeft);
    drawTag(g, Rectangle<float>(1280.0f - 18.0f - 200.0f, 628.0f - 14.0f - 24.0f, 200.0f, 24.0f));
    drawWear(g, r, 1.0f, {});
}

Image snapBackground(const Screen &pg)
{
    Image img(Image::RGB, TW, TH, true);
    Graphics g(img);
    drawScreen(g, pg);
    return img;
}

bool writePng(const Image &img, const File &f)
{
    f.deleteFile();
    FileOutputStream out(f);
    PNGImageFormat png;
    return out.openedOk() && png.writeImageToStream(img, out);
}
var rectVar(Rectangle<int> r) { return Array<var>{r.getX(), r.getY(), r.getWidth(), r.getHeight()}; }

Image drawCtl(const Image &bg, const Ctl &c, Rectangle<int> r, int index, int steps, int row = -1)
{
    Image img(Image::RGB, r.getWidth(), r.getHeight(), true);
    Graphics g(img);
    g.drawImageAt(bg.getClippedImage(r), 0, 0);
    g.addTransform(AffineTransform::translation((float)-r.getX(), (float)-r.getY()));
    if (c.kind == Ctl::knob) {
        float pos = steps > 1 ? (float)index / (float)(steps - 1) : 0.0f;
        float ks = c.big ? KS * 1.75f : KS;
        KnobKind k = c.big ? KnobKind::big : bipolarOf(c.id) ? KnobKind::bipolar : KnobKind::pot;
        auto kr = Rectangle<float>(ks, ks).withCentre({c.x, c.y});
        drawKnob(g, kr, k, true);
        drawKnobPointer(g, kr, pos, k);
    } else if (c.kind == Ctl::button) {
        bool lit = !c.cycle && index > 0;
        drawButton(g, Rectangle<float>(BW, BH).withCentre({c.x, c.y}), lit, false, false);
    } else {
        drawLed(g, {c.x + 9.0f, c.y + c.rh * ((float)row + 0.5f)}, 10.0f, index == row ? 1.0f : 0.0f);
    }
    return img;
}
} // namespace

int main(int argc, char **argv)
{
    ScopedJuceInitialiser_GUI gui;
    SharedResourcePointer<UiResources> res;
    if (argc < 2) {
        std::fprintf(stderr, "usage: fake5_skin_render <out folder>\n");
        return 2;
    }
    File dir = File::getCurrentWorkingDirectory().getChildFile(argv[1]);
    dir.deleteRecursively();
    dir.createDirectory();
    Array<var> pageList;
    for (auto &pg : screens()) {
        String tag = pg.name.toLowerCase().retainCharacters("abcdefghijklmnopqrstuvwxyz0123456789");
        Image bg = snapBackground(pg);
        writePng(bg, dir.getChildFile(tag + "_panel.png"));
        Array<var> list;
        int n = 0;
        for (auto &c : pg.ctls) {
            auto rg = rangeOf(c.id);
            int steps = rg.hi - rg.lo + 1;
            auto *o = new DynamicObject();
            o->setProperty("id", c.id);
            o->setProperty("steps", steps);
            Array<var> names;
            for (auto &t : rg.names)
                names.add(t);
            o->setProperty("names", names);
            String base = tag + "_" + String(n++).paddedLeft('0', 3) + "_" + c.id.retainCharacters("abcdefghijklmnopqrstuvwxyz0123456789_");
            if (c.kind == Ctl::knob) {
                float ks = c.big ? KS * 1.75f : KS;
                auto r = Rectangle<float>(ks + 6.0f, ks + 6.0f).withCentre({c.x, c.y}).toNearestInt();
                int frames = jmax(2, jmin(steps, 128, 9800 / r.getHeight()));
                Image strip(Image::RGB, r.getWidth(), r.getHeight() * frames, true);
                {
                    Graphics sg(strip);
                    for (int f = 0; f < frames; f++) {
                        int idx = frames == steps ? f : (int)std::lround((double)f * (steps - 1) / (frames - 1));
                        sg.drawImageAt(drawCtl(bg, c, r, idx, steps), 0, f * r.getHeight());
                    }
                }
                writePng(strip, dir.getChildFile(base + "_knob.png"));
                auto fr = r.getUnion(Rectangle<float>(c.x - 66.0f, c.y + ks * 0.5f, 132.0f, 24.0f).toNearestInt());
                o->setProperty("kind", "knob");
                o->setProperty("bipolar", bipolarOf(c.id));
                o->setProperty("rect", rectVar(r));
                o->setProperty("frame", rectVar(fr));
                o->setProperty("frames", frames);
                o->setProperty("strip", base + "_knob.png");
            } else if (c.kind == Ctl::button) {
                auto r = Rectangle<float>(BW + 8.0f, BH + 8.0f).withCentre({c.x, c.y}).toNearestInt();
                o->setProperty("kind", "button");
                o->setProperty("rect", rectVar(r));
                Array<var> rs;
                for (int i = 0; i < steps; i++) {
                    auto *ro = new DynamicObject();
                    ro->setProperty("a", i);
                    ro->setProperty("b", i);
                    ro->setProperty("next", c.cycle ? (i + 1) % steps : (i ? 0 : 1));
                    String f = base + "_button_" + String(i) + ".png";
                    writePng(drawCtl(bg, c, r, i, steps), dir.getChildFile(f));
                    ro->setProperty("image", f);
                    rs.add(var(ro));
                }
                o->setProperty("ranges", rs);
            } else {
                o->setProperty("kind", "leds");
                Array<var> rows;
                for (int k = 0; k < c.names.size(); k++) {
                    auto r = Rectangle<float>(c.x, c.y + c.rh * (float)k, 22.0f + c.w, c.rh).toNearestInt();
                    auto *ro = new DynamicObject();
                    ro->setProperty("rect", rectVar(r));
                    String on = base + "_led" + String(k) + "_on.png", off = base + "_led" + String(k) + "_off.png";
                    writePng(drawCtl(bg, c, r, k, steps, k), dir.getChildFile(on));
                    writePng(drawCtl(bg, c, r, -1, steps, k), dir.getChildFile(off));
                    ro->setProperty("on", on);
                    ro->setProperty("off", off);
                    rows.add(var(ro));
                }
                o->setProperty("rows", rows);
            }
            list.add(var(o));
        }
        auto *po = new DynamicObject();
        po->setProperty("name", pg.name);
        po->setProperty("background", tag + "_panel.png");
        po->setProperty("controls", list);
        pageList.add(var(po));
        std::printf("%-9s %d controls\n", pg.name.toRawUTF8(), (int)pg.ctls.size());
    }
    auto *m = new DynamicObject();
    m->setProperty("pages", pageList);
    dir.getChildFile("manifest.json").replaceWithText(JSON::toString(var(m)));
    std::printf("manifest.json in %s\n", dir.getFullPathName().toRawUTF8());
    return 0;
}
