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
    const float cutoff = juce::jmap(damping, 20000.0f, 800.0f);
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
    auto* const* audio = buffer.getArrayOfWritePointers();
    auto* const* delay = preDelayLine.getArrayOfWritePointers();

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        int pos = preDelayWritePos - tap;
        if (pos < 0) pos += length;
        const int older = pos > 0 ? pos - 1 : length - 1;
        const float diffusion = 0.5f + modulation * 0.05f * std::sin(modPhase);
        modPhase += phaseStep;
        if (modPhase >= juce::MathConstants<float>::twoPi) modPhase -= juce::MathConstants<float>::twoPi;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float dry = audio[ch][i];
            delay[ch][preDelayWritePos] = dry;
            const float input = delay[ch][pos] + fraction * (delay[ch][older] - delay[ch][pos]);

            auto& channel = channels[static_cast<size_t>(ch)];
            float wet = 0.0f;
            for (size_t c = 0; c < channel.combs.size(); ++c)
                wet += channel.combs[c].process(input, channel.dampingFilters[c]);
            wet *= 0.25f;
            for (auto& ap : channel.allpasses)
                wet = ap.process(wet, diffusion);
            audio[ch][i] = dry * (1.0f - mix) + wet * mix;
        }
        if (++preDelayWritePos == length) preDelayWritePos = 0;
    }
}
