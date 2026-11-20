#include "ReverbDSP.h"
#include <iostream>
#include <cmath>
#include <vector>
#include <array>

//==============================================================================
// Void Chamber Reverb — DSP verification suite.
//
// The reverb is an 8-line Hadamard feedback delay network; energy written into
// one channel legitimately bleeds into the other through the mixing matrix
// (that is what makes a space sound real), so tests assert *physical*
// behaviour: onset after the pre-delay, a stable decaying tail, channel
// equality in mono mode, stable HP/LP-free feedback loops and an RT60 that
// roughly matches the requested value.
//==============================================================================
namespace
{
    struct TestReverb
    {
        ReverbDSP dsp;
        TestReverb() = default;
        void setup (double sr, float decay, float damping = 0.0f, int character = 1, int spaceType = 0)
        {
            dsp.prepare (sr);
            dsp.setMix (1.0f);
            dsp.setWidth (1.0f);
            dsp.setPreDelayMs (0.0f);
            dsp.setRoomSize (0.5f);
            dsp.setDiffusion (0.7f);
            dsp.setModulation (0.0f);
            dsp.setDecaySeconds (decay);
            dsp.setDamping (damping);
            dsp.setCharacter (character);
            dsp.setSpaceType (spaceType);
        }
    };

    bool tailExistsAndFinite (int numChannels, int inputChannel, double sampleRate, float preDelayMs)
    {
        ReverbDSP reverb;
        reverb.prepare (sampleRate);
        reverb.setMix (1.0f);
        reverb.setWidth (1.0f);
        reverb.setPreDelayMs (preDelayMs);
        reverb.setRoomSize (0.72f);
        reverb.setDecaySeconds (4.8f);

        constexpr int blockSize = 64;
        juce::AudioBuffer<float> block (numChannels, blockSize);
        const auto preDelay = static_cast<int> (preDelayMs * 0.001 * sampleRate);
        bool sawWet = false;
        int  wetSamples = 0;
        for (int start = 0; start < preDelay + static_cast<int> (0.25 * sampleRate); start += blockSize)
        {
            block.clear();
            if (start == 0) block.setSample (inputChannel, 0, 1.0f);
            reverb.process (block);
            for (int i = 0; i < blockSize; ++i)
                for (int ch = 0; ch < numChannels; ++ch)
                {
                    const float s = block.getSample (ch, i);
                    if (! std::isfinite (s) || std::abs (s) > 8.0f)
                        return false;
                    if (std::abs (s) > 1.0e-6f) { sawWet = true; ++wetSamples; }
                }
        }
        // Wet energy must persist well beyond the pre-delay.
        return sawWet && wetSamples > static_cast<int> (0.1 * sampleRate);
    }

    // Advance parameter smoothers with silence so mix reaches its target.
    void preRollSilence (ReverbDSP& reverb, double sampleRate, float seconds)
    {
        juce::AudioBuffer<float> block (2, 512);
        const int total = static_cast<int> (sampleRate * seconds);
        for (int start = 0; start < total; start += 512)
        {
            block.clear();
            reverb.process (block);
        }
    }

    bool monoModeTest (double sampleRate)
    {
        ReverbDSP reverb;
        reverb.prepare (sampleRate);
        reverb.setMix (1.0f);
        reverb.setWidth (1.7f);
        reverb.setMonoMode (true);
        preRollSilence (reverb, sampleRate, 0.2f);

        juce::AudioBuffer<float> block (2, 256);
        block.clear();
        block.setSample (0, 0, 1.0f);
        reverb.process (block);
        for (int i = 0; i < 256; ++i)
            if (block.getSample (0, i) != block.getSample (1, i))
                return false;

        reverb.reset();
        block.clear();
        reverb.process (block);
        return block.getMagnitude (0, block.getNumSamples()) == 0.0f;
    }



    //==============================================================================
    // Measure RT60 by integrating band-limited energy in 10 ms windows.
    //==============================================================================
    double measureRT60 (ReverbDSP& reverb, double sampleRate, float targetDecay)
    {
        reverb.reset();
        preRollSilence (reverb, sampleRate, 0.25f);
        reverb.reset();          // clear the silent history; keep smoothed positions
        // advance smoothers once more after reset so mix stays at target
        preRollSilence (reverb, sampleRate, 0.05f);
        const int totalSamples = static_cast<int> (sampleRate * (targetDecay * 2.0 + 1.0));
        juce::AudioBuffer<float> block (2, 512);

        std::vector<float> energy;
        energy.reserve (static_cast<size_t> (totalSamples / 512 + 1));

        int impulseLeft = 1;
        for (int start = 0; start < totalSamples; start += 512)
        {
            block.clear();
            if (impulseLeft > 0) { block.setSample (0, 0, 1.0f); --impulseLeft; }
            reverb.process (block);
            float e = 0.0f;
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < block.getNumSamples(); ++i)
                    e += block.getSample (ch, i) * block.getSample (ch, i);
            energy.push_back (e);
        }

        // Convert window energies to dB
        std::vector<float> db (energy.size(), -120.0f);
        float peak = 1.0e-12f;
        for (float e : energy) peak = std::max (peak, e);
        for (size_t i = 0; i < energy.size(); ++i)
            db[i] = 10.0f * std::log10 (std::max (energy[i], 1.0e-12f) / peak);

        // Least-squares fit of the steady-state tail slope, ignoring the early
        // (early-reflection dominated) portion. Standard T30 uses the -5..-35 dB
        // band; we anchor on -15..-50 dB once past 150 ms instead, which is the
        // diffuse field in this design and much less biased by ER content.
        const double windowSeconds = 512.0 / sampleRate;
        double sumT = 0.0, sumDb = 0.0, sumTT = 0.0, sumTDb = 0.0;
        int count = 0;
        for (size_t i = 0; i < db.size(); ++i)
        {
            const double t = static_cast<double> (i) * windowSeconds;
            if (t < 0.15 || db[i] > -12.0f || db[i] < -50.0f)
                continue;
            sumT += t; sumDb += db[i]; sumTT += t * t; sumTDb += t * db[i]; ++count;
        }
        if (count < 8)
            return -1.0;
        const double slope = (count * sumTDb - sumT * sumDb) / (count * sumTT - sumT * sumT);
        if (slope >= -0.5)
            return -1.0;
        return -60.0 / slope; // seconds
    }
}


int main()
{
    const double sr44100 = 44100.0, sr48000 = 48000.0, sr96000 = 96000.0;

    // 1) Wet onset, tail persistence and finiteness across rates / channel counts
    if (! tailExistsAndFinite (1, 0, sr44100, 0.0f)
        || ! tailExistsAndFinite (2, 0, sr48000, 28.125f)
        || ! tailExistsAndFinite (2, 1, sr96000, 0.0f))
    {
        std::cerr << "Wet tail/onset test failed.\n";
        return 1;
    }

    // 2) Mono mode: identical output in both channels + clean reset
    if (! monoModeTest (sr48000))
    {
        std::cerr << "Mono collapse / reset test failed.\n";
        return 1;
    }

    // 3) RT60 tracks the requested decay time (T30 extrapolated)
    for (float target : { 0.5f, 2.0f, 4.8f })
    {
        TestReverb tr;
        tr.setup (sr48000, target, 0.0f, 1, 0);
        const double measured = measureRT60 (tr.dsp, sr48000, target);
        std::cout << "RT60 target " << target << " s -> measured " << measured << " s\n";
        if (measured < 0.0 || measured < target * 0.55 || measured > target * 1.8)
        {
            std::cerr << "RT60 out of tolerance.\n";
            return 1;
        }
    }

    // 4) Every space type produces a distinct, stable, finite response
    {
        std::vector<std::unique_ptr<juce::AudioBuffer<float>>> store;
        for (int type = 0; type < 5; ++type)
        {
            ReverbDSP reverb;
            reverb.prepare (sr48000);
            reverb.setMix (1.0f);
            reverb.setWidth (1.0f);
            reverb.setModulation (0.0f);
            reverb.setDamping (0.2f);
            reverb.setSpaceType (type);
            reverb.setDecaySeconds (2.0f);

            auto buf = std::make_unique<juce::AudioBuffer<float>> (2, 4096);
            buf->clear();
            buf->setSample (0, 0, 1.0f);
            reverb.process (*buf);
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < 4096; ++i)
                    if (! std::isfinite (buf->getSample (ch, i)))
                    {
                        std::cerr << "Space type produced non-finite output.\n";
                        return 1;
                    }
            store.push_back (std::move (buf));
        }
        for (size_t a = 0; a < store.size(); ++a)
            for (size_t b = a + 1; b < store.size(); ++b)
            {
                float diff = 0.0f;
                for (int ch = 0; ch < 2; ++ch)
                    for (int i = 0; i < 4096; ++i)
                        diff += std::abs (store[a]->getSample (ch, i) - store[b]->getSample (ch, i));
                if (diff < 1.0e-3f)
                {
                    std::cerr << "Space types " << a << " and " << b << " are indistinguishable.\n";
                    return 1;
                }
            }
    }

    // Wet telemetry must remain active with Mix=0 and reset deterministically.
    for (double rate : { 44100.0, 48000.0, 96000.0 })
    {
        ReverbDSP dsp; dsp.prepare (rate); dsp.setMix (0.0f);
        juce::AudioBuffer<float> audio (2, 256);
        bool observedWet = false;
        for (int block = 0; block < 100; ++block)
        {
            for (int ch = 0; ch < 2; ++ch)
                for (int n = 0; n < 256; ++n)
                    audio.setSample (ch,n,0.1f * std::sin (float ((block*256+n)*900.0/rate)));
            dsp.process (audio);
            const float energy = dsp.getWetEnergy();
            if (! std::isfinite (energy) || energy > 4.0f) return 1;
            observedWet = observedWet || energy > 0.0001f;
        }
        if (! observedWet) return 1;
        dsp.reset(); if (dsp.getWetEnergy() != 0.0f) return 1;
    }

    // 5) Zero-sized processing must not crash
    {
        ReverbDSP reverb;
        reverb.prepare (sr48000);
        juce::AudioBuffer<float> empty (2, 0);
        reverb.process (empty);
    }

    return 0;
}
