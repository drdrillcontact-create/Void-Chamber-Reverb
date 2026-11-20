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
    const juce::Colour textDim     { 0xffbac8d3 };
    const juce::Colour textFaint   { 0xff81939f };
    const juce::Colour panel       { 0xff0c0e14 };
    const juce::Colour panelStroke { 0xff3f535f };
    const juce::Colour panelInner  { 0xff3a4152 };

    // Fonts -----------------------------------------------------------------
    inline juce::Typeface::Ptr raleway (int weight)
    {
        static juce::Typeface::Ptr f200 = juce::Typeface::createSystemTypefaceFor(BinaryData::Raleway200_ttf, (size_t) BinaryData::Raleway200_ttfSize);
        static juce::Typeface::Ptr f300 = juce::Typeface::createSystemTypefaceFor(BinaryData::Raleway300_ttf, (size_t) BinaryData::Raleway300_ttfSize);
        static juce::Typeface::Ptr f400 = juce::Typeface::createSystemTypefaceFor(BinaryData::Raleway400_ttf, (size_t) BinaryData::Raleway400_ttfSize);
        static juce::Typeface::Ptr f500 = juce::Typeface::createSystemTypefaceFor(BinaryData::Raleway500_ttf, (size_t) BinaryData::Raleway500_ttfSize);
        static juce::Typeface::Ptr f600 = juce::Typeface::createSystemTypefaceFor(BinaryData::Raleway600_ttf, (size_t) BinaryData::Raleway600_ttfSize);
        static juce::Typeface::Ptr f700 = juce::Typeface::createSystemTypefaceFor(BinaryData::Raleway700_ttf, (size_t) BinaryData::Raleway700_ttfSize);
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

    inline float meterPosition (float linearGain)
    {
        return juce::jlimit (0.0f, 1.0f,
            (juce::Decibels::gainToDecibels (linearGain, -60.0f) + 60.0f) / 60.0f);
    }

    inline juce::Font titleFont (float size)
    {
        static const auto face = juce::Typeface::createSystemTypefaceFor (
            BinaryData::CormorantGaramond_ttf, static_cast<size_t> (BinaryData::CormorantGaramond_ttfSize));
        return juce::Font (juce::FontOptions (face != nullptr ? face : raleway (400)).withHeight (size));
    }

    // Smoked-glass construction shared by the header, meters and control bays.
    // Borders are vector strokes so they remain crisp at every host size.
    inline void drawPanel (juce::Graphics& g, juce::Rectangle<float> r,
                           float cornerRadius = 16.0f, float fillAlpha = 0.88f)
    {
        g.setColour (juce::Colours::black.withAlpha (0.45f));
        g.fillRoundedRectangle (r.translated (0.0f, 3.0f).expanded (2.0f), cornerRadius + 1.0f);
        juce::ColourGradient glass (juce::Colour (0xff1b2933).withAlpha (fillAlpha), r.getX(), r.getY(),
                                    juce::Colour (0xff050a10).withAlpha (fillAlpha), r.getRight(), r.getBottom(), false);
        glass.addColour (0.18, juce::Colour (0xff0b131b).withAlpha (fillAlpha));
        glass.addColour (0.65, juce::Colour (0xff060b11).withAlpha (fillAlpha));
        g.setGradientFill (glass);
        g.fillRoundedRectangle (r, cornerRadius);
        juce::ColourGradient edge (juce::Colour (0xffa1bac7).withAlpha (0.74f), r.getX(), r.getY(),
                                   juce::Colour (0xff3d4454).withAlpha (0.85f), r.getRight(), r.getBottom(), false);
        edge.addColour (0.3, juce::Colour (0xff2a3b47));
        edge.addColour (0.52, juce::Colour (0xff728795).withAlpha (0.6f));
        g.setGradientFill (edge);
        g.drawRoundedRectangle (r, cornerRadius, 1.3f);
        g.setColour (juce::Colour (0xffa8d9e8).withAlpha (0.13f));
        g.drawRoundedRectangle (r.reduced (2.5f), juce::jmax (1.0f, cornerRadius - 2.5f), 0.9f);
        g.setColour (juce::Colours::black.withAlpha (0.85f));
        g.drawRoundedRectangle (r.reduced (4.2f), juce::jmax (1.0f, cornerRadius - 4.2f), 0.8f);
    }

    // Soft neon glow stroke: three passes wide->thin.
    inline void strokeGlow (juce::Graphics& g, const juce::Path& path, juce::Colour colour, float coreWidth)
    {
        g.setColour (colour.withAlpha (0.08f));
        g.strokePath (path, juce::PathStrokeType (coreWidth * 2.7f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (colour.withAlpha (0.18f));
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
        juce::ignoreUnused (slider);
        const auto bounds = juce::Rectangle<float> (static_cast<float> (x), static_cast<float> (y),
                                                    static_cast<float> (width), static_cast<float> (height));
        const float size = juce::jmin (bounds.getWidth(), bounds.getHeight());
        if (size <= 0.0f) return;
        const auto centre = bounds.getCentre();
        const float cx = centre.x, cy = centre.y;
        const float pos = juce::jlimit (0.0f, 1.0f, sliderPosProportional);
        const float angle = rotaryStartAngle + (rotaryEndAngle - rotaryStartAngle) * pos;
        const auto colour = accent == Accent::pink ? VoidGUI::pink : VoidGUI::cyan;
        const auto at = [=] (float a, float radius)
        {
            return juce::Point<float> (cx + radius * std::sin (a), cy - radius * std::cos (a));
        };
        const auto circle = [=] (float radius)
        {
            return VoidGUI::centredRect (centre, radius * 2.0f, radius * 2.0f);
        };

        // Stationary machined bezel and engraved scale (not baked into artwork).
        g.setColour (juce::Colours::black.withAlpha (0.65f));
        g.fillEllipse (circle (size * 0.48f).translated (0.0f, size * 0.016f));
        juce::ColourGradient bezel (juce::Colour (0xff76777c), cx, cy - size * 0.47f,
                                    juce::Colour (0xff15161a), cx, cy + size * 0.47f, false);
        bezel.addColour (0.14, juce::Colour (0xff34353a));
        bezel.addColour (0.55, juce::Colour (0xff151619));
        g.setGradientFill (bezel);
        g.fillEllipse (circle (size * 0.465f));
        g.setColour (juce::Colour (0xff0d0e10));
        g.fillEllipse (circle (size * 0.444f));
        g.setColour (juce::Colour (0xff535459));
        g.drawEllipse (circle (size * 0.454f), size * 0.006f);
        for (int i = 0; i <= 30; ++i)
        {
            const float a = rotaryStartAngle + (rotaryEndAngle - rotaryStartAngle) * i / 30.0f;
            const bool major = i % 5 == 0;
            g.setColour (juce::Colour (major ? 0xff97989c : 0xff4d4f55));
            g.drawLine ({ at (a, size * (major ? 0.427f : 0.434f)), at (a, size * 0.444f) },
                        size * (major ? 0.006f : 0.003f));
        }

        juce::Path track;
        track.addCentredArc (cx, cy, size * 0.408f, size * 0.408f, 0.0f,
                             rotaryStartAngle, rotaryEndAngle, true);
        g.setColour (juce::Colour (0xff26272c));
        g.strokePath (track, juce::PathStrokeType (size * 0.020f));
        if (pos > 0.0001f)
        {
            juce::Path value;
            value.addCentredArc (cx, cy, size * 0.408f, size * 0.408f, 0.0f,
                                 rotaryStartAngle, angle, true);
            VoidGUI::strokeGlow (g, value, colour, size * 0.017f);
        }

        // Fixed studio lighting on the titanium; only the indicator rotates.
        // Cached decoded artwork is shared by all knobs, never resampled per tick.
        static const auto body = juce::ImageCache::getFromMemory (
            BinaryData::titaniumknobbody_png, BinaryData::titaniumknobbody_pngSize);
        g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
        g.drawImage (body, circle (size * 0.376f));

        const auto inner = at (angle, size * 0.211f);
        const auto outer = at (angle, size * 0.317f);
        const auto stroke = [] (float w)
        {
            return juce::PathStrokeType (w, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
        };
        juce::Path pointer;
        pointer.startNewSubPath (inner);
        pointer.lineTo (outer);
        g.setColour (juce::Colour (0xff1b1a17));
        g.strokePath (pointer, stroke (size * 0.032f));
        g.setColour (juce::Colour (0xffb7ad91));
        g.strokePath (pointer, stroke (size * 0.023f));
        g.setColour (juce::Colour (0xfffff2c8));
        g.strokePath (pointer, stroke (size * 0.016f));

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
        path.addRoundedRectangle (b, 6.0f);

        g.setColour (on ? cyan.withAlpha (0.10f) : juce::Colour (0xff0d0f15).withAlpha (0.85f));
        g.fillPath (path);

        if (on)
        {
            g.setColour (cyan.withAlpha (0.35f));
            g.strokePath (path, juce::PathStrokeType (1.8f));
        }
        g.setColour (on ? cyan : (isMouseOverButton ? VoidGUI::panelInner : VoidGUI::panelStroke));
        g.strokePath (path, juce::PathStrokeType (isButtonDown ? 2.0f : 1.3f));
    }

    void drawButtonText (juce::Graphics& g, juce::TextButton& button,
                         bool, bool) override
    {
        const bool on = button.getToggleState();
        VoidGUI::drawTrackedText (g, button.getButtonText().toUpperCase(),
                                  button.getLocalBounds().toFloat(), 1.0f,
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
        const auto b = button.getLocalBounds().toFloat().reduced (1.0f);
        VoidGUI::drawPanel (g, b, 6.0f, 0.95f);
        if (isMouseOverButton || isButtonDown)
        {
            g.setColour (VoidGUI::cyan.withAlpha (isButtonDown ? 0.10f : 0.035f));
            g.fillRoundedRectangle (b.reduced (3.0f), 4.0f);
        }
    }

    void drawButtonText (juce::Graphics& g, juce::TextButton& button,
                         bool, bool) override
    {
        const auto b = button.getLocalBounds().toFloat();
        g.setFont (VoidGUI::font (400, 21.0f));
        g.setColour (VoidGUI::white);
        g.drawFittedText (button.getButtonText(), b.reduced (10.0f, 2.0f).toNearestInt(),
                          juce::Justification::centred, 1, 0.65f);
    }
};
