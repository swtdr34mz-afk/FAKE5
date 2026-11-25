/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (FAKE5) */
/* FAKE5's factory programs: four banks of sixteen, made for FAKE5 (its own sounds, in the Take 5's manner: the filter
 * driven, VINTAGE up, the BBD and the chorus, the reverb a touch rather than a wash), then the BASIC PROGRAM. The
 * Take 5's own sounds come in through LOAD SYX, from its owner's files. Times in the Take 5's units: an envelope's 60 is about 90 ms, 80 half a second, 100 three seconds; CUTOFF
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


/* shorthands: the waves on SHAPE, a program's usual wheel and pressure */
constexpr int SINE = 0, SAW = 42, SAWSQ = 63, SQR = 85, PUL = 100, NEEDLE = 112;
const Mod WHEEL_VIB{MS_MODWHEEL, 34, MD_LFO1_AMT};
const Mod AT_CUT{MS_PRESSURE, 28, MD_CUTOFF};
const Mod VEL_CUT{MS_VELOCITY, 22, MD_CUTOFF};

std::vector<Def> defs()
{
    std::vector<Def> d;
    auto add = [&d](const char *n, std::vector<KV> kv, std::vector<Mod> m = {WHEEL_VIB, AT_CUT}, std::vector<std::vector<int>> sq = {},
                    std::vector<int> ch = {}) { d.push_back({n, std::move(kv), std::move(m), std::move(sq), std::move(ch)}); };

    /* ===================================================== BANK 1: polys and keys === */
    add("Golden Hour", {{P_O1_SHAPE, SAW}, {P_O2_SHAPE, SAW}, {P_O2_PITCH, 9}, {P_O1_OCT, -1}, {P_O2_OCT, -1}, {P_CUTOFF, 640},
                        {P_RESO, 18}, {P_DRIVE, 70}, {P_KEYAMT, 64}, {P_E1_DEC, 74}, {P_E1_SUS, 40}, {P_E1_REL, 72},
                        {P_E1_AMT, 38}, {P_E1_VEL, 1}, {P_E2_ATT, 8}, {P_E2_DEC, 80}, {P_E2_SUS, 110}, {P_E2_REL, 76},
                        {P_FX_ON, 1}, {P_FX_TYPE, FX_CHORUS}, {P_FX_TIME, 12}, {P_FX_MIX, 110}, {P_FX_MISC, 90},
                        {P_REV_ON, 1}, {P_REV_MIX, 16}, {P_REV_DECAY, 60}, {P_REV_DAMP, 70}, {P_VINTAGE, 46}, {P_SPREAD, 70}});
    add("Prophet Brass", {{P_O1_SHAPE, SAW}, {P_O2_SHAPE, SAW}, {P_O2_PITCH, 7}, {P_CUTOFF, 380}, {P_RESO, 10}, {P_DRIVE, 96},
                          {P_KEYAMT, 70}, {P_E1_ATT, 54}, {P_E1_DEC, 74}, {P_E1_SUS, 66}, {P_E1_REL, 66}, {P_E1_AMT, 74},
                          {P_E1_VEL, 1}, {P_E2_ATT, 30}, {P_E2_DEC, 76}, {P_E2_SUS, 116}, {P_E2_REL, 66}, {P_VINTAGE, 58},
                          {P_REV_ON, 1}, {P_REV_MIX, 14}, {P_REV_DECAY, 56}, {P_SPREAD, 50}},
        {WHEEL_VIB, AT_CUT, {MS_VELOCITY, 18, MD_ENV1_AMT}});
    add("Hard Sync Lead", {{P_O1_SHAPE, SAW}, {P_SYNC, 1}, {P_O1_OCT, 1}, {P_O2_SHAPE, SAW}, {P_O2_LEVEL, 0}, {P_CUTOFF, 820},
                           {P_RESO, 12}, {P_DRIVE, 110}, {P_ENV_ROUTING, ER_AUX_FILTERAMP}, {P_E1_DEST, MD_OSC1_FINE},
                           {P_E1_AMT, 70}, {P_E1_DEC, 80}, {P_E1_SUS, 10}, {P_E1_REL, 60}, {P_E2_AMT, 20}, {P_E2_DEC, 76},
                           {P_E2_SUS, 120}, {P_E2_REL, 58}, {P_UNISON, 1}, {P_UNI_VOICES, 2}, {P_UNI_DETUNE, 2},
                           {P_GLIDE, 1}, {P_GLIDE_RATE, 30}, {P_OVERDRIVE, 30}, {P_FX_ON, 1}, {P_FX_TYPE, FX_BBD},
                           {P_FX_SYNC, 1}, {P_FX_TIME, 64}, {P_FX_MIX, 30}, {P_FX_MISC, 40}, {P_VINTAGE, 34}},
        {WHEEL_VIB, {MS_PRESSURE, 50, MD_OSC1_FINE}});
    add("Rhodey", {{P_O1_SHAPE, SINE}, {P_O2_SHAPE, SINE}, {P_O2_OCT, 1}, {P_O2_LEVEL, 46}, {P_FM, 1}, {P_CUTOFF, 560},
                   {P_RESO, 0}, {P_DRIVE, 40}, {P_KEYAMT, 90}, {P_ENV_ROUTING, ER_AUX_FILTERAMP}, {P_E1_DEST, MD_FM_LEVEL},
                   {P_E1_AMT, 60}, {P_E1_VEL, 1}, {P_E1_DEC, 62}, {P_E1_SUS, 0}, {P_E1_REL, 50}, {P_E2_AMT, 40},
                   {P_E2_VEL, 1}, {P_E2_DEC, 92}, {P_E2_SUS, 0}, {P_E2_REL, 70}, {P_FX_ON, 1}, {P_FX_TYPE, FX_PHASER},
                   {P_FX_TIME, 22}, {P_FX_MIX, 60}, {P_FX_MISC, 40}, {P_VINTAGE, 40}, {P_OVERDRIVE, 18},
                   {P_REV_ON, 1}, {P_REV_MIX, 12}, {P_REV_DECAY, 50}, {P_SPREAD, 60}},
        {WHEEL_VIB, {MS_PRESSURE, 30, MD_FM_LEVEL}});
    add("Dark Wurli", {{P_O1_SHAPE, 20}, {P_O2_SHAPE, SQR}, {P_O2_LEVEL, 40}, {P_SUB, 30}, {P_CUTOFF, 420}, {P_RESO, 26},
                       {P_DRIVE, 127}, {P_KEYAMT, 80}, {P_E1_DEC, 66}, {P_E1_SUS, 10}, {P_E1_REL, 56}, {P_E1_AMT, 52},
                       {P_E1_VEL, 1}, {P_E2_VEL, 1}, {P_E2_DEC, 86}, {P_E2_SUS, 20}, {P_E2_REL, 60}, {P_FX_ON, 1},
                       {P_FX_TYPE, FX_TAPE1}, {P_FX_TIME, 60}, {P_FX_MIX, 22}, {P_FX_MISC, 30}, {P_VINTAGE, 64},
                       {P_OVERDRIVE, 40}, {P_SPREAD, 40}},
        {WHEEL_VIB, AT_CUT, {MS_LFO2, 10, MD_OSC1_LEVEL}});
    add("Analog Clav", {{P_O1_SHAPE, PUL}, {P_O2_SHAPE, NEEDLE}, {P_O2_PITCH, 4}, {P_CUTOFF, 520}, {P_RESO, 44}, {P_DRIVE, 90},
                        {P_KEYAMT, 100}, {P_E1_DEC, 56}, {P_E1_SUS, 0}, {P_E1_REL, 40}, {P_E1_AMT, 66}, {P_E1_VEL, 1},
                        {P_E2_DEC, 64}, {P_E2_SUS, 40}, {P_E2_REL, 40}, {P_E2_VEL, 1}, {P_FX_ON, 1},
                        {P_FX_TYPE, FX_PHASER}, {P_FX_TIME, 40}, {P_FX_MIX, 50}, {P_FX_MISC, 70}, {P_VINTAGE, 30}},
        {WHEEL_VIB, {MS_PRESSURE, 40, MD_RESO}});
    add("Juno Keys", {{P_O1_SHAPE, PUL}, {P_O2_LEVEL, 0}, {P_SUB, 70}, {P_CUTOFF, 600}, {P_RESO, 30}, {P_DRIVE, 60},
                      {P_KEYAMT, 60}, {P_E1_DEC, 68}, {P_E1_SUS, 30}, {P_E1_REL, 60}, {P_E1_AMT, 40}, {P_E1_VEL, 1},
                      {P_E2_DEC, 78}, {P_E2_SUS, 90}, {P_E2_REL, 64}, {P_LFO1 + LFO_FREQ, 50}, {P_LFO1 + LFO_AMT, 18},
                      {P_LFO1 + LFO_DEST, MD_OSC1_SHAPE}, {P_FX_ON, 1}, {P_FX_TYPE, FX_CHORUS}, {P_FX_TIME, 30},
                      {P_FX_MIX, 100}, {P_FX_MISC, 100}, {P_REV_ON, 1}, {P_REV_MIX, 12}, {P_REV_DECAY, 48},
                      {P_VINTAGE, 28}, {P_SPREAD, 80}});
    add("Velvet Poly", {{P_O1_SHAPE, SAW}, {P_O2_SHAPE, SAWSQ}, {P_O2_PITCH, -11}, {P_SUB, 30}, {P_CUTOFF, 470}, {P_RESO, 20},
                        {P_DRIVE, 84}, {P_KEYAMT, 64}, {P_E1_ATT, 20}, {P_E1_DEC, 78}, {P_E1_SUS, 50}, {P_E1_REL, 74},
                        {P_E1_AMT, 44}, {P_E1_VEL, 1}, {P_E2_ATT, 14}, {P_E2_DEC, 80}, {P_E2_SUS, 110}, {P_E2_REL, 78},
                        {P_FX_ON, 1}, {P_FX_TYPE, FX_BBD}, {P_FX_TIME, 50}, {P_FX_MIX, 24}, {P_FX_MISC, 34},
                        {P_REV_ON, 1}, {P_REV_MIX, 18}, {P_REV_DECAY, 62}, {P_VINTAGE, 70}, {P_SPREAD, 90}});
    add("Tape Piano", {{P_O1_SHAPE, 30}, {P_O2_SHAPE, SINE}, {P_O2_OCT, 1}, {P_O2_PITCH, 3}, {P_O2_LEVEL, 70}, {P_CUTOFF, 520},
                       {P_DRIVE, 50}, {P_KEYAMT, 100}, {P_E1_DEC, 72}, {P_E1_SUS, 0}, {P_E1_REL, 62}, {P_E1_AMT, 56},
                       {P_E1_VEL, 1}, {P_E2_DEC, 94}, {P_E2_SUS, 0}, {P_E2_REL, 66}, {P_E2_VEL, 1}, {P_FX_ON, 1},
                       {P_FX_TYPE, FX_LOFI}, {P_FX_TIME, 60}, {P_FX_MIX, 70}, {P_FX_MISC, 30}, {P_REV_ON, 1},
                       {P_REV_MIX, 14}, {P_REV_DECAY, 54}, {P_VINTAGE, 50}, {P_SPREAD, 50}});
    add("Drawbars", {{P_O1_SHAPE, SINE}, {P_O2_SHAPE, SINE}, {P_O2_OCT, 1}, {P_O2_PITCH, 700}, {P_O2_LEVEL, 80}, {P_SUB, 100},
                     {P_CUTOFF, 900}, {P_DRIVE, 100}, {P_E2_ATT, 0}, {P_E2_DEC, 0}, {P_E2_SUS, 127}, {P_E2_REL, 30},
                     {P_E1_AMT, 0}, {P_OVERDRIVE, 44}, {P_FX_ON, 1}, {P_FX_TYPE, FX_ROTARY}, {P_FX_TIME, 30},
                     {P_FX_MIX, 60}, {P_FX_MISC, 60}, {P_REV_ON, 1}, {P_REV_MIX, 10}, {P_REV_DECAY, 44}},
        {{MS_MODWHEEL, 90, MD_FX_TIME}, {MS_PRESSURE, 40, MD_OVERDRIVE}});
    add("Gnarly Organ", {{P_O1_SHAPE, SQR}, {P_O2_SHAPE, SQR}, {P_O2_OCT, 1}, {P_O2_LEVEL, 90}, {P_SUB, 110}, {P_CUTOFF, 700},
                         {P_RESO, 20}, {P_DRIVE, 127}, {P_E2_SUS, 127}, {P_E2_REL, 34}, {P_E1_AMT, 10}, {P_E1_DEC, 60},
                         {P_E1_SUS, 0}, {P_OVERDRIVE, 70}, {P_VINTAGE, 40}, {P_FX_ON, 1}, {P_FX_TYPE, FX_ROTARY},
                         {P_FX_TIME, 80}, {P_FX_MIX, 80}, {P_FX_MISC, 50}},
        {{MS_MODWHEEL, 90, MD_FX_TIME}, AT_CUT});
    add("Glass Harp", {{P_O1_SHAPE, SINE}, {P_O2_SHAPE, SINE}, {P_O2_OCT, 2}, {P_O2_LEVEL, 60}, {P_FM, 1}, {P_CUTOFF, 760},
                       {P_KEYAMT, 100}, {P_ENV_ROUTING, ER_AUX_FILTERAMP}, {P_E1_DEST, MD_FM_LEVEL}, {P_E1_AMT, 50},
                       {P_E1_VEL, 1}, {P_E1_DEC, 70}, {P_E1_SUS, 0}, {P_E2_AMT, 30}, {P_E2_DEC, 98}, {P_E2_SUS, 0},
                       {P_E2_REL, 84}, {P_E2_VEL, 1}, {P_FX_ON, 1}, {P_FX_TYPE, FX_DELAY}, {P_FX_SYNC, 1}, {P_FX_TIME, 76},
                       {P_FX_MIX, 26}, {P_FX_MISC, 50}, {P_REV_ON, 1}, {P_REV_MIX, 24}, {P_REV_DECAY, 76},
                       {P_VINTAGE, 20}, {P_SPREAD, 110}});
    add("Stab Machine", {{P_O1_SHAPE, SAW}, {P_O2_SHAPE, SAW}, {P_O2_PITCH, 12}, {P_CUTOFF, 360}, {P_RESO, 52}, {P_DRIVE, 127},
                         {P_KEYAMT, 60}, {P_E1_DEC, 60}, {P_E1_SUS, 0}, {P_E1_REL, 50}, {P_E1_AMT, 80}, {P_E1_VEL, 1},
                         {P_E2_DEC, 64}, {P_E2_SUS, 0}, {P_E2_REL, 54}, {P_UNISON, 1}, {P_UNI_VOICES, 5},
                         {P_UNI_DETUNE, 3}, {P_OVERDRIVE, 34}, {P_FX_ON, 1}, {P_FX_TYPE, FX_DELAY}, {P_FX_SYNC, 1},
                         {P_FX_TIME, 76}, {P_FX_MIX, 26}, {P_FX_MISC, 40}, {P_REV_ON, 1}, {P_REV_MIX, 14},
                         {P_REV_DECAY, 50}},
        {WHEEL_VIB, AT_CUT}, {}, {0, 3, 7, 10});
    add("Soft Pluck", {{P_O1_SHAPE, 34}, {P_O2_SHAPE, SQR}, {P_O2_LEVEL, 50}, {P_O2_PITCH, 5}, {P_CUTOFF, 400}, {P_RESO, 30},
                       {P_DRIVE, 60}, {P_KEYAMT, 80}, {P_E1_DEC, 64}, {P_E1_SUS, 0}, {P_E1_REL, 60}, {P_E1_AMT, 60},
                       {P_E1_VEL, 1}, {P_E2_DEC, 76}, {P_E2_SUS, 0}, {P_E2_REL, 68}, {P_FX_ON, 1}, {P_FX_TYPE, FX_BBD},
                       {P_FX_SYNC, 1}, {P_FX_TIME, 64}, {P_FX_MIX, 30}, {P_FX_MISC, 50}, {P_VINTAGE, 50}, {P_SPREAD, 90}});
    add("Cream Poly", {{P_O1_SHAPE, SAW}, {P_O2_SHAPE, SAW}, {P_O2_PITCH, 6}, {P_SUB, 50}, {P_CUTOFF, 540}, {P_RESO, 14},
                       {P_DRIVE, 112}, {P_KEYAMT, 50}, {P_E1_ATT, 10}, {P_E1_DEC, 76}, {P_E1_SUS, 60}, {P_E1_REL, 70},
                       {P_E1_AMT, 30}, {P_E1_VEL, 1}, {P_E2_ATT, 6}, {P_E2_SUS, 120}, {P_E2_REL, 72}, {P_UNISON, 0},
                       {P_FX_ON, 1}, {P_FX_TYPE, FX_CHORUS}, {P_FX_TIME, 20}, {P_FX_MIX, 70}, {P_FX_MISC, 60},
                       {P_REV_ON, 1}, {P_REV_MIX, 12}, {P_REV_DECAY, 52}, {P_VINTAGE, 60}, {P_SPREAD, 100}});
    add("Broken Toy", {{P_O1_SHAPE, SQR}, {P_O1_OCT, 1}, {P_O2_SHAPE, SINE}, {P_O2_OCT, 2}, {P_O2_PITCH, 13}, {P_O2_LEVEL, 70},
                       {P_FM, 1}, {P_CUTOFF, 640}, {P_DRIVE, 80}, {P_KEYAMT, 90}, {P_E1_DEC, 58}, {P_E1_SUS, 0},
                       {P_E1_AMT, 50}, {P_E1_VEL, 1}, {P_E2_DEC, 70}, {P_E2_SUS, 0}, {P_E2_REL, 60}, {P_FX_ON, 1},
                       {P_FX_TYPE, FX_LOFI}, {P_FX_TIME, 90}, {P_FX_MIX, 90}, {P_FX_MISC, 50}, {P_VINTAGE, 100},
                       {P_REV_ON, 1}, {P_REV_MIX, 14}, {P_REV_DECAY, 50}});

    /* ===================================================== BANK 2: basses and leads === */
    add("Fat Five Bass", {{P_O1_OCT, -2}, {P_O2_OCT, -2}, {P_O1_SHAPE, SAW}, {P_O2_SHAPE, SQR}, {P_O2_PITCH, 6}, {P_SUB, 110},
                          {P_CUTOFF, 300}, {P_RESO, 30}, {P_DRIVE, 127}, {P_KEYAMT, 40}, {P_E1_DEC, 64}, {P_E1_SUS, 20},
                          {P_E1_REL, 40}, {P_E1_AMT, 66}, {P_E1_VEL, 1}, {P_E2_DEC, 70}, {P_E2_SUS, 110}, {P_E2_REL, 36},
                          {P_UNISON, 1}, {P_UNI_VOICES, 3}, {P_UNI_DETUNE, 2}, {P_OVERDRIVE, 26}, {P_VINTAGE, 30}},
        {AT_CUT, {MS_MODWHEEL, 40, MD_RESO}});
    add("Rubber Bass", {{P_O1_OCT, -1}, {P_O1_SHAPE, SINE}, {P_O2_OCT, -1}, {P_O2_SHAPE, SINE}, {P_O2_LEVEL, 60}, {P_FM, 1},
                        {P_SUB, 80}, {P_CUTOFF, 380}, {P_RESO, 40}, {P_DRIVE, 100}, {P_ENV_ROUTING, ER_AUX_FILTERAMP},
                        {P_E1_DEST, MD_FM_LEVEL}, {P_E1_AMT, 50}, {P_E1_DEC, 56}, {P_E1_SUS, 0}, {P_E2_AMT, 60},
                        {P_E2_DEC, 66}, {P_E2_SUS, 60}, {P_E2_REL, 40}, {P_E2_VEL, 1}, {P_GLIDE, 1}, {P_GLIDE_RATE, 20},
                        {P_UNISON, 1}, {P_UNI_VOICES, 1}, {P_VINTAGE, 20}},
        {AT_CUT, {MS_MODWHEEL, 60, MD_FM_LEVEL}});
    add("Acid Five", {{P_O1_OCT, -1}, {P_O1_SHAPE, SAW}, {P_O2_LEVEL, 0}, {P_CUTOFF, 280}, {P_RESO, 96}, {P_DRIVE, 127},
                      {P_KEYAMT, 50}, {P_E1_DEC, 60}, {P_E1_SUS, 0}, {P_E1_REL, 40}, {P_E1_AMT, 70}, {P_E1_VEL, 1},
                      {P_E2_DEC, 64}, {P_E2_SUS, 80}, {P_E2_REL, 30}, {P_GLIDE, 1}, {P_GLIDE_RATE, 18}, {P_UNISON, 1},
                      {P_UNI_VOICES, 1}, {P_OVERDRIVE, 50}, {P_FX_ON, 1}, {P_FX_TYPE, FX_DELAY}, {P_FX_SYNC, 1},
                      {P_FX_TIME, 82}, {P_FX_MIX, 20}, {P_FX_MISC, 40}, {P_ARP_ON, 1}, {P_ARP_MODE, AM_UPDOWN},
                      {P_ARP_OCT, 2}, {P_DIVIDE, DV_16}, {P_BPM, 124}},
        {{MS_MODWHEEL, 60, MD_CUTOFF}, AT_CUT});
    add("Sub Growl", {{P_O1_OCT, -2}, {P_O1_SHAPE, SQR}, {P_O2_OCT, -2}, {P_O2_SHAPE, SAW}, {P_O2_PITCH, -8}, {P_SUB, 127},
                      {P_CUTOFF, 240}, {P_RESO, 50}, {P_DRIVE, 127}, {P_KEYAMT, 30}, {P_E1_DEC, 70}, {P_E1_SUS, 30},
                      {P_E1_AMT, 50}, {P_E2_SUS, 127}, {P_E2_REL, 30}, {P_LFO1 + LFO_N + LFO_FREQ, 62}, {P_LFO1 + LFO_N + LFO_AMT, 0}, {P_LFO1 + LFO_N + LFO_DEST, MD_CUTOFF}, {P_OVERDRIVE, 60},
                      {P_UNISON, 1}, {P_UNI_VOICES, 2}, {P_UNI_DETUNE, 3}, {P_VINTAGE, 50}},
        {{MS_MODWHEEL, 70, MD_LFO2_AMT}, AT_CUT});
    add("Pluck Bass", {{P_O1_OCT, -1}, {P_O2_OCT, -1}, {P_O1_SHAPE, SAW}, {P_O2_SHAPE, PUL}, {P_O2_PITCH, 4}, {P_SUB, 60},
                       {P_CUTOFF, 260}, {P_RESO, 40}, {P_DRIVE, 110}, {P_KEYAMT, 60}, {P_E1_DEC, 58}, {P_E1_SUS, 0},
                       {P_E1_REL, 40}, {P_E1_AMT, 76}, {P_E1_VEL, 1}, {P_E2_DEC, 72}, {P_E2_SUS, 30}, {P_E2_REL, 40},
                       {P_E2_VEL, 1}, {P_VINTAGE, 36}});
    add("Moog-ish Lead", {{P_O1_SHAPE, SAW}, {P_O2_SHAPE, SAW}, {P_O2_PITCH, -6}, {P_O2_OCT, -1}, {P_CUTOFF, 520}, {P_RESO, 30},
                          {P_DRIVE, 127}, {P_KEYAMT, 70}, {P_E1_ATT, 10}, {P_E1_DEC, 74}, {P_E1_SUS, 50}, {P_E1_AMT, 50},
                          {P_E2_SUS, 120}, {P_E2_REL, 50}, {P_UNISON, 1}, {P_UNI_VOICES, 3}, {P_UNI_DETUNE, 2},
                          {P_GLIDE, 1}, {P_GLIDE_RATE, 34}, {P_OVERDRIVE, 30}, {P_FX_ON, 1}, {P_FX_TYPE, FX_BBD},
                          {P_FX_SYNC, 1}, {P_FX_TIME, 64}, {P_FX_MIX, 24}, {P_FX_MISC, 40}, {P_REV_ON, 1},
                          {P_REV_MIX, 12}, {P_REV_DECAY, 50}, {P_VINTAGE, 40}});
    add("Square Lead", {{P_O1_SHAPE, SQR}, {P_O2_SHAPE, PUL}, {P_O2_PITCH, 8}, {P_CUTOFF, 640}, {P_RESO, 20}, {P_DRIVE, 90},
                        {P_KEYAMT, 60}, {P_E1_DEC, 70}, {P_E1_SUS, 60}, {P_E1_AMT, 30}, {P_E2_SUS, 120}, {P_E2_REL, 50},
                        {P_LFO1 + LFO_FREQ, 64}, {P_UNISON, 1}, {P_UNI_VOICES, 2}, {P_UNI_DETUNE, 1}, {P_GLIDE, 1},
                        {P_GLIDE_RATE, 26}, {P_FX_ON, 1}, {P_FX_TYPE, FX_TAPE2}, {P_FX_SYNC, 1}, {P_FX_TIME, 70},
                        {P_FX_MIX, 28}, {P_FX_MISC, 50}, {P_VINTAGE, 30}},
        {WHEEL_VIB, {MS_PRESSURE, 30, MD_LFO1_AMT}});
    add("Screamer", {{P_O1_SHAPE, SAW}, {P_O1_OCT, 1}, {P_SYNC, 1}, {P_O2_SHAPE, SAW}, {P_O2_LEVEL, 70}, {P_CUTOFF, 700},
                     {P_RESO, 60}, {P_DRIVE, 127}, {P_KEYAMT, 60}, {P_E1_DEC, 76}, {P_E1_SUS, 60}, {P_E1_AMT, 30},
                     {P_E2_SUS, 120}, {P_E2_REL, 50}, {P_OVERDRIVE, 90}, {P_UNISON, 1}, {P_UNI_VOICES, 2},
                     {P_UNI_DETUNE, 2}, {P_GLIDE, 1}, {P_GLIDE_RATE, 20}, {P_FX_ON, 1}, {P_FX_TYPE, FX_DELAY},
                     {P_FX_SYNC, 1}, {P_FX_TIME, 82}, {P_FX_MIX, 22}, {P_FX_MISC, 50}},
        {WHEEL_VIB, {MS_PRESSURE, 60, MD_OSC1_FINE}, {MS_LFO2, 8, MD_OSC2_FINE}});
    add("Whistle Lead", {{P_O1_SHAPE, SINE}, {P_O2_LEVEL, 0}, {P_O1_OCT, 1}, {P_NOISE, 10}, {P_CUTOFF, 800}, {P_DRIVE, 40},
                         {P_E2_ATT, 30}, {P_E2_SUS, 127}, {P_E2_REL, 56}, {P_E1_AMT, 0}, {P_LFO1 + LFO_FREQ, 68},
                         {P_LFO1 + LFO_AMT, 6}, {P_GLIDE, 1}, {P_GLIDE_RATE, 40}, {P_UNISON, 1}, {P_UNI_VOICES, 1},
                         {P_FX_ON, 1}, {P_FX_TYPE, FX_DELAY}, {P_FX_SYNC, 1}, {P_FX_TIME, 76}, {P_FX_MIX, 30},
                         {P_FX_MISC, 56}, {P_REV_ON, 1}, {P_REV_MIX, 22}, {P_REV_DECAY, 70}});
    add("Reso Bass", {{P_O1_OCT, -2}, {P_O1_SHAPE, SAW}, {P_O2_OCT, -1}, {P_O2_SHAPE, SQR}, {P_O2_LEVEL, 70}, {P_CUTOFF, 200},
                      {P_RESO, 80}, {P_DRIVE, 127}, {P_KEYAMT, 60}, {P_E1_DEC, 66}, {P_E1_SUS, 10}, {P_E1_AMT, 80},
                      {P_E1_VEL, 1}, {P_E2_SUS, 120}, {P_E2_REL, 30}, {P_OVERDRIVE, 20}, {P_VINTAGE, 30}});
    add("Wobble", {{P_O1_OCT, -2}, {P_O1_SHAPE, SAW}, {P_O2_OCT, -2}, {P_O2_SHAPE, SAW}, {P_O2_PITCH, 10}, {P_SUB, 100},
                   {P_CUTOFF, 220}, {P_RESO, 60}, {P_DRIVE, 127}, {P_E1_AMT, 0}, {P_E2_SUS, 127}, {P_E2_REL, 30},
                   {P_LFO1 + LFO_SHAPE, LS_TRI}, {P_LFO1 + LFO_SYNC, 1}, {P_LFO1 + LFO_FREQ, 7 * 8 + 4},
                   {P_LFO1 + LFO_AMT, 70}, {P_LFO1 + LFO_DEST, MD_CUTOFF}, {P_LFO1 + LFO_RESET, 1},
                   {P_OVERDRIVE, 50}, {P_UNISON, 1}, {P_UNI_VOICES, 2}, {P_UNI_DETUNE, 2}},
        {{MS_MODWHEEL, 60, MD_LFO1_FREQ}, AT_CUT});
    add("Octave Lead", {{P_O1_SHAPE, SAWSQ}, {P_O2_SHAPE, SAW}, {P_O2_OCT, 1}, {P_O2_PITCH, 4}, {P_O2_LEVEL, 90},
                        {P_CUTOFF, 600}, {P_RESO, 24}, {P_DRIVE, 100}, {P_KEYAMT, 60}, {P_E1_ATT, 20}, {P_E1_DEC, 74},
                        {P_E1_SUS, 60}, {P_E1_AMT, 36}, {P_E2_SUS, 120}, {P_E2_REL, 54}, {P_UNISON, 1},
                        {P_UNI_VOICES, 2}, {P_UNI_DETUNE, 3}, {P_GLIDE, 1}, {P_GLIDE_RATE, 30}, {P_FX_ON, 1},
                        {P_FX_TYPE, FX_CHORUS}, {P_FX_TIME, 30}, {P_FX_MIX, 60}, {P_FX_MISC, 50}, {P_REV_ON, 1},
                        {P_REV_MIX, 14}, {P_REV_DECAY, 56}, {P_VINTAGE, 40}});
    add("Mono Tom Bass", {{P_O1_OCT, -1}, {P_O1_SHAPE, SINE}, {P_O2_LEVEL, 0}, {P_SUB, 60}, {P_NOISE, 16}, {P_CUTOFF, 340},
                          {P_DRIVE, 127}, {P_ENV_ROUTING, ER_AUX_FILTERAMP}, {P_E1_DEST, MD_OSC1_FINE}, {P_E1_AMT, 50},
                          {P_E1_DEC, 50}, {P_E1_SUS, 0}, {P_E2_AMT, 60}, {P_E2_DEC, 70}, {P_E2_SUS, 0}, {P_E2_REL, 60},
                          {P_E2_VEL, 1}, {P_UNISON, 1}, {P_UNI_VOICES, 1}, {P_OVERDRIVE, 40}});
    add("Fifth Lead", {{P_O1_SHAPE, SAW}, {P_O2_SHAPE, SAW}, {P_O2_PITCH, 700}, {P_O2_LEVEL, 100}, {P_CUTOFF, 560},
                       {P_RESO, 30}, {P_DRIVE, 110}, {P_E1_DEC, 72}, {P_E1_SUS, 60}, {P_E1_AMT, 40}, {P_E2_SUS, 120},
                       {P_E2_REL, 50}, {P_UNISON, 1}, {P_UNI_VOICES, 3}, {P_UNI_DETUNE, 2}, {P_GLIDE, 1},
                       {P_GLIDE_RATE, 30}, {P_OVERDRIVE, 24}, {P_FX_ON, 1}, {P_FX_TYPE, FX_BBD}, {P_FX_SYNC, 1},
                       {P_FX_TIME, 64}, {P_FX_MIX, 24}, {P_FX_MISC, 40}, {P_VINTAGE, 40}});
    add("Dirt Bass", {{P_O1_OCT, -2}, {P_O1_SHAPE, PUL}, {P_O2_OCT, -1}, {P_O2_SHAPE, SAW}, {P_O2_PITCH, -5}, {P_O2_LEVEL, 80},
                      {P_SUB, 90}, {P_CUTOFF, 330}, {P_RESO, 20}, {P_DRIVE, 127}, {P_E1_DEC, 66}, {P_E1_SUS, 30},
                      {P_E1_AMT, 50}, {P_E1_VEL, 1}, {P_E2_SUS, 120}, {P_E2_REL, 34}, {P_OVERDRIVE, 80},
                      {P_FX_ON, 1}, {P_FX_TYPE, FX_DISTORTION}, {P_FX_TIME, 40}, {P_FX_MIX, 60}, {P_FX_MISC, 50},
                      {P_VINTAGE, 40}});
    add("Portamento", {{P_O1_SHAPE, SAW}, {P_O2_SHAPE, SQR}, {P_O2_PITCH, -5}, {P_CUTOFF, 540}, {P_RESO, 36}, {P_DRIVE, 100},
                       {P_E1_DEC, 74}, {P_E1_SUS, 50}, {P_E1_AMT, 40}, {P_E2_SUS, 120}, {P_E2_REL, 50}, {P_GLIDE, 1},
                       {P_GLIDE_RATE, 60}, {P_GLIDE_MODE, GM_TIME}, {P_FX_ON, 1}, {P_FX_TYPE, FX_TAPE1},
                       {P_FX_SYNC, 1}, {P_FX_TIME, 64}, {P_FX_MIX, 26}, {P_FX_MISC, 50}, {P_VINTAGE, 50},
                       {P_SPREAD, 80}});

    /* ===================================================== BANK 3: pads, strings, brass === */
    add("Warm Strings", {{P_O1_SHAPE, SAW}, {P_O2_SHAPE, SAW}, {P_O2_PITCH, 8}, {P_CUTOFF, 560}, {P_RESO, 8}, {P_DRIVE, 60},
                         {P_KEYAMT, 60}, {P_E1_ATT, 70}, {P_E1_DEC, 80}, {P_E1_SUS, 80}, {P_E1_REL, 80}, {P_E1_AMT, 24},
                         {P_E2_ATT, 66}, {P_E2_SUS, 120}, {P_E2_REL, 82}, {P_FX_ON, 1}, {P_FX_TYPE, FX_CHORUS},
                         {P_FX_TIME, 24}, {P_FX_MIX, 100}, {P_FX_MISC, 90}, {P_REV_ON, 1}, {P_REV_MIX, 20},
                         {P_REV_DECAY, 66}, {P_VINTAGE, 50}, {P_SPREAD, 110}});
    add("Slow Brass", {{P_O1_SHAPE, SAW}, {P_O2_SHAPE, SAW}, {P_O2_PITCH, 9}, {P_O1_OCT, -1}, {P_CUTOFF, 360}, {P_RESO, 16},
                       {P_DRIVE, 110}, {P_E1_ATT, 72}, {P_E1_DEC, 80}, {P_E1_SUS, 70}, {P_E1_REL, 72}, {P_E1_AMT, 70},
                       {P_E2_ATT, 64}, {P_E2_SUS, 120}, {P_E2_REL, 74}, {P_REV_ON, 1}, {P_REV_MIX, 16},
                       {P_REV_DECAY, 60}, {P_VINTAGE, 60}, {P_SPREAD, 80}});
    add("Vapor Pad", {{P_O1_SHAPE, 60}, {P_O2_SHAPE, PUL}, {P_O2_PITCH, -10}, {P_SUB, 40}, {P_CUTOFF, 460}, {P_RESO, 30},
                      {P_DRIVE, 70}, {P_E1_ATT, 82}, {P_E1_DEC, 90}, {P_E1_SUS, 60}, {P_E1_REL, 86}, {P_E1_AMT, 40},
                      {P_E2_ATT, 80}, {P_E2_SUS, 120}, {P_E2_REL, 88}, {P_LFO1 + LFO_FREQ, 20},
                      {P_LFO1 + LFO_AMT, 20}, {P_LFO1 + LFO_DEST, MD_OSC_SHAPE}, {P_FX_ON, 1}, {P_FX_TYPE, FX_TAPE2},
                      {P_FX_TIME, 90}, {P_FX_MIX, 40}, {P_FX_MISC, 60}, {P_REV_ON, 1}, {P_REV_MIX, 34},
                      {P_REV_DECAY, 86}, {P_VINTAGE, 80}, {P_SPREAD, 127}});
    add("Sweep Pad", {{P_O1_SHAPE, SAW}, {P_O2_SHAPE, SAW}, {P_O2_PITCH, 12}, {P_CUTOFF, 200}, {P_RESO, 70}, {P_DRIVE, 80},
                      {P_E1_ATT, 96}, {P_E1_DEC, 100}, {P_E1_SUS, 30}, {P_E1_REL, 90}, {P_E1_AMT, 80}, {P_E2_ATT, 70},
                      {P_E2_SUS, 120}, {P_E2_REL, 86}, {P_FX_ON, 1}, {P_FX_TYPE, FX_PHASER}, {P_FX_TIME, 10},
                      {P_FX_MIX, 60}, {P_FX_MISC, 60}, {P_REV_ON, 1}, {P_REV_MIX, 24}, {P_REV_DECAY, 74},
                      {P_VINTAGE, 50}, {P_SPREAD, 110}});
    add("Choir Ahh", {{P_O1_SHAPE, PUL}, {P_O2_SHAPE, 74}, {P_O2_PITCH, 6}, {P_NOISE, -12}, {P_CUTOFF, 430}, {P_RESO, 50},
                      {P_DRIVE, 40}, {P_KEYAMT, 60}, {P_E1_ATT, 70}, {P_E1_SUS, 90}, {P_E1_REL, 80}, {P_E1_AMT, 14},
                      {P_E2_ATT, 70}, {P_E2_SUS, 120}, {P_E2_REL, 84}, {P_LFO1 + LFO_FREQ, 54}, {P_LFO1 + LFO_AMT, 8},
                      {P_LFO1 + LFO_N + LFO_FREQ, 24}, {P_LFO1 + LFO_N + LFO_AMT, 10}, {P_LFO1 + LFO_N + LFO_DEST, MD_CUTOFF}, {P_FX_ON, 1}, {P_FX_TYPE, FX_CHORUS}, {P_FX_TIME, 26},
                      {P_FX_MIX, 110}, {P_FX_MISC, 90}, {P_REV_ON, 1}, {P_REV_MIX, 30}, {P_REV_DECAY, 78},
                      {P_VINTAGE, 40}, {P_SPREAD, 110}});
    add("Dust Pad", {{P_O1_SHAPE, SAW}, {P_O2_SHAPE, SINE}, {P_O2_OCT, 1}, {P_O2_LEVEL, 60}, {P_NOISE, -40}, {P_CUTOFF, 380},
                     {P_RESO, 40}, {P_DRIVE, 90}, {P_E1_ATT, 84}, {P_E1_SUS, 60}, {P_E1_REL, 86}, {P_E1_AMT, 30},
                     {P_E2_ATT, 84}, {P_E2_SUS, 120}, {P_E2_REL, 90}, {P_FX_ON, 1}, {P_FX_TYPE, FX_LOFI},
                     {P_FX_TIME, 70}, {P_FX_MIX, 80}, {P_FX_MISC, 40}, {P_REV_ON, 1}, {P_REV_MIX, 30},
                     {P_REV_DECAY, 84}, {P_VINTAGE, 100}, {P_SPREAD, 127}});
    add("Synth Horns", {{P_O1_SHAPE, SAW}, {P_O2_SHAPE, SAW}, {P_O2_PITCH, -7}, {P_CUTOFF, 420}, {P_RESO, 12}, {P_DRIVE, 120},
                        {P_KEYAMT, 70}, {P_E1_ATT, 44}, {P_E1_DEC, 70}, {P_E1_SUS, 60}, {P_E1_REL, 60}, {P_E1_AMT, 70},
                        {P_E1_VEL, 1}, {P_E2_ATT, 24}, {P_E2_SUS, 120}, {P_E2_REL, 60}, {P_OVERDRIVE, 16},
                        {P_REV_ON, 1}, {P_REV_MIX, 12}, {P_REV_DECAY, 54}, {P_VINTAGE, 66}, {P_SPREAD, 70}},
        {WHEEL_VIB, AT_CUT, {MS_VELOCITY, 20, MD_ENV1_AMT}});
    add("Wide Saws", {{P_O1_SHAPE, SAW}, {P_O2_SHAPE, SAW}, {P_O2_PITCH, 14}, {P_O1_PITCH, -6}, {P_SUB, 40}, {P_CUTOFF, 600},
                      {P_RESO, 10}, {P_DRIVE, 100}, {P_E1_ATT, 40}, {P_E1_SUS, 90}, {P_E1_AMT, 20}, {P_E2_ATT, 40},
                      {P_E2_SUS, 120}, {P_E2_REL, 80}, {P_FX_ON, 1}, {P_FX_TYPE, FX_CHORUS}, {P_FX_TIME, 16},
                      {P_FX_MIX, 80}, {P_FX_MISC, 70}, {P_REV_ON, 1}, {P_REV_MIX, 18}, {P_REV_DECAY, 64},
                      {P_VINTAGE, 70}, {P_SPREAD, 127}});
    add("Glacier", {{P_O1_SHAPE, SINE}, {P_O2_SHAPE, SAW}, {P_O2_OCT, 1}, {P_O2_PITCH, 4}, {P_O2_LEVEL, 50}, {P_CUTOFF, 520},
                    {P_RESO, 20}, {P_DRIVE, 50}, {P_E1_ATT, 90}, {P_E1_SUS, 80}, {P_E1_REL, 90}, {P_E1_AMT, 30},
                    {P_E2_ATT, 92}, {P_E2_SUS, 120}, {P_E2_REL, 96}, {P_FX_ON, 1}, {P_FX_TYPE, FX_FLANGER},
                    {P_FX_TIME, 6}, {P_FX_MIX, 60}, {P_FX_MISC, 70}, {P_REV_ON, 1}, {P_REV_MIX, 40},
                    {P_REV_DECAY, 92}, {P_REV_TONE, -10}, {P_VINTAGE, 30}, {P_SPREAD, 127}});
    add("Pulse Pad", {{P_O1_SHAPE, PUL}, {P_O2_SHAPE, PUL}, {P_O2_PITCH, 7}, {P_CUTOFF, 520}, {P_RESO, 14}, {P_DRIVE, 70},
                      {P_E1_ATT, 60}, {P_E1_SUS, 90}, {P_E1_AMT, 20}, {P_E2_ATT, 60}, {P_E2_SUS, 120}, {P_E2_REL, 80},
                      {P_LFO1 + LFO_FREQ, 34}, {P_LFO1 + LFO_AMT, 26}, {P_LFO1 + LFO_DEST, MD_OSC_SHAPE},
                      {P_FX_ON, 1}, {P_FX_TYPE, FX_CHORUS}, {P_FX_TIME, 20}, {P_FX_MIX, 70}, {P_FX_MISC, 60},
                      {P_REV_ON, 1}, {P_REV_MIX, 16}, {P_REV_DECAY, 60}, {P_VINTAGE, 50}, {P_SPREAD, 100}});
    add("Fifths Pad", {{P_O1_SHAPE, SAW}, {P_O2_SHAPE, SAWSQ}, {P_O2_PITCH, 702}, {P_O2_LEVEL, 90}, {P_CUTOFF, 440},
                       {P_RESO, 24}, {P_DRIVE, 90}, {P_E1_ATT, 80}, {P_E1_SUS, 60}, {P_E1_REL, 80}, {P_E1_AMT, 40},
                       {P_E2_ATT, 76}, {P_E2_SUS, 120}, {P_E2_REL, 84}, {P_FX_ON, 1}, {P_FX_TYPE, FX_BBD},
                       {P_FX_TIME, 70}, {P_FX_MIX, 26}, {P_FX_MISC, 50}, {P_REV_ON, 1}, {P_REV_MIX, 22},
                       {P_REV_DECAY, 72}, {P_VINTAGE, 60}, {P_SPREAD, 110}});
    add("Analog Chorus", {{P_O1_SHAPE, SAW}, {P_O2_LEVEL, 0}, {P_SUB, 40}, {P_CUTOFF, 620}, {P_RESO, 10}, {P_DRIVE, 64},
                          {P_E1_ATT, 30}, {P_E1_SUS, 80}, {P_E1_AMT, 20}, {P_E2_ATT, 30}, {P_E2_SUS, 120}, {P_E2_REL, 74},
                          {P_FX_ON, 1}, {P_FX_TYPE, FX_CHORUS}, {P_FX_TIME, 40}, {P_FX_MIX, 127}, {P_FX_MISC, 100},
                          {P_REV_ON, 1}, {P_REV_MIX, 12}, {P_REV_DECAY, 52}, {P_VINTAGE, 40}, {P_SPREAD, 90}});
    add("Breathing", {{P_O1_SHAPE, SAW}, {P_O2_SHAPE, SQR}, {P_O2_PITCH, 5}, {P_NOISE, 20}, {P_CUTOFF, 300}, {P_RESO, 50},
                      {P_DRIVE, 80}, {P_E1_ATT, 80}, {P_E1_DEC, 90}, {P_E1_SUS, 30}, {P_E1_REL, 84}, {P_E1_AMT, 60},
                      {P_E2_ATT, 70}, {P_E2_SUS, 120}, {P_E2_REL, 86}, {P_ENV_REPEAT, RP_ENV1},
                      {P_FX_ON, 1}, {P_FX_TYPE, FX_PHASER}, {P_FX_TIME, 16}, {P_FX_MIX, 60}, {P_FX_MISC, 50},
                      {P_REV_ON, 1}, {P_REV_MIX, 22}, {P_REV_DECAY, 72}, {P_VINTAGE, 60}, {P_SPREAD, 110}});
    add("Big Unison", {{P_O1_SHAPE, SAW}, {P_O2_SHAPE, SAW}, {P_O2_PITCH, 5}, {P_O2_OCT, -1}, {P_SUB, 60}, {P_CUTOFF, 520},
                       {P_RESO, 20}, {P_DRIVE, 127}, {P_E1_ATT, 30}, {P_E1_DEC, 80}, {P_E1_SUS, 60}, {P_E1_AMT, 40},
                       {P_E2_ATT, 20}, {P_E2_SUS, 120}, {P_E2_REL, 70}, {P_UNISON, 1}, {P_UNI_VOICES, 5},
                       {P_UNI_DETUNE, 4}, {P_OVERDRIVE, 20}, {P_REV_ON, 1}, {P_REV_MIX, 14}, {P_REV_DECAY, 60},
                       {P_VINTAGE, 50}, {P_SPREAD, 127}},
        {WHEEL_VIB, AT_CUT}, {}, {0, 7, 12});
    add("Haunted Air", {{P_O1_SHAPE, SINE}, {P_O2_SHAPE, SINE}, {P_O2_OCT, 1}, {P_O2_PITCH, 10}, {P_NOISE, -60},
                        {P_CUTOFF, 500}, {P_RESO, 90}, {P_DRIVE, 40}, {P_E1_ATT, 96}, {P_E1_SUS, 80}, {P_E1_REL, 96},
                        {P_E1_AMT, 20}, {P_E2_ATT, 96}, {P_E2_SUS, 120}, {P_E2_REL, 98}, {P_LFO1 + LFO_N + LFO_FREQ, 24}, {P_LFO1 + LFO_N + LFO_AMT, 10}, {P_LFO1 + LFO_N + LFO_DEST, MD_CUTOFF},
                        {P_FX_ON, 1}, {P_FX_TYPE, FX_TAPE2}, {P_FX_TIME, 100}, {P_FX_MIX, 36}, {P_FX_MISC, 70},
                        {P_REV_ON, 1}, {P_REV_MIX, 44}, {P_REV_DECAY, 96}, {P_VINTAGE, 90}, {P_SPREAD, 127}});

    /* ===================================================== BANK 4: FM, bells, percussion, motion === */
    add("FM Bells", {{P_O1_SHAPE, SINE}, {P_O2_SHAPE, SINE}, {P_O2_OCT, 1}, {P_O2_PITCH, 386}, {P_O2_LEVEL, 70}, {P_FM, 1},
                     {P_CUTOFF, 900}, {P_KEYAMT, 100}, {P_ENV_ROUTING, ER_AUX_FILTERAMP}, {P_E1_DEST, MD_FM_LEVEL},
                     {P_E1_AMT, 40}, {P_E1_VEL, 1}, {P_E1_DEC, 84}, {P_E1_SUS, 0}, {P_E2_AMT, 0}, {P_E2_DEC, 100},
                     {P_E2_SUS, 0}, {P_E2_REL, 90}, {P_E2_VEL, 1}, {P_FX_ON, 1}, {P_FX_TYPE, FX_DELAY},
                     {P_FX_SYNC, 1}, {P_FX_TIME, 76}, {P_FX_MIX, 20}, {P_FX_MISC, 40}, {P_REV_ON, 1},
                     {P_REV_MIX, 24}, {P_REV_DECAY, 80}, {P_SPREAD, 110}});
    add("Metal Mallet", {{P_O1_SHAPE, SINE}, {P_O2_SHAPE, SQR}, {P_O2_OCT, 2}, {P_O2_PITCH, 250}, {P_O2_LEVEL, 80}, {P_FM, 1},
                         {P_CUTOFF, 700}, {P_KEYAMT, 100}, {P_ENV_ROUTING, ER_AUX_FILTERAMP}, {P_E1_DEST, MD_FM_LEVEL},
                         {P_E1_AMT, 70}, {P_E1_VEL, 1}, {P_E1_DEC, 54}, {P_E1_SUS, 0}, {P_E2_AMT, 20}, {P_E2_DEC, 80},
                         {P_E2_SUS, 0}, {P_E2_REL, 74}, {P_E2_VEL, 1}, {P_FX_ON, 1}, {P_FX_TYPE, FX_BBD},
                         {P_FX_SYNC, 1}, {P_FX_TIME, 64}, {P_FX_MIX, 20}, {P_FX_MISC, 30}, {P_VINTAGE, 30},
                         {P_REV_ON, 1}, {P_REV_MIX, 16}, {P_REV_DECAY, 60}, {P_SPREAD, 100}});
    add("Marimba", {{P_O1_SHAPE, SINE}, {P_O2_SHAPE, SINE}, {P_O2_OCT, 2}, {P_O2_LEVEL, 40}, {P_FM, 1}, {P_CUTOFF, 540},
                    {P_KEYAMT, 100}, {P_E1_AMT, 50}, {P_E1_VEL, 1}, {P_E1_DEC, 54}, {P_E1_SUS, 0}, {P_E2_DEC, 68},
                    {P_E2_SUS, 0}, {P_E2_REL, 64}, {P_E2_VEL, 1}, {P_REV_ON, 1}, {P_REV_MIX, 14}, {P_REV_DECAY, 50},
                    {P_VINTAGE, 20}, {P_SPREAD, 90}},
        {{MS_MODWHEEL, 40, MD_FM_LEVEL}, AT_CUT});
    add("Kalimba", {{P_O1_SHAPE, SINE}, {P_O2_SHAPE, SINE}, {P_O2_OCT, 2}, {P_O2_PITCH, 300}, {P_O2_LEVEL, 30}, {P_FM, 1},
                    {P_CUTOFF, 640}, {P_KEYAMT, 100}, {P_E1_AMT, 40}, {P_E1_VEL, 1}, {P_E1_DEC, 50}, {P_E1_SUS, 0},
                    {P_E2_DEC, 74}, {P_E2_SUS, 0}, {P_E2_REL, 70}, {P_E2_VEL, 1}, {P_FX_ON, 1}, {P_FX_TYPE, FX_DELAY},
                    {P_FX_SYNC, 1}, {P_FX_TIME, 70}, {P_FX_MIX, 22}, {P_FX_MISC, 40}, {P_REV_ON, 1},
                    {P_REV_MIX, 14}, {P_REV_DECAY, 56}, {P_VINTAGE, 26}, {P_SPREAD, 100}});
    add("Ring Bell", {{P_O1_SHAPE, SQR}, {P_O2_SHAPE, SINE}, {P_O2_OCT, 1}, {P_O2_PITCH, 530}, {P_CUTOFF, 700},
                      {P_E1_AMT, 30}, {P_E1_DEC, 70}, {P_E1_SUS, 0}, {P_E2_DEC, 90}, {P_E2_SUS, 0}, {P_E2_REL, 84},
                      {P_FX_ON, 1}, {P_FX_TYPE, FX_RINGMOD}, {P_FX_TIME, 70}, {P_FX_MIX, 60}, {P_FX_MISC, 60},
                      {P_REV_ON, 1}, {P_REV_MIX, 20}, {P_REV_DECAY, 70}, {P_SPREAD, 100}});
    add("Synth Kick", {{P_O1_OCT, -2}, {P_O1_SHAPE, SINE}, {P_O2_LEVEL, 0}, {P_CUTOFF, 400}, {P_DRIVE, 127},
                       {P_ENV_ROUTING, ER_AUX_FILTERAMP}, {P_E1_DEST, MD_OSC1_OCT}, {P_E1_AMT, 70}, {P_E1_DEC, 46},
                       {P_E1_SUS, 0}, {P_E2_AMT, 40}, {P_E2_DEC, 68}, {P_E2_SUS, 0}, {P_E2_REL, 60}, {P_E2_VEL, 1},
                       {P_OVERDRIVE, 70}, {P_UNISON, 1}, {P_UNI_VOICES, 1}});
    add("Noise Snare", {{P_O1_SHAPE, SINE}, {P_O2_LEVEL, 0}, {P_NOISE, 120}, {P_CUTOFF, 760}, {P_RESO, 30}, {P_DRIVE, 100},
                        {P_E1_AMT, 40}, {P_E1_DEC, 50}, {P_E1_SUS, 0}, {P_E2_DEC, 60}, {P_E2_SUS, 0}, {P_E2_REL, 56},
                        {P_E2_VEL, 1}, {P_OVERDRIVE, 40}, {P_REV_ON, 1}, {P_REV_MIX, 18}, {P_REV_DECAY, 40}});
    add("Synth Toms", {{P_O1_SHAPE, SINE}, {P_O2_SHAPE, SINE}, {P_O2_PITCH, 30}, {P_NOISE, 14}, {P_CUTOFF, 500}, {P_DRIVE, 110},
                       {P_ENV_ROUTING, ER_AUX_FILTERAMP}, {P_E1_DEST, MD_OSC_FINE}, {P_E1_AMT, 50}, {P_E1_DEC, 60},
                       {P_E1_SUS, 0}, {P_E2_AMT, 40}, {P_E2_DEC, 70}, {P_E2_SUS, 0}, {P_E2_REL, 64}, {P_E2_VEL, 1},
                       {P_REV_ON, 1}, {P_REV_MIX, 16}, {P_REV_DECAY, 50}, {P_SPREAD, 100}});
    add("Hat Machine", {{P_O1_SHAPE, SQR}, {P_O1_OCT, 2}, {P_O2_OCT, 2}, {P_O2_PITCH, 421}, {P_O2_SHAPE, SQR}, {P_NOISE, 90},
                        {P_CUTOFF, 1000}, {P_RESO, 20}, {P_E2_DEC, 46}, {P_E2_SUS, 0}, {P_E2_REL, 44}, {P_E2_VEL, 1},
                        {P_E1_AMT, 0}, {P_FX_ON, 1}, {P_FX_TYPE, FX_HPF}, {P_FX_TIME, 100}, {P_FX_MIX, 127},
                        {P_FX_MISC, 30}, {P_ARP_ON, 1}, {P_ARP_MODE, AM_UP}, {P_DIVIDE, DV_16}, {P_BPM, 120},
                        {P_DRIVE, 90}, {P_OVERDRIVE, 30}});
    add("Pulse Arp", {{P_O1_SHAPE, PUL}, {P_O2_SHAPE, SAW}, {P_O2_PITCH, 7}, {P_O2_OCT, 1}, {P_O2_LEVEL, 60}, {P_CUTOFF, 440},
                      {P_RESO, 50}, {P_DRIVE, 100}, {P_KEYAMT, 60}, {P_E1_DEC, 58}, {P_E1_SUS, 0}, {P_E1_AMT, 60},
                      {P_E1_VEL, 1}, {P_E2_DEC, 64}, {P_E2_SUS, 0}, {P_E2_REL, 54}, {P_ARP_ON, 1},
                      {P_ARP_MODE, AM_UPDOWN}, {P_ARP_OCT, 2}, {P_DIVIDE, DV_16}, {P_BPM, 116}, {P_FX_ON, 1},
                      {P_FX_TYPE, FX_DELAY}, {P_FX_SYNC, 1}, {P_FX_TIME, 82}, {P_FX_MIX, 26}, {P_FX_MISC, 50},
                      {P_VINTAGE, 40}, {P_SPREAD, 90}},
        {{MS_MODWHEEL, 60, MD_CUTOFF}, AT_CUT});
    add("Sequence One", {{P_O1_SHAPE, SAW}, {P_O2_SHAPE, SQR}, {P_O2_PITCH, -6}, {P_CUTOFF, 380}, {P_RESO, 60}, {P_DRIVE, 120},
                         {P_E1_DEC, 58}, {P_E1_SUS, 0}, {P_E1_AMT, 66}, {P_E1_VEL, 1}, {P_E2_DEC, 62}, {P_E2_SUS, 0},
                         {P_E2_REL, 50}, {P_SEQ_PLAY, 1}, {P_DIVIDE, DV_16}, {P_BPM, 118}, {P_FX_ON, 1},
                         {P_FX_TYPE, FX_BBD}, {P_FX_SYNC, 1}, {P_FX_TIME, 76}, {P_FX_MIX, 24}, {P_FX_MISC, 50},
                         {P_VINTAGE, 40}, {P_SPREAD, 70}},
        {{MS_MODWHEEL, 60, MD_CUTOFF}, AT_CUT},
        {{36}, {48}, {43}, {48}, {36}, {46}, {48}, {51}, {36}, {48}, {43}, {48}, {39}, {46}, {48}, {55}});
    add("Echo Chords", {{P_O1_SHAPE, SAW}, {P_O2_SHAPE, SQR}, {P_O2_PITCH, 7}, {P_CUTOFF, 470}, {P_RESO, 30}, {P_DRIVE, 90},
                        {P_E1_DEC, 64}, {P_E1_SUS, 10}, {P_E1_AMT, 56}, {P_E1_VEL, 1}, {P_E2_DEC, 70}, {P_E2_SUS, 0},
                        {P_E2_REL, 60}, {P_FX_ON, 1}, {P_FX_TYPE, FX_TAPE1}, {P_FX_SYNC, 1}, {P_FX_TIME, 64},
                        {P_FX_MIX, 40}, {P_FX_MISC, 70}, {P_VINTAGE, 50}, {P_SPREAD, 110}});
    add("Laser Zap", {{P_O1_SHAPE, SAW}, {P_O1_OCT, 1}, {P_O2_SHAPE, SINE}, {P_O2_LEVEL, 80}, {P_FM, 1}, {P_CUTOFF, 800},
                      {P_RESO, 50}, {P_DRIVE, 90}, {P_ENV_ROUTING, ER_AUX_FILTERAMP}, {P_E1_DEST, MD_OSC1_OCT},
                      {P_E1_AMT, -60}, {P_E1_DEC, 60}, {P_E1_SUS, 0}, {P_E2_AMT, 30}, {P_E2_DEC, 72}, {P_E2_SUS, 0},
                      {P_E2_REL, 60}, {P_FX_ON, 1}, {P_FX_TYPE, FX_FLANGER}, {P_FX_TIME, 30}, {P_FX_MIX, 60},
                      {P_FX_MISC, 90}, {P_SPREAD, 110}});
    add("Wind Tunnel", {{P_O1_LEVEL, 0}, {P_O2_LEVEL, 0}, {P_NOISE, -127}, {P_CUTOFF, 400}, {P_RESO, 100}, {P_DRIVE, 40},
                        {P_E1_AMT, 0}, {P_E2_ATT, 80}, {P_E2_SUS, 120}, {P_E2_REL, 90}, {P_LFO1 + LFO_FREQ, 14},
                        {P_LFO1 + LFO_AMT, 60}, {P_LFO1 + LFO_DEST, MD_CUTOFF}, {P_LFO1 + LFO_SHAPE, LS_SH},
                        {P_LFO1 + LFO_SLEW, 90}, {P_FX_ON, 1}, {P_FX_TYPE, FX_PHASER}, {P_FX_TIME, 8},
                        {P_FX_MIX, 80}, {P_FX_MISC, 60}, {P_REV_ON, 1}, {P_REV_MIX, 30}, {P_REV_DECAY, 80},
                        {P_SPREAD, 127}},
        {{MS_MODWHEEL, 80, MD_LFO1_FREQ}, AT_CUT});
    add("Random Bloops", {{P_O1_SHAPE, SINE}, {P_O2_SHAPE, SQR}, {P_O2_OCT, 1}, {P_O2_LEVEL, 50}, {P_FM, 1}, {P_CUTOFF, 520},
                          {P_RESO, 70}, {P_DRIVE, 80}, {P_E1_DEC, 56}, {P_E1_SUS, 0}, {P_E1_AMT, 50}, {P_E2_DEC, 60},
                          {P_E2_SUS, 0}, {P_E2_REL, 60}, {P_ARP_ON, 1}, {P_ARP_MODE, AM_RANDOM}, {P_ARP_OCT, 3},
                          {P_DIVIDE, DV_16}, {P_BPM, 110}, {P_FX_ON, 1}, {P_FX_TYPE, FX_DELAY}, {P_FX_SYNC, 1},
                          {P_FX_TIME, 76}, {P_FX_MIX, 30}, {P_FX_MISC, 60}, {P_REV_ON, 1}, {P_REV_MIX, 16},
                          {P_REV_DECAY, 60}, {P_SPREAD, 127}},
        {{MS_RANDOM, 40, MD_CUTOFF}, {MS_RANDOM, 30, MD_FM_LEVEL}});
    add("Crushed Lead", {{P_O1_SHAPE, SQR}, {P_O2_SHAPE, SAW}, {P_O2_PITCH, 12}, {P_CUTOFF, 640}, {P_RESO, 40}, {P_DRIVE, 127},
                         {P_E1_DEC, 70}, {P_E1_SUS, 60}, {P_E1_AMT, 30}, {P_E2_SUS, 120}, {P_E2_REL, 50}, {P_UNISON, 1},
                         {P_UNI_VOICES, 2}, {P_UNI_DETUNE, 2}, {P_GLIDE, 1}, {P_GLIDE_RATE, 24}, {P_OVERDRIVE, 50},
                         {P_FX_ON, 1}, {P_FX_TYPE, FX_LOFI}, {P_FX_TIME, 40}, {P_FX_MIX, 100}, {P_FX_MISC, 90},
                         {P_VINTAGE, 60}},
        {WHEEL_VIB, {MS_PRESSURE, 50, MD_OVERDRIVE}});
    return d;
}

/* PROGRAM VOLUME each, matched by a stand-in for the ear: the loudest 300 ms of a chord (or the arp, the sequence)
 * about -16 dBFS, no peak over -3 dBFS (a pluck may stay under, PROGRAM VOLUME ends at 127) */
const int16_t VOL[] = {70, 56, 65, 96, 77, 105, 85, 63, 94, 52, 63, 98, 127, 83, 74, 99, 90, 127, 110, 97, 100, 76, 67, 96, 71, 72, 91, 102, 112, 85, 87, 71, 63, 60, 55, 110, 101, 70, 64, 67, 85, 60, 58, 62, 107, 92, 90, 95, 127, 127, 127, 122, 118, 119, 98, 127, 127, 111, 110, 112, 97, 119, 98};

std::vector<Program> build()
{
    std::vector<Program> v;
    size_t i = 0;
    for (auto &d : defs()) {
        v.push_back(make(d));
        if (i < sizeof VOL / sizeof VOL[0])
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
