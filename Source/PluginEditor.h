#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
class VoidChamberReverbAudioProcessorEditor : public juce::AudioProcessorEditor,
                                              private juce::AudioProcessorValueTreeState::Listener
{
public:
    VoidChamberReverbAudioProcessorEditor(VoidChamberReverbAudioProcessor&);
    ~VoidChamberReverbAudioProcessorEditor() override;

    //==============================================================================
    void paint(juce::Graphics&) override;
    void resized() override;

    // ValueTree listener — keeps buttons in sync with the Space Type parameter
    void parameterValueChanged(int parameterIndex, float newValue) override;

private:
    //==============================================================================
    VoidChamberReverbAudioProcessor& processorRef;

    // Custom look-and-feel for dark neon style
    class NeonLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
                              float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                              juce::Slider&) override;
    };

    NeonLookAndFeel neonLaf;

    // Controls
    std::unique_ptr<juce::Slider> sizeSlider;
    std::unique_ptr<juce::Slider> preDelaySlider;
    std::unique_ptr<juce::Slider> decaySlider;
    std::unique_ptr<juce::Slider> dampingSlider;
    std::unique_ptr<juce::Slider> modulationSlider;
    std::unique_ptr<juce::Slider> mixSlider;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sizeAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> preDelayAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> decayAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> dampingAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> modulationAttach;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttach;

    // Space type buttons (exclusive toggle group)
    std::unique_ptr<juce::ToggleButton> btnVoid;
    std::unique_ptr<juce::ToggleButton> btnHall;
    std::unique_ptr<juce::ToggleButton> btnPlate;
    std::unique_ptr<juce::ToggleButton> btnCavern;
    std::unique_ptr<juce::ToggleButton> btnAmbient;

    std::unique_ptr<juce::ToggleButton> spaceButtons[5];

    // Meters
    std::unique_ptr<juce::Component> inputMeter;
    std::unique_ptr<juce::Component> outputMeter;

    // Label helper — creates and adds a label, does not store it
    void addLabel(const juce::String& text, int x, int y, int w, int h);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoidChamberReverbAudioProcessorEditor)
};
