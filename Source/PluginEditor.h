#pragma once

#include "PluginProcessor.h"
#include "VoidGUI.h"
#include <array>
#include <memory>

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
    void updateValueLabels();
    void updatePresetName();
    void showPresetMenu();

    using SliderAttachment   = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ParameterAttachment = juce::ParameterAttachment;

    VoidChamberReverbAudioProcessor& processorRef;
    voidchamber::VoidLookAndFeel lookAndFeel;

    juce::Image background, orbVoid, orbHall, orbPlate, orbCavern, orbAmbient;

    // Top-right preset browser
    juce::TextButton presetPrevButton { "<" }, presetNextButton { ">" }, presetMenuButton;
    juce::Label presetNameLabel;

    // Meters
    voidchamber::NeonMeter inputMeter, outputMeter;

    // Mini mode switches
    juce::TextButton monoButton { "MONO" }, stereoButton { "STEREO" };

    // Main knobs
    juce::Slider sizeKnob, preDelayKnob, dampingKnob, modulationKnob;
    juce::Slider diffusionKnob, widthKnob, airKnob, mixKnob;

    // Central decay arc (doubles as the decay slider)
    voidchamber::DecayArcSlider decayArc;

    // Bottom-row components
    std::array<std::unique_ptr<voidchamber::SpaceTypeButton>, 5> spaceButtons;
    voidchamber::FrequencyDecayChart freqChart;
    voidchamber::CharacterButton characterButton;

    // Dynamic value readouts
    juce::Label sizeValue, preDelayValue, dampingValue, modulationValue;
    juce::Label diffusionValue, widthValue, airValue, mixValue;
    juce::Label characterValue;

    // Attachments (declared after the widgets they drive)
    std::unique_ptr<SliderAttachment> sizeAttach, preDelayAttach, dampingAttach, modulationAttach;
    std::unique_ptr<SliderAttachment> diffusionAttach, widthAttach, airAttach, mixAttach, decayAttach;
    std::unique_ptr<ParameterAttachment> spaceAttachment, characterAttachment, channelModeAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoidChamberReverbAudioProcessorEditor)
};
