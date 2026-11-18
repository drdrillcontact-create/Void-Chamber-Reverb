#include "PluginEditor.h"

//==============================================================================
VoidChamberReverbAudioProcessorEditor::VoidChamberReverbAudioProcessorEditor(
    VoidChamberReverbAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    setSize(900, 700);

    setLookAndFeel(&neonLaf);

    // ---- Knobs ----
    int kSize = 120;
    int kx = 60, ky = 330, gap = 170;

    auto setupKnob = [&](std::unique_ptr<juce::Slider>& s, const juce::String& paramID,
                         juce::Colour accentColour)
    {
        s = std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag,
                                           juce::Slider::NoTextBox);
        s->setColour(juce::Slider::rotarySliderFillColourId, accentColour);
        s->setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour(0xff1a1a1a));
        s->setColour(juce::Slider::thumbColourId, accentColour);
        addAndMakeVisible(*s);
    };

    sizeSlider = std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox);
    sizeSlider->setBounds(kx, ky, kSize, kSize);
    setupKnob(sizeSlider, VoidChamberReverbAudioProcessor::sizeID, juce::Colour(0xff00d2ff));
    sizeAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.parameters, VoidChamberReverbAudioProcessor::sizeID, *sizeSlider);

    preDelaySlider = std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox);
    preDelaySlider->setBounds(kx + gap, ky, kSize, kSize);
    setupKnob(preDelaySlider, VoidChamberReverbAudioProcessor::preDelayID, juce::Colour(0xff00d2ff));
    preDelayAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.parameters, VoidChamberReverbAudioProcessor::preDelayID, *preDelaySlider);

    decaySlider = std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox);
    decaySlider->setBounds(kx + gap * 2, ky, kSize, kSize);
    setupKnob(decaySlider, VoidChamberReverbAudioProcessor::decayID, juce::Colour(0xffff00ff));
    decayAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.parameters, VoidChamberReverbAudioProcessor::decayID, *decaySlider);

    dampingSlider = std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox);
    dampingSlider->setBounds(kx + gap * 3, ky, kSize, kSize);
    setupKnob(dampingSlider, VoidChamberReverbAudioProcessor::dampingID, juce::Colour(0xff00d2ff));
    dampingAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.parameters, VoidChamberReverbAudioProcessor::dampingID, *dampingSlider);

    modulationSlider = std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox);
    modulationSlider->setBounds(kx + gap * 4, ky, kSize, kSize);
    setupKnob(modulationSlider, VoidChamberReverbAudioProcessor::modulationID, juce::Colour(0xff00d2ff));
    modulationAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.parameters, VoidChamberReverbAudioProcessor::modulationID, *modulationSlider);

    mixSlider = std::make_unique<juce::Slider>(juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox);
    mixSlider->setBounds(kx + gap * 5, ky, kSize, kSize);
    setupKnob(mixSlider, VoidChamberReverbAudioProcessor::mixID, juce::Colour(0xffff00ff));
    mixAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.parameters, VoidChamberReverbAudioProcessor::mixID, *mixSlider);

    // ---- Labels ----
    addLabel("SIZE", kx, ky + kSize + 8, kSize, 18);
    addLabel("PRE DELAY", kx + gap, ky + kSize + 8, kSize, 18);
    addLabel("DECAY TIME", kx + gap * 2, ky + kSize + 8, kSize, 18);
    addLabel("DAMPING", kx + gap * 3, ky + kSize + 8, kSize, 18);
    addLabel("MODULATION", kx + gap * 4, ky + kSize + 8, kSize, 18);
    addLabel("OUTPUT MIX", kx + gap * 5, ky + kSize + 8, kSize, 18);

    // ---- Space type buttons ----
    const juce::String names[] = { "VOID", "HALL", "PLATE", "CAVERN", "AMBIENT" };
    int sw = 70, sh = 70, sx = 40, sy = 480, sg = 16;

    std::unique_ptr<juce::ToggleButton>* buttonPtrs[] = {
        &btnVoid, &btnHall, &btnPlate, &btnCavern, &btnAmbient
    };

    for (int i = 0; i < 5; ++i)
    {
        auto b = std::make_unique<juce::ToggleButton>(names[i]);
        b->setBounds(sx + i * (sw + sg), sy, sw, sh);
        b->setColour(juce::ToggleButton::tickColourId, juce::Colour(0xff00d2ff));
        b->setColour(juce::ToggleButton::textColourOffId, juce::Colour(0xff888888));
        b->setColour(juce::ToggleButton::textColourOnId, juce::Colour(0xffffffff));
        b->setToggleState(i == 0, juce::dontSendNotification);
        addAndMakeVisible(*b);
        spaceButtons[i] = b.get();
        *buttonPtrs[i] = std::move(b);

        int idx = i;
        spaceButtons[i]->onClick = [this, idx]()
        {
            // Set the choice parameter to this index
            processorRef.parameters.getParameter(VoidChamberReverbAudioProcessor::spaceTypeID)
                ->setValueNotifyingHost((float)idx);
        };
    }

    // ---- Meters ----
    inputMeter = std::make_unique<juce::Component>();
    inputMeter->setBounds(760, 120, 24, 240);
    addAndMakeVisible(*inputMeter);
    inputMeter->paint = [this](juce::Graphics& g)
    {
        float level = processorRef.getInputLevel();
        g.setColour(juce::Colour(0xff00d2ff).withAlpha(0.85f));
        g.fillRoundedRectangle(0.0f, 240.0f - level * 220.0f, 20.0f, level * 220.0f, 4.0f);
        g.setColour(juce::Colour(0xff333333));
        g.drawRoundedRectangle(0.0f, 20.0f, 20.0f, 220.0f, 4.0f, 1.0f);
    };

    outputMeter = std::make_unique<juce::Component>();
    outputMeter->setBounds(790, 120, 24, 240);
    addAndMakeVisible(*outputMeter);
    outputMeter->paint = [this](juce::Graphics& g)
    {
        float level = processorRef.getOutputLevel();
        g.setColour(juce::Colour(0xffff00ff).withAlpha(0.85f));
        g.fillRoundedRectangle(0.0f, 240.0f - level * 220.0f, 20.0f, level * 220.0f, 4.0f);
        g.setColour(juce::Colour(0xff333333));
        g.drawRoundedRectangle(0.0f, 20.0f, 20.0f, 220.0f, 4.0f, 1.0f);
    };

    // Listen to parameter changes (for space type sync)
    processorRef.parameters.addParameterListener(VoidChamberReverbAudioProcessor::spaceTypeID, this);

    startTimerHz(30);
}

VoidChamberReverbAudioProcessorEditor::~VoidChamberReverbAudioProcessorEditor()
{
    stopTimer();
    processorRef.parameters.removeParameterListener(VoidChamberReverbAudioProcessor::spaceTypeID, this);
    setLookAndFeel(nullptr);
}

//==============================================================================
void VoidChamberReverbAudioProcessorEditor::parameterValueChanged(int parameterIndex, float newValue)
{
    juce::ignoreUnused(parameterIndex);
    auto* param = processorRef.parameters.getParameter(VoidChamberReverbAudioProcessor::spaceTypeID);
    if (param == nullptr)
        return;

    int index = (int)juce::roundToInt(newValue);
    index = juce::jlimit(0, 4, index);

    for (int i = 0; i < 5; ++i)
    {
        if (spaceButtons[i] != nullptr)
            spaceButtons[i]->setToggleState(i == index, juce::dontSendNotification);
    }
}

//==============================================================================
void VoidChamberReverbAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Background — dark space gradient
    juce::ColourGradient bgGradient(juce::Colour(0xff050508), 0.0f, 0.0f,
                                    juce::Colour(0xff120a1a), (float)getWidth(), (float)getHeight());
    bgGradient.addColour(0.5f, juce::Colour(0xff0b0b12));
    g.setGradientFill(bgGradient);
    g.fillAll();

    // Title
    g.setColour(juce::Colour(0xffffffff));
    g.setFont(juce::Font(26.0f, juce::Font::bold));
    g.drawText("VOID CHAMBER", 40, 30, 320, 40, juce::Justification::left);
    g.setColour(juce::Colour(0xff888888));
    g.setFont(juce::Font(13.0f));
    g.drawText("REVERB", 40, 56, 120, 20, juce::Justification::left);

    // Subtitle top right
    g.setColour(juce::Colour(0xff555555));
    g.setFont(juce::Font(11.0f));
    g.drawText("INFINITE SPACES.", getWidth() - 260, 40, 220, 16, juce::Justification::right);
    g.drawText("REAL EMOTIONS", getWidth() - 260, 54, 220, 16, juce::Justification::right);

    // Section labels
    g.setColour(juce::Colour(0xff666666));
    g.setFont(juce::Font(12.0f, juce::Font::bold));
    g.drawText("SPACE TYPE", 40, 450, 120, 20, juce::Justification::left);

    // Meter labels
    g.setColour(juce::Colour(0xff555555));
    g.setFont(juce::Font(11.0f));
    g.drawText("INPUT", 760, 100, 24, 16, juce::Justification::centred);
    g.drawText("OUTPUT", 790, 100, 24, 16, juce::Justification::centred);

    // Decorative ring
    g.setColour(juce::Colour(0xff221122).withAlpha(0.5f));
    g.drawEllipse((float)getWidth() / 2 - 140, 140, 280, 280, 1.5f);
}

void VoidChamberReverbAudioProcessorEditor::resized()
{
    // Fixed layout in constructor; nothing dynamic required here.
}

void VoidChamberReverbAudioProcessorEditor::addLabel(const juce::String& text,
                                                      int x, int y, int w, int h)
{
    juce::Label label;
    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setFont(juce::Font(12.0f, juce::Font::bold));
    label.setColour(juce::Label::textColourId, juce::Colour(0xffaaaaaa));
    label.setBounds(x, y, w, h);
    addAndMakeVisible(label);
    // Labels are owned by the parent component via addAndMakeVisible.
}

void VoidChamberReverbAudioProcessorEditor::timerCallback()
{
    if (inputMeter) inputMeter->repaint();
    if (outputMeter) outputMeter->repaint();
}

//==============================================================================
void VoidChamberReverbAudioProcessorEditor::NeonLookAndFeel::drawRotarySlider(
    juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
    float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float>(x, y, width, height).toFloat().reduced(4);
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
    arc.addArc(bounds, rotaryStartAngle, angle);
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
