#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
constexpr char VoidChamberReverbAudioProcessor::sizeID[];
constexpr char VoidChamberReverbAudioProcessor::preDelayID[];
constexpr char VoidChamberReverbAudioProcessor::decayID[];
constexpr char VoidChamberReverbAudioProcessor::dampingID[];
constexpr char VoidChamberReverbAudioProcessor::modulationID[];
constexpr char VoidChamberReverbAudioProcessor::mixID[];
constexpr char VoidChamberReverbAudioProcessor::spaceTypeID[];

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
    reverb = std::make_unique<ReverbDSP>();
}

VoidChamberReverbAudioProcessor::~VoidChamberReverbAudioProcessor() = default;

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
VoidChamberReverbAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        sizeID, "Size", 0.0f, 1.0f, 0.72f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        preDelayID, "Pre Delay", 0.0f, 200.0f, 28.0f)); // ms

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        decayID, "Decay Time", 0.1f, 20.0f, 4.8f)); // seconds

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        dampingID, "Damping", 0.0f, 1.0f, 0.46f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        modulationID, "Modulation", 0.0f, 1.0f, 0.33f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        mixID, "Mix", 0.0f, 1.0f, 0.6f));

    juce::StringArray spaceTypes{ "Void", "Hall", "Plate", "Cavern", "Ambient" };
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        spaceTypeID, "Space Type", spaceTypes, 0));

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
    return 1.0;
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
    reverb->prepare(sampleRate, (int)samplesPerBlock);

    // Apply current parameter values
    reverb->setRoomSize(*parameters.getRawParameterValue(sizeID));
    reverb->setPreDelayMs(*parameters.getRawParameterValue(preDelayID));
    reverb->setDecaySeconds(*parameters.getRawParameterValue(decayID));
    reverb->setDamping(*parameters.getRawParameterValue(dampingID));
    reverb->setModulation(*parameters.getRawParameterValue(modulationID));
    reverb->setMix(*parameters.getRawParameterValue(mixID));
}

void VoidChamberReverbAudioProcessor::releaseResources()
{
    reverb->reset();
}

bool VoidChamberReverbAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // Only stereo in/out (or mono)
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono();
}

void VoidChamberReverbAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                                    juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear any output channels that don't have input
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Read parameters (atomic)
    reverb->setRoomSize(*parameters.getRawParameterValue(sizeID));
    reverb->setPreDelayMs(*parameters.getRawParameterValue(preDelayID));
    reverb->setDecaySeconds(*parameters.getRawParameterValue(decayID));
    reverb->setDamping(*parameters.getRawParameterValue(dampingID));
    reverb->setModulation(*parameters.getRawParameterValue(modulationID));
    reverb->setMix(*parameters.getRawParameterValue(mixID));

    // Metering: simple RMS
    float inRMS = 0.0f;
    for (int ch = 0; ch < totalNumInputChannels; ++ch)
    {
        auto* data = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            inRMS += data[i] * data[i];
    }
    inRMS = std::sqrt(inRMS / (buffer.getNumSamples() * totalNumInputChannels));
    inputLevel = juce::jlimit(0.0f, 1.0f, inRMS * 3.0f);

    reverb->process(buffer);

    float outRMS = 0.0f;
    for (int ch = 0; ch < totalNumOutputChannels; ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            outRMS += data[i] * data[i];
    }
    outRMS = std::sqrt(outRMS / (buffer.getNumSamples() * totalNumOutputChannels));
    outputLevel = juce::jlimit(0.0f, 1.0f, outRMS * 3.0f);
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
        if (xmlState->hasTagName(parameters.getState().getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VoidChamberReverbAudioProcessor();
}
