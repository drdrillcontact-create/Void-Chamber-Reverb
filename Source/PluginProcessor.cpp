#include "PluginProcessor.h"
#include "PluginEditor.h"

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
        juce::ParameterID { mixID, 1 }, "Mix", 0.0f, 1.0f, 0.6f));

    juce::StringArray spaceTypes{ "Void", "Hall", "Plate", "Cavern", "Ambient" };
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { spaceTypeID, 1 }, "Space Type", spaceTypes, 0));

    return { params.begin(), params.end() };
}

//==============================================================================
const juce::String VoidChamberReverbAudioProcessor::getName() const
{
    return JucePlugin_Name;
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
    return 1;
}

int VoidChamberReverbAudioProcessor::getCurrentProgram()
{
    return 0;
}

void VoidChamberReverbAudioProcessor::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
}

const juce::String VoidChamberReverbAudioProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
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
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void VoidChamberReverbAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VoidChamberReverbAudioProcessor();
}
