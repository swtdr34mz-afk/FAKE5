/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (FAKE5) */
/* FAKE5's engine: a Sequential Take 5 in the manner of PROPHET 67 and REEVES (their band-limited oscillators, ladder
 * and half-bands).
 *
 *   voices     5 (the Take 5's) or 10. Two oscillators with the Take 5's continuous SHAPE (sine, sawtooth, square,
 *              narrowing pulse), OCTAVE and PITCH (+-7 semitones), a square sub an octave under oscillator 1, white or
 *              pink noise, hard SYNC 2>1, linear FM 2>1 (OSC 2's level its depth, as on the Take 5), KEY FOLLOW each,
 *              OSC 2 FILTER BYPASS; all of it made at the filter's rate (4x HIGH, 2x ECO), so FM and sync stay clean
 *   filter     the Prophet-5 Rev 4's 4-pole low-pass: four saturating OTA stages, self-oscillating, the bass the
 *              resonance takes; DRIVE into it, KEY>LPF FREQ (127: semitone for semitone)
 *   envelopes  two DADSR, ENV ROUTING as the Take 5's (FILTER / AMP, AUX / FILTER+AMP, AUX / FILTER+GATE), VELOCITY
 *              each, ENVELOPE REPEAT
 *   LFOs       LFO 1 global (one for all the voices), LFO 2 per voice; triangle, saw, reverse saw, square, S&H; 0.022 to
 *              500 Hz or synced to the clock; RESET, SLEW
 *   modulation sixteen slots of the Take 5's 19 sources and 57 destinations; the sources that are sound (OSC 2, NOISE,
 *              FILTER OUT) work at audio rate on CUTOFF, OSC 1 FINE and OSC 1 SHAPE
 *   VINTAGE    each voice its own tuning, cutoff and envelope times, and a wander, more as it turns
 *   play       GLIDE (polyphonic, four modes, each oscillator its share), UNISON (up to all voices, DETUNE, KEY MODE,
 *              ENV RETRIG) with CHORD MEMORY, HOLD, LOW SPLIT, the ARPEGGIATOR, the 64-step SEQUENCER (record, rests,
 *              ties, transpose), the clock
 *   effects    OVERDRIVE, EFFECT 1 (twelve types), the plate REVERB (Fx.h)
 *
 * The engine renders in chunks of 16 samples (the modulation's rate); notes land on the next chunk. All calls from
 * the audio thread. */
#pragma once
#include "Dsp.h"
#include "Fx.h"
#include "Params.h"

namespace f5
{
constexpr int MAXV = 10, CH = 16, NKEYS = 32;
/* the values ramped across a chunk */
enum {
    SM_G1, SM_G2, SM_GB, SM_GS, SM_GN, SM_FM, SM_CUT, SM_ENV, SM_RES, SM_DRV, SM_AMP, SM_PGM, SM_PAN, SM_INC1, SM_INC2,
    SM_SH1, SM_SH2, SM_ACUT0, SM_ACUT1, SM_ACUT2, SM_AFQ0, SM_AFQ1, SM_AFQ2, SM_ASH0, SM_ASH1, SM_ASH2, NSM
};

class Engine
{
public:
    Engine();
    void prepare(double sampleRate);
    void setProgram(const Program &p);
    const Program &program() const { return prog; }
    void setParam(int p, int v);
    void setVoices(int n);                       /* 5 or 10 */
    void setQuality(int q) { quality = q ? 1 : 0; }   /* 1 HIGH: 4x; 0 ECO: 2x */
    void setTune(int coarse, int fineCents) { tuneSemis = (float)coarse + (float)fineCents * 0.01f; }
    int voiceCount() const { return nVoices; }

    void noteOn(int note, float vel);            /* vel 0..1 */
    void noteOff(int note);
    void sustain(bool on);
    void bend(float v);                          /* -1..1 */
    void modWheel(float v);                      /* 0..1 */
    void pressure(float v);
    void breath(float v);
    void foot(float v);
    void expression(float v);
    void allOff();
    void panic();
    /* the host's transport: playing, its position (quarters), its tempo; sync: follow it (else the program's BPM) */
    void host(bool playing, double ppq, double bpm, bool sync);

    void render(float *L, float *R, int n);

    /* the panel's buttons that act on the engine */
    void seqPlay(bool on);
    void seqRecord(bool on);                     /* RECORD: played notes into the sequence (while it plays: transpose) */
    void seqRest();
    void seqTie();
    void seqBack();
    int seqRecStep() const { return recStep; }
    bool seqRecording() const { return rec; }
    bool seqPlaying() const { return playing; }
    int seqTransposition() const { return seqTrans; }
    void hold(bool on);
    bool holding() const { return holdOn; }
    void storeChord();                           /* UNISON pressed with keys held: CHORD MEMORY (one key: cleared) */
    const SeqData &seqData() const { return prog.seq; }

    /* for the editor */
    int voicesSounding() const;
    float voiceLevel(int v) const { return v >= 0 && v < MAXV ? V[v].amp : 0.0f; }
    int seqStep() const;                         /* the step playing (-1 none) */
    bool clockBeat() const;                      /* the TAP TEMPO LED: the first half of a beat */
    float bpmNow() const { return bpmCur; }
    float lfoValue(int k) const { return k == 0 ? lfo1Out : lfo2Show; }
    float meter() const { return meterLvl; }

private:
    struct Env {
        float v = 0.0f, dly = 0.0f;
        uint8_t st = 0;                          /* ES_* */
    };
    enum { ES_IDLE, ES_DELAY, ES_ATT, ES_DEC, ES_REL };

    struct Voice {
        int note = -1, key = -1;                 /* key: the id of what plays it (a key, the arp, the sequencer) */
        bool gate = false, sus = false, used = false;
        float vel = 0.0f, uniDet = 0.0f, rnd = 0.0f;
        float p1 = 60.0f, p2 = 60.0f, t1 = 60.0f, t2 = 60.0f;   /* the oscillators' pitch now, where it glides */
        float gr1 = 0.0f, gr2 = 0.0f;            /* glide speed (semitones a second), 0: none */
        Osc o1, o2;
        BlRing subR;
        float flip = 1.0f;
        Env e[3];                                /* ENV 1, ENV 2, the gate (FILTER+GATE's amplifier) */
        float lph = 0.0f, lsh = 0.0f, lraw = 0.0f, lout = 0.0f;
        float f[4] = {}, ffb = 0.0f;
        HalfBand os1, os2;
        float sm[NSM] = {};
        bool smOk = false;
        float vt[4] = {};                        /* VINTAGE's own: osc 1 and 2 (cents), cutoff (semitones), env times */
        float dr1 = 0, dr2 = 0, dt1 = 0, dt2 = 0;
        int drn = 0;
        uint32_t seed = 1, age = 0;
        float dst[NMODDST] = {};
        float fade = 1.0f, amp = 0.0f;
        int pnote = -1, pkey = -1;
        float pvel = 0.0f, pdet = 0.0f;
        bool pend = false;
        float spread = 0.0f;
        int idx = 0;
        float hb[64] = {};                       /* VOICE SPREAD's delay on the far side */
        uint32_t hw = 0;
        float pink[3] = {}, lastY = 0.0f, lastO2 = 0.0f, lastN = 0.0f;
        float au[3][3] = {};                     /* audio-rate amounts: [osc 2, noise, filter out][cutoff, osc 1 fine, osc 1 shape] */
    };

    Program prog;
    const int16_t *P = prog.p;
    Voice V[MAXV];
    int nVoices = 5, quality = 1;
    float sr = 44100.0f, tuneSemis = 0.0f;
    const float *tab = nullptr;
    uint32_t ageN = 0, noiseSeed = 0x2545F491u, lfoSeed = 0x9E3779B9u;
    float bendV = 0, mwV = 0, prV = 0, brV = 0, ftV = 0, exV = 0;
    float ctl[6] = {};
    bool susOn = false, holdOn = false;
    bool hostPlaying = false, hostSync = true;
    double hostPpq = 0.0, hostBpm = 120.0;
    float outL[CH] = {}, outR[CH] = {};
    int gpos = CH;
    /* the keys held, in order */
    int8_t keys[NKEYS];
    int8_t splitMap[128];                        /* (LOW SPLIT: the note each key played, for its release) */
    float keyVel[NKEYS];
    int nKeys = 0;
    float lastPitch = 60.0f;
    bool anyHeld = false;
    /* LFO 1 (global) */
    float lfo1Ph = 0.0f, lfo1Sh = 0.0f, lfo1Raw = 0.0f, lfo1Out = 0.0f, lfo2Show = 0.0f;
    /* the clock */
    double pos = 0.0;
    float bpmCur = 120.0f;
    long stepIdx = -1;
    bool stepHigh = false;
    /* the arp */
    int8_t arpNotes[NKEYS];
    float arpVel[NKEYS];
    int arpN = 0, arpI = -1, arpDir = 1, arpRep = 0, arpPlaying = -1;
    bool arpLatched = false;
    /* the sequencer */
    bool playing = false, rec = false;
    int pStep = -1, recStep = 0, recHeld = 0, seqTrans = 0;
    bool recFresh = true;
    uint8_t sounding[SEQ_NOTES] = {};
    /* the global (mono) destinations, the output */
    float gdst[NMODDST] = {};
    float lastOut = 0.0f, meterLvl = 0.0f;
    float dcL = 0, dcR = 0, dcxL = 0, dcxR = 0, dcK = 0.999f;
    float warmL = 0, warmR = 0, warmK = 0.01f;   /* the output stage's low warmth */
    int uniN = 1;
    Overdrive od;
    Effect fx;
    Plate plate;

    void key(int note, float vel, bool down);
    void playNote(int key, int note, float vel);
    void stopNote(int key);
    void startVoice(Voice &v, int key, int note, float vel, float det, bool retrig);
    int findVoice(int note);
    void unisonUpdate(bool newKey);
    void releaseVoice(Voice &v);
    void chunk();
    void clockChunk();
    void arpStep();
    void arpRebuild();
    void seqStepNext();
    void voiceChunk(Voice &v, float *L, float *R);
    void modulate(Voice *v, const float *src, float *d, float au[3][3]);
    void sources(Voice *v, float *src);
    float lfoHz(int k, float extra) const;
    void envGate(Env &e, int ei);
    void envChunk(Env &e, int ei, const Voice &v, float *tgt, float *k, int *mode);
};
} // namespace f5
