/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (FAKE5) */
/* The editor: the Take 5 desktop module's front panel, drawn at 1840 x 690 and scaled to the window (drag the corner),
 * with the keyboard version's TRANSPOSE and LOW SPLIT, and FAKE5's own VOICES (5 or 10) and HOST SYNC. No keys: played
 * from MIDI (the standalone app: a MIDI keyboard).
 *
 * The OLED works as the Take 5's: its home shows the tempo, the clock divide, the program, the arp's mode and the
 * effect; a control touched shows its value for a moment; PROGRAM, GLOBAL, SRC ASSIGN and DEST ASSIGN open their
 * pages, SELECT picks a line and VALUE sets it. DEST ASSIGN (and an LFO's DEST) armed, the next knob touched becomes
 * the destination, as on the Take 5. With the mouse: the wheel over the display is VALUE, a click picks from a list
 * (on the home page: a program). */
#pragma once
#include "PluginProcessor.h"
#include "ui/Look.h"

class Fake5Panel : public juce::Component, private juce::Timer
{
public:
    static constexpr int W = 1840, H = 690;
    explicit Fake5Panel(Fake5Processor &);
    ~Fake5Panel() override;
    void paint(juce::Graphics &) override;

    Fake5Processor &proc;

    /* what the display shows */
    enum Page { P_HOME, P_PROGRAM, P_GLOBAL, P_MOD, P_LFODEST, P_SEQ };
    struct Entry {
        juce::String label;
        std::function<int()> get;
        std::function<void(int)> set;
        int lo = 0, hi = 127;
        std::function<juce::String(int)> text;
    };
    Page page = P_HOME;
    int item = 0;
    std::vector<Entry> entries() const;
    void openPage(Page p, int at = 0);
    void selectTurn(int d);
    void valueTurn(int d);
    void touch(int p);                           /* a parameter in hand */
    void touchGlobal(juce::RangedAudioParameter *);
    void entryMenu(juce::Rectangle<int> screen);
    void programMenu(juce::Rectangle<int> screen);
    void writeDialog();
    void loadSyx();
    static constexpr int LOAD_ITEM = 100000;
    void message(const juce::String &);          /* a line on the display for a moment */
    juce::String valueText(int p, int v) const;
    int bankOf(int program) const;               /* (banks of 16: factory 1.., your programs 1..) */
    void gotoProgram(int index);

    int lfoSel = 0, modSlot = 0, bank = 0;
    bool armDest = false, armLfoDest = false;    /* DEST ASSIGN / the LFO's DEST: the next knob touched */
    int shownParam = -1;
    juce::RangedAudioParameter *shownGlobal = nullptr;
    double shownAt = -10.0;
    juce::String msg;
    juce::Component *oledView = nullptr;

    struct Live {
        virtual ~Live() = default;
        virtual void poll() = 0;
    };

private:
    void timerCallback() override;
    void build();
    void paintBackground(juce::Graphics &, float sc);

    f5ui::Look look;
    std::vector<std::unique_ptr<juce::Component>> parts;
    std::vector<Live *> live;
    juce::Image background;
    float backgroundScale = 0.0f;
    std::vector<double> taps;
    juce::Component::SafePointer<juce::AlertWindow> dialog;
    std::unique_ptr<juce::FileChooser> chooser;
    template <class T, class... A> T *add(A &&...a);
    friend class Oled;
};

class Fake5Editor : public juce::AudioProcessorEditor
{
public:
    explicit Fake5Editor(Fake5Processor &);
    void resized() override;
    bool sized = false;                      /* (resizes during the constructor keep nothing) */

private:
    Fake5Processor &proc;
    juce::SharedResourcePointer<f5ui::UiResources> resources;   // (before the panel: freed after it)
    Fake5Panel panel;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Fake5Editor)
};
