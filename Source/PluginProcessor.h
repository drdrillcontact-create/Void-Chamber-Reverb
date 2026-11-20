#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
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

    using juce::AudioProcessor::processBlock;
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
    juce::AudioProcessorValueTreeState& getValueTreeState() noexcept { return parameters; }

    // Meters (for editor)
    float getInputLevel() const noexcept { return inputLevel.load(std::memory_order_relaxed); }
    float getOutputLevel() const noexcept { return outputLevel.load(std::memory_order_relaxed); }

    float getWetEnergy() const noexcept { return reverb.getWetEnergy(); }

    // --- Factory presets ---------------------------------------------------------
    // 50 presets in 5 categories x 10, ordered the same as the space-type orbs:
    // Void (0-9), Hall (10-19), Plate (20-29), Cavern (30-39), Ambient (40-49).
    static constexpr int numPresets = 50;
    static constexpr int presetsPerCategory = 10;
    static constexpr int numCategories = 5;

    int getNumPresets() const noexcept { return numPresets; }
    int getCurrentPresetIndex() const noexcept { return currentPresetIndex.load(std::memory_order_relaxed); }
    juce::String getCurrentPresetName() const;
    juce::String getCurrentCategoryName() const;

    static juce::String getPresetCategoryName(int categoryIndex);
    static juce::String getPresetName(int index);
    static int getPresetCategoryIndex(int index) noexcept { return juce::jlimit(0, numCategories - 1, index / presetsPerCategory); }

    void loadPreset(int index);
    void loadPreviousPreset();
    void loadNextPreset();

    // Parameter IDs
    static constexpr char sizeID[]        = "size";
    static constexpr char preDelayID[]    = "preDelay";
    static constexpr char decayID[]       = "decay";
    static constexpr char dampingID[]     = "damping";
    static constexpr char modulationID[]  = "modulation";
    static constexpr char mixID[]         = "mix";
    static constexpr char spaceTypeID[]   = "spaceType";
    static constexpr char diffusionID[]   = "diffusion";
    static constexpr char widthID[]       = "width";
    static constexpr char airID[]         = "air";
    static constexpr char characterID[]   = "character";
    static constexpr char channelModeID[] = "channelMode";


private:
    juce::AudioProcessorValueTreeState parameters;
    ReverbDSP reverb;
    std::atomic<float> inputLevel { 0.0f }, outputLevel { 0.0f };
    std::atomic<int> currentPresetIndex { 0 };

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void updateReverbParameters();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoidChamberReverbAudioProcessor)
};
