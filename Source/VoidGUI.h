#pragma once

// Self-contained neon-space LookAndFeel and custom components for the
// Void Chamber Reverb editor. Everything is drawn with JUCE primitives and
// the images embedded by the VoidChamberAssets binary-data target.

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "BinaryData.h"

namespace voidchamber
{
    inline juce::Font voidFont (float size, juce::Font::FontStyleFlags style = juce::Font::plain)
    {
        return juce::Font (juce::FontOptions (size, style));
    }

    // Draw uppercase text with manual letter tracking (JUCE has no tracking).
    inline void drawTrackedText (juce::Graphics& g, const juce::String& text, juce::Rectangle<int> area,
                                 float tracking, juce::Colour colour, float size,
                                 juce::Font::FontStyleFlags style = juce::Font::plain,
                                 juce::Justification justification = juce::Justification::centred)
    {
        const auto font = voidFont (size, style);
        g.setFont (font);
        g.setColour (colour);

        float total = 0.0f;
        for (auto ch : text)
            total += juce::GlyphArrangement::getStringWidth (font, juce::String::charToString (ch)) + tracking;
        total -= tracking;

        float x = area.getX() + (area.getWidth() - total) * 0.5f;
        if (justification == juce::Justification::left) x = area.getX();
        else if (justification == juce::Justification::right) x = area.getRight() - total;

        const float baselineY = area.getY() + (area.getHeight() + size) / 2.0f - size * 0.28f;
        for (auto ch : text)
        {
            const auto s = juce::String::charToString (ch);
            g.drawSingleLineText (s, (int) std::round (x), (int) baselineY);
            x += juce::GlyphArrangement::getStringWidth (font, s) + tracking;
        }
    }

    inline void drawCapsulePanel (juce::Graphics& g, juce::Rectangle<int> bounds, float corner = 12.0f)
    {
        auto r = bounds.toFloat();
        g.setColour (juce::Colour (0xb20a0d16));
        g.fillRoundedRectangle (r, corner);
        g.setColour (juce::Colour (0x1affffff));
        g.drawRoundedRectangle (r.reduced (0.5f), corner, 1.0f);
        g.setColour (juce::Colour (0x14ffffff));
        g.drawHorizontalLine ((int) r.getY() + 1, r.getX() + corner, r.getRight() - corner);
    }

    //==========================================================================
    class VoidLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                               float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                               juce::Slider& slider) override
        {
            auto bounds = juce::Rectangle<int> (x, y, width, height).toFloat().reduced (3.0f);
            const auto centre = bounds.getCentre();
            const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
            const auto accent = slider.findColour (juce::Slider::rotarySliderFillColourId);
            const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

            // Drop shadow
            g.setColour (juce::Colour (0x66000000));
            g.fillEllipse (bounds.translated (0.0f, 2.5f));

            // Knob body
            juce::ColourGradient body (juce::Colour (0xff24262f), centre.x, centre.y - radius,
                                       juce::Colour (0xff0c0d13), centre.x, centre.y + radius,
                                       false);
            g.setGradientFill (body);
            g.fillEllipse (bounds.reduced (radius * 0.14f));
            g.setColour (juce::Colour (0xff383b46));
            g.drawEllipse (bounds.reduced (radius * 0.14f), 1.2f);
            g.setColour (juce::Colour (0xff1c1e26));
            g.drawEllipse (bounds.reduced (radius * 0.14f).reduced (2.0f), 1.0f);

            // Inactive track
            juce::Path track;
            track.addCentredArc (centre.x, centre.y, radius, radius, 0.0f,
                                 rotaryStartAngle, rotaryEndAngle, true);
            g.setColour (juce::Colour (0xff232633));
            g.strokePath (track, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            // Active arc: halo pass then sharp pass
            juce::Path value;
            value.addCentredArc (centre.x, centre.y, radius, radius, 0.0f,
                                 rotaryStartAngle, angle, true);
            g.setColour (accent.withAlpha (0.16f));
            g.strokePath (value, juce::PathStrokeType (11.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour (accent.withAlpha (0.85f));
            g.strokePath (value, juce::PathStrokeType (4.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            // Pointer
            const float pointerLength = radius * 0.52f;
            const float pointerStart  = radius * 0.18f;
            const auto dx = std::sin (angle), dy = -std::cos (angle);
            g.setColour (juce::Colour (0xfff2f4fa));
            g.drawLine (centre.x + dx * pointerStart, centre.y + dy * pointerStart,
                        centre.x + dx * pointerLength, centre.y + dy * pointerLength, 2.6f);
            g.setColour (accent);
            g.fillEllipse (centre.x - 4.0f, centre.y - 4.0f, 8.0f, 8.0f);
        }

        void drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                   const juce::Colour&, bool, bool) override
        {
            auto bounds = button.getLocalBounds().toFloat().reduced (0.5f);
            const auto accent = button.findColour (juce::TextButton::buttonOnColourId);

            if (button.getClickingTogglesState())
            {
                const bool on = button.getToggleState();
                g.setColour (on ? accent.withAlpha (0.16f) : juce::Colour (0x5910141d));
                g.fillRoundedRectangle (bounds, bounds.getHeight() / 2.0f);
                g.setColour (on ? accent.withAlpha (0.9f) : juce::Colour (0xff3a3e4c));
                g.drawRoundedRectangle (bounds, bounds.getHeight() / 2.0f, on ? 1.6f : 1.1f);
            }
            else if (button.isMouseOver())
            {
                g.setColour (juce::Colour (0x14ffffff));
                g.fillRoundedRectangle (bounds, bounds.getHeight() / 2.0f);
            }
        }

        void drawButtonText (juce::Graphics& g, juce::TextButton& button, bool, bool) override
        {
            const bool on = button.getToggleState();
            g.setColour (button.getClickingTogglesState()
                             ? (on ? button.findColour (juce::TextButton::buttonOnColourId)
                                   : juce::Colour (0xff8d93a6))
                             : juce::Colour (0xffb9bfd2));
            g.setFont (voidFont (13.0f, juce::Font::plain));
            g.drawText (button.getButtonText(), button.getLocalBounds(), juce::Justification::centred);
        }

    };

    //==========================================================================
    // Vertical neon meter with dB tick marks. Reads its level from a source
    // callable and applies its own ballistics.
    class NeonMeter : public juce::Component
    {
    public:
        NeonMeter() = default;

        void setAccent (juce::Colour c)          { accent = c; }
        void setLevelSource (std::function<float()> fn) { source = std::move (fn); }
        void tick()
        {
            const float target = source ? juce::jlimit (0.0f, 1.0f, source()) : 0.0f;
            smoothed = target > smoothed ? target : smoothed * 0.82f;
            repaint();
        }

        void paint (juce::Graphics& g) override
        {
            auto area = getLocalBounds().toFloat();
            const float barWidth = 15.0f;
            const float centreX = area.getRight() - barWidth * 0.5f - 4.0f;
            juce::Rectangle<float> track (centreX - barWidth * 0.5f, area.getY() + 2.0f, barWidth, area.getHeight() - 4.0f);

            // Track
            g.setColour (juce::Colour (0x8c05070c));
            g.fillRoundedRectangle (track, 4.5f);
            g.setColour (juce::Colour (0xff262a36));
            g.drawRoundedRectangle (track, 4.5f, 1.0f);

            // Fill — map the smoothed linear level through a dB-ish scale
            const float db = juce::Decibels::gainToDecibels (smoothed, -80.0f);
            const float frac = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 66.0f);
            const float fillH = (track.getHeight() - 4.0f) * frac;
            if (fillH > 0.5f)
            {
                juce::ColourGradient fill (accent.withAlpha (0.25f), track.getX(), track.getBottom(),
                                           accent, track.getX(), track.getBottom() - fillH, false);
                fill.addColour (0.55, accent.withAlpha (0.75f));
                fill.addColour (1.0, juce::Colours::white);
                g.setGradientFill (fill);
                g.fillRoundedRectangle (track.getX() + 2.0f, track.getBottom() - fillH - 2.0f,
                                        barWidth - 4.0f, fillH, 3.0f);
                g.setColour (accent.withAlpha (0.55f));
                g.fillRect (track.getX() + 2.0f, track.getBottom() - fillH - 3.5f, barWidth - 4.0f, 2.0f);
            }

            // dB tick marks + labels on the left of the bar
            const float tickRight = track.getX() - 5.0f;
            const float tickLeft = tickRight - 7.0f;
            const float yBottom = track.getBottom() - 2.0f;
            const float yTop = track.getY() + 2.0f;
            struct Tick { float db; const char* text; };
            static constexpr Tick ticks[] = { { 6.0f, "+6" }, { 0.0f, "0" }, { -6.0f, "-6" },
                                              { -12.0f, "-12" }, { -24.0f, "-24" }, { -36.0f, "-36" }, { -60.0f, "-60" } };
            g.setFont (voidFont (8.0f));
            for (const auto& t : ticks)
            {
                const float f = juce::jlimit (0.0f, 1.0f, (t.db + 60.0f) / 66.0f);
                const float y = yBottom - (yBottom - yTop) * f;
                g.setColour (juce::Colour (0xff565d70));
                g.drawLine (tickLeft, y, tickRight, y, 1.0f);
                g.setColour (juce::Colour (0xff788098));
                g.drawSingleLineText (t.text, 0, (int) y + 3);
            }
        }

    private:
        juce::Colour accent { 0xff31e6ff };
        std::function<float()> source;
        float smoothed = 0.0f;
    };

    //==========================================================================
    // Circular Space Type selector with an orb image and neon ring.
    class SpaceTypeButton : public juce::Component
    {
    public:
        SpaceTypeButton (const juce::Image& orb, const juce::String& name)
            : orbImage (orb), displayName (name) {}

        std::function<void()> onClick;

        void setSelected (bool shouldBeSelected)
        {
            if (selected != shouldBeSelected)
            {
                selected = shouldBeSelected;
                repaint();
            }
        }

        void paint (juce::Graphics& g) override
        {
            auto area = getLocalBounds();
            const float diameter = (float) juce::jmin (area.getWidth(), area.getHeight() - 24) - 4.0f;
            const float cx = (float) area.getCentreX();
            const float cy = diameter * 0.5f + 2.0f;
            juce::Rectangle<float> circle (cx - diameter * 0.5f, cy - diameter * 0.5f, diameter, diameter);

            if (selected)
            {
                g.setColour (juce::Colour (0xffff2fb0).withAlpha (0.22f));
                g.fillEllipse (circle.expanded (7.0f));
            }
            g.setColour (juce::Colour (0xff0a0c12));
            g.fillEllipse (circle.expanded (1.5f));

            if (orbImage.isValid())
            {
                juce::Path clip;
                clip.addEllipse (circle);
                g.saveState();
                g.reduceClipRegion (clip);
                g.drawImage (orbImage, circle, false);
                g.restoreState();
            }

            const auto ring = selected ? juce::Colour (0xffff3fbd) : juce::Colour (0xff343844);
            g.setColour (ring.withAlpha (selected ? 0.35f : 1.0f));
            g.drawEllipse (circle.expanded (1.5f), selected ? 4.0f : 1.2f);
            g.setColour (ring);
            g.drawEllipse (circle, selected ? 1.8f : 1.1f);

            drawTrackedText (g, displayName.toUpperCase(),
                             { area.getX(), (int) (cy + diameter * 0.5f + 3.0f), area.getWidth(), 16 },
                             1.0f, selected ? juce::Colour (0xffff5fc8) : juce::Colour (0xff8d93a6),
                             8.5f, juce::Font::bold);
        }

        void mouseUp (const juce::MouseEvent& e) override
        {
            if (getLocalBounds().contains (e.getPosition()) && onClick)
                onClick();
        }

    private:
        juce::Image orbImage;
        juce::String displayName;
        bool selected = false;
    };

    //==========================================================================
    // The big central DECAY TIME arc. Acts as a slider (horizontal/vertical
    // drag), draws the glowing arc, the value readout, an analyser-style bar
    // row and the 0.1 s / 20 s end labels.
    class DecayArcSlider : public juce::Slider
    {
    public:
        DecayArcSlider()
        {
            setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
            setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
            setNumDecimalPlacesToDisplay (1);
        }

        std::function<float()> outputLevelSource;

        void setGlowTimeMs (float ms) { glowMs = ms; repaint(); }

        void tick (double phase01)
        {
            levelSmooth = juce::jmax (outputLevelSource ? juce::jlimit (0.0f, 1.0f, outputLevelSource()) : 0.0f,
                                      levelSmooth * 0.86f);
            phase = (float) phase01;
            repaint();
        }

        void paint (juce::Graphics& g) override
        {
            auto bounds = getLocalBounds().toFloat();
            const auto centre = juce::Point<float> (bounds.getCentreX(), bounds.getHeight() * 0.78f);
            const float radius = juce::jmin (bounds.getWidth() * 0.5f, bounds.getHeight() * 0.72f) - 26.0f;
            const float startAngle = juce::MathConstants<float>::pi * 1.25f;
            const float endAngle   = juce::MathConstants<float>::pi * 2.75f;
            const float pos = (float) valueToProportionOfLength (getValue());
            const float arcAngle = startAngle + pos * (endAngle - startAngle);

            auto arcPath = [&] (float from, float to)
            {
                juce::Path p;
                p.addCentredArc (centre.x, centre.y, radius, radius, 0.0f, from, to, true);
                return p;
            };

            // Inactive arc
            g.setColour (juce::Colour (0x1fffffff));
            g.strokePath (arcPath (startAngle, endAngle),
                          juce::PathStrokeType (14.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setColour (juce::Colour (0xff272b38));
            g.strokePath (arcPath (startAngle, endAngle),
                          juce::PathStrokeType (5.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            // Active arc: cyan -> magenta gradient along the angle
            const auto arc = arcPath (startAngle, arcAngle);
            juce::ColourGradient grad (juce::Colour (0xff2fe0ff), centre.x - radius, centre.y,
                                       juce::Colour (0xffff2fb0), centre.x + radius, centre.y, false);
            grad.addColour (0.5, juce::Colour (0xff8a4bff));
            g.setColour (juce::Colour (0xff2fe0ff).withAlpha (0.20f));
            g.strokePath (arc, juce::PathStrokeType (17.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            g.setGradientFill (grad);
            g.strokePath (arc, juce::PathStrokeType (6.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

            // Small bright dot at the arc tip
            const float tipX = centre.x + radius * std::sin (arcAngle);
            const float tipY = centre.y - radius * std::cos (arcAngle);
            g.setColour (juce::Colours::white.withAlpha (0.55f));
            g.fillEllipse (tipX - 3.6f, tipY - 3.6f, 7.2f, 7.2f);
            g.setColour (juce::Colours::white);
            g.fillEllipse (tipX - 2.0f, tipY - 2.0f, 4.0f, 4.0f);

            // Analyser bars driven by the output level (drawn before the readout)
            const int numBars = 56;
            const float barAreaBottom = centre.y + radius * 0.13f;
            const float barMaxH = radius * 0.19f;
            const float barWidth = 4.0f;
            const float barSpacing = (radius * 1.5f) / (float) numBars;
            for (int i = 0; i < numBars; ++i)
            {
                const float t = (float) i / (float) (numBars - 1);
                const float envelope = std::exp (-2.6f * t) * (0.72f + 0.55f * std::sin (12.0f * t * juce::MathConstants<float>::pi + phase * juce::MathConstants<float>::twoPi));
                const float noise = 0.35f + 0.65f * std::abs (std::sin (i * 12.9898f + phase * 6.2831f * 0.7f));
                const float h = juce::jmax (3.0f, envelope * noise * barMaxH * (0.30f + levelSmooth * 1.35f + 0.15f));
                const float x = centre.x - radius * 0.75f + i * barSpacing;
                const float fade = 1.0f - t * 0.55f;
                juce::ColourGradient barGrad (juce::Colour (0xffff2fb0).withAlpha (fade), x, barAreaBottom - h,
                                              juce::Colour (0xff31e6ff).withAlpha (fade * 0.85f), x, barAreaBottom, false);
                g.setGradientFill (barGrad);
                g.fillRoundedRectangle (x, barAreaBottom - h, barWidth, h, 1.6f);
            }

            // DECAY TIME + value (on top of the bars)
            drawTrackedText (g, "DECAY TIME", { (int) centre.x - 130, (int) (centre.y - radius * 0.66f), 260, 18 },
                             2.4f, juce::Colour (0xffb9bfd2), 12.0f, juce::Font::bold);
            const juce::String valueText = juce::String (getValue(), 1);
            const auto bigFont = voidFont (82.0f, juce::Font::bold);
            const auto unitFont = voidFont (26.0f, juce::Font::bold);
            const float valueWidth = juce::GlyphArrangement::getStringWidth (bigFont, valueText);
            const float unitWidth = juce::GlyphArrangement::getStringWidth (unitFont, "s");
            const float totalWidth = valueWidth + 10.0f + unitWidth;
            const float valueX = centre.x - totalWidth * 0.5f;
            const int baseline = (int) (centre.y - radius * 0.18f);
            g.setColour (juce::Colour (0xffe9ecf5).withAlpha (0.35f));
            g.drawSingleLineText (valueText, (int) valueX + 2, baseline + 2);
            g.setColour (juce::Colour (0xfff4f6fc));
            g.setFont (bigFont);
            g.drawSingleLineText (valueText, (int) valueX, baseline);
            g.setColour (juce::Colour (0xff31e6ff));
            g.setFont (unitFont);
            g.drawSingleLineText ("s", (int) (valueX + valueWidth + 10.0f), baseline);

            // End labels (with a dark halo for legibility over the artwork)
            const auto endLabel = [&g, &centre](const char* text, float x, juce::Justification just)
            {
                juce::Rectangle<int> area { (int) x, (int) (centre.y + 6.0f), 70, 16 };
                g.setColour (juce::Colour (0xbb05070c));
                g.drawSingleLineText (text, area.getX() + (just == juce::Justification::left ? 1 : -1), area.getY() + 12);
                voidchamber::drawTrackedText (g, text, area, 0.6f, juce::Colour (0xffb9bfd2), 11.0f,
                                              juce::Font::plain, just);
            };
            endLabel ("0.1 s", centre.x - radius - 22.0f, juce::Justification::left);
            endLabel ("20 s", centre.x + radius - 48.0f, juce::Justification::right);
        }

    private:
        float levelSmooth = 0.0f;
        float phase = 0.0f;
        float glowMs = 0.0f;
    };

    //==========================================================================
    // MEASURED vs estimated? No: analytical estimate of the reverb's decay
    // slope per band, driven live by the actual parameters.
    class FrequencyDecayChart : public juce::Component
    {
    public:
        explicit FrequencyDecayChart (juce::AudioProcessorValueTreeState& state) : apvts (state) {}

        void paint (juce::Graphics& g) override
        {
            auto area = getLocalBounds().toFloat().reduced (1.0f);
            const float size    = apvts.getRawParameterValue ("size")    != nullptr ? apvts.getRawParameterValue ("size")->load()    : 0.72f;
            const float decay   = apvts.getRawParameterValue ("decay")   != nullptr ? apvts.getRawParameterValue ("decay")->load()   : 4.8f;
            const float damping = apvts.getRawParameterValue ("damping") != nullptr ? apvts.getRawParameterValue ("damping")->load() : 0.46f;
            const float air     = apvts.getRawParameterValue ("air")     != nullptr ? apvts.getRawParameterValue ("air")->load() * 0.01f : 0.35f;
            const int   character = apvts.getRawParameterValue ("character") != nullptr
                                  ? (int) std::lround (apvts.getRawParameterValue ("character")->load()) : 0;

            const float charSlope = character == 0 ? 1.32f : (character == 2 ? 0.78f : 1.0f);
            const float decayScale = juce::jmap (decay, 0.1f, 20.0f, 1.45f, 0.48f);
            const float slope = (0.24f + damping * 1.55f) * charSlope * decayScale;
            const float sizeLift = 0.92f + size * 0.22f;

            // Slope and starting height of LOW / MID / HIGH on a log frequency axis
            const float bandStart[3] = { 0.92f, 0.87f, 0.82f };
            const float bandSlope[3] = { 0.50f * sizeLift, 0.78f * sizeLift, 1.08f * (1.0f - 0.42f * air) };
            const juce::Colour colours[3] = { juce::Colour (0xffff3fd4), juce::Colour (0xff9a55ff), juce::Colour (0xff31e6ff) };

            // Frame + gridlines
            g.setColour (juce::Colour (0xff1d2230));
            for (int i = 0; i <= 4; ++i)
            {
                const float x = area.getX() + area.getWidth() * (float) i / 4.0f;
                g.drawLine (x, area.getY() + 2.0f, x, area.getBottom(), 1.0f);
            }
            for (int i = 1; i <= 3; ++i)
            {
                const float y = area.getY() + area.getHeight() * (float) i / 4.0f;
                g.drawLine (area.getX(), y, area.getRight(), y, 1.0f);
            }

            // Curves
            const int numPoints = 72;
            for (int band = 0; band < 3; ++band)
            {
                juce::Path fillPath, strokePath;
                for (int i = 0; i < numPoints; ++i)
                {
                    const float x = (float) i / (float) (numPoints - 1);
                    const float px = area.getX() + x * area.getWidth();
                    const float falloff = bandStart[band] * std::exp (-std::pow (x, 1.28f) * slope * bandSlope[band] * 2.6f);
                    const float y = area.getY() + (1.0f - falloff) * area.getHeight();
                    if (i == 0) { fillPath.startNewSubPath (px, y); strokePath.startNewSubPath (px, y); }
                    else        { fillPath.lineTo (px, y);          strokePath.lineTo (px, y); }
                }
                if (band == 2)
                {
                    juce::Path filled = fillPath;
                    filled.lineTo (area.getRight(), area.getBottom());
                    filled.lineTo (area.getX(), area.getBottom());
                    filled.closeSubPath();
                    juce::ColourGradient fade (colours[2].withAlpha (0.16f), 0.0f, area.getY(),
                                               colours[2].withAlpha (0.0f), 0.0f, area.getBottom(), false);
                    g.setGradientFill (fade);
                    g.fillPath (filled);
                }
                g.setColour (colours[band].withAlpha (0.22f));
                g.strokePath (strokePath, juce::PathStrokeType (5.5f));
                g.setColour (colours[band]);
                g.strokePath (strokePath, juce::PathStrokeType (1.9f));
            }
        }

    private:
        juce::AudioProcessorValueTreeState& apvts;
    };

    //==========================================================================
    // Triangle "Character" selector: cycles Dark / Neutral / Bright.
    class CharacterButton : public juce::Component
    {
    public:
        std::function<void()> onClick;

        void paint (juce::Graphics& g) override
        {
            auto area = getLocalBounds().toFloat().reduced (2.0f);
            const auto centre = area.getCentre();
            const float r = juce::jmin (area.getWidth(), area.getHeight()) * 0.40f;

            juce::Path tri;
            tri.addTriangle (centre.x, centre.y - r,
                             centre.x - r * 0.9f, centre.y + r * 0.75f,
                             centre.x + r * 0.9f, centre.y + r * 0.75f);
            g.setColour (juce::Colour (0xff31e6ff).withAlpha (0.10f));
            g.fillPath (tri);
            g.setColour (juce::Colour (0xff31e6ff).withAlpha (0.35f));
            g.strokePath (tri, juce::PathStrokeType (5.0f, juce::PathStrokeType::mitered, juce::PathStrokeType::rounded));
            g.setColour (juce::Colour (0xffd6fbff));
            g.strokePath (tri, juce::PathStrokeType (1.6f));
        }

        void mouseUp (const juce::MouseEvent& e) override
        {
            if (getLocalBounds().contains (e.getPosition()) && onClick)
                onClick();
        }
    };
}
