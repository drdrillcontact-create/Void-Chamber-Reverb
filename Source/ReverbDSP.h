#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>

// Simple one-pole lowpass used inside comb-filter feedback loops (damping).
class OnePoleLowpass
{
public:
    void setCutoff(float cutoffHz, double sampleRate)
    {
        coeff = juce::jlimit(0.001f, 0.999f, cutoffHz / static_cast<float>(sampleRate));
    }

    float process(float sample) { return history += coeff * (sample - history); }
    void reset() { history = 0.0f; }

private:
    float coeff = 0.5f;
    float history = 0.0f;
};

// Allocate for the largest room in prepare(); changing the tap must not clear
// the tail or resize a buffer on the audio thread.
class FilterDelay
{
public:
    void prepare(float maxDelayMs, double sampleRate);
    void setDelayMs(float delayMs, double sampleRate);
    void reset();

protected:
    int delaySamples = 1;
    float read() const;
    void push(float sample);

private:
    juce::AudioBuffer<float> buffer;
    int writePos = 0;
};

class CombFilter : public FilterDelay
{
public:
    void setDecay(float seconds, double sampleRate)
    {
        // A gain of -60 dB after `seconds` seconds of round trips.
        feedback = std::pow(0.001f, static_cast<float>(delaySamples / sampleRate) / seconds);
    }

    float process(float input, OnePoleLowpass& lpf)
    {
        const float out = read();
        push(input + lpf.process(out * feedback));
        return out;
    }

private:
    float feedback = 0.0f;
};

class AllpassFilter : public FilterDelay
{
public:
    float process(float input, float feedback)
    {
        const float out = read();
        const float next = input + out * feedback;
        push(next);
        return out - next * feedback;
    }
};

class ReverbDSP
{
public:
    void prepare(double newSampleRate);
    void reset();

    void setRoomSize(float size);              // 0..1
    void setPreDelayMs(float ms);              // 0..200 ms
    void setDecaySeconds(float seconds);       // RT60
    void setDamping(float amount);             // 0..1 (higher = more absorption)
    void setModulation(float amount);          // 0..1
    void setMix(float wet);                    // 0..1
    void process(juce::AudioBuffer<float>& buffer);

private:
    double sampleRate = 44100.0;
    juce::AudioBuffer<float> preDelayLine;
    int preDelayWritePos = 0;
    float preDelaySamples = 0.0f;

    struct Channel
    {
        std::array<CombFilter, 4> combs;
        std::array<OnePoleLowpass, 4> dampingFilters;
        std::array<AllpassFilter, 2> allpasses;
    };
    std::array<Channel, 2> channels;

    float mix = 0.5f;
    float roomSize = 0.5f;
    float decaySeconds = 2.0f;
    float damping = 0.5f;
    float modulation = 0.0f;
    float modPhase = 0.0f;

    void updateDelays();
    void updateDecay();
    void updateDamping();
};
