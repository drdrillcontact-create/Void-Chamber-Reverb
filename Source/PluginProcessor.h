#pragma once

#include <JuceHeader.h>
#include "ReverbDSP.h"

//==============================================================================
class VoidChamberReverbAudioProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    VoidChamberReverbAudioProcessor();
    ~VoidChamberReverbAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    //==============================================================================
    juce::AudioProcessorValueTreeState& getParameters() { return parameters; }

    // Meters (for editor)
    float getInputLevel() const noexcept { return inputLevel; }
    float getOutputLevel() const noexcept { return outputLevel; }

private:
    //==============================================================================
    juce::AudioProcessorValueTreeState parameters;
    std::unique_ptr<ReverbDSP> reverb;

    float inputLevel = 0.0f;
    float outputLevel = 0.0f;

    // Parameter IDs
    static constexpr char sizeID[]        = "size";
    static constexpr char preDelayID[]    = "preDelay";
    static constexpr char decayID[]       = "decay";
    static constexpr char dampingID[]     = "damping";
    static constexpr char modulationID[]  = "modulation";
    static constexpr char mixID[]         = "mix";
    static constexpr char spaceTypeID[]   = "spaceType";

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoidChamberReverbAudioProcessor)
};
