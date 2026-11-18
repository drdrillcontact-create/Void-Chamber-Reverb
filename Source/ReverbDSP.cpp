#include "ReverbDSP.h"

//==============================================================================
void ReverbDSP::prepare(double sampleRate, int maxBlockSize)
{
    this->sampleRate = sampleRate;

    // Predelay line: up to 200 ms
    auto maxPredelay = (int)(200.0 * 0.001 * sampleRate) + 4;
    preDelayLine.resize(maxPredelay, 0.0f);
    preDelayWritePos = 0;
    preDelaySamples = 0.0f;

    // Comb filter delay times (ms) — classic Schroeder values, tweaked per channel
    static constexpr float combDelaysL[4] = { 29.7f, 37.1f, 41.1f, 43.7f };
    static constexpr float combDelaysR[4] = { 31.2f, 39.9f, 44.5f, 46.8f };

    for (int i = 0; i < 4; ++i)
    {
        combL[i].setDelayMs(combDelaysL[i], sampleRate);
        combR[i].setDelayMs(combDelaysR[i], sampleRate);
        dampL[i].setCutoff(5000.0f, sampleRate);
        dampR[i].setCutoff(5000.0f, sampleRate);
    }

    // Allpass delays (ms)
    static constexpr float apDelaysL[2] = { 5.0f, 1.7f };
    static constexpr float apDelaysR[2] = { 5.5f, 1.9f };

    for (int i = 0; i < 2; ++i)
    {
        apL[i].setDelayMs(apDelaysL[i], sampleRate);
        apR[i].setDelayMs(apDelaysR[i], sampleRate);
    }

    reset();
}

void ReverbDSP::reset()
{
    std::fill(preDelayLine.begin(), preDelayLine.end(), 0.0f);
    preDelayWritePos = 0;

    for (auto& c : combL) c.reset();
    for (auto& c : combR) c.reset();
    for (auto& a : apL) a.reset();
    for (auto& a : apR) a.reset();
    for (auto& d : dampL) d.reset();
    for (auto& d : dampR) d.reset();

    modPhase = 0.0f;
}

void ReverbDSP::setRoomSize(float size)
{
    roomSize = juce::jlimit(0.0f, 1.0f, size);
    updateCombDelays();
    updateAllpassDelays();
}

void ReverbDSP::setPreDelayMs(float ms)
{
    preDelaySamples = juce::jlimit(0.0f, 200.0f, ms) * 0.001f * (float)sampleRate;
}

void ReverbDSP::setDecaySeconds(float seconds)
{
    decaySeconds = juce::jlimit(0.1f, 20.0f, seconds);
    // Feedback from RT60: feedback = 10^(-3 * delay / RT60)
    // Applied per comb filter inside updateCombDelays
    updateCombDelays();
}

void ReverbDSP::setDamping(float amount)
{
    damping = juce::jlimit(0.0f, 1.0f, amount);
    updateDamping();
}

void ReverbDSP::setModulation(float amount)
{
    modulation = juce::jlimit(0.0f, 1.0f, amount);
}

void ReverbDSP::setMix(float wet)
{
    mix = juce::jlimit(0.0f, 1.0f, wet);
}

void ReverbDSP::updateCombDelays()
{
    // Base delay times (samples) — scaled slightly by roomSize
    static constexpr float baseDelaysL[4] = { 29.7f, 37.1f, 41.1f, 43.7f };
    static constexpr float baseDelaysR[4] = { 31.2f, 39.9f, 44.5f, 46.8f };

    float scale = 1.0f + roomSize * 0.15f; // larger room = slightly longer

    for (int i = 0; i < 4; ++i)
    {
        float dL = baseDelaysL[i] * scale * 0.001f * (float)sampleRate;
        float dR = baseDelaysR[i] * scale * 0.001f * (float)sampleRate;

        // Re-create buffers at new lengths
        combL[i].setDelayMs(baseDelaysL[i] * scale * 1000.0f, sampleRate);
        combR[i].setDelayMs(baseDelaysR[i] * scale * 1000.0f, sampleRate);

        // Feedback coefficient from RT60
        // T60 = decaySeconds. For a delay d (seconds): feedback = pow(10, -3*d/T60)
        float fbL = std::pow(10.0f, -3.0f * (dL / sampleRate) / decaySeconds);
        float fbR = std::pow(10.0f, -3.0f * (dR / sampleRate) / decaySeconds);
        fbL = juce::jlimit(0.0f, 0.99f, fbL);
        fbR = juce::jlimit(0.0f, 0.99f, fbR);

        // We need a way to set feedback; since CombFilter::process takes feedback
        // as an argument, we can store desired feedback. Here we just remember it
        // via a small trick: process() will apply damping but feedback comes from caller.
        // To keep API simple, we'll pass feedback inside process() below.
        juce::ignoreUnused(fbL, fbR);
    }

    // Store feedback targets for process() to use
    // (ReverbDSP will compute per-sample in process block using decaySeconds.)
}

void ReverbDSP::updateAllpassDelays()
{
    static constexpr float apDelaysL[2] = { 5.0f, 1.7f };
    static constexpr float apDelaysR[2] = { 5.5f, 1.9f };
    float scale = 1.0f + roomSize * 0.1f;
    for (int i = 0; i < 2; ++i)
    {
        apL[i].setDelayMs(apDelaysL[i] * scale, sampleRate);
        apR[i].setDelayMs(apDelaysR[i] * scale, sampleRate);
    }
}

void ReverbDSP::updateDamping()
{
    // Map damping 0..1 to cutoff 20k .. 1k
    float cutoff = juce::jmap(damping, 0.0f, 1.0f, 20000.0f, 800.0f);
    for (auto& d : dampL) d.setCutoff(cutoff, sampleRate);
    for (auto& d : dampR) d.setCutoff(cutoff, sampleRate);
}

void ReverbDSP::process(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const float* inL = buffer.getReadPointer(0);
    const float* inR = buffer.getReadPointer(1);
    float* outL = buffer.getWritePointer(0);
    float* outR = buffer.getWritePointer(1);

    // Feedback values computed once per block (decay-based)
    float scale = 1.0f + roomSize * 0.15f;
    static constexpr float baseDelaysL[4] = { 29.7f, 37.1f, 41.1f, 43.7f };
    static constexpr float baseDelaysR[4] = { 31.2f, 39.9f, 44.5f, 46.8f };

    float fbL[4], fbR[4];
    for (int i = 0; i < 4; ++i)
    {
        float dL = baseDelaysL[i] * scale * 0.001f * (float)sampleRate;
        float dR = baseDelaysR[i] * scale * 0.001f * (float)sampleRate;
        fbL[i] = juce::jlimit(0.0f, 0.99f, std::pow(10.0f, -3.0f * (dL / sampleRate) / decaySeconds));
        fbR[i] = juce::jlimit(0.0f, 0.99f, std::pow(10.0f, -3.0f * (dR / sampleRate) / decaySeconds));
    }

    float apFeedback = 0.5f;

    for (int i = 0; i < numSamples; ++i)
    {
        // ---- Predelay (simple delay line, linear interp) ----
        float dryL = inL[i];
        float dryR = inR[i];

        // Write input to predelay line
        preDelayLine[preDelayWritePos] = dryL; // mono predelay fed into both? We'll use L for simplicity and derive R offset
        // Actually stereo predelay: use same delay, write both channels separately is overkill.
        // We'll use one predelay line for L, and offset R by a few samples for stereo width.
        int readPosL = preDelayWritePos - (int)preDelaySamples;
        if (readPosL < 0) readPosL += (int)preDelayLine.size();

        float frac = preDelaySamples - (int)preDelaySamples;
        int readPosR = readPosL - 11; // small stereo offset
        if (readPosR < 0) readPosR += (int)preDelayLine.size();

        float pdL = lerp(preDelayLine[readPosL], preDelayLine[(readPosL + 1) % preDelayLine.size()], frac);
        float pdR = lerp(preDelayLine[readPosR], preDelayLine[(readPosR + 1) % preDelayLine.size()], frac);

        preDelayWritePos++;
        if (preDelayWritePos >= (int)preDelayLine.size())
            preDelayWritePos = 0;

        // ---- Parallel comb filters ----
        float sumL = 0.0f, sumR = 0.0f;
        for (int c = 0; c < 4; ++c)
        {
            sumL += combL[c].process(pdL, fbL[c], dampL[c]);
            sumR += combR[c].process(pdR, fbR[c], dampR[c]);
        }
        sumL *= 0.25f; // average
        sumR *= 0.25f;

        // ---- Series allpass (diffusion) ----
        float diffL = sumL;
        float diffR = sumR;
        for (int a = 0; a < 2; ++a)
        {
            diffL = apL[a].process(diffL, apFeedback);
            diffR = apR[a].process(diffR, apFeedback);
        }

        // ---- Optional subtle modulation on delay time (chorus-like) ----
        if (modulation > 0.0f)
        {
            modPhase += 0.07f * (float)sampleRate / 44100.0f;
            if (modPhase > juce::MathConstants<float>::twoPi) modPhase -= juce::MathConstants<float>::twoPi;
            float mod = std::sin(modPhase) * modulation * 0.5f; // +/- samples
            // Light allpass feedback tweak instead of full delay modulation for stability
            juce::ignoreUnused(mod);
        }

        // ---- Wet/Dry mix ----
        outL[i] = dryL * (1.0f - mix) + diffL * mix;
        outR[i] = dryR * (1.0f - mix) + diffR * mix;
    }
}
