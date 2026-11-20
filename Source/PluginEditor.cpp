#include "PluginEditor.h"
#include <cmath>

namespace
{
    // Design-space canvas the whole UI is laid out against.
    constexpr float designW = 1536.0f;
    constexpr float designH = 1024.0f;

    const juce::Colour bgVoid { 0xff050608 };

    juce::Rectangle<float> dr (float x, float y, float w, float h)
    {
        return { x, y, w, h };
    }
}

//==============================================================================
// LevelMeter
//==============================================================================
LevelMeter::LevelMeter()
{
    setInterceptsMouseClicks (false, false);
}

PinkLevelMeter::PinkLevelMeter()
{
    setInterceptsMouseClicks (false, false);
}

void LevelMeter::setLevel (float linearLevel)
{
    const float target = juce::jlimit (0.0f, 1.0f, linearLevel);
    // Fast attack, slower release, gentle peak hold.
    if (target > display)
        display += (target - display) * 0.65f;
    else
        display += (target - display) * 0.18f;

    if (target >= peak)
    {
        peak = target;
        peakHoldSeconds = 1.1f;
    }
    clip = linearLevel >= 1.0f ? 1.0f : juce::jmax (0.0f, clip - 0.02f);
}

void LevelMeter::update (float dt)
{
    if (peakHoldSeconds > 0.0f)
        peakHoldSeconds -= dt;
    else
        peak *= juce::Decibels::decibelsToGain (-12.0f * dt); // 12 dB/s after hold
    repaint();
}

void LevelMeter::paint (juce::Graphics& g)
{
    paintMeter (g, false);
}

void PinkLevelMeter::paint (juce::Graphics& g)
{
    paintMeter (g, true);
}

void LevelMeter::paintMeter (juce::Graphics& g, bool pink)
{
    const float w = static_cast<float> (getWidth()), h = static_cast<float> (getHeight());
    const auto accent = pink ? VoidGUI::pinkHot : VoidGUI::cyanHot;
    const juce::Rectangle<float> glass (4.0f, 2.0f, w - 8.0f, h - 46.0f);
    VoidGUI::drawPanel (g, glass, 10.0f, 0.93f);
    const juce::Rectangle<float> track (w * 0.49f, 45.0f, w * 0.14f, h - 107.0f);
    g.setColour (juce::Colour (0xff010508));
    g.fillRect (track.expanded (3.0f));
    g.setColour (accent.withAlpha (0.10f));
    g.fillRect (track);
    // The ticks and signal share the same dB coordinate mapping, not linear gain.
    for (int db = 0; db >= -60; db -= 3)
    {
        const float y = track.getBottom() - (db + 60.0f) / 60.0f * track.getHeight();
        const bool major = db % 6 == 0;
        g.setColour (VoidGUI::textDim.withAlpha (major ? 0.8f : 0.35f));
        g.drawLine (track.getX() - (major ? 13.0f : 8.0f), y, track.getX() - 4.0f, y, 0.8f);
        if (major)
        {
            g.setFont (VoidGUI::font (400, 12.5f));
            g.drawText (juce::String (db), juce::Rectangle<float> (7.0f, y - 8.0f, track.getX() - 23.0f, 16.0f),
                        juce::Justification::centredRight);
        }
    }
    const float fraction = VoidGUI::meterPosition (display);
    if (fraction > 0.0f)
    {
        const auto fill = track.withTrimmedTop (track.getHeight() * (1.0f - fraction));
        juce::ColourGradient light (accent.withAlpha (0.38f), fill.getX(), fill.getBottom(),
                                    accent, fill.getX(), fill.getY(), false);
        g.setGradientFill (light);
        g.fillRect (fill);
        g.setColour (accent.withAlpha (0.12f));
        g.fillRect (fill.expanded (4.0f, 0.0f));
    }
    if (peak > 0.001f)
    {
        const float y = track.getBottom() - VoidGUI::meterPosition (peak) * track.getHeight();
        g.setColour (juce::Colour (0xffeffbff));
        g.fillRoundedRectangle (track.getX() - 3.0f, y - 1.4f, track.getWidth() + 6.0f, 2.8f, 1.0f);
    }
    g.setColour (clip > 0.02f ? juce::Colour (0xffff546e) : accent.withAlpha (0.6f));
    g.fillEllipse (track.getCentreX() - 3.0f, 16.0f, 6.0f, 6.0f);
    const float db = juce::Decibels::gainToDecibels (display, -60.0f);
    const auto text = display <= 0.001f ? juce::String ("-inf dB") : juce::String (db, 1) + " dB";
    g.setColour (accent);
    g.setFont (VoidGUI::font (400, 22.0f));
    g.drawText (text, juce::Rectangle<float> (0.0f, h - 37.0f, w, 29.0f), juce::Justification::centred);
}

//==============================================================================
// DecayArcSlider
//==============================================================================
DecayArcSlider::DecayArcSlider()
{
    setSliderStyle (juce::Slider::Rotary);
    setRotaryParameters (-1.493f, 1.493f, true);
    setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    setRange (0.1, 20.0, 0.01);
    setSkewFactorFromMidPoint (2.2);
    setValue (4.8, juce::dontSendNotification);
}

void DecayArcSlider::paint (juce::Graphics& g)
{
    g.addTransform (juce::AffineTransform::scale (getWidth() / 420.0f, getHeight() / 248.0f));
    juce::Path dome;
    dome.startNewSubPath (2.0f, 244.0f);
    dome.lineTo (2.0f, 236.0f);
    dome.addCentredArc (210.0f, 236.0f, 208.0f, 232.0f, 0.0f,
                        -juce::MathConstants<float>::halfPi, juce::MathConstants<float>::halfPi, false);
    dome.lineTo (418.0f, 244.0f);
    dome.closeSubPath();
    juce::ColourGradient glass (juce::Colour (0xff293643), 80.0f, 0.0f,
                                juce::Colour (0xff050a10), 330.0f, 244.0f, false);
    glass.addColour (0.35, juce::Colour (0xff0b141e));
    g.setGradientFill (glass);
    g.fillPath (dome);
    g.setColour (juce::Colour (0xff91a9b9).withAlpha (0.7f));
    g.strokePath (dome, juce::PathStrokeType (1.4f));
    g.setColour (juce::Colour (0xff8fcddd).withAlpha (0.12f));
    g.strokePath (dome, juce::PathStrokeType (5.0f));
    const float a0 = -1.49f, a1 = 1.49f;
    juce::Path track;
    track.addCentredArc (210.0f, 232.0f, 190.0f, 210.0f, 0.0f, a0, a1, true);
    g.setColour (juce::Colour (0xff315664));
    g.strokePath (track, juce::PathStrokeType (1.7f));
    const float norm = static_cast<float> ((getValue() - getMinimum()) / (getMaximum() - getMinimum()));
    const float angle = a0 + (a1 - a0) * norm;
    if (norm > 0.0001f)
    {
        juce::Path value;
        value.addCentredArc (210.0f, 232.0f, 190.0f, 210.0f, 0.0f, a0, angle, true);
        VoidGUI::strokeGlow (g, value, VoidGUI::cyanHot, 2.0f);
        const float x = 210.0f + 190.0f * std::sin (angle);
        const float y = 232.0f - 210.0f * std::cos (angle);
        g.setColour (juce::Colours::white);
        g.fillEllipse (x - 2.8f, y - 2.8f, 5.6f, 5.6f);
    }
    VoidGUI::drawTrackedText (g, "DECAY TIME", {100.0f, 78.0f, 220.0f, 21.0f}, 1.7f,
                              VoidGUI::font (400, 15.0f), VoidGUI::textDim);
    g.setFont (VoidGUI::font (300, 57.0f));
    g.setColour (VoidGUI::white);
    g.drawText (juce::String (getValue(), 1) + " s", juce::Rectangle<float> (104.0f, 102.0f, 220.0f, 59.0f),
                juce::Justification::centred);

    // Stylised decay envelope (not an audio analyser). Its time constant follows
    // the actual RT60 value; the readout and drag attachment remain independent.
    const float rate = 4.0f + 5.0f * (1.0f - norm);
    juce::Path envelope, fill;
    constexpr float base = 211.0f;
    for (int i = 0; i <= 180; ++i)
    {
        const float t = i / 180.0f;
        const float x = 30.0f + t * 360.0f;
        const float amp = 155.0f * (1.0f - std::exp (-45.0f * t)) * std::exp (-rate * t);
        const float y = base - amp;
        if (i == 0) { envelope.startNewSubPath (x, y); fill.startNewSubPath (x, y); }
        else { envelope.lineTo (x, y); fill.lineTo (x, y); }
        if (i % 2 == 0)
        {
            g.setColour (VoidGUI::pinkHot.withAlpha (0.55f * (1.0f - t)));
            g.drawLine (x, y, x, base + amp * 0.22f, 0.65f);
        }
    }
    fill.lineTo (390.0f, base); fill.lineTo (30.0f, base); fill.closeSubPath();
    juce::ColourGradient haze (VoidGUI::pink.withAlpha (0.20f), 60.0f, 130.0f,
                               VoidGUI::purple.withAlpha (0.01f), 330.0f, base, false);
    g.setGradientFill (haze); g.fillPath (fill);
    g.setColour (VoidGUI::pinkHot.withAlpha (0.9f));
    g.strokePath (envelope, juce::PathStrokeType (1.1f));
    g.setFont (VoidGUI::font (400, 13.0f));
    g.setColour (VoidGUI::textDim);
    g.drawText ("0.1 s", juce::Rectangle<float> (15.0f, 222.0f, 60.0f, 19.0f), juce::Justification::centredLeft);
    g.drawText ("20 s", juce::Rectangle<float> (345.0f, 222.0f, 60.0f, 19.0f), juce::Justification::centredRight);
}

//==============================================================================

OrbButton::OrbButton()
{
    setInterceptsMouseClicks (true, false);
}

void OrbButton::setOrb (const juce::Image& image, const juce::String& orbName)
{
    orb = image;
    name = orbName;
    repaint();
}

void OrbButton::setSelected (bool shouldBeSelected)
{
    if (selected != shouldBeSelected)
    {
        selected = shouldBeSelected;
        repaint();
    }
}

void OrbButton::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const float size = juce::jmin (b.getWidth() - 4.0f, b.getHeight() - 38.0f);
    const juce::Rectangle<float> r ((b.getWidth() - size) * 0.5f, 3.0f, size, size);
    if (selected || hovered)
    {
        g.setColour ((selected ? VoidGUI::pink : VoidGUI::cyan).withAlpha (selected ? 0.12f : 0.05f));
        g.fillEllipse (r.expanded (2.0f));
    }
    if (orb.isValid())
    {
        juce::Graphics::ScopedSaveState save (g);
        juce::Path clip; clip.addEllipse (r);
        g.reduceClipRegion (clip);
        g.drawImage (orb, r, juce::RectanglePlacement::stretchToFit);
    }
    g.setColour (selected ? VoidGUI::pinkHot : juce::Colour (0xff7293a7).withAlpha (0.65f));
    g.drawEllipse (r, selected ? 1.4f : 0.85f);
    g.setColour (juce::Colours::white.withAlpha (0.20f));
    juce::Path rim;
    rim.addCentredArc (r.getCentreX(), r.getCentreY(), size * 0.48f, size * 0.48f, 0.0f, -1.2f, 0.35f, true);
    g.strokePath (rim, juce::PathStrokeType (1.0f));
    if (selected)
    {
        g.setColour (VoidGUI::pinkHot);
        g.fillEllipse (b.getCentreX() - 2.5f, r.getBottom() + 6.0f, 5.0f, 5.0f);
    }
    VoidGUI::drawTrackedText (g, name.toUpperCase(), {0.0f, b.getHeight() - 23.0f, b.getWidth(), 20.0f},
                              1.8f, VoidGUI::font (400, 14.0f), selected ? VoidGUI::pinkHot : VoidGUI::textDim);
}

//==============================================================================
AdvancedCell::AdvancedCell (VoidChamberReverbAudioProcessor& processorToUse, const char* parameterID,
                            Icon iconToUse, juce::String labelToUse, bool isCharacterCell)
    : processor (processorToUse), paramID (parameterID), icon (iconToUse),
      label (std::move (labelToUse)), isCharacter (isCharacterCell)
{
    parameter = processor.getValueTreeState().getParameter (paramID);
    setInterceptsMouseClicks (true, false);
}

juce::String AdvancedCell::valueText() const
{
    if (parameter == nullptr)
        return {};
    if (isCharacter)
    {
        static const char* names[] = { "DARK", "NEUTRAL", "BRIGHT" };
        const int idx = juce::jlimit (0, 2, juce::roundToInt (parameter->getValue() * 2.0f));
        return names[idx];
    }
    const float plain = parameter->convertFrom0to1 (parameter->getValue());
    return juce::String (juce::roundToInt (plain)) + "%";
}

void AdvancedCell::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const auto colour = isCharacter ? VoidGUI::pinkHot : VoidGUI::cyanHot;
    const float square = juce::jmin (b.getWidth() - 2.0f, b.getHeight() - 2.0f);
    const juce::Rectangle<float> tile ((b.getWidth() - square) * 0.5f, 1.0f, square, b.getHeight() - 3.0f);
    VoidGUI::drawPanel (g, tile, 10.0f, hovered ? 0.98f : 0.88f);
    g.setColour (colour.withAlpha (hovered ? 0.42f : 0.16f));
    g.drawRoundedRectangle (tile.reduced (1.6f), 9.0f, 0.85f);
    const auto iconArea = juce::Rectangle<float> (tile.getX() + 19.0f, 17.0f, tile.getWidth() - 38.0f, 53.0f);
    paintIcon (g, iconArea);
    VoidGUI::drawTrackedText (g, label.toUpperCase(), {0.0f, 84.0f, b.getWidth(), 20.0f}, 0.8f,
                              VoidGUI::font (400, 13.5f), VoidGUI::textDim);
    g.setFont (VoidGUI::font (400, 22.0f));
    g.setColour (colour);
    g.drawText (valueText(), juce::Rectangle<float> (0.0f, 118.0f, b.getWidth(), 28.0f), juce::Justification::centred);
}

void AdvancedCell::paintIcon (juce::Graphics& g, juce::Rectangle<float> r)
{
    const float cx = r.getCentreX(), cy = r.getCentreY();
    const float s = r.getWidth();
    const juce::Colour ic = VoidGUI::white.withAlpha (0.92f);
    g.setColour (ic);
    const float lw = juce::jmax (0.8f, s * 0.022f);

    switch (icon)
    {
        case Icon::diffusion:
        {
            g.drawLine (r.getX(), cy - s * 0.34f, r.getX(), cy + s * 0.34f, lw);
            g.drawLine (r.getRight(), cy - s * 0.34f, r.getRight(), cy + s * 0.34f, lw);
            juce::Path wave;
            for (int i = 0; i <= 48; ++i)
            {
                const float t = (float) i / 48.0f;
                const float x = r.getX() + s * 0.16f + t * s * 0.68f;
                const float env = std::sin (t * juce::MathConstants<float>::pi);
                const float y = cy + std::sin (t * 3.4f * juce::MathConstants<float>::twoPi) * env * s * 0.26f;
                if (i == 0) wave.startNewSubPath (x, y); else wave.lineTo (x, y);
            }
            g.strokePath (wave, juce::PathStrokeType (lw * 0.9f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            break;
        }
        case Icon::width:
        {
            const float pad = s * 0.05f;
            g.drawLine (r.getX() + pad, cy - s * 0.32f, r.getX() + pad, cy + s * 0.32f, lw);
            g.drawLine (r.getRight() - pad, cy - s * 0.32f, r.getRight() - pad, cy + s * 0.32f, lw);
            g.drawLine (r.getX() + pad, cy, r.getRight() - pad, cy, lw * 0.9f);
            // arrowheads
            juce::Path leftArrow;
            leftArrow.addTriangle (r.getX() + pad - 0.5f, cy, r.getX() + pad + s * 0.22f, cy - s * 0.17f, r.getX() + pad + s * 0.22f, cy + s * 0.17f);
            juce::Path rightArrow;
            rightArrow.addTriangle (r.getRight() - pad + 0.5f, cy, r.getRight() - pad - s * 0.22f, cy - s * 0.17f, r.getRight() - pad - s * 0.22f, cy + s * 0.17f);
            g.strokePath (leftArrow, juce::PathStrokeType (lw));
            g.strokePath (rightArrow, juce::PathStrokeType (lw));
            break;
        }
        case Icon::air:
        {
            juce::Path shelf;
            shelf.startNewSubPath (r.getX(), cy + s * 0.2f);
            shelf.cubicTo (cx + s * 0.1f, cy + s * 0.2f, cx - s * 0.12f, cy - s * 0.22f,
                           r.getRight(), cy - s * 0.22f);
            g.setColour (VoidGUI::cyanHot);
            g.strokePath (shelf, juce::PathStrokeType (lw));
            break;
        }
        case Icon::character:
        {
            juce::Path tri;
            const float ty = s * 0.42f;
            tri.startNewSubPath (cx, cy - ty * 0.62f);
            tri.lineTo (cx - ty * 0.78f, cy + ty * 0.52f);
            tri.lineTo (cx + ty * 0.78f, cy + ty * 0.52f);
            tri.closeSubPath();
            g.strokePath (tri, juce::PathStrokeType (lw, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            break;
        }
    }
}

void AdvancedCell::mouseEnter (const juce::MouseEvent&) { hovered = true; repaint(); }
void AdvancedCell::mouseExit  (const juce::MouseEvent&) { hovered = false; repaint(); }

void AdvancedCell::mouseDown (const juce::MouseEvent&)
{
    dragOriginValue = parameter != nullptr ? parameter->getValue() : 0.0f;
}

void AdvancedCell::mouseDrag (const juce::MouseEvent& e)
{
    if (isCharacter || parameter == nullptr)
        return;
    const float dy = static_cast<float> (e.getMouseDownY() - e.y);
    const float newValue = juce::jlimit (0.0f, 1.0f, dragOriginValue + dy * 0.0042f);
    parameter->setValueNotifyingHost (newValue);
    repaint();
}

void AdvancedCell::mouseDoubleClick (const juce::MouseEvent&)
{
    if (parameter == nullptr || isCharacter)
        return;
    float def = 0.0f;
    if (paramID == VoidChamberReverbAudioProcessor::diffusionID) def = 0.68f;
    else if (paramID == VoidChamberReverbAudioProcessor::widthID) def = 0.60f;
    else if (paramID == VoidChamberReverbAudioProcessor::airID)   def = 0.35f;
    parameter->setValueNotifyingHost (def);
    repaint();
}

void AdvancedCell::mouseUp (const juce::MouseEvent& e)
{
    if (! isCharacter || parameter == nullptr)
        return;
    if (std::abs (e.x - e.getMouseDownX()) < 4 && std::abs (e.y - e.getMouseDownY()) < 4)
    {
        const int current = juce::roundToInt (parameter->getValue() * 2.0f);
        const int next = (current + 1) % 3;
        parameter->setValueNotifyingHost (next * 0.5f);
        repaint();
    }
}

void AdvancedCell::syncFromParameter()
{
    repaint();
}

//==============================================================================
// ChevronButton
//==============================================================================
ChevronButton::ChevronButton (Direction directionToUse) : direction (directionToUse)
{
    setInterceptsMouseClicks (true, false);
}

void ChevronButton::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const auto centre = b.getCentre();

    if (hovered)
    {
        g.setColour (juce::Colours::white.withAlpha (0.05f));
        g.fillRoundedRectangle (b, b.getHeight() * 0.3f);
    }

    const juce::Colour glyph = hovered ? VoidGUI::white : VoidGUI::textDim;
    g.setColour (glyph);

    if (direction == Direction::menu)
    {
        const float lw = juce::jmax (1.6f, b.getHeight() * 0.045f);
        const float half = b.getWidth() * 0.21f;
        for (int i = -1; i <= 1; ++i)
        {
            const float y = centre.y + static_cast<float> (i) * b.getHeight() * 0.17f;
            g.drawLine (centre.x - half, y, centre.x + half, y, lw);
        }
        return;
    }

    const float size = juce::jmin (b.getWidth(), b.getHeight());
    const float t = size * 0.30f;
    juce::Path chevron;
    if (direction == Direction::left)
    {
        chevron.startNewSubPath (centre.x + t * 0.55f, centre.y - t);
        chevron.lineTo (centre.x - t * 0.55f, centre.y);
        chevron.lineTo (centre.x + t * 0.55f, centre.y + t);
    }
    else
    {
        chevron.startNewSubPath (centre.x - t * 0.55f, centre.y - t);
        chevron.lineTo (centre.x + t * 0.55f, centre.y);
        chevron.lineTo (centre.x - t * 0.55f, centre.y + t);
    }
    g.strokePath (chevron, juce::PathStrokeType (juce::jmax (2.0f, size * 0.075f),
                                                 juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));
}

//==============================================================================
// FrequencyDecayGraph — analytical per-band decay slopes, live from parameters
//==============================================================================
FrequencyDecayGraph::FrequencyDecayGraph (juce::AudioProcessorValueTreeState& stateToRead) : state (stateToRead)
{
    setInterceptsMouseClicks (false, false);
}

void FrequencyDecayGraph::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    if (bounds.isEmpty())
        return;

    const float damping   = state.getRawParameterValue (VoidChamberReverbAudioProcessor::dampingID)->load();
    const float air       = state.getRawParameterValue (VoidChamberReverbAudioProcessor::airID)->load() * 0.01f;
    const int   character = static_cast<int> (std::lround (state.getRawParameterValue (VoidChamberReverbAudioProcessor::characterID)->load()));

    struct Band { juce::Colour colour; float slope; };
    // dB of absorption per octave, low -> high. Air lifts the top end; damping
    // progressively pulls it down; character tilts the whole fan.
    const float tilt = character == 0 ? 0.85f : (character == 2 ? -0.7f : 0.0f);
    std::array<Band, 3> bands {{
        { VoidGUI::pink,   0.55f },
        { VoidGUI::purple, 1.15f + damping * 2.6f + tilt * 0.6f },
        { VoidGUI::cyan,   2.05f + damping * 6.4f + tilt * 1.35f - air * 1.6f },
    }};

    const float left   = bounds.getX() + 31.0f;
    const float right  = bounds.getRight() - 10.0f;
    const float top    = bounds.getY() + 6.0f;
    const float bottom = bounds.getBottom() - 24.0f;
    const float height = bottom - top;

    const auto freqToX = [&] (float freq)
    {
        const float t = (std::log10 (freq) - std::log10 (20.0f)) / (std::log10 (20000.0f) - std::log10 (20.0f));
        return left + t * (right - left);
    };
    // Analytical absorption display, not a fabricated live spectrum.
    // 60 dB headroom and readable labelled calibration.
    const float pxPerDb = height / 60.0f;
    const auto lossToY = [&] (float db) { return top + juce::jlimit (0.0f, 60.0f, db) * pxPerDb; };

    // Grid: verticals + one horizontal midline
    g.setColour (juce::Colours::white.withAlpha (0.055f));
    for (float f : { 20.0f, 100.0f, 1000.0f, 10000.0f, 20000.0f })
        g.drawLine (freqToX (f), top, freqToX (f), bottom, 1.0f);
    g.setFont (VoidGUI::font (400, 11.0f));
    for (int db = 0; db >= -60; db -= 12)
    {
        const float y = top - db * pxPerDb;
        g.setColour (juce::Colours::white.withAlpha (0.055f));
        g.drawLine (left, y, right, y, 0.7f);
        g.setColour (VoidGUI::textFaint);
        g.drawText (juce::String (db), juce::Rectangle<float> (0.0f, y - 7.0f, 25.0f, 14.0f),
                    juce::Justification::centredRight);
    }

    // X labels
    g.setFont (VoidGUI::font (400, 12.0f));
    g.setColour (VoidGUI::textFaint);
    const struct { float f; const char* label; } labels[5] = {
        { 20.0f, "20" }, { 100.0f, "100" }, { 1000.0f, "1K" }, { 10000.0f, "10K" }, { 20000.0f, "20K" }
    };
    for (const auto& entry : labels)
    {
        const float x = freqToX (entry.f);
        g.drawText (entry.label,
                    juce::Rectangle<float> (x - 30.0f, bottom + 5.0f, 60.0f, 16.0f),
                    juce::Justification::centred);
    }

    // Curves: integrate slope over log2 octaves from 20 Hz.
    for (const auto& band : bands)
    {
        juce::Path path;
        juce::Path fill;
        for (int i = 0; i <= 96; ++i)
        {
            const float t = (float) i / 96.0f;
            const float f = 20.0f * std::pow (1000.0f, t);   // 20 Hz .. 20 kHz
            const float octaves = std::log2 (f / 20.0f);
            const float db = band.slope * octaves;
            const float x = freqToX (f);
            const float y = lossToY (db);
            if (db > 60.0f)
            {
                // Terminate at the floor instead of drawing a clamped flat tail.
                path.lineTo (x, bottom + 1.0f);
                fill.lineTo (x, bottom + 1.0f);
                break;
            }
            if (i == 0) { path.startNewSubPath (x, y); fill.startNewSubPath (x, y); }
            else        { path.lineTo (x, y);          fill.lineTo (x, y); }
        }
        fill.lineTo (right, bottom);
        fill.lineTo (left, bottom);
        fill.closeSubPath();

        g.setColour (band.colour.withAlpha (0.025f));
        g.fillPath (fill);
        g.setColour (band.colour.withAlpha (0.12f));
        g.strokePath (path, juce::PathStrokeType (4.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (band.colour);
        g.strokePath (path, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
}

//==============================================================================
// PresetBrowser
//==============================================================================
PresetBrowser::PresetBrowser (VoidChamberReverbAudioProcessor& processorToUse)
    : processor (processorToUse)
{
    setInterceptsMouseClicks (true, true);
    setOpaque (false);
    setWantsKeyboardFocus (true);
    setVisible (false);
}

void PresetBrowser::open (int initialPresetIndex)
{
    activeCategory = VoidChamberReverbAudioProcessor::getPresetCategoryIndex (initialPresetIndex);
    hoveredPreset = -1;
    hoveredCategory = -1;
    hoveredClose = false;
    layout();
    setVisible (true);
    toFront (true);
    grabKeyboardFocus();
    repaint();
}

void PresetBrowser::layout()
{
    tabs.clear();
    cells.clear();

    const auto b = getLocalBounds().toFloat();
    const float panelW = juce::jmin (1120.0f, b.getWidth() - 140.0f);
    const float panelH = juce::jmin (640.0f, b.getHeight() - 90.0f);
    const auto panel = juce::Rectangle<float> (b.getCentreX() - panelW * 0.5f, b.getCentreY() - panelH * 0.5f,
                                               panelW, panelH);

    closeButton = { panel.getRight() - 62.0f, panel.getY() + 22.0f, 40.0f, 40.0f };

    // Tabs row
    const float tabAreaX = panel.getX() + 36.0f;
    const float tabAreaW = panel.getWidth() - 72.0f;
    const float tabGap = 12.0f;
    const float tabW = (tabAreaW - tabGap * 4.0f) / 5.0f;
    const float tabY = panel.getY() + 108.0f;
    const float tabH = 128.0f;
    for (int i = 0; i < 5; ++i)
        tabs.push_back ({ { tabAreaX + static_cast<float> (i) * (tabW + tabGap), tabY, tabW, tabH }, i });

    // Preset grid: 2 columns x 5 rows
    const float gridX = panel.getX() + 36.0f;
    const float gridW = panel.getWidth() - 72.0f;
    const float cellGap = 12.0f;
    const float cellW = (gridW - cellGap) * 0.5f;
    const float cellH = 56.0f;
    const float gridY = tabY + tabH + 18.0f;
    int i = 0;
    for (int row = 0; row < 5; ++row)
    {
        for (int col = 0; col < 2; ++col)
        {
            cells.push_back ({ { gridX + static_cast<float> (col) * (cellW + cellGap),
                                 gridY + static_cast<float> (row) * (cellH + 8.0f),
                                 cellW, cellH },
                               activeCategory * VoidChamberReverbAudioProcessor::presetsPerCategory + i });
            ++i;
        }
    }
}

void PresetBrowser::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0x04060a).withAlpha (0.82f));
    g.fillRect (b);

    const float panelW = juce::jmin (1120.0f, b.getWidth() - 140.0f);
    const float panelH = juce::jmin (640.0f, b.getHeight() - 90.0f);
    const auto panel = juce::Rectangle<float> (b.getCentreX() - panelW * 0.5f, b.getCentreY() - panelH * 0.5f,
                                               panelW, panelH);

    // Panel body
    juce::Path panelPath;
    panelPath.addRoundedRectangle (panel, 20.0f);
    g.setColour (juce::Colour (0xff0d1017).withAlpha (0.97f));
    g.fillPath (panelPath);
    g.setColour (VoidGUI::panelInner.withAlpha (0.8f));
    g.strokePath (panelPath, juce::PathStrokeType (1.5f));
    g.setColour (VoidGUI::pink.withAlpha (0.10f));
    g.strokePath (panelPath, juce::PathStrokeType (6.0f));

    // Header
    g.setFont (VoidGUI::font (600, 23.0f));
    g.setColour (VoidGUI::white);
    g.drawSingleLineText ("P R E S E T   L I B R A R Y", juce::roundToInt (panel.getX() + 40.0f),
                          juce::roundToInt (panel.getY() + 52.0f));
    g.setFont (VoidGUI::font (400, 11.5f));
    g.setColour (VoidGUI::textFaint);
    g.drawSingleLineText ("F I F T Y   F A C T O R Y   R E V E R B S   I N   F I V E   C O L L E C T I O N S",
                          juce::roundToInt (panel.getX() + 40.0f),
                          juce::roundToInt (panel.getY() + 78.0f));

    // Close button
    {
        juce::Path closeIcon;
        const auto c = closeButton;
        const float pad = c.getWidth() * 0.30f;
        closeIcon.startNewSubPath (c.getX() + pad, c.getY() + pad);
        closeIcon.lineTo (c.getRight() - pad, c.getBottom() - pad);
        closeIcon.startNewSubPath (c.getRight() - pad, c.getY() + pad);
        closeIcon.lineTo (c.getX() + pad, c.getBottom() - pad);
        g.setColour (hoveredClose ? VoidGUI::pink : VoidGUI::textDim);
        g.strokePath (closeIcon, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Category tabs
    for (const auto& tab : tabs)
    {
        const bool active = tab.categoryIndex == activeCategory;
        const bool hover = tab.categoryIndex == hoveredCategory;
        const auto r = tab.bounds;

        juce::Path tabPath;
        tabPath.addRoundedRectangle (r, 14.0f);
        if (active)
        {
            g.setColour (VoidGUI::pink.withAlpha (0.12f));
            g.fillPath (tabPath);
            g.setColour (VoidGUI::pink.withAlpha (0.45f));
            g.strokePath (tabPath, juce::PathStrokeType (5.0f));
            g.setColour (VoidGUI::pink);
            g.strokePath (tabPath, juce::PathStrokeType (1.5f));
        }
        else
        {
            g.setColour (juce::Colour (0xff12151d).withAlpha (0.9f));
            g.fillPath (tabPath);
            g.setColour (hover ? VoidGUI::panelInner : VoidGUI::panelStroke);
            g.strokePath (tabPath, juce::PathStrokeType (1.1f));
        }

        const float orbSize = 58.0f;
        const auto orbRect = juce::Rectangle<float> (r.getCentreX() - orbSize * 0.5f, r.getY() + 12.0f, orbSize, orbSize);
        const char* data = nullptr;
        int dataSize = 0;
        switch (tab.categoryIndex)
        {
            case 0: data = BinaryData::spacevoid_jpg;    dataSize = BinaryData::spacevoid_jpgSize;    break;
            case 1: data = BinaryData::spacehall_jpg;    dataSize = BinaryData::spacehall_jpgSize;    break;
            case 2: data = BinaryData::spaceplate_jpg;   dataSize = BinaryData::spaceplate_jpgSize;   break;
            case 3: data = BinaryData::spacecavern_jpg;  dataSize = BinaryData::spacecavern_jpgSize;  break;
            case 4: data = BinaryData::spaceambient_jpg; dataSize = BinaryData::spaceambient_jpgSize; break;
            default: break;
        }
        if (data != nullptr)
        {
            const auto img = juce::ImageCache::getFromMemory (data, dataSize);
            juce::Path clip;
            clip.addEllipse (orbRect.reduced (1.0f));
            g.saveState();
            g.reduceClipRegion (clip);
            g.drawImage (img, orbRect, juce::RectanglePlacement::centred | juce::RectanglePlacement::fillDestination);
            g.restoreState();
            g.setColour (juce::Colours::black.withAlpha (0.5f));
            g.drawEllipse (orbRect, 2.0f);
        }

        VoidGUI::drawTrackedText (g, VoidChamberReverbAudioProcessor::getPresetCategoryName (tab.categoryIndex).toUpperCase(),
                                  { r.getX(), r.getBottom() - 44.0f, r.getWidth(), 20.0f }, 3.4f,
                                  VoidGUI::font (600, 15.0f),
                                  active ? VoidGUI::pinkHot : VoidGUI::textDim);
        g.setFont (VoidGUI::font (400, 10.0f));
        g.setColour (VoidGUI::textFaint);
        g.drawText ("1 0   P R E S E T S",
                    juce::Rectangle<float> (r.getX(), r.getBottom() - 20.0f, r.getWidth(), 14.0f),
                    juce::Justification::centred);
    }

    // Preset cells
    const int currentIndex = processor.getCurrentProgram();
    for (const auto& cell : cells)
    {
        const bool current = cell.presetIndex == currentIndex;
        const bool hover = cell.presetIndex == hoveredPreset;
        const auto r = cell.bounds;

        juce::Path cellPath;
        cellPath.addRoundedRectangle (r, 9.0f);
        if (current)
            g.setColour (VoidGUI::pink.withAlpha (0.13f));
        else if (hover)
            g.setColour (juce::Colours::white.withAlpha (0.055f));
        else
            g.setColour (juce::Colour (0xff12151d).withAlpha (0.85f));
        g.fillPath (cellPath);
        g.setColour (current ? VoidGUI::pink.withAlpha (0.9f)
                             : (hover ? VoidGUI::panelInner : VoidGUI::panelStroke));
        g.strokePath (cellPath, juce::PathStrokeType (current ? 1.6f : 1.0f));

        if (current)
        {
            g.setColour (VoidGUI::pink);
            g.fillRoundedRectangle (r.getX() + 3.0f, r.getY() + 8.0f, 3.0f, r.getHeight() - 16.0f, 1.5f);
        }

        const juce::String indexText = juce::String::formatted ("%02d", cell.presetIndex + 1);
        g.setFont (VoidGUI::font (300, 12.0f));
        g.setColour (VoidGUI::textFaint);
        g.drawSingleLineText (indexText, juce::roundToInt (r.getX() + 16.0f), juce::roundToInt (r.getCentreY() + 4.5f));

        g.setFont (VoidGUI::font (500, 16.5f));
        g.setColour (current ? VoidGUI::white : juce::Colour (0xffd6dae4));
        g.drawSingleLineText (VoidChamberReverbAudioProcessor::getPresetName (cell.presetIndex),
                              juce::roundToInt (r.getX() + 44.0f),
                              juce::roundToInt (r.getCentreY() + 6.0f));
    }

    // Footer hint
    g.setFont (VoidGUI::font (400, 10.5f));
    g.setColour (VoidGUI::textFaint);
    g.drawText ("C L I C K   T O   L O A D        E S C   T O   C L O S E",
                juce::Rectangle<float> (panel.getX(), panel.getBottom() - 18.0f, panel.getWidth(), 20.0f),
                juce::Justification::centred);
}

void PresetBrowser::mouseDown (const juce::MouseEvent& e)
{
    const auto pos = e.position;

    if (closeButton.contains (pos))
    {
        setVisible (false);
        if (onRequestClose) onRequestClose();
        return;
    }

    for (const auto& tab : tabs)
        if (tab.bounds.contains (pos))
        {
            if (activeCategory != tab.categoryIndex)
            {
                activeCategory = tab.categoryIndex;
                layout();
                repaint();
            }
            return;
        }

    for (const auto& cell : cells)
        if (cell.bounds.contains (pos))
        {
            processor.loadPreset (cell.presetIndex);
            setVisible (false);
            if (onRequestClose) onRequestClose();
            return;
        }

    // Click on the dim backdrop closes the overlay.
    const auto b = getLocalBounds().toFloat();
    const float panelW = juce::jmin (1120.0f, b.getWidth() - 140.0f);
    const float panelH = juce::jmin (640.0f, b.getHeight() - 90.0f);
    const auto panel = juce::Rectangle<float> (b.getCentreX() - panelW * 0.5f, b.getCentreY() - panelH * 0.5f,
                                               panelW, panelH);
    if (! panel.contains (pos))
    {
        setVisible (false);
        if (onRequestClose) onRequestClose();
    }
}

bool PresetBrowser::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey)
    {
        setVisible (false);
        if (onRequestClose) onRequestClose();
        return true;
    }
    return false;
}

void PresetBrowser::mouseMove (const juce::MouseEvent& e)
{
    const auto pos = e.position;
    const int oldHoverPreset = hoveredPreset;
    const int oldHoverCategory = hoveredCategory;
    const bool oldHoverClose = hoveredClose;

    hoveredPreset = -1;
    for (const auto& cell : cells)
        if (cell.bounds.contains (pos)) { hoveredPreset = cell.presetIndex; break; }

    hoveredCategory = -1;
    for (const auto& tab : tabs)
        if (tab.bounds.contains (pos)) { hoveredCategory = tab.categoryIndex; break; }

    hoveredClose = closeButton.contains (pos);

    if (oldHoverPreset != hoveredPreset || oldHoverCategory != hoveredCategory || oldHoverClose != hoveredClose)
        repaint();
}

void PresetBrowser::mouseExit (const juce::MouseEvent&)
{
    if (hoveredPreset != -1 || hoveredCategory != -1 || hoveredClose)
    {
        hoveredPreset = -1;
        hoveredCategory = -1;
        hoveredClose = false;
        repaint();
    }
}

//==============================================================================
// Editor
//==============================================================================
VoidChamberReverbAudioProcessorEditor::VoidChamberReverbAudioProcessorEditor (VoidChamberReverbAudioProcessor& processorToEdit)
    : juce::AudioProcessorEditor (&processorToEdit),
      processor (processorToEdit),
      freqGraph (processor.getValueTreeState()),
      diffusionCell (processor, VoidChamberReverbAudioProcessor::diffusionID, AdvancedCell::Icon::diffusion, "Diffusion"),
      widthCell     (processor, VoidChamberReverbAudioProcessor::widthID,     AdvancedCell::Icon::width,     "Width"),
      airCell       (processor, VoidChamberReverbAudioProcessor::airID,       AdvancedCell::Icon::air,       "Air"),
      characterCell (processor, VoidChamberReverbAudioProcessor::characterID, AdvancedCell::Icon::character, "Character", true)
{
    setOpaque (true);

    background = juce::ImageCache::getFromMemory (BinaryData::voidbackground_jpg, BinaryData::voidbackground_jpgSize);
    static const char* orbFiles[5] = { BinaryData::spacevoid_jpg, BinaryData::spacehall_jpg, BinaryData::spaceplate_jpg,
                                       BinaryData::spacecavern_jpg, BinaryData::spaceambient_jpg };
    static const int orbSizes[5] = { BinaryData::spacevoid_jpgSize, BinaryData::spacehall_jpgSize, BinaryData::spaceplate_jpgSize,
                                     BinaryData::spacecavern_jpgSize, BinaryData::spaceambient_jpgSize };
    const char* orbNames[5] = { "Void", "Hall", "Plate", "Cavern", "Ambient" };
    for (int i = 0; i < 5; ++i)
    {
        orbImages[i] = juce::ImageCache::getFromMemory (orbFiles[i], orbSizes[i]);
        orbs[i].setOrb (orbImages[i], orbNames[i]);
        orbs[i].onClick = [this, i]
        {
            if (auto* p = processor.getValueTreeState().getParameter (VoidChamberReverbAudioProcessor::spaceTypeID))
                p->setValueNotifyingHost (p->convertTo0to1 (static_cast<float> (i)));
        };
    }

    auto& state = processor.getValueTreeState();

    const auto setupKnob = [this] (juce::Slider& slider, VoidKnobLookAndFeel& feel)
    {
        slider.setLookAndFeel (&feel);
        slider.setSliderStyle (juce::Slider::Rotary);
        slider.setRotaryParameters (-2.3561945f, 2.3561945f, true); // +/-135 deg
        slider.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
        slider.setPopupDisplayEnabled (false, false, this);
        addAndMakeVisible (slider);
    };
    setupKnob (sizeSlider,       cyanFeel);
    setupKnob (preDelaySlider,   cyanFeel);
    setupKnob (dampingSlider,    cyanFeel);
    setupKnob (modulationSlider, cyanFeel);
    setupKnob (mixSlider,        pinkFeel);

    sizeAttachment       = std::make_unique<juce::SliderParameterAttachment> (*state.getParameter (VoidChamberReverbAudioProcessor::sizeID),       sizeSlider,       nullptr);
    preDelayAttachment   = std::make_unique<juce::SliderParameterAttachment> (*state.getParameter (VoidChamberReverbAudioProcessor::preDelayID),   preDelaySlider,   nullptr);
    dampingAttachment    = std::make_unique<juce::SliderParameterAttachment> (*state.getParameter (VoidChamberReverbAudioProcessor::dampingID),    dampingSlider,    nullptr);
    modulationAttachment = std::make_unique<juce::SliderParameterAttachment> (*state.getParameter (VoidChamberReverbAudioProcessor::modulationID), modulationSlider, nullptr);
    mixAttachment        = std::make_unique<juce::SliderParameterAttachment> (*state.getParameter (VoidChamberReverbAudioProcessor::mixID),        mixSlider,        nullptr);

    addAndMakeVisible (decaySlider);
    decayAttachment = std::make_unique<juce::SliderParameterAttachment> (*state.getParameter (VoidChamberReverbAudioProcessor::decayID), decaySlider, nullptr);

    addAndMakeVisible (inputMeter);
    addAndMakeVisible (outputMeter);

    monoButton.setLookAndFeel (&toggleFeel);
    stereoButton.setLookAndFeel (&toggleFeel);
    monoButton.setClickingTogglesState (false);
    stereoButton.setClickingTogglesState (false);
    monoButton.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    stereoButton.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    const auto setChannelMode = [this] (int index)
    {
        if (auto* p = processor.getValueTreeState().getParameter (VoidChamberReverbAudioProcessor::channelModeID))
            p->setValueNotifyingHost (p->convertTo0to1 (static_cast<float> (index)));
    };
    monoButton.onClick   = [setChannelMode] { setChannelMode (1); };
    stereoButton.onClick = [setChannelMode] { setChannelMode (0); };
    addAndMakeVisible (monoButton);
    addAndMakeVisible (stereoButton);

    for (auto& orb : orbs)
        addAndMakeVisible (orb);

    addAndMakeVisible (freqGraph);
    addAndMakeVisible (diffusionCell);
    addAndMakeVisible (widthCell);
    addAndMakeVisible (airCell);
    addAndMakeVisible (characterCell);

    presetPrev.onClick = [this] { processor.loadPreviousPreset(); };
    presetNext.onClick = [this] { processor.loadNextPreset(); };
    menuButton.onClick = [this] { if (browser) browser->open (processor.getCurrentProgram()); };
    addAndMakeVisible (presetPrev);
    addAndMakeVisible (presetNext);
    addAndMakeVisible (menuButton);

    presetNameButton.setLookAndFeel (&pillFeel);
    presetNameButton.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    presetNameButton.setColour (juce::TextButton::textColourOffId, VoidGUI::white);
    presetNameButton.setTooltip ("Open the preset library");
    presetNameButton.onClick = [this] { if (browser) browser->open (processor.getCurrentProgram()); };
    addAndMakeVisible (presetNameButton);

    browser = std::make_unique<PresetBrowser> (processor);
    browser->onRequestClose = [this] { repaint(); };
    addAndMakeVisible (browser.get());
    browser->setVisible (false);

    updateCachedTexts();
    presetNameButton.setButtonText (presetNameText);
        presetNameButton.setTooltip (presetNameText + " — Open preset library");

    setResizable (true, true);
    setResizeLimits (960, 640, 3072, 2048);
    if (auto* constrainer = getConstrainer())
        constrainer->setFixedAspectRatio (designW / designH);
    setSize (1536, 1024);

    startTimerHz (30);
}

VoidChamberReverbAudioProcessorEditor::~VoidChamberReverbAudioProcessorEditor()
{
    sizeSlider.setLookAndFeel (nullptr);
    preDelaySlider.setLookAndFeel (nullptr);
    dampingSlider.setLookAndFeel (nullptr);
    modulationSlider.setLookAndFeel (nullptr);
    mixSlider.setLookAndFeel (nullptr);
    monoButton.setLookAndFeel (nullptr);
    stereoButton.setLookAndFeel (nullptr);
    presetNameButton.setLookAndFeel (nullptr);
    setLookAndFeel (nullptr);
}

void VoidChamberReverbAudioProcessorEditor::updateCachedTexts()
{
    auto& state = processor.getValueTreeState();
    const auto raw = [&state] (const char* id) { return state.getRawParameterValue ((id))->load(); };

    const auto pct = [] (float norm, float minV, float maxV)
    {
        return juce::String (juce::roundToInt (juce::jmap (norm, 0.0f, 1.0f, minV, maxV)));
    };

    const juce::String newSize       = juce::String (juce::roundToInt (raw (VoidChamberReverbAudioProcessor::sizeID) * 100.0f)) + "%";
    const juce::String newPreDelay   = juce::String (juce::roundToInt (raw (VoidChamberReverbAudioProcessor::preDelayID))) + " ms";
    const juce::String newDamping    = juce::String (juce::roundToInt (raw (VoidChamberReverbAudioProcessor::dampingID) * 100.0f)) + "%";
    const juce::String newModulation = juce::String (juce::roundToInt (raw (VoidChamberReverbAudioProcessor::modulationID) * 100.0f)) + "%";
    const juce::String newMix        = juce::String (juce::roundToInt (raw (VoidChamberReverbAudioProcessor::mixID) * 100.0f)) + "%";
    const juce::String newPreset     = processor.getCurrentPresetName();
    juce::ignoreUnused (pct);

    bool changed = false;
    const auto check = [&changed] (juce::String& slot, const juce::String& value)
    {
        if (slot != value) { slot = value; changed = true; }
    };
    check (sizeText, newSize);
    check (preDelayText, newPreDelay);
    check (dampingText, newDamping);
    check (modulationText, newModulation);
    check (mixText, newMix);
    check (presetNameText, newPreset);

    if (changed)
    {
        presetNameButton.setButtonText (presetNameText);
        presetNameButton.setTooltip (presetNameText + " — Open preset library");
        repaint();
    }
}

void VoidChamberReverbAudioProcessorEditor::paintPanels (juce::Graphics& g)
{
    VoidGUI::drawPanel (g, dr (39, 100, 170, 498), 15, 0.76f);
    VoidGUI::drawPanel (g, dr (1325, 100, 172, 619), 15, 0.76f);
    VoidGUI::drawPanel (g, dr (238, 523, 304, 196), 16, 0.9f);
    VoidGUI::drawPanel (g, dr (983, 523, 315, 196), 16, 0.9f);
    VoidGUI::drawPanel (g, dr (32, 742, 515, 222), 17, 0.91f);
    VoidGUI::drawPanel (g, dr (557, 742, 411, 222), 17, 0.91f);
    VoidGUI::drawPanel (g, dr (973, 742, 525, 222), 17, 0.91f);
}

void VoidChamberReverbAudioProcessorEditor::paintFrame (juce::Graphics& g)
{
    juce::ColourGradient metal (juce::Colour (0xff91acba), 0, 0,
                                juce::Colour (0xff303743), designW, designH, false);
    metal.addColour (0.15, juce::Colour (0xff25313c));
    metal.addColour (0.5, juce::Colour (0xff657786));
    metal.addColour (0.8, juce::Colour (0xff25313c));
    g.setGradientFill (metal);
    g.drawRoundedRectangle (5, 5, designW - 10, designH - 10, 24, 2.3f);
    g.setColour (juce::Colour (0xff708493).withAlpha (0.55f));
    g.drawRoundedRectangle (12, 12, designW - 24, designH - 24, 20, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.11f));
    g.drawLine (35, 15, designW - 35, 15, 1.0f);
}

void VoidChamberReverbAudioProcessorEditor::paintHeader (juce::Graphics& g)
{
    VoidGUI::drawPanel (g, dr (24, 24, 1488, 49), 10, 0.95f);
    // A fine wave emblem rather than the previous colored equalizer bars.
    juce::Path wave;
    for (int i = 0; i <= 80; ++i)
    {
        const float t = i / 80.0f;
        const float x = 49 + t * 42;
        const float y = 48 + std::sin (t * 19.0f) * std::sin (t * juce::MathConstants<float>::pi) * 11;
        if (i == 0) wave.startNewSubPath (x, y); else wave.lineTo (x, y);
    }
    g.setColour (VoidGUI::white.withAlpha (0.86f));
    g.strokePath (wave, juce::PathStrokeType (1.0f));
    g.setFont (VoidGUI::font (400, 22));
    g.drawText ("D.A Production", dr (100, 32, 240, 31), juce::Justification::centredLeft);
    VoidGUI::drawTrackedText (g, "SPACE BEYOND SOUND", dr (510, 34, 530, 26), 2.3f,
                              VoidGUI::font (400, 14), VoidGUI::textDim);
    VoidGUI::drawTrackedText (g, "VOID CHAMBER", dr (420, 84, 696, 58), 2.0f,
                              VoidGUI::titleFont (53), VoidGUI::white);
    VoidGUI::drawTrackedText (g, "REVERB", dr (580, 143, 376, 26), 6.0f,
                              VoidGUI::font (400, 22), VoidGUI::white);
}

void VoidChamberReverbAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (bgVoid);
    g.saveState();
    g.addTransform (juce::AffineTransform::scale (getWidth() / designW, getHeight() / designH));
    if (background.isValid())
        g.drawImage (background, dr (0, 0, designW, designH));
    paintPanels (g); paintFrame (g); paintHeader (g);

    const char* names[] = { "SIZE", "PRE DELAY", "DAMPING", "MODULATION" };
    const float centers[] = {316, 467, 1061, 1216};
    const juce::String texts[] = {sizeText, preDelayText, dampingText, modulationText};
    for (int i = 0; i < 4; ++i)
    {
        VoidGUI::drawTrackedText (g, names[i], dr (centers[i] - 74, 540, 148, 25), 1.1f,
                                  VoidGUI::font (400, 16), VoidGUI::textDim);
        g.setFont (VoidGUI::font (400, 21));
        g.setColour (VoidGUI::cyanHot);
        g.drawText (texts[i], dr (centers[i] - 70, 682, 140, 28), juce::Justification::centred);
    }
    VoidGUI::drawTrackedText (g, "INPUT", dr (49, 109, 150, 25), 1.8f,
                              VoidGUI::font (400, 17), VoidGUI::cyanHot);
    VoidGUI::drawTrackedText (g, "OUTPUT", dr (1336, 109, 150, 25), 1.8f,
                              VoidGUI::font (400, 17), VoidGUI::pinkHot);
    VoidGUI::drawTrackedText (g, "MIX", dr (1351, 555, 120, 24), 1.6f,
                              VoidGUI::font (400, 16), VoidGUI::textDim);
    g.setFont (VoidGUI::font (400, 20));
    g.setColour (VoidGUI::pinkHot);
    g.drawText (mixText, dr (1351, 682, 120, 28), juce::Justification::centred);
    VoidGUI::drawTrackedText (g, "SPACE TYPE", dr (62, 752, 455, 30), 3.0f,
                              VoidGUI::font (400, 16), VoidGUI::textDim);
    VoidGUI::drawTrackedText (g, "FREQUENCY DECAY", dr (577, 752, 371, 30), 1.8f,
                              VoidGUI::font (400, 16), VoidGUI::textDim);
    VoidGUI::drawTrackedText (g, "ADVANCED", dr (995, 752, 481, 30), 2.4f,
                              VoidGUI::font (400, 16), VoidGUI::textDim);
    g.restoreState();
}

void VoidChamberReverbAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    // Host-driven sizes can bypass the corner constrainer. Fill both axes
    // rather than letterboxing; paint and hit targets share this transform.
    layoutContents (static_cast<float> (area.getWidth()) / designW, 0.0f, 0.0f);

    if (browser)
        browser->setBounds (area);
}

void VoidChamberReverbAudioProcessorEditor::layoutContents (float scale, float offsetX, float offsetY)
{
    juce::ignoreUnused (offsetX, offsetY);
    const float scaleY = getHeight() / designH;
    const auto place = [scale, scaleY] (juce::Component& comp, float x, float y, float w, float h)
    {
        comp.setTransform (juce::AffineTransform());
        comp.setBounds (juce::roundToInt (x), juce::roundToInt (y),
                        juce::roundToInt (w), juce::roundToInt (h));
        comp.setTransform (juce::AffineTransform::scale (scale, scaleY));
    };

    place (presetNameButton, 1177, 29, 164, 38);
    place (presetPrev, 1344, 31, 35, 34);
    place (presetNext, 1384, 31, 35, 34);
    place (menuButton, 1444, 31, 43, 34);

    place (inputMeter, 54, 142, 140, 405);
    place (outputMeter, 1341, 142, 140, 405);
    place (monoButton, 57, 553, 63, 31);
    place (stereoButton, 123, 553, 76, 31);
    place (mixSlider, 1346, 575, 130, 112);

    place (sizeSlider, 251, 567, 130, 116);
    place (preDelaySlider, 402, 567, 130, 116);
    place (dampingSlider, 996, 567, 130, 116);
    place (modulationSlider, 1151, 567, 130, 116);
    place (decaySlider, 558, 476, 420, 248);
    const float centers[] = {91, 190, 289, 388, 487};
    for (int i = 0; i < 5; ++i)
        place (orbs[i], centers[i] - 49, 791, 98, 145);
    place (freqGraph, 577, 790, 371, 158);
    place (diffusionCell, 991, 791, 115, 155);
    place (widthCell, 1114, 791, 115, 155);
    place (airCell, 1237, 791, 115, 155);
    place (characterCell, 1360, 791, 115, 155);
}

void VoidChamberReverbAudioProcessorEditor::timerCallback()
{
    inputMeter.setLevel (processor.getInputLevel());
    outputMeter.setLevel (processor.getOutputLevel());
    inputMeter.update (1.0f / 30.0f);
    outputMeter.update (1.0f / 30.0f);

    auto& state = processor.getValueTreeState();
    const float mode = state.getRawParameterValue (VoidChamberReverbAudioProcessor::channelModeID)->load();
    stereoButton.setToggleState (mode < 0.5f, juce::dontSendNotification);
    monoButton.setToggleState   (mode >= 0.5f, juce::dontSendNotification);

    const int selectedSpace = juce::roundToInt (state.getRawParameterValue (VoidChamberReverbAudioProcessor::spaceTypeID)->load());
    for (int i = 0; i < 5; ++i)
        orbs[i].setSelected (i == selectedSpace);

    diffusionCell.syncFromParameter();
    widthCell.syncFromParameter();
    airCell.syncFromParameter();
    characterCell.syncFromParameter();

    if ((++frameCounter % 3) == 0)
        freqGraph.repaint();

    updateCachedTexts();
}

bool VoidChamberReverbAudioProcessorEditor::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress::escapeKey && browser != nullptr && browser->isVisible())
    {
        browser->setVisible (false);
        repaint();
        return true;
    }
    return juce::AudioProcessorEditor::keyPressed (key);
}
