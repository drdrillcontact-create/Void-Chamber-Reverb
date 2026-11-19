// Headless render of the editor to a PNG so the layout can be inspected
// without a display server. Usage: EditorSnapshotTest output.png
#include <cmath>
#include "PluginProcessor.h"
#include "PluginEditor.h"

int main(int argc, char* argv[])
{
    if (argc < 2) return 2;

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
    // Include host-driven off-ratio sizes: some DAWs bypass the constrainer.
    const juce::Point<int> sizes[] = { {1536, 1024}, {960, 640}, {1200, 900}, {1800, 900} };
    for (int index = 0; index < 4; ++index)
    {
        editor->setSize (sizes[index].x, sizes[index].y);
        static_cast<VoidChamberReverbAudioProcessorEditor*>(editor.get())->timerCallback();
        juce::Image image (juce::Image::ARGB, editor->getWidth(), editor->getHeight(), true);
        { juce::Graphics g (image); editor->paintEntireComponent (g, true); }

        // The bezel must stay next to BOTH edges, never in a letterboxed centre.
        const int y = image.getHeight() / 2;
        const int edge = juce::roundToInt (5.0f * image.getWidth() / 1536.0f);
        const auto backgroundColour = image.getPixelAt (0, y);
        if (image.getPixelAt (edge, y) == backgroundColour
            || image.getPixelAt (image.getWidth() - edge - 1, y) == backgroundColour)
            return 3;

        const juce::File base (juce::String (argv[1]));
        const auto out = index == 0 ? base : base.getSiblingFile (
            base.getFileNameWithoutExtension() + "-" + juce::String (sizes[index].x)
            + "x" + juce::String (sizes[index].y) + ".png");
        out.deleteFile();
        juce::FileOutputStream stream (out);
        if (stream.failedToOpen() || ! juce::PNGImageFormat().writeImageToStream (image, stream))
            return 1;
    }
    return 0;
}
