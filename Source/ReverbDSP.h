#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <cmath>

//==============================================================================
// Void Chamber Reverb DSP core — written from scratch for this plugin.
//
// Signal chain per sample:
//
//   input ─► DC blocker ─► pre-delay (fractional) ─► 4 series input allpasses
//        ─► early reflections (6 feed-forward taps per side)
//        ─► 8-line Feedback Delay Network (normalized Hadamard mixing,
//           per-line damping lowpass, per-line subsonic highpass,
//           per-line slow LFO modulation with interpolated reads)
//        ─► output high-shelf ("Air") ─► mid/side width matrix
//        ─► equal-power dry/wet crossfade
//
// Every audible parameter is smoothed at block rate; delay feedback gains are
// recomputed from an exponential RT60 model: g = 10^(-3 * delay / RT60).
//==============================================================================

class SVF // one-pole low/high pass pair (stable inside the feedback loop)
{
public:
    void setLowpass (float cutoffHz, double sampleRate);
    void setHighpass (float cutoffHz, double sampleRate);
    void reset() { z1 = 0.0f; }
    float processLowpass (float sample);
    float processHighpass (float sample);

private:
    float a = 0.0f;
    float z1 = 0.0f;
};

// Ring buffer with interpolated read; writer wrap is branch-light.
class FractionalDelay
{
public:
    void prepare (float maxDelayMs, double sampleRate);
    void reset();
    void write (float sample);
    float readInterpolated (float delaySamples) const;

private:
    juce::AudioBuffer<float> buffer;
    int writePos = 0;
public:
    int getSize() const { return buffer.getNumSamples(); }
};

class FeedbackAllpass
{
public:
    void prepare (float maxDelayMs, double sampleRate);
    void reset();
    void setDelayMs (float ms, double sampleRate);
    float process (float input, float feedback);

private:
    FractionalDelay delay;
    float delaySamples = 100.0f;
};

class HighShelf
{
public:
    void setGainDb (float dB, float cutoffHz, double sampleRate);
    void reset() { x1 = x2 = y1 = y2 = 0.0f; }
    float process (float x);

private:
    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
    float x1 = 0.0f, x2 = 0.0f, y1 = 0.0f, y2 = 0.0f;
};

//==============================================================================
class ReverbDSP
{
public:
    static constexpr int numLines   = 8;   // FDN order
    static constexpr int numInputAP = 4;   // diffusion chain
    static constexpr int numERTaps  = 6;   // early reflection taps per channel

    void prepare (double newSampleRate);
    void reset();

    void setRoomSize (float size);        // 0..1 — scales all delay times
    void setPreDelayMs (float ms);        // 0..250 ms
    void setDecaySeconds (float seconds); // RT60, 0.1..20 s
    void setDamping (float amount);       // 0..1 — high-frequency absorption
    void setModulation (float amount);    // 0..1 — chorus depth of the FDN
    void setMix (float wet);              // 0..1
    void setDiffusion (float amount);     // 0..1 — allpass feedback amount
    void setWidth (float width);          // 0..2 (1 = unity)
    void setAir (float amount);           // 0..1 — high shelf on the wet path
    void setCharacter (int index);        // 0 Dark, 1 Neutral, 2 Bright
    void setMonoMode (bool mono);         // collapse wet to mono
    void setSpaceType (int index);        // 0 Void, 1 Hall, 2 Plate, 3 Cavern, 4 Ambient

    void process (juce::AudioBuffer<float>& buffer);

    // Estimated modal density is useful for tests only.
    static float referenceLineMs (int index) noexcept;

private:
    struct SpaceProfile
    {
        float delayScale;   // multiplies the base delay set
        float lpScale;      // scales the damping lowpass cutoff
        float hpHz;         // subsonic corner
        float modRate;      // base LFO rate in Hz
        float modDepth;     // base LFO depth in samples
        float erGain;       // early reflection level 0..1
        float erCutoffHz;   // early reflection lowpass
    };

    static constexpr int numSpaceTypes = 5;
    static const SpaceProfile profiles[numSpaceTypes];

    double sampleRate = 44100.0;

    // Pre-delay (independent L/R lines)
    FractionalDelay preDelayL, preDelayR;
    juce::SmoothedValue<float> preDelaySamples;

    // Input diffusion
    std::array<FeedbackAllpass, numInputAP> diffusionL, diffusionR;

    // Early reflections (per channel)
    FractionalDelay erDelayL, erDelayR;

    // FDN
    std::array<FractionalDelay, numLines> lines;
    std::array<SVF, numLines> dampingLP;
    std::array<SVF, numLines> subsonicHP;
    std::array<float, numLines> lineDelaySamples {};
    std::array<float, numLines> lineFeedback {};
    std::array<float, numLines> lfoPhase {};
    std::array<float, numLines> lfoRateMul {};

    HighShelf airShelfL, airShelfR;

    // Parameters (raw values, applied at block rate through smoothers)
    juce::SmoothedValue<float> mixS        { 0.5f };
    juce::SmoothedValue<float> widthS      { 1.0f };
    juce::SmoothedValue<float> airS        { 0.35f };
    juce::SmoothedValue<float> dampingS    { 0.5f };
    juce::SmoothedValue<float> modulationS { 0.0f };
    juce::SmoothedValue<float> diffusionS  { 0.7f };

    float roomSize = 0.5f;
    float decaySeconds = 2.0f;
    int   character = 1;
    int   spaceType = 0;
    bool  monoMode = false;

    void configureSpace();       // size/space/character dependent recalculation
    void updateDamping();        // damping lowpass cutoff from params
    void updateLineFeedback();   // RT60 model
    float characterTiltDb() const noexcept;
    float lineDelayMsBase (int index) const noexcept;
};
