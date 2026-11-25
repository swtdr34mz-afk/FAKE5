/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (FAKE5) */
#include "Engine.h"
#include <algorithm>
#include <vector>

namespace f5
{
/* ------------------------------------------------------------------ tables --- */
const float *blepTables()
{
    /* PROPHET 67's: a Kaiser-windowed sinc (cutoff 0.9 of Nyquist, beta 8) over 16 samples, integrated once (the
     * band-limited step) and its residual once more (the ramp's), 1024 points a sample. Made once, shared */
    static const std::vector<float> t = [] {
        const int H = BL_H, OS = BL_OS, N = BL_N;
        const double fc = 0.9, beta = 8.0, pi = 3.14159265358979323846;
        auto i0 = [](double x) {
            double s = 1, term = 1;
            for (int k = 1; k < 60; k++) {
                term *= (x / (2 * k)) * (x / (2 * k));
                s += term;
            }
            return s;
        };
        std::vector<double> h(N), B(N), R(N);
        for (int i = 0; i < N; i++) {
            double x = -H + (double)i / OS, r = x / H;
            double w = std::fabs(r) <= 1 ? i0(beta * std::sqrt(1 - r * r)) / i0(beta) : 0;
            double sc = x == 0 ? 1.0 : std::sin(pi * fc * x) / (pi * fc * x);
            h[i] = fc * sc * w;
        }
        double acc = 0;
        B[0] = 0;
        for (int i = 1; i < N; i++)
            B[i] = (acc += 0.5 * (h[i - 1] + h[i]) / OS);
        double total = B[2 * H * OS];
        for (int i = 0; i < N; i++)
            B[i] /= total;
        acc = 0;
        R[0] = 0;
        for (int i = 1; i < N; i++) {
            double x0 = -H + (double)(i - 1) / OS, x1 = -H + (double)i / OS;
            double e0 = B[i - 1] - (x0 >= 0 ? 1.0 : 0.0), e1 = B[i] - (x1 >= 0 ? 1.0 : 0.0);
            if (x0 < 0 && x1 >= 0)
                e0 = B[i - 1];
            R[i] = (acc += 0.5 * (e0 + e1) / OS);
        }
        double drift = R[2 * H * OS];
        std::vector<float> out(2 * N);
        for (int i = 0; i < N; i++) {
            out[i] = (float)B[i];
            out[N + i] = (float)(R[i] - drift * std::min(1.0, (double)i / (2 * H * OS)));
        }
        out[N - 1] = 1.0f;
        out[2 * N - 1] = 0.0f;
        return out;
    }();
    return t.data();
}

namespace
{
/* each unison voice's place in the detune */
const float U_DET[MAXV] = {0.0f, -1.0f, 1.0f, -0.5f, 0.5f, -0.75f, 0.75f, -0.25f, 0.25f, -0.9f};
/* VOICE SPREAD: each voice's own value (-1..1), every other voice the other side */
const float SPREAD[MAXV] = {-1.0f, 1.0f, -0.5f, 0.5f, 0.0f, -0.75f, 0.75f, -0.25f, 0.25f, 0.1f};
/* VOICE SPREAD (as PROPHET 67's): each voice's place in the field (SPREAD above), a little detune of its own (cents
 * at full) and a colour (the cutoff, semitones at full); the side away from it hears it a fraction of a ms late */
const float S_DET[MAXV] = {-3.0f, 3.5f, -1.5f, 1.2f, 0.4f, -2.5f, 2.2f, -0.8f, 1.8f, -0.4f};
const float S_COL[MAXV] = {0.35f, -0.45f, 0.2f, -0.15f, 0.0f, 0.5f, -0.3f, 0.1f, -0.25f, 0.05f};
/* a synced LFO's cycle in beats (LFOSYNC_NAME's order) */
const float LFO_BEATS[16] = {16, 8, 4, 3, 2, 1.5f, 4.0f / 3.0f, 1, 0.75f, 2.0f / 3.0f, 0.5f, 0.375f, 1.0f / 3.0f,
                             0.25f, 1.0f / 6.0f, 0.125f};
const float DIV_SPB[NDIVIDE] = {0.5f, 1, 2, 3, 4, 6, 8, 16};   /* steps a beat */

enum { K_ARP = 1000, K_UNI = 2000, K_SEQ = 3000 };
constexpr float VOICE_GAIN = 0.30f;           /* a voice's level: a four-note chord of the BASIC PROGRAM about -10 dB RMS */

inline float envSec(float v) { return v <= 0.0f ? 0.0005f : 0.0005f * std::exp2(v * (15.9f / 127.0f)); }   /* 0.5 ms .. 30 s */
inline float delaySec(float v) { return v <= 0.5f ? 0.0f : 0.001f * std::exp2(v * (13.3f / 127.0f)); }   /* .. 10 s */
inline float glideSec(float v) { return 0.0005f * std::exp2(v * (14.3f / 127.0f)); }                    /* .. 10 s */
inline float fineSemis(float a)                     /* a FINE destination's amount (127 a unit) in semitones */
{
    float x = clampf(a, -254.0f, 254.0f) * (1.0f / 127.0f);
    return 24.0f * x * std::fabs(x);
}
inline float octSemis(float a) { return 12.0f * std::round(clampf(a, -254.0f, 254.0f) * (2.0f / 127.0f)); }
inline bool audioSrc(int s) { return s == MS_OSC2 || s == MS_NOISE || s == MS_FILTER_OUT; }
inline int audioDst(int d)
{
    switch (d) {
    case MD_CUTOFF: return 0;
    case MD_OSC1_FINE:
    case MD_OSC_FINE: return 1;
    case MD_OSC1_SHAPE:
    case MD_OSC_SHAPE: return 2;
    default: return -1;
    }
}
inline float lfoShape(int sh, float ph, float held)
{
    switch (sh) {
    case LS_TRI: return ph < 0.25f ? 4.0f * ph : ph < 0.75f ? 2.0f - 4.0f * ph : 4.0f * ph - 4.0f;   /* bipolar */
    case LS_SAW: return ph;                                                                         /* the rest 0..1 */
    case LS_REVSAW: return 1.0f - ph;
    case LS_SQUARE: return ph < 0.5f ? 1.0f : 0.0f;
    default: return held;
    }
}
inline float slewK(int s, float dt) { return s <= 0 ? 1.0f : 1.0f - std::exp(-dt / (0.0004f * std::exp2((float)s * (10.5f / 127.0f)))); }
} // namespace

/* ------------------------------------------------------------------- setup --- */
Engine::Engine()
{
    programInit(prog);
    tab = blepTables();
    uint32_t s = 0x51F15EEDu;
    for (int i = 0; i < MAXV; i++) {
        V[i].seed = 0x1234567u + 7919u * (uint32_t)i;
        V[i].spread = SPREAD[i];
        V[i].idx = i;
        for (float &t : V[i].vt)                 /* VINTAGE: each voice's own, for good */
            t = rndF(s);
    }
    std::memset(splitMap, 0xFF, sizeof splitMap);
}

void Engine::prepare(double sampleRate)
{
    sr = (float)sampleRate;
    for (auto &v : V) {
        uint32_t s = v.seed;
        float sp = v.spread, vt[4];
        int ix = v.idx;
        std::memcpy(vt, v.vt, sizeof vt);
        v = Voice();
        v.seed = s;
        v.spread = sp;
        v.idx = ix;
        std::memcpy(v.vt, vt, sizeof vt);
    }
    od.prepare(sr);
    fx.prepare(sr);
    plate.prepare(sr);
    dcK = 1.0f - 2.0f * PI * 8.0f / sr;
    warmK = 1.0f - std::exp(-2.0f * PI * 120.0f / sr);
    warmL = warmR = 0.0f;
    dcL = dcR = dcxL = dcxR = 0.0f;
    for (float &c : ctl)
        c = 0.0f;
    gpos = CH;
}

void Engine::setProgram(const Program &p)
{
    if (playing)
        seqPlay(false);                          /* (a new program: the sequencer stops; PLAY starts it) */
    prog = p;
    prog.p[P_SEQ_PLAY] = 0;
    rec = false;
    seqTrans = 0;
}
void Engine::setParam(int p, int v)
{
    if (p < 0 || p >= NPARAM)
        return;
    int old = prog.p[p];
    prog.p[p] = (int16_t)clampf((float)v, (float)PARAMS[p].lo, (float)PARAMS[p].hi);
    if (p == P_UNISON && old != prog.p[p])
        allOff();
}
void Engine::setVoices(int n)
{
    n = n <= 5 ? 5 : 10;
    if (n == nVoices)
        return;
    nVoices = n;
    for (int i = n; i < MAXV; i++)
        if (V[i].e[1].st != ES_IDLE || V[i].e[2].st != ES_IDLE) {
            V[i].pend = false;
            V[i].gate = V[i].sus = false;
            if (V[i].fade >= 1.0f)
                V[i].fade = 0.9999f;
        }
}

/* ------------------------------------------------------------------- notes --- */
void Engine::noteOn(int note, float vel)
{
    note = note < 0 ? 0 : note > 127 ? 127 : note;
    int n = note;
    if (note < P[P_SPLIT_POINT])                  /* LOW SPLIT: under the split point an octave or two down */
        n -= P[P_SPLIT_2] ? 24 : P[P_SPLIT_1] ? 12 : 0;
    n = n < 0 ? 0 : n;
    splitMap[note] = (int8_t)n;
    key(n, vel, true);
}
void Engine::noteOff(int note)
{
    note = note < 0 ? 0 : note > 127 ? 127 : note;
    int n = splitMap[note] >= 0 ? splitMap[note] : note;
    splitMap[note] = -1;
    key(n, 0.0f, false);
}

void Engine::key(int note, float vel, bool down)
{
    int at = -1;
    for (int i = 0; i < nKeys; i++)
        if (keys[i] == note)
            at = i;
    if (down) {
        if (at >= 0)
            return;
        bool wasHeld = nKeys > 0;
        if (playing && rec) {                    /* RECORD held while it plays: a key transposes (middle C as recorded) */
            seqTrans = note - 60;
            return;
        }
        if (nKeys < NKEYS) {
            keys[nKeys] = (int8_t)note;
            keyVel[nKeys++] = vel;
        }
        /* LFO 1's RESET: from the start with the first key */
        if (P[P_LFO1 + LFO_RESET] && !wasHeld) {
            lfo1Ph = 0.0f;
            lfo1Sh = 0.5f + 0.5f * rndF(lfoSeed);
        }
        /* RECORD: into the sequence (and heard) */
        if (rec) {
            SeqData &s = prog.seq;
            if (recFresh) {
                for (int k = 0; k < SEQ_NOTES; k++)
                    s.note[recStep][k] = PN_REST;
                recFresh = false;
            }
            for (int k = 0; k < SEQ_NOTES; k++)
                if (s.note[recStep][k] == PN_REST) {
                    s.note[recStep][k] = (uint8_t)(note + 2);
                    s.vel[recStep][k] = (uint8_t)clampf(vel * 127.0f, 1.0f, 127.0f);
                    break;
                }
            recHeld++;
        }
        if (P[P_ARP_ON] && !playing) {
            if (P[P_ARP_RELATCH] && holdOn && !wasHeld)
                arpLatched = false;              /* a new chord: the old one goes */
            if (arpLatched && !wasHeld && !holdOn)
                arpLatched = false;
            arpRebuild();
            if (arpI < 0 && !(hostSync && hostPlaying)) {   /* the first note: at once, the clock from here */
                pos = 0.0;
                stepIdx = 0;
                stepHigh = true;
                arpStep();
            }
            return;
        }
        anyHeld = wasHeld;                       /* (glide's A modes: legato or not) */
        if (P[P_UNISON])
            unisonUpdate(!wasHeld);
        else
            playNote(note, note, vel);
    } else {
        if (at < 0)
            return;
        for (int i = at; i < nKeys - 1; i++) {
            keys[i] = keys[i + 1];
            keyVel[i] = keyVel[i + 1];
        }
        nKeys--;
        if (rec && recHeld > 0 && --recHeld == 0) {
            SeqData &s = prog.seq;
            recStep = (recStep + 1) % SEQ_STEPS;
            s.length = (uint8_t)std::max<int>(s.length, recStep == 0 ? SEQ_STEPS : recStep);
            recFresh = true;
        }
        if (P[P_ARP_ON] && !playing) {
            if (holdOn || susOn) {
                arpLatched = true;
                return;
            }
            arpRebuild();
            return;
        }
        if (P[P_UNISON])
            unisonUpdate(false);
        else
            stopNote(note);
    }
}

void Engine::playNote(int k, int note, float vel)
{
    /* the arp's and the sequencer's notes with UNISON on: each a stack of its own */
    int n = 1;
    float det = 0.0f;
    if (P[P_UNISON] && k >= K_ARP) {
        n = std::max(1, std::min<int>(P[P_UNI_VOICES], nVoices));
        det = 2.0f + (float)P[P_UNI_DETUNE] * 4.7f;
        uniN = n;
    }
    for (int j = 0; j < n; j++) {
        int i = findVoice(note);
        if (i < 0)
            return;
        startVoice(V[i], k, note, vel, n > 1 ? U_DET[j] * det : 0.0f, true);
    }
}

void Engine::stopNote(int k)
{
    for (int i = 0; i < MAXV; i++) {
        Voice &v = V[i];
        if (v.pend && v.pkey == k) {
            v.pend = false;                      /* (the note it was taken for, let go before it began) */
            continue;
        }
        if (v.key == k && v.gate) {
            if (susOn || holdOn)
                v.sus = true;
            else
                releaseVoice(v);
            v.gate = false;
        }
    }
}

int Engine::findVoice(int note)
{
    /* a free one (the one that last played this note, else the longest silent), else the oldest let go, else the
     * oldest */
    int best = -1;
    uint32_t bestAge = 0xFFFFFFFFu;
    for (int i = 0; i < nVoices; i++) {
        Voice &v = V[i];
        bool idle = v.e[1].st == ES_IDLE && v.e[2].st == ES_IDLE && !v.pend && !v.gate;
        if (idle && v.note == note && v.used)
            return i;
        if (idle && v.age < bestAge) {
            best = i;
            bestAge = v.age;
        }
    }
    if (best >= 0)
        return best;
    for (int pass = 0; pass < 2 && best < 0; pass++) {
        bestAge = 0xFFFFFFFFu;
        for (int i = 0; i < nVoices; i++) {
            Voice &v = V[i];
            if (pass == 0 && (v.gate || v.sus))
                continue;
            if (v.age < bestAge) {
                best = i;
                bestAge = v.age;
            }
        }
    }
    return best;
}

void Engine::envGate(Env &e, int ei)
{
    if (ei == 2) {
        e.dly = 0.0f;
        e.st = ES_ATT;
        return;
    }
    float d = delaySec((float)P[ei == 0 ? P_E1_DEL : P_E2_DEL]);
    e.dly = d;
    e.st = d > 0.0f ? ES_DELAY : ES_ATT;
}

void Engine::startVoice(Voice &v, int k, int note, float vel, float det, bool retrig)
{
    bool busy = (v.e[1].st != ES_IDLE || v.e[2].st != ES_IDLE) && v.key >= 0;
    float lvl = maxf(v.e[1].v, v.e[2].v);
    if (busy && v.key != k && retrig && lvl > 0.001f && v.note != note) {
        /* taken while it sounds: out in 4.5 ms, then this note */
        v.pend = true;
        v.pnote = note;
        v.pkey = k;
        v.pvel = vel;
        v.pdet = det;
        if (v.fade >= 1.0f)
            v.fade = 0.9999f;
        v.gate = false;
        v.age = ++ageN;
        return;
    }
    bool fresh = v.e[1].st == ES_IDLE && v.e[2].st == ES_IDLE;
    float from = v.used ? v.p1 : lastPitch;      /* (polyphonic glide: each voice from where it was) */
    float from2 = v.used ? v.p2 : lastPitch;
    v.key = k;
    v.note = note;
    v.vel = vel;
    v.uniDet = det;
    v.gate = true;
    v.sus = false;
    v.age = ++ageN;
    v.fade = 1.0f;
    v.pend = false;
    v.rnd = rndF(v.seed);
    /* glide */
    bool unison = P[P_UNISON] != 0;
    int gm = P[P_GLIDE_MODE];
    bool legatoOnly = unison && (gm == GM_RATE_A || gm == GM_TIME_A);
    bool glideOn = P[P_GLIDE] && P[P_GLIDE_RATE] > 0 && (!legatoOnly || anyHeld);
    v.t1 = v.t2 = (float)note;
    if (glideOn) {
        bool timeMode = unison && gm >= GM_TIME;
        float r1 = (float)P[P_GLIDE_RATE] * (float)P[P_O1_GLIDE] / 127.0f, r2 = (float)P[P_GLIDE_RATE] * (float)P[P_O2_GLIDE] / 127.0f;
        float T1 = glideSec(r1), T2 = glideSec(r2);
        v.p1 = r1 > 0.0f ? from : (float)note;
        v.p2 = r2 > 0.0f ? from2 : (float)note;
        v.gr1 = r1 > 0.0f ? (timeMode ? maxf(std::fabs((float)note - from), 0.01f) / T1 : 12.0f / T1) : 0.0f;
        v.gr2 = r2 > 0.0f ? (timeMode ? maxf(std::fabs((float)note - from2), 0.01f) / T2 : 12.0f / T2) : 0.0f;
    } else {
        v.p1 = v.p2 = (float)note;
        v.gr1 = v.gr2 = 0.0f;
    }
    v.used = true;
    lastPitch = (float)note;
    if (retrig || fresh) {
        for (int i = 0; i < 3; i++)
            envGate(v.e[i], i);
        if (P[P_LFO1 + LFO_N + LFO_RESET]) {     /* LFO 2's RESET: each note from the start */
            v.lph = 0.0f;
            v.lsh = 0.5f + 0.5f * rndF(v.seed);
        } else if (fresh)
            v.lph = 0.5f + 0.5f * rndF(v.seed);  /* (free: each voice somewhere else in its cycle) */
    }
    if (fresh) {
        for (float &d : v.dst)
            d = 0.0f;
        v.smOk = false;
        v.e[0].v = v.e[1].v = v.e[2].v = 0.0f;
    }
}

void Engine::releaseVoice(Voice &v)
{
    for (auto &e : v.e)
        if (e.st != ES_IDLE)
            e.st = ES_REL;
    v.sus = false;
}

void Engine::unisonUpdate(bool newKey)
{
    if (nKeys == 0) {
        stopNote(K_UNI);
        anyHeld = false;
        return;
    }
    /* the note with priority */
    int km = P[P_UNI_KEYMODE], pick = nKeys - 1;
    for (int i = 0; i < nKeys; i++) {
        if (km == KM_LOW && keys[i] < keys[pick])
            pick = i;
        if (km == KM_HIGH && keys[i] > keys[pick])
            pick = i;
    }
    int note = keys[pick];
    float vel = keyVel[pick];
    bool retrig = newKey || P[P_UNI_RETRIG];
    /* the voices: N stacked, or CHORD MEMORY's notes (doubled when there are voices for it) */
    int offs[MAXV], n = 0, want = std::min<int>(P[P_UNI_VOICES], nVoices);
    float dets[MAXV];
    float det = 2.0f + (float)P[P_UNI_DETUNE] * 4.7f;
    int cc = chordCount(prog);
    if (cc > 1) {
        int reps = std::max(1, std::min(want / cc, nVoices / cc));
        for (int r = 0; r < reps; r++)
            for (int i = 0; i < CHORD_N && n < nVoices; i++)
                if (prog.chord[i] >= 0) {
                    offs[n] = prog.chord[i];
                    dets[n++] = r ? (r & 1 ? det * 0.6f : -det * 0.6f) : 0.0f;
                }
    } else {
        n = std::max(1, want);
        for (int i = 0; i < n; i++) {
            offs[i] = 0;
            dets[i] = n > 1 ? U_DET[i] * det : 0.0f;
        }
    }
    /* voices already on the unison: move them (legato) or strike again */
    int k = 0;
    for (int i = 0; i < nVoices && k < n; i++) {
        Voice &v = V[i];
        if (v.key == K_UNI && (v.gate || v.sus) && !retrig) {
            float tgt = (float)(note + offs[k]);
            v.t1 = v.t2 = tgt;
            int gm = P[P_GLIDE_MODE];
            if (P[P_GLIDE] && P[P_GLIDE_RATE] > 0) {
                bool tm = gm >= GM_TIME;
                float r1 = (float)P[P_GLIDE_RATE] * (float)P[P_O1_GLIDE] / 127.0f, r2 = (float)P[P_GLIDE_RATE] * (float)P[P_O2_GLIDE] / 127.0f;
                v.gr1 = r1 > 0 ? (tm ? std::fabs(tgt - v.p1) / glideSec(r1) : 12.0f / glideSec(r1)) : 0.0f;
                v.gr2 = r2 > 0 ? (tm ? std::fabs(tgt - v.p2) / glideSec(r2) : 12.0f / glideSec(r2)) : 0.0f;
            } else {
                v.p1 = v.p2 = tgt;
                v.gr1 = v.gr2 = 0.0f;
            }
            v.note = note + offs[k];
            v.vel = vel;
            v.gate = true;
            v.sus = false;
            k++;
        }
    }
    uniN = n;
    if (k == 0) {
        for (int i = 0; i < MAXV; i++)
            if (V[i].key == K_UNI && i >= n) {
                V[i].key = -1;
                if (V[i].gate || V[i].sus)
                    releaseVoice(V[i]);
                V[i].gate = V[i].sus = false;
            }
        for (int j = 0; j < n && j < nVoices; j++)
            startVoice(V[j], K_UNI, note + offs[j], vel, dets[j], true);
    }
}

void Engine::sustain(bool on)
{
    susOn = on;
    if (!on && !holdOn) {
        for (auto &v : V)
            if (v.sus) {
                releaseVoice(v);
                v.sus = false;
            }
        if (arpLatched) {
            arpLatched = false;
            arpRebuild();
        }
    }
}
void Engine::hold(bool on)
{
    holdOn = on;
    if (!on)
        sustain(susOn);
}
void Engine::bend(float v) { bendV = clampf(v, -1.0f, 1.0f); }
void Engine::modWheel(float v) { mwV = clampf(v, 0.0f, 1.0f); }
void Engine::pressure(float v) { prV = clampf(v, 0.0f, 1.0f); }
void Engine::breath(float v) { brV = clampf(v, 0.0f, 1.0f); }
void Engine::foot(float v) { ftV = clampf(v, 0.0f, 1.0f); }
void Engine::expression(float v) { exV = clampf(v, 0.0f, 1.0f); }

void Engine::allOff()
{
    nKeys = 0;
    arpN = 0;
    arpI = -1;
    arpLatched = false;
    anyHeld = false;
    recHeld = 0;
    for (auto &s : sounding)
        s = 0;
    for (auto &v : V) {
        v.gate = v.sus = false;
        v.pend = false;
        releaseVoice(v);
    }
}
void Engine::panic()
{
    allOff();
    for (auto &v : V) {
        for (auto &e : v.e) {
            e.v = 0.0f;
            e.st = ES_IDLE;
        }
        v.key = -1;
        v.fade = 1.0f;
        v.amp = 0.0f;
    }
    od.clear();
    fx.clear();
    plate.clear();
    playing = false;
    prog.p[P_SEQ_PLAY] = 0;
    std::memset(splitMap, 0xFF, sizeof splitMap);
}

void Engine::host(bool playing_, double ppq, double bpm, bool sync)
{
    hostSync = sync;
    if (bpm > 10.0)
        hostBpm = bpm;
    if (playing_ && !hostPlaying)
        stepIdx = -1;
    hostPlaying = playing_;
    hostPpq = ppq;
    if (sync && playing_)
        pos = ppq;
}

/* ----------------------------------------------------------------- the sequencer --- */
void Engine::seqPlay(bool on)
{
    if (on == playing)
        return;
    playing = on;
    prog.p[P_SEQ_PLAY] = on ? 1 : 0;
    if (on) {
        pStep = -1;
        if (rec) {                               /* (PLAY ends a recording: RECORD then transposes) */
            rec = false;
            recHeld = 0;
        }
        if (!(hostSync && hostPlaying)) {
            pos = 0.0;
            stepIdx = 0;
            stepHigh = true;
            seqStepNext();
        }
    } else {
        for (int k = 0; k < SEQ_NOTES; k++)
            if (sounding[k]) {
                stopNote(K_SEQ + k);
                sounding[k] = 0;
            }
        pStep = -1;
        seqTrans = 0;
    }
}
void Engine::seqRecord(bool on)
{
    rec = on;
    if (on && !playing) {
        recStep = 0;
        recHeld = 0;
        recFresh = true;
        prog.seq.length = 0;
        for (auto &st : prog.seq.note)
            for (auto &n : st)
                n = PN_REST;
    }
}
void Engine::seqRest()
{
    if (!rec || playing)
        return;
    for (int k = 0; k < SEQ_NOTES; k++)
        prog.seq.note[recStep][k] = PN_REST;
    recStep = (recStep + 1) % SEQ_STEPS;
    prog.seq.length = (uint8_t)std::max<int>(prog.seq.length, recStep == 0 ? SEQ_STEPS : recStep);
    recFresh = true;
}
void Engine::seqTie()
{
    if (!rec || playing)
        return;
    for (int k = 0; k < SEQ_NOTES; k++)
        prog.seq.note[recStep][k] = k == 0 ? PN_TIE : PN_REST;
    recStep = (recStep + 1) % SEQ_STEPS;
    prog.seq.length = (uint8_t)std::max<int>(prog.seq.length, recStep == 0 ? SEQ_STEPS : recStep);
    recFresh = true;
}
void Engine::seqBack()
{
    if (rec && recStep > 0) {
        recStep--;
        recFresh = true;
    }
}
void Engine::storeChord()
{
    for (auto &c : prog.chord)
        c = -1;
    if (nKeys < 2)
        return;                                  /* (one key: CHORD MEMORY cleared) */
    int8_t sorted[NKEYS];
    int n = nKeys;
    for (int i = 0; i < n; i++)
        sorted[i] = keys[i];
    std::sort(sorted, sorted + n);
    for (int i = 0; i < CHORD_N && i < n; i++)
        prog.chord[i] = (int8_t)(sorted[i] - sorted[0]);
}

int Engine::seqStep() const { return playing ? pStep : (rec ? recStep : -1); }
bool Engine::clockBeat() const { return pos - std::floor(pos) < 0.5; }

void Engine::clockChunk()
{
    float dt = (float)CH / sr;
    bpmCur = hostSync ? (float)hostBpm : (float)P[P_BPM];
    pos += (double)(dt * bpmCur / 60.0f);
    float spb = DIV_SPB[P[P_DIVIDE]];
    double x = pos * spb;
    long s = (long)std::floor(x);
    if (s < stepIdx - 1 || s > stepIdx + 64)     /* (the host jumped: from here) */
        stepIdx = s - 1;
    bool arp = P[P_ARP_ON] && !playing && arpN > 0;
    if (s != stepIdx) {
        stepIdx = s;
        stepHigh = true;
        if (arp)
            arpStep();
        if (playing)
            seqStepNext();
    } else if (stepHigh && x - (double)s >= 0.5) {
        stepHigh = false;
        /* the gate's second half: the arp's note let go, the sequencer's (unless the next step is a tie) */
        if (arpPlaying >= 0) {
            stopNote(K_ARP);
            arpPlaying = -1;
        }
        if (playing && pStep >= 0) {
            const SeqData &sd = prog.seq;
            int nx = sd.length ? (pStep + 1) % sd.length : 0;
            if (sd.note[nx][0] != PN_TIE)
                for (int k = 0; k < SEQ_NOTES; k++)
                    if (sounding[k]) {
                        stopNote(K_SEQ + k);
                        sounding[k] = 0;
                    }
        }
    }
}

void Engine::arpRebuild()
{
    if (arpLatched && nKeys == 0)
        return;                                  /* (held: the notes stay) */
    if (arpLatched && nKeys > 0 && !P[P_ARP_RELATCH]) {
        for (int i = 0; i < nKeys; i++) {        /* held, more keys: added */
            bool in = false;
            for (int j = 0; j < arpN; j++)
                in |= arpNotes[j] == keys[i];
            if (!in && arpN < NKEYS) {
                arpNotes[arpN] = keys[i];
                arpVel[arpN++] = keyVel[i];
            }
        }
        return;
    }
    arpN = nKeys;
    for (int i = 0; i < nKeys; i++) {
        arpNotes[i] = keys[i];
        arpVel[i] = keyVel[i];
    }
    if (arpN == 0) {
        arpI = -1;
        if (arpPlaying >= 0) {
            stopNote(K_ARP);
            arpPlaying = -1;
        }
    }
}

void Engine::arpStep()
{
    if (arpN == 0)
        return;
    if (arpPlaying >= 0) {
        stopNote(K_ARP);
        arpPlaying = -1;
    }
    int8_t order[NKEYS];
    float ov[NKEYS];
    int n = arpN;
    for (int i = 0; i < n; i++) {
        order[i] = arpNotes[i];
        ov[i] = arpVel[i];
    }
    int mode = P[P_ARP_MODE];
    if (mode != AM_ASSIGN)
        for (int i = 1; i < n; i++)
            for (int j = i; j > 0 && order[j] < order[j - 1]; j--) {
                std::swap(order[j], order[j - 1]);
                std::swap(ov[j], ov[j - 1]);
            }
    int oct = std::max(1, (int)P[P_ARP_OCT]), rep = P[P_ARP_REPEAT];
    int total = n * oct;
    if (arpRep < rep && arpI >= 0)
        arpRep++;                                /* REPEAT: each note again */
    else {
        arpRep = 0;
        switch (mode) {
        case AM_DOWN: arpI = arpI <= 0 ? total - 1 : arpI - 1; break;
        case AM_UPDOWN:
            if (total == 1)
                arpI = 0;
            else if (arpI < 0) {
                arpI = 0;
                arpDir = 1;
            } else {
                arpI += arpDir;
                if (arpI >= total) {
                    arpI = total - 2;
                    arpDir = -1;
                } else if (arpI < 0) {
                    arpI = 1;
                    arpDir = 1;
                }
            }
            break;
        case AM_RANDOM: arpI = (int)(rndU(noiseSeed) % (uint32_t)total); break;
        default: arpI = arpI + 1 >= total ? 0 : arpI + 1;
        }
    }
    if (arpI >= total || arpI < 0)
        arpI = 0;
    int o = arpI / n, k = arpI % n;
    int note = order[k] + 12 * o;
    while (note > 127)
        note -= 12;
    arpPlaying = note;
    anyHeld = false;
    playNote(K_ARP, note, ov[k]);
}

void Engine::seqStepNext()
{
    const SeqData &s = prog.seq;
    if (s.length == 0)
        return;
    pStep = (pStep + 1) % s.length;
    const uint8_t *st = s.note[pStep];
    if (st[0] == PN_TIE)
        return;                                 /* the notes before go on */
    for (int k = 0; k < SEQ_NOTES; k++)
        if (sounding[k]) {
            stopNote(K_SEQ + k);
            sounding[k] = 0;
        }
    for (int k = 0; k < SEQ_NOTES; k++)
        if (st[k] >= 2) {
            int note = std::min(127, std::max(0, st[k] - 2 + seqTrans));
            playNote(K_SEQ + k, note, s.vel[pStep][k] / 127.0f);
            sounding[k] = st[k];
        }
}

/* --------------------------------------------------------------- modulation --- */
float Engine::lfoHz(int k, float extra) const
{
    const int16_t *Q = P + P_LFO1 + k * LFO_N;
    if (Q[LFO_SYNC]) {
        int i = (int)clampf((float)Q[LFO_FREQ] * 16.0f / 128.0f, 0.0f, 15.0f);
        return bpmCur / 60.0f / LFO_BEATS[i];
    }
    float v = clampf((float)Q[LFO_FREQ] + extra, 0.0f, 127.0f);
    return 0.022f * std::exp2(v * (14.47f / 127.0f));   /* 0.022 .. 500 Hz */
}

void Engine::sources(Voice *v, float *src)
{
    for (int i = 0; i < NMODSRC; i++)
        src[i] = 0.0f;
    src[MS_LFO1] = lfo1Out;
    src[MS_BEND] = ctl[0];
    src[MS_MODWHEEL] = ctl[1];
    src[MS_PRESSURE] = ctl[2];
    src[MS_BREATH] = ctl[3];
    src[MS_FOOT] = ctl[4];
    src[MS_EXPRESSION] = ctl[5];
    src[MS_DC] = 1.0f;
    src[MS_AUDIO_OUT] = lastOut;
    if (!v)
        return;
    src[MS_OSC2] = v->lastO2;
    src[MS_NOISE] = v->lastN;
    src[MS_LFO2] = v->lout;
    src[MS_ENV1] = v->e[0].v;
    src[MS_ENV2] = v->e[1].v;
    src[MS_SPREAD] = v->spread;
    src[MS_VELOCITY] = v->vel;
    src[MS_NOTE] = (float)v->note / 127.0f;
    src[MS_FILTER_OUT] = v->lastY;
    src[MS_RANDOM] = v->rnd;
}

void Engine::modulate(Voice *v, const float *src, float *d, float au[3][3])
{
    const float *old = v ? v->dst : gdst;        /* (the last chunk's: what modulates the modulation) */
    for (int i = 0; i < NMODDST; i++)
        d[i] = 0.0f;
    if (au)
        std::memset(au, 0, sizeof(float) * 9);
    /* the LFOs */
    for (int k = 0; k < 2; k++) {
        const int16_t *Q = P + P_LFO1 + k * LFO_N;
        float amt = clampf((float)Q[LFO_AMT] + old[MD_LFO1_AMT + k] + old[MD_LFO_AMT], -127.0f, 127.0f);
        d[Q[LFO_DEST]] += src[MS_LFO1 + k] * amt;
    }
    /* ENV 1 on its AUX destination */
    if (P[P_ENV_ROUTING] != ER_FILTER_AMP && v) {
        float amt = clampf((float)P[P_E1_AMT] + old[MD_ENV1_AMT], -127.0f, 127.0f);
        if (P[P_E1_VEL])
            amt *= v->vel;
        d[P[P_E1_DEST]] += src[MS_ENV1] * amt;
    }
    /* the sixteen slots */
    for (int k = 0; k < NMOD; k++) {
        const int16_t *Q = P + P_MOD1 + k * MOD_N;
        int s = Q[MOD_SRC], t = Q[MOD_DST];
        if (s == MS_OFF || t == MD_OFF)
            continue;
        float amt = clampf((float)Q[MOD_AMT] + old[MD_MOD1_AMT + k], -127.0f, 127.0f);
        int ad = audioDst(t);
        if (au && audioSrc(s) && ad >= 0) {
            int si = s == MS_OSC2 ? 0 : s == MS_NOISE ? 1 : 2;
            au[si][ad] += amt;
        } else
            d[t] += src[s] * amt;
    }
    d[MD_OFF] = 0.0f;
}

void Engine::envChunk(Env &e, int ei, const Voice &v, float *tgt, float *k, int *mode)
{
    *mode = 0;
    float dt = (float)CH / sr;
    float vint = clampf((float)P[P_VINTAGE] + v.dst[MD_VINTAGE], 0.0f, 127.0f) / 127.0f;
    float tsc = 1.0f + v.vt[3] * 0.22f * vint;   /* VINTAGE: each voice's envelopes a little faster or slower */
    if (e.st == ES_DELAY) {
        e.dly -= dt;
        if (e.dly <= 0.0f)
            e.st = ES_ATT;
    }
    if (ei == 2) {                               /* the gate: on, off */
        float T = e.st == ES_REL ? 0.004f : 0.0015f;
        *tgt = e.st == ES_REL || e.st == ES_IDLE ? 0.0f : 1.0f;
        *k = 1.0f - std::exp(-1.0f / (T * sr));
        *mode = 1;
        return;
    }
    const int base = ei == 0 ? P_E1_DEL : P_E2_DEL;
    const int16_t *Q = P + base;
    float sus = (float)Q[3] / 127.0f;
    if (e.st == ES_ATT && e.v >= 1.0f)
        e.st = ES_DEC;
    int rp = P[P_ENV_REPEAT];
    if (e.st == ES_DEC && (rp == RP_BOTH || rp == ei + 1) && e.v - sus < 0.004f + 0.01f * (1.0f - sus)) {
        e.dly = delaySec((float)Q[0]);           /* REPEAT: delay, attack and decay again */
        e.st = e.dly > 0.0f ? ES_DELAY : ES_ATT;
    }
    switch (e.st) {
    case ES_ATT: {
        float mt = v.dst[MD_ENV1_ATT + ei];
        float T = envSec(clampf((float)Q[1] + mt, 0.0f, 127.0f)) * tsc;
        *tgt = 1.3f;                             /* (an analog curve: charging toward past the top) */
        *k = 1.0f - std::exp(-1.609f / (T * sr));
        *mode = 2;
        break;
    }
    case ES_DEC: {
        float T = envSec(clampf((float)Q[2] + v.dst[MD_ENV1_DEC + ei], 0.0f, 127.0f)) * tsc;
        *tgt = sus;
        *k = 1.0f - std::exp(-4.6f / (T * sr));
        *mode = 1;
        break;
    }
    case ES_REL: {
        float T = envSec(clampf((float)Q[4] + v.dst[MD_ENV1_REL + ei], 0.0f, 127.0f)) * tsc;
        *tgt = 0.0f;
        *k = 1.0f - std::exp(-4.6f / (T * sr));
        *mode = 1;
        break;
    }
    default:
        *tgt = e.v;
        *k = 0.0f;
    }
}

/* --------------------------------------------------------------- the voice --- */
void Engine::voiceChunk(Voice &v, float *Lo, float *Ro)
{
    const float dt = (float)CH / sr, isr = 1.0f / sr;
    /* LFO 2: the voice's own */
    {
        const int16_t *Q = P + P_LFO1 + LFO_N;
        float hz = lfoHz(1, v.dst[MD_LFO2_FREQ] + v.dst[MD_LFO_FREQ]);
        float ph = v.lph + hz * dt;
        if (ph >= 1.0f) {
            ph -= std::floor(ph);
            v.lsh = 0.5f + 0.5f * rndF(v.seed);
        }
        v.lph = ph;
        v.lraw = lfoShape(Q[LFO_SHAPE], ph, v.lsh);
        v.lout += (v.lraw - v.lout) * slewK(Q[LFO_SLEW], dt);
    }
    float src[NMODSRC];
    sources(&v, src);
    float d[NMODDST], au[3][3];
    modulate(&v, src, d, au);
    std::memcpy(v.dst, d, sizeof d);

    /* VINTAGE: each voice's own tuning, and a slow wander */
    float vint = clampf((float)P[P_VINTAGE] + d[MD_VINTAGE], 0.0f, 127.0f) / 127.0f;
    {                                            /* (the wander runs always: even calibrated, a VCO is never still) */
        if (--v.drn <= 0) {
            v.dt1 = rndF(v.seed);
            v.dt2 = rndF(v.seed);
            v.drn = (int)((0.25f + 0.5f * (float)(rndU(v.seed) & 1023u) / 1023.0f) / dt);
        }
        float kd = 1.0f - std::exp(-dt / 0.3f);
        v.dr1 += (v.dt1 - v.dr1) * kd;
        v.dr2 += (v.dt2 - v.dr2) * kd;
    }
    const float spr = clampf((float)P[P_SPREAD], 0.0f, 127.0f) / 127.0f;
    float c1 = vint * (v.vt[0] * 9.0f + v.dr1 * 5.0f + v.rnd * 2.0f) + 0.6f * v.dr1 + spr * S_DET[v.idx],
          c2 = vint * (v.vt[1] * 9.0f + v.dr2 * 5.0f) + 0.6f * v.dr2 + spr * S_DET[v.idx] * 0.8f;
    /* glide */
    if (v.gr1 > 0.0f)
        v.p1 = v.p1 < v.t1 ? minf(v.p1 + v.gr1 * dt, v.t1) : maxf(v.p1 - v.gr1 * dt, v.t1);
    else
        v.p1 = v.t1;
    if (v.gr2 > 0.0f)
        v.p2 = v.p2 < v.t2 ? minf(v.p2 + v.gr2 * dt, v.t2) : maxf(v.p2 - v.gr2 * dt, v.t2);
    else
        v.p2 = v.t2;
    float bendS = ctl[0] >= 0.0f ? ctl[0] * (float)P[P_PB_UP] : ctl[0] * (float)P[P_PB_DOWN];
    float k1 = P[P_O1_KEY] ? v.p1 : 60.0f, k2 = P[P_O2_KEY] ? v.p2 : 60.0f;
    float n1 = k1 + 12.0f * (float)P[P_O1_OCT] + (float)P[P_O1_PITCH] * 0.01f + tuneSemis + (c1 + v.uniDet) * 0.01f + bendS +
               octSemis(d[MD_OSC1_OCT]) + fineSemis(d[MD_OSC1_FINE] + d[MD_OSC_FINE]);
    float n2 = k2 + 12.0f * (float)P[P_O2_OCT] + (float)P[P_O2_PITCH] * 0.01f + tuneSemis + (c2 - v.uniDet * 0.8f) * 0.01f +
               bendS + octSemis(d[MD_OSC2_OCT]) + fineSemis(d[MD_OSC2_FINE] + d[MD_OSC_FINE]);
    /* the rate everything here runs at: HIGH 4x, ECO 2x (each halved from 88.2 kHz up) */
    int os = quality ? 4 : 2;
    if (sr > 80000.0f)
        os /= 2;
    if (sr > 160000.0f)
        os = 1;
    const float fos = sr * (float)os, iso = 1.0f / (float)os, wmax = 0.42f * fos, wk = 2.0f * PI / fos;
    float inc1 = minf(hzOf(n1) * isr, 0.45f), inc2 = minf(hzOf(n2) * isr, 0.45f);
    float sh1 = (float)P[P_O1_SHAPE] / 127.0f + (d[MD_OSC1_SHAPE] + d[MD_OSC_SHAPE]) / 127.0f;
    float sh2 = (float)P[P_O2_SHAPE] / 127.0f + (d[MD_OSC2_SHAPE] + d[MD_OSC_SHAPE]) / 127.0f;
    bool sync = P[P_SYNC] != 0, fmOn = P[P_FM] != 0;
    /* the envelopes */
    float et[3], ek[3];
    int em[3];
    for (int i = 0; i < 3; i++)
        envChunk(v.e[i], i, v, &et[i], &ek[i], &em[i]);
    int routing = P[P_ENV_ROUTING];
    const int feI = routing == ER_FILTER_AMP ? 0 : 1, aeI = routing == ER_AUX_FILTERGATE ? 2 : 1;
    /* the filter: CUTOFF over ten octaves from 20 Hz, KEY>LPF FREQ, the envelope */
    float famt = routing == ER_FILTER_AMP ? clampf((float)P[P_E1_AMT] + d[MD_ENV1_AMT], -127.0f, 127.0f)
                                          : clampf((float)P[P_E2_AMT] + d[MD_ENV2_AMT], -127.0f, 127.0f);
    if ((routing == ER_FILTER_AMP && P[P_E1_VEL]) || (routing != ER_FILTER_AMP && P[P_E2_VEL]))
        famt *= 0.12f + 0.88f * v.vel;
    float cutU = (float)P[P_CUTOFF] + d[MD_CUTOFF] * (1023.0f / 127.0f);
    float oct0 = cutU * (10.3f / 1023.0f) + ((float)P[P_KEYAMT] / 127.0f) * (v.p1 - 60.0f) / 12.0f + vint * v.vt[2] * 0.22f +
                 spr * S_COL[v.idx] * (1.0f / 12.0f);
    float envOct = famt / 127.0f * 9.0f;
    float reso = clampf((float)P[P_RESO] + d[MD_RESO], 0.0f, 127.0f) / 127.0f;
    float drv = clampf((float)P[P_DRIVE] + d[MD_DRIVE], 0.0f, 127.0f) / 127.0f;
    /* the amplifier */
    float ampAmt = 1.0f;
    if (routing == ER_FILTER_AMP) {
        ampAmt = clampf((float)P[P_E2_AMT] + d[MD_ENV2_AMT], 0.0f, 127.0f) / 127.0f;
        if (P[P_E2_VEL])
            ampAmt *= 0.18f + 0.82f * v.vel;
    }
    /* PROGRAM VOLUME: as the Take 5's, an amplifier's gain (100 as made, 127 +2.7 dB, 50 -7.8 dB, 0 off) */
    float pgm = P[P_PGM_VOL] <= 0 ? 0.0f : VOICE_GAIN * std::pow((float)P[P_PGM_VOL] * 0.01f, 1.3f);
    if ((v.key == K_UNI || (v.key >= K_ARP && P[P_UNISON])) && uniN > 1)
        pgm *= 1.0f / std::sqrt((float)uniN);    /* (a stack as loud as a fat note, not N notes) */
    float pan = clampf(d[MD_PAN] / 127.0f + v.spread * spr * (0.6f + 0.4f * spr), -1.0f, 1.0f);
    const float haas = std::fabs(v.spread) * spr * 0.00036f * sr;   /* (up to 0.36 ms) */
    /* the mixer */
    float l1 = clampf((float)P[P_O1_LEVEL] + d[MD_OSC1_LEVEL], 0.0f, 160.0f) / 127.0f;
    float l2 = clampf((float)P[P_O2_LEVEL] + d[MD_OSC2_LEVEL], 0.0f, 160.0f) / 127.0f;
    float fmL = clampf((fmOn ? l2 : 0.0f) + d[MD_FM_LEVEL] / 127.0f, 0.0f, 1.25f);
    float tgt[NSM];
    tgt[SM_G1] = 0.5f * l1;
    tgt[SM_G2] = fmOn || P[P_O2_BYPASS] ? 0.0f : 0.5f * l2;
    tgt[SM_GB] = !fmOn && P[P_O2_BYPASS] ? 0.5f * l2 : 0.0f;
    tgt[SM_GS] = 0.5f * clampf((float)P[P_SUB] + d[MD_SUB], 0.0f, 160.0f) / 127.0f;
    tgt[SM_GN] = 0.55f * clampf(std::fabs((float)P[P_NOISE]) + d[MD_NOISE], 0.0f, 160.0f) / 127.0f;
    tgt[SM_FM] = 5.5f * fmL * fmL;               /* (the depth: OSC 2 swings oscillator 1's frequency by this many times) */
    tgt[SM_CUT] = oct0;
    tgt[SM_ENV] = envOct;
    tgt[SM_RES] = reso;
    tgt[SM_DRV] = drv;
    tgt[SM_AMP] = ampAmt;
    tgt[SM_PGM] = pgm;
    tgt[SM_PAN] = pan;
    tgt[SM_INC1] = inc1;
    tgt[SM_INC2] = inc2;
    tgt[SM_SH1] = sh1;
    tgt[SM_SH2] = sh2;
    for (int si = 0; si < 3; si++) {             /* the sources at audio rate: octaves, semitones, shape */
        tgt[SM_ACUT0 + si] = au[si][0] / 127.0f * 4.0f;
        tgt[SM_AFQ0 + si] = au[si][1] / 127.0f * 24.0f;
        tgt[SM_ASH0 + si] = au[si][2] / 127.0f;
    }
    if (!v.smOk) {
        for (int i = 0; i < NSM; i++)
            v.sm[i] = tgt[i];
        v.smOk = true;
    }
    float cur[NSM], stp[NSM];
    const float inv = 1.0f / (float)CH;
    for (int i = 0; i < NSM; i++) {
        cur[i] = v.sm[i];
        stp[i] = (tgt[i] - v.sm[i]) * inv;
        v.sm[i] = tgt[i];
    }
    const bool audio = tgt[SM_ACUT0] != 0 || tgt[SM_ACUT1] != 0 || tgt[SM_ACUT2] != 0 || tgt[SM_AFQ0] != 0 ||
                       tgt[SM_AFQ1] != 0 || tgt[SM_AFQ2] != 0 || tgt[SM_ASH0] != 0 || tgt[SM_ASH1] != 0 || tgt[SM_ASH2] != 0 ||
                       cur[SM_ACUT0] != 0 || cur[SM_AFQ0] != 0 || cur[SM_ASH0] != 0;
    const float steal = 1.0f / (0.0045f * sr);
    const bool pink = P[P_NOISE] < 0;
    /* a filter step: the Prophet-5 Rev 4's 4-pole (four OTA stages, the resonance from the last averaged over two
     * samples; each stage's transconductor saturates as the chip's: the resonance rounds off, the stages slew when
     * driven hard) */
    auto f4 = [&v](float x, float g, float k) {
        float fb = 0.5f * (v.f[3] + v.ffb);
        v.ffb = v.f[3];
        float u = sat(x - 4.0f * k * fb + 0.06f) - 0.0597f;   /* (a little offset: the chip's even harmonics) */
        v.f[0] += g * ota(u - v.f[0]);
        v.f[1] += g * ota(v.f[0] - v.f[1]);
        v.f[2] += g * ota(v.f[1] - v.f[2]);
        v.f[3] += g * ota(v.f[2] - v.f[3]);
        return v.f[3];
    };
    Wave w1, w2;
    float amax = 0.0f;

    for (int s = 0; s < CH; s++) {
        for (int i = 0; i < NSM; i++)
            cur[i] += stp[i];
        for (int i = 0; i < 3; i++) {
            Env &e = v.e[i];
            if (em[i] == 2) {
                e.v += (et[i] - e.v) * ek[i];
                if (e.v >= 1.0f) {
                    e.v = 1.0f;
                    if (i < 2)
                        e.st = ES_DEC;           /* (the top: decay from here, this very sample) */
                    ek[i] = 0.0f;
                }
            } else if (em[i] == 1)
                e.v += (et[i] - e.v) * ek[i];
        }
        float ae = v.e[aeI].v;
        if (v.fade < 1.0f) {                     /* taken: out on a smooth curve, then the note it was taken for */
            v.fade -= steal;
            if (v.fade <= 0.0f) {
                v.fade = 1.0f;
                for (auto &e : v.e) {
                    e.v = 0.0f;
                    e.st = ES_IDLE;
                }
                if (v.pend) {
                    v.pend = false;
                    startVoice(v, v.pkey, v.pnote, v.pvel, v.pdet, true);
                    for (int i = 0; i < 3; i++)
                        envChunk(v.e[i], i, v, &et[i], &ek[i], &em[i]);
                } else
                    v.key = -1;
                continue;
            }
            float f = v.fade;
            ae *= f * f * (3.0f - 2.0f * f);
        }
        /* noise: white, or pink (Kellet's three poles) */
        float wn = rndF(v.seed), nz = wn;
        if (pink) {
            v.pink[0] = 0.99765f * v.pink[0] + wn * 0.0990460f;
            v.pink[1] = 0.96300f * v.pink[1] + wn * 0.2965164f;
            v.pink[2] = 0.57000f * v.pink[2] + wn * 1.0526913f;
            nz = (v.pink[0] + v.pink[1] + v.pink[2] + wn * 0.1848f) * 0.21f;
        }
        v.lastN = nz;
        /* the audio-rate modulation (from the sample before: OSC 2, NOISE, FILTER OUT) */
        float aSrc[3] = {v.lastO2, nz, v.lastY};
        float aCut = 0.0f, aFq = 0.0f, aSh = 0.0f;
        if (audio)
            for (int si = 0; si < 3; si++) {
                aCut += cur[SM_ACUT0 + si] * aSrc[si];
                aFq += cur[SM_AFQ0 + si] * aSrc[si];
                aSh += cur[SM_ASH0 + si] * aSrc[si];
            }
        w1.set(cur[SM_SH1] + aSh);
        w2.set(cur[SM_SH2]);
        float i1 = cur[SM_INC1] * iso, i2 = cur[SM_INC2] * iso;
        if (aFq != 0.0f)
            i1 = minf(i1 * fexp2(clampf(aFq, -48.0f, 48.0f) * (1.0f / 12.0f)), 0.45f);
        /* the filter's coefficient for this sample */
        float oct = cur[SM_CUT] + cur[SM_ENV] * v.e[feI].v + aCut;
        float hz = minf(20.0f * fexp2(clampf(oct, -1.5f, 10.6f)), wmax);
        float g = 1.0f - fexp2(-wk * hz * 1.4427f * 1.06f);
        const float drvC = maxf(cur[SM_DRV], 0.0f);
        float k = cur[SM_RES] * 1.24f * (1.0f + 0.28f * drvC);   /* (driven, the resonance bites harder, not less) */
        float dg = 1.0f + 7.0f * drvC * std::sqrt(drvC);
        /* DRIVE: its own stage before the filter (an asymmetric soft clip whose ceiling comes down as it is turned up),
         * so the harmonics it makes go through the filter while the resonance keeps its headroom in the loop: driven,
         * the filter growls and squelches rather than choking its own resonance */
        const float dA = 2.0f - 1.38f * std::sqrt(drvC * std::sqrt(drvC)), dB = 0.6f * dA * drvC, dB0 = dA * softT(dB / dA);
        const float dInv = 1.0f / dA;
        float fmI = cur[SM_FM];
        float sub = 0.0f, o2 = 0.0f, yos[4];
        for (int q = 0; q < os; q++) {
            int wr1 = 0, wr2 = 0;
            float wd1 = 0.0f, wd2 = 0.0f;
            o2 = v.o2.tick(tab, w2, i2, -1.0f, wr2, wd2);
            float ii = i1;
            if (fmI > 0.0f)                      /* FM 2>1: linear, oscillator 1 stalls rather than runs backwards */
                ii = clampf(i1 * (1.0f + fmI * o2), 1e-7f, 0.45f);
            float o1 = v.o1.tick(tab, w1, ii, sync && wr2 ? wd2 : -1.0f, wr1, wd1);
            if (wr1) {                           /* the sub: a square an octave under oscillator 1 */
                v.flip = -v.flip;
                v.subR.add(tab, 2.0f * v.flip, minf(wd1, 0.999f), 0);
            }
            sub = v.subR.out(v.flip);
            float x = cur[SM_G1] * o1 + cur[SM_G2] * o2 + cur[SM_GS] * sub + (cur[SM_GN] + 2e-5f) * nz;
            float pre = dA * softT((x * dg * 1.35f + dB) * dInv) - dB0;   /* (the mixer runs hot, as on the Take 5) */
            float y = f4(pre, g, k);
            yos[q] = y * (1.0f + 0.3f * k) * (1.0f / 1.25f) * (1.0f + 0.75f * drvC) + cur[SM_GB] * o2;
        }
        float y;
        if (os == 4)
            y = v.os1.down(v.os2.down(yos[0], yos[1]), v.os2.down(yos[2], yos[3]));
        else if (os == 2)
            y = v.os1.down(yos[0], yos[1]);
        else
            y = yos[0];
        v.lastO2 = o2;
        v.lastY = y;
        float amp = clampf(cur[SM_AMP] * ae, 0.0f, 1.2f);
        amax = maxf(amax, amp);
        /* the VCA: round at the top, as an analogue one driven by a hot filter */
        float out = softT(y * 0.9f) * (1.0f / 0.9f) * amp * cur[SM_PGM];
        float p0 = cur[SM_PAN];
        float th = (p0 + 1.0f) * (0.25f * PI);
        float gl = std::cos(th) * 1.41421f, gr = std::sin(th) * 1.41421f;
        v.hb[v.hw & 63u] = out;
        float late = out;
        if (haas > 0.01f) {
            float hd = minf(haas, 60.0f);
            uint32_t hi = (uint32_t)hd;
            float hf = hd - (float)hi, h0 = v.hb[(v.hw - hi) & 63u];
            late = h0 + hf * (v.hb[(v.hw - hi - 1u) & 63u] - h0);
        }
        v.hw++;
        if (v.spread < 0.0f) {                   /* (left of centre: the right hears it later) */
            Lo[s] += out * gl;
            Ro[s] += late * gr;
        } else {
            Lo[s] += late * gl;
            Ro[s] += out * gr;
        }
    }
    v.amp = amax;
    /* finished: its amplifier's envelope at its end, its key let go */
    Env &A = v.e[aeI];
    if (A.st == ES_REL && A.v < 3.2e-4f && !v.pend) {
        for (auto &e : v.e) {
            e.v = 0.0f;
            e.st = ES_IDLE;
        }
        if (!v.gate)
            v.key = -1;
    }
    for (float &f : v.f)
        f = flushDn(f);
    v.ffb = flushDn(v.ffb);
    for (float &p : v.pink)
        p = flushDn(p);
    v.os1.flush();
    v.os2.flush();
}

/* ------------------------------------------------------------------ render --- */
void Engine::chunk()
{
    const float dt = (float)CH / sr;
    /* the wheels, the pressure and the pedals glide (5 ms) */
    {
        const float in[6] = {bendV, mwV, prV, brV, ftV, exV}, kc = 1.0f - std::exp(-dt / 0.005f);
        for (int i = 0; i < 6; i++)
            ctl[i] = flushDn(ctl[i] + (in[i] - ctl[i]) * kc);
    }
    clockChunk();
    /* LFO 1: one for all the voices */
    {
        const int16_t *Q = P + P_LFO1;
        if (Q[LFO_SYNC] && !Q[LFO_RESET]) {      /* (synced and free: locked to the beat) */
            int i = (int)clampf((float)Q[LFO_FREQ] * 16.0f / 128.0f, 0.0f, 15.0f);
            double ph = pos / LFO_BEATS[i];
            float nph = (float)(ph - std::floor(ph));
            if (nph < lfo1Ph)
                lfo1Sh = 0.5f + 0.5f * rndF(lfoSeed);
            lfo1Ph = nph;
        } else {
            lfo1Ph += lfoHz(0, gdst[MD_LFO1_FREQ] + gdst[MD_LFO_FREQ]) * dt;
            if (lfo1Ph >= 1.0f) {
                lfo1Ph -= std::floor(lfo1Ph);
                lfo1Sh = 0.5f + 0.5f * rndF(lfoSeed);
            }
        }
        lfo1Raw = lfoShape(Q[LFO_SHAPE], lfo1Ph, lfo1Sh);
        lfo1Out += (lfo1Raw - lfo1Out) * slewK(Q[LFO_SLEW], dt);
    }
    /* the voices */
    std::memset(outL, 0, sizeof outL);
    std::memset(outR, 0, sizeof outR);
    int newest = -1;
    uint32_t na = 0;
    float trackLo = 1e9f;
    for (int i = 0; i < MAXV; i++) {
        Voice &v = V[i];
        if (v.key < 0 && v.e[1].st == ES_IDLE && v.e[2].st == ES_IDLE && !v.pend) {
            v.amp = 0.0f;
            continue;
        }
        if (v.e[1].st == ES_IDLE && v.e[2].st == ES_IDLE && !v.pend && v.fade >= 1.0f) {
            if (!v.gate)
                v.key = -1;
            v.amp = 0.0f;
            continue;
        }
        voiceChunk(v, outL, outR);
        if (v.key >= 0 && v.age >= na) {
            na = v.age;
            newest = i;
        }
        if (v.gate)
            trackLo = minf(trackLo, (float)v.note);
    }
    lfo2Show = newest >= 0 ? V[newest].lout : lfo2Show * 0.9f;
    /* the global destinations (OVERDRIVE, the effects, LFO 1): the newest voice's, or the mono sources alone */
    if (newest >= 0)
        std::memcpy(gdst, V[newest].dst, sizeof gdst);
    else {
        float src[NMODSRC], d[NMODDST];
        sources(nullptr, src);
        modulate(nullptr, src, d, nullptr);
        std::memcpy(gdst, d, sizeof gdst);
    }
    /* the DC let go (8 Hz) */
    for (int i = 0; i < CH; i++) {
        float xl = outL[i], xr = outR[i];
        dcL = xl - dcxL + dcK * dcL;
        dcR = xr - dcxR + dcK * dcR;
        dcxL = xl;
        dcxR = xr;
        outL[i] = dcL;
        outR[i] = dcR;
    }
    dcL = flushDn(dcL);
    dcR = flushDn(dcR);
    /* the output stage: a little weight under 120 Hz, and its top rounded as an analogue line stage's */
    for (int i = 0; i < CH; i++) {
        warmL += (outL[i] - warmL) * warmK;
        warmR += (outR[i] - warmR) * warmK;
        outL[i] = softT((outL[i] + 0.22f * warmL) * 0.8f) * 1.25f;
        outR[i] = softT((outR[i] + 0.22f * warmR) * 0.8f) * 1.25f;
    }
    warmL = flushDn(warmL);
    warmR = flushDn(warmR);
    /* OVERDRIVE, EFFECT 1, the REVERB */
    od.process(outL, outR, CH, ((float)P[P_OVERDRIVE] + gdst[MD_OVERDRIVE]) / 127.0f);
    fx.set(P[P_FX_ON] != 0, P[P_FX_TYPE], (float)P[P_FX_TIME] + gdst[MD_FX_TIME], (float)P[P_FX_MIX] + gdst[MD_FX_MIX],
           (float)P[P_FX_MISC] + gdst[MD_FX_MISC], P[P_FX_SYNC] != 0, bpmCur, trackLo < 1e8f ? hzOf(trackLo) : 0.0f);
    fx.process(outL, outR, CH);
    plate.set(P[P_REV_ON] != 0, (float)P[P_REV_MIX] + gdst[MD_REV_MIX], (float)P[P_REV_DAMP] + gdst[MD_REV_SIZE],
              (float)P[P_REV_PREDELAY] + gdst[MD_REV_PREDELAY], (float)P[P_REV_DECAY] + gdst[MD_REV_DECAY],
              (float)P[P_REV_TONE] + gdst[MD_REV_TONE] * 0.5f);
    plate.process(outL, outR, CH);
    float pk = 0.0f;
    for (int i = 0; i < CH; i++)
        pk = maxf(pk, maxf(std::fabs(outL[i]), std::fabs(outR[i])));
    meterLvl = maxf(pk, meterLvl * 0.985f);
    lastOut = 0.5f * (outL[CH - 1] + outR[CH - 1]);
}

void Engine::render(float *L, float *R, int n)
{
    while (n > 0) {
        if (gpos >= CH) {
            chunk();
            gpos = 0;
        }
        int k = std::min(CH - gpos, n);
        std::memcpy(L, outL + gpos, sizeof(float) * (size_t)k);
        std::memcpy(R, outR + gpos, sizeof(float) * (size_t)k);
        gpos += k;
        L += k;
        R += k;
        n -= k;
    }
}

int Engine::voicesSounding() const
{
    int n = 0;
    for (int i = 0; i < MAXV; i++)
        n += V[i].key >= 0 && (V[i].e[1].st != ES_IDLE || V[i].e[2].st != ES_IDLE);
    return n;
}
} // namespace f5
