/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (FAKE5) */
/* The MPC skin's pictures: the Take 5's panel, section by section, a section (or two that belong together) a page, as
 * the VST draws it. Where the VST's panel has the section (OSCILLATORS and MIXER; FILTER and ENVELOPES; EFFECT and
 * REVERB) the page is the VST's own panel, painted by the VST's own code and enlarged, its controls in their places;
 * what the Take 5 keeps in its program menu sits beside it. The rest (LFO 1 beside LFO 2; the arpeggiator; GLIDE RATE,
 * VINTAGE, unison, glide) is drawn in the same hand. A choice is a row of buttons, one a value (the arp's modes, the
 * glide's), or a knob with a little display over it (the effect's type) or a display alone (the LFOs' destinations):
 *
 *   OSC      OSCILLATORS, MIXER; LOW SPLIT bottom left, the keys and osc 2's bypass beside it
 *   FILTER   FILTER, ENVELOPES (+ key amount, the envelopes' delays, env 1's aux destination, env repeat)
 *   LFO      LFO 1: GLOBAL | LFO 2: PER VOICE  /  MOD 1-8  /  MOD 9-16 (the matrix on the display)
 *   FX       EFFECT, REVERB
 *   ARP      CLOCK; ARPEGGIATOR (on, relatch, hold, octaves, repeats); MODE
 *   VOICE    GLIDE RATE, VINTAGE, VOLUME, OVERDRIVE, VOICE SPREAD, PROGRAM VOL; unison's and glide's settings; the
 *            VST's GLIDE, UNISON, HOLD; VOICES, QUALITY; the bend, the split point
 *
 * Each screen is drawn at the MPC's 1280 x 628 and cut up: its background, and every control at every value it can
 * take (a knob's film strip, a button's look for each value with the value a press gives, an LED's on and off).
 * manifest.json says what each is, for make_skin.py.
 *
 *   fake5_skin_render <out folder> */
#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;
using namespace f5ui;
using namespace f5;

namespace {
constexpr int TW = 1280, TH = 628;
/* the VST panel's places (PluginEditor.cpp, namespace L) */
constexpr float VKD = 70.0f, VBW = 34.0f, VBH = 25.0f, VR1 = 108, VR2 = 222, VO1 = 352, VO2 = 458;
constexpr float VENVX[5] = {1236, 1336, 1436, 1536, 1636}, VENV_VEL = 1730;
constexpr float VFXX[6] = {1366, 1444, 1522, 1600, 1678, 1758};
/* a drawn page's sizes */
constexpr float KS = 74.0f, BW = 46.0f, BH = 32.0f, LR = 22.0f;
/* MOD's display: its box, the table's rows and columns (slot, source, amount, destination) */
constexpr float OLED_X0 = 150, OLED_X1 = 1130, OLED_Y0 = 64, OLED_Y1 = 566;
constexpr float OLED_ROW0 = 126, OLED_RH = 54;
constexpr float OLED_COL[4] = {178, 270, 640, 820};

struct Ctl {
    enum Kind { knob, button, leds, fixed, cell, radio } kind;   /* fixed: a button with no parameter, drawn into the
                                                           * background; cell: a value on the display, its text the MPC's;
                                                           * radio: a button a value, lit while it is the one */
    String id;
    float x = 0, y = 0;                  /* the centre (knob, button) */
    String legend;                       /* (a drawn page's; the VST's own pages have theirs in the background) */
    float size = KS;                     /* a knob's diameter, a button's width */
    KnobKind kk = KnobKind::pot;
    bool cycle = false;                  /* a button stepping through the values (else a switch) */
    std::vector<Point<float>> pts;       /* LEDs: each one's centre */
    std::vector<int> vals;               /* LEDs: the value (its index) each one lights for */
    float ledR = 10.0f;                  /* an LED's size */
    StringArray names;                   /* LEDs on a drawn page: each one's name, right of it (radio: under it) */
    float nameW = 0;
    std::vector<Point<float>> kleds;     /* a knob's own LEDs (OCTAVE): LED i lit at the knob's value i, drawn in its film
                                          * strip (no LEDs of their own over the knob to take its touches) */
    float legDy = 0;                     /* a knob's legend this much lower (room for its pictures round it) */
    bool boxed = false;                  /* a cell on a panel page: in a little display of its own */
    float bh = 0;                        /* a radio button's height (0: as wide as BW's) */
};
struct Sec {
    Rectangle<float> r;
    String title;
    int style;                           /* 0 a white frame with its title, 1 a blue-grey plate (its title below right),
                                          * 2 a band: the title alone over it, as the VST's top */
};
struct Crop {
    Rectangle<float> src;                /* the VST panel's part shown */
    float s = 1.0f;
    Point<float> off;                    /* its top left on the page */
};
struct Screen {
    String name;
    std::vector<Crop> crops;
    bool oled = false;                   /* MOD: the Take 5's display, its matrix a table on it */
    String oledTitle;
    std::vector<Sec> secs;
    std::vector<std::pair<float, Point<float>>> vlines;   /* x, (y0, y1) */
    std::vector<Ctl> ctls;
    std::vector<std::tuple<Rectangle<float>, int>> icons;   /* LFO shapes' pictures */
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
                r.names.add(d.choice[k - d.lo]);
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
    /* the VST's panel, its part src at scale sc, its top left at (x, y) on the page */
    void vst(Rectangle<float> src, float sc, float x, float y) { s.crops.push_back({src, sc, {x, y}}); }
    Point<float> at(float vx, float vy) const
    {
        auto &c = s.crops.back();
        return c.off + (Point<float>(vx, vy) - c.src.getPosition()) * c.s;
    }
    float sc() const { return s.crops.back().s; }
    /* a VST control at its VST place */
    void vK(float vx, float vy, const char *id, KnobKind k = KnobKind::pot, float vsize = VKD)
    {
        Ctl c{Ctl::knob, id};
        auto p = at(vx, vy);
        c.x = p.x;
        c.y = p.y;
        c.size = vsize * sc();
        c.kk = k == KnobKind::encoder ? KnobKind::pot : k;   /* (the MPC: an encoder shows where it is) */
        if (c.kk == KnobKind::pot && bipolarOf(id))
            c.kk = KnobKind::bipolar;
        s.ctls.push_back(c);
    }
    void vB(float vx, float vy, const char *id, bool cycle = false)
    {
        Ctl c{id[0] ? Ctl::button : Ctl::fixed, id};
        auto p = at(vx, vy);
        c.x = p.x;
        c.y = p.y;
        c.size = VBW * sc();
        c.cycle = cycle;
        s.ctls.push_back(c);
    }
    void vL(const char *id, std::vector<Point<float>> vpts, std::vector<int> vals)
    {
        Ctl c{Ctl::leds, id};
        for (auto &p : vpts)
            c.pts.push_back(at(p.x, p.y));
        c.vals = std::move(vals);
        c.ledR = 7.0f * sc();
        s.ctls.push_back(c);
    }
    /* a value on the display (MOD): its box */
    void cell(Rectangle<float> r, const char *id, bool centred = false)
    {
        Ctl c{Ctl::cell, id, r.getX(), r.getY()};
        c.size = r.getWidth();
        c.nameW = r.getHeight();
        c.cycle = centred;
        s.ctls.push_back(c);
    }
    /* a value in a little display (the effect's type, a destination): its box; legend under it */
    void box(Rectangle<float> r, const char *id, const char *leg = "")
    {
        Ctl c{Ctl::cell, id, r.getX(), r.getY(), leg};
        c.size = r.getWidth();
        c.nameW = r.getHeight();
        c.cycle = true;
        c.boxed = true;
        s.ctls.push_back(c);
    }
    /* a button for each value (from the first, or as vals says), their names under them */
    void R(std::vector<Point<float>> pts, const char *id, StringArray names = {}, std::vector<int> vals = {}, float w = BW, float h = BH)
    {
        auto r = rangeOf(id);
        Ctl c{Ctl::radio, id};
        c.names = names.isEmpty() ? r.names : names;
        c.pts = std::move(pts);
        for (size_t k = 0; k < c.pts.size(); k++)
            c.vals.push_back(vals.empty() ? (int)k : vals[k]);
        c.size = w;
        c.bh = h;
        s.ctls.push_back(c);
    }
    /* a drawn page's */
    void sec(float x0, float y0, float x1, float y1, const char *t, int style = 0) { s.secs.push_back({{x0, y0, x1 - x0, y1 - y0}, t, style}); }
    void K(float x, float y, const char *id, const char *leg, float size = KS)
    {
        Ctl c{Ctl::knob, id, x, y, leg, size};
        c.kk = bipolarOf(id) ? KnobKind::bipolar : KnobKind::pot;
        s.ctls.push_back(c);
    }
    void B(float x, float y, const char *id, const char *leg, bool cycle = false)
    {
        Ctl c{Ctl::button, id, x, y, leg, BW};
        c.cycle = cycle;
        s.ctls.push_back(c);
    }
    /* a list of LEDs, one under the other from (x, y) (the first LED's centre), their names right of them */
    void L(float x, float y, const char *id, float w, StringArray names = {}, float rh = LR)
    {
        auto r = rangeOf(id);
        Ctl c{Ctl::leds, id};
        c.names = names.isEmpty() ? r.names : names;
        for (int k = 0; k < c.names.size(); k++) {
            c.pts.push_back({x, y + rh * (float)k});
            c.vals.push_back(k);
        }
        c.nameW = w;
        s.ctls.push_back(c);
    }
};

std::vector<Screen> screens()
{
    std::vector<Screen> ps;
    /* ---------------------------------------------------- OSC: OSCILLATORS, MIXER (the VST's); LOW SPLIT and the keys */
    {
        Builder b;
        b.s.name = "OSC";
        b.vst({152, 287, 676, 266}, 1.6f, 99, 8);
        for (int o = 0; o < 2; o++) {
            float y = o ? VO2 : VO1;
            const char *oct = o ? "o2_oct" : "o1_oct";
            b.vK(262, y, oct, KnobKind::plain);
            auto &kc = b.s.ctls.back();
            for (int i = 0; i < 5; i++) {
                auto p = Point<float>(262.0f, y).getPointOnCircumference(40.0f, -1.45f + 0.36f * (float)i);
                kc.kleds.push_back(b.at(p.x, p.y));
            }
            kc.ledR = 7.0f * b.sc();
            b.vK(382, y, o ? "o2_pitch" : "o1_pitch");
            b.vK(552, y, o ? "o2_shape" : "o1_shape");
        }
        b.vB(470, (VO1 + VO2) * 0.5f - 6.0f, "sync");
        b.vK(672, VO1, "o1_level");
        b.vB(722, VO1 + 30, "fm");
        b.vK(774, VO1, "sub");
        b.vK(672, VO2, "o2_level");
        b.vK(774, VO2, "noise");
        /* bottom left: LOW SPLIT, the VST's own; right of it the program menu's keys and OSC 2's way round the filter */
        b.vst({596, 566, 108, 80}, 1.7f, 36, 436);
        b.vB(612, 592, "split2");
        b.vB(676, 592, "split1");
        const float ky = b.at(0, 592).y;
        b.B(330, ky, "o1_key", "OSC 1 KEY FOLLOW");
        b.B(520, ky, "o2_key", "OSC 2 KEY FOLLOW");
        b.B(710, ky, "o2_bypass", "OSC 2 FILTER BYPASS");
        ps.push_back(b.s);
    }
    /* ---------------------------------------------------------------------- FILTER: FILTER, ENVELOPES (the VST's) */
    {
        Builder b;
        b.s.name = "FILTER";
        b.vst({820, 287, 982, 266}, 1.28f, 11, 8);
        b.vK(874, VO1, "reso");
        b.vK(874, VO2, "drive");
        b.vK(990, (VO1 + VO2) * 0.5f - 4.0f, "cutoff", KnobKind::big, 150.0f);
        b.vL("env_routing", {{1092.0f, 338.0f}, {1092.0f, 355.0f}}, {0, 1});
        b.vL("env_routing", {{1092.0f, 410.0f}, {1092.0f, 427.0f}, {1092.0f, 444.0f}}, {0, 1, 2});
        b.vB(1100, 494, "env_routing", true);
        const char *e1[5] = {"e1_att", "e1_dec", "e1_sus", "e1_rel", "e1_amt"}, *e2[5] = {"e2_att", "e2_dec", "e2_sus", "e2_rel", "e2_amt"};
        for (int i = 0; i < 5; i++) {
            b.vK(VENVX[i], VO1, e1[i]);
            b.vK(VENVX[i], VO2, e2[i]);
        }
        b.vB(VENV_VEL, VO1 - 6, "e1_vel");
        b.vB(VENV_VEL, VO2 - 6, "e2_vel");
        /* the program menu's */
        b.sec(11, 372, 1269, 574, "PROGRAM MENU: FILTER, ENVELOPES");
        b.K(130, 470, "keyamt", "KEY AMOUNT");
        b.K(300, 470, "e1_del", "ENV 1 DELAY");
        b.K(470, 470, "e2_del", "ENV 2 DELAY");
        b.box({540, 390, 200, 34}, "e1_dest");
        b.K(640, 476, "e1_dest", "ENV 1 AUX DEST");
        b.R({{830, 460}, {930, 460}, {1030, 460}, {1140, 460}}, "env_repeat", {"OFF", "ENV 1", "ENV 2", "ENV 1+2"});
        b.s.secs.push_back({{865, 512, 240, 20}, "ENVELOPE REPEAT", 3});
        ps.push_back(b.s);
    }
    /* ------------------------------------------------------- LFO: LFO 1: GLOBAL | LFO 2: PER VOICE (as the keyboard's) */
    {
        Builder b;
        b.s.name = "LFO";
        b.s.vlines.push_back({640.0f, {28.0f, 570.0f}});
        for (int k = 0; k < 2; k++) {
            float x0 = k ? 640.0f : 0.0f;
            b.sec(x0, 14, x0 + 640, 50, k ? "LFO 2: PER VOICE" : "LFO 1: GLOBAL", 2);
            static char ids[2][7][16];
            const char *suf[7] = {"freq", "amt", "shape", "sync", "dest", "reset", "slew"};
            for (int i = 0; i < 7; i++)
                std::snprintf(ids[k][i], 16, "lfo%d_%s", k + 1, suf[i]);
            b.K(x0 + 120, 140, ids[k][0], "FREQUENCY", 92.0f);
            b.K(x0 + 320, 140, ids[k][1], "AMOUNT", 92.0f);
            /* SHAPE: a knob, its shapes' pictures round it where it points at each */
            b.K(x0 + 520, 140, ids[k][2], "SHAPE", 92.0f);
            b.s.ctls.back().legDy = 30.0f;
            for (int i = 0; i < NLFOSHAPE; i++) {
                float a = -2.62f + 5.24f * (float)i / (float)(NLFOSHAPE - 1);
                auto c = Point<float>(x0 + 520, 140).getPointOnCircumference(76.0f, a);
                b.s.icons.push_back({Rectangle<float>(34.0f, 13.0f).withCentre(c), i});
            }
            b.B(x0 + 120, 316, ids[k][3], "CLK SYNC");
            b.B(x0 + 320, 316, ids[k][5], "KEY RESET");
            b.K(x0 + 520, 316, ids[k][6], "SLEW", 80.0f);
            b.box({x0 + 70, 430, 500, 52}, ids[k][4], "DESTINATION");
        }
        ps.push_back(b.s);
    }
    /* MOD: the matrix on the Take 5's display, a slot a line: SOURCE, AMOUNT, DESTINATION (a tap and a drag sets
     * one, a double tap opens its list) */
    for (int half = 0; half < 2; half++) {
        Builder b;
        b.s.name = half ? "MOD 9-16" : "MOD 1-8";
        b.s.oled = true;
        b.s.oledTitle = half ? "MODULATION  9 - 16" : "MODULATION  1 - 8";
        for (int k = 0; k < 8; k++) {
            int slot = half * 8 + k;
            float y = OLED_ROW0 + OLED_RH * (float)k;
            static char ids[16][3][16];
            std::snprintf(ids[slot][0], 16, "mod%d_src", slot + 1);
            std::snprintf(ids[slot][1], 16, "mod%d_amt", slot + 1);
            std::snprintf(ids[slot][2], 16, "mod%d_dst", slot + 1);
            b.cell({OLED_COL[1], y, OLED_COL[2] - OLED_COL[1] - 10.0f, OLED_RH}, ids[slot][0]);
            b.cell({OLED_COL[2], y, OLED_COL[3] - OLED_COL[2] - 10.0f, OLED_RH}, ids[slot][1], true);
            b.cell({OLED_COL[3], y, OLED_X1 - 20.0f - OLED_COL[3], OLED_RH}, ids[slot][2]);
        }
        ps.push_back(b.s);
    }
    /* ------------------------------------------------------------------------- FX: EFFECT, REVERB (the VST's) */
    {
        Builder b;
        b.s.name = "FX";
        b.vst({1332, 16, 468, 268}, 2.08f, 153, 10);
        b.vB(VFXX[0], VR1 - 6, "fx_on");
        b.vK(VFXX[1], VR1 - 4, "fx_type", KnobKind::encoder, VKD * 0.92f);
        {
            auto t = b.s.ctls.back();       /* the effect's name over TYPE */
            b.box(Rectangle<float>(250.0f, 40.0f).withCentre({t.x, t.y - t.size * 0.5f - 44.0f}), "fx_type");
        }
        b.vK(VFXX[2], VR1, "fx_time");
        b.vK(VFXX[3], VR1, "fx_mix");
        b.vK(VFXX[4], VR1, "fx_misc");
        b.vB(VFXX[5], VR1 - 26, "fx_sync");
        b.vB(VFXX[0], VR2 - 6, "rev_on");
        b.vK(VFXX[1], VR2, "rev_mix");
        b.vK(VFXX[2], VR2, "rev_damp");
        b.vK(VFXX[3], VR2, "rev_predelay");
        b.vK(VFXX[4], VR2, "rev_decay");
        b.vK(VFXX[5], VR2, "rev_tone");
        ps.push_back(b.s);
    }
    /* ----------------------------------------------- ARP: CLOCK; ARPEGGIATOR; its MODE, a button each (drawn) */
    {
        Builder b;
        b.s.name = "ARP";
        b.sec(20, 30, 420, 312, "CLOCK");
        b.box({52, 66, 136, 40}, "bpm");
        b.K(120, 180, "bpm", "BPM", 92.0f);
        b.box({252, 66, 136, 40}, "divide");
        b.K(320, 180, "divide", "CLOCK DIVIDE", 92.0f);
        b.sec(440, 30, 1260, 312, "ARPEGGIATOR");
        b.B(610, 104, "arp_on", "ON");
        b.B(850, 104, "arp_relatch", "RELATCH");
        b.B(1090, 104, "hold", "HOLD");
        b.R({{560, 220}, {640, 220}, {720, 220}}, "arp_oct", {"1", "2", "3"});
        b.s.secs.push_back({{520, 266, 240, 20}, "OCTAVES", 3});
        b.R({{900, 220}, {980, 220}, {1060, 220}, {1140, 220}}, "arp_repeat", {"OFF", "1", "2", "3"});
        b.s.secs.push_back({{900, 266, 240, 20}, "REPEATS", 3});
        b.sec(20, 334, 1260, 566, "MODE");
        b.R({{180, 440}, {410, 440}, {640, 440}, {870, 440}, {1100, 440}}, "arp_mode", {"UP", "DOWN", "UP + DOWN", "RANDOM", "ASSIGN"}, {},
            110.0f, 60.0f);
        ps.push_back(b.s);
    }
    /* ------------------------- VOICE: the keyboard's left end, the output, unison and glide, the bend; the VST's keys */
    {
        Builder b;
        b.s.name = "VOICE";
        const char *kn[6][2] = {{"glide_rate", "GLIDE RATE"}, {"vintage", "VINTAGE"}, {"vol", "VOLUME"},
                                {"overdrive", "OVERDRIVE"}, {"spread", "VOICE SPREAD"}, {"pgm_vol", "PROGRAM VOL"}};
        for (int i = 0; i < 6; i++)
            b.K(140.0f + 200.0f * (float)i, 78, kn[i][0], kn[i][1], 84.0f);
        b.s.vlines.push_back({440.0f, {30.0f, 150.0f}});
        /* the program menu's: unison | glide */
        b.sec(14, 168, 1266, 380, "PROGRAM MENU: UNISON, GLIDE");
        b.K(84, 262, "uni_voices", "UNI VOICES");
        b.K(222, 262, "uni_detune", "UNI DETUNE");
        b.R({{330, 252}, {405, 252}, {480, 252}}, "uni_keymode", {"LOW", "HIGH", "LAST"});
        b.s.secs.push_back({{285, 300, 240, 20}, "KEY MODE", 3});
        b.B(585, 252, "uni_retrig", "RETRIGGER");
        b.s.vlines.push_back({660.0f, {200.0f, 360.0f}});
        b.K(735, 262, "o1_glide", "OSC 1 GLIDE");
        b.K(865, 262, "o2_glide", "OSC 2 GLIDE");
        b.R({{1000, 232}, {1160, 232}, {1000, 312}, {1160, 312}}, "glide_mode", {"FIXED RATE", "FIXED RATE A", "FIXED TIME", "FIXED TIME A"});
        /* GLIDE, UNISON, HOLD: the VST's own bottom row; the voices, the bend */
        b.vst({60, 566, 242, 56}, 1.95f, 20, 404);
        b.vB(96, 588, "glide");
        b.vB(184, 588, "unison");
        b.vB(268, 588, "hold");
        b.R({{560, 448}, {680, 448}}, "voices", {"5 VOICES", "10 VOICES"});
        b.R({{560, 528}, {680, 528}}, "quality", {"ECO (2x)", "HIGH (4x)"});
        b.K(860, 486, "pb_up", "BEND UP");
        b.K(1010, 486, "pb_down", "BEND DOWN");
        b.K(1160, 486, "split_point", "SPLIT POINT");
        ps.push_back(b.s);
    }
    return ps;
}

/* ----------------------------------------------------------------------------- drawing --- */
/* an LED as the VST's (Look.cpp's drawLed), its glow kept within maxR of its centre: a picture of its own cuts
 * nothing off */
void drawLedFit(Graphics &g, Point<float> c, float s, float level, float maxR)
{
    const float rr = s * 0.5f, gr = jmin(s * 2.4f, maxR);
    if (level > 0.02f && gr > rr) {
        g.setGradientFill(ColourGradient(Colour(0xffff3a1a).withAlpha(0.62f * level), c.x, c.y, Colours::transparentBlack, c.x + gr, c.y, true));
        g.fillEllipse(Rectangle<float>(gr * 2.0f, gr * 2.0f).withCentre(c));
    }
    g.setColour(Colours::black.withAlpha(0.75f));
    g.fillEllipse(Rectangle<float>(rr * 2.5f, rr * 2.5f).withCentre(c.translated(0.2f, 0.5f)));
    ColourGradient cg(Colour(0xff5a1712).interpolatedWith(Colour(0xffffd0a8), level), c.x - rr * 0.3f, c.y - rr * 0.35f,
                      Colour(0xff250806).interpolatedWith(Colour(0xffe8200c), level), c.x + rr, c.y + rr, true);
    g.setGradientFill(cg);
    g.fillEllipse(Rectangle<float>(rr * 2.0f, rr * 2.0f).withCentre(c));
    g.setColour(Colours::white.withAlpha(0.35f + 0.3f * level));
    g.fillEllipse(Rectangle<float>(rr * 0.6f, rr * 0.45f).withCentre(c.translated(-rr * 0.32f, -rr * 0.38f)));
}
/* a little display (a cell's on a panel page): bezel, glass */
void drawMiniOled(Graphics &g, Rectangle<float> box)
{
    g.setColour(Colours::black.withAlpha(0.55f));
    g.fillRoundedRectangle(box.expanded(8.0f).translated(0.0f, 3.0f), 9.0f);
    g.setGradientFill(ColourGradient(Colour(0xff2a2b2e), 0.0f, box.getY() - 7.0f, Colour(0xff121214), 0.0f, box.getBottom() + 7.0f, false));
    g.fillRoundedRectangle(box.expanded(7.0f), 8.0f);
    g.setColour(Colours::black);
    g.fillRoundedRectangle(box.expanded(2.0f), 4.0f);
    g.setColour(col::oledBg);
    g.fillRoundedRectangle(box, 3.0f);
}
float buttonH(const Ctl &c);
Image vstPanel;                          /* the VST's panel, its face and legends (no controls), twice its size */

void drawScreen(Graphics &g, const Screen &pg)
{
    auto r = Rectangle<float>(0.0f, 0.0f, (float)TW, (float)TH);
    drawPanelFace(g, r, 1.0f);
    for (auto &cr : pg.crops) {
        /* the VST's part, its edges let into the page's face (a fade of 28 pixels: no seam where they meet) */
        auto src = (cr.src * 2.0f).toNearestInt();
        int w = (int)std::lround(cr.src.getWidth() * cr.s), h = (int)std::lround(cr.src.getHeight() * cr.s);
        Image part(Image::ARGB, w, h, true);
        {
            Graphics pg2(part);
            pg2.setImageResamplingQuality(Graphics::highResamplingQuality);
            pg2.drawImage(vstPanel, 0, 0, w, h, src.getX(), src.getY(), src.getWidth(), src.getHeight());
        }
        const float F = 28.0f;
        for (int y = 0; y < h; y++)
            for (int x = 0; x < w; x++) {
                float d = jmin(jmin((float)x, (float)(w - 1 - x)), jmin((float)y, (float)(h - 1 - y)));
                if (d < F) {
                    float a = d / F;
                    a = a * a * (3.0f - 2.0f * a);
                    auto c = part.getPixelAt(x, y);
                    part.setPixelAt(x, y, c.withAlpha(c.getFloatAlpha() * a));
                }
            }
        g.drawImageAt(part, (int)cr.off.x, (int)cr.off.y);
    }
    if (pg.oled) {
        /* the display as the VST draws it, larger: its bezel, the glass, the dim headings, the slots' numbers */
        auto box = Rectangle<float>(OLED_X0, OLED_Y0, OLED_X1 - OLED_X0, OLED_Y1 - OLED_Y0);
        g.setColour(Colours::black.withAlpha(0.6f));
        g.fillRoundedRectangle(box.expanded(14.0f).translated(0.0f, 4.0f), 14.0f);
        g.setGradientFill(ColourGradient(Colour(0xff2a2b2e), 0.0f, box.getY() - 12.0f, Colour(0xff121214), 0.0f, box.getBottom() + 12.0f, false));
        g.fillRoundedRectangle(box.expanded(12.0f), 12.0f);
        g.setColour(Colours::black);
        g.fillRoundedRectangle(box.expanded(3.0f), 6.0f);
        g.setColour(col::oledBg);
        g.fillRoundedRectangle(box, 5.0f);
        g.setColour(col::legend);
        g.setFont(font(Face::title, 24.0f).withExtraKerningFactor(0.06f));
        g.drawText(pg.oledTitle, Rectangle<float>(0.0f, 14.0f, (float)TW, 30.0f), Justification::centred, false);
        g.setColour(col::oledDim);
        g.setFont(font(Face::mono, 19.0f));
        const char *head[4] = {"#", "SOURCE", "AMOUNT", "DESTINATION"};
        for (int i = 0; i < 4; i++)
            g.drawText(head[i], Rectangle<float>(OLED_COL[i], OLED_Y0 + 14.0f, i == 2 ? 170.0f : 300.0f, 30.0f),
                       i == 2 ? Justification::centred : Justification::centredLeft, false);
        g.fillRect(Rectangle<float>(OLED_X0 + 16.0f, OLED_ROW0 - 8.0f, OLED_X1 - OLED_X0 - 32.0f, 1.5f));
        int first = pg.name.endsWith("9-16") ? 9 : 1;
        g.setFont(font(Face::mono, 26.0f));
        for (int k = 0; k < 8; k++) {
            float y = OLED_ROW0 + OLED_RH * (float)k;
            g.setColour(col::oledDim);
            g.drawText(String(first + k), Rectangle<float>(OLED_COL[0], y, 60.0f, OLED_RH), Justification::centredLeft, false);
            if (k < 7) {
                g.setColour(col::oledDim.withAlpha(0.25f));
                g.fillRect(Rectangle<float>(OLED_X0 + 16.0f, y + OLED_RH - 1.0f, OLED_X1 - OLED_X0 - 32.0f, 1.0f));
            }
        }
        g.setColour(col::oledDim);
        g.setFont(font(Face::label, 15.0f));
        g.drawText("tap and drag a value, double-tap for its list", Rectangle<float>(0.0f, OLED_Y1 + 14.0f, (float)TW, 20.0f),
                   Justification::centred, false);
    }
    for (auto &s : pg.secs) {
        if (s.style == 1) {
            drawPlate(g, s.r, 14.0f, 1.0f);
            drawWide(g, s.title, Rectangle<float>(s.r.getRight() - 330.0f, s.r.getBottom() - 30.0f, 310.0f, 22.0f), 19.0f, 1.18f, col::legend,
                     Justification::right);
        } else if (s.style == 2) {
            g.setColour(col::legend);
            g.setFont(font(Face::title, 24.0f).withExtraKerningFactor(0.06f));
            g.drawText(s.title, s.r, Justification::centred, false);
        } else if (s.style == 3)
            drawLegend(g, s.title, s.r, 15.5f);
        else
            drawSection(g, s.r, s.title);
    }
    for (auto &[x, yy] : pg.vlines) {
        g.setColour(col::line.withAlpha(0.85f));
        g.fillRect(Rectangle<float>(x - 1.0f, yy.x, 2.0f, yy.y - yy.x));
    }
    g.setColour(col::legend);
    for (auto &[ir, shape] : pg.icons)
        drawWaveIcon(g, ir, shape, 2.0f);
    for (auto &c : pg.ctls) {
        if (c.kind == Ctl::cell && c.boxed) {
            auto bx = Rectangle<float>(c.x, c.y, c.size, c.nameW);
            drawMiniOled(g, bx);
            if (c.legend.isNotEmpty())
                drawLegend(g, c.legend, Rectangle<float>(bx.getCentreX() - 150.0f, bx.getBottom() + 12.0f, 300.0f, 20.0f), 15.5f);
        } else if (c.kind == Ctl::radio) {
            for (size_t i = 0; i < c.pts.size(); i++)
                drawLegend(g, c.names[(int)i], Rectangle<float>(c.pts[i].x - 80.0f, c.pts[i].y + buttonH(c) * 0.5f + 6.0f, 160.0f, 20.0f),
                           c.bh > BH ? 17.0f : 15.0f);
        } else if (c.kind == Ctl::knob && c.legend.isNotEmpty())
            drawLegend(g, c.legend, Rectangle<float>(c.x - 80.0f, c.y + c.size * 0.5f + 2.0f + c.legDy, 160.0f, 20.0f), 15.5f);
        else if (c.kind == Ctl::button && c.legend.isNotEmpty())
            drawLegend(g, c.legend, Rectangle<float>(c.x - 110.0f, c.y + BH * 0.5f + 6.0f, 220.0f, 20.0f), 15.0f);
        else if (c.kind == Ctl::leds && !c.names.isEmpty()) {
            g.setColour(col::legend);
            g.setFont(font(Face::title, 16.0f));
            for (size_t i = 0; i < c.pts.size(); i++)
                g.drawText(c.names[(int)i], Rectangle<float>(c.pts[i].x + 16.0f, c.pts[i].y - 11.0f, c.nameW, 22.0f), Justification::centredLeft, false);
        } else if (c.kind == Ctl::fixed)
            drawButton(g, Rectangle<float>(c.size, c.size * (VBH / VBW)).withCentre({c.x, c.y}), false, false, false);
    }
    /* the bottom strip: the name, the page; the maker's tag */
    drawLogo(g, Rectangle<float>(18.0f, 588.0f, 100.0f, 26.0f));
    drawLegend(g, pg.name, Rectangle<float>(140.0f, 586.0f, 300.0f, 30.0f), 16.0f, Justification::centredLeft);
    drawTag(g, Rectangle<float>(1280.0f - 18.0f - 200.0f, 628.0f - 14.0f - 24.0f, 200.0f, 24.0f));
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

float buttonH(const Ctl &c)
{
    if (c.bh > 0)
        return c.bh;
    return c.size * (c.size == BW ? BH / BW : VBH / VBW);
}

Image drawCtl(const Image &bg, const Ctl &c, Rectangle<int> r, int index, int steps, int led = -1)
{
    Image img(Image::RGB, r.getWidth(), r.getHeight(), true);
    Graphics g(img);
    g.drawImageAt(bg.getClippedImage(r), 0, 0);
    /* the control on a layer of its own, let into the background at the picture's edges (a glow or a shadow cut off
     * at them would show as a line) */
    Image layer(Image::ARGB, r.getWidth(), r.getHeight(), true);
    Rectangle<float> body;               /* the control itself (kept whole); what lies round it fades out to the edge */
    {
        Graphics lg(layer);
        lg.addTransform(AffineTransform::translation((float)-r.getX(), (float)-r.getY()));
        if (c.kind == Ctl::knob) {
            float pos = steps > 1 ? (float)index / (float)(steps - 1) : 0.0f;
            auto kr = Rectangle<float>(c.size, c.size).withCentre({c.x, c.y});
            for (size_t i = 0; i < c.kleds.size(); i++)
                drawLedFit(lg, c.kleds[i], c.ledR, (int)i == index ? 1.0f : 0.0f, c.ledR * 2.4f);
            drawKnob(lg, kr, c.kk, true);
            drawKnobPointer(lg, kr, pos, c.kk);
            body = kr;
            for (auto &lp : c.kleds)
                body = body.getUnion(Rectangle<float>(c.ledR * 1.3f, c.ledR * 1.3f).withCentre(lp));
        } else if (c.kind == Ctl::button) {
            bool lit = !c.cycle && index > 0;
            body = Rectangle<float>(c.size, buttonH(c)).withCentre({c.x, c.y});
            drawButton(lg, body, lit, false, false);
        } else if (c.kind == Ctl::radio) {
            auto p = c.pts[(size_t)led];
            body = Rectangle<float>(c.size, buttonH(c)).withCentre(p);
            drawButton(lg, body, index >= 0, false, false);
            drawLegend(lg, c.names[led], Rectangle<float>(p.x - 80.0f, p.y + buttonH(c) * 0.5f + 6.0f, 160.0f, 20.0f), c.bh > BH ? 17.0f : 15.0f);
        } else {
            auto p = c.pts[(size_t)led];
            float room = jmin(jmin(p.x - (float)r.getX(), (float)r.getRight() - p.x), jmin(p.y - (float)r.getY(), (float)r.getBottom() - p.y));
            drawLedFit(lg, p, c.ledR, index >= 0 ? 1.0f : 0.0f, room - 1.0f);
            body = Rectangle<float>(c.ledR * 1.3f, c.ledR * 1.3f).withCentre(p);
        }
    }
    body = body.translated((float)-r.getX(), (float)-r.getY());
    const int w = r.getWidth(), h = r.getHeight();
    auto along = [](float v, float lo, float b0, float b1, float hi) {   /* 1 over the body, to 0 at the edge */
        if (v < b0)
            return b0 - lo > 0.5f ? jlimit(0.0f, 1.0f, (v - lo) / (b0 - lo)) : 1.0f;
        if (v > b1)
            return hi - b1 > 0.5f ? jlimit(0.0f, 1.0f, (hi - v) / (hi - b1)) : 1.0f;
        return 1.0f;
    };
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++) {
            float a = jmin(along((float)x + 0.5f, 0.0f, body.getX(), body.getRight(), (float)w),
                           along((float)y + 0.5f, 0.0f, body.getY(), body.getBottom(), (float)h));
            if (a < 1.0f) {
                a = a * a * (3.0f - 2.0f * a);
                auto px = layer.getPixelAt(x, y);
                layer.setPixelAt(x, y, px.withAlpha(px.getFloatAlpha() * a));
            }
        }
    g.drawImageAt(layer, 0, 0);
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
    /* the VST's panel: its own paint, its controls left out */
    {
        Fake5Processor proc;
        Fake5Panel panel(proc);
        panel.setSize(Fake5Panel::W, Fake5Panel::H);
        for (auto *c : panel.getChildren())
            c->setVisible(false);
        vstPanel = panel.createComponentSnapshot(panel.getLocalBounds(), true, 2.0f);
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
        /* what takes touches where: no two may overlap (a knob's round face may have a small one in its picture's
         * corner: make_skin.py puts the small ones on top) */
        struct Hit {
            String id;
            Rectangle<int> r;
            bool knob;
            Point<float> c;
            float rad;
        };
        std::vector<Hit> hits;
        std::vector<Point<float>> allLeds;
        for (auto &c : pg.ctls)
            if (c.kind == Ctl::leds)
                for (auto &p : c.pts)
                    allLeds.push_back(p);
        for (auto &c : pg.ctls) {
            if (c.kind == Ctl::fixed)
                continue;
            if (c.kind == Ctl::cell) {
                auto rg0 = rangeOf(c.id);
                auto *co = new DynamicObject();
                co->setProperty("id", c.id);
                co->setProperty("steps", rg0.hi - rg0.lo + 1);
                Array<var> nm;
                for (auto &t : rg0.names)
                    nm.add(t);
                co->setProperty("names", nm);
                co->setProperty("kind", "cell");
                co->setProperty("rect", rectVar(Rectangle<float>(c.x, c.y, c.size, c.nameW).toNearestInt()));
                hits.push_back({c.id, Rectangle<float>(c.x, c.y, c.size, c.nameW).toNearestInt(), false, {}, 0});
                co->setProperty("centred", c.cycle);
                co->setProperty("bipolar", bipolarOf(c.id));
                list.add(var(co));
                continue;
            }
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
                auto rf = Rectangle<float>(c.size + 6.0f, c.size + 6.0f).withCentre({c.x, c.y});
                for (auto &p : c.kleds)
                    rf = rf.getUnion(Rectangle<float>(c.ledR * 5.0f, c.ledR * 5.0f).withCentre(p));
                auto r = rf.toNearestInt();
                hits.push_back({c.id, r, true, {c.x, c.y}, c.size * 0.5f});
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
                auto fr = r;
                if (c.legend.isNotEmpty())
                    fr = r.getUnion(Rectangle<float>(c.x - 66.0f, c.y + c.size * 0.5f + c.legDy, 132.0f, 24.0f).toNearestInt());
                o->setProperty("kind", "knob");
                o->setProperty("bipolar", bipolarOf(c.id));
                o->setProperty("rect", rectVar(r));
                o->setProperty("frame", rectVar(fr));
                o->setProperty("frames", frames);
                o->setProperty("strip", base + "_knob.png");
            } else if (c.kind == Ctl::button) {
                auto r = Rectangle<float>(c.size + 4.0f, buttonH(c) + 4.0f).withCentre({c.x, c.y}).toNearestInt();
                hits.push_back({c.id, r, false, {}, 0});
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
            } else if (c.kind == Ctl::radio) {
                /* a button a value: the LED rows' way (a picture lit and unlit each, the value its press gives), the
                 * button and its name the place to touch */
                o->setProperty("kind", "leds");
                Array<var> rows;
                for (size_t k = 0; k < c.pts.size(); k++) {
                    auto p = c.pts[k];
                    float w = c.size + 24.0f, h = buttonH(c);
                    auto r = Rectangle<float>(p.x - w * 0.5f, p.y - h * 0.5f - 4.0f, w, h + 32.0f).toNearestInt();
                    hits.push_back({c.id, r, false, {}, 0});
                    auto *ro = new DynamicObject();
                    ro->setProperty("rect", rectVar(r));
                    ro->setProperty("value", c.vals[k]);
                    String on = base + "_btn" + String((int)k) + "_on.png", off = base + "_btn" + String((int)k) + "_off.png";
                    writePng(drawCtl(bg, c, r, 1, steps, (int)k), dir.getChildFile(on));
                    writePng(drawCtl(bg, c, r, -1, steps, (int)k), dir.getChildFile(off));
                    ro->setProperty("on", on);
                    ro->setProperty("off", off);
                    rows.add(var(ro));
                }
                o->setProperty("rows", rows);
            } else {
                o->setProperty("kind", "leds");
                Array<var> rows;
                for (size_t k = 0; k < c.pts.size(); k++) {
                    float hw = c.ledR * 1.6f + 4.0f;
                    for (auto &q : allLeds)          /* (no closer than half way to the next LED) */
                        if (q != c.pts[k])
                            hw = jmin(hw, q.getDistanceFrom(c.pts[k]) * 0.5f - 0.5f);
                    auto rr = Rectangle<float>(c.pts[k].x - hw, c.pts[k].y - hw, hw * 2.0f, hw * 2.0f);
                    if (!c.names.isEmpty())   /* (its name with it: a bigger place to touch) */
                        rr = rr.withRight(c.pts[k].x + 16.0f + c.nameW);
                    auto r = rr.getSmallestIntegerContainer().reduced(1);
                    hits.push_back({c.id, r, false, {}, 0});
                    auto *ro = new DynamicObject();
                    ro->setProperty("rect", rectVar(r));
                    ro->setProperty("value", c.vals[k]);
                    String on = base + "_led" + String((int)k) + "_on.png", off = base + "_led" + String((int)k) + "_off.png";
                    writePng(drawCtl(bg, c, r, 1, steps, (int)k), dir.getChildFile(on));
                    writePng(drawCtl(bg, c, r, -1, steps, (int)k), dir.getChildFile(off));
                    ro->setProperty("on", on);
                    ro->setProperty("off", off);
                    rows.add(var(ro));
                }
                o->setProperty("rows", rows);
            }
            list.add(var(o));
        }
        int clashes = 0;
        for (size_t i = 0; i < hits.size(); i++)
            for (size_t j = i + 1; j < hits.size(); j++) {
                auto &a = hits[i], &b = hits[j];
                if (!a.r.intersects(b.r))
                    continue;
                bool bad = true;
                if (a.knob != b.knob) {          /* a small one in a knob picture's corner: off its face */
                    auto &k = a.knob ? a : b, &o2 = a.knob ? b : a;
                    auto rr = o2.r.toFloat();
                    auto np = Point<float>(jlimit(rr.getX(), rr.getRight(), k.c.x), jlimit(rr.getY(), rr.getBottom(), k.c.y));
                    bad = np.getDistanceFrom(k.c) < k.rad;
                }
                if (bad) {
                    std::fprintf(stderr, "%s: %s and %s overlap\n", pg.name.toRawUTF8(), a.id.toRawUTF8(), b.id.toRawUTF8());
                    clashes++;
                }
            }
        if (clashes)
            return 1;
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
