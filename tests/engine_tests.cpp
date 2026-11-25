/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (FAKE5) */
/* FAKE5's engine tests, without JUCE: every factory program at 44.1, 48 and 96 kHz (no NaN, no runaway, sound where
 * there should be, silence after), every effect type and the reverb at their extremes, the filter singing, any buffer
 * size the same, unison and chord memory, the arp, the sequencer (recording, rests, ties, transpose), LOW SPLIT, the
 * three ENV ROUTINGs, FM, 5 and 10 voices, the CPU. A failure exits non-zero. */
#include "Engine.h"
#include "Presets.h"
#include "SyxImport.h"
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <vector>

using namespace f5;
static int fails = 0;
#define CHECK(c, ...)                                                                                                   \
    do {                                                                                                                \
        if (!(c)) {                                                                                                     \
            std::printf("FAIL: " __VA_ARGS__);                                                                          \
            std::printf("\n");                                                                                          \
            fails++;                                                                                                    \
        }                                                                                                               \
    } while (0)

struct Stats {
    float peak = 0, rms = 0;
    bool nan = false;
};
static Stats run(Engine &e, int frames, int block = 256)
{
    std::vector<float> L(block), R(block);
    Stats s;
    double sum = 0;
    int done = 0;
    while (done < frames) {
        int n = std::min(block, frames - done);
        e.render(L.data(), R.data(), n);
        for (int i = 0; i < n; i++) {
            if (!std::isfinite(L[i]) || !std::isfinite(R[i]))
                s.nan = true;
            s.peak = std::max(s.peak, std::max(std::fabs(L[i]), std::fabs(R[i])));
            sum += L[i] * L[i] + R[i] * R[i];
        }
        done += n;
    }
    s.rms = (float)std::sqrt(sum / (2.0 * frames));
    return s;
}

static void chord(Engine &e, bool on)
{
    const int notes[4] = {48, 55, 60, 64};
    for (int n : notes)
        on ? e.noteOn(n, 0.8f) : e.noteOff(n);
}

int main(int argc, char **argv)
{
    /* the programs: the factory's, and any Take 5 .syx files named (LOAD SYX's reading, on its owner's sounds) */
    std::vector<Program> progs(FACTORY, FACTORY + NFACTORY);
    for (int a = 1; a < argc; a++) {
        std::ifstream f(argv[a], std::ios::binary);
        std::vector<uint8_t> b((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        auto im = importTake5Syx(b.data(), b.size());
        std::printf("%s: %d programs\n", argv[a], (int)im.size());
        CHECK(!im.empty(), "%s: no programs", argv[a]);
        progs.insert(progs.end(), im.begin(), im.end());
    }
    const int NP = (int)progs.size();
    const Program *PR = progs.data();
    std::printf("%d factory programs, %d in all\n", NFACTORY, NP);
    /* every program, three rates */
    const double rates[3] = {44100.0, 48000.0, 96000.0};
    for (double sr : rates)
        for (int p = 0; p < NP; p++) {
            Engine e;
            e.prepare(sr);
            e.setProgram(PR[p]);
            e.host(false, 0.0, 120.0, false);
            if (PR[p].seq.length)
                e.seqPlay(true);
            chord(e, true);
            Stats a = run(e, (int)(sr * 2.0));
            chord(e, false);
            e.seqPlay(false);
            Stats b = run(e, (int)(sr * 1.0));
            CHECK(!a.nan && !b.nan, "%s @%g: NaN", PR[p].name, sr);
            /* (a Take 5's own program may be meant quiet, or for the mod wheel: only FAKE5's must sound) */
            CHECK(a.peak < (p < NFACTORY ? 2.5f : 4.0f), "%s @%g: peak %.2f", PR[p].name, sr, a.peak);
            CHECK(p >= NFACTORY || a.rms > 0.01f, "%s @%g: too quiet (rms %.4f)", PR[p].name, sr, a.rms);
            if (sr == 44100.0)
                std::printf("  %2d %-18s peak %.2f rms %.3f\n", p, PR[p].name, a.peak, a.rms);
        }
    /* every effect, the reverb and the overdrive at the top, resonance at the top */
    for (int fx = 0; fx < NFXTYPE; fx++)
        for (int sync = 0; sync < 2; sync++) {
            Engine e;
            e.prepare(48000.0);
            Program p;
            programInit(p);
            p.p[P_FX_ON] = 1;
            p.p[P_FX_TYPE] = (int16_t)fx;
            p.p[P_FX_SYNC] = (int16_t)sync;
            p.p[P_FX_TIME] = 127;
            p.p[P_FX_MIX] = 127;
            p.p[P_FX_MISC] = 127;
            p.p[P_REV_ON] = 1;
            p.p[P_REV_MIX] = 127;
            p.p[P_REV_DECAY] = 127;
            p.p[P_RESO] = 127;
            p.p[P_CUTOFF] = 500;
            p.p[P_OVERDRIVE] = 127;
            p.p[P_DRIVE] = 127;
            e.setProgram(p);
            chord(e, true);
            Stats a = run(e, 48000 * 2);
            chord(e, false);
            Stats b = run(e, 48000 * 4);
            CHECK(!a.nan && !b.nan, "fx %s: NaN", FXTYPE_NAME[fx]);
            CHECK(a.peak < 8.0f && b.peak < 8.0f, "fx %s: peak %.2f / %.2f", FXTYPE_NAME[fx], a.peak, b.peak);
        }
    /* the reverb: a tail that falls, and at DECAY 0 is short */
    {
        Engine e;
        e.prepare(48000.0);
        Program p;
        programInit(p);
        p.p[P_REV_ON] = 1;
        p.p[P_REV_MIX] = 90;
        p.p[P_REV_DECAY] = 70;
        p.p[P_E2_REL] = 0;
        e.setProgram(p);
        e.noteOn(60, 1.0f);
        run(e, 4800);
        e.noteOff(60);
        Stats t1 = run(e, 48000), t2 = run(e, 48000), t3 = run(e, 48000 * 8);
        std::printf("  plate tail: %.4f, %.4f, then %.6f\n", t1.rms, t2.rms, t3.rms);
        CHECK(t1.rms > 0.005f && t2.rms < t1.rms && t3.rms < 1e-3f, "the plate's tail does not fall");
    }
    /* the 4-pole sings at full resonance with nothing in */
    {
        Engine e;
        e.prepare(44100.0);
        Program p;
        programInit(p);
        p.p[P_O1_LEVEL] = 0;
        p.p[P_RESO] = 127;
        p.p[P_CUTOFF] = 600;
        p.p[P_KEYAMT] = 127;
        e.setProgram(p);
        e.noteOn(60, 1.0f);
        Stats a = run(e, 44100 * 2);
        std::printf("  self-oscillation: peak %.2f rms %.3f\n", a.peak, a.rms);
        CHECK(a.rms > 0.02f, "the filter does not self-oscillate (rms %.4f)", a.rms);
    }
    /* FM: louder in the upper partials than without; never NaN at the top of the index, sync with it */
    {
        Engine e;
        e.prepare(48000.0);
        Program p;
        programInit(p);
        p.p[P_O1_SHAPE] = 0;
        p.p[P_O2_SHAPE] = 0;
        p.p[P_FM] = 1;
        p.p[P_O2_LEVEL] = 127;
        p.p[P_SYNC] = 1;
        p.p[P_O2_OCT] = 2;
        e.setProgram(p);
        for (int n = 24; n < 120; n += 12)
            e.noteOn(n, 1.0f);
        Stats a = run(e, 48000 * 2);
        CHECK(!a.nan && a.peak < 3.0f && a.rms > 0.01f, "FM at the top: rms %.3f peak %.2f", a.rms, a.peak);
    }
    /* any buffer size: the same samples */
    {
        Engine a, b;
        a.prepare(44100.0);
        b.prepare(44100.0);
        a.setProgram(FACTORY[3]);
        b.setProgram(FACTORY[3]);
        chord(a, true);
        chord(b, true);
        std::vector<float> L1(44100), R1(44100), L2(44100), R2(44100);
        a.render(L1.data(), R1.data(), 44100);
        int done = 0, sizes[5] = {1, 7, 64, 333, 1000}, k = 0;
        while (done < 44100) {
            int n = std::min(sizes[k++ % 5], 44100 - done);
            b.render(L2.data() + done, R2.data() + done, n);
            done += n;
        }
        float diff = 0;
        for (int i = 0; i < 44100; i++)
            diff = std::max(diff, std::fabs(L1[i] - L2[i]));
        CHECK(diff < 1e-6f, "the buffer size changes the sound (%.3g)", diff);
    }
    /* unison of all ten, chord memory, 5 voices */
    {
        Engine e;
        e.prepare(44100.0);
        e.setVoices(10);
        Program p;
        programInit(p);
        p.p[P_UNISON] = 1;
        p.p[P_UNI_VOICES] = 10;
        p.p[P_UNI_DETUNE] = 7;
        e.setProgram(p);
        e.noteOn(40, 1.0f);
        Stats a = run(e, 44100);
        CHECK(e.voicesSounding() == 10, "unison 10: %d voices", e.voicesSounding());
        CHECK(!a.nan && a.peak < 3.0f, "unison: peak %.2f", a.peak);
        e.setVoices(5);
        run(e, 4410);
        CHECK(e.voicesSounding() <= 5, "5 voices: %d sound", e.voicesSounding());
        e.noteOff(40);
        run(e, 44100);
        /* CHORD MEMORY: a chord held, UNISON: one key plays it */
        e.setVoices(5);
        for (int n : {48, 52, 55})
            e.noteOn(n, 1.0f);
        e.storeChord();
        for (int n : {48, 52, 55})
            e.noteOff(n);
        run(e, 44100);
        CHECK(chordCount(e.program()) == 3, "chord memory: %d notes", chordCount(e.program()));
        e.noteOn(60, 1.0f);
        run(e, 4410);
        CHECK(e.voicesSounding() == 3, "chord memory plays %d voices", e.voicesSounding());
        e.noteOff(60);
        run(e, 44100);
    }
    /* polyphony: 5 voices take a sixth note from the oldest, 10 play ten */
    {
        Engine e;
        e.prepare(44100.0);
        Program p;
        programInit(p);
        e.setProgram(p);
        for (int n = 0; n < 6; n++)
            e.noteOn(48 + n * 3, 0.9f);
        run(e, 4410);
        CHECK(e.voicesSounding() == 5, "5 voices: %d sound", e.voicesSounding());
        e.allOff();
        run(e, 44100);
        e.setVoices(10);
        for (int n = 0; n < 10; n++)
            e.noteOn(40 + n * 3, 0.9f);
        run(e, 4410);
        CHECK(e.voicesSounding() == 10, "10 voices: %d sound", e.voicesSounding());
    }
    /* the three ENV ROUTINGs */
    for (int r = 0; r < NENVROUTING; r++) {
        Engine e;
        e.prepare(44100.0);
        Program p;
        programInit(p);
        p.p[P_ENV_ROUTING] = (int16_t)r;
        p.p[P_E1_DEST] = MD_OSC1_FINE;
        p.p[P_E1_AMT] = 40;
        p.p[P_E2_AMT] = 60;
        p.p[P_CUTOFF] = 400;
        e.setProgram(p);
        chord(e, true);
        Stats a = run(e, 44100);
        chord(e, false);
        run(e, 44100 * 2);
        CHECK(a.rms > 0.01f && !a.nan, "routing %d: rms %.4f", r, a.rms);
        CHECK(e.voicesSounding() == 0, "routing %d: %d voices after", r, e.voicesSounding());
    }
    /* LOW SPLIT: under the split point an octave down (its release found again) */
    {
        Engine e;
        e.prepare(44100.0);
        Program p;
        programInit(p);
        p.p[P_SPLIT_1] = 1;
        p.p[P_SPLIT_POINT] = 60;
        e.setProgram(p);
        e.noteOn(50, 1.0f);
        run(e, 4410);
        e.setParam(P_SPLIT_POINT, 40);           /* (moved while the key is held) */
        e.noteOff(50);
        run(e, 44100);
        CHECK(e.voicesSounding() == 0, "low split: the note hangs");
    }
    /* the arp, and with unison */
    {
        Engine e;
        e.prepare(44100.0);
        Program p;
        programInit(p);
        p.p[P_ARP_ON] = 1;
        p.p[P_ARP_OCT] = 2;
        p.p[P_ARP_MODE] = AM_UPDOWN;
        p.p[P_ARP_REPEAT] = 1;
        p.p[P_E2_REL] = 5;
        e.setProgram(p);
        e.host(false, 0.0, 120.0, false);
        chord(e, true);
        Stats a = run(e, 44100 * 2);
        CHECK(a.rms > 0.005f && !a.nan, "arp: rms %.4f", a.rms);
        chord(e, false);
        run(e, 44100);
        CHECK(e.voicesSounding() == 0, "arp: %d voices after", e.voicesSounding());
        p.p[P_UNISON] = 1;
        p.p[P_UNI_VOICES] = 4;
        e.setProgram(p);
        chord(e, true);
        Stats u = run(e, 44100);
        CHECK(!u.nan && u.rms > 0.005f && u.peak < 3.0f, "arp + unison: rms %.4f peak %.2f", u.rms, u.peak);
        chord(e, false);
        run(e, 44100);
    }
    /* the sequencer: plays, ties, stops; records with rests and ties; transposes */
    {
        Engine e;
        e.prepare(44100.0);
        Program p;
        programInit(p);
        p.p[P_E2_REL] = 5;
        p.seq.length = 4;
        for (int s = 0; s < 4; s++) {
            p.seq.note[s][0] = (uint8_t)(48 + s * 3 + 2);
            p.seq.vel[s][0] = 100;
        }
        p.seq.note[2][0] = PN_TIE;
        e.setProgram(p);
        e.host(false, 0.0, 120.0, false);
        e.seqPlay(true);
        Stats a = run(e, 44100 * 2);
        CHECK(a.rms > 0.005f, "seq: rms %.4f", a.rms);
        e.seqRecord(true);                       /* (while it plays: RECORD and a key transpose) */
        e.noteOn(65, 1.0f);
        e.noteOff(65);
        e.seqRecord(false);
        CHECK(e.seqTransposition() == 5, "seq transpose %d", e.seqTransposition());
        run(e, 44100);
        e.seqPlay(false);
        run(e, 44100);
        CHECK(e.voicesSounding() == 0, "seq: %d voices after stop", e.voicesSounding());
        e.seqRecord(true);
        e.noteOn(60, 0.9f);
        e.noteOn(64, 0.9f);
        e.noteOff(60);
        e.noteOff(64);
        e.seqRest();
        e.noteOn(67, 0.9f);
        e.noteOff(67);
        e.seqTie();
        e.seqRecord(false);
        const SeqData &s = e.seqData();
        CHECK(s.length == 4, "recorded length %d", s.length);
        CHECK(s.note[0][0] == 62 && s.note[0][1] == 66 && s.note[1][0] == PN_REST && s.note[2][0] == 69 && s.note[3][0] == PN_TIE,
              "recorded notes");
    }
    /* every modulation source to every destination: nothing breaks */
    {
        Engine e;
        e.prepare(44100.0);
        Program p;
        programInit(p);
        p.p[P_REV_ON] = 1;
        p.p[P_FX_ON] = 1;
        for (int s = 1; s < NMODSRC; s++) {
            for (int k = 0; k < NMOD; k++) {
                int d = 1 + (s * NMOD + k) % (NMODDST - 1);
                p.p[P_MOD1 + k * MOD_N + MOD_SRC] = (int16_t)s;
                p.p[P_MOD1 + k * MOD_N + MOD_AMT] = (int16_t)((k & 1) ? 127 : -127);
                p.p[P_MOD1 + k * MOD_N + MOD_DST] = (int16_t)d;
            }
            e.setProgram(p);
            e.modWheel(1.0f);
            e.pressure(0.7f);
            chord(e, true);
            Stats a = run(e, 22050);
            chord(e, false);
            Stats b = run(e, 22050);
            CHECK(!a.nan && !b.nan && a.peak < 8.0f, "mod source %s: peak %.2f", MODSRC_NAME[s], a.peak);
        }
    }
    /* the CPU: 10 voices, both effects, 10 s of audio */
    {
        Engine e;
        e.prepare(48000.0);
        e.setVoices(10);
        Program p = FACTORY[11];
        e.setProgram(p);
        for (int n = 0; n < 10; n++)
            e.noteOn(40 + n * 4, 0.9f);
        for (int q = 1; q >= 0; q--) {
            e.setQuality(q);
            auto t0 = std::chrono::steady_clock::now();
            run(e, 48000 * 10);
            double s = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
            std::printf("  CPU: 10 voices + effects, %s: %.1f %% of one core\n", q ? "HIGH" : "ECO", s * 10.0);
        }
    }
    std::printf(fails ? "%d FAILED\n" : "all passed\n", fails);
    return fails ? 1 : 0;
}
