#include "ReverbDSP.h"

namespace
{
    // Delay times in milliseconds (not samples), detuned between channels.
    constexpr float combMs[][4] = { { 29.7f, 37.1f, 41.1f, 43.7f }, { 31.2f, 39.9f, 44.5f, 46.8f } };
    constexpr float allpassMs[][2] = { { 5.0f, 1.7f }, { 5.5f, 1.9f } };
    constexpr float combRoomScale = 0.15f, allpassRoomScale = 0.1f;
}

void FilterDelay::prepare(float maxDelayMs, double sampleRate)
{
    const auto maxTap = juce::jmax(1, juce::roundToInt(maxDelayMs * 0.001 * sampleRate));
    buffer.setSize(1, maxTap + 1);
    reset();
}

void FilterDelay::setDelayMs(float delayMs, double sampleRate)
{
    delaySamples = juce::jlimit(1, buffer.getNumSamples() - 1, juce::roundToInt(delayMs * 0.001 * sampleRate));
}

float FilterDelay::read() const
{
    const int pos = writePos - delaySamples;
    return buffer.getSample(0, pos >= 0 ? pos : pos + buffer.getNumSamples());
}

void FilterDelay::push(float sample)
{
    buffer.setSample(0, writePos, sample);
    if (++writePos == buffer.getNumSamples())
        writePos = 0;
}

void FilterDelay::reset()
{
    buffer.clear();
    writePos = 0;
}

void ReverbDSP::prepare(double newSampleRate)
{
    sampleRate = newSampleRate;
    preDelayLine.setSize(2, juce::roundToInt(0.2 * sampleRate) + 2);
    for (size_t ch = 0; ch < channels.size(); ++ch)
    {
        auto& channel = channels[ch];
        for (size_t i = 0; i < channel.combs.size(); ++i)
            channel.combs[i].prepare(combMs[ch][i] * (1.0f + combRoomScale), sampleRate);
        for (size_t i = 0; i < channel.allpasses.size(); ++i)
            channel.allpasses[i].prepare(allpassMs[ch][i] * (1.0f + allpassRoomScale), sampleRate);
    }
    for (auto& channel : channels)
        channel.airFilter.setCutoff(6500.0f, sampleRate);
    updateDelays();
    updateDamping();
    reset();
}

void ReverbDSP::reset()
{
    preDelayLine.clear();
    preDelayWritePos = 0;
    for (auto& channel : channels)
    {
        for (auto& comb : channel.combs) comb.reset();
        for (auto& ap : channel.allpasses) ap.reset();
        for (auto& filter : channel.dampingFilters) filter.reset();
        channel.airFilter.reset();
    }
    modPhase = 0.0f;
}

void ReverbDSP::setRoomSize(float size)
{
    size = juce::jlimit(0.0f, 1.0f, size);
    if (!juce::approximatelyEqual(roomSize, size))
    {
        roomSize = size;
        updateDelays();
    }
}

void ReverbDSP::setPreDelayMs(float ms)
{
    preDelaySamples = juce::jlimit(0.0f, 200.0f, ms) * 0.001f * static_cast<float>(sampleRate);
}

void ReverbDSP::setDecaySeconds(float seconds)
{
    seconds = juce::jlimit(0.1f, 20.0f, seconds);
    if (!juce::approximatelyEqual(decaySeconds, seconds))
    {
        decaySeconds = seconds;
        updateDecay();
    }
}

void ReverbDSP::setDamping(float amount)
{
    amount = juce::jlimit(0.0f, 1.0f, amount);
    if (!juce::approximatelyEqual(damping, amount))
    {
        damping = amount;
        updateDamping();
    }
}

void ReverbDSP::setModulation(float amount) { modulation = juce::jlimit(0.0f, 1.0f, amount); }
void ReverbDSP::setMix(float wet)           { mix = juce::jlimit(0.0f, 1.0f, wet); }
void ReverbDSP::setDiffusion(float amount)  { diffusion = juce::jlimit(0.0f, 1.0f, amount); }
void ReverbDSP::setWidth(float value)       { width = juce::jlimit(0.0f, 2.0f, value); }
void ReverbDSP::setAir(float amount)        { air = juce::jlimit(0.0f, 1.0f, amount); }
void ReverbDSP::setCharacter(int index)
{
    index = juce::jlimit(0, 2, index);
    if (character != index)
    {
        character = index;
        updateDamping();
    }
}
void ReverbDSP::setMonoMode(bool mono)      { monoMode = mono; }

void ReverbDSP::updateDelays()
{
    for (size_t ch = 0; ch < channels.size(); ++ch)
    {
        auto& channel = channels[ch];
        for (size_t i = 0; i < channel.combs.size(); ++i)
            channel.combs[i].setDelayMs(combMs[ch][i] * (1.0f + roomSize * combRoomScale), sampleRate);
        for (size_t i = 0; i < channel.allpasses.size(); ++i)
            channel.allpasses[i].setDelayMs(allpassMs[ch][i] * (1.0f + roomSize * allpassRoomScale), sampleRate);
    }
    updateDecay();
}

void ReverbDSP::updateDecay()
{
    for (auto& channel : channels)
        for (auto& comb : channel.combs)
            comb.setDecay(decaySeconds, sampleRate);
}

void ReverbDSP::updateDamping()
{
    float cutoff = juce::jmap(damping, 20000.0f, 800.0f);
    if (character == 0)      cutoff *= 0.72f;   // Dark: extra absorption up top
    else if (character == 2) cutoff = juce::jmin(20000.0f, cutoff * 1.45f); // Bright
    cutoff = juce::jlimit(200.0f, 20000.0f, cutoff);
    for (auto& channel : channels)
        for (auto& filter : channel.dampingFilters)
            filter.setCutoff(cutoff, sampleRate);
}

void ReverbDSP::process(juce::AudioBuffer<float>& buffer)
{
    const int numChannels = juce::jmin(buffer.getNumChannels(), static_cast<int>(channels.size()));
    const int length = preDelayLine.getNumSamples();
    const auto tap = static_cast<int>(preDelaySamples);
    const float fraction = preDelaySamples - static_cast<float>(tap);
    const float phaseStep = juce::MathConstants<float>::twoPi * 0.3f / static_cast<float>(sampleRate);
    const float wetGain = juce::jlimit(0.05f, 0.95f, 0.25f + 0.5f * diffusion);
    auto* const* audio = buffer.getArrayOfWritePointers();
    auto* const* delay = preDelayLine.getArrayOfWritePointers();
    const bool stereo = numChannels == 2;

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        const float dry0 = audio[0][i];
        const float dry1 = stereo ? audio[1][i] : dry0;

        // MONO mode feeds the average of both channels through a single chain;
        // output is identical in both channels.
        const float monoIn = stereo ? 0.5f * (dry0 + dry1) : dry0;
        const int channelCount = monoMode && stereo ? 1 : numChannels;

        int pos = preDelayWritePos - tap;
        if (pos < 0) pos += length;
        const int older = pos > 0 ? pos - 1 : length - 1;
        const float allpassFeedback = juce::jlimit(
            0.05f, 0.95f, wetGain + modulation * 0.06f * std::sin(modPhase));
        modPhase += phaseStep;
        if (modPhase >= juce::MathConstants<float>::twoPi) modPhase -= juce::MathConstants<float>::twoPi;

        delay[0][preDelayWritePos] = monoMode ? monoIn : dry0;
        if (stereo) delay[1][preDelayWritePos] = monoMode ? monoIn : dry1;

        float wet[2] = { 0.0f, 0.0f };
        const float dry[2] = { dry0, dry1 };
        for (int ch = 0; ch < channelCount; ++ch)
        {
            const float input = delay[ch][pos] + fraction * (delay[ch][older] - delay[ch][pos]);
            auto& channel = channels[static_cast<size_t>(ch)];
            float channelWet = 0.0f;
            for (size_t c = 0; c < channel.combs.size(); ++c)
                channelWet += channel.combs[c].process(input, channel.dampingFilters[c]);
            channelWet *= 0.25f;
            for (auto& ap : channel.allpasses)
                channelWet = ap.process(channelWet, allpassFeedback);

            // AIR: wet-path high-shelf, adding presence above ~6.5 kHz
            channelWet += air * 1.7f * (channelWet - channel.airFilter.process(channelWet));
            wet[ch] = channelWet;
        }
        if (monoMode && stereo) wet[1] = wet[0];

        if (stereo && !monoMode && std::abs(width - 1.0f) > 1.0e-3f)
        {
            const float mid = 0.5f * (wet[0] + wet[1]);
            const float side = 0.5f * (wet[0] - wet[1]) * width;
            wet[0] = mid + side;
            wet[1] = mid - side;
        }

        for (int ch = 0; ch < numChannels; ++ch)
            audio[ch][i] = dry[ch] * (1.0f - mix) + wet[ch] * mix;

        if (++preDelayWritePos == length) preDelayWritePos = 0;
    }
}
