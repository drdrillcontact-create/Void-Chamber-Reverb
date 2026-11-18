// Headless render of the editor to a PNG so the layout can be inspected
// without a display server. Usage: EditorSnapshotTest output.png
#include "PluginProcessor.h"
#include "PluginEditor.h"

int main(int argc, char* argv[])
{
    juce::ignoreUnused(argc);
    VoidChamberReverbAudioProcessor processor;

    std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
    editor->setSize(1440, 960);

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
