#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "VoidGUI.h"

//==============================================================================
class LevelMeter : public juce::Component
{
public:
    LevelMeter();
    void setLevel (float linearLevel);   // linear 0..1 input, RMS-ish
    void paint (juce::Graphics&) override;
    void update (float dt);              // falloff/peak decay; called by editor timer

private:
    float display = 0.0f;
    float peak = 0.0f;
    float peakHoldSeconds = 0.0f;
    float clip = 0.0f;

protected:
    // Shared renderer. pink selects the magenta accent used for the output
    // meter while the input meter stays cyan. Adds no state to the class.
    void paintMeter (juce::Graphics&, bool pink);
};

// Magenta-accented output meter. Deliberately stateless: same size and
// layout as LevelMeter, only a different paint() entry point.
class PinkLevelMeter : public LevelMeter
{
public:
    PinkLevelMeter();
    void paint (juce::Graphics&) override;
};

//==============================================================================
// Large signature arc: RT60 from 0.1 s to 20 s. Horizontal/vertical drag.
//==============================================================================
class DecayArcSlider : public juce::Slider
{
public:
    DecayArcSlider();
    void paint (juce::Graphics&) override;

private:
    static constexpr float arcLen = 516.0f, arcHei = 248.0f;
};

//==============================================================================
class OrbButton : public juce::Component
{
public:
    OrbButton();
    void setOrb (const juce::Image& image, const juce::String& name);
    void setSelected (bool shouldBeSelected);
    void setHovered (bool shouldBeHovered);
    bool isSelected() const { return selected; }

    std::function<void()> onClick;

    void paint (juce::Graphics&) override;
    void mouseEnter (const juce::MouseEvent&) override { hovered = true;  repaint(); }
    void mouseExit  (const juce::MouseEvent&) override { hovered = false; repaint(); }
    void mouseUp    (const juce::MouseEvent& e) override
    {
        if (getLocalBounds().contains (e.getPosition()) && onClick)
            onClick();
    }

private:
    juce::Image orb;
    juce::String name;
    bool selected = false, hovered = false;
};

//==============================================================================
// Small captioned value control for the ADVANCED panel (drag vertically).
//==============================================================================
class AdvancedCell : public juce::Component
{
public:
    enum class Icon { diffusion, width, air, character };

    AdvancedCell (VoidChamberReverbAudioProcessor& processor, const char* parameterID,
                  Icon icon, juce::String label, bool isCharacter = false);

    void paint (juce::Graphics&) override;
    void mouseEnter (const juce::MouseEvent&) override;
    void mouseExit  (const juce::MouseEvent&) override;
    void mouseDown  (const juce::MouseEvent&) override;
    void mouseDrag  (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void syncFromParameter();

private:
    void paintIcon (juce::Graphics&, juce::Rectangle<float> r);
    juce::String valueText() const;

    VoidChamberReverbAudioProcessor& processor;
    juce::String paramID, label;
    Icon icon;
    bool isCharacter;
    juce::RangedAudioParameter* parameter = nullptr;
    float dragOriginValue = 0.0f;
    bool hovered = false;
};

//==============================================================================
// Chevron arrow button (preset navigation / hamburger style).
//==============================================================================
class ChevronButton : public juce::Component
{
public:
    enum class Direction { left, right, menu };
    ChevronButton (Direction direction);

    std::function<void()> onClick;

    void paint (juce::Graphics&) override;
    void mouseEnter (const juce::MouseEvent&) override { hovered = true;  repaint(); }
    void mouseExit  (const juce::MouseEvent&) override { hovered = false; repaint(); }
    void mouseUp    (const juce::MouseEvent& e) override
    {
        if (getLocalBounds().contains (e.getPosition()) && onClick)
            onClick();
    }

private:
    Direction direction;
    bool hovered = false;
};

//==============================================================================
// Analytical per-band decay display, driven by the live parameter state.
//==============================================================================
class FrequencyDecayGraph : public juce::Component
{
public:
    FrequencyDecayGraph (juce::AudioProcessorValueTreeState& stateToRead);
    void paint (juce::Graphics&) override;

private:
    juce::AudioProcessorValueTreeState& state;
};

//==============================================================================
// Modal overlay listing all 50 factory presets in five categories.
//==============================================================================
class PresetBrowser : public juce::Component
{
public:
    PresetBrowser (VoidChamberReverbAudioProcessor& processor);

    std::function<void()> onRequestClose;

    void open (int initialPresetIndex);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    bool keyPressed (const juce::KeyPress&) override;

private:
    struct CategoryTab { juce::Rectangle<float> bounds; int categoryIndex; };
    struct PresetCell  { juce::Rectangle<float> bounds; int presetIndex; };

    void layout();

    VoidChamberReverbAudioProcessor& processor;
    std::vector<CategoryTab> tabs;
    std::vector<PresetCell> cells;
    juce::Rectangle<float> closeButton;
    int activeCategory = 0;
    int hoveredPreset  = -1;
    int hoveredCategory = -1;
    bool hoveredClose  = false;
};

//==============================================================================
class VoidChamberReverbAudioProcessorEditor : public juce::AudioProcessorEditor,
                                              private juce::Timer
{
public:
    explicit VoidChamberReverbAudioProcessorEditor (VoidChamberReverbAudioProcessor&);
    ~VoidChamberReverbAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;

    // Called once per refresh tick; public so headless renders (snapshot
    // test) can settle the dynamic visual state deterministically.
    void timerCallback() override;

private:
    void layoutContents (float scale, float offsetX, float offsetY);
    void updateCachedTexts();
    void paintSpacePanel (juce::Graphics&);
    void paintHeader (juce::Graphics&);
    void paintPanels (juce::Graphics&);
    void paintFrame (juce::Graphics&);

    juce::String sizeText { "72%" }, preDelayText { "28 ms" }, dampingText { "46%" },
                  modulationText { "33%" }, mixText { "75%" }, presetNameText { "Astral Hall" };
    int frameCounter = 0;

    VoidChamberReverbAudioProcessor& processor;

    // Assets
    juce::Image background;
    juce::Image orbImages[5];

    // Look & feel instances (shared per accent)
    VoidKnobLookAndFeel pinkFeel { VoidKnobLookAndFeel::Accent::pink };
    VoidKnobLookAndFeel cyanFeel { VoidKnobLookAndFeel::Accent::cyan };
    NeonButtonLookAndFeel toggleFeel;
    NeonPillLookAndFeel pillFeel;

    // Widgets
    juce::Slider sizeSlider, preDelaySlider, dampingSlider, modulationSlider, mixSlider;
    DecayArcSlider decaySlider;
    std::unique_ptr<juce::SliderParameterAttachment> sizeAttachment, preDelayAttachment,
                                                     decayAttachment, dampingAttachment,
                                                     modulationAttachment, mixAttachment;

    LevelMeter inputMeter;
    PinkLevelMeter outputMeter;
    juce::TextButton monoButton { "MONO" }, stereoButton { "STEREO" };
    OrbButton orbs[5];
    FrequencyDecayGraph freqGraph;
    AdvancedCell diffusionCell, widthCell, airCell, characterCell;
    ChevronButton presetPrev { ChevronButton::Direction::left };
    ChevronButton presetNext { ChevronButton::Direction::right };
    ChevronButton menuButton { ChevronButton::Direction::menu };
    juce::TextButton presetNameButton;
    std::unique_ptr<PresetBrowser> browser;

    float dBTextTimer = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VoidChamberReverbAudioProcessorEditor)
};
