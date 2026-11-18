#include "ReverbDSP.h"

namespace
{
    constexpr float pi = juce::MathConstants<float>::pi;

    // Base FDN delay times (ms) before the space-type scale is applied.
    // Mutually incommensurate lengths keep modal density even.
    constexpr float baseLineMs[ReverbDSP::numLines] =
        { 19.171f, 27.731f, 33.037f, 38.923f, 44.897f, 51.317f, 57.413f, 63.827f };

    // Normalised 8x8 Hadamard matrix (rows orthogonal, |H| = 1/sqrt(8)).
    constexpr float hadamard[8][8] =
    {
        {  0.3535534f,  0.3535534f,  0.3535534f,  0.3535534f,  0.3535534f,  0.3535534f,  0.3535534f,  0.3535534f },
        {  0.3535534f, -0.3535534f,  0.3535534f, -0.3535534f,  0.3535534f, -0.3535534f,  0.3535534f, -0.3535534f },
        {  0.3535534f,  0.3535534f, -0.3535534f, -0.3535534f,  0.3535534f,  0.3535534f, -0.3535534f, -0.3535534f },
        {  0.3535534f, -0.3535534f, -0.3535534f,  0.3535534f,  0.3535534f, -0.3535534f, -0.3535534f,  0.3535534f },
        {  0.3535534f,  0.3535534f,  0.3535534f,  0.3535534f, -0.3535534f, -0.3535534f, -0.3535534f, -0.3535534f },
        {  0.3535534f, -0.3535534f,  0.3535534f, -0.3535534f, -0.3535534f,  0.3535534f, -0.3535534f,  0.3535534f },
        {  0.3535534f,  0.3535534f, -0.3535534f, -0.3535534f, -0.3535534f, -0.3535534f,  0.3535534f,  0.3535534f },
        {  0.3535534f, -0.3535534f, -0.3535534f,  0.3535534f, -0.3535534f,  0.3535534f,  0.3535534f, -0.3535534f }
    };

    // Input diffusion allpass times (ms), scaled by the space size factor.
    constexpr float inputApMs[ReverbDSP::numInputAP] = { 4.077f, 5.977f, 8.931f, 13.337f };
    constexpr float inputApScale = 0.10f;   // ±10 % modulation-free size coupling

    // Early reflection tap times (ms) and gains (dB), per channel, alternating.
    constexpr float erTapMs[ReverbDSP::numERTaps]  = { 8.93f, 13.47f, 17.89f, 23.17f, 28.71f, 34.63f };
    constexpr float erTapDb[ReverbDSP::numERTaps]  = { -6.5f, -9.5f, -12.0f, -15.0f, -18.0f, -21.0f };

    inline float dbToGain (float dB) { return std::pow (10.0f, dB * 0.05f); }
}

//==============================================================================
void SVF::setLowpass (float cutoffHz, double sampleRate)
{
    const float x = std::exp (-2.0f * pi * juce::jlimit (20.0f, 20000.0f, cutoffHz) / static_cast<float> (sampleRate));
    a = 1.0f - x;
}

void SVF::setHighpass (float cutoffHz, double sampleRate)
{
    const float x = std::exp (-2.0f * pi * juce::jlimit (2.0f, 500.0f, cutoffHz) / static_cast<float> (sampleRate));
    a = 1.0f - x;   // leaky integrator: z1 tracks the slow average, y = x - z1
}

float SVF::processLowpass (float sample)
{
    z1 += a * (sample - z1);
    return z1;
}

float SVF::processHighpass (float sample)
{
    z1 += a * (sample - z1);
    return sample - z1;
}

void FractionalDelay::prepare (float maxDelayMs, double sampleRate)
{
    const int size = juce::jmax (4, static_cast<int> (std::ceil (maxDelayMs * 0.001 * sampleRate)) + 2);
    buffer.setSize (1, size, false, true, false);
    reset();
    writePos = 0;
}

void FractionalDelay::reset()
{
    buffer.clear();
    writePos = 0;
}

void FractionalDelay::write (float sample)
{
    buffer.setSample (0, writePos, sample);
    if (++writePos == buffer.getNumSamples())
        writePos = 0;
}

float FractionalDelay::readInterpolated (float delaySamples) const
{
    const auto size = static_cast<float> (buffer.getNumSamples());
    // writePos points one slot past the most recent write; measure the delay
    // back from that write so delaySamples == 0 reads the current sample.
    float readPos = static_cast<float> (writePos) - 1.0f - delaySamples;
    while (readPos < 0.0f) readPos += size;

    const int i0 = static_cast<int> (readPos);
    const int i1 = i0 + 1 == buffer.getNumSamples() ? 0 : i0 + 1;
    const float frac = readPos - static_cast<float> (i0);
    const float s0 = buffer.getSample (0, i0);
    const float s1 = buffer.getSample (0, i1);
    return s0 + frac * (s1 - s0);
}

void FeedbackAllpass::prepare (float maxDelayMs, double sampleRate)
{
    delay.prepare (maxDelayMs, sampleRate);
    delaySamples = delay.getSize() * 0.5f;
}

void FeedbackAllpass::reset() { delay.reset(); }

void FeedbackAllpass::setDelayMs (float ms, double sampleRate)
{
    delaySamples = juce::jlimit (1.0f, static_cast<float> (delay.getSize()) - 2.0f,
                                 ms * 0.001f * static_cast<float> (sampleRate));
}

float FeedbackAllpass::process (float input, float feedback)
{
    const float delayed = delay.readInterpolated (delaySamples);
    const float inner   = input + delayed * feedback;
    delay.write (inner);
    return delayed - inner * feedback;
}

void HighShelf::setGainDb (float dB, float cutoffHz, double sampleRate)
{
    const float A  = std::pow (10.0f, dB / 40.0f);
    const float w0 = 2.0f * pi * juce::jlimit (1000.0f, 16000.0f, cutoffHz) / static_cast<float> (sampleRate);
    const float alpha = std::sin (w0) * 0.5f * std::sqrt ((A + 1.0f / A) * (1.0f / 0.65f - 1.0f) + 2.0f);
    const float cosw0 = std::cos (w0);
    const float sqrtA2 = 2.0f * std::sqrt (A) * alpha;

    const float b0_ =        A * ((A + 1.0f) + (A - 1.0f) * cosw0 + sqrtA2);
    const float b1_ = -2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosw0);
    const float b2_ =        A * ((A + 1.0f) + (A - 1.0f) * cosw0 - sqrtA2);
    const float a0_ =             (A + 1.0f) - (A - 1.0f) * cosw0 + sqrtA2;
    const float a1_ =  2.0f *     ((A - 1.0f) - (A + 1.0f) * cosw0);
    const float a2_ =             (A + 1.0f) - (A - 1.0f) * cosw0 - sqrtA2;

    b0 = b0_ / a0_; b1 = b1_ / a0_; b2 = b2_ / a0_; a1 = a1_ / a0_; a2 = a2_ / a0_;
}

float HighShelf::process (float x)
{
    const float y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
    x2 = x1; x1 = x;
    y2 = y1; y1 = y;
    return y;
}

//==============================================================================
const ReverbDSP::SpaceProfile ReverbDSP::profiles[numSpaceTypes] =
{
    // delayScale  lpScale  hpHz  modRate  modDepth  erGain  erCutoff
    { 1.50f,       0.55f,   16.0f, 0.30f,  1.9f,     0.26f,  1500.0f },  // Void — vast, dark, slow
    { 1.12f,       0.90f,   26.0f, 0.80f,  1.1f,     0.24f,  2600.0f },  // Hall — balanced concert hall
    { 0.60f,       1.50f,   110.0f,1.35f,  0.7f,     0.05f,  3200.0f },  // Plate — small, dense, bright
    { 1.85f,       0.42f,   13.0f, 0.17f,  2.5f,     0.40f,  1100.0f },  // Cavern — huge, dark, boomy ER
    { 1.00f,       0.95f,   22.0f, 0.55f,  1.4f,     0.12f,  2100.0f }   // Ambient — smooth wash
};

float ReverbDSP::referenceLineMs (int index) noexcept
{
    return baseLineMs[static_cast<size_t> (juce::jlimit (0, numLines - 1, index))];
}

void ReverbDSP::prepare (double newSampleRate)
{
    sampleRate = newSampleRate;

    // 250 ms pre-delay headroom
    preDelayL.prepare (250.0f, sampleRate);
    preDelayR.prepare (250.0f, sampleRate);

    for (size_t i = 0; i < diffusionL.size(); ++i)
    {
        const float maxMs = inputApMs[i] * (1.0f + inputApScale) * 2.0f + 2.0f;
        diffusionL[i].prepare (maxMs, sampleRate);
        diffusionR[i].prepare (maxMs, sampleRate);
    }

    erDelayL.prepare (erTapMs[numERTaps - 1] * 2.0f + 2.0f, sampleRate);
    erDelayR.prepare (erTapMs[numERTaps - 1] * 2.0f + 2.0f, sampleRate);

    const float maxLineMs = baseLineMs[numLines - 1] * 1.85f * 1.15f + 2.0f;
    for (auto& line : lines)
        line.prepare (maxLineMs, sampleRate);

    for (size_t i = 0; i < lfoRateMul.size(); ++i)
    {
        // Incommensurate rate multipliers, evenly spread phases.
        constexpr float rateMuls[numLines] = { 1.00f, 1.13f, 0.79f, 1.31f, 0.91f, 1.21f, 1.07f, 0.85f };
        lfoRateMul[i] = rateMuls[i];
        lfoPhase[i]   = static_cast<float> (i) * 1.7f + 0.4f;
    }

    preDelaySamples.reset (sampleRate, 0.005);
    mixS.reset (sampleRate, 0.010);
    widthS.reset (sampleRate, 0.010);
    airS.reset (sampleRate, 0.020);
    dampingS.reset (sampleRate, 0.020);
    modulationS.reset (sampleRate, 0.030);
    diffusionS.reset (sampleRate, 0.020);

    preDelaySamples.setCurrentAndTargetValue (0.0f);
    mixS.setCurrentAndTargetValue (0.5f);
    widthS.setCurrentAndTargetValue (1.0f);
    airS.setCurrentAndTargetValue (0.35f);
    dampingS.setCurrentAndTargetValue (0.5f);
    modulationS.setCurrentAndTargetValue (0.0f);
    diffusionS.setCurrentAndTargetValue (0.7f);

    configureSpace();
    updateDamping();
    updateLineFeedback();
    reset();
}

void ReverbDSP::reset()
{
    preDelayL.reset();
    preDelayR.reset();
    for (auto& ap : diffusionL) ap.reset();
    for (auto& ap : diffusionR) ap.reset();
    erDelayL.reset();
    erDelayR.reset();
    for (auto& line : lines) line.reset();
    for (auto& lp : dampingLP) lp.reset();
    for (auto& hp : subsonicHP) hp.reset();
    airShelfL.reset();
    airShelfR.reset();
}

float ReverbDSP::characterTiltDb() const noexcept
{
    switch (character)
    {
        case 0:  return -3.0f;  // Dark
        case 2:  return  3.0f;  // Bright
        default: return  0.0f;  // Neutral
    }
}

float ReverbDSP::lineDelayMsBase (int index) const noexcept
{
    return baseLineMs[static_cast<size_t> (index)] * profiles[spaceType].delayScale
         * (0.72f + roomSize * 0.56f);   // 0.72x .. 1.28x around the profile scale
}

void ReverbDSP::configureSpace()
{
    const int sr = static_cast<int> (sampleRate);
    for (int i = 0; i < numLines; ++i)
    {
        const float ms = lineDelayMsBase (i);
        lineDelaySamples[static_cast<size_t> (i)] =
            juce::jlimit (8.0f, static_cast<float> (lines[static_cast<size_t> (i)].getSize()) - 2.0f,
                          ms * 0.001f * static_cast<float> (sr));
    }
    // Diffusion allpasses follow size a little.
    for (int i = 0; i < numInputAP; ++i)
    {
        const float scale = (0.78f + roomSize * 0.44f) * (1.0f + inputApScale);
        diffusionL[static_cast<size_t> (i)].setDelayMs (inputApMs[i] * scale, sampleRate);
        diffusionR[static_cast<size_t> (i)].setDelayMs (inputApMs[i] * scale * 1.07f, sampleRate);
    }
    for (auto& n : subsonicHP)
        n.setHighpass (profiles[spaceType].hpHz, sampleRate);
    updateLineFeedback();
}

void ReverbDSP::updateDamping()
{
    // Logarithmic sweep 18 kHz -> 550 Hz as damping goes 0 -> 1,
    // then per-space and per-character scaling.
    const float cutoff = std::exp (juce::jmap (juce::jlimit (0.0f, 1.0f, dampingS.getTargetValue()),
                                               std::log (18000.0f), std::log (550.0f)));
    float c = cutoff * profiles[spaceType].lpScale;
    if (character == 0)      c *= 0.62f;
    else if (character == 2) c *= 1.60f;
    c = juce::jlimit (240.0f, 20000.0f, c);
    for (auto& lp : dampingLP)
        lp.setLowpass (c, sampleRate);
}

void ReverbDSP::updateLineFeedback()
{
    const float t60 = juce::jlimit (0.1f, 20.0f, decaySeconds);
    for (int i = 0; i < numLines; ++i)
    {
        // -60 dB after t60 seconds of round trips; clamp strictly inside the unit circle.
        const float tau = lineDelaySamples[static_cast<size_t> (i)] / static_cast<float> (sampleRate);
        lineFeedback[static_cast<size_t> (i)] = juce::jlimit (0.0f, 0.995f,
                                                              std::pow (10.0f, -3.0f * tau / t60));
    }
}

void ReverbDSP::setRoomSize (float size)
{
    size = juce::jlimit (0.0f, 1.0f, size);
    if (!juce::approximatelyEqual (roomSize, size))
    {
        roomSize = size;
        configureSpace();
    }
}

void ReverbDSP::setPreDelayMs (float ms)
{
    preDelaySamples.setTargetValue (juce::jlimit (0.0f, 250.0f, ms) * 0.001f * static_cast<float> (sampleRate));
}

void ReverbDSP::setDecaySeconds (float seconds)
{
    seconds = juce::jlimit (0.1f, 20.0f, seconds);
    if (!juce::approximatelyEqual (decaySeconds, seconds))
    {
        decaySeconds = seconds;
        updateLineFeedback();
    }
}

void ReverbDSP::setDamping (float amount)
{
    amount = juce::jlimit (0.0f, 1.0f, amount);
    if (!juce::approximatelyEqual (dampingS.getTargetValue(), amount))
    {
        dampingS.setTargetValue (amount);
        updateDamping();
    }
}

void ReverbDSP::setModulation (float amount) { modulationS.setTargetValue (juce::jlimit (0.0f, 1.0f, amount)); }
void ReverbDSP::setMix (float wet)           { mixS.setTargetValue (juce::jlimit (0.0f, 1.0f, wet)); }
void ReverbDSP::setDiffusion (float amount)  { diffusionS.setTargetValue (juce::jlimit (0.0f, 1.0f, amount)); }
void ReverbDSP::setWidth (float width)       { widthS.setTargetValue (juce::jlimit (0.0f, 2.0f, width)); }
void ReverbDSP::setAir (float amount)        { airS.setTargetValue (juce::jlimit (0.0f, 1.0f, amount)); }

void ReverbDSP::setCharacter (int index)
{
    index = juce::jlimit (0, 2, index);
    if (character != index)
    {
        character = index;
        updateDamping();
        airShelfL.setGainDb (juce::jmap (airS.getTargetValue(), 0.0f, 1.0f, -6.0f, 7.0f) + characterTiltDb(), 6500.0f, sampleRate);
        airShelfR.setGainDb (juce::jmap (airS.getTargetValue(), 0.0f, 1.0f, -6.0f, 7.0f) + characterTiltDb(), 6500.0f, sampleRate);
    }
}

void ReverbDSP::setMonoMode (bool mono) { monoMode = mono; }

void ReverbDSP::setSpaceType (int index)
{
    index = juce::jlimit (0, numSpaceTypes - 1, index);
    if (spaceType != index)
    {
        spaceType = index;
        configureSpace();
        updateDamping();
        airShelfL.setGainDb (juce::jmap (airS.getTargetValue(), 0.0f, 1.0f, -6.0f, 7.0f) + characterTiltDb(), 6500.0f, sampleRate);
        airShelfR.setGainDb (juce::jmap (airS.getTargetValue(), 0.0f, 1.0f, -6.0f, 7.0f) + characterTiltDb(), 6500.0f, sampleRate);
    }
}

void ReverbDSP::process (juce::AudioBuffer<float>& buffer)
{
    const juce::ScopedNoDenormals noDenormals;
    const int numChannels = juce::jmin (buffer.getNumChannels(), 2);
    const int numSamples  = buffer.getNumSamples();
    if (numChannels == 0 || numSamples == 0)
        return;

    const auto profile = profiles[spaceType];

    // Early-reflection tap positions depend only on size; compute once per block.
    float erTapSamples[numERTaps];
    for (int t = 0; t < numERTaps; ++t)
        erTapSamples[t] = erTapMs[t] * (0.72f + roomSize * 0.56f) * 0.001f * static_cast<float> (sampleRate);

    // Air shelf coefficient cache; recomputed only when the ramped value moves.
    float lastAirNorm = -1.0f;
    auto updateAirShelf = [&] (float norm)
    {
        if (std::abs (norm - lastAirNorm) < 0.001f)
            return;
        lastAirNorm = norm;
        const float airDb = juce::jmap (norm, 0.0f, 1.0f, -6.0f, 7.0f) + characterTiltDb();
        airShelfL.setGainDb (airDb, 6500.0f, sampleRate);
        airShelfR.setGainDb (airDb, 6500.0f, sampleRate);
    };

    // Wet output gain: sum of 8 correlated lines through the mixer; keep headroom sane.
    constexpr float fdnOutputNorm = 0.55f;

    auto* const* audio = buffer.getArrayOfWritePointers();
    const bool stereo = numChannels == 2;

    for (int i = 0; i < numSamples; ++i)
    {
        const float inL = audio[0][i];
        const float inR = stereo ? audio[1][i] : inL;

        // ---- Pre-delay (stereo; mono mode feeds the average to both sides) -----
        const float monoSum = 0.5f * (inL + inR);
        preDelayL.write (monoMode ? monoSum : inL);
        preDelayR.write (monoMode ? monoSum : inR);
        const float pd     = preDelaySamples.getNextValue();
        const float delayL = preDelayL.readInterpolated (pd);
        const float delayR = preDelayR.readInterpolated (pd);

        // ---- Input diffusion ----------------------------------------------------
        const float apFb = 0.30f + 0.42f * diffusionS.getNextValue();
        float diffL = delayL, diffR = delayR;
        for (int a = 0; a < numInputAP; ++a)
        {
            diffL = diffusionL[static_cast<size_t> (a)].process (diffL, apFb);
            diffR = diffusionR[static_cast<size_t> (a)].process (diffR, apFb);
        }

        // ---- Early reflections ---------------------------------------------------
        erDelayL.write (diffL);
        erDelayR.write (diffR);
        float er = 0.0f, erRight = 0.0f;
        for (int t = 0; t < numERTaps; ++t)
        {
            const float tapSamples = erTapSamples[t];
            const float g = dbToGain (erTapDb[t]);
            // Alternate taps between L and R for stereo spread.
            if ((t & 1) == 0) { er += erDelayL.readInterpolated (tapSamples) * g; }
            else              { erRight += erDelayR.readInterpolated (tapSamples) * g; }
        }
        const float erLp = profile.erGain;
        er      *= erLp;
        erRight *= erLp;

        // ---- FDN core ------------------------------------------------------------
        const float modDepth = profile.modDepth * (0.35f + 1.65f * modulationS.getNextValue());
        const float modRate  = profile.modRate;
        const float timeStep = 1.0f / static_cast<float> (sampleRate);
        float lineOut[numLines];
        float lineState[numLines];
        const float inMix = 0.70710678f; // 1/sqrt(2)

        for (int l = 0; l < numLines; ++l)
        {
            // Slow per-line LFO modulates the read position (small excursion).
            const float phaseInc = 2.0f * pi * modRate * lfoRateMul[static_cast<size_t> (l)] * timeStep;
            lfoPhase[static_cast<size_t> (l)] += phaseInc;
            if (lfoPhase[static_cast<size_t> (l)] >= 2.0f * pi)
                lfoPhase[static_cast<size_t> (l)] -= 2.0f * pi;
            const float lfo = std::sin (lfoPhase[static_cast<size_t> (l)]) * modDepth;

            const float d = juce::jlimit (4.0f,
                                          static_cast<float> (lines[static_cast<size_t> (l)].getSize()) - 2.0f,
                                          lineDelaySamples[static_cast<size_t> (l)] + lfo);
            lineState[l] = lines[static_cast<size_t> (l)].readInterpolated (d);
            // Damping lowpass inside the feedback path + subsonic highpass.
            float fb = dampingLP[static_cast<size_t> (l)].processLowpass (lineState[l]);
            fb = subsonicHP[static_cast<size_t> (l)].processHighpass (fb);
            lineOut[l] = fb * lineFeedback[static_cast<size_t> (l)];
        }

        // Hadamard mixing matrix
        float mixed[numLines];
        for (int r = 0; r < numLines; ++r)
        {
            float acc = 0.0f;
            for (int c = 0; c < numLines; ++c)
                acc += hadamard[r][c] * lineOut[c];
            mixed[r] = acc;
        }

        // Write back: input (left feeds even lines, right feeds odd lines) + mix
        for (int l = 0; l < numLines; ++l)
        {
            const float inStereo = (l & 1) == 0 ? diffL : diffR;
            lines[static_cast<size_t> (l)].write (inStereo * inMix + mixed[l]);
        }

        // Wet sum (slight decorrelation between L/R output tap groups)
        float wetL = (lineOut[0] + lineOut[2] + lineOut[4] + lineOut[6]) * 0.5f + er * 0.9f;
        float wetR = (lineOut[1] + lineOut[3] + lineOut[5] + lineOut[7]) * 0.5f + erRight * 0.9f;

        // Mono mode: identical output in both channels.
        if (monoMode)
        {
            const float m = 0.5f * (wetL + wetR);
            wetL = m;
            wetR = m;
        }

        // Air shelf (presence above ~6.5 kHz)
        const float airNorm = airS.getNextValue();
        updateAirShelf (airNorm);
        wetL = airShelfL.process (wetL * fdnOutputNorm);
        wetR = airShelfR.process (wetR * fdnOutputNorm);

        // Width (mid/side)
        const float width = widthS.getNextValue();
        if (stereo && !monoMode && std::abs (width - 1.0f) > 1.0e-3f)
        {
            const float mid  = 0.5f * (wetL + wetR);
            const float side = 0.5f * (wetL - wetR) * width;
            wetL = mid + side;
            wetR = mid - side;
        }

        // Equal-power dry/wet (with exact endpoint snaps)
        const float m   = mixS.getNextValue();
        float dryG, wetG;
        if (m <= 1.0e-4f)      { dryG = 1.0f; wetG = 0.0f; }
        else if (m >= 0.9999f) { dryG = 0.0f; wetG = 1.0f; }
        else                   { const float a = m * 0.5f * pi; dryG = std::cos (a); wetG = std::sin (a); }

        audio[0][i] = inL * dryG + wetL * wetG;
        if (stereo)
            audio[1][i] = inR * dryG + wetR * wetG;
    }
}
