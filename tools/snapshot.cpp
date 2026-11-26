/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (FAKE5) */
/* A picture of the editor (a PNG), without a window: for checking the panel's drawing. First it right-clicks (and
 * right-drags) every control: no parameter may change (a right click is the host's menu, never a value).
 *   Fake5Shot out.png [scale] [program] [display page] [line] */
#include "PluginProcessor.h"
#include "PluginEditor.h"

int main(int argc, char **argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    juce::String out = argc > 1 ? argv[1] : "shot.png";
    float scale = argc > 2 ? (float)std::atof(argv[2]) : 1.0f;
    int prog = argc > 3 ? std::atoi(argv[3]) : 0;
    Fake5Processor proc;
    proc.setCurrentProgram(prog);
    proc.uiScale = 1.0f;
    {
        /* the processor as a host drives it: prepare, blocks with notes, its state out and back in */
        proc.setPlayConfigDetails(0, 2, 48000.0, 512);
        proc.prepareToPlay(48000.0, 512);
        juce::AudioBuffer<float> buf(2, 512);
        float peak = 0.0f;
        for (int b = 0; b < 200; b++) {
            juce::MidiBuffer m;
            if (b == 2)
                for (int n : {48, 55, 60, 64})
                    m.addEvent(juce::MidiMessage::noteOn(1, n, (juce::uint8)100), 10);
            if (b == 190)
                for (int n : {48, 55, 60, 64})
                    m.addEvent(juce::MidiMessage::noteOff(1, n), 0);
            buf.clear();
            proc.processBlock(buf, m);
            peak = juce::jmax(peak, buf.getMagnitude(0, 512));
        }
        juce::MemoryBlock st;
        proc.getStateInformation(st);
        proc.setStateInformation(st.getData(), (int)st.getSize());
        std::printf("processed: peak %.3f, state %d bytes\n", peak, (int)st.getSize());
    }
    std::unique_ptr<juce::AudioProcessorEditor> ed(proc.createEditor());
    ed->setSize(Fake5Panel::W, Fake5Panel::H);
    if (argc > 4)
        if (auto *panel = dynamic_cast<Fake5Panel *>(ed->getChildComponent(0)))
            panel->openPage((Fake5Panel::Page)std::atoi(argv[4]), argc > 5 ? std::atoi(argv[5]) : 0);
    for (auto *c : ed->getChildren())             /* (the live parts: their state, as the timer would) */
        for (auto *k : c->getChildren())
            if (auto *lv = dynamic_cast<Fake5Panel::Live *>(k))
                lv->poll();
    {
        auto values = [&proc] {
            std::vector<float> v;
            for (auto *p : proc.getParameters())
                v.push_back(p->getValue());
            return v;
        };
        auto before = values();
        int n = 0;
        auto right = juce::ModifierKeys(juce::ModifierKeys::rightButtonModifier);
        auto src = juce::Desktop::getInstance().getMainMouseSource();
        for (auto *c : ed->getChildren())
            for (auto *k : c->getChildren()) {
                if (!k->isVisible() || dynamic_cast<Fake5Panel::Live *>(k) == nullptr)
                    continue;
                auto mid = k->getLocalBounds().getCentre().toFloat();
                auto t = juce::Time::getCurrentTime();
                juce::MouseEvent down(src, mid, right, 0.0f, 0, 0, 0, 0, k, k, t, mid, t, 1, false);
                juce::MouseEvent drag(src, mid.translated(0.0f, -80.0f), right, 0.0f, 0, 0, 0, 0, k, k, t, mid, t, 1, true);
                k->mouseDown(down);
                k->mouseDrag(drag);
                k->mouseUp(drag);
                n++;
            }
        auto after = values();
        int changed = 0;
        for (size_t i = 0; i < before.size(); i++)
            if (before[i] != after[i]) {
                std::printf("FAIL: a right click changed %s\n", proc.getParameters()[(int)i]->getName(40).toRawUTF8());
                changed++;
            }
        std::printf("right clicks on %d controls: %d parameters changed\n", n, changed);
        if (changed)
            return 1;
    }
    auto img = ed->createComponentSnapshot(ed->getLocalBounds(), true, scale);
    juce::File f = juce::File::getCurrentWorkingDirectory().getChildFile(out);
    f.deleteFile();
    juce::FileOutputStream os(f);
    juce::PNGImageFormat().writeImageToStream(img, os);
    std::printf("wrote %s (%d x %d)\n", f.getFullPathName().toRawUTF8(), img.getWidth(), img.getHeight());
    ed.reset();
    return 0;
}
