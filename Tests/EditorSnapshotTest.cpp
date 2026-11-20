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

    // A moving indicator and per-accent value arc must not be baked into the
    // Titanium face. Exercise both accents at minimum, middle and maximum.
    const auto face = juce::ImageCache::getFromMemory (
        BinaryData::titaniumknobbody_png, BinaryData::titaniumknobbody_pngSize);
    if (! face.isValid()) return 4;
    juce::Image knobSheet (juce::Image::ARGB, 600, 400, true);
    for (int row = 0; row < 2; ++row)
    {
        VoidKnobLookAndFeel feel (row == 0 ? VoidKnobLookAndFeel::Accent::pink
                                          : VoidKnobLookAndFeel::Accent::cyan);
        juce::Slider slider;
        juce::Image renders[3];
        for (int col = 0; col < 3; ++col)
        {
            renders[col] = juce::Image (juce::Image::ARGB, 200, 200, true);
            {
                // Flush the native graphics context before copying the image
                // (Direct2D may defer drawing until Graphics is destroyed).
                juce::Graphics g (renders[col]);
                feel.drawRotarySlider (g, 0, 0, 200, 200, col * 0.5f, -2.3561945f, 2.3561945f, slider);
            }
            juce::Graphics sheet (knobSheet);
            sheet.drawImageAt (renders[col], col * 200, row * 200);
        }
        // Centre of the moving indicator at the minimum and maximum angles.
        for (int col : { 0, 2 })
        {
            const float a = col == 0 ? -2.3561945f : 2.3561945f;
            const int x = juce::roundToInt (100.0f + 52.8f * std::sin (a));
            const int y = juce::roundToInt (100.0f - 52.8f * std::cos (a));
            if (renders[col].getPixelAt (x, y) == renders[1].getPixelAt (x, y)) return 5;
        }
        const auto arcPixel = renders[2].getPixelAt (41, 43);
        if (row == 0 ? arcPixel.getRed() <= arcPixel.getBlue()
                     : arcPixel.getBlue() <= arcPixel.getRed()) return 6;
    }
    if (knobSheet.getPixelAt (100, 100).isTransparent()) return 7;
    {
        const juce::File base { juce::String (argv[1]) };
        const auto out = base.getSiblingFile ("editor-snapshot-knobs.png");
        out.deleteFile();
        juce::FileOutputStream stream (out);
        if (stream.failedToOpen() || ! juce::PNGImageFormat().writeImageToStream (knobSheet, stream)) return 1;
    }

    // dB calibration used by both the fill and peak line must match the ticks.
    for (int db : { 0, -6, -12, -24, -48, -60 })
        if (std::abs (VoidGUI::meterPosition (juce::Decibels::decibelsToGain (static_cast<float> (db)))
                      - (db + 60.0f) / 60.0f) > 0.0001f) return 8;
    if (VoidGUI::meterPosition (0.0f) != 0.0f || VoidGUI::meterPosition (2.0f) != 1.0f) return 8;

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
    // The redesign must retain real parameter controls and the preset overlay.
    auto& state = processor.getValueTreeState();
    int orbIndex = 0;
    bool monoFound = false, stereoFound = false, presetOpened = false;
    const float initialSpace = state.getRawParameterValue (VoidChamberReverbAudioProcessor::spaceTypeID)->load();
    const float initialMode = state.getRawParameterValue (VoidChamberReverbAudioProcessor::channelModeID)->load();
    for (int i = 0; i < editor->getNumChildComponents(); ++i)
    {
        auto* child = editor->getChildComponent (i);
        if (auto* orb = dynamic_cast<OrbButton*> (child))
        {
            if (orbIndex == 3)
            {
                orb->onClick();
                if (state.getRawParameterValue (VoidChamberReverbAudioProcessor::spaceTypeID)->load() != 3.0f) return 9;
            }
            ++orbIndex;
        }
        if (auto* button = dynamic_cast<juce::TextButton*> (child))
        {
            if (button->getButtonText() == "MONO")
            {
                button->onClick(); monoFound = true;
                if (state.getRawParameterValue (VoidChamberReverbAudioProcessor::channelModeID)->load() != 1.0f) return 9;
            }
            else if (button->getButtonText() == "STEREO")
            {
                button->onClick(); stereoFound = true;
                if (state.getRawParameterValue (VoidChamberReverbAudioProcessor::channelModeID)->load() != 0.0f) return 9;
            }
            else if (button->getButtonText() == processor.getCurrentPresetName())
                button->onClick();
        }
    }
    for (int i = 0; i < editor->getNumChildComponents(); ++i)
        if (auto* browser = dynamic_cast<PresetBrowser*> (editor->getChildComponent (i)))
            presetOpened = browser->isVisible();
    if (orbIndex != 5 || ! monoFound || ! stereoFound || ! presetOpened) return 9;
    editor->keyPressed (juce::KeyPress (juce::KeyPress::escapeKey));
    for (const auto& restore : { std::make_pair (VoidChamberReverbAudioProcessor::spaceTypeID, initialSpace),
                                std::make_pair (VoidChamberReverbAudioProcessor::channelModeID, initialMode) })
        if (auto* parameter = state.getParameter (restore.first))
            parameter->setValueNotifyingHost (parameter->convertTo0to1 (restore.second));

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

        const juce::File base { juce::String (argv[1]) };
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
