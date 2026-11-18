#include "ReverbDSP.h"
#include <iostream>

// A comb tap is longer than a 64-sample host block. Re-applying the same host
// parameters must not erase its history, and a right-only input must stay right.
static bool impulseTest(int numChannels, int inputChannel, double sampleRate, float preDelayMs)
{
    ReverbDSP reverb;
    reverb.prepare(sampleRate);
    reverb.setMix(1.0f);
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

int main()
{
    if (!impulseTest(1, 0, 44100.0, 0.0f)
        || !impulseTest(2, 0, 48000.0, 200.0f)
        || !impulseTest(2, 1, 96000.0, 28.125f))
    {
        std::cerr << "Impulse test failed: check wet onset, tail, channel isolation and reset.\n";
        return 1;
    }
    return 0;
}
