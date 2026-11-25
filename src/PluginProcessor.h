/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (FAKE5) */
/* FAKE5 as a plugin (VST3, AU on the Mac) and a standalone app: the engine (dsp/Engine.h), its parameters for the host
 * (automation, saved with the project), the factory programs and the user's own as the host's programs, MIDI in
 * (notes, velocity, pressure, pitch bend, mod wheel, breath, foot, expression, sustain), the host's tempo and position
 * for the clock. The panel's buttons that act on the engine (RECORD, HOLD, ...) come through a small queue; what the
 * engine changes itself (a recorded sequence, the chord memory) comes back for the editor and the project. */
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <atomic>
#include "Engine.h"
#include "Presets.h"

class Fake5Processor : public juce::AudioProcessor
{
public:
    Fake5Processor();
    ~Fake5Processor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout &layouts) const override;
    void processBlock(juce::AudioBuffer<float> &, juce::MidiBuffer &) override;

    juce::AudioProcessorEditor *createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return "FAKE5"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 12.0; }

    int getNumPrograms() override { return f5::NFACTORY + (int)userPrograms.size(); }
    int getCurrentProgram() override { return program; }
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int, const juce::String &) override {}
    void getStateInformation(juce::MemoryBlock &destData) override;
    void setStateInformation(const void *data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    juce::RangedAudioParameter *param(int p) const { return params[p]; }
    int value(int p) const;                      /* a parameter now (its integer) */
    void setValue(int p, int v);                 /* (from the editor: a gesture, the host told) */

    /* the panel's commands to the engine (any thread; done at the next block) */
    enum Cmd { C_RECORD, C_REST, C_TIE, C_BACK, C_HOLD, C_CHORD, C_PANIC };
    void command(Cmd c, int v = 0);

    /* what the engine shows (the editor's LEDs and display) */
    std::atomic<float> voiceLevel[f5::MAXV] = {};
    std::atomic<int> seqStepNow{-1}, recStepNow{0}, seqTransNow{0};
    std::atomic<bool> recording{false}, beatLed{false}, holdNow{false};
    std::atomic<float> bpmNow{120.0f}, lfoNow[2] = {}, meter{0.0f};
    std::atomic<int> keysHeld{0};
    juce::MidiKeyboardState keyboardState;
    juce::MidiMessageCollector uiMidi;          /* (the standalone's computer keys: the editor's MIDI) */
    std::atomic<float> uiScale{0.9f};
    std::atomic<bool> uiScaleSet{false};         /* (a size chosen: else the window opens as large as the screen allows) */

    /* the program's name, its sequence and chord memory (the engine's live copy) */
    juce::String programName() const;
    void setProgramName(const juce::String &);
    f5::SeqData seqCopy() const;
    int chordNotes() const;

    /* the user's programs (WRITE): files in Documents/FAKE5/Programs, after the factory ones */
    std::vector<f5::Program> userPrograms;
    static juce::File programFolder();
    void loadUserPrograms();
    int writeProgram(const juce::String &name);
    /* LOAD SYX: a Take 5's .syx into the user's programs (one file each, in the file's order); how many, -1: none */
    int importSyx(const juce::File &syx);
    int importedFirst = -1;                      /* (the first of them, as a program number) */
    std::vector<juce::File> userFiles;
    f5::Program currentProgram() const;          /* the sound as it is now, whole */
    bool compare = false;                        /* COMPARE: the stored program heard, the edit kept aside */
    void setCompare(bool on);
    bool edited() const;                         /* the sound no longer the stored program's */

private:
    f5::Engine engine;
    juce::RangedAudioParameter *params[f5::NPARAM] = {};
    int16_t sent[f5::NPARAM] = {};
    int sentVoices = -1;
    int program = 0;
    float lastGain = -1.0f;
    int transpose = 0;
    bool held[128] = {};                         /* (the keys down: the panel's UNISON stores them as a chord) */
    int nHeld = 0;

    mutable juce::SpinLock progLock;
    f5::Program pending, live;                   /* (live: the engine's sequence and chord, for the editor and saves) */
    std::atomic<bool> pendingSet{false};
    f5::Program editKept;                        /* COMPARE's: the edit, while the stored one plays */

    struct Command {
        int c, v;
    };
    juce::AbstractFifo fifo{256};
    std::array<Command, 256> cmds;

    void handleMidi(const juce::MidiMessage &m);
    void clearHeld();
    void applyProgram(const f5::Program &p, bool toHost);
    const f5::Program &storedProgram() const;
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    static void programToXml(const f5::Program &p, juce::XmlElement &x);
    static bool programFromXml(const juce::XmlElement &x, f5::Program &p);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Fake5Processor)
};
