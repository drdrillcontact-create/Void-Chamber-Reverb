#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <BinaryData.h>
#include <cmath>

//==============================================================================
// Void Chamber Reverb — shared visual identity.
// Palette, fonts, tracked-text drawing helpers and the neon look-and-feel.
//==============================================================================
namespace VoidGUI
{
    // Neon accent palette (sampled/graded from the concept artwork)
    const juce::Colour pink        { 0xffff2e88 };
    const juce::Colour pinkHot     { 0xffff7dbd };
    const juce::Colour pinkDeep    { 0xffc2185b };
    const juce::Colour cyan        { 0xff23e6ff };
    const juce::Colour cyanHot     { 0xffa5f4ff };
    const juce::Colour purple      { 0xffb14bff };
    const juce::Colour white       { 0xfff2f4fa };
    const juce::Colour textDim     { 0xff9aa0b0 };
    const juce::Colour textFaint   { 0xff5b6270 };
    const juce::Colour panel       { 0xff0c0e14 };
    const juce::Colour panelStroke { 0xff232833 };
    const juce::Colour panelInner  { 0xff3a4152 };

    // Fonts -----------------------------------------------------------------
    inline juce::Typeface::Ptr raleway (int weight)
    {
        static juce::Typeface::Ptr f200 = juce::Typeface::createSystemTypefaceFor(BinaryData::Raleway_200_ttf, (size_t) BinaryData::Raleway_200_ttfSize);
        static juce::Typeface::Ptr f300 = juce::Typeface::createSystemTypefaceFor(BinaryData::Raleway_300_ttf, (size_t) BinaryData::Raleway_300_ttfSize);
        static juce::Typeface::Ptr f400 = juce::Typeface::createSystemTypefaceFor(BinaryData::Raleway_400_ttf, (size_t) BinaryData::Raleway_400_ttfSize);
        static juce::Typeface::Ptr f500 = juce::Typeface::createSystemTypefaceFor(BinaryData::Raleway_500_ttf, (size_t) BinaryData::Raleway_500_ttfSize);
        static juce::Typeface::Ptr f600 = juce::Typeface::createSystemTypefaceFor(BinaryData::Raleway_600_ttf, (size_t) BinaryData::Raleway_600_ttfSize);
        static juce::Typeface::Ptr f700 = juce::Typeface::createSystemTypefaceFor(BinaryData::Raleway_700_ttf, (size_t) BinaryData::Raleway_700_ttfSize);
        switch (weight)
        {
            case 200: return f200;
            case 300: return f300;
            case 400: return f400;
            case 500: return f500;
            case 600: return f600;
            default:  return f700;
        }
    }

    inline juce::Font font (int weight, float size)
    {
        return juce::Font (juce::FontOptions (raleway (weight)).withHeight (size));
    }

    inline float textWidth (const juce::Font& f, const juce::String& text)
    {
        return juce::GlyphArrangement::getStringWidth (f, text);
    }

    inline juce::Rectangle<float> centredRect (juce::Point<float> centre, float w, float h)
    {
        return { centre.x - w * 0.5f, centre.y - h * 0.5f, w, h };
    }

    // Draws horizontally letter-spaced text. tracking is added per glyph in px.
    inline void drawTrackedText (juce::Graphics& g, const juce::String& text,
                                 juce::Rectangle<float> area, float tracking,
                                 const juce::Font& f, juce::Colour colour,
                                 juce::Justification justification = juce::Justification::centred)
    {
        if (text.isEmpty())
            return;

        g.setFont (f);
        g.setColour (colour);

        float total = 0.0f;
        for (int i = 0; i < text.length(); ++i)
            total += textWidth (f, text.substring (i, i + 1)) + tracking;
        total -= tracking;

        float x = area.getX();
        if (justification.testFlags (juce::Justification::centred))      x = area.getCentreX() - total * 0.5f;
        else if (justification.testFlags (juce::Justification::right))   x = area.getRight() - total;

        const float yBaseline = area.getCentreY() + f.getAscent() * 0.42f;
        for (int i = 0; i < text.length(); ++i)
        {
            const auto ch = text.substring (i, i + 1);
            g.drawSingleLineText (ch, juce::roundToInt (x), juce::roundToInt (yBaseline));
            x += textWidth (f, ch) + tracking;
        }
    }

    // Rounded panel with the signature inner-top highlight.
    inline void drawPanel (juce::Graphics& g, juce::Rectangle<float> r, float cornerRadius = 16.0f, float fillAlpha = 0.72f)
    {
        juce::Path path;
        path.addRoundedRectangle (r, cornerRadius);

        g.setColour (panel.withAlpha (fillAlpha));
        g.fillPath (path);

        g.setColour (panelStroke);
        g.strokePath (path, juce::PathStrokeType (1.2f));

        // Inner top highlight
        juce::Path top;
        top.addRoundedRectangle (r.reduced (1.0f), cornerRadius - 1.0f);
        g.setColour (juce::Colours::white.withAlpha (0.045f));
        g.strokePath (top, juce::PathStrokeType (1.0f));
    }

    // Soft neon glow stroke: three passes wide->thin.
    inline void strokeGlow (juce::Graphics& g, const juce::Path& path, juce::Colour colour, float coreWidth)
    {
        g.setColour (colour.withAlpha (0.16f));
        g.strokePath (path, juce::PathStrokeType (coreWidth * 2.7f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (colour.withAlpha (0.38f));
        g.strokePath (path, juce::PathStrokeType (coreWidth * 1.55f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (colour);
        g.strokePath (path, juce::PathStrokeType (coreWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
}

//==============================================================================
// Neon rotary knob painted entirely in vector graphics.
//==============================================================================
class VoidKnobLookAndFeel : public juce::LookAndFeel_V4
{
public:
    enum class Accent { pink, cyan };

    VoidKnobLookAndFeel()
    {
        setColour (juce::Slider::rotarySliderFillColourId,    VoidGUI::pink);
        setColour (juce::Slider::rotarySliderOutlineColourId, VoidGUI::panelStroke);
        setColour (juce::Slider::thumbColourId,               VoidGUI::white);
    }

    explicit VoidKnobLookAndFeel (Accent a) : VoidKnobLookAndFeel() { accent = a; }

    void setAccent (Accent a) { accent = a; }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider& slider) override
    {
        const auto bounds = juce::Rectangle<float> (static_cast<float> (x), static_cast<float> (y),
                                                    static_cast<float> (width), static_cast<float> (height));
        const float size  = juce::jmin (bounds.getWidth(), bounds.getHeight());
        const auto centre = bounds.getCentre();

        const auto accentColour = accent == Accent::pink ? VoidGUI::pink : VoidGUI::cyan;
        const float arcRadius   = size * 0.400f;
        const float trackWidth  = size * 0.062f;

        const float a0 = rotaryStartAngle;
        const float a1 = a0 + (rotaryEndAngle - rotaryStartAngle) * sliderPosProportional;

        // --- halo track (dim) --------------------------------------------------
        juce::Path track;
        const float r = arcRadius;
        const float cx = centre.x, cy = centre.y;
        const auto arcPoint = [&] (float angle, float radius)
        {
            return juce::Point<float> (cx + radius * std::sin (angle),
                                       cy - radius * std::cos (angle));
        };
        track.startNewSubPath (arcPoint (a0, r));
        track.addCentredArc (cx, cy, r, r, 0.0f, a0, rotaryEndAngle, true);
        g.setColour (juce::Colour (0xff1b1e26));
        g.strokePath (track, juce::PathStrokeType (trackWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (juce::Colour (0xff343a47));
        g.strokePath (track, juce::PathStrokeType (trackWidth * 0.32f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // --- value arc (neon) ----------------------------------------------------
        if (sliderPosProportional > 0.004f)
        {
            juce::Path value;
            value.startNewSubPath (arcPoint (a0, r));
            value.addCentredArc (cx, cy, r, r, 0.0f, a0, a1, true);
            VoidGUI::strokeGlow (g, value, accentColour, trackWidth * 0.62f);

            // Hot tip dot
            const auto tip = arcPoint (a1, r);
            g.setColour (juce::Colours::white.withAlpha (0.95f));
            g.fillEllipse (tip.x - trackWidth * 0.34f, tip.y - trackWidth * 0.34f,
                           trackWidth * 0.68f, trackWidth * 0.68f);
        }

        // --- body -----------------------------------------------------------------
        const float bodyR = size * 0.315f;
        const auto bodyRect = VoidGUI::centredRect (centre, bodyR * 2.0f, bodyR * 2.0f);

        juce::ColourGradient bodyGrad (juce::Colour (0xff3a3d46), centre.x - bodyR * 0.4f, centre.y - bodyR * 0.5f,
                                       juce::Colour (0xff101218), centre.x + bodyR * 0.35f, centre.y + bodyR * 0.5f, true);
        bodyGrad.addColour (0.45, juce::Colour (0xff23252c));
        g.setGradientFill (bodyGrad);
        g.fillEllipse (bodyRect);

        g.setColour (juce::Colour (0xff4a5060).withAlpha (0.9f));
        g.drawEllipse (bodyRect, 1.1f);

        // Inner top bevel catch-light
        juce::Path bevel;
        bevel.addCentredArc (centre.x, centre.y, bodyR - 1.5f, bodyR - 1.5f, 0.0f,
                             juce::MathConstants<float>::pi * 1.15f,
                             juce::MathConstants<float>::pi * 1.85f, true);
        g.setColour (juce::Colours::white.withAlpha (0.16f));
        g.strokePath (bevel, juce::PathStrokeType (1.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        // --- pointer ----------------------------------------------------------------
        const float pointerAngle = a0 + (rotaryEndAngle - rotaryStartAngle) * sliderPosProportional;
        const auto  dir          = juce::Point<float> (std::sin (pointerAngle), -std::cos (pointerAngle));
        const auto  p0           = centre + dir * (bodyR * 0.52f);
        const auto  p1           = centre + dir * (bodyR * 0.86f);

        g.setColour (juce::Colour (0xffe8ecf5));
        g.drawLine ({ p0, p1 }, size * 0.028f);
        g.setColour (accentColour.withAlpha (0.9f));
        g.fillEllipse (p0.x - size * 0.016f, p0.y - size * 0.016f, size * 0.032f, size * 0.032f);
    }

private:
    Accent accent = Accent::pink;
};

//==============================================================================
// Toggle-style neon text button (MONO / STEREO).
//==============================================================================
class NeonButtonLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawButtonBackground (juce::Graphics& g, juce::Button& button,
                               const juce::Colour&, bool isMouseOverButton, bool isButtonDown) override
    {
        const auto b = button.getLocalBounds().toFloat().reduced (1.0f);
        const bool on = button.getToggleState();
        const auto cyan = VoidGUI::cyan;

        juce::Path path;
        path.addRoundedRectangle (b, b.getHeight() * 0.28f);

        g.setColour (on ? cyan.withAlpha (0.10f) : juce::Colour (0xff0d0f15).withAlpha (0.85f));
        g.fillPath (path);

        if (on)
        {
            g.setColour (cyan.withAlpha (0.35f));
            g.strokePath (path, juce::PathStrokeType (4.5f));
        }
        g.setColour (on ? cyan : (isMouseOverButton ? VoidGUI::panelInner : VoidGUI::panelStroke));
        g.strokePath (path, juce::PathStrokeType (isButtonDown ? 2.0f : 1.3f));
    }

    void drawButtonText (juce::Graphics& g, juce::TextButton& button,
                         bool, bool) override
    {
        const bool on = button.getToggleState();
        VoidGUI::drawTrackedText (g, button.getButtonText().toUpperCase(),
                                  button.getLocalBounds().toFloat(), 3.2f,
                                  VoidGUI::font (500, 15.0f),
                                  on ? VoidGUI::cyan : VoidGUI::textDim);
    }
};

//==============================================================================
// Preset name pill (current preset, sits between the arrows in the header).
//==============================================================================
class NeonPillLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawButtonBackground (juce::Graphics& g, juce::Button& button,
                               const juce::Colour&, bool isMouseOverButton, bool isButtonDown) override
    {
        const auto b = button.getLocalBounds().toFloat().reduced (0.5f);
        juce::Path path;
        path.addRoundedRectangle (b, 12.0f);
        g.setColour (juce::Colour (0xff0b0d13).withAlpha (0.9f));
        g.fillPath (path);
        g.setColour (isMouseOverButton ? VoidGUI::panelInner : VoidGUI::panelStroke);
        g.strokePath (path, juce::PathStrokeType (1.2f));
        if (isButtonDown)
            g.setColour (VoidGUI::pink.withAlpha (0.25f)), g.fillPath (path);
    }

    void drawButtonText (juce::Graphics& g, juce::TextButton& button,
                         bool, bool) override
    {
        const auto b = button.getLocalBounds().toFloat();
        VoidGUI::drawTrackedText (g, button.getButtonText(),
                                  b.reduced (56.0f, 0.0f), 0.6f,
                                  VoidGUI::font (500, 21.0f),
                                  VoidGUI::white,
                                  juce::Justification::centred);
    }
};
