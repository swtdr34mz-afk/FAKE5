/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (FAKE5) */
/* FAKE5's building blocks, from PROPHET 67's and REEVES' engine (the same band-limited corners, ladder and half-bands):
 * band-limited oscillators (a windowed sinc's step over 16 samples, each corner placed to 1/1024 of a sample, -80 dB
 * of aliasing) with the Take 5's continuous SHAPE (sine, sawtooth, square, narrowing pulse), the low-pass as four
 * one-poles in a loop solved without a delay with saturating stages, a polyphase half-band for the 2x and 4x, and
 * small helpers. */
#pragma once
#include <cmath>
#include <cstdint>
#include <cstring>

namespace f5
{
constexpr float PI = 3.14159265358979f;

inline float clampf(float x, float lo, float hi) { return x < lo ? lo : x > hi ? hi : x; }
inline float minf(float a, float b) { return a < b ? a : b; }
inline float maxf(float a, float b) { return a > b ? a : b; }
/* 2^x, fast (a polynomial: 2e-7 off), for what runs per sample */
inline float fexp2(float x)
{
    if (x < -126.0f)
        return 0.0f;
    if (x > 127.0f)
        x = 127.0f;
    float fl = std::floor(x), f = x - fl;
    float p = 1.534580891e-04f;
    p = p * f + 1.339993158e-03f;
    p = p * f + 9.618488972e-03f;
    p = p * f + 5.550328776e-02f;
    p = p * f + 2.402264689e-01f;
    p = p * f + 6.931472057e-01f;
    p = p * f + 1.0f;
    return std::ldexp(p, (int)fl);
}
/* a transconductor's curve: L tanh(x / L), L the level it saturates at (Pade, flat at +-3) */
inline float ota(float x)
{
    const float L = 1.25f;
    float t = x * (1.0f / L);
    if (t > 3.0f)
        return L;
    if (t < -3.0f)
        return -L;
    float t2 = t * t;
    return L * t * (27.0f + t2) / (27.0f + 9.0f * t2);
}
inline float hzOf(float midi) { return 440.0f * std::exp2((midi - 69.0f) * (1.0f / 12.0f)); }
inline float flushDn(float x) { return std::fabs(x) < 1e-15f ? 0.0f : x; }
inline float tanW(float w)                          /* tan(w) for the prewarp, w < ~1.2 */
{
    float w2 = w * w;
    return w * (1.0f + w2 * (0.33333f + w2 * (0.13333f + w2 * 0.05397f)));
}
inline uint32_t rndU(uint32_t &s)
{
    s ^= s << 13;
    s ^= s >> 17;
    s ^= s << 5;
    return s;
}
inline float rndF(uint32_t &s) { return (float)(int32_t)rndU(s) * (1.0f / 2147483648.0f); }   /* -1..1 */
/* the filter's input stage: a soft clip without a division (flat at +-1.5, where it reaches 1) */
inline float sat(float x)
{
    x = clampf(x, -1.5f, 1.5f);
    return x - 0.148148f * x * x * x;
}
/* a gentle tanh (Pade, flat at +-3) */
inline float softT(float x)
{
    if (x > 3.0f)
        return 1.0f;
    if (x < -3.0f)
        return -1.0f;
    float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}
/* sin(2 pi ph), ph 0..1 (a parabola refined: 0.1 % off, no table) */
inline float sinTurn(float ph)
{
    float x = ph - std::floor(ph);
    x = x < 0.5f ? x : x - 1.0f;                   /* -0.5..0.5 */
    float y = 8.0f * x - 16.0f * x * std::fabs(x);
    return 0.225f * (y * std::fabs(y) - y) + y;
}

/* ----------------------------------------------------------- the BLEP tables --- */
constexpr int BL_H = 8, BL_OS = 1024, BL_N = 2 * BL_H * BL_OS + 2;
constexpr uint32_t ORING = 32u;
const float *blepTables();                           /* the step's (0..BL_N) and the ramp's residual (BL_N..) */

/* A band-limited output line: corrections land in a ring around where they happen, the output is BL_H samples late */
struct BlRing {
    float ring[ORING];
    uint32_t rp;
    void clear() { std::memset(this, 0, sizeof *this); }
    /* a step of height J (ramp 0) or a change of slope J per sample (ramp 1), d samples before this one */
    inline void add(const float *tab, float J, float d, int ramp)
    {
        const float *tb = tab + (ramp ? BL_N : 0);
        float di = d * (float)BL_OS;
        int i0 = (int)di;
        if (i0 >= BL_OS)
            i0 = BL_OS - 1;
        if (i0 < 0)
            i0 = 0;
        float fr = di - (float)i0;
        for (int j = -BL_H; j < BL_H; j++) {
            const float *q = tb + (j + BL_H) * BL_OS + i0;
            float b = q[0] + fr * (q[1] - q[0]);
            if (!ramp && j >= 0)
                b -= 1.0f;
            ring[(rp + (uint32_t)j) & (ORING - 1u)] += J * b;
        }
    }
    /* this sample's naive value in, the corrected one (BL_H late) out */
    inline float out(float naive)
    {
        ring[rp & (ORING - 1u)] += naive;
        uint32_t r = (rp - (uint32_t)BL_H) & (ORING - 1u);
        float o = ring[r];
        ring[r] = 0.0f;
        rp++;
        return o;
    }
};


/* ----------------------------------------------------------- the waveforms --- */
/* The Take 5's SHAPE, one knob from the sine through the sawtooth to the pulse (0..1):
 *   0 .. 1/3     the sine into the sawtooth (the Take 5's programs keep the sawtooth at a third: 84 of 254)
 *   1/3 .. 2/3   the sawtooth into the square (at two thirds: 169 of 254)
 *   2/3 .. 1     the pulse narrowing from the square to a needle (at the end it all but vanishes, as the manual says)
 * As pieces: the value at a phase, and its corners (a jump in value) */
struct Wave {
    float wSin = 0, wSaw = 1, wPul = 0, pw = 0.5f, dc = 0;
    void set(float s)
    {
        s = clampf(s, 0.0f, 1.0f);
        wSin = wSaw = wPul = 0.0f;
        pw = 0.5f;
        if (s < 1.0f / 3.0f) {
            float t = s * 3.0f;
            wSin = 1.0f - t;
            wSaw = t;
        } else if (s < 2.0f / 3.0f) {
            float t = (s - 1.0f / 3.0f) * 3.0f;
            wSaw = 1.0f - t;
            wPul = t * 0.9f;                         /* (a square as loud as the saw beside it) */
        } else {
            float t = (s - 2.0f / 3.0f) * 3.0f;
            wPul = 0.9f;
            pw = 0.5f - 0.495f * t * (0.6f + 0.4f * t);
        }
        dc = 2.0f * pw - 1.0f;
    }
    inline float at(float ph) const              /* the value at a phase */
    {
        float y = 0.0f;
        if (wSin > 0.0f)
            y += wSin * sinTurn(ph);
        if (wSaw > 0.0f)
            y += wSaw * (2.0f * ph - 1.0f);
        if (wPul > 0.0f)
            y += wPul * ((ph < pw ? 1.0f : -1.0f) - dc);
        return y;
    }
};

/* An oscillator: its phase, its ring. tick() steps it a sample, a correction for every corner it passed */
struct Osc {
    float ph = 0.0f;
    BlRing r;
    void clear()
    {
        ph = 0.0f;
        r.clear();
    }
    inline void corners(const float *tab, const Wave &w, float a, float b, float inc, float ago)
    {
        if (w.wPul > 0.0f && a < w.pw && b >= w.pw) {
            float d = minf((b - w.pw) / inc + ago, 0.999f);
            r.add(tab, -2.0f * w.wPul, d, 0);
        }
    }
    /* reset >= 0: hard sync, that many samples ago. wrapped (and wd, how long ago): the phase came round */
    inline float tick(const float *tab, const Wave &w, float inc, float reset, int &wrapped, float &wd)
    {
        float old = ph, p = old + inc;
        wrapped = 0;
        if (reset >= 0.0f) {                      /* synced: from where it got to straight back to the start */
            float pr = old + inc * (1.0f - reset);
            if (pr >= 1.0f)
                pr -= 1.0f;
            else
                corners(tab, w, old, pr, inc, reset);
            r.add(tab, w.at(0.0f) - w.at(pr), minf(reset, 0.999f), 0);
            p = inc * reset;
            wrapped = 1;
            wd = reset;
        } else if (p >= 1.0f) {
            p -= 1.0f;
            wrapped = 1;
            wd = minf(p / inc, 0.999f);
            corners(tab, w, old, 1.0f, inc, wd);
            r.add(tab, w.at(0.0f) - w.at(0.999999f), wd, 0);
            corners(tab, w, -1.0f, p, inc, 0.0f);
        } else {
            corners(tab, w, old, p, inc, 0.0f);
        }
        ph = p;
        return r.out(w.at(p));
    }
};

/* ----------------------------------------------------------- the half-band --- */
/* a polyphase IIR half-band (two paths of four first-order allpasses; de Soras' elliptic design: -91 dB from 24.7 kHz
 * at 88.2, flat to 19.4 kHz) */
static const float HB_A0[4] = {0.0472898531f, 0.3385179585f, 0.6552730688f, 0.8745126132f};
static const float HB_A1[4] = {0.1727806110f, 0.5069776792f, 0.7764899192f, 0.9589457184f};
inline float hbAp4(float x, float *t, const float *a)
{
    float y0 = a[0] * (x - t[1]) + t[0], y1 = a[1] * (y0 - t[2]) + t[1], y2 = a[2] * (y1 - t[3]) + t[2],
          y3 = a[3] * (y2 - t[4]) + t[3];
    t[0] = x;
    t[1] = y0;
    t[2] = y1;
    t[3] = y2;
    t[4] = y3;
    return y3;
}
/* a 2x stage: up (one sample in, two out) and down (two in, one out), each through the half-band */
struct HalfBand {
    float u0[5] = {}, u1[5] = {}, d0[5] = {}, d1[5] = {}, dp = 0.0f;
    inline void up(float x, float &a, float &b)
    {
        a = hbAp4(x, u0, HB_A0);
        b = hbAp4(x, u1, HB_A1);
    }
    inline float down(float a, float b)
    {
        float y = 0.5f * (hbAp4(a, d0, HB_A0) + hbAp4(dp, d1, HB_A1));
        dp = b;
        return y;
    }
    void flush()
    {
        for (int i = 0; i < 5; i++) {
            u0[i] = flushDn(u0[i]);
            u1[i] = flushDn(u1[i]);
            d0[i] = flushDn(d0[i]);
            d1[i] = flushDn(d1[i]);
        }
        dp = flushDn(dp);
    }
};
} // namespace f5
