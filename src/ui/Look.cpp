/* SPDX-License-Identifier: MPL-2.0
 * Copyright (C) 2026 swtdr34mz (FAKE5) */
#include "Look.h"
#include "BinaryData.h"
#include <map>

namespace f5ui
{
/* the fonts and the tag: plain pointers, made when first drawn and freed when the last editor closes. Never function
 * statics: those are freed while the host unloads the DLL, under Windows' loader lock (PROPHET 67's lesson) */
struct Store
{
    juce::Typeface::Ptr t[4];
    juce::Image tag;
    std::map<int, juce::Image> coat[2];  /* the powder coat's tile, per pixel density: the panel's, the plates' */
};
static Store *store = nullptr;
static Store &get()
{
    if (store == nullptr)
    {
        store = new Store();
        store->t[0] = juce::Typeface::createSystemTypefaceFor(BinaryData::FiraSansSemiBold_ttf, BinaryData::FiraSansSemiBold_ttfSize);
        store->t[1] = juce::Typeface::createSystemTypefaceFor(BinaryData::FiraSansBold_ttf, BinaryData::FiraSansBold_ttfSize);
        store->t[2] = juce::Typeface::createSystemTypefaceFor(BinaryData::FiraSansExtraBold_ttf, BinaryData::FiraSansExtraBold_ttfSize);
        store->t[3] = juce::Typeface::createSystemTypefaceFor(BinaryData::AnonymousProBold_ttf, BinaryData::AnonymousProBold_ttfSize);
        store->tag = juce::ImageFileFormat::loadFrom(BinaryData::swtdr34mztag_png, BinaryData::swtdr34mztag_pngSize);
    }
    return *store;
}
UiResources::~UiResources()
{
    /* (the last editor closed: the fonts, the coat and the tag let go here, on the message thread, never when the
     * host unloads the plugin: on Windows that runs under the loader's lock) */
    delete store;
    store = nullptr;
    juce::Typeface::clearTypefaceCache();
    juce::ImageCache::releaseUnusedImages();
}

juce::Font font(Face f, float height) { return juce::Font(juce::FontOptions(get().t[(int)f]).withHeight(height)); }

float textWidth(const juce::Font &f, const juce::String &t)
{
    juce::GlyphArrangement ga;
    ga.addLineOfText(f, t, 0.0f, 0.0f);
    return ga.getBoundingBox(0, -1, true).getWidth();
}

Look::Look()
{
    using namespace juce;
    setColour(ResizableWindow::backgroundColourId, col::panel);
    setColour(PopupMenu::backgroundColourId, Colour(0xff1f2023));
    setColour(PopupMenu::textColourId, col::legend);
    setColour(PopupMenu::highlightedBackgroundColourId, Colour(0xffc0281a));
    setColour(PopupMenu::highlightedTextColourId, Colours::white);
    setColour(PopupMenu::headerTextColourId, col::orange);
    setColour(AlertWindow::backgroundColourId, Colour(0xff1f2023));
    setColour(AlertWindow::textColourId, col::legend);
    setColour(TextEditor::backgroundColourId, Colour(0xff0b0b0c));
    setColour(TextEditor::textColourId, col::legend);
    setColour(TextButton::buttonColourId, Colour(0xff2d2e31));
}

juce::Typeface::Ptr Look::getTypefaceForFont(const juce::Font &) { return get().t[0]; }

void Look::drawPopupMenuBackground(juce::Graphics &g, int w, int h)
{
    g.fillAll(findColour(juce::PopupMenu::backgroundColourId));
    g.setColour(col::orange.withAlpha(0.7f));
    g.drawRect(0, 0, w, h, 1);
}

/* ------------------------------------------------------------------------------- the knob --- */
static constexpr float A0 = -2.62f, A1 = 2.62f;     /* a pot's travel: 300 degrees */

void drawKnob(juce::Graphics &g, juce::Rectangle<float> r, KnobKind kind, bool scale)
{
    using namespace juce;
    const float d = jmin(r.getWidth(), r.getHeight()), R = d * 0.5f;
    const Point<float> c = r.getCentre();
    const bool big = kind == KnobKind::big;
    const float rb = R * (big ? 0.66f : 0.70f);       /* the skirt's foot */
    const float ro = rb * (big ? 0.58f : 0.64f);      /* the chrome ring */
    const float rc = ro * 0.86f;                       /* the cap */

    if (big) {                                         /* CUTOFF's orange ring: a band and its rays */
        g.setColour(col::orange);
        Path ring;
        ring.addCentredArc(c.x, c.y, R * 0.80f, R * 0.80f, 0.0f, A0 - 0.06f, A1 + 0.06f, true);
        g.strokePath(ring, PathStrokeType(R * 0.07f, PathStrokeType::curved, PathStrokeType::butt));
        for (int i = 0; i <= 10; i++) {
            float a = A0 + (A1 - A0) * (float)i / 10.0f;
            g.drawLine(Line<float>(c.getPointOnCircumference(R * 0.80f, a), c.getPointOnCircumference(R * (i % 5 == 0 ? 0.99f : 0.93f), a)),
                       jmax(1.4f, d * 0.022f));
        }
    } else if (scale && kind != KnobKind::encoder && kind != KnobKind::plain) {   /* the white scale */
        g.setColour(col::legend);
        for (int i = 0; i <= 10; i++) {
            float a = A0 + (A1 - A0) * (float)i / 10.0f;
            bool mid = kind == KnobKind::bipolar && i == 5;
            g.drawLine(Line<float>(c.getPointOnCircumference(R * 0.86f, a), c.getPointOnCircumference(R * (mid ? 1.0f : 0.97f), a)),
                       jmax(1.0f, d * (mid ? 0.034f : 0.026f)));
        }
    }
    /* its shadow on the panel (the light from the upper left): soft, several layers */
    for (int i = 0; i < 6; i++) {
        float s = rb * (1.04f + 0.09f * (float)i);
        g.setColour(Colours::black.withAlpha(0.13f - 0.018f * (float)i));
        g.fillEllipse(Rectangle<float>(s * 2.0f, s * 2.0f).withCentre(c.translated(rb * (0.06f + 0.035f * (float)i), rb * (0.12f + 0.06f * (float)i))));
    }
    /* the skirt: black, knurled, its ribs catching the light toward the upper left */
    auto foot = Rectangle<float>(rb * 2.0f, rb * 2.0f).withCentre(c);
    g.setColour(Colour(0xff040405));
    g.fillEllipse(foot);
    {
        ColourGradient cg(Colour(0xff26272a), c.x - rb * 0.7f, c.y - rb * 0.8f, Colour(0xff060607), c.x + rb * 0.6f, c.y + rb * 0.8f, false);
        g.setGradientFill(cg);
        g.fillEllipse(foot.reduced(rb * 0.03f));
    }
    const int ribs = big ? 64 : 44;
    for (int i = 0; i < ribs; i++) {
        float a = MathConstants<float>::twoPi * (float)i / (float)ribs;
        float lit = 0.5f + 0.5f * std::cos(a + 2.35f);
        auto q0 = c.getPointOnCircumference(ro * 1.04f, a), q1 = c.getPointOnCircumference(rb * 0.985f, a);
        g.setColour(Colours::black.withAlpha(0.55f));
        g.drawLine(Line<float>(q0, q1), jmax(0.9f, d * 0.016f));
        auto off = c.getPointOnCircumference(jmax(0.5f, d * 0.006f), a + 1.5708f) - c;
        g.setColour(Colours::white.withAlpha(0.03f + 0.17f * lit * lit));
        g.drawLine(Line<float>(q0 + off, q1 + off), jmax(0.6f, d * 0.009f));
    }
    {   /* the skirt's top edge: a bevel, light above, dark below */
        Path hi;
        hi.addCentredArc(c.x, c.y, rb * 0.985f, rb * 0.985f, 0.0f, -2.6f, -0.3f, true);
        g.setColour(Colours::white.withAlpha(0.16f));
        g.strokePath(hi, PathStrokeType(jmax(0.7f, d * 0.012f)));
    }
    g.setColour(Colours::black.withAlpha(0.8f));
    g.drawEllipse(foot, jmax(0.8f, d * 0.01f));

    /* the chrome ring under the cap: turned metal, two bright sides */
    const int slices = 96;
    for (int i = 0; i < slices; i++) {
        float a = MathConstants<float>::twoPi * (float)i / (float)slices, a2 = a + MathConstants<float>::twoPi / (float)slices + 0.012f;
        float k = 0.5f + 0.5f * std::cos(2.0f * (a + 0.75f));
        float v = 0.34f + 0.60f * std::pow(k, 1.7f);
        Path p;
        p.addCentredArc(c.x, c.y, ro, ro, 0.0f, a, a2, true);
        p.lineTo(c.getPointOnCircumference(rc, a2));
        p.addCentredArc(c.x, c.y, rc, rc, 0.0f, a2, a, false);
        p.closeSubPath();
        g.setColour(Colour::fromFloatRGBA(v, v, v * 1.025f, 1.0f));
        g.fillPath(p);
    }
    g.setColour(Colours::black.withAlpha(0.65f));
    g.drawEllipse(Rectangle<float>(ro * 2.0f, ro * 2.0f).withCentre(c), jmax(0.7f, d * 0.008f));

    /* the cap: spun aluminium, grey, light sweeping round it as on a turned top, a gentle dome over it */
    auto cap = Rectangle<float>(rc * 2.0f, rc * 2.0f).withCentre(c);
    for (int i = 0; i < slices; i++) {
        float a = MathConstants<float>::twoPi * (float)i / (float)slices, a2 = a + MathConstants<float>::twoPi / (float)slices + 0.012f;
        float k = 0.5f + 0.5f * std::cos(2.0f * (a + 0.75f));
        float v = 0.50f + 0.30f * std::pow(k, 2.2f) + 0.015f * std::sin(a * 7.0f);
        Path p;
        p.addPieSegment(cap, a, a2, 0.0f);
        g.setColour(Colour::fromFloatRGBA(v, v, v * 1.02f, 1.0f));
        g.fillPath(p);
    }
    {
        ColourGradient dome(Colours::white.withAlpha(0.22f), c.x - rc * 0.45f, c.y - rc * 0.55f, Colours::black.withAlpha(0.28f), c.x + rc * 0.8f, c.y + rc * 0.9f, true);
        dome.addColour(0.5, Colours::transparentBlack);
        g.setGradientFill(dome);
        g.fillEllipse(cap);
    }
    g.setColour(Colours::black.withAlpha(0.05f));      /* the fine rings of the lathe */
    for (int i = 1; i < 9; i++)
        g.drawEllipse(cap.reduced(rc * 0.1f * (float)i), 0.5f);
    g.setColour(Colours::white.withAlpha(0.6f));       /* its rim lit at the upper left */
    {
        Path hl;
        hl.addCentredArc(c.x, c.y, rc * 0.95f, rc * 0.95f, 0.0f, -2.3f, -0.6f, true);
        g.strokePath(hl, PathStrokeType(jmax(0.7f, rc * 0.05f)));
    }
    g.setColour(Colours::black.withAlpha(0.7f));
    g.drawEllipse(cap, jmax(0.7f, rc * 0.035f));
    g.setColour(Colours::white.withAlpha(0.5f));       /* a small sheen on the dome */
    g.fillEllipse(Rectangle<float>(rc * 0.34f, rc * 0.2f).withCentre(c.translated(-rc * 0.38f, -rc * 0.46f)));
}

void drawKnobPointer(juce::Graphics &g, juce::Rectangle<float> r, float pos, KnobKind kind)
{
    using namespace juce;
    if (kind == KnobKind::encoder)
        return;                                        /* (an encoder has no line: it turns for ever) */
    const float d = jmin(r.getWidth(), r.getHeight()), R = d * 0.5f;
    const bool big = kind == KnobKind::big;
    const float rb = R * (big ? 0.66f : 0.70f), ro = rb * (big ? 0.58f : 0.64f), rc = ro * 0.86f;
    const Point<float> c = r.getCentre();
    const float ang = A0 + jlimit(0.0f, 1.0f, pos) * (A1 - A0);
    /* the white line on the skirt, set into it */
    auto p0 = c.getPointOnCircumference(ro * 1.06f, ang), p1 = c.getPointOnCircumference(rb * 0.95f, ang);
    g.setColour(Colours::black.withAlpha(0.6f));
    g.drawLine(Line<float>(p0, p1).withShortenedStart(-0.5f), jmax(2.6f, d * (big ? 0.052f : 0.074f)));
    g.setColour(Colour(0xfff6f5f0));
    g.drawLine({p0, p1}, jmax(1.6f, d * (big ? 0.034f : 0.05f)));
    /* and its groove across the cap */
    auto q0 = c.getPointOnCircumference(rc * 0.30f, ang), q1 = c.getPointOnCircumference(rc * 0.90f, ang);
    g.setColour(Colours::white.withAlpha(0.35f));
    g.drawLine(Line<float>(q0.translated(0.0f, 0.7f), q1.translated(0.0f, 0.7f)), jmax(1.0f, d * 0.022f));
    g.setColour(Colour(0xff1b1c1e).withAlpha(0.85f));
    g.drawLine({q0, q1}, jmax(1.0f, d * 0.022f));
}

/* ---------------------------------------------------------------------------- the button --- */
/* a Take 5 key: soft-touch rubber in a hole in the panel; lit, red light glows through it */
void drawButton(juce::Graphics &g, juce::Rectangle<float> r, bool lit, bool down, bool over)
{
    using namespace juce;
    const float rad = jmin(r.getWidth(), r.getHeight()) * 0.12f + 1.5f;
    if (lit) {                                         /* its light on the panel round it */
        g.setGradientFill(ColourGradient(Colour(0xffff2a10).withAlpha(0.40f), r.getCentreX(), r.getCentreY(), Colours::transparentBlack,
                                         r.getRight() + r.getWidth() * 0.55f, r.getCentreY(), true));
        g.fillEllipse(r.expanded(r.getWidth() * 0.6f, r.getHeight() * 0.8f));
    }
    /* the hole: a dark gap, its lower edge catching the light */
    auto hole = r.expanded(2.2f);
    g.setColour(Colours::white.withAlpha(0.07f));
    g.fillRoundedRectangle(hole.translated(0.0f, 1.0f), rad + 2.0f);
    g.setColour(Colour(0xff030303));
    g.fillRoundedRectangle(hole, rad + 2.0f);
    /* the key: raised, its sides below its face */
    auto key = r.translated(0.0f, down ? 0.9f : 0.0f);
    auto face = key.withTrimmedBottom(down ? 1.2f : 2.4f).reduced(0.6f, 0.0f);
    g.setGradientFill(ColourGradient(lit ? Colour(0xff9a1408) : Colour(0xff1c1d20), key.getX(), key.getY(),
                                     lit ? Colour(0xff4a0804) : Colour(0xff0b0b0c), key.getX(), key.getBottom(), false));
    g.fillRoundedRectangle(key, rad);
    if (lit) {
        ColourGradient cg(Colour(0xffffc29a), face.getCentreX(), face.getCentreY() - face.getHeight() * 0.05f,
                          Colour(0xffb0120a), face.getRight() + face.getWidth() * 0.05f, face.getBottom(), true);
        cg.addColour(0.30, Colour(0xffff6a3c));
        cg.addColour(0.62, Colour(0xffec2a12));
        g.setGradientFill(cg);
    } else {
        ColourGradient cg(Colour(0xff45474c), face.getX(), face.getY(), Colour(0xff26272b), face.getX(), face.getBottom(), false);
        cg.addColour(0.15, Colour(0xff3a3c40));
        g.setGradientFill(cg);
    }
    g.fillRoundedRectangle(face, rad);
    {   /* the soft-touch surface: a fine, irregular grain */
        Graphics::ScopedSaveState ss(g);
        Path clip;
        clip.addRoundedRectangle(face, rad);
        g.reduceClipRegion(clip);
        Random rnd(1234);
        int n = (int)(face.getWidth() * face.getHeight() * 0.35f);
        for (int i = 0; i < n; i++) {
            float x = face.getX() + rnd.nextFloat() * face.getWidth(), y = face.getY() + rnd.nextFloat() * face.getHeight();
            bool light = rnd.nextBool();
            g.setColour(light ? Colours::white.withAlpha(lit ? 0.10f : 0.045f) : Colours::black.withAlpha(lit ? 0.10f : 0.16f));
            g.fillRect(x, y, 0.7f, 0.7f);
        }
        /* a slight hollow in the face: darker toward its middle, as a concave key */
        ColourGradient dish(Colours::transparentBlack, face.getCentreX(), face.getY(), Colours::black.withAlpha(lit ? 0.0f : 0.10f),
                            face.getCentreX(), face.getCentreY(), false);
        dish.addColour(1.0, Colours::transparentBlack);
        g.setGradientFill(dish);
        g.fillRect(face);
    }
    /* the bevel: the top and left edges lit, a soft sheen along the top */
    g.setColour(Colours::white.withAlpha(lit ? 0.45f : 0.20f));
    g.drawLine(face.getX() + rad, face.getY() + 0.7f, face.getRight() - rad, face.getY() + 0.7f, 1.0f);
    g.setColour(Colours::white.withAlpha(lit ? 0.18f : 0.07f));
    g.drawLine(face.getX() + 0.8f, face.getY() + rad, face.getX() + 0.8f, face.getBottom() - rad, 0.8f);
    g.setGradientFill(ColourGradient(Colours::white.withAlpha(lit ? 0.22f : 0.08f), face.getX(), face.getY(),
                                     Colours::transparentWhite, face.getX(), face.getY() + face.getHeight() * 0.4f, false));
    g.fillRoundedRectangle(face.reduced(1.2f).withHeight(face.getHeight() * 0.4f), rad * 0.8f);
    g.setColour(Colours::black.withAlpha(0.55f));
    g.drawRoundedRectangle(key, rad, 0.8f);
    if (over && !down) {
        g.setColour(Colours::white.withAlpha(0.06f));
        g.fillRoundedRectangle(face, rad);
    }
}

void drawLed(juce::Graphics &g, juce::Point<float> c, float s, float level)
{
    using namespace juce;
    level = jlimit(0.0f, 1.0f, level);
    const float rr = s * 0.5f;
    if (level > 0.02f) {
        g.setGradientFill(ColourGradient(Colour(0xffff3a1a).withAlpha(0.62f * level), c.x, c.y, Colours::transparentBlack, c.x + s * 2.4f, c.y, true));
        g.fillEllipse(Rectangle<float>(s * 4.8f, s * 4.8f).withCentre(c));
    }
    g.setColour(Colours::black.withAlpha(0.75f));
    g.fillEllipse(Rectangle<float>(rr * 2.5f, rr * 2.5f).withCentre(c.translated(0.2f, 0.5f)));
    ColourGradient cg(Colour(0xff5a1712).interpolatedWith(Colour(0xffffd0a8), level), c.x - rr * 0.3f, c.y - rr * 0.35f,
                      Colour(0xff250806).interpolatedWith(Colour(0xffe8200c), level), c.x + rr, c.y + rr, true);
    g.setGradientFill(cg);
    g.fillEllipse(Rectangle<float>(rr * 2.0f, rr * 2.0f).withCentre(c));
    g.setColour(Colours::white.withAlpha(0.35f + 0.3f * level));
    g.fillEllipse(Rectangle<float>(rr * 0.6f, rr * 0.45f).withCentre(c.translated(-rr * 0.32f, -rr * 0.38f)));
}

/* ------------------------------------------------------------------------- seven segments --- */
void drawSevenSeg(juce::Graphics &g, juce::Rectangle<float> r, const juce::String &text, const bool dots[3])
{
    using namespace juce;
    g.setColour(Colour(0xff0a0303));
    g.fillRoundedRectangle(r, 2.5f);
    g.setGradientFill(ColourGradient(Colour(0x30ff2010), r.getCentreX(), r.getCentreY(), Colour(0x00ff2010), r.getRight(), r.getCentreY(), true));
    g.fillRoundedRectangle(r, 2.5f);
    g.setColour(Colours::black);
    g.drawRoundedRectangle(r, 2.5f, 1.0f);
    /* a..g of each digit; the digits drawn slanted a little */
    static const uint8_t SEG[16] = {0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f, 0x77, 0x7c, 0x39, 0x5e, 0x79, 0x71};
    auto in = r.reduced(r.getWidth() * 0.08f, r.getHeight() * 0.16f);
    float dw = in.getWidth() / 3.0f, h = in.getHeight(), w = dw * 0.62f, t = jmax(1.6f, h * 0.11f);
    for (int k = 0; k < 3; k++) {
        juce_wchar ch = k < text.length() ? text[k] : ' ';
        uint8_t m = 0;
        if (ch >= '0' && ch <= '9')
            m = SEG[ch - '0'];
        else if (ch == 'U' || ch == 'u')
            m = 0x3e;
        else if (ch == 'F')
            m = 0x71;
        else if (ch == 'P')
            m = 0x73;
        else if (ch == '-')
            m = 0x40;
        else if (ch == 'b')
            m = 0x7c;
        float x0 = in.getX() + dw * (float)k + (dw - w) * 0.3f, y0 = in.getY();
        auto seg = [&](int i, float ax, float ay, float bx, float by) {
            bool on = (m >> i) & 1;
            Path p;
            float sl = 0.08f;
            Point<float> a(x0 + ax * w - ay * h * sl + h * sl, y0 + ay * h), b(x0 + bx * w - by * h * sl + h * sl, y0 + by * h);
            p.startNewSubPath(a);
            p.lineTo(b);
            g.setColour(on ? Colour(0xffff3018) : Colour(0xff2a0805));
            g.strokePath(p, PathStrokeType(t, PathStrokeType::mitered, PathStrokeType::rounded));
            if (on) {
                g.setColour(Colour(0x60ff6040));
                g.strokePath(p, PathStrokeType(t * 2.2f, PathStrokeType::mitered, PathStrokeType::rounded));
                g.setColour(Colour(0xffff6a48));
                g.strokePath(p, PathStrokeType(t * 0.45f, PathStrokeType::mitered, PathStrokeType::rounded));
            }
        };
        const float e = 0.1f;
        seg(0, e, 0.0f, 1.0f - e, 0.0f);
        seg(1, 1.0f, e, 1.0f, 0.5f - e * 0.6f);
        seg(2, 1.0f, 0.5f + e * 0.6f, 1.0f, 1.0f - e);
        seg(3, e, 1.0f, 1.0f - e, 1.0f);
        seg(4, 0.0f, 0.5f + e * 0.6f, 0.0f, 1.0f - e);
        seg(5, 0.0f, e, 0.0f, 0.5f - e * 0.6f);
        seg(6, e, 0.5f, 1.0f - e, 0.5f);
        bool dt = dots && dots[k];
        g.setColour(dt ? Colour(0xffff3a20) : Colour(0xff2a0805));
        g.fillEllipse(Rectangle<float>(t * 1.15f, t * 1.15f).withCentre({x0 + w + t * 1.4f, y0 + h}));
    }
}

/* ------------------------------------------------------------------------------- legends --- */
void drawLegend(juce::Graphics &g, const juce::String &t, juce::Rectangle<float> r, float size, juce::Justification j, juce::Colour c)
{
    g.setColour(c);
    g.setFont(font(Face::title, size).withExtraKerningFactor(0.03f));
    g.drawFittedText(t, r.toNearestInt(), j, 2, 0.75f);
}

void drawWide(juce::Graphics &g, const juce::String &t, juce::Rectangle<float> r, float height, float stretch, juce::Colour c,
              juce::Justification j)
{
    using namespace juce;
    GlyphArrangement ga;
    ga.addLineOfText(font(Face::logo, height).withExtraKerningFactor(0.04f), t, 0.0f, 0.0f);
    Path p;
    ga.createPath(p);
    auto b = p.getBounds();
    float w = b.getWidth() * stretch, h = b.getHeight();
    float x = j.testFlags(Justification::left) ? r.getX() : j.testFlags(Justification::right) ? r.getRight() - w : r.getCentreX() - w * 0.5f;
    float y = r.getCentreY() - h * 0.5f;
    p.applyTransform(AffineTransform::translation(-b.getX(), -b.getY()).scaled(stretch, 1.0f).translated(x, y));
    g.setColour(c);
    g.fillPath(p);
}

void drawSection(juce::Graphics &g, juce::Rectangle<float> r, const juce::String &title)
{
    using namespace juce;
    auto f = font(Face::title, 17.0f).withExtraKerningFactor(0.06f);
    float tw = textWidth(f, title), cx = r.getCentreX(), gl = cx - tw * 0.5f - 9.0f, gr = cx + tw * 0.5f + 9.0f, rad = 9.0f;
    Path p;
    p.startNewSubPath(gr, r.getY());
    p.lineTo(r.getRight() - rad, r.getY());
    p.quadraticTo(r.getRight(), r.getY(), r.getRight(), r.getY() + rad);
    p.lineTo(r.getRight(), r.getBottom() - rad);
    p.quadraticTo(r.getRight(), r.getBottom(), r.getRight() - rad, r.getBottom());
    p.lineTo(r.getX() + rad, r.getBottom());
    p.quadraticTo(r.getX(), r.getBottom(), r.getX(), r.getBottom() - rad);
    p.lineTo(r.getX(), r.getY() + rad);
    p.quadraticTo(r.getX(), r.getY(), r.getX() + rad, r.getY());
    if (title.isNotEmpty())
        p.lineTo(gl, r.getY());
    else
        p.closeSubPath();
    g.setColour(col::line.withAlpha(0.9f));
    g.strokePath(p, PathStrokeType(1.5f));
    if (title.isNotEmpty()) {
        g.setColour(col::legend);
        g.setFont(f);
        g.drawText(title, Rectangle<float>(gl, r.getY() - 11.0f, gr - gl, 22.0f), Justification::centred, false);
    }
}

/* ---------------------------------------------------------------------------------- the coat --- */
/* The finish: a powder coat, a dense, even field of tiny round bumps lit from the upper left (a bright crown, a dark
 * foot), the pigment's grain between them: at arm's length soft matte metal, close up a fine pebbling. Made at the
 * pixel density it is shown at, kept per density, tiling seamlessly. */
static juce::Image makeCoat(float sc, float strength)
{
    using namespace juce;
    const int T = 512;
    std::vector<float> h((size_t)(T * T), 0.0f);
    Random rnd(7719);
    const float spacing = jmax(1.6f, 1.05f * sc);
    const int n = (int)((float)T / spacing);
    const float cell = (float)T / (float)n;
    for (int gy = 0; gy < n; gy++)
        for (int gx = 0; gx < n; gx++) {
            float cx = ((float)gx + 0.2f + 0.6f * rnd.nextFloat()) * cell, cy = ((float)gy + 0.2f + 0.6f * rnd.nextFloat()) * cell;
            float rad = cell * (0.42f + 0.18f * rnd.nextFloat()), amp = 0.7f + 0.5f * rnd.nextFloat();
            int x0 = (int)std::floor(cx - rad * 1.8f), x1 = (int)std::ceil(cx + rad * 1.8f);
            int y0 = (int)std::floor(cy - rad * 1.8f), y1 = (int)std::ceil(cy + rad * 1.8f);
            for (int y = y0; y <= y1; y++)
                for (int x = x0; x <= x1; x++) {
                    float dx = ((float)x - cx) / rad, dy = ((float)y - cy) / rad, e = dx * dx + dy * dy;
                    if (e < 3.2f)
                        h[(size_t)(((y % T + T) % T) * T + ((x % T + T) % T))] += amp * std::exp(-e);
                }
        }
    Image im(Image::ARGB, T, T, true);
    Image::BitmapData bd(im, Image::BitmapData::writeOnly);
    const float LX = -0.55f, LY = -0.68f, LZ = 0.48f, ln = std::sqrt(LX * LX + LY * LY + LZ * LZ);
    const float lx = LX / ln, ly = LY / ln, lz = LZ / ln, k = 0.9f;
    const float soft = jlimit(0.55f, 1.0f, sc * 0.5f) * strength;
    for (int y = 0; y < T; y++)
        for (int x = 0; x < T; x++) {
            float dx = h[(size_t)(y * T + (x + 1) % T)] - h[(size_t)(y * T + (x + T - 1) % T)];
            float dy = h[(size_t)(((y + 1) % T) * T + x)] - h[(size_t)(((y + T - 1) % T) * T + x)];
            float nx = -dx * k, ny = -dy * k, nl = std::sqrt(nx * nx + ny * ny + 1.0f);
            float v = (nx * lx + ny * ly + lz) / nl - lz + (rnd.nextFloat() - 0.5f) * 0.05f;
            Colour c = v > 0.0f ? Colour(0xffe8e6e0).withAlpha(soft * jmin(0.08f, v * 0.22f)) : Colours::black.withAlpha(soft * jmin(0.42f, -v * 0.75f));
            bd.setPixelColour(x, y, c);
        }
    return im;
}

static void coat(juce::Graphics &g, juce::Rectangle<float> r, float sc, int which, const juce::Path *clip = nullptr)
{
    using namespace juce;
    auto &cache = get().coat[which];
    int key = jlimit(4, 32, (int)std::lround(sc * 4.0f));
    auto it = cache.find(key);
    if (it == cache.end())
        it = cache.emplace(key, makeCoat((float)key / 4.0f, which ? 0.8f : 1.0f)).first;
    Graphics::ScopedSaveState ss(g);
    if (clip)
        g.reduceClipRegion(*clip);
    else
        g.reduceClipRegion(r.toNearestInt());
    float k = 4.0f / (float)key;
    g.setFillType(FillType(it->second, AffineTransform::scale(k).translated(r.getX(), r.getY())));
    g.fillRect(r);
}

void drawPanelFace(juce::Graphics &g, juce::Rectangle<float> r, float sc)
{
    using namespace juce;
    g.setColour(col::panel);
    g.fillRect(r);
    Random rnd(1923);                                  /* (the coat a little thicker and thinner in soft patches) */
    for (int i = 0; i < (int)(r.getWidth() * r.getHeight() / 30000.0f); i++) {
        float x = r.getX() + rnd.nextFloat() * r.getWidth(), y = r.getY() + rnd.nextFloat() * r.getHeight();
        float s = 40.0f + rnd.nextFloat() * 180.0f;
        Colour c = rnd.nextFloat() < 0.5f ? Colour(0xff3a3b3f) : Colour(0xff1c1d1f);
        g.setGradientFill(ColourGradient(c.withAlpha(0.10f), x, y, c.withAlpha(0.0f), x + s, y, true));
        g.fillEllipse(x - s, y - s, s * 2.0f, s * 2.0f);
    }
    coat(g, r, sc, 0);
    g.setGradientFill(ColourGradient(Colours::white.withAlpha(0.045f), r.getX(), r.getY(), Colours::black.withAlpha(0.15f), r.getRight(),
                                     r.getBottom(), false));
    g.fillRect(r);
    ColourGradient sheen(Colours::white.withAlpha(0.0f), r.getX(), r.getY() + r.getHeight() * 0.1f, Colours::white.withAlpha(0.0f),
                         r.getRight(), r.getBottom(), false);
    sheen.addColour(0.40, Colours::white.withAlpha(0.03f));
    g.setGradientFill(sheen);
    g.fillRect(r);
}

void drawPlate(juce::Graphics &g, juce::Rectangle<float> r, float radius, float sc)
{
    using namespace juce;
    Path p;
    p.addRoundedRectangle(r, radius);
    g.setColour(Colours::black.withAlpha(0.28f));       /* (printed thick: the faintest edge) */
    g.strokePath(p, PathStrokeType(1.6f), AffineTransform::translation(0.5f, 1.0f));
    g.setGradientFill(ColourGradient(Colour(0xff5d636b), r.getX(), r.getY(), Colour(0xff4b5058), r.getX() + r.getWidth() * 0.3f, r.getBottom(), false));
    g.fillPath(p);
    coat(g, r, sc, 1, &p);
    ColourGradient sh(Colours::white.withAlpha(0.06f), r.getX(), r.getY(), Colours::black.withAlpha(0.10f), r.getRight(), r.getBottom(), false);
    g.setGradientFill(sh);
    g.fillPath(p);
    g.setColour(Colours::white.withAlpha(0.10f));
    g.strokePath(p, PathStrokeType(0.8f));
}

void drawEndCap(juce::Graphics &g, juce::Rectangle<float> r, bool left)
{
    using namespace juce;
    /* black anodized aluminium, brushed along its length, its chamfers catching the light */
    g.setGradientFill(ColourGradient(Colour(0xff1b1c1e), r.getX(), 0.0f, Colour(0xff0b0b0c), r.getRight(), 0.0f, false));
    g.fillRect(r);
    Random rnd(left ? 17 : 29);
    for (int i = 0; i < (int)(r.getWidth() * 3.0f); i++) {
        float x = r.getX() + rnd.nextFloat() * r.getWidth();
        g.setColour((rnd.nextBool() ? Colours::white : Colours::black).withAlpha(0.025f + 0.035f * rnd.nextFloat()));
        g.drawLine(x, r.getY(), x + rnd.nextFloat() * 0.6f - 0.3f, r.getBottom(), 0.6f);
    }
    float ix = left ? r.getRight() - 3.0f : r.getX() + 1.0f;
    g.setGradientFill(ColourGradient(Colours::white.withAlpha(0.22f), ix, 0.0f, Colours::white.withAlpha(0.0f), ix + (left ? -6.0f : 6.0f), 0.0f, false));
    g.fillRect(left ? Rectangle<float>(r.getRight() - 8.0f, r.getY(), 8.0f, r.getHeight()) : Rectangle<float>(r.getX(), r.getY(), 8.0f, r.getHeight()));
    float ox = left ? r.getX() : r.getRight() - 4.0f;
    g.setColour(Colours::white.withAlpha(0.12f));
    g.fillRect(ox, r.getY(), 1.2f, r.getHeight());
    for (float y : {r.getY() + 28.0f, r.getBottom() - 28.0f}) /* two hex screws, as the module's */
        for (int k = 0; k < 1; k++) {
            Point<float> c(r.getCentreX(), y);
            g.setColour(Colours::black.withAlpha(0.7f));
            g.fillEllipse(Rectangle<float>(13.0f, 13.0f).withCentre(c.translated(0.4f, 0.8f)));
            g.setGradientFill(ColourGradient(Colour(0xff4a4b4e), c.x - 4.0f, c.y - 4.0f, Colour(0xff111112), c.x + 5.0f, c.y + 5.0f, false));
            g.fillEllipse(Rectangle<float>(12.0f, 12.0f).withCentre(c));
            Path hex;
            for (int s = 0; s < 6; s++) {
                auto q = c.getPointOnCircumference(3.2f, (float)s * MathConstants<float>::pi / 3.0f + 0.3f);
                s ? hex.lineTo(q) : hex.startNewSubPath(q);
            }
            hex.closeSubPath();
            g.setColour(Colour(0xff050505));
            g.fillPath(hex);
        }
}

void drawLip(juce::Graphics &g, juce::Rectangle<float> r)
{
    using namespace juce;
    /* the front extrusion: black anodized, the light along its upper chamfer */
    g.setGradientFill(ColourGradient(Colour(0xff1d1e20), 0.0f, r.getY(), Colour(0xff0c0c0d), 0.0f, r.getBottom(), false));
    g.fillRect(r);
    Random rnd(53);
    for (int i = 0; i < (int)(r.getHeight() * 5.0f); i++) {
        float y = r.getY() + rnd.nextFloat() * r.getHeight();
        g.setColour((rnd.nextBool() ? Colours::white : Colours::black).withAlpha(0.02f + 0.03f * rnd.nextFloat()));
        g.drawLine(r.getX(), y, r.getRight(), y + rnd.nextFloat() - 0.5f, 0.5f);
    }
    g.setGradientFill(ColourGradient(Colours::white.withAlpha(0.20f), 0.0f, r.getY(), Colours::white.withAlpha(0.0f), 0.0f, r.getY() + 7.0f, false));
    g.fillRect(r.withHeight(7.0f));
    g.setColour(Colours::black);
    g.fillRect(r.withHeight(1.2f));
}

/* A few years of use, over the print: the coat rubbed glossy round the knobs played most, light scuffs, fine
 * scratches, a few chips at the edges, the print worn thin in pin-holes. Gentle: a cared-for instrument. Fixed seeds:
 * the same panel every time. */
void drawWear(juce::Graphics &g, juce::Rectangle<float> r, float px, const std::vector<juce::Point<float>> &played)
{
    using namespace juce;
    Random rnd(4242);
    auto P = [&](float mx, float my) { return Point<float>(r.getX() + mx * r.getWidth(), r.getY() + my * r.getHeight()); };
    for (auto c : played) {
        float rr = 30.0f + rnd.nextFloat() * 8.0f;
        ColourGradient cg(Colours::white.withAlpha(0.0f), c.x, c.y, Colours::white.withAlpha(0.0f), c.x + rr * 1.6f, c.y, true);
        cg.addColour(0.62, Colours::white.withAlpha(0.03f + 0.02f * rnd.nextFloat()));
        g.setGradientFill(cg);
        g.fillEllipse(Rectangle<float>(rr * 3.2f, rr * 3.2f).withCentre(c));
        for (int k = 0; k < 22; k++) {
            float a0 = rnd.nextFloat() * 6.283f, len = 0.3f + rnd.nextFloat() * 1.1f, rad = rr * (0.85f + rnd.nextFloat() * 0.6f);
            Path p;
            p.addCentredArc(c.x, c.y, rad, rad, 0.0f, a0, a0 + len, true);
            g.setColour(Colour(0xffcfccc4).withAlpha(0.04f + 0.06f * rnd.nextFloat()));
            g.strokePath(p, PathStrokeType(px));
        }
    }
    for (int i = 0; i < 9; i++) {
        auto c = P(rnd.nextFloat(), rnd.nextFloat());
        float w = 30.0f + rnd.nextFloat() * 120.0f, h = w * (0.2f + rnd.nextFloat() * 0.3f), ang = rnd.nextFloat() * 3.14f;
        Graphics::ScopedSaveState s(g);
        g.addTransform(AffineTransform::rotation(ang, c.x, c.y));
        float a = 0.02f + 0.035f * rnd.nextFloat();
        g.setGradientFill(ColourGradient(Colour(0xff8a8780).withAlpha(a), c.x, c.y, Colour(0xff8a8780).withAlpha(0.0f), c.x + w * 0.5f, c.y, true));
        g.fillEllipse(c.x - w * 0.5f, c.y - h * 0.5f, w, h);
        for (int k = 0; k < 10; k++) {
            float yy = c.y + (rnd.nextFloat() - 0.5f) * h * 0.8f, xx = c.x + (rnd.nextFloat() - 0.5f) * w * 0.6f;
            g.setColour(Colour(0xff9a968c).withAlpha(a * 0.9f));
            g.drawLine(xx - w * 0.2f, yy, xx + w * 0.2f, yy, px);
        }
    }
    for (int i = 0; i < 240; i++) {
        auto a = P(rnd.nextFloat(), rnd.nextFloat());
        float len = rnd.nextFloat() < 0.85f ? 4.0f + rnd.nextFloat() * 30.0f : 30.0f + rnd.nextFloat() * 120.0f;
        float ang = rnd.nextFloat() < 0.6f ? -0.3f + rnd.nextFloat() * 0.6f : rnd.nextFloat() * 3.14f;
        auto b = a + Point<float>(std::cos(ang), std::sin(ang)) * len;
        Path p;
        p.startNewSubPath(a);
        p.quadraticTo((a + b) * 0.5f + Point<float>(rnd.nextFloat() - 0.5f, rnd.nextFloat() - 0.5f) * len * 0.15f, b);
        float w = px * (1.0f + rnd.nextFloat() * 0.6f);
        PathStrokeType st(w, PathStrokeType::curved, PathStrokeType::rounded);
        g.setColour(Colours::black.withAlpha(0.22f));
        g.strokePath(p, st, AffineTransform::translation(w * 0.7f, w * 0.7f));
        g.setColour(Colour(0xffc4c1b8).withAlpha(len > 50.0f ? 0.03f + 0.035f * rnd.nextFloat() : 0.045f + 0.09f * rnd.nextFloat()));
        g.strokePath(p, st);
    }
    for (int i = 0; i < 7; i++) {
        float t = rnd.nextFloat();
        int side = rnd.nextInt(2);
        Point<float> c = side == 0 ? P(t, 0.0f) : P(t, 1.0f);
        c.y += (side == 0 ? 1.0f : -1.0f) * rnd.nextFloat() * 3.0f;
        Path chip;
        int n = 7 + rnd.nextInt(4);
        float s = 0.8f + rnd.nextFloat() * rnd.nextFloat() * 3.5f;
        for (int k = 0; k < n; k++) {
            float a = (float)k / (float)n * 6.283f;
            auto q = c + Point<float>(std::cos(a), std::sin(a)) * s * (0.45f + rnd.nextFloat() * 0.7f);
            k ? chip.lineTo(q) : chip.startNewSubPath(q);
        }
        chip.closeSubPath();
        g.setColour(Colours::black.withAlpha(0.5f));
        g.strokePath(chip, PathStrokeType(px * 1.4f));
        g.setGradientFill(ColourGradient(Colour(0xffc8c9c4), c.x - s, c.y - s, Colour(0xff626360), c.x + s, c.y + s, false));
        g.fillPath(chip);
    }
    const int n = (int)(r.getWidth() * r.getHeight() * 0.002f / jmax(0.25f, px));
    for (int i = 0; i < n; i++) {
        auto c = P(rnd.nextFloat(), rnd.nextFloat());
        float s = px * (0.8f + rnd.nextFloat() * 1.4f);
        g.setColour(Colour(0xff2a2b2e).withAlpha(0.4f + 0.5f * rnd.nextFloat()));
        g.fillEllipse(c.x, c.y, s, s * (0.6f + rnd.nextFloat() * 0.8f));
    }
}

void drawScrew(juce::Graphics &g, juce::Point<float> c, float rr)
{
    using namespace juce;
    Random rnd((int64)(c.x * 13.0f + c.y * 7.0f));
    g.setColour(Colours::black.withAlpha(0.6f));
    g.fillEllipse(Rectangle<float>(rr * 2.3f, rr * 2.3f).withCentre(c.translated(0.4f, 0.7f)));
    ColourGradient hg(Colour(0xff6a6b6e), c.x - rr * 0.5f, c.y - rr * 0.6f, Colour(0xff141416), c.x + rr, c.y + rr, true);
    hg.addColour(0.35, Colour(0xff3a3b3e));
    g.setGradientFill(hg);
    g.fillEllipse(Rectangle<float>(rr * 2.0f, rr * 2.0f).withCentre(c));
    float a = 0.4f + rnd.nextFloat();
    Path p;                                            /* a hex socket */
    for (int s = 0; s < 6; s++) {
        auto q = Point<float>(0.0f, 0.0f).getPointOnCircumference(rr * 0.48f, (float)s * MathConstants<float>::pi / 3.0f);
        s ? p.lineTo(q) : p.startNewSubPath(q);
    }
    p.closeSubPath();
    g.setColour(Colour(0xff060606));
    g.fillPath(p, AffineTransform::rotation(a).translated(c));
    g.setColour(Colours::white.withAlpha(0.15f));
    g.strokePath(p, PathStrokeType(rr * 0.06f), AffineTransform::rotation(a).translated(c.translated(rr * 0.05f, rr * 0.08f)));
}

void drawTag(juce::Graphics &g, juce::Rectangle<float> r, juce::Colour c)
{
    const juce::Image &tag = get().tag;
    if (tag.isNull())
        return;
    auto fit = juce::RectanglePlacement(juce::RectanglePlacement::centred).appliedTo(tag.getBounds().toFloat(), r);
    g.setColour(c);
    g.drawImage(tag, fit, juce::RectanglePlacement::stretchToFit, true);
}

void drawLogo(juce::Graphics &g, juce::Rectangle<float> r, juce::Colour c)
{
    using namespace juce;
    /* "FAK", a bar-made E, then "5": heavy, a little extended */
    auto f = font(Face::logo, 100.0f);
    auto glyphs = [&](const String &t) {
        GlyphArrangement ga;
        ga.addLineOfText(f, t, 0.0f, 0.0f);
        Path p;
        ga.createPath(p);
        return p;
    };
    Path a = glyphs("FAK"), five = glyphs("5"), capRef = glyphs("F");
    auto cb = capRef.getBounds();                       /* the cap height */
    const float capTop = cb.getY(), capH = cb.getHeight(), stretch = 1.14f;
    auto ab = a.getBounds();
    const float gap = capH * 0.10f, eW = capH * 0.56f, bar = capH * 0.21f;
    Path e;
    float ex = ab.getRight() + gap;
    for (int i = 0; i < 3; i++) {
        float y = capTop + (capH - bar) * (float)i / 2.0f;
        e.addRectangle(ex, y, eW, bar);
    }
    Path all = a;
    all.addPath(e);
    auto fb = five.getBounds();
    all.addPath(five, AffineTransform::translation(ex + eW + capH * 0.42f - fb.getX(), 0.0f));
    auto b = all.getBounds();
    float sc = jmin(r.getWidth() / (b.getWidth() * stretch), r.getHeight() / b.getHeight());
    float w = b.getWidth() * stretch * sc, h = b.getHeight() * sc;
    all.applyTransform(AffineTransform::translation(-b.getX(), -b.getY()).scaled(stretch * sc, sc)
                           .translated(r.getCentreX() - w * 0.5f, r.getCentreY() - h * 0.5f));
    g.setColour(c);
    g.fillPath(all);
}

void drawWaveIcon(juce::Graphics &g, juce::Rectangle<float> r, int shape, float thick)
{
    using namespace juce;
    Path p;
    float x0 = r.getX(), x1 = r.getRight(), y0 = r.getY(), y1 = r.getBottom(), ym = r.getCentreY(), w = r.getWidth();
    switch (shape) {
    case 0: p.startNewSubPath(x0, ym); p.lineTo(x0 + w * 0.25f, y0); p.lineTo(x0 + w * 0.75f, y1); p.lineTo(x1, ym); break;
    case 1: p.startNewSubPath(x0, y1); p.lineTo(x0 + w * 0.5f, y0); p.lineTo(x0 + w * 0.5f, y1); p.lineTo(x1, y0); p.lineTo(x1, y1); break;
    case 2: p.startNewSubPath(x0, y0); p.lineTo(x0 + w * 0.5f, y1); p.lineTo(x0 + w * 0.5f, y0); p.lineTo(x1, y1); p.lineTo(x1, y0); break;
    case 3:
        p.startNewSubPath(x0, y1); p.lineTo(x0, y0); p.lineTo(x0 + w * 0.5f, y0); p.lineTo(x0 + w * 0.5f, y1); p.lineTo(x1, y1);
        p.lineTo(x1, y0);
        break;
    case 4: {
        const float hs[4] = {0.3f, 0.0f, 0.7f, 0.15f};
        p.startNewSubPath(x0, y0 + (y1 - y0) * hs[0]);
        for (int i = 0; i < 4; i++) {
            float xa = x0 + w * (float)i / 4.0f, xb = x0 + w * (float)(i + 1) / 4.0f, y = y0 + (y1 - y0) * hs[i];
            p.lineTo(xa, y);
            p.lineTo(xb, y);
        }
        break;
    }
    case 5:
        p.startNewSubPath(x0, ym);
        for (int i = 1; i <= 24; i++) {
            float t = (float)i / 24.0f;
            p.lineTo(x0 + w * t, ym - (y1 - y0) * 0.5f * std::sin(t * MathConstants<float>::twoPi));
        }
        break;
    default:
        p.startNewSubPath(x0, y1); p.lineTo(x0, y0); p.lineTo(x0 + w * 0.22f, y0); p.lineTo(x0 + w * 0.22f, y1); p.lineTo(x1, y1);
        p.lineTo(x1, y0);
    }
    g.strokePath(p, PathStrokeType(thick, PathStrokeType::mitered, PathStrokeType::square));
}
} // namespace f5ui
