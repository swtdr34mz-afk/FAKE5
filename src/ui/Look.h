/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (FAKE5) */
/* The look of a Take 5 desktop module: a charcoal powder-coated steel panel with white legends, the synthesis section
 * on raised blue-grey plates, tall black knurled knobs with light grey caps and a white line down the skirt, white
 * scale ticks, small square buttons with a fine grid that glow red from inside, small round red LEDs, the big CUTOFF
 * knob in its orange ring, a red seven-segment display, a blue-white OLED, black anodized end caps and the front lip.
 * The fonts are embedded (Fira Sans, Anonymous Pro: SIL OFL, fonts/). */
#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <vector>

namespace f5ui
{
namespace col
{
const juce::Colour panel{0xff2a2b2e}, plate{0xff545a62}, plateDark{0xff3e434a}, legend{0xfff3f3ef}, dim{0xff9a9c9f},
    line{0xffd9d9d4}, orange{0xffff8a1e}, red{0xffff2a14}, oledBg{0xff02040a}, oled{0xffaee0ff}, oledDim{0xff4d6f88};
}

enum class Face { label, title, logo, mono };

/* the fonts and the tag: they live while an editor is open (a SharedResourcePointer in each) */
struct UiResources
{
    UiResources() = default;
    ~UiResources();
};
juce::Font font(Face f, float height);
float textWidth(const juce::Font &, const juce::String &);

class Look : public juce::LookAndFeel_V4
{
public:
    Look();
    juce::Typeface::Ptr getTypefaceForFont(const juce::Font &) override;
    juce::Font getPopupMenuFont() override { return font(Face::label, 15.0f); }
    void drawPopupMenuBackground(juce::Graphics &, int w, int h) override;
};

enum class KnobKind { pot, bipolar, encoder, big, plain };   /* plain: a pointer, no scale (a switch with its LEDs) */
/* a knob in r (square), seen from above: its scale (a pot's), the black knurled skirt, the light grey cap. The body is
 * drawn once and kept; the line (drawKnobPointer) moves. pos 0..1 (an encoder: turns, any value) */
void drawKnob(juce::Graphics &, juce::Rectangle<float> r, KnobKind kind, bool scale = true);
void drawKnobPointer(juce::Graphics &, juce::Rectangle<float> r, float pos, KnobKind kind);
/* a Take 5 button: a small square key with a fine grid, glowing red from inside when lit */
void drawButton(juce::Graphics &, juce::Rectangle<float> r, bool lit, bool down, bool over);
/* a small round LED */
void drawLed(juce::Graphics &, juce::Point<float> c, float size, float level);
/* the program display: three red seven-segment digits (and their points) */
void drawSevenSeg(juce::Graphics &, juce::Rectangle<float> r, const juce::String &text, const bool dots[3]);
/* a legend (white, the panel's sans) */
void drawLegend(juce::Graphics &, const juce::String &, juce::Rectangle<float>, float size = 12.0f,
                juce::Justification j = juce::Justification::centred, juce::Colour c = col::legend);
/* a wide title, stretched as the Take 5's extended face ("OSCILLATORS", "MIXER") */
void drawWide(juce::Graphics &, const juce::String &, juce::Rectangle<float> r, float height, float stretch = 1.25f,
              juce::Colour c = col::legend, juce::Justification j = juce::Justification::centred);
/* a section: a fine white frame, its name in its top line (the MPC's pages) */
void drawSection(juce::Graphics &, juce::Rectangle<float> r, const juce::String &title);
/* the panel: charcoal steel under a fine powder coat (sc: the pixels a panel unit, for the coat's density) */
void drawPanelFace(juce::Graphics &, juce::Rectangle<float> r, float sc);
/* a raised blue-grey plate of the synthesis section */
void drawPlate(juce::Graphics &, juce::Rectangle<float> r, float radius, float sc);
void drawEndCap(juce::Graphics &, juce::Rectangle<float> r, bool left);
void drawLip(juce::Graphics &, juce::Rectangle<float> r);
/* a few years of use over the print (px: a screen pixel in panel units; played: the knobs worn round most) */
void drawWear(juce::Graphics &, juce::Rectangle<float> r, float px, const std::vector<juce::Point<float>> &played);
void drawScrew(juce::Graphics &, juce::Point<float> c, float r);
void drawTag(juce::Graphics &, juce::Rectangle<float> r, juce::Colour c = col::legend);
/* FAKE 5's name, its E three bars (as the Take 5's) */
void drawLogo(juce::Graphics &, juce::Rectangle<float> r, juce::Colour c = col::legend);
/* the wave symbols the panel prints (0 triangle, 1 saw, 2 reverse saw, 3 square, 4 S&H; 5 sine, 6 pulse) */
void drawWaveIcon(juce::Graphics &, juce::Rectangle<float> r, int shape, float thick = 1.3f);
} // namespace f5ui
