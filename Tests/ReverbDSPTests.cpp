#include "ReverbDSP.h"
#include <iostream>

// A comb tap is longer than a 64-sample host block. Re-applying the same host
// parameters must not erase its history, and a right-only input must stay right
// (at unity width). Width above 100 % intentionally leaks anti-phase HF/side
// content into the opposite channel, so isolation is tested at 1.0x.
static bool impulseTest(int numChannels, int inputChannel, double sampleRate, float preDelayMs)
{
    ReverbDSP reverb;
    reverb.prepare(sampleRate);
    reverb.setMix(1.0f);
    reverb.setWidth(1.0f);
    reverb.setPreDelayMs(preDelayMs);

    constexpr int blockSize = 64;
    juce::AudioBuffer<float> block(numChannels, blockSize);
    int firstWet = -1, lastWet = -1;
    const auto preDelay = static_cast<int>(preDelayMs * 0.001 * sampleRate);
    for (int start = 0; start < preDelay + static_cast<int>(0.25 * sampleRate); start += blockSize)
    {
        block.clear();
        if (start == 0) block.setSample(inputChannel, 0, 1.0f);
        reverb.setRoomSize(0.72f);
        reverb.setDecaySeconds(4.8f);
        reverb.process(block);
        for (int ch = 0; ch < numChannels; ++ch)
            for (int i = 0; i < blockSize; ++i)
            {
                const float sample = block.getSample(ch, i);
                if (!std::isfinite(sample)) return false;
                if (std::abs(sample) > 1.0e-6f)
                {
                    if (ch != inputChannel) return false;
                    if (firstWet < 0) firstWet = start + i;
                    lastWet = start + i;
                }
            }
    }
    if (firstWet < preDelay || firstWet > preDelay + 0.05 * sampleRate
        || lastWet < preDelay + 0.1 * sampleRate)
        return false;

    reverb.reset();
    block.clear();
    reverb.process(block);
    return block.getMagnitude(0, blockSize) == 0.0f;
}

// In mono mode the wet path is identical in both channels (dry is passed
// through with the mix, so use full wet for the check).
static bool monoModeTest(double sampleRate)
{
    ReverbDSP reverb;
    reverb.prepare(sampleRate);
    reverb.setMix(1.0f);
    reverb.setWidth(1.7f);
    reverb.setMonoMode(true);

    juce::AudioBuffer<float> block(2, 256);
    block.clear();
    block.setSample(0, 0, 1.0f);
    reverb.process(block);
    for (int i = 0; i < 256; ++i)
        if (block.getSample(0, i) != block.getSample(1, i))
            return false;
    return true;
}

int main()
{
    if (!impulseTest(1, 0, 44100.0, 0.0f)
        || !impulseTest(2, 0, 48000.0, 200.0f)
        || !impulseTest(2, 1, 96000.0, 28.125f)
        || !monoModeTest(48000.0))
    {
        std::cerr << "Impulse/mono test failed: check wet onset, tail, channel isolation, mono collapse and reset.\n";
        return 1;
    }
    return 0;
}
