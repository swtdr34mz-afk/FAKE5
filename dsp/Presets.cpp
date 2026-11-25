/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (FAKE5) */
/* FAKE5's factory programs: three banks of sixteen, made for FAKE5 (its own sounds, in the spirit of the Take 5's:
 * warm Prophet-style polys, the FM section's bells and pianos, the plate on almost everything), then the BASIC
 * PROGRAM. Times in the Take 5's units: an envelope's 60 is about 90 ms, 80 half a second, 100 three seconds; CUTOFF
 * 500 about 650 Hz, 700 2.6 kHz, 900 10 kHz. */
#include "Presets.h"
#include <cstdio>
#include <initializer_list>
#include <utility>
#include <vector>

namespace f5
{
namespace
{
using KV = std::pair<int, int>;
struct Mod {
    int src, amt, dst;
};
struct Def {
    const char *name;
    std::vector<KV> kv;
    std::vector<Mod> mods = {};
    std::vector<std::vector<int>> seq = {};   /* steps: notes (a step {} a rest, {-1} a tie), all at velocity 100 */
    std::vector<int> chord = {};
};

Program make(const Def &d)
{
    Program p;
    programInit(p);
    std::snprintf(p.name, NAME_LEN, "%s", d.name);
    for (auto &kv : d.kv)
        p.p[kv.first] = (int16_t)kv.second;
    for (int k = 0; k < NMOD; k++) {                   /* the Basic Program's mod wheel goes unless said again */
        p.p[P_MOD1 + k * MOD_N + MOD_SRC] = MS_OFF;
        p.p[P_MOD1 + k * MOD_N + MOD_AMT] = 0;
        p.p[P_MOD1 + k * MOD_N + MOD_DST] = MD_OFF;
    }
    int k = 0;
    for (auto &m : d.mods) {
        p.p[P_MOD1 + k * MOD_N + MOD_SRC] = (int16_t)m.src;
        p.p[P_MOD1 + k * MOD_N + MOD_AMT] = (int16_t)m.amt;
        p.p[P_MOD1 + k * MOD_N + MOD_DST] = (int16_t)m.dst;
        k++;
    }
    int s = 0;
    for (auto &st : d.seq) {
        if (s >= SEQ_STEPS)
            break;
        int n = 0;
        for (int note : st) {
            if (n >= SEQ_NOTES)
                break;
            p.seq.note[s][n] = note < 0 ? PN_TIE : (uint8_t)(note + 2);
            p.seq.vel[s][n] = 100;
            n++;
        }
        s++;
    }
    p.seq.length = (uint8_t)s;
    for (int i = 0; i < CHORD_N && i < (int)d.chord.size(); i++)
        p.chord[i] = (int8_t)d.chord[(size_t)i];
    return p;
}

/* the wheel's vibrato, the pressure's brightness: most programs have them */
const Mod VIB{MS_MODWHEEL, 34, MD_LFO1_AMT};
const Mod PRS{MS_PRESSURE, 30, MD_CUTOFF};

std::vector<Def> defs()
{
    std::vector<Def> d;
    /* ======================================================================= BANK 1 === */
    d.push_back({"Golden Coast",
                 {{P_O1_SHAPE, 60}, {P_O2_SHAPE, 66}, {P_O2_PITCH, 7}, {P_O2_LEVEL, 112}, {P_SUB, 40}, {P_CUTOFF, 610},
                  {P_RESO, 34}, {P_E1_ATT, 0}, {P_E1_DEC, 72}, {P_E1_SUS, 30}, {P_E1_REL, 70}, {P_E1_AMT, 46}, {P_E1_VEL, 1},
                  {P_E2_DEC, 76}, {P_E2_SUS, 70}, {P_E2_REL, 74}, {P_ARP_ON, 1}, {P_ARP_MODE, AM_UPDOWN}, {P_ARP_OCT, 2},
                  {P_DIVIDE, DV_16}, {P_BPM, 112}, {P_FX_ON, 1}, {P_FX_TYPE, FX_BBD}, {P_FX_SYNC, 1}, {P_FX_TIME, 75},
                  {P_FX_MIX, 46}, {P_FX_MISC, 58}, {P_REV_ON, 1}, {P_REV_MIX, 46}, {P_REV_DECAY, 78}, {P_REV_DAMP, 48},
                  {P_REV_PREDELAY, 26}, {P_VINTAGE, 30}, {P_KEYAMT, 50}},
                 {VIB, PRS}});
    d.push_back({"Orchestra Hit",
                 {{P_O1_SHAPE, 64}, {P_O2_SHAPE, 64}, {P_O2_OCT, 1}, {P_O2_PITCH, -6}, {P_O2_LEVEL, 100}, {P_SUB, 70},
                  {P_NOISE, 18}, {P_CUTOFF, 470}, {P_E1_DEC, 70}, {P_E1_SUS, 20}, {P_E1_REL, 72}, {P_E1_AMT, 70},
                  {P_E1_VEL, 1}, {P_E2_ATT, 8}, {P_E2_DEC, 78}, {P_E2_SUS, 40}, {P_E2_REL, 78}, {P_E2_VEL, 1},
                  {P_REV_ON, 1}, {P_REV_MIX, 54}, {P_REV_DECAY, 88}, {P_REV_PREDELAY, 34}, {P_REV_TONE, -10},
                  {P_VINTAGE, 24}, {P_UNISON, 1}, {P_UNI_VOICES, 5}, {P_UNI_DETUNE, 4}},
                 {VIB, PRS},
                 {},
                 {0, 7, 12, 16}});
    d.push_back({"Sync Sweep Lead",
                 {{P_O1_OCT, 1}, {P_O1_SHAPE, 64}, {P_SYNC, 1}, {P_O2_SHAPE, 60}, {P_O2_LEVEL, 60}, {P_CUTOFF, 760},
                  {P_RESO, 20}, {P_ENV_ROUTING, ER_AUX_FILTERAMP}, {P_E1_DEST, MD_OSC1_FINE}, {P_E1_AMT, 92},
                  {P_E1_DEC, 82}, {P_E1_SUS, 22}, {P_E1_REL, 60}, {P_E2_DEC, 80}, {P_E2_SUS, 100}, {P_E2_REL, 60},
                  {P_E2_AMT, 24}, {P_UNISON, 1}, {P_UNI_VOICES, 3}, {P_UNI_DETUNE, 3}, {P_GLIDE, 1}, {P_GLIDE_RATE, 46},
                  {P_FX_ON, 1}, {P_FX_TYPE, FX_DELAY}, {P_FX_SYNC, 1}, {P_FX_TIME, 64}, {P_FX_MIX, 36}, {P_FX_MISC, 48},
                  {P_REV_ON, 1}, {P_REV_MIX, 36}, {P_REV_DECAY, 76}, {P_OVERDRIVE, 22}},
                 {VIB, {MS_MODWHEEL, 60, MD_OSC1_FINE}, PRS}});
    d.push_back({"Velvet Tines",
                 /* FM 2>1 at 1:1, ENV 1 bringing the index in and out: a tine's bark */
                 {{P_O1_SHAPE, 0}, {P_O2_SHAPE, 0}, {P_FM, 1}, {P_O2_LEVEL, 26}, {P_O2_PITCH, 3}, {P_SUB, 0},
                  {P_CUTOFF, 900}, {P_ENV_ROUTING, ER_AUX_FILTERAMP}, {P_E1_DEST, MD_FM_LEVEL}, {P_E1_AMT, 66},
                  {P_E1_VEL, 1}, {P_E1_DEC, 74}, {P_E1_SUS, 0}, {P_E1_REL, 66}, {P_E2_DEC, 98}, {P_E2_SUS, 0},
                  {P_E2_REL, 74}, {P_E2_AMT, 0}, {P_E2_VEL, 1}, {P_FX_ON, 1}, {P_FX_TYPE, FX_CHORUS}, {P_FX_TIME, 40},
                  {P_FX_MIX, 60}, {P_FX_MISC, 90}, {P_REV_ON, 1}, {P_REV_MIX, 40}, {P_REV_DECAY, 74}, {P_REV_DAMP, 60},
                  {P_REV_PREDELAY, 18}, {P_VINTAGE, 16}, {P_KEYAMT, 64}},
                 {{MS_MODWHEEL, 40, MD_LFO2_AMT}, {MS_VELOCITY, 22, MD_FM_LEVEL}},
                 {}});
    d.push_back({"Punch Bass",
                 {{P_O1_OCT, -1}, {P_O1_SHAPE, 64}, {P_O2_OCT, -1}, {P_O2_SHAPE, 74}, {P_O2_PITCH, 8}, {P_O2_LEVEL, 96},
                  {P_SUB, 110}, {P_CUTOFF, 330}, {P_RESO, 58}, {P_DRIVE, 40}, {P_E1_DEC, 64}, {P_E1_SUS, 8}, {P_E1_REL, 40},
                  {P_E1_AMT, 70}, {P_E1_VEL, 1}, {P_E2_DEC, 70}, {P_E2_SUS, 96}, {P_E2_REL, 38}, {P_KEYAMT, 64},
                  {P_UNISON, 1}, {P_UNI_VOICES, 2}, {P_UNI_DETUNE, 1}, {P_UNI_RETRIG, 1}, {P_OVERDRIVE, 30},
                  {P_REV_ON, 0}},
                 {PRS}});
    d.push_back({"FM Bells",
                 /* osc 2 at 2.5 times the carrier: inharmonic, a bell; the index falls with ENV 1 */
                 {{P_O1_SHAPE, 0}, {P_O2_SHAPE, 0}, {P_FM, 1}, {P_O2_OCT, 1}, {P_O2_PITCH, 386}, {P_O2_LEVEL, 50},
                  {P_CUTOFF, 1000}, {P_ENV_ROUTING, ER_AUX_FILTERAMP}, {P_E1_DEST, MD_FM_LEVEL}, {P_E1_AMT, 54},
                  {P_E1_VEL, 1}, {P_E1_DEC, 92}, {P_E1_SUS, 0}, {P_E1_REL, 90}, {P_E2_DEC, 104}, {P_E2_SUS, 0},
                  {P_E2_REL, 100}, {P_E2_AMT, 0}, {P_REV_ON, 1}, {P_REV_MIX, 58}, {P_REV_DECAY, 96}, {P_REV_DAMP, 30},
                  {P_REV_PREDELAY, 40}, {P_VINTAGE, 10}},
                 {{MS_MODWHEEL, 50, MD_LFO2_AMT}, {MS_VELOCITY, 20, MD_OSC2_LEVEL}},
                 {}});
    d.push_back({"Small Hours",
                 {{P_O1_SHAPE, 50}, {P_O2_SHAPE, 56}, {P_O2_PITCH, -9}, {P_O2_LEVEL, 118}, {P_SUB, 30}, {P_NOISE, -10},
                  {P_CUTOFF, 520}, {P_RESO, 26}, {P_E1_ATT, 96}, {P_E1_DEC, 100}, {P_E1_SUS, 60}, {P_E1_REL, 98},
                  {P_E1_AMT, 30}, {P_E2_ATT, 92}, {P_E2_DEC, 90}, {P_E2_SUS, 110}, {P_E2_REL, 98}, {P_LFO1 + LFO_FREQ, 36},
                  {P_LFO1 + LFO_AMT, 18}, {P_LFO1 + LFO_DEST, MD_CUTOFF}, {P_LFO1 + LFO_N + LFO_FREQ, 46},
                  {P_LFO1 + LFO_N + LFO_AMT, 26}, {P_LFO1 + LFO_N + LFO_DEST, MD_OSC_SHAPE}, {P_REV_ON, 1},
                  {P_REV_MIX, 70}, {P_REV_DECAY, 108}, {P_REV_DAMP, 58}, {P_REV_PREDELAY, 50}, {P_REV_TONE, -14},
                  {P_FX_ON, 1}, {P_FX_TYPE, FX_CHORUS}, {P_FX_TIME, 24}, {P_FX_MIX, 70}, {P_FX_MISC, 70}, {P_VINTAGE, 64}},
                 {VIB, PRS}});
    d.push_back({"Starfall",
                 {{P_KEYAMT, 100}, {P_E2_DEC, 72}, {P_O1_OCT, 1}, {P_O1_SHAPE, 20}, {P_O2_OCT, 2}, {P_O2_SHAPE, 90}, {P_O2_LEVEL, 70}, {P_CUTOFF, 600},
                  {P_RESO, 70}, {P_E1_DEC, 58}, {P_E1_SUS, 0}, {P_E1_REL, 58}, {P_E1_AMT, 60},
                  {P_E2_SUS, 0}, {P_E2_REL, 66}, {P_LFO1 + LFO_SHAPE, LS_SH}, {P_LFO1 + LFO_SYNC, 1},
                  {P_LFO1 + LFO_FREQ, 104}, {P_LFO1 + LFO_AMT, 52}, {P_LFO1 + LFO_DEST, MD_CUTOFF}, {P_ARP_ON, 1},
                  {P_ARP_MODE, AM_RANDOM}, {P_ARP_OCT, 3}, {P_DIVIDE, DV_16}, {P_BPM, 100}, {P_FX_ON, 1},
                  {P_FX_TYPE, FX_DELAY}, {P_FX_SYNC, 1}, {P_FX_TIME, 76}, {P_FX_MIX, 52}, {P_FX_MISC, 70}, {P_REV_ON, 1},
                  {P_REV_MIX, 64}, {P_REV_DECAY, 100}, {P_REV_DAMP, 20}, {P_REV_TONE, 10}},
                 {VIB}});
    d.push_back({"Mirror Lake",
                 {{P_O1_SHAPE, 100}, {P_O2_SHAPE, 30}, {P_O2_PITCH, 4}, {P_O2_LEVEL, 100}, {P_CUTOFF, 610}, {P_RESO, 30},
                  {P_E1_ATT, 80}, {P_E1_DEC, 96}, {P_E1_SUS, 50}, {P_E1_REL, 94}, {P_E1_AMT, 26}, {P_E2_ATT, 70},
                  {P_E2_DEC, 90}, {P_E2_SUS, 110}, {P_E2_REL, 96}, {P_LFO1 + LFO_N + LFO_FREQ, 40},
                  {P_LFO1 + LFO_N + LFO_AMT, 36}, {P_LFO1 + LFO_N + LFO_DEST, MD_OSC1_SHAPE}, {P_LFO1 + LFO_FREQ, 30},
                  {P_LFO1 + LFO_AMT, 30}, {P_LFO1 + LFO_DEST, MD_PAN}, {P_REV_ON, 1}, {P_REV_MIX, 76}, {P_REV_DECAY, 112},
                  {P_REV_DAMP, 40}, {P_REV_PREDELAY, 60}, {P_VINTAGE, 40}},
                 {VIB, PRS, {MS_SPREAD, 50, MD_PAN}}});
    d.push_back({"Brushed Keys",
                 {{P_O1_SHAPE, 30}, {P_O2_SHAPE, 112}, {P_O2_OCT, 1}, {P_O2_LEVEL, 44}, {P_CUTOFF, 580}, {P_RESO, 12},
                  {P_E1_DEC, 84}, {P_E1_SUS, 10}, {P_E1_REL, 70}, {P_E1_AMT, 48}, {P_E1_VEL, 1}, {P_E2_DEC, 94},
                  {P_E2_SUS, 20}, {P_E2_REL, 74}, {P_E2_VEL, 1}, {P_KEYAMT, 70}, {P_FX_ON, 1}, {P_FX_TYPE, FX_PHASER},
                  {P_FX_TIME, 30}, {P_FX_MIX, 50}, {P_FX_MISC, 40}, {P_REV_ON, 1}, {P_REV_MIX, 38}, {P_REV_DECAY, 70},
                  {P_VINTAGE, 20}},
                 {VIB, PRS}});
    d.push_back({"Night Drive",
                 {{P_KEYAMT, 80}, {P_O1_SHAPE, 64}, {P_O2_SHAPE, 64}, {P_O2_OCT, -1}, {P_O2_LEVEL, 90}, {P_CUTOFF, 420}, {P_RESO, 66},
                  {P_E1_DEC, 56}, {P_E1_SUS, 0}, {P_E1_REL, 50}, {P_E1_AMT, 64}, {P_E2_DEC, 64}, {P_E2_SUS, 30},
                  {P_E2_REL, 50}, {P_KEYAMT, 60}, {P_ARP_ON, 1}, {P_ARP_MODE, AM_UP}, {P_ARP_OCT, 2}, {P_ARP_REPEAT, 1},
                  {P_DIVIDE, DV_16}, {P_BPM, 124}, {P_LFO1 + LFO_FREQ, 22}, {P_LFO1 + LFO_AMT, 40},
                  {P_LFO1 + LFO_DEST, MD_CUTOFF}, {P_FX_ON, 1}, {P_FX_TYPE, FX_TAPE1}, {P_FX_SYNC, 1}, {P_FX_TIME, 64},
                  {P_FX_MIX, 40}, {P_FX_MISC, 56}, {P_REV_ON, 1}, {P_REV_MIX, 30}, {P_REV_DECAY, 70}},
                 {VIB, {MS_MODWHEEL, 60, MD_CUTOFF}}});
    d.push_back({"Wide Pad",
                 {{P_O1_SHAPE, 64}, {P_O2_SHAPE, 64}, {P_O2_PITCH, 12}, {P_O2_LEVEL, 118}, {P_SUB, 50}, {P_CUTOFF, 640},
                  {P_RESO, 18}, {P_E1_ATT, 70}, {P_E1_DEC, 92}, {P_E1_SUS, 60}, {P_E1_REL, 92}, {P_E1_AMT, 22},
                  {P_E2_ATT, 66}, {P_E2_DEC, 88}, {P_E2_SUS, 120}, {P_E2_REL, 94}, {P_FX_ON, 1}, {P_FX_TYPE, FX_CHORUS},
                  {P_FX_TIME, 30}, {P_FX_MIX, 80}, {P_FX_MISC, 90}, {P_REV_ON, 1}, {P_REV_MIX, 62}, {P_REV_DECAY, 100},
                  {P_REV_PREDELAY, 36}, {P_VINTAGE, 50}},
                 {VIB, PRS, {MS_SPREAD, 70, MD_PAN}}});
    d.push_back({"Chapel Organ",
                 {{P_O1_SHAPE, 80}, {P_O2_SHAPE, 20}, {P_O2_OCT, 1}, {P_O2_PITCH, 700}, {P_O2_LEVEL, 70}, {P_SUB, 90},
                  {P_CUTOFF, 820}, {P_E2_ATT, 10}, {P_E2_DEC, 0}, {P_E2_SUS, 127}, {P_E2_REL, 50}, {P_FX_ON, 1},
                  {P_FX_TYPE, FX_ROTARY}, {P_FX_TIME, 30}, {P_FX_MIX, 50}, {P_FX_MISC, 40}, {P_REV_ON, 1},
                  {P_REV_MIX, 40}, {P_REV_DECAY, 90}, {P_REV_PREDELAY, 30}, {P_OVERDRIVE, 18}},
                 {{MS_MODWHEEL, 80, MD_FX_TIME}}});
    d.push_back({"Neon Brass",
                 {{P_O1_SHAPE, 64}, {P_O2_SHAPE, 64}, {P_O2_PITCH, 10}, {P_O2_LEVEL, 120}, {P_CUTOFF, 380}, {P_RESO, 10},
                  {P_E1_ATT, 56}, {P_E1_DEC, 76}, {P_E1_SUS, 70}, {P_E1_REL, 66}, {P_E1_AMT, 60}, {P_E1_VEL, 1},
                  {P_E2_ATT, 40}, {P_E2_DEC, 70}, {P_E2_SUS, 110}, {P_E2_REL, 66}, {P_KEYAMT, 50}, {P_REV_ON, 1},
                  {P_REV_MIX, 40}, {P_REV_DECAY, 80}, {P_VINTAGE, 36}},
                 {VIB, PRS, {MS_ENV1, 6, MD_OSC_FINE}}});
    d.push_back({"Twin Tenths",
                 {{P_O1_SHAPE, 64}, {P_O2_SHAPE, 80}, {P_O2_OCT, 1}, {P_O2_PITCH, 400}, {P_O2_LEVEL, 92}, {P_CUTOFF, 560},
                  {P_RESO, 44}, {P_DRIVE, 60}, {P_E1_DEC, 74}, {P_E1_SUS, 60}, {P_E1_REL, 56}, {P_E1_AMT, 50},
                  {P_E2_DEC, 70}, {P_E2_SUS, 120}, {P_E2_REL, 56}, {P_UNISON, 1}, {P_UNI_VOICES, 2}, {P_UNI_DETUNE, 2},
                  {P_GLIDE, 1}, {P_GLIDE_RATE, 50}, {P_OVERDRIVE, 46}, {P_FX_ON, 1}, {P_FX_TYPE, FX_TAPE2},
                  {P_FX_SYNC, 1}, {P_FX_TIME, 80}, {P_FX_MIX, 30}, {P_FX_MISC, 44}, {P_REV_ON, 1}, {P_REV_MIX, 30}},
                 {VIB, PRS}});
    d.push_back({"Signal Lost",
                 {{P_O1_SHAPE, 100}, {P_O2_SHAPE, 0}, {P_O2_OCT, -2}, {P_O2_LEVEL, 70}, {P_FM, 1}, {P_NOISE, 30},
                  {P_CUTOFF, 520}, {P_RESO, 90}, {P_E1_ATT, 70}, {P_E1_DEC, 100}, {P_E1_SUS, 40}, {P_E1_REL, 100},
                  {P_E1_AMT, 40}, {P_E2_ATT, 60}, {P_E2_SUS, 127}, {P_E2_REL, 100}, {P_LFO1 + LFO_SHAPE, LS_SH},
                  {P_LFO1 + LFO_FREQ, 70}, {P_LFO1 + LFO_AMT, 70}, {P_LFO1 + LFO_DEST, MD_CUTOFF},
                  {P_LFO1 + LFO_SLEW, 60}, {P_LFO1 + LFO_N + LFO_FREQ, 20}, {P_LFO1 + LFO_N + LFO_AMT, 60},
                  {P_LFO1 + LFO_N + LFO_DEST, MD_OSC2_FINE}, {P_FX_ON, 1}, {P_FX_TYPE, FX_FLANGER}, {P_FX_TIME, 20},
                  {P_FX_MIX, 90}, {P_FX_MISC, 100}, {P_REV_ON, 1}, {P_REV_MIX, 70}, {P_REV_DECAY, 116}},
                 {{MS_MODWHEEL, 80, MD_LFO1_FREQ}}});
    /* ======================================================================= BANK 2 === */
    d.push_back({"Unison Hero",
                 {{P_O1_SHAPE, 90}, {P_O2_SHAPE, 64}, {P_O2_PITCH, -7}, {P_O2_LEVEL, 100}, {P_CUTOFF, 640}, {P_RESO, 30},
                  {P_E1_DEC, 70}, {P_E1_SUS, 60}, {P_E1_AMT, 36}, {P_E2_DEC, 60}, {P_E2_SUS, 120}, {P_E2_REL, 50},
                  {P_UNISON, 1}, {P_UNI_VOICES, 4}, {P_UNI_DETUNE, 3}, {P_GLIDE, 1}, {P_GLIDE_RATE, 40},
                  {P_GLIDE_MODE, GM_RATE_A}, {P_LFO1 + LFO_N + LFO_FREQ, 60}, {P_FX_ON, 1}, {P_FX_TYPE, FX_DELAY},
                  {P_FX_SYNC, 1}, {P_FX_TIME, 70}, {P_FX_MIX, 34}, {P_FX_MISC, 50}, {P_REV_ON, 1}, {P_REV_MIX, 30}},
                 {VIB, PRS, {MS_PRESSURE, 24, MD_LFO2_AMT}}});
    d.push_back({"Pulse Pad",
                 {{P_O1_SHAPE, 70}, {P_O2_SHAPE, 100}, {P_O2_PITCH, 6}, {P_O2_LEVEL, 100}, {P_CUTOFF, 460}, {P_RESO, 46},
                  {P_E1_ATT, 30}, {P_E1_DEC, 74}, {P_E1_SUS, 30}, {P_E1_REL, 84}, {P_E1_AMT, 50}, {P_E2_ATT, 30},
                  {P_E2_DEC, 80}, {P_E2_SUS, 100}, {P_E2_REL, 86}, {P_LFO1 + LFO_SYNC, 1}, {P_LFO1 + LFO_FREQ, 88},
                  {P_LFO1 + LFO_SHAPE, LS_SQUARE}, {P_LFO1 + LFO_AMT, 30}, {P_LFO1 + LFO_DEST, MD_CUTOFF},
                  {P_LFO1 + LFO_SLEW, 40}, {P_REV_ON, 1}, {P_REV_MIX, 56}, {P_REV_DECAY, 92}, {P_VINTAGE, 30}},
                 {VIB}});
    d.push_back({"Narrow Lead",
                 {{P_O1_SHAPE, 64}, {P_O2_SHAPE, 64}, {P_O2_OCT, 1}, {P_O2_PITCH, 4}, {P_O2_LEVEL, 80}, {P_CUTOFF, 620},
                  {P_RESO, 80}, {P_E1_DEC, 70}, {P_E1_SUS, 40}, {P_E1_AMT, 40}, {P_E2_SUS, 120}, {P_E2_REL, 50},
                  {P_FX_ON, 1}, {P_FX_TYPE, FX_HPF}, {P_FX_TIME, 64}, {P_FX_MIX, 127}, {P_FX_MISC, 50}, {P_GLIDE, 1},
                  {P_GLIDE_RATE, 30}, {P_REV_ON, 1}, {P_REV_MIX, 34}, {P_REV_DECAY, 80}},
                 {VIB, {MS_MODWHEEL, 60, MD_FX_TIME}, PRS}});
    d.push_back({"Section Horns",
                 {{P_O1_SHAPE, 64}, {P_O2_SHAPE, 64}, {P_O2_PITCH, -11}, {P_O2_LEVEL, 124}, {P_CUTOFF, 360},
                  {P_E1_ATT, 50}, {P_E1_DEC, 72}, {P_E1_SUS, 76}, {P_E1_REL, 60}, {P_E1_AMT, 66}, {P_E1_VEL, 1},
                  {P_E2_ATT, 36}, {P_E2_SUS, 120}, {P_E2_REL, 62}, {P_E2_VEL, 1}, {P_KEYAMT, 64},
                  {P_LFO1 + LFO_N + LFO_FREQ, 66}, {P_REV_ON, 1}, {P_REV_MIX, 42}, {P_REV_DECAY, 86},
                  {P_REV_PREDELAY, 28}, {P_VINTAGE, 44}},
                 {{MS_MODWHEEL, 30, MD_LFO2_AMT}, PRS, {MS_ENV1, 5, MD_OSC_FINE}}});
    d.push_back({"Warm Saws",
                 {{P_O1_SHAPE, 64}, {P_O2_SHAPE, 64}, {P_O2_PITCH, 9}, {P_O2_LEVEL, 120}, {P_SUB, 40}, {P_CUTOFF, 600},
                  {P_RESO, 16}, {P_E1_DEC, 86}, {P_E1_SUS, 50}, {P_E1_REL, 80}, {P_E1_AMT, 30}, {P_E2_ATT, 20},
                  {P_E2_SUS, 120}, {P_E2_REL, 80}, {P_FX_ON, 1}, {P_FX_TYPE, FX_CHORUS}, {P_FX_TIME, 36}, {P_FX_MIX, 64},
                  {P_FX_MISC, 80}, {P_REV_ON, 1}, {P_REV_MIX, 48}, {P_REV_DECAY, 90}, {P_VINTAGE, 56}},
                 {VIB, PRS}});
    d.push_back({"Long Sweep",
                 {{P_O1_SHAPE, 64}, {P_O2_SHAPE, 64}, {P_O2_PITCH, 14}, {P_O2_LEVEL, 120}, {P_CUTOFF, 300}, {P_RESO, 74},
                  {P_E1_ATT, 104}, {P_E1_DEC, 108}, {P_E1_SUS, 60}, {P_E1_REL, 104}, {P_E1_AMT, 64}, {P_E2_ATT, 70},
                  {P_E2_SUS, 120}, {P_E2_REL, 100}, {P_REV_ON, 1}, {P_REV_MIX, 64}, {P_REV_DECAY, 110},
                  {P_REV_PREDELAY, 64}, {P_VINTAGE, 40}},
                 {VIB, PRS}});
    d.push_back({"Quill Keys",
                 {{P_O1_SHAPE, 104}, {P_O2_SHAPE, 64}, {P_O2_OCT, 1}, {P_O2_LEVEL, 70}, {P_CUTOFF, 640}, {P_RESO, 20},
                  {P_E1_DEC, 74}, {P_E1_SUS, 0}, {P_E1_REL, 70}, {P_E1_AMT, 70}, {P_E1_VEL, 1}, {P_E2_DEC, 86},
                  {P_E2_SUS, 0}, {P_E2_REL, 72}, {P_E2_VEL, 1}, {P_KEYAMT, 90}, {P_REV_ON, 1}, {P_REV_MIX, 36},
                  {P_REV_DECAY, 76}, {P_REV_TONE, 12}},
                 {VIB}});
    d.push_back({"Brownout",
                 {{P_O1_SHAPE, 64}, {P_O2_SHAPE, 90}, {P_O2_LEVEL, 100}, {P_O2_PITCH, -5}, {P_CUTOFF, 700}, {P_RESO, 40},
                  {P_ENV_ROUTING, ER_AUX_FILTERAMP}, {P_E1_DEST, MD_OSC_FINE}, {P_E1_AMT, -110}, {P_E1_ATT, 70},
                  {P_E1_DEC, 104}, {P_E1_SUS, 0}, {P_E2_DEC, 104}, {P_E2_SUS, 0}, {P_E2_REL, 100}, {P_E2_AMT, 30},
                  {P_OVERDRIVE, 50}, {P_FX_ON, 1}, {P_FX_TYPE, FX_TAPE2}, {P_FX_TIME, 60}, {P_FX_MIX, 40},
                  {P_FX_MISC, 70}, {P_REV_ON, 1}, {P_REV_MIX, 50}, {P_REV_DECAY, 100}},
                 {}});
    d.push_back({"Dice Harp",
                 {{P_KEYAMT, 60}, {P_O1_SHAPE, 40}, {P_O2_SHAPE, 64}, {P_O2_OCT, 1}, {P_O2_LEVEL, 60}, {P_CUTOFF, 520}, {P_RESO, 36},
                  {P_E1_DEC, 70}, {P_E1_SUS, 0}, {P_E1_REL, 70}, {P_E1_AMT, 60}, {P_E2_DEC, 84}, {P_E2_SUS, 0},
                  {P_E2_REL, 84}, {P_ARP_ON, 1}, {P_ARP_MODE, AM_RANDOM}, {P_ARP_OCT, 3}, {P_DIVIDE, DV_16T},
                  {P_BPM, 96}, {P_REV_ON, 1}, {P_REV_MIX, 60}, {P_REV_DECAY, 100}, {P_REV_DAMP, 30}},
                 {{MS_RANDOM, 40, MD_CUTOFF}, {MS_RANDOM, 30, MD_PAN}, VIB}});
    d.push_back({"Synth Tom",
                 {{P_O1_SHAPE, 0}, {P_O1_OCT, -1}, {P_O2_LEVEL, 0}, {P_NOISE, 24}, {P_CUTOFF, 640},
                  {P_ENV_ROUTING, ER_AUX_FILTERAMP}, {P_E1_DEST, MD_OSC1_FINE}, {P_E1_AMT, 60}, {P_E1_DEC, 62},
                  {P_E1_SUS, 0}, {P_E1_REL, 62}, {P_E2_DEC, 74}, {P_E2_SUS, 0}, {P_E2_REL, 74}, {P_E2_AMT, 10},
                  {P_E2_VEL, 1}, {P_REV_ON, 1}, {P_REV_MIX, 40}, {P_REV_DECAY, 72}},
                 {}});
    d.push_back({"Alpha Strings",
                 {{P_O1_SHAPE, 64}, {P_O2_SHAPE, 88}, {P_O2_PITCH, 8}, {P_O2_LEVEL, 110}, {P_CUTOFF, 540}, {P_RESO, 14},
                  {P_E1_ATT, 80}, {P_E1_DEC, 90}, {P_E1_SUS, 80}, {P_E1_REL, 92}, {P_E1_AMT, 20}, {P_E2_ATT, 78},
                  {P_E2_SUS, 124}, {P_E2_REL, 94}, {P_LFO1 + LFO_N + LFO_FREQ, 50}, {P_LFO1 + LFO_N + LFO_AMT, 30},
                  {P_LFO1 + LFO_N + LFO_DEST, MD_OSC2_SHAPE}, {P_FX_ON, 1}, {P_FX_TYPE, FX_CHORUS}, {P_FX_TIME, 50},
                  {P_FX_MIX, 90}, {P_FX_MISC, 70}, {P_REV_ON, 1}, {P_REV_MIX, 60}, {P_REV_DECAY, 104},
                  {P_REV_PREDELAY, 42}, {P_VINTAGE, 50}},
                 {VIB, PRS}});
    d.push_back({"Rain Arp",
                 {{P_KEYAMT, 80}, {P_O1_SHAPE, 64}, {P_O2_SHAPE, 64}, {P_O2_PITCH, 700}, {P_O2_LEVEL, 70}, {P_CUTOFF, 500}, {P_RESO, 60},
                  {P_E1_DEC, 60}, {P_E1_SUS, 0}, {P_E1_AMT, 70}, {P_E1_VEL, 1}, {P_E2_DEC, 66}, {P_E2_SUS, 0},
                  {P_E2_REL, 60}, {P_ARP_ON, 1}, {P_ARP_MODE, AM_UPDOWN}, {P_ARP_OCT, 3}, {P_DIVIDE, DV_16}, {P_BPM, 128},
                  {P_FX_ON, 1}, {P_FX_TYPE, FX_DELAY}, {P_FX_SYNC, 1}, {P_FX_TIME, 82}, {P_FX_MIX, 54}, {P_FX_MISC, 76},
                  {P_REV_ON, 1}, {P_REV_MIX, 60}, {P_REV_DECAY, 98}},
                 {{MS_MODWHEEL, 60, MD_CUTOFF}}});
    d.push_back({"Subterranean",
                 {{P_O1_OCT, -1}, {P_O1_SHAPE, 64}, {P_O2_OCT, -1}, {P_O2_SHAPE, 64}, {P_O2_PITCH, -14}, {P_O2_LEVEL, 127},
                  {P_SUB, 127}, {P_CUTOFF, 380}, {P_RESO, 30}, {P_DRIVE, 70}, {P_E1_DEC, 70}, {P_E1_SUS, 30},
                  {P_E1_AMT, 50}, {P_E2_SUS, 127}, {P_E2_REL, 40}, {P_UNISON, 1}, {P_UNI_VOICES, 5}, {P_UNI_DETUNE, 4},
                  {P_OVERDRIVE, 40}},
                 {PRS}});
    d.push_back({"Ocean Floor",
                 {{P_O1_SHAPE, 30}, {P_O2_SHAPE, 64}, {P_O2_OCT, 1}, {P_O2_PITCH, 5}, {P_O2_LEVEL, 80}, {P_NOISE, -26},
                  {P_CUTOFF, 560}, {P_RESO, 30}, {P_E1_ATT, 96}, {P_E1_DEC, 100}, {P_E1_SUS, 70}, {P_E1_REL, 100},
                  {P_E1_AMT, 34}, {P_E2_ATT, 96}, {P_E2_SUS, 124}, {P_E2_REL, 104}, {P_LFO1 + LFO_FREQ, 26},
                  {P_LFO1 + LFO_AMT, 26}, {P_LFO1 + LFO_DEST, MD_CUTOFF}, {P_REV_ON, 1}, {P_REV_MIX, 80},
                  {P_REV_DECAY, 118}, {P_REV_DAMP, 64}, {P_REV_PREDELAY, 70}, {P_REV_TONE, -18}, {P_VINTAGE, 36}},
                 {VIB, {MS_SPREAD, 60, MD_PAN}}});
    d.push_back({"Overdrive Organ",
                 {{P_O1_SHAPE, 82}, {P_O2_SHAPE, 82}, {P_O2_OCT, 1}, {P_O2_LEVEL, 100}, {P_SUB, 100}, {P_CUTOFF, 760},
                  {P_E2_SUS, 127}, {P_E2_REL, 46}, {P_OVERDRIVE, 70}, {P_FX_ON, 1}, {P_FX_TYPE, FX_ROTARY},
                  {P_FX_TIME, 110}, {P_FX_MIX, 80}, {P_FX_MISC, 30}, {P_REV_ON, 1}, {P_REV_MIX, 26}},
                 {{MS_MODWHEEL, -90, MD_FX_TIME}}});
    d.push_back({"Zen Pluck",
                 {{P_O1_SHAPE, 108}, {P_O2_SHAPE, 64}, {P_O2_OCT, 1}, {P_O2_LEVEL, 40}, {P_CUTOFF, 560}, {P_RESO, 54},
                  {P_E1_DEC, 66}, {P_E1_SUS, 0}, {P_E1_AMT, 66}, {P_E1_VEL, 1}, {P_E2_DEC, 82}, {P_E2_SUS, 0},
                  {P_E2_REL, 80}, {P_KEYAMT, 100}, {P_REV_ON, 1}, {P_REV_MIX, 52}, {P_REV_DECAY, 92}, {P_REV_PREDELAY, 40}},
                 {{MS_MODWHEEL, 40, MD_LFO1_AMT}, {MS_PRESSURE, 30, MD_OSC_FINE}}});
    /* ======================================================================= BANK 3 === */
    d.push_back({"Temple Bells",
                 {{P_O1_SHAPE, 0}, {P_O2_SHAPE, 0}, {P_FM, 1}, {P_O2_OCT, 1}, {P_O2_PITCH, 560}, {P_O2_LEVEL, 62},
                  {P_CUTOFF, 1023}, {P_ENV_ROUTING, ER_AUX_FILTERAMP}, {P_E1_DEST, MD_FM_LEVEL}, {P_E1_AMT, 40},
                  {P_E1_DEC, 100}, {P_E1_SUS, 0}, {P_E1_REL, 100}, {P_E2_DEC, 110}, {P_E2_SUS, 0}, {P_E2_REL, 106},
                  {P_E2_AMT, 0}, {P_E2_VEL, 1}, {P_REV_ON, 1}, {P_REV_MIX, 62}, {P_REV_DECAY, 104}, {P_REV_DAMP, 26},
                  {P_REV_PREDELAY, 50}, {P_VINTAGE, 14}},
                 {{MS_MODWHEEL, 40, MD_LFO2_AMT}, {MS_VELOCITY, 26, MD_FM_LEVEL}}});
    d.push_back({"Glass Marimba",
                 {{P_O1_SHAPE, 0}, {P_O2_SHAPE, 0}, {P_FM, 1}, {P_O2_OCT, 2}, {P_O2_LEVEL, 30}, {P_CUTOFF, 1023},
                  {P_ENV_ROUTING, ER_AUX_FILTERAMP}, {P_E1_DEST, MD_FM_LEVEL}, {P_E1_AMT, 70}, {P_E1_VEL, 1},
                  {P_E1_DEC, 54}, {P_E1_SUS, 0}, {P_E1_REL, 54}, {P_E2_DEC, 80}, {P_E2_SUS, 0}, {P_E2_REL, 80},
                  {P_E2_AMT, 0}, {P_E2_VEL, 1}, {P_REV_ON, 1}, {P_REV_MIX, 46}, {P_REV_DECAY, 84}, {P_REV_TONE, 8}},
                 {}});
    d.push_back({"FM Clav",
                 {{P_O1_SHAPE, 76}, {P_O2_SHAPE, 0}, {P_FM, 1}, {P_O2_OCT, 1}, {P_O2_LEVEL, 40}, {P_CUTOFF, 720},
                  {P_RESO, 40}, {P_E1_DEC, 60}, {P_E1_SUS, 10}, {P_E1_AMT, 60}, {P_E1_VEL, 1}, {P_E2_DEC, 72},
                  {P_E2_SUS, 40}, {P_E2_REL, 40}, {P_E2_VEL, 1}, {P_KEYAMT, 80}, {P_FX_ON, 1}, {P_FX_TYPE, FX_PHASER},
                  {P_FX_TIME, 50}, {P_FX_MIX, 60}, {P_FX_MISC, 60}, {P_REV_ON, 1}, {P_REV_MIX, 24}},
                 {{MS_MODWHEEL, 50, MD_FM_LEVEL}, PRS}});
    d.push_back({"Iron Bass",
                 /* FM on a bass: a sine carrier, the modulator an octave down, the index snapping shut */
                 {{P_O1_OCT, -1}, {P_O1_SHAPE, 0}, {P_O2_OCT, -2}, {P_O2_SHAPE, 0}, {P_FM, 1}, {P_O2_LEVEL, 40},
                  {P_SUB, 70}, {P_CUTOFF, 560}, {P_ENV_ROUTING, ER_AUX_FILTERAMP}, {P_E1_DEST, MD_FM_LEVEL},
                  {P_E1_AMT, 80}, {P_E1_VEL, 1}, {P_E1_DEC, 62}, {P_E1_SUS, 10}, {P_E1_REL, 40}, {P_E2_DEC, 70},
                  {P_E2_SUS, 100}, {P_E2_REL, 40}, {P_E2_AMT, 20}, {P_DRIVE, 40}, {P_OVERDRIVE, 26}},
                 {{MS_MODWHEEL, 60, MD_FM_LEVEL}}});
    d.push_back({"Tine Piano",
                 {{P_O1_SHAPE, 0}, {P_O2_SHAPE, 0}, {P_FM, 1}, {P_O2_OCT, 0}, {P_O2_PITCH, 0}, {P_O2_LEVEL, 18},
                  {P_CUTOFF, 860}, {P_ENV_ROUTING, ER_AUX_FILTERAMP}, {P_E1_DEST, MD_FM_LEVEL}, {P_E1_AMT, 80},
                  {P_E1_VEL, 1}, {P_E1_DEC, 70}, {P_E1_SUS, 0}, {P_E1_REL, 60}, {P_E2_DEC, 100}, {P_E2_SUS, 0},
                  {P_E2_REL, 70}, {P_E2_AMT, 0}, {P_E2_VEL, 1}, {P_KEYAMT, 60}, {P_FX_ON, 1}, {P_FX_TYPE, FX_ROTARY},
                  {P_FX_TIME, 20}, {P_FX_MIX, 10}, {P_FX_MISC, 70}, {P_REV_ON, 1}, {P_REV_MIX, 34}, {P_REV_DECAY, 72}},
                 {{MS_MODWHEEL, 100, MD_FX_TIME}, {MS_VELOCITY, 30, MD_FM_LEVEL}}});
    d.push_back({"Metal Pluck",
                 {{P_KEYAMT, 80}, {P_O1_SHAPE, 64}, {P_O2_SHAPE, 0}, {P_FM, 1}, {P_O2_OCT, 1}, {P_O2_PITCH, 314}, {P_O2_LEVEL, 70},
                  {P_CUTOFF, 520}, {P_RESO, 50}, {P_E1_DEC, 60}, {P_E1_SUS, 0}, {P_E1_AMT, 80}, {P_E1_VEL, 1},
                  {P_E2_DEC, 76}, {P_E2_SUS, 0}, {P_E2_REL, 76}, {P_FX_ON, 1}, {P_FX_TYPE, FX_DELAY}, {P_FX_SYNC, 1},
                  {P_FX_TIME, 70}, {P_FX_MIX, 40}, {P_FX_MISC, 60}, {P_REV_ON, 1}, {P_REV_MIX, 50}, {P_REV_DECAY, 90}},
                 {{MS_MODWHEEL, 50, MD_FM_LEVEL}}});
    d.push_back({"Crystal Sequence",
                 {{P_KEYAMT, 70}, {P_O1_SHAPE, 20}, {P_O2_SHAPE, 64}, {P_O2_OCT, 1}, {P_O2_LEVEL, 50}, {P_CUTOFF, 520}, {P_RESO, 56},
                  {P_E1_DEC, 58}, {P_E1_SUS, 0}, {P_E1_AMT, 60}, {P_E2_DEC, 64}, {P_E2_SUS, 0}, {P_E2_REL, 58},
                  {P_BPM, 118}, {P_DIVIDE, DV_16}, {P_LFO1 + LFO_SYNC, 1}, {P_LFO1 + LFO_FREQ, 16}, {P_LFO1 + LFO_AMT, 50},
                  {P_LFO1 + LFO_DEST, MD_CUTOFF}, {P_FX_ON, 1}, {P_FX_TYPE, FX_DELAY}, {P_FX_SYNC, 1}, {P_FX_TIME, 76},
                  {P_FX_MIX, 50}, {P_FX_MISC, 66}, {P_REV_ON, 1}, {P_REV_MIX, 56}, {P_REV_DECAY, 96}},
                 {{MS_MODWHEEL, 60, MD_CUTOFF}},
                 {{48}, {60}, {55}, {63}, {-1}, {60}, {67}, {70}, {48}, {60}, {58}, {63}, {}, {67}, {65}, {63}}});
    d.push_back({"Plate Strings",
                 {{P_O1_SHAPE, 64}, {P_O2_SHAPE, 64}, {P_O2_PITCH, -10}, {P_O2_LEVEL, 116}, {P_CUTOFF, 500}, {P_RESO, 10},
                  {P_E1_ATT, 86}, {P_E1_DEC, 90}, {P_E1_SUS, 90}, {P_E1_REL, 96}, {P_E1_AMT, 26}, {P_E2_ATT, 84},
                  {P_E2_SUS, 120}, {P_E2_REL, 98}, {P_FX_ON, 1}, {P_FX_TYPE, FX_CHORUS}, {P_FX_TIME, 44}, {P_FX_MIX, 86},
                  {P_FX_MISC, 66}, {P_REV_ON, 1}, {P_REV_MIX, 72}, {P_REV_DECAY, 112}, {P_REV_DAMP, 44},
                  {P_REV_PREDELAY, 56}, {P_VINTAGE, 46}},
                 {VIB, PRS}});
    d.push_back({"Ghost Choir",
                 {{P_O1_SHAPE, 26}, {P_O2_SHAPE, 96}, {P_O2_PITCH, 5}, {P_O2_LEVEL, 80}, {P_NOISE, -30}, {P_CUTOFF, 470},
                  {P_RESO, 70}, {P_E1_ATT, 90}, {P_E1_DEC, 96}, {P_E1_SUS, 80}, {P_E1_REL, 96}, {P_E1_AMT, 18},
                  {P_E2_ATT, 90}, {P_E2_SUS, 120}, {P_E2_REL, 100}, {P_LFO1 + LFO_N + LFO_FREQ, 56},
                  {P_LFO1 + LFO_N + LFO_AMT, 10}, {P_LFO1 + LFO_N + LFO_DEST, MD_CUTOFF}, {P_FX_ON, 1},
                  {P_FX_TYPE, FX_FLANGER}, {P_FX_TIME, 14}, {P_FX_MIX, 50}, {P_FX_MISC, 40}, {P_REV_ON, 1},
                  {P_REV_MIX, 84}, {P_REV_DECAY, 120}, {P_REV_DAMP, 70}, {P_REV_PREDELAY, 80}, {P_REV_TONE, -22},
                  {P_VINTAGE, 60}},
                 {VIB}});
    d.push_back({"Lo-Fi Keys",
                 {{P_O1_SHAPE, 20}, {P_O2_SHAPE, 64}, {P_O2_OCT, 1}, {P_O2_LEVEL, 40}, {P_CUTOFF, 520}, {P_E1_DEC, 80},
                  {P_E1_SUS, 20}, {P_E1_AMT, 40}, {P_E1_VEL, 1}, {P_E2_DEC, 92}, {P_E2_SUS, 30}, {P_E2_REL, 70},
                  {P_E2_VEL, 1}, {P_FX_ON, 1}, {P_FX_TYPE, FX_LOFI}, {P_FX_TIME, 80}, {P_FX_MIX, 70}, {P_FX_MISC, 50},
                  {P_REV_ON, 1}, {P_REV_MIX, 40}, {P_REV_DECAY, 80}, {P_VINTAGE, 70}},
                 {VIB}});
    d.push_back({"Drawbar Spin",
                 {{P_O1_SHAPE, 4}, {P_O2_SHAPE, 4}, {P_O2_OCT, 1}, {P_O2_PITCH, 700}, {P_O2_LEVEL, 90}, {P_SUB, 110},
                  {P_CUTOFF, 1023}, {P_E2_SUS, 127}, {P_E2_REL, 40}, {P_FX_ON, 1}, {P_FX_TYPE, FX_ROTARY},
                  {P_FX_TIME, 30}, {P_FX_MIX, 40}, {P_FX_MISC, 50}, {P_REV_ON, 1}, {P_REV_MIX, 30}, {P_OVERDRIVE, 24}},
                 {{MS_MODWHEEL, 100, MD_FX_TIME}}});
    d.push_back({"Tape Echo Lead",
                 {{P_O1_SHAPE, 64}, {P_O2_SHAPE, 100}, {P_O2_PITCH, 5}, {P_O2_LEVEL, 90}, {P_CUTOFF, 600}, {P_RESO, 34},
                  {P_E1_DEC, 74}, {P_E1_SUS, 50}, {P_E1_AMT, 40}, {P_E2_SUS, 120}, {P_E2_REL, 60}, {P_GLIDE, 1},
                  {P_GLIDE_RATE, 36}, {P_LFO1 + LFO_N + LFO_FREQ, 64}, {P_FX_ON, 1}, {P_FX_TYPE, FX_TAPE1},
                  {P_FX_SYNC, 1}, {P_FX_TIME, 64}, {P_FX_MIX, 50}, {P_FX_MISC, 80}, {P_REV_ON, 1}, {P_REV_MIX, 40},
                  {P_REV_DECAY, 90}},
                 {{MS_MODWHEEL, 34, MD_LFO2_AMT}, PRS}});
    d.push_back({"Ring Bells",
                 {{P_O1_SHAPE, 0}, {P_O2_LEVEL, 0}, {P_CUTOFF, 1023}, {P_E2_DEC, 96}, {P_E2_SUS, 0}, {P_E2_REL, 96},
                  {P_FX_ON, 1}, {P_FX_TYPE, FX_RINGMOD}, {P_FX_TIME, 82}, {P_FX_MIX, 80}, {P_FX_MISC, 100},
                  {P_REV_ON, 1}, {P_REV_MIX, 60}, {P_REV_DECAY, 100}},
                 {{MS_MODWHEEL, 40, MD_FX_TIME}}});
    d.push_back({"Phase Pad",
                 {{P_O1_SHAPE, 64}, {P_O2_SHAPE, 64}, {P_O2_PITCH, 7}, {P_O2_LEVEL, 110}, {P_CUTOFF, 620}, {P_RESO, 20},
                  {P_E1_ATT, 80}, {P_E1_SUS, 80}, {P_E1_AMT, 20}, {P_E2_ATT, 70}, {P_E2_SUS, 120}, {P_E2_REL, 94},
                  {P_FX_ON, 1}, {P_FX_TYPE, FX_PHASER}, {P_FX_TIME, 20}, {P_FX_MIX, 90}, {P_FX_MISC, 80}, {P_REV_ON, 1},
                  {P_REV_MIX, 56}, {P_REV_DECAY, 100}, {P_VINTAGE, 34}},
                 {VIB, PRS}});
    d.push_back({"Flanged Saw",
                 {{P_O1_SHAPE, 64}, {P_O2_SHAPE, 64}, {P_O2_OCT, -1}, {P_O2_LEVEL, 80}, {P_CUTOFF, 700}, {P_RESO, 30},
                  {P_E1_DEC, 80}, {P_E1_SUS, 40}, {P_E1_AMT, 30}, {P_E2_SUS, 120}, {P_E2_REL, 70}, {P_FX_ON, 1},
                  {P_FX_TYPE, FX_FLANGER}, {P_FX_TIME, 24}, {P_FX_MIX, 70}, {P_FX_MISC, 90}, {P_REV_ON, 1},
                  {P_REV_MIX, 40}},
                 {VIB, PRS}});
    d.push_back({"Chord Stabs",
                 {{P_KEYAMT, 60}, {P_O1_SHAPE, 64}, {P_O2_SHAPE, 64}, {P_O2_PITCH, 6}, {P_O2_LEVEL, 110}, {P_CUTOFF, 480}, {P_RESO, 40},
                  {P_E1_DEC, 62}, {P_E1_SUS, 0}, {P_E1_AMT, 66}, {P_E1_VEL, 1}, {P_E2_DEC, 68}, {P_E2_SUS, 0},
                  {P_E2_REL, 62}, {P_UNISON, 1}, {P_UNI_VOICES, 5}, {P_FX_ON, 1}, {P_FX_TYPE, FX_DELAY}, {P_FX_SYNC, 1},
                  {P_FX_TIME, 66}, {P_FX_MIX, 40}, {P_FX_MISC, 50}, {P_REV_ON, 1}, {P_REV_MIX, 50}, {P_REV_DECAY, 92}},
                 {{MS_MODWHEEL, 60, MD_CUTOFF}},
                 {},
                 {0, 3, 7, 10, 14}});
    return d;
}

/* PROGRAM VOLUME each, matched by ear's stand-in: the loudest 300 ms of a chord (or the arp, the sequence) about
 * -16 dBFS, no peak over -3 dBFS */
const int16_t VOL[48] = {125, 125, 114, 105, 101, 99, 104, 127, 99, 127, 127, 99, 72, 100, 91, 106, 107, 108, 114, 96, 99, 99, 118, 97, 127, 111, 91, 127, 85, 110, 66, 124, 94, 109, 126, 88, 108, 127, 127, 90, 105, 118, 76, 94, 110, 103, 98, 127};

std::vector<Program> build()
{
    std::vector<Program> v;
    int i = 0;
    for (auto &d : defs()) {
        v.push_back(make(d));
        if (i < 48)
            v.back().p[P_PGM_VOL] = VOL[i];
        i++;
    }
    Program basic;
    programInit(basic);
    v.push_back(basic);
    return v;
}
const std::vector<Program> &all()
{
    static const std::vector<Program> a = build();   /* (the engine's data: plain, no destructor work at unload) */
    return a;
}
} // namespace

const int NFACTORY = (int)all().size();
const Program *const FACTORY = all().data();
} // namespace f5
