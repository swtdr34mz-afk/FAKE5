/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (FAKE5) */
/* FAKE5's effects, as the Take 5's: the OVERDRIVE, EFFECT 1 (twelve types, each with TIME, DEPTH/MIX and FBACK/MISC as
 * the manual's table names them) and EFFECT 2, the plate REVERB (MIX, DAMPING, PRE-DELAY, DECAY, TONE).
 *
 *   Overdrive     a tube-like stage, asymmetric (the even harmonics), at 2x, level-matched, rounding off its own top
 *   Delay Stereo  a clean digital ping-pong
 *   BBD Delay     a bucket brigade: darker with longer times, its clock wobbling, saturating in its loop
 *   Tape Delay 1  a tape echo: wow and flutter, each repeat a little duller and thinner, the tape's soft compression
 *   Tape Delay 2  the same machine driven harder: more saturation, darker, more wow
 *   Chorus        a stereo bucket-brigade chorus, two voices a side, its wet through a low-pass (FBACK/MISC)
 *   Flanger       a through-the-middle flanger with feedback
 *   Phaser        six stages with feedback, the two sides a quarter apart
 *   HP Filter     a resonant 12 dB high-pass
 *   Distortion    gain, output level, tone
 *   Ring Mod      a sine carrier, or the lowest key's pitch times a ratio (KEY TRACK)
 *   Rotating Spkr a horn and a drum turning (each its own inertia), a crossover, the cabinet's drive, two mics
 *   Lo-Fi         a badly calibrated tape machine: wow and flutter, depth and the head's azimuth, drive, hiss
 *   Reverb        Dattorro's plate: four input diffusers into a figure-eight tank of two modulated allpasses, damped,
 *                 seven taps a side; PRE-DELAY with early reflections; TONE tilts its colour (LPF left, HPF right)
 *
 * Everything glides (no zipper, no click), a new type crossfades in. */
#pragma once
#include "Dsp.h"
#include "Params.h"
#include <vector>

namespace f5
{
/* a delay line of a power of two, read at a fractional distance (cubic) */
struct DLine {
    std::vector<float> b;
    int n = 1, w = 0;
    void alloc(int len)
    {
        n = 1;
        while (n < len + 4)
            n <<= 1;
        b.assign((size_t)n, 0.0f);
        w = 0;
    }
    void clear()
    {
        std::fill(b.begin(), b.end(), 0.0f);
        w = 0;
    }
    inline void push(float x)
    {
        b[(size_t)w] = x;
        w = (w + 1) & (n - 1);
    }
    inline float tap(int d) const { return b[(size_t)((w - 1 - d) & (n - 1))]; }
    inline float read(float d) const                 /* d samples back (>= 1), Hermite */
    {
        d = clampf(d, 1.0f, (float)(n - 4));
        int i = (int)d;
        float f = d - (float)i;
        float y0 = tap(i - 1), y1 = tap(i), y2 = tap(i + 1), y3 = tap(i + 2);
        float c1 = 0.5f * (y2 - y0), c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3, c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
        return ((c3 * f + c2) * f + c1) * f + y1;
    }
};

struct OnePole {
    float s = 0.0f;
    inline float lp(float x, float k) { return s += (x - s) * k; }
    inline float hp(float x, float k) { return x - lp(x, k); }
};
inline float opK(float hz, float sr) { return 1.0f - std::exp(-2.0f * PI * clampf(hz, 1.0f, 0.45f * sr) / sr); }

/* a state-variable filter (Simper's), low and high outputs */
struct Svf {
    float s1 = 0, s2 = 0;
    inline void run(float x, float g, float k, float &lo, float &hi, float &bp)
    {
        float a1 = 1.0f / (1.0f + g * (g + k)), a2 = g * a1;
        float v3 = x - s2, v1 = a1 * s1 + a2 * v3, v2 = s2 + g * v1;
        s1 = 2.0f * v1 - s1;
        s2 = 2.0f * v2 - s2;
        lo = v2;
        bp = v1;
        hi = x - k * v1 - v2;
    }
    void flush()
    {
        s1 = flushDn(s1);
        s2 = flushDn(s2);
    }
};

/* ============================================================================ OVERDRIVE === */
class Overdrive
{
public:
    void prepare(float sampleRate)
    {
        sr = sampleRate;
        clear();
    }
    void clear()
    {
        for (int c = 0; c < 2; c++) {
            hb[c] = HalfBand();
            tone[c] = OnePole();
            dcx[c] = dcy[c] = 0.0f;
        }
        cur = -1.0f;
    }
    void process(float *L, float *R, int n, float amount)
    {
        float a = clampf(amount, 0.0f, 1.0f);
        if (cur < 0.0f)
            cur = a;
        if (a <= 0.0f && cur <= 1e-4f) {
            cur = 0.0f;
            return;
        }
        float step = (a - cur) / (float)n;
        const float kd = 1.0f - 2.0f * PI * 12.0f / sr;
        for (int i = 0; i < n; i++) {
            cur += step;
            float d = cur * cur;
            float g = 1.0f + 38.0f * d, comp = (1.0f - 0.32f * clampf(cur * 2.0f, 0.0f, 1.0f)) / (1.0f + 0.5f * std::sqrt(d * 38.0f));
            float k = opK(16000.0f - 9000.0f * cur, sr);   /* (driven harder, its top rounds off) */
            for (int c = 0; c < 2; c++) {
                float x = c ? R[i] : L[i];
                float u0, u1;
                hb[c].up(x * g, u0, u1);
                u0 = shape(u0);
                u1 = shape(u1);
                float y = hb[c].down(u0, u1);
                /* (the asymmetry's DC let go) */
                float o = y - dcx[c] + kd * dcy[c];
                dcx[c] = y;
                dcy[c] = o;
                o = tone[c].lp(o, k) * comp;
                float wet = clampf(cur * 6.0f, 0.0f, 1.0f);   /* (from nothing: the first degree blends in) */
                (c ? R[i] : L[i]) = x * (1.0f - wet) + o * wet;
            }
        }
        cur = a;
        for (int c = 0; c < 2; c++) {
            hb[c].flush();
            dcx[c] = flushDn(dcx[c]);
            dcy[c] = flushDn(dcy[c]);
            tone[c].s = flushDn(tone[c].s);
        }
    }

private:
    static inline float shape(float x)
    {
        /* a triode's curve: the positive side softer and later than the negative */
        float y = x >= 0.0f ? softT(x * 0.9f) * 1.08f : softT(x * 1.15f) * 0.94f;
        return y + 0.06f * y * y;
    }
    float sr = 44100.0f, cur = -1.0f;
    HalfBand hb[2];
    OnePole tone[2];
    float dcx[2] = {}, dcy[2] = {};
};

/* ============================================================================== EFFECT 1 === */
class Effect
{
public:
    void prepare(float sampleRate)
    {
        sr = sampleRate;
        dl[0].alloc((int)(sr * 2.1f));
        dl[1].alloc((int)(sr * 2.1f));
        md[0].alloc((int)(sr * 0.06f));
        md[1].alloc((int)(sr * 0.06f));
        clear();
    }
    void clear()
    {
        for (auto &l : dl)
            l.clear();
        for (auto &l : md)
            l.clear();
        for (int c = 0; c < 2; c++) {
            for (auto &o : lp[c])
                o = OnePole();
            for (auto &o : hp[c])
                o = OnePole();
            svf[c] = Svf();
            for (float &p : ap[c])
                p = 0.0f;
            fb[c] = 0.0f;
        }
        rotHorn = rotDrum = 0.0f;
        tail = 0.0f;
    }
    /* each block: on, type, the three knobs 0..127 (modulation added), synced (delays), the tempo, the lowest key (Hz,
     * RING MOD's tracking; 0 none) */
    void set(bool on_, int type_, float time_, float mix_, float misc_, bool synced_, float bpm_, float trackHz_)
    {
        wantOn = on_;
        wantType = type_ < 0 ? 0 : type_ >= NFXTYPE ? NFXTYPE - 1 : type_;
        tT = clampf(time_, 0.0f, 127.0f) / 127.0f;
        tM = clampf(mix_, 0.0f, 127.0f) / 127.0f;
        tF = clampf(misc_, 0.0f, 127.0f) / 127.0f;
        synced = synced_;
        bpm = bpm_ < 20.0f ? 120.0f : bpm_;
        trackHz = trackHz_;
        if (!init) {
            type = wantType;
            sT = tT;
            sM = tM;
            sF = tF;
            xf = wantOn ? 1.0f : 0.0f;
            dTime = targetTime();
            init = true;
        }
    }
    float tailLevel() const { return tail; }

    void process(float *L, float *R, int n)
    {
        const float kp = 1.0f - std::exp(-1.0f / (0.02f * sr));     /* the knobs' glide: 20 ms */
        const float kx = 1.0f / (0.015f * sr);                      /* a crossfade: 15 ms */
        for (int i = 0; i < n; i++) {
            if ((wantType != type || !wantOn) && xf > 0.0f) {
                xf -= kx;
                if (xf <= 0.0f) {
                    xf = 0.0f;
                    if (wantType != type) {
                        type = wantType;
                        clearState();
                    }
                }
            } else if (wantType == type && wantOn && xf < 1.0f)
                xf = minf(1.0f, xf + kx);
            sT += (tT - sT) * kp;
            sM += (tM - sM) * kp;
            sF += (tF - sF) * kp;
            float x0 = L[i], x1 = R[i];
            if (xf <= 0.0f) {
                tail *= 0.9999f;
                continue;
            }
            float y0 = x0, y1 = x1;
            run(x0, x1, y0, y1);
            L[i] = x0 + (y0 - x0) * xf;
            R[i] = x1 + (y1 - x1) * xf;
            float a = std::fabs(y0 - x0) + std::fabs(y1 - x1);
            tail = a > tail ? a : tail * 0.99995f;
        }
        for (int c = 0; c < 2; c++) {
            for (auto &o : lp[c])
                o.s = flushDn(o.s);
            for (auto &o : hp[c])
                o.s = flushDn(o.s);
            svf[c].flush();
            for (float &p : ap[c])
                p = flushDn(p);
            fb[c] = flushDn(fb[c]);
        }
    }

private:
    static constexpr float DLY_BEATS[11] = {4.0f, 3.0f, 2.0f, 1.5f, 1.0f, 2.0f / 3.0f, 0.75f, 0.5f, 1.0f / 3.0f, 0.375f, 0.25f};
    float sr = 44100.0f;
    bool init = false, wantOn = false, synced = false;
    int wantType = FX_BBD, type = FX_BBD;
    float tT = 0, tM = 0, tF = 0, sT = 0, sM = 0, sF = 0, xf = 0, bpm = 120, trackHz = 0, tail = 0;
    float dTime = 0.3f;                                   /* the delay's time, gliding (seconds: a tape's bend) */
    DLine dl[2], md[2];
    OnePole lp[2][3], hp[2][2];
    Svf svf[2];
    float ap[2][6] = {}, fb[2] = {};
    float ph1 = 0, ph2 = 0, ph3 = 0, rotHorn = 0, rotDrum = 0, hornSpeed = 0.8f, drumSpeed = 0.7f, ringPh = 0;
    float wow = 0, wowT = 0;
    uint32_t seed = 0x13579BDFu;

    void clearState()
    {
        float keep = tail;
        clear();
        tail = keep;
    }
    float targetTime() const
    {
        if (synced) {
            int k = (int)(sT * 127.0f * 11.0f / 128.0f);
            float t = DLY_BEATS[k < 0 ? 0 : k > 10 ? 10 : k] * 60.0f / bpm;
            while (t > 2.0f)
                t *= 0.5f;
            return t;
        }
        return 0.004f * std::exp2(sT * 7.95f);          /* 4 ms .. 1 s */
    }
    static inline void mixDW(float m, float &d, float &w)   /* MIX: the dry full to the middle, the wet full from it */
    {
        d = minf(1.0f, 2.0f * (1.0f - m));
        w = minf(1.0f, 2.0f * m);
    }
    inline float lfo(float &ph, float hz)
    {
        ph += hz / sr;
        if (ph >= 1.0f)
            ph -= 1.0f;
        return sinTurn(ph);
    }

    void run(float x0, float x1, float &yl, float &yr)
    {
        switch (type) {
        case FX_DELAY:
        case FX_BBD:
        case FX_TAPE1:
        case FX_TAPE2: delays(x0, x1, yl, yr); break;
        case FX_CHORUS: {
            float rate = 0.08f * std::exp2(sT * 6.6f), depth = sM;
            float a = lfo(ph1, rate), b = lfo(ph2, rate * 0.71f);
            md[0].push(x0);
            md[1].push(x1);
            float base = 0.0065f * sr, sw = (0.0004f + 0.0055f * depth) * sr;
            float l = 0.5f * (md[0].read(base + sw * a) + md[0].read(base * 1.4f + sw * 0.8f * b));
            float r = 0.5f * (md[1].read(base - sw * a) + md[1].read(base * 1.4f - sw * 0.8f * b));
            float k = opK(1200.0f * std::exp2(sF * 4.0f), sr);
            l = lp[0][0].lp(l, k);
            r = lp[1][0].lp(r, k);
            yl = 0.72f * x0 + 0.72f * l;
            yr = 0.72f * x1 + 0.72f * r;
            break;
        }
        case FX_FLANGER: {
            float rate = 0.03f * std::exp2(sT * 7.5f), depth = sM, f = sF * 0.93f;
            float a = 0.5f + 0.5f * lfo(ph1, rate), b = 0.5f + 0.5f * std::sin(2.0f * PI * (ph1 + 0.25f));
            md[0].push(x0 + softT(fb[0] * f));
            md[1].push(x1 + softT(fb[1] * f));
            float base = 0.0003f * sr, sw = (0.0003f + 0.0068f * depth) * sr;
            float l = md[0].read(base + sw * a), r = md[1].read(base + sw * b);
            fb[0] = l;
            fb[1] = r;
            yl = 0.62f * (x0 + l);
            yr = 0.62f * (x1 + r);
            break;
        }
        case FX_PHASER: {
            float rate = 0.03f * std::exp2(sT * 7.5f), depth = sM, f = sF * 0.85f;
            lfo(ph1, rate);
            for (int c = 0; c < 2; c++) {
                float m = 0.5f + 0.5f * sinTurn(ph1 + (c ? 0.25f : 0.0f));
                float fc = 160.0f * std::exp2(depth * 4.8f * m + (1.0f - depth) * 2.2f);
                float t = tanW(clampf(PI * fc / sr, 0.0f, 1.2f)), a = (t - 1.0f) / (t + 1.0f);
                float x = (c ? x1 : x0) + softT(fb[c] * f);
                for (int k = 0; k < 6; k++) {
                    float y = a * x + ap[c][k];
                    ap[c][k] = x - a * y;
                    x = y;
                }
                fb[c] = x;
                (c ? yr : yl) = 0.62f * ((c ? x1 : x0) + x);
            }
            break;
        }
        case FX_HPF: {
            float fc = 20.0f * std::exp2(sT * 9.0f), q = 0.55f + sF * sF * 11.0f;
            float g = tanW(clampf(PI * fc / sr, 0.0f, 1.2f)), k = 1.0f / q, d = 1.0f - sM, w = sM;
            for (int c = 0; c < 2; c++) {
                float lo, hi, bp, x = c ? x1 : x0;
                svf[c].run(x, g, k, lo, hi, bp);
                (c ? yr : yl) = x * d + softT(hi * 0.8f) * 1.25f * w;
            }
            break;
        }
        case FX_DISTORTION: {
            float g = 1.0f + std::pow(sT, 1.6f) * 70.0f, lvl = sM * 0.85f;
            float k = opK(700.0f * std::exp2(sF * 4.6f), sr);
            float pre = opK(5200.0f, sr);
            for (int c = 0; c < 2; c++) {
                float x = c ? x1 : x0;
                float h = hp[c][0].hp(x, opK(90.0f, sr));
                float e = h + 0.4f * (h - lp[c][1].lp(h, pre));  /* (a little presence into the clipper) */
                float y = softT(e * g);
                y = 0.7f * y + 0.3f * clampf(e * g * 0.6f, -1.0f, 1.0f);
                (c ? yr : yl) = lp[c][0].lp(y, k) * lvl / (1.0f + 0.15f * std::sqrt(g));
            }
            break;
        }
        case FX_RINGMOD: {
            float f;
            if (sF >= 0.5f && trackHz > 0.0f)
                f = trackHz * std::exp2((sT - 0.5f) * 4.0f);     /* (a ratio to the lowest key: two octaves each way) */
            else
                f = 20.0f * std::exp2(sT * 8.0f);
            ringPh += f / sr;
            if (ringPh >= 1.0f)
                ringPh -= std::floor(ringPh);
            float c = sinTurn(ringPh), d = 1.0f - sM, w = sM * 1.3f;
            yl = x0 * d + x0 * c * w;
            yr = x1 * d + x1 * c * w;
            break;
        }
        case FX_ROTARY: rotary(x0, x1, yl, yr); break;
        case FX_LOFI: lofi(x0, x1, yl, yr); break;
        default: yl = x0; yr = x1;
        }
    }

    void delays(float x0, float x1, float &yl, float &yr)
    {
        float t = targetTime();
        bool tape = type == FX_TAPE1 || type == FX_TAPE2;
        float kt = type == FX_DELAY ? 0.00045f : 0.00012f;
        kt = 1.0f - std::pow(1.0f - kt, 44100.0f / sr);
        dTime += (t - dTime) * kt;
        float fbk = minf(sF * 1.04f, 0.985f), d, w;
        mixDW(sM, d, w);
        float D = dTime * sr;
        if (type == FX_DELAY) {                             /* ping-pong: in on the left, across with each repeat */
            float a = dl[0].read(D), b = dl[1].read(D);
            dl[0].push(0.5f * (x0 + x1) + b * fbk);
            dl[1].push(a * fbk);
            yl = x0 * d + a * w;
            yr = x1 * d + b * w;
            return;
        }
        if (type == FX_BBD) {                               /* dark and darker with time, its clock wobbling */
            float wb = lfo(ph1, 0.55f) * 0.0022f + lfo(ph2, 4.7f) * 0.0004f;
            float fc = clampf(8200.0f - dTime * 5200.0f, 1700.0f, 8200.0f), k = opK(fc, sr);
            for (int c = 0; c < 2; c++) {
                float y = dl[c].read(D * (c ? 1.013f : 1.0f) * (1.0f + wb));
                y = lp[c][0].lp(lp[c][1].lp(y, k), k);
                float in = c ? x1 : x0;
                dl[c].push(softT((in + y * fbk) * 0.85f) * 1.12f + 2e-6f * rndF(seed));
                y *= 0.85f;
                (c ? yr : yl) = in * d + y * w;
            }
            return;
        }
        /* tape: wow (slow, wandering) and flutter (fast), each repeat duller and thinner, saturating in its loop */
        (void)tape;
        bool hard = type == FX_TAPE2;
        if (--wowT < 0.0f) {
            wowT = sr * 0.25f;
            wow = rndF(seed);
        }
        float wv = lfo(ph1, hard ? 0.45f : 0.6f) * (hard ? 0.0042f : 0.0028f) + lfo(ph2, 6.3f) * 0.00055f + wow * 0.0006f;
        float klp = opK(hard ? 3800.0f : 6200.0f, sr), khp = opK(hard ? 120.0f : 75.0f, sr);
        float drive = hard ? 2.6f : 1.35f;
        for (int c = 0; c < 2; c++) {
            float y = dl[c].read(D * (c ? 1.006f : 1.0f) * (1.0f + wv));
            y = hp[c][0].hp(lp[c][0].lp(y, klp), khp);
            float in = c ? x1 : x0;
            float s = softT((in + y * fbk) * drive) / drive * (hard ? 1.25f : 1.08f);   /* (the loop's small-signal gain at one) */
            dl[c].push(s + 4e-6f * rndF(seed));
            (c ? yr : yl) = in * d + y * w;
        }
    }

    /* a Leslie: the horn (above 800 Hz) and the drum (below) turning, each speeding up and slowing down at its own
     * pace; the horn's doppler and both their tremolo heard by two mics */
    void rotary(float x0, float x1, float &yl, float &yr)
    {
        float target = 0.6f * std::exp2(sT * 3.55f);       /* 0.6 .. 7 turns a second */
        hornSpeed += (target - hornSpeed) * (1.0f / (0.9f * sr));
        drumSpeed += (target * 0.88f - drumSpeed) * (1.0f / (3.2f * sr));
        rotHorn += hornSpeed / sr;
        rotDrum += drumSpeed / sr;
        rotHorn -= std::floor(rotHorn);
        rotDrum -= std::floor(rotDrum);
        float drive = 1.0f + sM * sM * 9.0f;
        float x = softT(0.5f * (x0 + x1) * drive) / (0.6f + 0.4f * softT(drive * 0.5f)) * (1.0f / (1.0f + 0.12f * drive));
        float lo = lp[0][0].lp(lp[0][1].lp(x, opK(800.0f, sr)), opK(800.0f, sr)), hi = x - lo;
        float near = 1.0f - sF;                             /* MIC DIST: near, deep; far, gentle */
        md[0].push(hi);
        float depthS = (0.00025f + 0.0006f * near) * sr;
        float hl = md[0].read(0.0012f * sr + depthS * sinTurn(rotHorn));
        float hr = md[0].read(0.0012f * sr + depthS * sinTurn(rotHorn + 0.5f));
        float am = 0.25f + 0.35f * near;
        hl *= 1.0f - am * (0.5f + 0.5f * sinTurn(rotHorn + 0.25f));
        hr *= 1.0f - am * (0.5f + 0.5f * sinTurn(rotHorn + 0.75f));
        float dm = 0.12f + 0.2f * near;
        float dlv = lo * (1.0f - dm * (0.5f + 0.5f * sinTurn(rotDrum + 0.25f)));
        float drv = lo * (1.0f - dm * (0.5f + 0.5f * sinTurn(rotDrum + 0.75f)));
        float width = 0.55f + 0.45f * near;
        float l = hl + dlv, r = hr + drv, m = 0.5f * (l + r), s = 0.5f * (l - r) * width;
        yl = (m + s) * 0.62f;
        yr = (m - s) * 0.62f;
    }

    /* Lo-Fi: a tape machine out of calibration */
    void lofi(float x0, float x1, float &yl, float &yr)
    {
        if (--wowT < 0.0f) {
            wowT = sr * 0.35f;
            wow = rndF(seed);
        }
        wowSm += (wow - wowSm) * (1.0f / (0.3f * sr));
        float amt = sT;
        float mod = (lfo(ph1, 0.42f) * 0.6f + wowSm * 0.6f) * 0.0045f * amt + lfo(ph2, 7.1f) * 0.0006f * amt;
        float depth = sM, drive = 1.0f + sF * sF * 7.0f;
        float bw = 13000.0f - 8500.0f * depth;
        float k = opK(bw, sr), kh = opK(70.0f + 90.0f * depth, sr);
        md[0].push(x0);
        md[1].push(x1);
        float base = 0.006f * sr;
        float azi = depth * 0.00022f * sr;                  /* (the head tilted: the right side's top a little late) */
        float l = md[0].read(base * (1.0f + mod)), r = md[1].read(base * (1.0f + mod) + azi * (0.6f + 0.4f * lfo(ph3, 0.07f)));
        for (int c = 0; c < 2; c++) {
            float y = c ? r : l;
            y = softT(y * drive) / (0.35f + 0.65f * drive);
            y = hp[c][0].hp(lp[c][0].lp(lp[c][1].lp(y, k), k), kh);
            y += (1.2e-4f + 4e-4f * depth) * rndF(seed);    /* hiss */
            (c ? yr : yl) = y * 1.05f;
        }
    }
    float wowSm = 0.0f;
};

/* ================================================================================ REVERB === */
/* Dattorro's plate (J. Dattorro, "Effect Design, Part 1", JAES 1997), its lengths at its own 29761 Hz scaled to the
 * rate: the signature plate of the Take 5's second effect. */
class Plate
{
public:
    void prepare(float sampleRate)
    {
        sr = sampleRate;
        const float s = sr / 29761.0f;
        auto L = [s](float n) { return (int)std::lround(n * s); };
        pre.alloc((int)(sr * 0.32f));
        for (int i = 0; i < 4; i++) {
            inAp[i].alloc(L(IN_AP[i]) + 2);
            inLen[i] = L(IN_AP[i]);
        }
        exc = 13.0f * s;
        for (int t = 0; t < 2; t++) {
            mAp[t].alloc(L(MAP[t]) + (int)exc + 8);
            mLen[t] = (float)L(MAP[t]);
            d1[t].alloc(L(D1[t]) + 2);
            d1Len[t] = L(D1[t]);
            ap2[t].alloc(L(AP2[t]) + 2);
            ap2Len[t] = L(AP2[t]);
            d2[t].alloc(L(D2[t]) + 2);
            d2Len[t] = L(D2[t]);
        }
        for (int i = 0; i < 7; i++) {
            tapL[i] = L(TAP_L[i]);
            tapR[i] = L(TAP_R[i]);
        }
        loopSec = (MAP[0] + D1[0] + AP2[0] + D2[0]) / 29761.0f;
        clear();
    }
    void clear()
    {
        pre.clear();
        for (auto &a : inAp)
            a.clear();
        for (int t = 0; t < 2; t++) {
            mAp[t].clear();
            d1[t].clear();
            ap2[t].clear();
            d2[t].clear();
            damp[t] = OnePole();
            tone[t] = Svf();
            tank[t] = 0.0f;
        }
        bw = OnePole();
        inHp = OnePole();
        ph[0] = 0.0f;
        ph[1] = 0.37f;
    }
    /* each block: on, MIX, DAMPING, PRE-DELAY, DECAY (0..127), TONE (-64..63), modulation added */
    void set(bool on_, float mix_, float damp_, float pre_, float decay_, float tone_)
    {
        wantOn = on_;
        tMix = clampf(mix_, 0.0f, 127.0f) / 127.0f;
        tDamp = clampf(damp_, 0.0f, 127.0f) / 127.0f;
        tPre = clampf(pre_, 0.0f, 127.0f) / 127.0f;
        tDecay = clampf(decay_, 0.0f, 127.0f) / 127.0f;
        tTone = clampf(tone_, -64.0f, 64.0f) / 64.0f;
        if (!init) {
            sMix = tMix;
            sDamp = tDamp;
            sPre = tPre;
            sDecay = tDecay;
            sTone = tTone;
            on = wantOn ? 1.0f : 0.0f;
            init = true;
        }
    }
    bool sounding() const { return on > 0.0f || tailE > 1e-6f; }

    void process(float *L, float *R, int n)
    {
        const float kp = 1.0f - std::exp(-1.0f / (0.03f * sr));
        const float kOn = 1.0f / (0.02f * sr);
        for (int i = 0; i < n; i++) {
            on = wantOn ? minf(1.0f, on + kOn) : maxf(0.0f, on - kOn);
            if (on <= 0.0f && tailE < 1e-7f)
                continue;
            sMix += (tMix - sMix) * kp;
            sDamp += (tDamp - sDamp) * kp;
            sPre += (tPre - sPre) * kp;
            sDecay += (tDecay - sDecay) * kp;
            sTone += (tTone - sTone) * kp;
            /* once per sample the coefficients that glide (cheap: a few exps) */
            if ((i & 7) == 0)
                coefficients();
            float x0 = L[i], x1 = R[i];
            /* in: mono, its lows thinned (a plate never booms), the bandwidth, the pre-delay */
            float in = inHp.hp(0.5f * (x0 + x1), kInHp) * on;
            in = bw.lp(in, kBw);
            pre.push(in);
            float x = pre.read(maxf(1.0f, preSamp));
            for (int k = 0; k < 4; k++) {                       /* input diffusion */
                float g = k < 2 ? 0.75f : 0.625f;
                float z = inAp[k].tap(inLen[k] - 1);
                float v = x - g * z;
                inAp[k].push(v);
                x = z + g * v;
            }
            /* the tank: two halves in a figure eight */
            float m0 = sinTurn(ph[0]) * exc, m1 = sinTurn(ph[1]) * exc;
            ph[0] += 0.53f / sr;
            ph[1] += 0.71f / sr;
            if (ph[0] >= 1.0f)
                ph[0] -= 1.0f;
            if (ph[1] >= 1.0f)
                ph[1] -= 1.0f;
            float mod[2] = {m0, m1};
            for (int t = 0; t < 2; t++) {
                float u = x + tank[1 - t] * g;
                /* the modulated allpass (decay diffusion 1: -0.7) */
                float z = mAp[t].read(mLen[t] + mod[t]);
                float v = u + 0.7f * z;
                mAp[t].push(v);
                float a = z - 0.7f * v;
                d1[t].push(a);
                float b = d1[t].tap(d1Len[t] - 1);
                b = damp[t].lp(b, kDamp);
                b *= g;
                /* the second allpass (decay diffusion 2: 0.5) */
                float z2 = ap2[t].tap(ap2Len[t] - 1);
                float v2 = b - 0.5f * z2;
                ap2[t].push(v2);
                float c = z2 + 0.5f * v2;
                d2[t].push(c);
                tank[t] = softT(d2[t].tap(d2Len[t] - 1) * 0.5f) * 2.0f;   /* (never runs away, even at the top) */
            }
            /* seven taps a side */
            float yl = d1[0].tap(tapL[0]) + d1[0].tap(tapL[1]) - ap2[0].tap(tapL[2]) + d2[0].tap(tapL[3]) - d1[1].tap(tapL[4]) -
                       ap2[1].tap(tapL[5]) - d2[1].tap(tapL[6]);
            float yr = d1[1].tap(tapR[0]) + d1[1].tap(tapR[1]) - ap2[1].tap(tapR[2]) + d2[1].tap(tapR[3]) - d1[0].tap(tapR[4]) -
                       ap2[0].tap(tapR[5]) - d2[0].tap(tapR[6]);
            yl *= 0.62f;
            yr *= 0.62f;
            /* early reflections: PRE-DELAY brings them in */
            if (erGain > 0.0f) {
                float el = 0.0f, er = 0.0f;
                for (int k = 0; k < 6; k++) {
                    float tp = pre.read(maxf(1.0f, ER_T[k] * erScale * sr));
                    (k & 1 ? er : el) += tp * ER_G[k];
                }
                yl += el * erGain;
                yr += er * erGain;
            }
            /* TONE: left the highs off, right the lows */
            float lo, hi, bp, ol = yl, orr = yr;
            tone[0].run(yl, gTone, 1.2f, lo, hi, bp);
            ol = sTone < 0.0f ? lo : sTone > 0.0f ? hi : yl;
            tone[1].run(yr, gTone, 1.2f, lo, hi, bp);
            orr = sTone < 0.0f ? lo : sTone > 0.0f ? hi : yr;
            float tw = std::fabs(sTone) < 0.02f ? 0.0f : 1.0f;
            yl = yl + (ol - yl) * tw;
            yr = yr + (orr - yr) * tw;
            tailE = maxf(std::fabs(yl) + std::fabs(yr), tailE * 0.99997f);
            /* MIX: the wet comes in gently (a little is a little: the low half of the knob a touch of room), the dry
             * stays until the top half */
            float dry = std::cos(sMix * sMix * 0.5f * PI), wet = sMix * std::sqrt(sMix) * 0.8f;
            float wo = maxf(on, 0.0f);
            dry = 1.0f + (dry - 1.0f) * wo;
            L[i] = x0 * dry + yl * wet;
            R[i] = x1 * dry + yr * wet;
        }
        for (int t = 0; t < 2; t++) {
            damp[t].s = flushDn(damp[t].s);
            tone[t].flush();
            tank[t] = flushDn(tank[t]);
        }
        bw.s = flushDn(bw.s);
        inHp.s = flushDn(inHp.s);
    }

private:
    static constexpr float IN_AP[4] = {142, 107, 379, 277};
    static constexpr float MAP[2] = {672, 908}, D1[2] = {4453, 4217}, AP2[2] = {1800, 2656}, D2[2] = {3720, 3163};
    static constexpr float TAP_L[7] = {266, 2974, 1913, 1996, 1990, 187, 1066};
    static constexpr float TAP_R[7] = {353, 3627, 1228, 2673, 2111, 335, 121};
    static constexpr float ER_T[6] = {0.0043f, 0.0071f, 0.0119f, 0.0157f, 0.0213f, 0.0277f};
    static constexpr float ER_G[6] = {0.62f, 0.55f, 0.47f, 0.41f, 0.33f, 0.27f};

    void coefficients()
    {
        /* DECAY: 0.3 s to 12 s (the gain at each of a half's two decay points, for that T60) */
        float t60 = 0.3f * std::exp2(sDecay * 5.3f);
        g = minf(std::pow(10.0f, -1.5f * loopSec / t60), 0.9992f);
        /* DAMPING: the tank's highs from 16 kHz down to 1.4 kHz */
        kDamp = opK(16000.0f * std::exp2(-sDamp * 3.5f), sr);
        kBw = opK(15000.0f - 4000.0f * sDamp, sr);
        kInHp = opK(70.0f, sr);
        /* PRE-DELAY: 0..200 ms, the early reflections with it */
        preSamp = sPre * sPre * 0.2f * sr + 1.0f;
        erGain = sPre * 0.55f;
        erScale = 0.5f + 1.5f * sPre;
        /* TONE: a low-pass down to 1.2 kHz, or a high-pass up to 1.5 kHz */
        float a = std::fabs(sTone);
        float fc = sTone < 0.0f ? 18000.0f * std::exp2(-a * 3.9f) : 25.0f * std::exp2(a * 5.9f);
        gTone = tanW(clampf(PI * fc / sr, 0.0f, 1.2f));
    }

    float sr = 44100.0f;
    bool init = false, wantOn = false;
    float on = 0.0f, tailE = 0.0f;
    float tMix = 0, tDamp = 0, tPre = 0, tDecay = 0, tTone = 0, sMix = 0, sDamp = 0, sPre = 0, sDecay = 0, sTone = 0;
    float g = 0.5f, kDamp = 0.5f, kBw = 0.9f, kInHp = 0.01f, preSamp = 1.0f, erGain = 0.0f, erScale = 1.0f, gTone = 0.5f;
    float loopSec = 0.35f, exc = 13.0f;
    DLine pre, inAp[4], mAp[2], d1[2], ap2[2], d2[2];
    int inLen[4] = {}, d1Len[2] = {}, ap2Len[2] = {}, d2Len[2] = {}, tapL[7] = {}, tapR[7] = {};
    float mLen[2] = {};
    OnePole damp[2], bw, inHp;
    Svf tone[2];
    float tank[2] = {}, ph[2] = {};
};
} // namespace f5
