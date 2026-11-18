#pragma once

#include "PluginProcessor.h"
#include <array>

class VoidChamberReverbAudioProcessorEditor : public juce::AudioProcessorEditor,
                                              private juce::Timer
{
public:
    explicit VoidChamberReverbAudioProcessorEditor(VoidChamberReverbAudioProcessor&);
    ~VoidChamberReverbAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    class NeonLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
                              float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                              juce::Slider&) override;
    };

    VoidChamberReverbAudioProcessor& processorRef;
    NeonLookAndFeel neonLaf;
    std::array<juce::Slider, 6> knobs;
    std::array<juce::Label, 6> labels;
    std::array<juce::ToggleButton, 5> spaceButtons;

    // Attachments must be destroyed before the controls they reference.
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::array<std::unique_ptr<SliderAttachment>, 6> knobAttachments;
    juce::ParameterAttachment spaceAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoidChamberReverbAudioProcessorEditor)
};
