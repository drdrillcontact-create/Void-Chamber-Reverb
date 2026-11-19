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
    clip = target > 0.96f ? 1.0f : juce::jmax (0.0f, clip - 0.02f);
}

void LevelMeter::update (float dt)
{
    if (peakHoldSeconds > 0.0f)
        peakHoldSeconds -= dt;
    else
        peak *= std::pow (10.0f, -1.6f * dt); // ~1.6 dB/s falloff
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
    const auto full = getLocalBounds().toFloat();
    const float trackW = juce::jmin (22.0f, full.getWidth() * 0.24f);
    const float trackH = full.getHeight() - 34.0f;
    const float x = full.getCentreX() - trackW * 0.5f;
    const auto track = juce::Rectangle<float> (x, 0.0f, trackW, trackH);

    // Scale ticks (left and right)
    g.setColour (VoidGUI::textFaint.withAlpha (0.75f));
    for (int i = 0; i <= 8; ++i)
    {
        const float ty = track.getY() + track.getHeight() * (static_cast<float> (i) / 8.0f);
        const bool major = i % 2 == 0;
        const float tick = major ? 15.0f : 8.0f;
        g.drawLine (x - tick - 4.0f, ty, x - 5.0f, ty, 1.1f);
        g.drawLine (x + trackW + 5.0f, ty, x + trackW + tick + 4.0f, ty, 1.1f);
    }

    // Track body
    juce::Path trackPath;
    trackPath.addRoundedRectangle (track, trackW * 0.32f);
    g.setColour (juce::Colour (0xff0b0d12));
    g.fillPath (trackPath);
    g.setColour (VoidGUI::panelStroke);
    g.strokePath (trackPath, juce::PathStrokeType (1.1f));

    // Fill
    const float level = juce::jlimit (0.0f, 1.0f, display);
    const float fillH = level * (track.getHeight() - 4.0f);
    if (fillH > 1.0f)
    {
        const auto fillRect = juce::Rectangle<float> (track.getX() + 2.0f, track.getBottom() - 2.0f - fillH,
                                                      track.getWidth() - 4.0f, fillH).toFloat();
        g.setColour (pink ? juce::Colour (0xff3a0f26) : juce::Colour (0xff0e3a45)); // deep base glow
        g.fillRect (track.getX() + 2.0f, track.getY() + 2.0f, track.getWidth() - 4.0f, track.getHeight() - 4.0f);

        juce::ColourGradient grad (pink ? juce::Colour (0xff94184f) : juce::Colour (0xff0a7d94),
                                   fillRect.getX(), fillRect.getBottom(),
                                   pink ? VoidGUI::pinkHot : VoidGUI::cyanHot,
                                   fillRect.getX(), fillRect.getY(), false);
        grad.addColour (0.55, pink ? VoidGUI::pink : VoidGUI::cyan);
        g.setGradientFill (grad);
        juce::Path fillPath;
        fillPath.addRoundedRectangle (fillRect, trackW * 0.22f);
        g.fillPath (fillPath);

        // Segment gaps
        g.setColour (juce::Colour (0xff0b0d12).withAlpha (0.55f));
        for (float yy = fillRect.getBottom() - 6.0f; yy > fillRect.getY() + 2.0f; yy -= 6.0f)
            g.drawLine (fillRect.getX(), yy, fillRect.getRight(), yy, 1.6f);
    }

    // Peak hold line
    if (peak > 0.005f)
    {
        const float py = track.getBottom() - 2.0f - peak * (track.getHeight() - 4.0f);
        g.setColour (peak > 0.96f ? juce::Colour (0xffff4d5e)
                                  : (pink ? VoidGUI::pinkHot : VoidGUI::cyanHot).withAlpha (0.95f));
        g.drawLine (track.getX() + 2.0f, py, track.getRight() - 2.0f, py, 2.0f);
    }

    // Clip indicator
    if (clip > 0.02f)
    {
        g.setColour (juce::Colour (0xffff3b47).withAlpha (clip));
        g.fillEllipse (track.getCentreX() - 4.0f, -10.0f, 8.0f, 8.0f);
    }

    // dB readout
    const float db = level <= 0.001f ? -60.0f : 20.0f * std::log10 (level);
    const auto dbText = (db <= -59.5f) ? juce::String ("-inf dB") : juce::String (db, 1) + " dB";
    g.setFont (VoidGUI::font (400, 15.0f));
    g.setColour (VoidGUI::textDim);
    g.drawText (dbText, juce::Rectangle<float> (full.getX(), full.getHeight() - 30.0f, full.getWidth(), 26.0f),
                juce::Justification::centred);
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
    const float w = static_cast<float> (getWidth());
    const float h = static_cast<float> (getHeight());
    const float sx = w / arcLen, sy = h / arcHei;
    const float s  = juce::jmin (sx, sy);
    g.addTransform (juce::AffineTransform::scale (s).translated ((w - arcLen * s) * 0.5f, (h - arcHei * s) * 0.5f));

    const float cx = arcLen * 0.5f;
    const float cy = 252.0f;
    const float radius = 238.0f;
    const float a0 = -1.493f, a1 = 1.493f;

    const auto arcPoint = [&] (float angle)
    {
        return juce::Point<float> (cx + radius * std::sin (angle), cy - radius * std::cos (angle));
    };

    // ---- Track
    juce::Path track;
    track.startNewSubPath (arcPoint (a0));
    track.addCentredArc (cx, cy, radius, radius, 0.0f, a0, a1, true);
    g.setColour (juce::Colour (0xff191c23));
    g.strokePath (track, juce::PathStrokeType (11.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (juce::Colour (0xff363d49));
    g.strokePath (track, juce::PathStrokeType (2.4f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // ---- Value arc: cyan -> white -> magenta neon tube
    const float norm = static_cast<float> (valueToProportionOfLength ((double) getValue()));
    const float aNow = a0 + (a1 - a0) * norm;
    if (norm > 0.002f)
    {
        juce::Path value;
        value.startNewSubPath (arcPoint (a0));
        value.addCentredArc (cx, cy, radius, radius, 0.0f, a0, aNow, true);

        const auto leftEnd  = arcPoint (a0);
        const auto apexPt   = arcPoint (0.0f);
        const auto rightEnd = arcPoint (a1);
        juce::ColourGradient neon (VoidGUI::cyan, leftEnd.x, leftEnd.y, VoidGUI::pink, rightEnd.x, rightEnd.y, false);
        neon.addColour (0.5, juce::Colours::white);
        juce::ignoreUnused (apexPt);

        g.setColour (VoidGUI::cyan.withAlpha (0.18f));
        g.strokePath (value, juce::PathStrokeType (26.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (VoidGUI::cyan.withAlpha (0.35f));
        g.strokePath (value, juce::PathStrokeType (15.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setGradientFill (neon);
        g.strokePath (value, juce::PathStrokeType (7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

        const auto tip = arcPoint (aNow);
        g.setColour (juce::Colours::white);
        g.fillEllipse (tip.x - 4.6f, tip.y - 4.6f, 9.2f, 9.2f);
    }

    // ---- Labels
    VoidGUI::drawTrackedText (g, "DECAY TIME", { cx - 130.0f, 56.0f, 260.0f, 20.0f }, 5.5f,
                              VoidGUI::font (400, 15.0f), juce::Colour (0xffcdd2de));

    const auto seconds = (float) getValue();
    const auto numberText = seconds < 10.0f ? juce::String (seconds, 1) : juce::String (juce::roundToInt (seconds));
    g.setFont (VoidGUI::font (200, 66.0f));
    g.setColour (VoidGUI::white);
    const float numW = VoidGUI::textWidth (g.getCurrentFont(), numberText);
    g.setFont (VoidGUI::font (300, 27.0f));
    const float unitW = VoidGUI::textWidth (g.getCurrentFont(), " s");
    const float totalW = numW + unitW;
    g.setFont (VoidGUI::font (200, 66.0f));
    g.drawSingleLineText (numberText, juce::roundToInt (cx - totalW * 0.5f), 142);
    g.setFont (VoidGUI::font (300, 27.0f));
    g.setColour (VoidGUI::textDim);
    g.drawSingleLineText (" s", juce::roundToInt (cx - totalW * 0.5f + numW), 142);

    // ---- Decay waveform (mirrored bars + two envelope curves)
    const float baseY = 205.0f;
    const float halfSpan = 208.0f;
    const int barCount = 58;
    for (int i = 0; i < barCount; ++i)
    {
        const float t = (float) i / (float) (barCount - 1);
        const float env = std::exp (-2.6f * t)
                        * (0.58f + 0.42f * std::sin (t * 21.0f + 0.6f) * std::sin (t * 6.3f + 1.2f));
        const float barH = juce::jmax (1.5f, env * 46.0f);
        const float bx = cx - halfSpan + t * halfSpan * 2.0f;
        const float alpha = 0.15f + 0.75f * (1.0f - t);
        g.setColour (VoidGUI::pink.withAlpha (alpha));
        g.drawLine (bx, baseY - barH, bx, baseY + barH * 0.82f, 2.0f);
    }

    juce::Path curve1, curve2;
    for (int i = 0; i <= 120; ++i)
    {
        const float t = (float) i / 120.0f;
        const float env = std::exp (-2.15f * t);
        const float x = cx - halfSpan + t * halfSpan * 2.0f;
        const float y1 = baseY - std::sin (t * 9.2f + 0.55f) * env * 34.0f - 4.0f;
        const float y2 = baseY - std::sin (t * 9.2f + 1.75f) * env * 22.0f - 2.0f;
        if (i == 0) { curve1.startNewSubPath (x, y1); curve2.startNewSubPath (x, y2); }
        else        { curve1.lineTo (x, y1);         curve2.lineTo (x, y2); }
    }
    g.setColour (VoidGUI::purple.withAlpha (0.55f));
    g.strokePath (curve2, juce::PathStrokeType (1.3f));
    g.setColour (VoidGUI::purple.withAlpha (0.30f));
    g.strokePath (curve1, juce::PathStrokeType (4.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (VoidGUI::pinkHot.withAlpha (0.9f));
    g.strokePath (curve1, juce::PathStrokeType (1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Baseline
    juce::ColourGradient baseline (VoidGUI::pink.withAlpha (0.55f), cx - halfSpan, baseY,
                                   VoidGUI::pink.withAlpha (0.0f), cx + halfSpan, baseY, false);
    g.setGradientFill (baseline);
    g.drawLine (cx - halfSpan, baseY + 0.5f, cx + halfSpan, baseY + 0.5f, 1.1f);

    // ---- Range captions
    g.setFont (VoidGUI::font (400, 14.0f));
    g.setColour (juce::Colour (0xffb9bfcf));
    g.drawSingleLineText ("0.1 s", juce::roundToInt (14.0f), 213);
    g.drawSingleLineText ("20 s", juce::roundToInt (arcLen - 14.0f - VoidGUI::textWidth (g.getCurrentFont(), "20 s")), 213);
}

//==============================================================================
// OrbButton (SPACE TYPE)
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
    const float orbSize = juce::jmin (b.getWidth(), b.getHeight() * 0.78f);
    const auto orbRect = juce::Rectangle<float> (b.getCentreX() - orbSize * 0.5f, 2.0f, orbSize, orbSize);

    // Selection plate (rounded square, neon pink)
    if (selected)
    {
        const auto plate = b.withTrimmedBottom (b.getHeight() - orbSize - 8.0f).expanded (6.0f, 4.0f);
        juce::Path platePath;
        platePath.addRoundedRectangle (plate, 14.0f);
        g.setColour (VoidGUI::pink.withAlpha (0.10f));
        g.fillPath (platePath);
        g.setColour (VoidGUI::pink.withAlpha (0.35f));
        g.strokePath (platePath, juce::PathStrokeType (5.5f));
        g.setColour (VoidGUI::pink);
        g.strokePath (platePath, juce::PathStrokeType (1.6f));
    }
    else if (hovered)
    {
        g.setColour (juce::Colours::white.withAlpha (0.045f));
        g.fillRoundedRectangle (orbRect.expanded (4.0f), 10.0f);
    }

    // Orb image clipped to a circle
    if (orb.isValid())
    {
        juce::Path clip;
        clip.addEllipse (orbRect.reduced (1.0f));
        g.saveState();
        g.reduceClipRegion (clip);
        g.drawImage (orb, orbRect, juce::RectanglePlacement::centred | juce::RectanglePlacement::fillDestination);
        g.restoreState();

        // Rim light + inner shadow
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.drawEllipse (orbRect, 2.4f);
        g.setColour (juce::Colour (0xff6de8ff).withAlpha (0.28f));
        g.drawEllipse (orbRect.reduced (1.6f), 1.0f);
    }

    // Caption
    VoidGUI::drawTrackedText (g, name.toUpperCase(),
                              { b.getX(), b.getBottom() - 24.0f, b.getWidth(), 20.0f }, 2.6f,
                              VoidGUI::font (500, 14.0f),
                              selected ? VoidGUI::pink : VoidGUI::textDim);
}

//==============================================================================
// AdvancedCell
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
    const float square = juce::jmin (b.getWidth(), b.getHeight() * 0.58f);
    const auto iconBox = juce::Rectangle<float> (b.getCentreX() - square * 0.5f, 0.0f, square, square);

    // Cell plate
    juce::Path plate;
    plate.addRoundedRectangle (iconBox, square * 0.22f);
    g.setColour (juce::Colour (0xff11141b).withAlpha (hovered ? 0.95f : 0.82f));
    g.fillPath (plate);
    g.setColour (hovered ? VoidGUI::panelInner : VoidGUI::panelStroke);
    g.strokePath (plate, juce::PathStrokeType (hovered ? 1.6f : 1.1f));

    paintIcon (g, iconBox.reduced (square * 0.24f));

    // Label + value
    VoidGUI::drawTrackedText (g, label.toUpperCase(),
                              { b.getX(), iconBox.getBottom() + 4.0f, b.getWidth(), 17.0f }, 2.2f,
                              VoidGUI::font (500, 12.5f), VoidGUI::textDim);
    g.setFont (VoidGUI::font (isCharacter ? 500 : 400, 17.0f));
    g.setColour (isCharacter ? VoidGUI::pinkHot : VoidGUI::white);
    g.drawText (valueText(),
                juce::Rectangle<float> (b.getX(), iconBox.getBottom() + 20.0f, b.getWidth(), 22.0f),
                juce::Justification::centred);
}

void AdvancedCell::paintIcon (juce::Graphics& g, juce::Rectangle<float> r)
{
    const float cx = r.getCentreX(), cy = r.getCentreY();
    const float s = r.getWidth();
    const juce::Colour ic = VoidGUI::white.withAlpha (0.92f);
    g.setColour (ic);
    const float lw = juce::jmax (1.4f, s * 0.055f);

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
            g.fillPath (leftArrow);
            g.fillPath (rightArrow);
            break;
        }
        case Icon::air:
        {
            juce::Path arc;
            arc.addCentredArc (cx, cy, s * 0.34f, s * 0.34f, 0.0f,
                               juce::MathConstants<float>::pi * 0.35f,
                               juce::MathConstants<float>::pi * 1.72f, true);
            g.strokePath (arc, juce::PathStrokeType (lw, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            // Arrow head at arc end
            const float endAngle = juce::MathConstants<float>::pi * 1.72f;
            const auto tip = juce::Point<float> (cx + s * 0.34f * std::sin (endAngle), cy - s * 0.34f * std::cos (endAngle));
            juce::Path head;
            head.startNewSubPath (tip);
            head.lineTo (tip.x - s * 0.17f, tip.y - s * 0.03f);
            head.lineTo (tip.x - s * 0.02f, tip.y - s * 0.17f);
            head.closeSubPath();
            g.fillPath (head);
            g.fillEllipse (cx - lw * 0.6f, cy - lw * 0.6f, lw * 1.2f, lw * 1.2f);
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

    const float left   = bounds.getX() + 10.0f;
    const float right  = bounds.getRight() - 10.0f;
    const float top    = bounds.getY() + 6.0f;
    const float bottom = bounds.getBottom() - 24.0f;
    const float height = bottom - top;

    const auto freqToX = [&] (float freq)
    {
        const float t = (std::log10 (freq) - std::log10 (20.0f)) / (std::log10 (20000.0f) - std::log10 (20.0f));
        return left + t * (right - left);
    };
    // 0 dB at the top of the graph; 45 dB of headroom so the high band
    // approaches but never clamps flat at the floor.
    const float pxPerDb = height / 50.0f;
    const auto lossToY = [&] (float db) { return top + juce::jlimit (0.0f, 50.0f, db) * pxPerDb; };

    // Grid: verticals + one horizontal midline
    g.setColour (juce::Colours::white.withAlpha (0.055f));
    for (float f : { 20.0f, 100.0f, 1000.0f, 10000.0f, 20000.0f })
        g.drawLine (freqToX (f), top, freqToX (f), bottom, 1.0f);
    g.drawLine (left, top + height * 0.5f, right, top + height * 0.5f, 1.0f);

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
            if (db > 50.0f)
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

        g.setColour (band.colour.withAlpha (0.075f));
        g.fillPath (fill);
        g.setColour (band.colour.withAlpha (0.32f));
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
    setupKnob (sizeSlider,       pinkFeel);
    setupKnob (preDelaySlider,   cyanFeel);
    setupKnob (dampingSlider,    cyanFeel);
    setupKnob (modulationSlider, pinkFeel);
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

    setResizable (true, true);
    setResizeLimits (1075, 717, 3072, 2048);
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

namespace
{
    void drawLogoBars (juce::Graphics& g, juce::Rectangle<float> area)
    {
        const float barW = 5.0f, gap = 5.0f;
        const float heights[5] = { 20.0f, 38.0f, 26.0f, 46.0f, 32.0f };
        const juce::Colour colours[5] = { VoidGUI::white, VoidGUI::cyan, VoidGUI::white, VoidGUI::cyan, VoidGUI::white };
        float x = area.getX();
        for (int i = 0; i < 5; ++i)
        {
            const float h = heights[i];
            const auto bar = juce::Rectangle<float> (x, area.getCentreY() - h * 0.5f, barW, h);
            g.setColour (colours[i].withAlpha (i == 1 || i == 3 ? 1.0f : 0.85f));
            g.fillRoundedRectangle (bar, 2.5f);
            x += barW + gap;
        }
    }
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
        repaint();
    }
}

void VoidChamberReverbAudioProcessorEditor::paintPanels (juce::Graphics& g)
{
    VoidGUI::drawPanel (g, dr (34.0f, 575.0f, 471.0f, 173.0f), 17.0f, 0.70f);
    VoidGUI::drawPanel (g, dr (1030.0f, 575.0f, 472.0f, 173.0f), 17.0f, 0.70f);
    VoidGUI::drawPanel (g, dr (34.0f, 756.0f, 534.0f, 192.0f), 17.0f, 0.72f);
    VoidGUI::drawPanel (g, dr (578.0f, 756.0f, 432.0f, 192.0f), 17.0f, 0.72f);
    VoidGUI::drawPanel (g, dr (1020.0f, 756.0f, 482.0f, 192.0f), 17.0f, 0.72f);
}

void VoidChamberReverbAudioProcessorEditor::paintFrame (juce::Graphics& g)
{
    // Outer bezel: brushed-metal vertical gradient stroke
    juce::ColourGradient bezel (juce::Colour (0xff7c8494), 0.0f, 0.0f,
                                juce::Colour (0xff14161c), 0.0f, designH, false);
    bezel.addColour (0.12, juce::Colour (0xff565e6c));
    bezel.addColour (0.5, juce::Colour (0xff22252d));
    bezel.addColour (0.9, juce::Colour (0xff191c23));
    juce::Path outer;
    outer.addRoundedRectangle (5.0f, 5.0f, designW - 10.0f, designH - 10.0f, 22.0f);
    g.setGradientFill (bezel);
    g.strokePath (outer, juce::PathStrokeType (3.4f));

    // Inner hairline
    juce::Path inner;
    inner.addRoundedRectangle (11.5f, 11.5f, designW - 23.0f, designH - 23.0f, 17.0f);
    g.setColour (juce::Colours::white.withAlpha (0.055f));
    g.strokePath (inner, juce::PathStrokeType (1.0f));

    // Top sheen fading to the sides
    juce::ColourGradient sheen (juce::Colours::transparentWhite, 60.0f, 12.0f,
                                juce::Colours::transparentWhite, designW - 60.0f, 12.0f, false);
    sheen.addColour (0.5, juce::Colours::white.withAlpha (0.20f));
    g.setGradientFill (sheen);
    g.drawLine (60.0f, 12.0f, designW - 60.0f, 12.0f, 1.2f);

    // Corner screws
    const float screwR = 5.2f;
    const juce::Point<float> screws[4] = { { 24.0f, 24.0f }, { designW - 24.0f, 24.0f },
                                           { 24.0f, designH - 24.0f }, { designW - 24.0f, designH - 24.0f } };
    for (const auto& screw : screws)
    {
        juce::ColourGradient sg (juce::Colour (0xff4a505e), screw.x - screwR, screw.y - screwR,
                                 juce::Colour (0xff14161c), screw.x + screwR, screw.y + screwR, true);
        g.setGradientFill (sg);
        g.fillEllipse (screw.x - screwR, screw.y - screwR, screwR * 2.0f, screwR * 2.0f);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawEllipse (screw.x - screwR, screw.y - screwR, screwR * 2.0f, screwR * 2.0f, 1.0f);
        g.drawLine (screw.x - screwR * 0.62f, screw.y + screwR * 0.62f,
                    screw.x + screwR * 0.62f, screw.y - screwR * 0.62f, 1.2f);
    }
}

void VoidChamberReverbAudioProcessorEditor::paintHeader (juce::Graphics& g)
{
    // Left: logo + maker
    drawLogoBars (g, dr (46.0f, 44.0f, 76.0f, 44.0f));
    VoidGUI::drawTrackedText (g, "D.A Production", dr (118.0f, 50.0f, 260.0f, 34.0f), 2.6f,
                              VoidGUI::font (500, 19.5f), juce::Colour (0xffd5d9e2),
                              juce::Justification::centredLeft);

    // Centre: tagline + wordmark + sub-wordmark
    VoidGUI::drawTrackedText (g, "SPACE BEYOND SOUND", dr (designW * 0.5f - 260.0f, 34.0f, 520.0f, 16.0f), 5.2f,
                              VoidGUI::font (400, 11.5f), VoidGUI::textFaint);

    // VOID CHAMBER — wide, hairline, with a faint bloom underneath
    VoidGUI::drawTrackedText (g, "VOID CHAMBER", dr (designW * 0.5f - 500.0f, 108.0f, 1000.0f, 62.0f), 26.0f,
                              VoidGUI::font (200, 57.0f), juce::Colour (0xffe9ebf2));

    VoidGUI::drawTrackedText (g, "R E V E R B", dr (designW * 0.5f - 200.0f, 168.0f, 400.0f, 20.0f), 17.0f,
                              VoidGUI::font (300, 16.0f), VoidGUI::textDim);

    // Right side: stacked caption
    const juce::String caption[4] = { "INFINITE", "SPACES.", "REAL", "EMOTIONS" };
    for (int i = 0; i < 4; ++i)
        VoidGUI::drawTrackedText (g, caption[i], dr (1216.0f, 168.0f + static_cast<float> (i) * 34.0f, 150.0f, 20.0f), 4.4f,
                                  VoidGUI::font (300, 15.0f), juce::Colour (0xffbcc1cd),
                                  juce::Justification::centredRight);
}

void VoidChamberReverbAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (bgVoid);

    const float s = uiScale;
    g.saveState();
    g.addTransform (juce::AffineTransform::scale (s).translated (designBounds.getX() / s, designBounds.getY() / s));

    // Backdrop scene
    if (background.isValid())
        g.drawImage (background, 0.0f, 0.0f, designW, designH,
                     0.0f, 0.0f, static_cast<float> (background.getWidth()), static_cast<float> (background.getHeight()));

    paintPanels (g);
    paintFrame (g);
    paintHeader (g);

    // Panel titles
    VoidGUI::drawTrackedText (g, "SIZE",      dr (34.0f, 587.0f, 260.0f, 22.0f), 5.0f, VoidGUI::font (400, 15.0f), VoidGUI::textDim);
    VoidGUI::drawTrackedText (g, "PRE DELAY", dr (236.0f, 587.0f, 260.0f, 22.0f), 5.0f, VoidGUI::font (400, 15.0f), VoidGUI::textDim);
    VoidGUI::drawTrackedText (g, "DAMPING",   dr (964.0f, 587.0f, 260.0f, 22.0f), 5.0f, VoidGUI::font (400, 15.0f), VoidGUI::textDim);
    VoidGUI::drawTrackedText (g, "M O D U L A T I O N", dr (1216.0f, 587.0f, 268.0f, 22.0f), 5.0f, VoidGUI::font (400, 15.0f), VoidGUI::textDim);

    // Knob value readouts
    const auto valueFont = VoidGUI::font (400, 19.0f);
    g.setFont (valueFont);
    g.setColour (VoidGUI::white);
    g.drawText (sizeText,       dr (34.0f, 706.0f, 260.0f, 24.0f),   juce::Justification::centred);
    g.drawText (preDelayText,   dr (236.0f, 706.0f, 260.0f, 24.0f),  juce::Justification::centred);
    g.drawText (dampingText,    dr (964.0f, 706.0f, 260.0f, 24.0f),  juce::Justification::centred);
    g.drawText (modulationText, dr (1216.0f, 706.0f, 268.0f, 24.0f), juce::Justification::centred);

    // Meter labels + mix readout
    VoidGUI::drawTrackedText (g, "INPUT",  dr (46.0f, 148.0f, 100.0f, 20.0f), 5.0f, VoidGUI::font (400, 14.0f), VoidGUI::textDim);
    VoidGUI::drawTrackedText (g, "OUTPUT", dr (1390.0f, 148.0f, 100.0f, 20.0f), 5.0f, VoidGUI::font (400, 14.0f), VoidGUI::textDim);
    g.setFont (valueFont);
    g.setColour (VoidGUI::white);
    g.drawText (mixText, dr (1392.0f, 544.0f, 96.0f, 22.0f), juce::Justification::centred);

    // Bottom panel titles
    VoidGUI::drawTrackedText (g, "SPACE TYPE",     dr (56.0f, 766.0f, 260.0f, 18.0f), 4.2f, VoidGUI::font (400, 14.0f), VoidGUI::textDim, juce::Justification::centredLeft);
    VoidGUI::drawTrackedText (g, "FREQUENCY DECAY", dr (600.0f, 766.0f, 280.0f, 18.0f), 4.2f, VoidGUI::font (400, 14.0f), VoidGUI::textDim, juce::Justification::centredLeft);
    VoidGUI::drawTrackedText (g, "ADVANCED",       dr (1042.0f, 766.0f, 220.0f, 18.0f), 4.2f, VoidGUI::font (400, 14.0f), VoidGUI::textDim, juce::Justification::centredLeft);

    // FREQUENCY DECAY legend — right-justified against the panel edge and
    // pushed clear of the title by measuring both.
    const auto legendFont = VoidGUI::font (400, 12.5f);
    const char* legendLabels[3] = { "LOW", "MID", "HIGH" };
    const juce::Colour legendColours[3] = { VoidGUI::pink, VoidGUI::purple, VoidGUI::cyan };
    const float dot = 9.5f, dotGap = 6.0f, itemGap = 20.0f;
    float legendWidth = itemGap * 2.0f;
    float labelWidths[3];
    for (int i = 0; i < 3; ++i)
    {
        labelWidths[i] = VoidGUI::textWidth (legendFont, legendLabels[i]);
        legendWidth += dot + dotGap + labelWidths[i];
    }
    const float panelRight = 1000.0f;
    // Measure the tracked title so the legend never overlaps it.
    const auto titleFont = VoidGUI::font (400, 14.0f);
    float titleWidth = 0.0f;
    const juce::String titleText = "FREQUENCY DECAY";
    for (int i = 0; i < titleText.length(); ++i)
        titleWidth += VoidGUI::textWidth (titleFont, titleText.substring (i, i + 1)) + 4.2f;
    titleWidth -= 4.2f;
    float legendX = juce::jmax (panelRight - legendWidth, 600.0f + titleWidth + 22.0f);
    for (int i = 0; i < 3; ++i)
    {
        g.setColour (legendColours[i]);
        g.fillEllipse (legendX, 770.0f, dot, dot);
        g.setColour (VoidGUI::textDim);
        g.setFont (legendFont);
        g.drawSingleLineText (legendLabels[i], juce::roundToInt (legendX + dot + dotGap), 780);
        legendX += dot + dotGap + labelWidths[i] + itemGap;
    }

    g.restoreState();
}

void VoidChamberReverbAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    const float scale = juce::jmin (static_cast<float> (area.getWidth()) / designW,
                                    static_cast<float> (area.getHeight()) / designH);
    const float offsetX = (static_cast<float> (area.getWidth()) - designW * scale) * 0.5f;
    const float offsetY = (static_cast<float> (area.getHeight()) - designH * scale) * 0.5f;

    uiScale = scale;
    designBounds = juce::Rectangle<int> (juce::roundToInt (offsetX), juce::roundToInt (offsetY),
                                         juce::roundToInt (designW * scale), juce::roundToInt (designH * scale));

    layoutContents (scale, offsetX, offsetY);

    if (browser)
        browser->setBounds (area);
}

void VoidChamberReverbAudioProcessorEditor::layoutContents (float scale, float offsetX, float offsetY)
{
    const auto place = [scale, offsetX, offsetY] (juce::Component& comp,
                                                  float x, float y, float w, float h)
    {
        comp.setBounds (juce::roundToInt (offsetX + x * scale),
                        juce::roundToInt (offsetY + y * scale),
                        juce::roundToInt (w * scale),
                        juce::roundToInt (h * scale));
    };

    // Header preset navigation
    place (presetPrev,      1002.0f, 36.0f, 48.0f, 52.0f);
    place (presetNameButton, 1054.0f, 29.0f, 270.0f, 66.0f);
    place (presetNext,      1328.0f, 36.0f, 48.0f, 52.0f);
    place (menuButton,      1400.0f, 32.0f, 48.0f, 60.0f);

    // Left / right meter columns
    place (inputMeter,  46.0f, 168.0f, 100.0f, 244.0f);
    place (monoButton,  48.0f, 446.0f, 128.0f, 40.0f);
    place (stereoButton, 48.0f, 490.0f, 128.0f, 40.0f);
    place (outputMeter, 1390.0f, 168.0f, 100.0f, 244.0f);
    place (mixSlider,   1392.0f, 442.0f, 96.0f, 96.0f);

    // Knob row
    place (sizeSlider,       110.0f, 601.0f, 124.0f, 124.0f);
    place (preDelaySlider,   312.0f, 607.0f, 112.0f, 112.0f);
    place (dampingSlider,   1038.0f, 607.0f, 112.0f, 112.0f);
    place (modulationSlider, 1290.0f, 601.0f, 124.0f, 124.0f);

    // Decay arc (sits above the bottom panels, in front of the scene)
    place (decaySlider, 510.0f, 520.0f, 516.0f, 248.0f);

    // SPACE TYPE orbs
    const float orbCenters[5] = { 92.0f, 190.0f, 288.0f, 386.0f, 484.0f };
    for (int i = 0; i < 5; ++i)
        place (orbs[i], orbCenters[i] - 47.0f, 792.0f, 94.0f, 150.0f);

    // FREQUENCY DECAY graph
    place (freqGraph, 596.0f, 792.0f, 396.0f, 152.0f);

    // ADVANCED cells
    const float cellX[4] = { 1042.0f, 1154.0f, 1266.0f, 1378.0f };
    place (diffusionCell, cellX[0], 792.0f, 92.0f, 152.0f);
    place (widthCell,     cellX[1], 792.0f, 92.0f, 152.0f);
    place (airCell,       cellX[2], 792.0f, 92.0f, 152.0f);
    place (characterCell, cellX[3], 792.0f, 92.0f, 152.0f);
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
