#include "PluginEditor.h"

namespace
{
    using Processor = VoidChamberReverbAudioProcessor;
    const juce::Colour cyan { 0xff00d2ff }, magenta { 0xffff00ff };
    const juce::Rectangle<int> inputMeter { 752, 130, 20, 180 };
    const auto outputMeter = inputMeter.translated(50, 0);

    void drawMeter(juce::Graphics& g, juce::Rectangle<int> bounds, float level, juce::Colour colour)
    {
        const auto area = bounds.toFloat().reduced(1.0f);
        g.setColour(colour.withAlpha(0.85f));
        g.fillRoundedRectangle(area.withTop(area.getBottom() - area.getHeight() * level), 4.0f);
        g.setColour(juce::Colour(0xff333333));
        g.drawRoundedRectangle(area, 4.0f, 1.0f);
    }
}

VoidChamberReverbAudioProcessorEditor::VoidChamberReverbAudioProcessorEditor(Processor& p)
    : AudioProcessorEditor(&p), processorRef(p),
      spaceAttachment(*p.getValueTreeState().getParameter(Processor::spaceTypeID), [this](float choice)
      {
          const auto index = juce::roundToInt(choice);
          for (size_t i = 0; i < spaceButtons.size(); ++i)
              spaceButtons[i].setToggleState(static_cast<int>(i) == index, juce::dontSendNotification);
      })
{
    setLookAndFeel(&neonLaf);

    const char* const ids[] = { Processor::sizeID, Processor::preDelayID, Processor::decayID,
                                Processor::dampingID, Processor::modulationID, Processor::mixID };
    const char* const titles[] = { "SIZE", "PRE DELAY", "DECAY TIME", "DAMPING", "MODULATION", "OUTPUT MIX" };
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        auto& knob = knobs[i];
        knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        knob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        knob.setName(titles[i]);
        knob.setColour(juce::Slider::rotarySliderFillColourId, i == 2 || i == 5 ? magenta : cyan);
        addAndMakeVisible(knob);
        knobAttachments[i] = std::make_unique<SliderAttachment>(p.getValueTreeState(), ids[i], knob);

        auto& label = labels[i];
        label.setText(titles[i], juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setFont(juce::FontOptions(12.0f, juce::Font::bold));
        label.setColour(juce::Label::textColourId, juce::Colour(0xffaaaaaa));
        addAndMakeVisible(label);
    }

    const char* const names[] = { "VOID", "HALL", "PLATE", "CAVERN", "AMBIENT" };
    for (size_t i = 0; i < spaceButtons.size(); ++i)
    {
        auto& button = spaceButtons[i];
        button.setButtonText(names[i]);
        button.setRadioGroupId(1);
        button.setColour(juce::ToggleButton::tickColourId, cyan);
        button.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
        button.onClick = [this, i] { spaceAttachment.setValueAsCompleteGesture(static_cast<float>(i)); };
        addAndMakeVisible(button);
    }

    spaceAttachment.sendInitialUpdate();
    setSize(900, 700);
    startTimerHz(30);
}

VoidChamberReverbAudioProcessorEditor::~VoidChamberReverbAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void VoidChamberReverbAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Background — dark space gradient
    juce::ColourGradient bgGradient(juce::Colour(0xff050508), 0.0f, 0.0f,
                                    juce::Colour(0xff120a1a), (float)getWidth(), (float)getHeight(), false);
    bgGradient.addColour(0.5f, juce::Colour(0xff0b0b12));
    g.setGradientFill(bgGradient);
    g.fillAll();

    // Title
    g.setColour(juce::Colour(0xffffffff));
    g.setFont(juce::FontOptions(26.0f, juce::Font::bold));
    g.drawText("VOID CHAMBER", 40, 30, 320, 40, juce::Justification::left);
    g.setColour(juce::Colour(0xff888888));
    g.setFont(juce::FontOptions(13.0f));
    g.drawText("REVERB", 40, 56, 120, 20, juce::Justification::left);

    // Subtitle top right
    g.setColour(juce::Colour(0xff555555));
    g.setFont(juce::FontOptions(11.0f));
    g.drawText("INFINITE SPACES.", getWidth() - 260, 40, 220, 16, juce::Justification::right);
    g.drawText("REAL EMOTIONS", getWidth() - 260, 54, 220, 16, juce::Justification::right);

    // Section labels
    g.setColour(juce::Colour(0xff666666));
    g.setFont(juce::FontOptions(12.0f, juce::Font::bold));
    g.drawText("SPACE TYPE", 40, 480, 120, 20, juce::Justification::left);

    // Meter labels
    g.setColour(juce::Colour(0xff555555));
    g.setFont(juce::FontOptions(11.0f));
    g.drawText("INPUT", inputMeter.getCentreX() - 24, 106, 48, 16, juce::Justification::centred);
    g.drawText("OUTPUT", outputMeter.getCentreX() - 24, 106, 48, 16, juce::Justification::centred);

    drawMeter(g, inputMeter, processorRef.getInputLevel(), cyan);
    drawMeter(g, outputMeter, processorRef.getOutputLevel(), magenta);

    // Decorative ring
    g.setColour(juce::Colour(0xff221122).withAlpha(0.5f));
    g.drawEllipse((float)getWidth() / 2 - 140, 140, 280, 280, 1.5f);
}

void VoidChamberReverbAudioProcessorEditor::resized()
{
    const int cellWidth = (getWidth() - 60) / static_cast<int>(knobs.size());
    for (size_t i = 0; i < knobs.size(); ++i)
    {
        const int x = 30 + static_cast<int>(i) * cellWidth;
        knobs[i].setBounds(x + (cellWidth - 110) / 2, 330, 110, 110);
        labels[i].setBounds(x, 448, cellWidth, 18);
    }
    for (size_t i = 0; i < spaceButtons.size(); ++i)
        spaceButtons[i].setBounds(40 + static_cast<int>(i) * 100, 510, 96, 40);
}

void VoidChamberReverbAudioProcessorEditor::timerCallback()
{
    repaint(inputMeter.getUnion(outputMeter));
}

void VoidChamberReverbAudioProcessorEditor::NeonLookAndFeel::drawRotarySlider(
    juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
    float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider)
{
    auto bounds = juce::Rectangle<int>(x, y, width, height).toFloat().reduced(4.0f);
    auto centre = bounds.getCentre();
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;

    // Outer ring
    g.setColour(juce::Colour(0xff121212));
    g.fillEllipse(bounds);
    g.setColour(juce::Colour(0xff333333));
    g.drawEllipse(bounds, 1.5f);

    // Filled arc
    float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    juce::Colour fillColour = slider.findColour(juce::Slider::rotarySliderFillColourId);
    g.setColour(fillColour);

    juce::Path arc;
    arc.addCentredArc(centre.x, centre.y, radius, radius, 0.0f, rotaryStartAngle, angle, true);
    g.strokePath(arc, juce::PathStrokeType(4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Pointer line
    g.setColour(juce::Colour(0xffffffff));
    auto pointerLength = radius * 0.6f;
    juce::Point<float> pointerTip(
        centre.getX() + std::sin(angle) * pointerLength,
        centre.getY() - std::cos(angle) * pointerLength);
    g.drawLine(centre.getX(), centre.getY(), pointerTip.getX(), pointerTip.getY(), 2.0f);

    // Center dot
    g.setColour(fillColour);
    g.fillEllipse(centre.getX() - 5, centre.getY() - 5, 10, 10);
}
