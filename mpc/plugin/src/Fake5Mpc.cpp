// SPDX-License-Identifier: MPL-2.0
// Copyright (C) 2026 swtdr34mz (FAKE5)
//
// FAKE5 for the MPC: the same engine as the VST3 and AU (dsp/: Engine, Params, Presets), hosted by the MPC Plugin SDK's
// framework (mpc_plugin.h). mpc/build.py copies the engine's files beside this one before the SDK builds it.
//
// The sound: 5 or 10 voices, two oscillators with the Take 5's continuous SHAPE, sub, noise (white or pink), SYNC 2>1
// and FM 2>1, the Prophet-5 Rev 4-style 4-pole low-pass with DRIVE (4x HIGH or 2x ECO), two DADSR envelopes with ENV
// ROUTING, two LFOs (global and per voice), sixteen mod slots, VINTAGE, OVERDRIVE, EFFECT 1 (twelve types) and the
// plate REVERB, glide, unison with chord memory, the arpeggiator and the 64-step sequencer; on the MPC's tempo, locked
// to its bar while it plays.
//
// Parameters (their ids never change; new ones only at the end):
//   0           VOLUME (the host nudges parameter 0 on every prepare: harmless here, it ramps)
//   1..137      the program's 137 parameters, in dsp/Params.h's order ("Cutoff", ...)
//   138         VOICES (5 or 10)
//   139         QUALITY (ECO: 2x; HIGH: 4x)
//   140         HOLD
// The program's sequence and chord memory ride in the state (saveExtraState); a program change brings its own.
//
// MIDI in: notes and velocity, pitch bend, channel and poly pressure, CC 1 mod wheel, CC 2 breath, CC 4 foot,
// CC 11 expression, CC 64 sustain, CC 120 all sound off, CC 121 reset controllers, CC 123 all notes off.

#include "mpc_plugin.h"
#include "Engine.h"
#include "Presets.h"
#include "fake5_names.h"

#include <cmath>
#include <cstring>

namespace {
using namespace f5;

enum {
    kVolume = 0,
    kP = 1,
    kVoices = kP + NPARAM,
    kQuality,
    kHold,
    kNumParams
};
static_assert(kNumParams == 141, "the parameters' order is permanent");
static_assert(sizeof(SHORT) / sizeof(SHORT[0]) == NPARAM, "a short name for each parameter");

class Fake5Mpc : public mpc::Plugin
{
public:
    Fake5Mpc() : mpc::Plugin(makeInfo())
    {
        addParam(mpc::floatParam("vol", "Volume", 0.0f, 1.0f, 0.7f, "", 2));
        for (int p = 0; p < NPARAM; p++) {
            const ParamDef &d = PARAMS[p];
            if (d.kind == K_CHOICE || d.kind == K_ONOFF) {
                std::vector<std::string> ch;
                for (int k = d.lo; k <= d.hi; k++)
                    ch.push_back(d.choice[k]);
                addParam(mpc::choiceParam(d.id, SHORT[p], ch, d.def - d.lo));
            } else {
                addParam(mpc::intParam(d.id, SHORT[p], d.lo, d.hi, d.def));
            }
        }
        addParam(mpc::choiceParam("voices", "Voices", {"5 Voices", "10 Voices"}, 0));
        addParam(mpc::choiceParam("quality", "Quality", {"ECO", "HIGH"}, 0));
        addParam(mpc::boolParam("hold", "Hold", false));

        /* the factory programs: VOLUME and the sound; VOICES, QUALITY and HOLD they leave as they are */
        for (int k = 0; k < NFACTORY; k++) {
            std::vector<float> v;
            v.push_back(0.7f);
            for (int p = 0; p < NPARAM; p++) {
                int val = FACTORY[k].p[p];
                if (p == P_SEQ_PLAY)
                    val = 0;                     /* (the sequencer waits for PLAY) */
                const ParamDef &d = PARAMS[p];
                if (d.kind == K_CHOICE || d.kind == K_ONOFF)
                    val -= d.lo;
                v.push_back((float)val);
            }
            addProgram(FACTORY[k].name, v);
        }
        engine.prepare(44100.0);         // (the MPC's rate: made here, on the UI thread, never in process)
        rate = 44100.0;
        cur = FACTORY[0];
    }

    void prepare(double sampleRate, int) override
    {
        if (sampleRate != rate) {        // (only if the rate really changes: the MPC's is fixed at 44.1 kHz)
            engine.prepare(sampleRate);
            rate = sampleRate;
        }
        forceSend();
    }

    /* resume, or a program / state loaded (on the audio thread): the voices let go; the program's sequence and chord
     * (a state's own, else the factory program's when the program changed) */
    void reset() override
    {
        engine.panic();
        if (stagedReady.exchange(false)) {
            std::memcpy(&cur.seq, &staged.seq, sizeof cur.seq);
            std::memcpy(cur.chord, staged.chord, sizeof cur.chord);
            std::memcpy(cur.name, staged.name, sizeof cur.name);
            lastProgram = currentProgram();
        } else if (currentProgram() != lastProgram) {
            int k = currentProgram();
            if (k >= 0 && k < NFACTORY) {
                std::memcpy(&cur.seq, &FACTORY[k].seq, sizeof cur.seq);
                std::memcpy(cur.chord, FACTORY[k].chord, sizeof cur.chord);
                std::memcpy(cur.name, FACTORY[k].name, sizeof cur.name);
            }
            lastProgram = k;
        }
        forceSend();
    }

    void process(mpc::ProcessContext &ctx) override
    {
        const int n = ctx.numFrames;
        float *L = ctx.outputs[0], *R = ctx.numOutputs > 1 ? ctx.outputs[1] : ctx.outputs[0];
        sync();
        /* the MPC's tempo, and its position while it plays */
        double bpm = ctx.transport.valid && ctx.transport.tempoValid ? ctx.transport.bpm : 120.0;
        bool playing = ctx.transport.valid && ctx.transport.playing && ctx.transport.ppqValid;
        engine.host(playing, playing ? ctx.transport.ppqPos : 0.0, bpm, true);
        int done = 0;
        for (int i = 0; i < ctx.numMidiEvents; i++) {
            const mpc::MidiEvent &e = ctx.midi[i];
            int at = mpc::clamp(e.sampleOffset, 0, n);
            if (at > done) {
                engine.render(L + done, R + done, at - done);
                done = at;
            }
            midi(e);
        }
        if (n > done)
            engine.render(L + done, R + done, n - done);
        /* VOLUME, a ramp over the block (0.7: about 0 dB) */
        float v = (float)get(kVolume), g = v * v * 2.0f;
        if (lastGain < 0.0f)
            lastGain = g;
        float step = (g - lastGain) / (float)(n > 0 ? n : 1), gg = lastGain;
        for (int s = 0; s < n; s++) {
            gg += step;
            L[s] *= gg;
            if (R != L)
                R[s] *= gg;
        }
        lastGain = g;
    }

    bool formatValue(int index, float plain, std::string &out) override
    {
        int v = (int)std::lround(plain);
        char t[40];
        if (index == kVolume) {
            if (plain <= 0.0001f)
                out = "-inf dB";
            else {
                std::snprintf(t, sizeof t, "%.1f dB", 20.0 * std::log10(plain * plain * 2.0));
                out = t;
            }
            return true;
        }
        if (index >= kP && index < kVoices) {
            int p = index - kP;
            const ParamDef &d = PARAMS[p];
            if (d.kind == K_CHOICE || d.kind == K_ONOFF)
                v += d.lo;               /* (a choice's plain value is its index from 0) */
            v = mpc::clamp(v, d.lo, d.hi);
            bool synced = false;
            if (p == P_LFO1 + LFO_FREQ || p == P_LFO1 + LFO_N + LFO_FREQ)
                synced = paramValue(p - LFO_FREQ + LFO_SYNC) != 0;
            if (p == P_FX_TIME)
                synced = paramValue(P_FX_SYNC) != 0;
            paramText(p, v, t, sizeof t, synced, paramValue(P_FX_TYPE));
            out = t;
            if (out.size() > 12)
                out.resize(12);
            return true;
        }
        return false;
    }

    /* the sequence, the chord and the name, in the state */
    void saveExtraState(std::vector<uint8_t> &out) override
    {
        const uint8_t magic[4] = {'F', '5', 'S', '1'};
        out.insert(out.end(), magic, magic + 4);
        auto put = [&](const void *p, size_t n) { out.insert(out.end(), (const uint8_t *)p, (const uint8_t *)p + n); };
        put(&cur.seq, sizeof cur.seq);
        put(cur.chord, sizeof cur.chord);
        put(cur.name, sizeof cur.name);
    }
    void loadExtraState(const uint8_t *data, size_t size) override
    {
        if (size != 4 + sizeof cur.seq + sizeof cur.chord + sizeof cur.name || std::memcmp(data, "F5S1", 4) != 0)
            return;
        const uint8_t *p = data + 4;
        std::memcpy(&staged.seq, p, sizeof staged.seq);
        p += sizeof staged.seq;
        std::memcpy(staged.chord, p, sizeof staged.chord);
        p += sizeof staged.chord;
        std::memcpy(staged.name, p, sizeof staged.name);
        staged.name[NAME_LEN - 1] = 0;
        if (staged.seq.length > SEQ_STEPS)
            staged.seq.length = SEQ_STEPS;
        stagedReady.store(true);         /* (taken in on the audio thread: reset()) */
    }

private:
    static mpc::PluginInfo makeInfo()
    {
        mpc::PluginInfo i;
        i.name = "FAKE5";
        i.vendor = "swtdr34mz";
        i.uniqueId = MPC_FOURCC('F', 'a', 'k', '5');
        i.version = 1000;
        i.isInstrument = true;
        i.numInputs = 0;
        i.numOutputs = 2;
        i.tailSamples = 44100 * 12;
        return i;
    }

    void forceSend()
    {
        programSent = false;
        lastGain = -1.0f;
    }

    int paramValue(int p) const
    {
        const ParamDef &d = PARAMS[p];
        int v = getInt(kP + p);
        if (d.kind == K_CHOICE || d.kind == K_ONOFF)
            v += d.lo;
        return mpc::clamp(v, d.lo, d.hi);
    }

    /* the parameters to the engine, when they change (atomics: safe here) */
    void sync()
    {
        if (!programSent) {              /* (a whole program: the sequence and chord with the parameters) */
            for (int p = 0; p < NPARAM; p++)
                cur.p[p] = (int16_t)paramValue(p);
            engine.setProgram(cur);
            for (int p = 0; p < NPARAM; p++)
                sent[p] = engine.program().p[p];
            programSent = true;
        }
        for (int p = 0; p < NPARAM; p++) {
            int v = paramValue(p);
            if (v != sent[p]) {
                if (p == P_SEQ_PLAY)
                    engine.seqPlay(v != 0);
                engine.setParam(p, v);
                sent[p] = (int16_t)v;
                cur.p[p] = (int16_t)v;
            }
        }
        engine.setVoices(getInt(kVoices) ? 10 : 5);
        engine.setQuality(getInt(kQuality));
        bool hold = getBool(kHold);
        if (hold != holdSent) {
            engine.hold(hold);
            holdSent = hold;
        }
    }

    void midi(const mpc::MidiEvent &e)
    {
        const uint8_t st = e.status();
        if (e.isNoteOn())
            engine.noteOn(e.note(), (float)e.velocity() / 127.0f);
        else if (e.isNoteOff())
            engine.noteOff(e.note());
        else if (e.isPitchBend())
            engine.bend((float)e.pitchBend() / 8192.0f);
        else if (st == 0xA0)             /* poly pressure: as the channel's */
            engine.pressure((float)e.data[2] / 127.0f);
        else if (e.isAftertouch())
            engine.pressure((float)e.data[1] / 127.0f);
        else if (e.isController()) {
            int cc = e.controller();
            float v = (float)e.controllerValue() / 127.0f;
            switch (cc) {
            case 1: engine.modWheel(v); break;
            case 2: engine.breath(v); break;
            case 4: engine.foot(v); break;
            case 11: engine.expression(v); break;
            case 64: engine.sustain(e.controllerValue() >= 64); break;
            case 120: engine.panic(); break;
            case 123: engine.allOff(); break;
            case 121:
                engine.bend(0.0f);
                engine.modWheel(0.0f);
                engine.pressure(0.0f);
                engine.sustain(false);
                break;
            default: break;
            }
        }
    }

    Engine engine;
    double rate = 0.0;
    Program cur, staged;
    std::atomic<bool> stagedReady{false};
    int lastProgram = 0;
    int16_t sent[NPARAM] = {};
    bool programSent = false, holdSent = false;
    float lastGain = -1.0f;
};

} // namespace

MPC_PLUGIN_ENTRY(Fake5Mpc)
