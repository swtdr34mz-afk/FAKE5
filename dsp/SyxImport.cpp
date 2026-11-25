/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (FAKE5) */
/* The Take 5's program dump, as read from its factory sets (Sequential's packed MS-bit format: 8 bytes carry 7; the
 * unpacked bytes, found by comparing 256 programs against what they are):
 *
 *    0,1 osc 1/2 octave (0..4, 2 = 8')     2..5 fine (lo 2,3, hi 4,5; minus 700 = cents)   6,7 shape 0..254
 *    8,9 glide rate per osc   10,11 key follow   12 sync   19 glide mode   20 glide   21,22 bend up / down
 *    23 osc 1  24 osc 2  25 sub  26 noise (64 off, below pink, above white)   27,28 cutoff lo / hi (0..1023)
 *    29 resonance 0..255  30 drive  31 key amount  34 program volume  35 unison  36 its voices  37 its detune
 *    43 vintage  44 overdrive  45..51 effect: on, type (1..), mix, time, misc, sync, sync rate
 *    54 reverb on  56..60 mix, damping, pre-delay, decay, tone   63..78 the LFOs (freq, amount at 127 = 0, shape,
 *    sync, destination, sync rate, key reset, slew: each a pair)   83,84 env amounts  86,87 velocity  89,90 attack
 *    92,93 decay  95,96 sustain  98,99 release  107 routing  108 env 1's aux destination
 *    109.. 125.. 157.. the 16 slots' sources, amounts (127 = 0), destinations   173 bpm  175 arp  177 sequencer
 *    181 env repeat  182 osc 2 filter bypass   195 the name (20)   218 FM   219..222 arp mode, octaves, repeats,
 *    clock divide   313.. the sequence: 5 tracks of 64 notes and 64 velocities
 *
 * Destinations are stored without Vintage and LFO All Freq (55 of the appendix's 57). */
#include "SyxImport.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace f5
{
namespace
{
int clampi(int x, int lo, int hi) { return x < lo ? lo : x > hi ? hi : x; }
int h(int v) { return clampi((int)std::lround(v * 127.0 / 255.0), 0, 127); }   /* 0..255 to 0..127 */
int dest(int s)
{
    if (s <= 16)
        return s;
    if (s <= 27)
        return s + 1;
    if (s == 28)
        return MD_LFO2_FREQ;
    return clampi(s + 2, 0, NMODDST - 1);
}

bool convert(const std::vector<uint8_t> &d, Program &p)
{
    if (d.size() < 313 + 5 * 128)
        return false;
    programInit(p);
    int16_t *q = p.p;
    auto put = [q](int i, int v) { q[i] = (int16_t)clampi(v, PARAMS[i].lo, PARAMS[i].hi); };
    /* the name */
    char nm[NAME_LEN + 1] = {};
    for (int i = 0; i < NAME_LEN; i++) {
        uint8_t c = d[195 + (size_t)i];
        nm[i] = c >= 32 && c < 127 ? (char)c : (c == 0 ? 0 : ' ');
        if (!c)
            break;
    }
    int e = (int)std::strlen(nm);
    while (e > 0 && nm[e - 1] == ' ')
        nm[--e] = 0;
    std::snprintf(p.name, NAME_LEN, "%s", nm[0] ? nm : "Take 5");
    put(P_O1_OCT, d[0] - 2);
    put(P_O2_OCT, d[1] - 2);
    put(P_O1_PITCH, d[2] + 256 * d[4] - 700);
    put(P_O2_PITCH, d[3] + 256 * d[5] - 700);
    put(P_O1_SHAPE, (d[6] + 1) / 2);
    put(P_O2_SHAPE, (d[7] + 1) / 2);
    put(P_O1_KEY, d[10]);
    put(P_O2_KEY, d[11]);
    put(P_SYNC, d[12]);
    put(P_GLIDE_MODE, d[19]);
    put(P_GLIDE, d[20]);
    int g = std::max(d[8], d[9]);
    put(P_GLIDE_RATE, g);
    put(P_O1_GLIDE, g ? 127 * d[8] / g : 127);
    put(P_O2_GLIDE, g ? 127 * d[9] / g : 127);
    put(P_PB_UP, d[21]);
    put(P_PB_DOWN, d[22]);
    put(P_O1_LEVEL, d[23]);
    put(P_O2_LEVEL, d[24]);
    put(P_SUB, d[25]);
    int nz = d[26] - 64;
    put(P_NOISE, std::abs(nz) <= 1 ? 0 : nz * 2);
    put(P_FM, d[218] ? 1 : 0);
    put(P_CUTOFF, d[27] + 256 * d[28]);
    put(P_RESO, h(d[29]));
    put(P_DRIVE, d[30]);
    put(P_KEYAMT, d[31]);
    put(P_PGM_VOL, d[34]);
    put(P_UNISON, d[35]);
    put(P_UNI_VOICES, d[36] + 1);
    put(P_UNI_DETUNE, d[37]);
    put(P_VINTAGE, d[43]);
    put(P_OVERDRIVE, d[44]);
    put(P_FX_ON, d[45]);
    put(P_FX_TYPE, d[46] - 1);
    put(P_FX_MIX, d[47]);
    put(P_FX_SYNC, d[50]);
    put(P_FX_TIME, d[50] ? clampi(d[51], 0, 10) * 128 / 11 + 5 : h(d[48]));
    put(P_FX_MISC, h(d[49]));
    put(P_REV_ON, d[54] && d[56] > 2);
    put(P_REV_MIX, d[56]);
    put(P_REV_DAMP, h(d[57]));
    put(P_REV_PREDELAY, h(d[58]));
    put(P_REV_DECAY, h(d[59]));
    put(P_REV_TONE, (int)std::lround((d[60] - 64) / 3.0));
    for (int l = 0; l < 2; l++) {
        int b = P_LFO1 + l * LFO_N;
        put(b + LFO_SYNC, d[69 + (size_t)l]);
        put(b + LFO_FREQ, d[69 + (size_t)l] ? clampi(d[73 + (size_t)l], 0, 15) * 8 + 4 : h(d[63 + (size_t)l]));
        put(b + LFO_AMT, d[65 + (size_t)l] - 127);
        put(b + LFO_SHAPE, d[67 + (size_t)l]);
        put(b + LFO_DEST, dest(d[71 + (size_t)l]));
        put(b + LFO_RESET, d[75 + (size_t)l]);
        put(b + LFO_SLEW, d[77 + (size_t)l]);
    }
    put(P_E1_AMT, d[83] - 127);
    put(P_E2_AMT, d[84]);
    put(P_E1_VEL, d[86]);
    put(P_E2_VEL, d[87]);
    put(P_E1_DEL, 0);
    put(P_E2_DEL, 0);
    put(P_E1_ATT, d[89]);
    put(P_E2_ATT, d[90]);
    put(P_E1_DEC, d[92]);
    put(P_E2_DEC, d[93]);
    put(P_E1_SUS, d[95]);
    put(P_E2_SUS, d[96]);
    put(P_E1_REL, d[98]);
    put(P_E2_REL, d[99]);
    put(P_ENV_ROUTING, d[107]);
    put(P_E1_DEST, dest(d[108]));
    put(P_BPM, d[173]);
    put(P_DIVIDE, d[222]);
    put(P_ARP_ON, d[175]);
    put(P_ARP_MODE, d[219]);
    put(P_ARP_OCT, d[220] + 1);
    put(P_ARP_REPEAT, d[221]);
    put(P_SEQ_PLAY, d[177]);
    put(P_ENV_REPEAT, d[181]);
    put(P_O2_BYPASS, d[182]);
    put(P_SPREAD, 0);
    /* the slots (Voice Spread to Panning becomes VOICE SPREAD) */
    for (int s = 0; s < NMOD; s++) {
        int src = d[109 + (size_t)s], amt = d[125 + (size_t)s] - 127, dst = d[157 + (size_t)s];
        q[P_MOD1 + s * MOD_N + MOD_SRC] = MS_OFF;
        q[P_MOD1 + s * MOD_N + MOD_AMT] = 0;
        q[P_MOD1 + s * MOD_N + MOD_DST] = MD_OFF;
        if (src <= 0 || src >= NMODSRC || dst <= 0 || amt == 0)
            continue;
        int md = dest(dst);
        if (src == MS_SPREAD && md == MD_PAN) {
            put(P_SPREAD, std::abs(amt) + 20);
            continue;
        }
        q[P_MOD1 + s * MOD_N + MOD_SRC] = (int16_t)src;
        q[P_MOD1 + s * MOD_N + MOD_AMT] = (int16_t)clampi(amt, -127, 127);
        q[P_MOD1 + s * MOD_N + MOD_DST] = (int16_t)md;
    }
    /* the sequence: a step all of whose tracks are left as made (C4 at 100) rests */
    seqClear(p.seq);
    if (d[177]) {
        int last = -1;
        for (int st = 0; st < SEQ_STEPS; st++) {
            int n = 0;
            for (int t = 0; t < 5; t++) {
                int nt = d[313 + (size_t)(t * 128 + st)], vl = d[313 + (size_t)(t * 128 + 64 + st)];
                if ((nt == 60 && vl == 100) || nt <= 0 || nt > 125)
                    continue;
                p.seq.note[st][n] = (uint8_t)(nt + 2);
                p.seq.vel[st][n] = (uint8_t)clampi(vl, 1, 127);
                n++;
            }
            if (n)
                last = st;
        }
        p.seq.length = (uint8_t)(last < 0 ? 0 : (last / 4 + 1) * 4);
    }
    return true;
}
} // namespace

std::vector<Program> importTake5Syx(const uint8_t *data, size_t size)
{
    std::vector<Program> out;
    size_t i = 0;
    while (i < size) {
        while (i < size && data[i] != 0xF0)
            i++;
        size_t s = i;
        while (i < size && data[i] != 0xF7)
            i++;
        if (i >= size)
            break;
        size_t e = i++;
        if (e - s < 100 || data[s + 1] != 0x01 || data[s + 2] != 0x35)
            continue;
        size_t at = data[s + 3] == 0x02 ? s + 6 : data[s + 3] == 0x03 ? s + 4 : 0;
        if (!at)
            continue;
        std::vector<uint8_t> d;
        d.reserve(4200);
        for (size_t k = at; k < e; k += 8) {
            uint8_t hb = data[k];
            for (size_t n = 0; n < 7 && k + 1 + n < e; n++)
                d.push_back((uint8_t)(data[k + 1 + n] | (((hb >> n) & 1) << 7)));
        }
        Program p;
        if (convert(d, p))
            out.push_back(p);
    }
    return out;
}
} // namespace f5
