// Headless render of the editor to a PNG so the layout can be inspected
// without a display server. Usage: EditorSnapshotTest output.png
#include <cmath>
#include "PluginProcessor.h"
#include "PluginEditor.h"

int main(int argc, char* argv[])
{
    juce::ignoreUnused(argc);

    // Required before touching any JUCE GUI code on every platform —
    // without it the graphics subsystem is not initialised.
    juce::ScopedJuceInitialiser_GUI juceInitialiser;

    VoidChamberReverbAudioProcessor processor;

    // Feed a bright test tone through the effect so the meters show signal,
    // approximating a live host session rather than an idle plugin.
    processor.prepareToPlay(44100.0, 512);
    {
        juce::AudioBuffer<float> buffer(2, 512);
        juce::MidiBuffer midi;
        constexpr double twoPi = 6.28318530717958647692;
        for (int block = 0; block < 86; ++block)   // ~1 second at 44.1 kHz
        {
            for (int ch = 0; ch < 2; ++ch)
                for (int i = 0; i < 512; ++i)
                {
                    const double t = static_cast<double>(block * 512 + i) / 44100.0;
                    const float s = static_cast<float>(0.55 * std::sin(twoPi * 220.0 * t)
                                                     + 0.30 * std::sin(twoPi * 1730.0 * t)
                                                     + (ch == 0 ? 1.0 : 0.8) * 0.18 * std::sin(twoPi * 61.0 * t));
                    buffer.setSample(ch, i, s);
                }
            processor.processBlock(buffer, midi);
        }
    }

    std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
    editor->setSize(1440, 960);

    // Settle the timer-driven visual state (selected orb, toggle states,
    // cached value texts, meter levels) so the snapshot matches a live editor.
    static_cast<VoidChamberReverbAudioProcessorEditor*>(editor.get())->timerCallback();

    // Let any pending layout settle, then render the full component tree.
    juce::Image image(juce::Image::ARGB, editor->getWidth(), editor->getHeight(), true);
    {
        juce::Graphics g(image);
        editor->paintEntireComponent(g, true);
    }

    const auto out = juce::File(juce::String(argv[1]));
    out.deleteFile();
    juce::FileOutputStream stream(out);
    juce::PNGImageFormat().writeImageToStream(image, stream);
    return stream.failedToOpen() ? 1 : 0;
}
