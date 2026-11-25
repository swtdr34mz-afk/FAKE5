/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (FAKE5) */
#include "Params.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace f5
{
const char *const LFOSHAPE_NAME[NLFOSHAPE] = {"Triangle", "Sawtooth", "Rev Saw", "Square", "S&H"};
static const char *const ONOFF[] = {"Off", "On"};
static const char *const GLIDEMODE[] = {"Fixed Rate", "Fixed Rate A", "Fixed Time", "Fixed Time A"};
static const char *const KEYMODE[] = {"Low", "High", "Last"};
static const char *const ARPMODE[] = {"Up", "Down", "Up+Down", "Random", "Assign"};
const char *const ENVROUTING_NAME[NENVROUTING] = {"Filter / Amp", "Aux / Filter+Amp", "Aux / Filter+Gate"};
static const char *const ENVREPEAT[] = {"Off", "Env 1", "Env 2", "Env 1 + Env 2"};
const char *const FXTYPE_NAME[NFXTYPE] = {"Delay Stereo", "BBD Delay", "Tape Delay 1", "Tape Delay 2",
                                          "Chorus",       "Flanger",   "Phaser",       "HP Filter",
                                          "Distortion",   "Ring Mod",  "Rotating Spkr", "Lo-Fi"};
const char *const FXTYPE_SHORT[NFXTYPE] = {"Delay", "BBD", "Tape 1", "Tape 2", "Chorus", "Flanger",
                                           "Phaser", "HPF", "Dist", "RingMod", "Rotary", "Lo-Fi"};
const char *const FXKNOB_NAME[NFXTYPE][3] = {
    {"Time", "Mix", "Feedback"}, {"Time", "Mix", "Feedback"}, {"Time", "Mix", "Feedback"}, {"Time", "Mix", "Feedback"},
    {"Rate", "Depth", "LPF"},    {"Rate", "Depth", "Feedback"}, {"Rate", "Depth", "Feedback"}, {"Freq", "Mix", "Resonance"},
    {"Gain", "Level", "Tone"},   {"Carrier", "Mix", "Key Track"}, {"Speed", "Drive", "Mic Dist"}, {"Wow/Flutter", "Depth", "Drive"}};
const char *const DIVIDE_NAME[NDIVIDE] = {"1/2", "1/4", "1/8", "1/8 T", "1/16", "1/16 T", "1/32", "64th"};
/* a synced LFO's cycle, from four bars down to a 32nd */
const char *const LFOSYNC_NAME[16] = {"4 Bars", "2 Bars", "1 Bar", "1/2 Dot", "1/2", "1/4 Dot", "1/2 T", "1/4",
                                      "1/8 Dot", "1/4 T", "1/8", "1/16 Dot", "1/8 T", "1/16", "1/16 T", "1/32"};
const char *const DLYSYNC_NAME[11] = {"1/1", "1/2 Dot", "1/2", "1/4 Dot", "1/4", "1/4 T", "1/8 Dot", "1/8", "1/8 T",
                                      "1/16 Dot", "1/16"};
const char *const MODSRC_NAME[NMODSRC] = {"No Source", "Osc 2",    "Noise",       "LFO 1",       "LFO 2",
                                          "Env 1",     "Env 2",    "Voice Spread", "Pitch Bend",  "Mod Wheel",
                                          "Pressure",  "Breath",   "Foot Pedal",  "Expression",  "Velocity",
                                          "Note Number", "Filter Out", "Random",   "DC",          "Audio Out"};
const bool MODSRC_POLY[NMODSRC] = {false, true, true,  false, true,  true, true, true,  false, false,
                                   false, false, false, false, true, true, true, true, false, false};
const char *const MODDST_NAME[NMODDST] = {
    "No Dest",      "Osc 1 Octave",  "Osc 2 Octave",  "Osc 1 Fine",    "Osc 2 Fine",    "Osc All Fine",
    "Osc 1 Shape",  "Osc 2 Shape",   "Osc All Shape", "Osc 1 Level",   "Osc 2 Level",   "Osc FM Level",
    "Sub Level",    "Noise",         "Drive",         "Cutoff",        "Resonance",     "Vintage",
    "Overdrive",    "Panning",       "FX Mix",        "FX Time",       "FX Misc",       "Reverb Mix",
    "Reverb Size",  "Reverb PreDly", "Reverb Decay",  "Reverb Tone",   "LFO 1 Freq",    "LFO 2 Freq",
    "LFO All Freq", "LFO 1 Amt",     "LFO 2 Amt",     "LFO All Amt",   "Env 1 Amt",     "Env 2 Amt",
    "Env 1 Attack", "Env 2 Attack",  "Env 1 Decay",   "Env 2 Decay",   "Env 1 Release", "Env 2 Release",
    "Mod 1 Amount", "Mod 2 Amount",  "Mod 3 Amount",  "Mod 4 Amount",  "Mod 5 Amount",  "Mod 6 Amount",
    "Mod 7 Amount", "Mod 8 Amount",  "Mod 9 Amount",  "Mod 10 Amount", "Mod 11 Amount", "Mod 12 Amount",
    "Mod 13 Amount", "Mod 14 Amount", "Mod 15 Amount", "Mod 16 Amount"};
const bool MODDST_POLY[NMODDST] = {
    false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false,
    false, true, false, false, false, false, false, false, false, false, false, true, true, false, true, true, true,
    true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true,
    true, true, true, true};

#define N(id, name, lo, hi, def) {id, name, lo, hi, def, K_NUM, nullptr}
#define S(id, name, lo, hi, def) {id, name, lo, hi, def, K_SIGNED, nullptr}
#define C(id, name, n, def, list) {id, name, 0, (n) - 1, def, K_CHOICE, list}
#define B(id, name, def) {id, name, 0, 1, def, K_ONOFF, ONOFF}
#define LFO(n, dest)                                                                                                    \
    {"lfo" #n "_freq", "LFO " #n " Frequency", 0, 127, 64, K_LFOFREQ, nullptr},                                         \
        S("lfo" #n "_amt", "LFO " #n " Amount", -127, 127, 0),                                                          \
        C("lfo" #n "_shape", "LFO " #n " Shape", NLFOSHAPE, 0, LFOSHAPE_NAME),                                          \
        B("lfo" #n "_sync", "LFO " #n " Clock Sync", 0),                                                                \
        C("lfo" #n "_dest", "LFO " #n " Destination", NMODDST, dest, MODDST_NAME),                                      \
        B("lfo" #n "_reset", "LFO " #n " Reset", 0), N("lfo" #n "_slew", "LFO " #n " Slew", 0, 127, 0)
#define MOD(n)                                                                                                          \
    C("mod" #n "_src", "Mod " #n " Source", NMODSRC, 0, MODSRC_NAME), S("mod" #n "_amt", "Mod " #n " Amount", -127, 127, 0), \
        C("mod" #n "_dst", "Mod " #n " Destination", NMODDST, 0, MODDST_NAME)

const ParamDef PARAMS[NPARAM] = {
    S("o1_oct", "Osc 1 Octave", -2, 2, 0),
    {"o1_pitch", "Osc 1 Fine Freq", -700, 700, 0, K_CENTS, nullptr},
    N("o1_shape", "Osc 1 Shape", 0, 127, 64),
    B("sync", "Sync 2>1", 0),
    S("o2_oct", "Osc 2 Octave", -2, 2, 0),
    {"o2_pitch", "Osc 2 Fine Freq", -700, 700, 0, K_CENTS, nullptr},
    N("o2_shape", "Osc 2 Shape", 0, 127, 64),
    B("o1_key", "Osc 1 Key Follow", 1),
    B("o2_key", "Osc 2 Key Follow", 1),
    B("o2_bypass", "Osc 2 Filter Bypass", 0),
    N("o1_level", "Osc 1 Level", 0, 127, 127),
    N("sub", "Osc 1 Sub Level", 0, 127, 0),
    N("o2_level", "Osc 2 Level", 0, 127, 0),
    B("fm", "FM 2>1", 0),
    S("noise", "Noise Level", -127, 127, 0),
    N("cutoff", "Cutoff", 0, 1023, 1023),
    N("reso", "Resonance", 0, 127, 0),
    N("drive", "Drive", 0, 127, 0),
    N("keyamt", "Key>LPF Freq", 0, 127, 0),
    N("e1_del", "Env 1 Delay", 0, 127, 0),
    N("e1_att", "Env 1 Attack", 0, 127, 0),
    N("e1_dec", "Env 1 Decay", 0, 127, 60),
    N("e1_sus", "Env 1 Sustain", 0, 127, 0),
    N("e1_rel", "Env 1 Release", 0, 127, 45),
    S("e1_amt", "Env 1 Amount", -127, 127, 0),
    B("e1_vel", "Env 1 Velocity", 0),
    C("e1_dest", "Env 1 Aux Dest", NMODDST, MD_OSC2_FINE, MODDST_NAME),
    N("e2_del", "Env 2 Delay", 0, 127, 0),
    N("e2_att", "Env 2 Attack", 0, 127, 0),
    N("e2_dec", "Env 2 Decay", 0, 127, 0),
    N("e2_sus", "Env 2 Sustain", 0, 127, 127),
    N("e2_rel", "Env 2 Release", 0, 127, 45),
    S("e2_amt", "Env 2 Amount", -127, 127, 127),
    B("e2_vel", "Env 2 Velocity", 0),
    C("env_routing", "Env Routing", NENVROUTING, ER_FILTER_AMP, ENVROUTING_NAME),
    C("env_repeat", "Envelope Repeat", NENVREPEAT, RP_OFF, ENVREPEAT),
    LFO(1, MD_OSC_FINE),
    LFO(2, MD_CUTOFF),
    MOD(1), MOD(2), MOD(3), MOD(4), MOD(5), MOD(6), MOD(7), MOD(8),
    MOD(9), MOD(10), MOD(11), MOD(12), MOD(13), MOD(14), MOD(15), MOD(16),
    B("fx_on", "Effect On", 0),
    C("fx_type", "Effect Type", NFXTYPE, FX_BBD, FXTYPE_NAME),
    {"fx_time", "Effect Time", 0, 127, 50, K_FXTIME, nullptr},
    N("fx_mix", "Effect Depth/Mix", 0, 127, 40),
    N("fx_misc", "Effect Fback/Misc", 0, 127, 50),
    B("fx_sync", "Effect Clock Sync", 0),
    B("rev_on", "Reverb On", 0),
    N("rev_mix", "Reverb Mix", 0, 127, 40),
    N("rev_damp", "Reverb Damping", 0, 127, 50),
    N("rev_predelay", "Reverb Pre-Delay", 0, 127, 20),
    N("rev_decay", "Reverb Decay", 0, 127, 70),
    S("rev_tone", "Reverb Tone", -64, 63, 0),
    N("vintage", "Vintage", 0, 127, 0),
    N("overdrive", "Overdrive", 0, 127, 0),
    B("glide", "Glide", 0),
    N("glide_rate", "Glide Rate", 0, 127, 40),
    C("glide_mode", "Glide Mode", NGLIDEMODE, GM_RATE, GLIDEMODE),
    N("o1_glide", "Osc 1 Glide", 0, 127, 127),
    N("o2_glide", "Osc 2 Glide", 0, 127, 127),
    B("unison", "Unison", 0),
    {"uni_voices", "Unison Voices", 1, 10, 5, K_UNIVOICES, nullptr},
    N("uni_detune", "Unison Detune", 0, 7, 3),
    C("uni_keymode", "Unison Key Mode", NKEYMODE, KM_LAST, KEYMODE),
    B("uni_retrig", "Unison Env Retrig", 1),
    N("bpm", "BPM", 30, 250, 120),
    C("divide", "Clock Divide", NDIVIDE, DV_16, DIVIDE_NAME),
    B("arp_on", "Arp On", 0),
    C("arp_mode", "Arp Mode", NARPMODE, AM_UP, ARPMODE),
    N("arp_oct", "Arp Octave", 1, 3, 1),
    N("arp_repeat", "Arp Repeat", 0, 3, 0),
    B("arp_relatch", "Arp Relatch", 0),
    B("seq_play", "Seq Play", 0),
    N("pgm_vol", "Program Volume", 0, 127, 100),
    N("pb_up", "Pitch Bend Up", 0, 12, 2),
    N("pb_down", "Pitch Bend Down", 0, 24, 2),
    B("split1", "Low Split -1 Oct", 0),
    B("split2", "Low Split -2 Oct", 0),
    {"split_point", "Low Split Point", 0, 127, 60, K_NOTE, nullptr},
};

static const char *const NOTE12[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

const char *fxKnobName(int t, int k)
{
    if (t < 0 || t >= NFXTYPE || k < 0 || k > 2)
        return "";
    return FXKNOB_NAME[t][k];
}

void paramText(int p, int v, char *out, int n, bool synced, int fxType)
{
    const ParamDef &d = PARAMS[p];
    switch (d.kind) {
    case K_SIGNED:
        if (p == P_NOISE)
            std::snprintf(out, (size_t)n, v < 0 ? "Pink %d" : v > 0 ? "White %d" : "0", std::abs(v));
        else if (p == P_REV_TONE)
            std::snprintf(out, (size_t)n, v < 0 ? "LPF %d" : v > 0 ? "HPF %d" : "Flat", std::abs(v));
        else
            std::snprintf(out, (size_t)n, v > 0 ? "+%d" : "%d", v);
        break;
    case K_CHOICE:
    case K_ONOFF: std::snprintf(out, (size_t)n, "%s", d.choice[v < d.lo ? d.lo : v > d.hi ? d.hi : v]); break;
    case K_NOTE: std::snprintf(out, (size_t)n, "%s%d", NOTE12[((v % 12) + 12) % 12], v / 12 - 2); break;
    case K_CENTS: {
        int a = std::abs(v);
        std::snprintf(out, (size_t)n, "%s%d.%02d", v < 0 ? "-" : v > 0 ? "+" : "", a / 100, a % 100);
        break;
    }
    case K_LFOFREQ:
        if (synced)
            std::snprintf(out, (size_t)n, "%s", LFOSYNC_NAME[v * 16 / 128]);
        else
            std::snprintf(out, (size_t)n, "%d", v);
        break;
    case K_FXTIME:
        if (synced && fxType <= FX_TAPE2)
            std::snprintf(out, (size_t)n, "%s", DLYSYNC_NAME[v * 11 / 128]);
        else
            std::snprintf(out, (size_t)n, "%d", v);
        break;
    case K_UNIVOICES: std::snprintf(out, (size_t)n, v == 1 ? "1 Voice" : "%d Voices", v); break;
    default: std::snprintf(out, (size_t)n, "%d", v);
    }
}

void seqClear(SeqData &s) { std::memset(&s, 0, sizeof s); }

int chordCount(const Program &p)
{
    int n = 0;
    for (int8_t c : p.chord)
        n += c >= 0;
    return n;
}

void programInit(Program &p)
{
    std::memset(&p, 0, sizeof p);
    std::snprintf(p.name, NAME_LEN, "Basic Program");
    for (int i = 0; i < NPARAM; i++)
        p.p[i] = (int16_t)PARAMS[i].def;
    /* the Basic Program's wheel: the mod wheel opens LFO 1's vibrato */
    p.p[P_MOD1 + MOD_SRC] = MS_MODWHEEL;
    p.p[P_MOD1 + MOD_AMT] = 40;
    p.p[P_MOD1 + MOD_DST] = MD_LFO1_AMT;
    seqClear(p.seq);
    for (auto &c : p.chord)
        c = -1;
}
} // namespace f5
