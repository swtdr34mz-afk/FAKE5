/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (FAKE5) */
/* FAKE5's parameters, laid out as a Sequential Take 5 program (its User's Guide 2.0: the front panel, the program
 * menu, the NRPN table). Every value is an integer in the Take 5's own range (OSC PITCH -700..+700 cents, CUTOFF
 * 0..1023, ENV AMOUNT -127..+127, ...), so what the display shows is what the Take 5 shows.
 *
 * The modulation sources and destinations are the Take 5's (appendix: 19 sources, 57 destinations, the same order). */
#pragma once
#include <cstdint>

namespace f5
{
/* ------------------------------------------------------------------ choices --- */
enum LfoShape { LS_TRI, LS_SAW, LS_REVSAW, LS_SQUARE, LS_SH, NLFOSHAPE };
enum GlideMode { GM_RATE, GM_RATE_A, GM_TIME, GM_TIME_A, NGLIDEMODE };
enum KeyMode { KM_LOW, KM_HIGH, KM_LAST, NKEYMODE };
enum ArpMode { AM_UP, AM_DOWN, AM_UPDOWN, AM_RANDOM, AM_ASSIGN, NARPMODE };
enum EnvRouting { ER_FILTER_AMP, ER_AUX_FILTERAMP, ER_AUX_FILTERGATE, NENVROUTING };
enum EnvRepeat { RP_OFF, RP_ENV1, RP_ENV2, RP_BOTH, NENVREPEAT };
enum FxType {
    FX_DELAY, FX_BBD, FX_TAPE1, FX_TAPE2, FX_CHORUS, FX_FLANGER, FX_PHASER, FX_HPF, FX_DISTORTION, FX_RINGMOD,
    FX_ROTARY, FX_LOFI, NFXTYPE
};
enum Divide { DV_2, DV_4, DV_8, DV_8T, DV_16, DV_16T, DV_32, DV_64, NDIVIDE };

/* the modulation sources (appendix) */
enum ModSrc {
    MS_OFF, MS_OSC2, MS_NOISE, MS_LFO1, MS_LFO2, MS_ENV1, MS_ENV2, MS_SPREAD, MS_BEND, MS_MODWHEEL, MS_PRESSURE,
    MS_BREATH, MS_FOOT, MS_EXPRESSION, MS_VELOCITY, MS_NOTE, MS_FILTER_OUT, MS_RANDOM, MS_DC, MS_AUDIO_OUT, NMODSRC
};
/* the destinations (appendix) */
enum ModDst {
    MD_OFF, MD_OSC1_OCT, MD_OSC2_OCT, MD_OSC1_FINE, MD_OSC2_FINE, MD_OSC_FINE, MD_OSC1_SHAPE, MD_OSC2_SHAPE, MD_OSC_SHAPE,
    MD_OSC1_LEVEL, MD_OSC2_LEVEL, MD_FM_LEVEL, MD_SUB, MD_NOISE, MD_DRIVE, MD_CUTOFF, MD_RESO, MD_VINTAGE, MD_OVERDRIVE,
    MD_PAN, MD_FX_MIX, MD_FX_TIME, MD_FX_MISC, MD_REV_MIX, MD_REV_SIZE, MD_REV_PREDELAY, MD_REV_DECAY, MD_REV_TONE,
    MD_LFO1_FREQ, MD_LFO2_FREQ, MD_LFO_FREQ, MD_LFO1_AMT, MD_LFO2_AMT, MD_LFO_AMT, MD_ENV1_AMT, MD_ENV2_AMT,
    MD_ENV1_ATT, MD_ENV2_ATT, MD_ENV1_DEC, MD_ENV2_DEC, MD_ENV1_REL, MD_ENV2_REL, MD_MOD1_AMT,
    MD_MOD16_AMT = MD_MOD1_AMT + 15, NMODDST
};
constexpr int NMOD = 16;

/* ------------------------------------------------------------- the parameters --- */
enum Param {
    /* OSCILLATORS */
    P_O1_OCT, P_O1_PITCH, P_O1_SHAPE, P_SYNC, P_O2_OCT, P_O2_PITCH, P_O2_SHAPE,
    P_O1_KEY, P_O2_KEY, P_O2_BYPASS,
    /* MIXER */
    P_O1_LEVEL, P_SUB, P_O2_LEVEL, P_FM, P_NOISE,
    /* FILTER */
    P_CUTOFF, P_RESO, P_DRIVE, P_KEYAMT,
    /* ENVELOPES */
    P_E1_DEL, P_E1_ATT, P_E1_DEC, P_E1_SUS, P_E1_REL, P_E1_AMT, P_E1_VEL, P_E1_DEST,
    P_E2_DEL, P_E2_ATT, P_E2_DEC, P_E2_SUS, P_E2_REL, P_E2_AMT, P_E2_VEL,
    P_ENV_ROUTING, P_ENV_REPEAT,
    /* LFOs: two of LFO_N each */
    P_LFO1, LFO_FREQ = 0, LFO_AMT, LFO_SHAPE, LFO_SYNC, LFO_DEST, LFO_RESET, LFO_SLEW, LFO_N,
    P_LFO_END = P_LFO1 + 2 * LFO_N,
    /* MODULATION: sixteen slots of three */
    P_MOD1 = P_LFO_END, MOD_SRC = 0, MOD_AMT, MOD_DST, MOD_N,
    P_MOD_END = P_MOD1 + NMOD * MOD_N,
    /* EFFECT, REVERB */
    P_FX_ON = P_MOD_END, P_FX_TYPE, P_FX_TIME, P_FX_MIX, P_FX_MISC, P_FX_SYNC,
    P_REV_ON, P_REV_MIX, P_REV_DAMP, P_REV_PREDELAY, P_REV_DECAY, P_REV_TONE,
    /* VINTAGE, OVERDRIVE, GLIDE, UNISON */
    P_VINTAGE, P_OVERDRIVE, P_GLIDE, P_GLIDE_RATE, P_GLIDE_MODE, P_O1_GLIDE, P_O2_GLIDE,
    P_UNISON, P_UNI_VOICES, P_UNI_DETUNE, P_UNI_KEYMODE, P_UNI_RETRIG,
    /* CLOCK, ARPEGGIATOR, SEQUENCER */
    P_BPM, P_DIVIDE, P_ARP_ON, P_ARP_MODE, P_ARP_OCT, P_ARP_REPEAT, P_ARP_RELATCH, P_SEQ_PLAY,
    /* the rest of the program menu, LOW SPLIT */
    P_PGM_VOL, P_PB_UP, P_PB_DOWN, P_SPLIT_1, P_SPLIT_2, P_SPLIT_POINT,
    /* VOICE SPREAD (FAKE5's own: the Take 5 spreads its voices through the matrix) */
    P_SPREAD,
    NPARAM
};

enum Kind : uint8_t {
    K_NUM,      /* a number as it is */
    K_SIGNED,   /* with its sign (+12, -3) */
    K_CHOICE,   /* a name from the list */
    K_ONOFF,
    K_NOTE,     /* a MIDI note's name (C3, F#4) */
    K_CENTS,    /* OSC PITCH: semitones and cents (+3.25) */
    K_LFOFREQ,  /* 0..127, a note value when synced */
    K_FXTIME,   /* the effect's TIME (a delay synced: a note value) */
    K_UNIVOICES /* 1..10 voices */
};

struct ParamDef {
    const char *id;           /* the host's id (stable: saved projects) */
    const char *name;         /* the host's and the display's name */
    int lo, hi, def;
    Kind kind;
    const char *const *choice;
};

extern const ParamDef PARAMS[NPARAM];
extern const char *const MODSRC_NAME[NMODSRC];
extern const char *const MODDST_NAME[NMODDST];
extern const bool MODSRC_POLY[NMODSRC];
extern const bool MODDST_POLY[NMODDST];
extern const char *const FXTYPE_NAME[NFXTYPE];
extern const char *const FXTYPE_SHORT[NFXTYPE];
extern const char *const DIVIDE_NAME[NDIVIDE];
extern const char *const LFOSYNC_NAME[16];
extern const char *const DLYSYNC_NAME[11];
extern const char *const LFOSHAPE_NAME[NLFOSHAPE];
extern const char *const ENVROUTING_NAME[NENVROUTING];
/* the effect's three knobs, as each type names them */
extern const char *const FXKNOB_NAME[NFXTYPE][3];

/* the value as the display shows it (synced: the LFO's or the delay's clock sync; fxType: the effect's knobs) */
void paramText(int p, int v, char *out, int n, bool synced = false, int fxType = 0);
/* an effect knob's value as its type names it ("Rate 42", "Mix 64") */
const char *fxKnobName(int fxType, int knob);

/* the sequencer: in the program, not host parameters */
constexpr int SEQ_STEPS = 64, SEQ_NOTES = 10;
constexpr uint8_t PN_REST = 0, PN_TIE = 1;          /* a step's note slot: 0 rest, 1 tie, else note + 2 */
struct SeqData {
    uint8_t note[SEQ_STEPS][SEQ_NOTES];             /* PN_REST, PN_TIE, or MIDI note + 2 */
    uint8_t vel[SEQ_STEPS][SEQ_NOTES];
    uint8_t length;                                 /* steps (0: none recorded) */
};
void seqClear(SeqData &s);

/* a whole program: its parameters, its sequence, its chord memory */
constexpr int NAME_LEN = 20, CHORD_N = 10;
struct Program {
    char name[NAME_LEN];
    int16_t p[NPARAM];
    SeqData seq;
    int8_t chord[CHORD_N];                          /* CHORD MEMORY's notes over the lowest (-1: none) */
};
void programInit(Program &p);                        /* the BASIC PROGRAM */
int chordCount(const Program &p);

} // namespace f5
