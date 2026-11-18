#pragma once

#include <JuceHeader.h>
#include <vector>
#include <array>

//==============================================================================
// Simple one-pole lowpass used inside comb-filter feedback loops (damping).
//==============================================================================
class OnePoleLowpass
{
public:
    OnePoleLowpass() = default;

    void setCutoff(float cutoffHz, double sampleRate)
    {
        auto fc = juce::jlimit(0.001f, 0.999f, cutoffHz / (float)sampleRate);
        coeff = fc;
    }

    inline float process(float sample)
    {
        history = history + coeff * (sample - history);
        return history;
    }

    inline void reset() { history = 0.0f; }

private:
    float coeff = 0.5f;
    float history = 0.0f;
};

//==============================================================================
// Comb filter with optional lowpass damping in the feedback path.
//==============================================================================
class CombFilter
{
public:
    CombFilter() = default;

    void setDelayMs(float delayMs, double sampleRate)
    {
        auto numSamples = (int)(delayMs * 0.001 * sampleRate);
        numSamples = juce::jlimit(1, 16384, numSamples);
        buffer.setSize(1, numSamples + 1, false, false, true);
        buffer.clear();
        writePos = 0;
        delaySamples = (float)numSamples;
    }

    inline float process(float input, float feedback, OnePoleLowpass& lpf)
    {
        auto readPos = writePos - (int)delaySamples;
        if (readPos < 0)
            readPos += buffer.getNumSamples();

        float out = buffer.getSample(0, readPos);

        // Damping: lowpass the feedback signal
        float dampened = lpf.process(out * feedback);
        buffer.setSample(0, writePos, input + dampened);

        writePos++;
        if (writePos >= buffer.getNumSamples())
            writePos = 0;

        return out;
    }

    inline void reset()
    {
        buffer.clear();
        writePos = 0;
    }

private:
    juce::AudioBuffer<float> buffer;
    int writePos = 0;
    float delaySamples = 0.0f;
};

//==============================================================================
// Allpass filter (Schroeder style).
//==============================================================================
class AllpassFilter
{
public:
    AllpassFilter() = default;

    void setDelayMs(float delayMs, double sampleRate)
    {
        auto numSamples = (int)(delayMs * 0.001 * sampleRate);
        numSamples = juce::jlimit(1, 4096, numSamples);
        buffer.setSize(1, numSamples + 1, false, false, true);
        buffer.clear();
        writePos = 0;
        delaySamples = (float)numSamples;
    }

    inline float process(float input, float feedback)
    {
        auto readPos = writePos - (int)delaySamples;
        if (readPos < 0)
            readPos += buffer.getNumSamples();

        float out = buffer.getSample(0, readPos);

        buffer.setSample(0, writePos, input + out * feedback);
        float result = out - input * feedback;

        writePos++;
        if (writePos >= buffer.getNumSamples())
            writePos = 0;

        return result;
    }

    inline void reset()
    {
        buffer.clear();
        writePos = 0;
    }

private:
    juce::AudioBuffer<float> buffer;
    int writePos = 0;
    float delaySamples = 0.0f;
};

//==============================================================================
// Stereo Schroeder reverb core.
//==============================================================================
class ReverbDSP
{
public:
    ReverbDSP() = default;

    void prepare(double sampleRate, int maxBlockSize);
    void reset();

    void setRoomSize(float size);              // 0..1
    void setPreDelayMs(float ms);              // 0..200 ms
    void setDecaySeconds(float seconds);       // RT60
    void setDamping(float amount);             // 0..1 (higher = more high freq absorption)
    void setModulation(float amount);          // 0..1
    void setMix(float wet);                    // 0..1

    // Process a stereo block. `wet` is applied internally from setMix.
    void process(juce::AudioBuffer<float>& buffer);

private:
    double sampleRate = 44100.0;

    // Predelay line (simple circular buffer with linear interpolation)
    std::vector<float> preDelayLine;
    int preDelayWritePos = 0;
    float preDelaySamples = 0.0f;

    // Four parallel comb filters per channel
    std::array<CombFilter, 4> combL;
    std::array<CombFilter, 4> combR;

    // Two series allpass filters per channel
    std::array<AllpassFilter, 2> apL;
    std::array<AllpassFilter, 2> apR;

    // Damping filters per comb
    std::array<OnePoleLowpass, 4> dampL;
    std::array<OnePoleLowpass, 4> dampR;

    float mix = 0.5f;
    float roomSize = 0.5f;
    float decaySeconds = 2.0f;
    float damping = 0.5f;
    float modulation = 0.0f;
    float modPhase = 0.0f;

    void updateCombDelays();
    void updateAllpassDelays();
    void updateDamping();
    inline float lerp(float a, float b, float t) { return a + (b - a) * t; }
};
