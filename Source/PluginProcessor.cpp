#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <array>
#include <cmath>
#include <iterator>
#include <cstddef>

//==============================================================================
VoidChamberReverbAudioProcessor::VoidChamberReverbAudioProcessor()
    : AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
    ),
    parameters(*this, nullptr, juce::Identifier("VoidChamberReverb"), createParameterLayout())
{
}

VoidChamberReverbAudioProcessor::~VoidChamberReverbAudioProcessor() = default;

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
VoidChamberReverbAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { sizeID, 1 }, "Size", 0.0f, 1.0f, 0.72f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { preDelayID, 1 }, "Pre Delay", 0.0f, 200.0f, 28.0f)); // ms

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { decayID, 1 }, "Decay Time", 0.1f, 20.0f, 4.8f)); // seconds

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { dampingID, 1 }, "Damping", 0.0f, 1.0f, 0.46f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { modulationID, 1 }, "Modulation", 0.0f, 1.0f, 0.33f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { mixID, 1 }, "Mix", 0.0f, 1.0f, 0.75f));

    juce::StringArray spaceTypes{ "Void", "Hall", "Plate", "Cavern", "Ambient" };
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { spaceTypeID, 1 }, "Space Type", spaceTypes, 0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { diffusionID, 1 }, "Diffusion",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 68.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { widthID, 1 }, "Width",
        juce::NormalisableRange<float>(0.0f, 200.0f, 1.0f), 120.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { airID, 1 }, "Air",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 35.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    juce::StringArray characters{ "Dark", "Neutral", "Bright" };
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { characterID, 1 }, "Character", characters, 0));

    juce::StringArray channelModes{ "Stereo", "Mono" };
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { channelModeID, 1 }, "Channel Mode", channelModes, 0));

    return { params.begin(), params.end() };
}

//==============================================================================
namespace
{
    struct FactoryPreset
    {
        const char* name;
        float size, preDelay, decay, damping, modulation, mix;
        float diffusion, width, air;
        int character, spaceType, channelMode;
    };

    const FactoryPreset factoryPresets[VoidChamberReverbAudioProcessor::numPresets] =
    {
        // name            size  pre  decay damping mod   mix   diff  width air  char space  mode
        { "Astral Hall",   0.72f, 28.0f, 4.8f, 0.46f, 0.33f, 0.75f, 68.0f, 120.0f, 35.0f, 0, 0, 0 },
        { "Void Infinity", 0.90f, 32.0f, 12.0f, 0.22f, 0.48f, 0.80f, 82.0f, 135.0f, 50.0f, 0, 0, 0 },
        { "Plate Whisper", 0.55f, 18.0f, 1.6f, 0.30f, 0.20f, 0.45f, 75.0f, 110.0f, 45.0f, 1, 2, 0 },
        { "Cavern Cathedral", 1.00f, 45.0f, 7.2f, 0.55f, 0.40f, 0.70f, 85.0f, 125.0f, 30.0f, 0, 3, 0 },
        { "Ambient Drift", 0.65f, 60.0f, 8.5f, 0.35f, 0.60f, 0.65f, 60.0f, 140.0f, 55.0f, 2, 4, 0 },
        { "Neon Plate",    0.70f, 10.0f, 3.4f, 0.18f, 0.35f, 0.55f, 70.0f, 115.0f, 60.0f, 2, 2, 0 },
    };
}

const juce::String VoidChamberReverbAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

juce::String VoidChamberReverbAudioProcessor::getCurrentPresetName() const
{
    const auto index = currentPresetIndex.load(std::memory_order_relaxed);
    if (juce::isPositiveAndBelow(index, (int) std::size(factoryPresets)))
        return juce::String(factoryPresets[index].name);
    return {};
}

void VoidChamberReverbAudioProcessor::loadPreset(int index)
{
    const int n = (int) std::size(factoryPresets);
    index = ((index % n) + n) % n;
    const auto& preset = factoryPresets[index];

    const auto set = [this](const char* paramID, float plainValue)
    {
        if (auto* parameter = parameters.getParameter(paramID))
            parameter->setValueNotifyingHost(parameter->convertTo0to1(plainValue));
    };

    set(sizeID, preset.size);
    set(preDelayID, preset.preDelay);
    set(decayID, preset.decay);
    set(dampingID, preset.damping);
    set(modulationID, preset.modulation);
    set(mixID, preset.mix);
    set(diffusionID, preset.diffusion);
    set(widthID, preset.width);
    set(airID, preset.air);
    set(characterID, (float) preset.character);
    set(spaceTypeID, (float) preset.spaceType);
    set(channelModeID, (float) preset.channelMode);

    currentPresetIndex.store(index, std::memory_order_relaxed);
}

bool VoidChamberReverbAudioProcessor::acceptsMidi() const
{
    return false;
}

bool VoidChamberReverbAudioProcessor::producesMidi() const
{
    return false;
}

bool VoidChamberReverbAudioProcessor::isMidiEffect() const
{
    return false;
}

double VoidChamberReverbAudioProcessor::getTailLengthSeconds() const
{
    // Allow for the maximum RT60 and pre-delay when a host renders the tail.
    return 20.2;
}

int VoidChamberReverbAudioProcessor::getNumPrograms()
{
    return numPresets;
}

int VoidChamberReverbAudioProcessor::getCurrentProgram()
{
    return currentPresetIndex.load(std::memory_order_relaxed);
}

void VoidChamberReverbAudioProcessor::setCurrentProgram(int index)
{
    loadPreset(index);
}

const juce::String VoidChamberReverbAudioProcessor::getProgramName(int index)
{
    if (juce::isPositiveAndBelow(index, (int) std::size(factoryPresets)))
        return juce::String(factoryPresets[index].name);
    return {};
}

void VoidChamberReverbAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

//==============================================================================
void VoidChamberReverbAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused(samplesPerBlock);
    reverb.prepare(sampleRate);
    updateReverbParameters();
}

void VoidChamberReverbAudioProcessor::updateReverbParameters()
{
    reverb.setRoomSize(*parameters.getRawParameterValue(sizeID));
    reverb.setPreDelayMs(*parameters.getRawParameterValue(preDelayID));
    reverb.setDecaySeconds(*parameters.getRawParameterValue(decayID));
    reverb.setDamping(*parameters.getRawParameterValue(dampingID));
    reverb.setModulation(*parameters.getRawParameterValue(modulationID));
    reverb.setMix(*parameters.getRawParameterValue(mixID));
    reverb.setDiffusion(*parameters.getRawParameterValue(diffusionID) * 0.01f);
    reverb.setWidth(*parameters.getRawParameterValue(widthID) * 0.01f);
    reverb.setAir(*parameters.getRawParameterValue(airID) * 0.01f);
    reverb.setCharacter((int) std::lround(*parameters.getRawParameterValue(characterID)));
    reverb.setMonoMode(*parameters.getRawParameterValue(channelModeID) >= 0.5f);
}

void VoidChamberReverbAudioProcessor::releaseResources()
{
    reverb.reset();
}

bool VoidChamberReverbAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto output = layouts.getMainOutputChannelSet();
    return layouts.getMainInputChannelSet() == output
        && (output == juce::AudioChannelSet::mono() || output == juce::AudioChannelSet::stereo());
}

void VoidChamberReverbAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                                    juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);

    juce::ScopedNoDenormals noDenormals;
    const auto numChannels = getMainBusNumOutputChannels();
    const auto numSamples = buffer.getNumSamples();
    if (numChannels == 0 || numSamples == 0)
        return;

    updateReverbParameters();

    const auto meterLevel = [&]
    {
        float sum = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            const auto rms = buffer.getRMSLevel(ch, 0, numSamples);
            sum += rms * rms;
        }
        return juce::jlimit(0.0f, 1.0f, 3.0f * std::sqrt(sum / static_cast<float>(numChannels)));
    };

    inputLevel.store(meterLevel(), std::memory_order_relaxed);
    reverb.process(buffer);
    outputLevel.store(meterLevel(), std::memory_order_relaxed);
}

//==============================================================================
juce::AudioProcessorEditor* VoidChamberReverbAudioProcessor::createEditor()
{
    return new VoidChamberReverbAudioProcessorEditor(*this);
}

bool VoidChamberReverbAudioProcessor::hasEditor() const
{
    return true;
}

//==============================================================================
void VoidChamberReverbAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    state.setProperty("presetIndex", currentPresetIndex.load(std::memory_order_relaxed), nullptr);
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void VoidChamberReverbAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(parameters.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml(*xmlState);
            currentPresetIndex.store(tree.getProperty("presetIndex", 0), std::memory_order_relaxed);
            parameters.replaceState(tree);
        }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VoidChamberReverbAudioProcessor();
}
