/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (FAKE5) */
#include "PluginProcessor.h"
#include "PanelDialog.h"
#include "PluginEditor.h"
#include "SyxImport.h"

using namespace f5;

/* ------------------------------------------------------------------------------- parameters --- */
juce::AudioProcessorValueTreeState::ParameterLayout Fake5Processor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for (int p = 0; p < NPARAM; p++) {
        const ParamDef &d = PARAMS[p];
        layout.add(std::make_unique<juce::AudioParameterInt>(
            juce::ParameterID{d.id, 1}, d.name, d.lo, d.hi, d.def,
            juce::AudioParameterIntAttributes().withStringFromValueFunction([p](int v, int) {
                char t[32];
                paramText(p, v, t, sizeof t);
                return juce::String(t);
            })));
    }
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"voices", 1}, "Voices",
                                                            juce::StringArray{"5 Voices", "10 Voices"}, 0));
    layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"host_sync", 1}, "Host Sync", true));
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"quality", 1}, "Quality",
                                                            juce::StringArray{"ECO (2x)", "HIGH (4x)"}, 1));
    layout.add(std::make_unique<juce::AudioParameterInt>(juce::ParameterID{"transpose", 1}, "Transpose", -2, 2, 0));
    layout.add(std::make_unique<juce::AudioParameterInt>(juce::ParameterID{"master_coarse", 1}, "Master Coarse", -12, 12, 0));
    layout.add(std::make_unique<juce::AudioParameterInt>(juce::ParameterID{"master_fine", 1}, "Master Fine", -50, 50, 0));
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"volume", 1}, "Volume", juce::NormalisableRange<float>(0.0f, 1.0f), 0.7f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction([](float v, int) {
            return v <= 0.0001f ? juce::String("-inf") : juce::String(20.0f * std::log10(v * v * 2.0f), 1) + " dB";
        })));
    return layout;
}

Fake5Processor::Fake5Processor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "FAKE5", createLayout())
{
    for (int p = 0; p < NPARAM; p++)
        params[p] = apvts.getParameter(PARAMS[p].id);
    loadUserPrograms();
    live = FACTORY[0];
    applyProgram(FACTORY[0], true);
}

int Fake5Processor::value(int p) const { return (int)std::lround(params[p]->convertFrom0to1(params[p]->getValue())); }
void Fake5Processor::setValue(int p, int v)
{
    auto *q = params[p];
    v = juce::jlimit(PARAMS[p].lo, PARAMS[p].hi, v);
    q->beginChangeGesture();
    q->setValueNotifyingHost(q->convertTo0to1((float)v));
    q->endChangeGesture();
}

void Fake5Processor::command(Cmd c, int v)
{
    int s1, n1, s2, n2;
    fifo.prepareToWrite(1, s1, n1, s2, n2);
    if (n1 > 0)
        cmds[(size_t)s1] = {(int)c, v};
    else if (n2 > 0)
        cmds[(size_t)s2] = {(int)c, v};
    fifo.finishedWrite(n1 + n2);
}

/* ------------------------------------------------------------------------------- programs --- */
void Fake5Processor::applyProgram(const Program &p0, bool toHost)
{
    Program p = p0;
    p.p[P_SEQ_PLAY] = 0;                       /* (a new program: the sequencer waits for PLAY) */
    {
        juce::SpinLock::ScopedLockType lock(progLock);
        pending = p;
        live = p;
    }
    pendingSet.store(true);
    if (toHost)
        for (int i = 0; i < NPARAM; i++)
            params[i]->setValueNotifyingHost(params[i]->convertTo0to1((float)p.p[i]));
}

const Program &Fake5Processor::storedProgram() const
{
    return program < NFACTORY ? FACTORY[program] : userPrograms[(size_t)(program - NFACTORY)];
}

void Fake5Processor::setCurrentProgram(int index)
{
    if (index < 0 || index >= getNumPrograms())
        return;
    program = index;
    compare = false;
    applyProgram(storedProgram(), true);
    updateHostDisplay(ChangeDetails().withProgramChanged(true));
}

const juce::String Fake5Processor::getProgramName(int index)
{
    if (index >= 0 && index < NFACTORY)
        return juce::String(FACTORY[index].name);
    if (index >= NFACTORY && index < getNumPrograms())
        return juce::String(userPrograms[(size_t)(index - NFACTORY)].name);
    return {};
}

juce::String Fake5Processor::programName() const
{
    juce::SpinLock::ScopedLockType lock(progLock);
    return juce::String(live.name);
}
void Fake5Processor::setProgramName(const juce::String &n)
{
    juce::SpinLock::ScopedLockType lock(progLock);
    std::snprintf(live.name, NAME_LEN, "%s", n.substring(0, NAME_LEN - 1).toRawUTF8());
}
SeqData Fake5Processor::seqCopy() const
{
    juce::SpinLock::ScopedLockType lock(progLock);
    return live.seq;
}
int Fake5Processor::chordNotes() const
{
    juce::SpinLock::ScopedLockType lock(progLock);
    return chordCount(live);
}

Program Fake5Processor::currentProgram() const
{
    Program p;
    {
        juce::SpinLock::ScopedLockType lock(progLock);
        p = live;
    }
    for (int i = 0; i < NPARAM; i++)
        p.p[i] = (int16_t)value(i);
    p.p[P_SEQ_PLAY] = 0;
    return p;
}

bool Fake5Processor::edited() const
{
    if (program < 0 || program >= f5::NFACTORY + (int)userPrograms.size())
        return false;
    const Program &s = storedProgram();
    for (int i = 0; i < NPARAM; i++)
        if (i != P_SEQ_PLAY && value(i) != s.p[i])
            return true;
    return false;
}

void Fake5Processor::setCompare(bool on)
{
    if (on == compare)
        return;
    if (on) {
        editKept = currentProgram();
        compare = true;
        applyProgram(storedProgram(), true);
    } else {
        compare = false;
        applyProgram(editKept, true);
    }
}

juce::File Fake5Processor::programFolder()
{
    return pdlg::docsOrAppData().getChildFile("FAKE5").getChildFile("Programs");
}

void Fake5Processor::programToXml(const Program &p, juce::XmlElement &x)
{
    x.setAttribute("name", juce::String(p.name));
    x.setAttribute("version", 1);
    for (int i = 0; i < NPARAM; i++)
        x.setAttribute(PARAMS[i].id, (int)p.p[i]);
    juce::MemoryBlock mb(&p.seq, sizeof p.seq);
    x.setAttribute("seq", mb.toBase64Encoding());
    juce::MemoryBlock cb(p.chord, sizeof p.chord);
    x.setAttribute("chord", cb.toBase64Encoding());
}

bool Fake5Processor::programFromXml(const juce::XmlElement &x, Program &p)
{
    programInit(p);
    std::snprintf(p.name, NAME_LEN, "%s", x.getStringAttribute("name", "Program").substring(0, NAME_LEN - 1).toRawUTF8());
    for (int i = 0; i < NPARAM; i++)
        p.p[i] = (int16_t)juce::jlimit(PARAMS[i].lo, PARAMS[i].hi, x.getIntAttribute(PARAMS[i].id, PARAMS[i].def));
    juce::MemoryBlock mb;
    if (mb.fromBase64Encoding(x.getStringAttribute("seq")) && mb.getSize() == sizeof p.seq)
        std::memcpy(&p.seq, mb.getData(), sizeof p.seq);
    juce::MemoryBlock cb;
    if (cb.fromBase64Encoding(x.getStringAttribute("chord")) && cb.getSize() == sizeof p.chord)
        std::memcpy(p.chord, cb.getData(), sizeof p.chord);
    if (p.seq.length > SEQ_STEPS)
        p.seq.length = SEQ_STEPS;
    return true;
}

void Fake5Processor::loadUserPrograms()
{
    userPrograms.clear();
    userFiles.clear();
    auto files = programFolder().findChildFiles(juce::File::findFiles, false, "*.fake5");
    files.sort();
    for (auto &f : files)
        if (auto xml = juce::XmlDocument::parse(f))
            if (xml->hasTagName("FAKE5PROGRAM")) {
                Program p;
                if (programFromXml(*xml, p)) {
                    userPrograms.push_back(p);
                    userFiles.push_back(f);
                }
            }
}

int Fake5Processor::writeProgram(const juce::String &name)
{
    auto clean = name.trim().substring(0, NAME_LEN - 1);
    if (clean.isEmpty())
        return -1;
    Program p = currentProgram();
    std::snprintf(p.name, NAME_LEN, "%s", clean.toRawUTF8());
    juce::XmlElement xml("FAKE5PROGRAM");
    programToXml(p, xml);
    auto folder = programFolder();
    folder.createDirectory();
    if (!xml.writeTo(folder.getChildFile(juce::File::createLegalFileName(clean) + ".fake5")))
        return -1;
    setProgramName(clean);
    loadUserPrograms();
    for (size_t i = 0; i < userPrograms.size(); i++)
        if (juce::String(userPrograms[i].name) == clean) {
            program = NFACTORY + (int)i;
            updateHostDisplay(ChangeDetails().withProgramChanged(true));
            return program;
        }
    return -1;
}

int Fake5Processor::importSyx(const juce::File &syx)
{
    juce::MemoryBlock mb;
    if (!syx.loadFileAsData(mb))
        return -1;
    auto progs = importTake5Syx((const uint8_t *)mb.getData(), mb.getSize());
    if (progs.empty())
        return -1;
    auto folder = programFolder();
    folder.createDirectory();
    auto stem = juce::File::createLegalFileName(syx.getFileNameWithoutExtension()).substring(0, 40);
    int n = 0;
    juce::File first;
    for (auto &p : progs) {
        juce::XmlElement xml("FAKE5PROGRAM");
        programToXml(p, xml);
        auto file = folder.getChildFile(stem + " " + juce::String(n + 1).paddedLeft('0', 3) + " " +
                                        juce::File::createLegalFileName(juce::String(p.name)) + ".fake5");
        if (xml.writeTo(file)) {
            if (!n)
                first = file;
            n++;
        }
    }
    loadUserPrograms();
    importedFirst = -1;
    for (size_t i = 0; i < userFiles.size(); i++)
        if (userFiles[i] == first)
            importedFirst = NFACTORY + (int)i;
    updateHostDisplay(ChangeDetails().withProgramChanged(true));
    return n > 0 ? n : -1;
}

/* ------------------------------------------------------------------------------- the block --- */
bool Fake5Processor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void Fake5Processor::prepareToPlay(double sampleRate, int)
{
    uiMidi.reset(sampleRate);
    engine.prepare(sampleRate);
    Program cur = currentProgram();             /* (it takes the lock itself: not inside it) */
    {
        juce::SpinLock::ScopedLockType lock(progLock);
        pending = cur;
    }
    pendingSet.store(true);
    sentVoices = -1;
    lastGain = -1.0f;
}

void Fake5Processor::clearHeld()
{
    for (bool &h : held)
        h = false;
    nHeld = 0;
}

void Fake5Processor::handleMidi(const juce::MidiMessage &m)
{
    if (m.isNoteOn()) {
        int n = juce::jlimit(0, 127, m.getNoteNumber() + 12 * transpose);
        if (!held[n]) {
            held[n] = true;
            nHeld++;
        }
        engine.noteOn(n, m.getFloatVelocity());
    } else if (m.isNoteOff()) {
        int n = juce::jlimit(0, 127, m.getNoteNumber() + 12 * transpose);
        if (held[n]) {
            held[n] = false;
            nHeld--;
        }
        engine.noteOff(n);
    } else if (m.isPitchWheel())
        engine.bend((float)(m.getPitchWheelValue() - 8192) / 8192.0f);
    else if (m.isChannelPressure())
        engine.pressure((float)m.getChannelPressureValue() / 127.0f);
    else if (m.isAftertouch())
        engine.pressure((float)m.getAfterTouchValue() / 127.0f);
    else if (m.isSustainPedalOn() || m.isSustainPedalOff())
        engine.sustain(m.isSustainPedalOn());
    else if (m.isController()) {
        int cc = m.getControllerNumber();
        float v = (float)m.getControllerValue() / 127.0f;
        switch (cc) {
        case 1: engine.modWheel(v); break;
        case 2: engine.breath(v); break;
        case 4: engine.foot(v); break;
        case 11: engine.expression(v); break;
        case 120: engine.panic(); clearHeld(); break;
        case 123: engine.allOff(); clearHeld(); break;
        case 121:
            engine.bend(0.0f);
            engine.modWheel(0.0f);
            engine.pressure(0.0f);
            engine.sustain(false);
            break;
        default: break;
        }
    } else if (m.isAllNotesOff() || m.isAllSoundOff()) {
        engine.allOff();
        clearHeld();
    }
}

void Fake5Processor::processBlock(juce::AudioBuffer<float> &buffer, juce::MidiBuffer &midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    float *L = buffer.getWritePointer(0), *R = buffer.getWritePointer(buffer.getNumChannels() > 1 ? 1 : 0);

    /* a whole program (a program change, a project): its sequence and chord, then the parameters below */
    if (pendingSet.exchange(false)) {
        Program p;
        {
            juce::SpinLock::ScopedLockType lock(progLock);
            p = pending;
        }
        engine.setProgram(p);
        for (int i = 0; i < NPARAM; i++)
            sent[i] = p.p[i];
    }
    /* the parameters, when they change */
    for (int i = 0; i < NPARAM; i++) {
        int v = (int)std::lround(params[i]->convertFrom0to1(params[i]->getValue()));
        if (v != sent[i]) {
            if (i == P_SEQ_PLAY)
                engine.seqPlay(v != 0);
            engine.setParam(i, v);
            sent[i] = (int16_t)v;
        }
    }
    int voices = apvts.getRawParameterValue("voices")->load() > 0.5f ? 10 : 5;
    if (voices != sentVoices) {
        engine.setVoices(voices);
        sentVoices = voices;
    }
    transpose = (int)std::lround(apvts.getRawParameterValue("transpose")->load());
    engine.setQuality(apvts.getRawParameterValue("quality")->load() > 0.5f ? 1 : 0);
    engine.setTune((int)std::lround(apvts.getRawParameterValue("master_coarse")->load()),
                   (int)std::lround(apvts.getRawParameterValue("master_fine")->load()));

    /* the panel's commands */
    {
        int s1, n1, s2, n2;
        fifo.prepareToRead(fifo.getNumReady(), s1, n1, s2, n2);
        auto run = [&](const Command &c) {
            switch (c.c) {
            case C_RECORD: engine.seqRecord(c.v != 0); break;
            case C_REST: engine.seqRest(); break;
            case C_TIE: engine.seqTie(); break;
            case C_BACK: engine.seqBack(); break;
            case C_HOLD: engine.hold(c.v != 0); break;
            case C_CHORD: engine.storeChord(); break;
            case C_PANIC: engine.panic(); clearHeld(); break;
            default: break;
            }
        };
        for (int i = 0; i < n1; i++)
            run(cmds[(size_t)(s1 + i)]);
        for (int i = 0; i < n2; i++)
            run(cmds[(size_t)(s2 + i)]);
        fifo.finishedRead(n1 + n2);
    }

    /* the host's tempo and position */
    bool sync = apvts.getRawParameterValue("host_sync")->load() > 0.5f;
    double bpm = 120.0, ppq = 0.0;
    bool playing = false;
    if (auto *ph = getPlayHead())
        if (auto pos = ph->getPosition()) {
            if (auto b = pos->getBpm())
                bpm = *b;
            playing = pos->getIsPlaying();
            if (auto q = pos->getPpqPosition())
                ppq = *q;
        }
    engine.host(playing, ppq, bpm, sync);

    uiMidi.removeNextBlockOfMessages(midi, n);
    keyboardState.processNextMidiBuffer(midi, 0, n, true);

    int done = 0;
    for (const auto meta : midi) {
        int at = juce::jlimit(0, n, meta.samplePosition);
        if (at > done) {
            engine.render(L + done, R + done, at - done);
            done = at;
        }
        handleMidi(meta.getMessage());
    }
    if (n > done)
        engine.render(L + done, R + done, n - done);

    /* for the editor */
    for (int v = 0; v < MAXV; v++)
        voiceLevel[v].store(engine.voiceLevel(v), std::memory_order_relaxed);
    seqStepNow.store(engine.seqStep(), std::memory_order_relaxed);
    recStepNow.store(engine.seqRecStep(), std::memory_order_relaxed);
    seqTransNow.store(engine.seqTransposition(), std::memory_order_relaxed);
    recording.store(engine.seqRecording(), std::memory_order_relaxed);
    beatLed.store(engine.clockBeat(), std::memory_order_relaxed);
    lfoNow[0].store(engine.lfoValue(0), std::memory_order_relaxed);
    lfoNow[1].store(engine.lfoValue(1), std::memory_order_relaxed);
    bpmNow.store(engine.bpmNow(), std::memory_order_relaxed);
    holdNow.store(engine.holding(), std::memory_order_relaxed);
    keysHeld.store(nHeld, std::memory_order_relaxed);
    /* what the engine changed itself: the sequence, the chord */
    {
        const Program &ep = engine.program();
        juce::SpinLock::ScopedTryLockType lock(progLock);
        if (lock.isLocked() && !pendingSet.load()) {
            std::memcpy(&live.seq, &ep.seq, sizeof live.seq);
            std::memcpy(live.chord, ep.chord, sizeof live.chord);
        }
    }

    float vol = apvts.getRawParameterValue("volume")->load();
    float gain = vol * vol * 2.0f;
    if (lastGain < 0.0f)
        lastGain = gain;
    buffer.applyGainRamp(0, 0, n, lastGain, gain);
    if (buffer.getNumChannels() > 1)
        buffer.applyGainRamp(1, 0, n, lastGain, gain);
    lastGain = gain;
    meter.store(juce::jmax(buffer.getMagnitude(0, 0, n), meter.load() * 0.8f), std::memory_order_relaxed);
    for (int c = 2; c < buffer.getNumChannels(); c++)
        buffer.clear(c, 0, n);
}

/* ------------------------------------------------------------------------------ state --- */
void Fake5Processor::getStateInformation(juce::MemoryBlock &destData)
{
    auto state = apvts.copyState();
    state.setProperty("program", program, nullptr);
    state.setProperty("uiScale", (double)uiScale.load(), nullptr);
    state.setProperty("uiScaleSet", uiScaleSet.load(), nullptr);
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    auto *pg = xml->createNewChildElement("FAKE5PROGRAM");
    programToXml(currentProgram(), *pg);
    copyXmlToBinary(*xml, destData);
}

void Fake5Processor::setStateInformation(const void *data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (!xml || !xml->hasTagName(apvts.state.getType()))
        return;
    Program p;
    bool hasProg = false;
    if (auto *pg = xml->getChildByName("FAKE5PROGRAM")) {
        hasProg = programFromXml(*pg, p);
        xml->removeChildElement(pg, true);
    }
    auto state = juce::ValueTree::fromXml(*xml);
    program = juce::jlimit(0, juce::jmax(0, getNumPrograms() - 1), (int)state.getProperty("program", 0));
    uiScale = (float)(double)state.getProperty("uiScale", 0.9);
    uiScaleSet = (bool)state.getProperty("uiScaleSet", false);
    apvts.replaceState(state);
    if (hasProg) {
        /* the parameters came with the state; the sequence, chord and name with the program */
        for (int i = 0; i < NPARAM; i++)
            p.p[i] = (int16_t)value(i);
        applyProgram(p, false);
    }
}

juce::AudioProcessorEditor *Fake5Processor::createEditor() { return new Fake5Editor(*this); }

juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() { return new Fake5Processor(); }
