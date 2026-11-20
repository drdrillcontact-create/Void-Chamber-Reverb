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
        juce::ParameterID { sizeID, 1 }, "Size",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.72f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { preDelayID, 1 }, "Pre Delay",
        juce::NormalisableRange<float>(0.0f, 250.0f, 0.1f, 0.7f), 28.0f,
        juce::AudioParameterFloatAttributes().withLabel("ms")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { decayID, 1 }, "Decay Time",
        juce::NormalisableRange<float>(0.1f, 20.0f, 0.01f, 0.4f), 4.8f,
        juce::AudioParameterFloatAttributes().withLabel("s")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { dampingID, 1 }, "Damping",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.46f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { modulationID, 1 }, "Modulation",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.33f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { mixID, 1 }, "Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.75f));

    juce::StringArray spaceTypes{ "Void", "Hall", "Plate", "Cavern", "Ambient" };
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { spaceTypeID, 1 }, "Space Type", spaceTypes, 0));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { diffusionID, 1 }, "Diffusion",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 68.0f,
        juce::AudioParameterFloatAttributes().withLabel("%")));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { widthID, 1 }, "Width",
        juce::NormalisableRange<float>(0.0f, 200.0f, 0.1f), 120.0f,
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
        const char* category;
        const char* name;
        float size, preDelay, decay, damping, modulation, mix;
        float diffusion, width, air;
        int character, spaceType, channelMode;
    };

    // 5 categories x 10 presets, ordered as the space-type orbs: Void, Hall,
    // Plate, Cavern, Ambient. All values are hand-voiced, not randomised.
    const FactoryPreset factoryPresets[] =
    {
        // ---- Void (spaceType = 0) --------------------------------------------
        { "Void", "Void Infinity",   0.90f, 32.0f, 12.0f, 0.22f, 0.48f, 0.80f, 82.0f, 135.0f, 50.0f, 0, 0, 0 },
        { "Void", "Event Horizon",   1.00f, 40.0f, 15.0f, 0.18f, 0.55f, 0.82f, 85.0f, 150.0f, 55.0f, 1, 0, 0 },
        { "Void", "Dark Matter",     0.85f, 28.0f,  9.5f, 0.15f, 0.50f, 0.78f, 80.0f, 140.0f, 60.0f, 2, 0, 0 },
        { "Void", "Quantum Silence", 0.70f, 20.0f,  6.5f, 0.30f, 0.45f, 0.70f, 76.0f, 130.0f, 38.0f, 0, 0, 0 },
        { "Void", "Singularity",     0.95f, 35.0f, 16.5f, 0.12f, 0.60f, 0.85f, 88.0f, 160.0f, 65.0f, 2, 0, 0 },
        { "Void", "Astral Rift",     0.78f, 26.0f,  7.5f, 0.26f, 0.52f, 0.74f, 79.0f, 142.0f, 58.0f, 1, 0, 0 },
        { "Void", "Void Bloom",      0.65f, 18.0f,  4.2f, 0.32f, 0.58f, 0.66f, 73.0f, 125.0f, 64.0f, 2, 0, 0 },
        { "Void", "The Between",     0.88f, 30.0f, 10.5f, 0.20f, 0.42f, 0.80f, 83.0f, 148.0f, 46.0f, 0, 0, 0 },
        { "Void", "Hyperspace",      0.92f, 24.0f, 13.5f, 0.16f, 0.65f, 0.82f, 86.0f, 155.0f, 70.0f, 2, 0, 0 },
        { "Void", "Cosmic Breath",   0.72f, 22.0f,  5.8f, 0.28f, 0.50f, 0.71f, 78.0f, 135.0f, 54.0f, 1, 0, 0 },

        // ---- Hall (spaceType = 1) ---------------------------------------------
        { "Hall", "Astral Hall",          0.72f, 28.0f, 4.8f, 0.46f, 0.33f, 0.75f, 68.0f, 120.0f, 35.0f, 0, 1, 0 },
        { "Hall", "Concert Grand",        0.78f, 24.0f, 2.4f, 0.42f, 0.18f, 0.30f, 62.0f, 115.0f, 45.0f, 1, 1, 0 },
        { "Hall", "Marble Philharmonic",  0.85f, 32.0f, 3.1f, 0.50f, 0.22f, 0.35f, 66.0f, 125.0f, 30.0f, 0, 1, 0 },
        { "Hall", "Velvet Opera House",   0.70f, 26.0f, 1.9f, 0.55f, 0.25f, 0.30f, 60.0f, 110.0f, 25.0f, 0, 1, 0 },
        { "Hall", "Neon Cathedral Hall",  0.90f, 36.0f, 5.2f, 0.38f, 0.40f, 0.45f, 72.0f, 135.0f, 50.0f, 2, 1, 0 },
        { "Hall", "Orion Symphony",       0.95f, 42.0f, 6.0f, 0.44f, 0.30f, 0.50f, 75.0f, 140.0f, 40.0f, 1, 1, 0 },
        { "Hall", "Silent Atrium",        0.55f, 18.0f, 1.2f, 0.60f, 0.15f, 0.25f, 55.0f, 100.0f, 20.0f, 0, 1, 0 },
        { "Hall", "Starlight Auditorium", 0.82f, 30.0f, 3.8f, 0.40f, 0.35f, 0.40f, 68.0f, 130.0f, 55.0f, 2, 1, 0 },
        { "Hall", "Grand Chamber",        0.68f, 22.0f, 1.6f, 0.48f, 0.20f, 0.28f, 58.0f, 105.0f, 30.0f, 1, 1, 0 },
        { "Hall", "Celestial Foyer",      0.60f, 12.0f, 0.9f, 0.52f, 0.18f, 0.22f, 52.0f, 100.0f, 35.0f, 1, 1, 0 },

        // ---- Plate (spaceType = 2) --------------------------------------------
        { "Plate", "Plate Whisper",      0.55f, 18.0f, 1.6f, 0.30f, 0.20f, 0.45f, 75.0f, 110.0f, 45.0f, 1, 2, 0 },
        { "Plate", "Neon Plate",         0.70f, 10.0f, 3.4f, 0.18f, 0.35f, 0.55f, 70.0f, 115.0f, 60.0f, 2, 2, 0 },
        { "Plate", "Silver Ghost",       0.50f,  8.0f, 1.1f, 0.25f, 0.30f, 0.40f, 78.0f, 105.0f, 55.0f, 2, 2, 0 },
        { "Plate", "Vintage Tank 140",   0.65f,  4.0f, 2.6f, 0.35f, 0.28f, 0.50f, 80.0f, 120.0f, 30.0f, 0, 2, 0 },
        { "Plate", "Vocal Sheet",        0.45f, 14.0f, 1.4f, 0.32f, 0.22f, 0.35f, 72.0f, 100.0f, 40.0f, 1, 2, 0 },
        { "Plate", "Chrome Ambience",    0.72f, 20.0f, 3.9f, 0.15f, 0.42f, 0.60f, 76.0f, 125.0f, 65.0f, 2, 2, 0 },
        { "Plate", "Blue Plate Solo",    0.58f, 12.0f, 2.0f, 0.28f, 0.25f, 0.48f, 74.0f, 112.0f, 38.0f, 0, 2, 0 },
        { "Plate", "Shimmer Plate",      0.66f, 16.0f, 4.5f, 0.12f, 0.50f, 0.62f, 82.0f, 130.0f, 70.0f, 2, 2, 0 },
        { "Plate", "Percussion Plate",   0.40f,  6.0f, 0.8f, 0.40f, 0.15f, 0.30f, 68.0f,  95.0f, 25.0f, 1, 2, 0 },
        { "Plate", "Echo Chamber Plate", 0.80f, 28.0f, 5.5f, 0.20f, 0.38f, 0.58f, 79.0f, 135.0f, 48.0f, 1, 2, 0 },

        // ---- Cavern (spaceType = 3) -------------------------------------------
        { "Cavern", "Cavern Cathedral", 1.00f, 45.0f,  7.2f, 0.55f, 0.40f, 0.70f, 85.0f, 125.0f, 30.0f, 0, 3, 0 },
        { "Cavern", "Obsidian Depths",  0.95f, 55.0f,  8.8f, 0.62f, 0.30f, 0.72f, 82.0f, 118.0f, 22.0f, 0, 3, 0 },
        { "Cavern", "Thunder Hollow",   1.00f, 38.0f,  5.6f, 0.50f, 0.45f, 0.65f, 80.0f, 130.0f, 28.0f, 1, 3, 0 },
        { "Cavern", "Whispering Grotto",0.75f, 25.0f,  3.4f, 0.58f, 0.50f, 0.55f, 76.0f, 115.0f, 35.0f, 0, 3, 0 },
        { "Cavern", "Subterranean Choir",0.90f,48.0f,  6.8f, 0.48f, 0.55f, 0.68f, 84.0f, 140.0f, 32.0f, 1, 3, 0 },
        { "Cavern", "Magma Throat",     0.85f, 33.0f,  4.2f, 0.38f, 0.35f, 0.60f, 72.0f, 108.0f, 18.0f, 0, 3, 0 },
        { "Cavern", "Frozen Abyss",     1.00f, 62.0f,  9.5f, 0.68f, 0.28f, 0.75f, 88.0f, 145.0f, 26.0f, 0, 3, 0 },
        { "Cavern", "Echoing Chasm",    0.92f, 50.0f,  6.0f, 0.45f, 0.42f, 0.66f, 78.0f, 122.0f, 34.0f, 1, 3, 0 },
        { "Cavern", "Druid's Vault",    0.78f, 28.0f,  2.8f, 0.52f, 0.30f, 0.50f, 70.0f, 112.0f, 30.0f, 0, 3, 0 },
        { "Cavern", "Titan's Cavern",   1.00f, 70.0f, 12.0f, 0.60f, 0.48f, 0.78f, 90.0f, 150.0f, 40.0f, 1, 3, 0 },

        // ---- Ambient (spaceType = 4) -------------------------------------------
        { "Ambient", "Ambient Drift",    0.65f, 60.0f,  8.5f, 0.35f, 0.60f, 0.65f, 60.0f, 140.0f, 55.0f, 2, 4, 0 },
        { "Ambient", "Weightless",       0.70f, 75.0f, 10.0f, 0.30f, 0.68f, 0.70f, 58.0f, 155.0f, 62.0f, 2, 4, 0 },
        { "Ambient", "Slow Tides",       0.55f, 45.0f,  6.2f, 0.40f, 0.52f, 0.60f, 62.0f, 130.0f, 48.0f, 1, 4, 0 },
        { "Ambient", "Liminal Spaces",   0.62f, 52.0f,  7.0f, 0.42f, 0.58f, 0.63f, 55.0f, 138.0f, 44.0f, 0, 4, 0 },
        { "Ambient", "Ultraviolet Haze", 0.68f, 40.0f,  5.5f, 0.25f, 0.65f, 0.58f, 65.0f, 145.0f, 68.0f, 2, 4, 0 },
        { "Ambient", "Nocturne Drift",   0.60f, 66.0f,  9.0f, 0.38f, 0.70f, 0.70f, 52.0f, 148.0f, 50.0f, 1, 4, 0 },
        { "Ambient", "Celestial Blanket",0.75f, 35.0f,  4.5f, 0.32f, 0.50f, 0.55f, 60.0f, 135.0f, 58.0f, 2, 4, 0 },
        { "Ambient", "Gravity Well",     0.80f, 30.0f, 11.0f, 0.28f, 0.60f, 0.72f, 57.0f, 142.0f, 52.0f, 0, 4, 0 },
        { "Ambient", "Ether Bloom",      0.50f, 24.0f,  3.2f, 0.22f, 0.55f, 0.50f, 68.0f, 128.0f, 72.0f, 2, 4, 0 },
        { "Ambient", "Still Horizon",    0.58f, 50.0f,  5.0f, 0.45f, 0.40f, 0.52f, 63.0f, 125.0f, 42.0f, 1, 4, 0 },
    };

    static_assert(std::size(factoryPresets) == VoidChamberReverbAudioProcessor::numPresets,
                  "Preset table must contain exactly numPresets entries");

    const juce::String categoryNames[VoidChamberReverbAudioProcessor::numCategories] =
        { "Void", "Hall", "Plate", "Cavern", "Ambient" };
}

const juce::String VoidChamberReverbAudioProcessor::getName() const
{
   #ifdef JucePlugin_Name
    return JucePlugin_Name;
   #else
    return "Void Chamber Reverb";
   #endif
}

juce::String VoidChamberReverbAudioProcessor::getPresetCategoryName(int categoryIndex)
{
    return categoryNames[static_cast<size_t>(juce::jlimit(0, numCategories - 1, categoryIndex))];
}

juce::String VoidChamberReverbAudioProcessor::getPresetName(int index)
{
    if (juce::isPositiveAndBelow(index, static_cast<int>(std::size(factoryPresets))))
        return juce::String(factoryPresets[static_cast<size_t>(index)].name);
    return {};
}

juce::String VoidChamberReverbAudioProcessor::getCurrentPresetName() const
{
    return getPresetName(currentPresetIndex.load(std::memory_order_relaxed));
}

juce::String VoidChamberReverbAudioProcessor::getCurrentCategoryName() const
{
    return getPresetCategoryName(getPresetCategoryIndex(currentPresetIndex.load(std::memory_order_relaxed)));
}

void VoidChamberReverbAudioProcessor::loadPreset(int index)
{
    index = ((index % numPresets) + numPresets) % numPresets;
    const auto& preset = factoryPresets[static_cast<size_t>(index)];

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
    set(characterID, static_cast<float>(preset.character));
    set(spaceTypeID, static_cast<float>(preset.spaceType));
    set(channelModeID, static_cast<float>(preset.channelMode));

    currentPresetIndex.store(index, std::memory_order_relaxed);
    // Let the host know every parameter changed so its cache stays in sync.
    updateHostDisplay(juce::AudioProcessorListener::ChangeDetails()
                          .withProgramChanged(true)
                          .withParameterInfoChanged(true));
}

void VoidChamberReverbAudioProcessor::loadPreviousPreset()
{
    loadPreset(currentPresetIndex.load(std::memory_order_relaxed) - 1);
}

void VoidChamberReverbAudioProcessor::loadNextPreset()
{
    loadPreset(currentPresetIndex.load(std::memory_order_relaxed) + 1);
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
    return 20.3;
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
    if (juce::isPositiveAndBelow(index, numPresets))
    {
        const auto& preset = factoryPresets[static_cast<size_t>(index)];
        return juce::String(preset.category) + " - " + preset.name;
    }
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
    reverb.setCharacter(static_cast<int>(std::lround(*parameters.getRawParameterValue(characterID))));
    reverb.setSpaceType(static_cast<int>(std::lround(*parameters.getRawParameterValue(spaceTypeID))));
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
        // Unboosted block sample peak, in linear full-scale units. Meter
        // calibration must not include the old cosmetic 3x RMS multiplier.
        float peak = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
            peak = juce::jmax (peak, buffer.getMagnitude (ch, 0, numSamples));
        return peak;
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
            // A stored preset index that no longer fits the table is clamped
            // rather than dropped, so old projects keep loading.
            currentPresetIndex.store(juce::jlimit(0, numPresets - 1,
                                                  static_cast<int>(tree.getProperty("presetIndex", 0))),
                                     std::memory_order_relaxed);
            parameters.replaceState(tree);
        }
}

//==============================================================================
// This creates new instances of the plugin.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VoidChamberReverbAudioProcessor();
}
