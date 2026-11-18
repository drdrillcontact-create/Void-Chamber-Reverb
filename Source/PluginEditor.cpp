#include "PluginEditor.h"

namespace
{
    using Processor = VoidChamberReverbAudioProcessor;

    const juce::Colour cyan        { 0xff31e6ff };
    const juce::Colour magenta     { 0xffff3fbd };
    const juce::Colour purple      { 0xff9a55ff };
    const juce::Colour greyText    { 0xff8d93a6 };
    const juce::Colour dimText     { 0xff6b7288 };
    const juce::Colour brightText  { 0xfff2f4fc };

    // Design surface — the editor is a fixed 1440 x 960 panel
    const juce::Rectangle<int> inputMeterBounds  { 16, 156, 96, 288 };
    const juce::Rectangle<int> outputMeterBounds { 1328, 156, 96, 288 };
    const juce::Rectangle<int> monoBounds        { 16, 514, 96, 30 };
    const juce::Rectangle<int> stereoBounds      { 16, 548, 96, 30 };
    const juce::Rectangle<int> mixKnobBounds     { 1286, 468, 136, 136 };
    const juce::Rectangle<int> leftPanelBounds   { 36, 610, 412, 176 };
    const juce::Rectangle<int> rightPanelBounds  { 992, 610, 412, 176 };
    const juce::Rectangle<int> decayArcBounds    { 330, 150, 780, 428 };
    const juce::Rectangle<int> spacePanelBounds  { 20, 800, 400, 140 };
    const juce::Rectangle<int> freqPanelBounds   { 436, 800, 576, 140 };
    const juce::Rectangle<int> advPanelBounds    { 1028, 800, 392, 140 };
}

//==============================================================================
VoidChamberReverbAudioProcessorEditor::VoidChamberReverbAudioProcessorEditor(Processor& p)
    : AudioProcessorEditor(&p), processorRef(p),
      background (juce::ImageCache::getFromMemory (BinaryData::voidbackground_jpg, BinaryData::voidbackground_jpgSize)),
      orbVoid    (juce::ImageCache::getFromMemory (BinaryData::spacevoid_jpg,   BinaryData::spacevoid_jpgSize)),
      orbHall    (juce::ImageCache::getFromMemory (BinaryData::spacehall_jpg,   BinaryData::spacehall_jpgSize)),
      orbPlate   (juce::ImageCache::getFromMemory (BinaryData::spaceplate_jpg,  BinaryData::spaceplate_jpgSize)),
      orbCavern  (juce::ImageCache::getFromMemory (BinaryData::spacecavern_jpg, BinaryData::spacecavern_jpgSize)),
      orbAmbient (juce::ImageCache::getFromMemory (BinaryData::spaceambient_jpg,BinaryData::spaceambient_jpgSize)),
      freqChart (p.getValueTreeState())
{
    setLookAndFeel(&lookAndFeel);

    // Knob styling
    const auto styleKnob = [this](juce::Slider& knob, float min, float max, juce::Colour accent)
    {
        knob.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        knob.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        knob.setColour(juce::Slider::rotarySliderFillColourId, accent);
        knob.setRange(min, max);
        addAndMakeVisible(knob);
    };
    styleKnob(sizeKnob,       0.0f, 1.0f,   magenta);
    styleKnob(preDelayKnob,   0.0f, 200.0f, cyan);
    styleKnob(dampingKnob,    0.0f, 1.0f,   cyan);
    styleKnob(modulationKnob, 0.0f, 1.0f,   magenta);
    styleKnob(diffusionKnob,  0.0f, 100.0f, cyan);
    styleKnob(widthKnob,      0.0f, 200.0f, cyan);
    styleKnob(airKnob,        0.0f, 100.0f, magenta);
    styleKnob(mixKnob,        0.0f, 1.0f,   magenta);

    decayArc.setRange(0.1, 20.0);
    decayArc.setColour(juce::Slider::rotarySliderFillColourId, magenta);
    decayArc.outputLevelSource = [this] { return processorRef.getOutputLevel(); };
    addAndMakeVisible(decayArc);

    sizeAttach       = std::make_unique<SliderAttachment>(p.getValueTreeState(), Processor::sizeID,       sizeKnob);
    preDelayAttach   = std::make_unique<SliderAttachment>(p.getValueTreeState(), Processor::preDelayID,   preDelayKnob);
    dampingAttach    = std::make_unique<SliderAttachment>(p.getValueTreeState(), Processor::dampingID,    dampingKnob);
    modulationAttach = std::make_unique<SliderAttachment>(p.getValueTreeState(), Processor::modulationID, modulationKnob);
    diffusionAttach  = std::make_unique<SliderAttachment>(p.getValueTreeState(), Processor::diffusionID,  diffusionKnob);
    widthAttach      = std::make_unique<SliderAttachment>(p.getValueTreeState(), Processor::widthID,      widthKnob);
    airAttach        = std::make_unique<SliderAttachment>(p.getValueTreeState(), Processor::airID,        airKnob);
    mixAttach        = std::make_unique<SliderAttachment>(p.getValueTreeState(), Processor::mixID,        mixKnob);
    decayAttach      = std::make_unique<SliderAttachment>(p.getValueTreeState(), Processor::decayID,      decayArc);

    // Value readouts
    const auto styleValue = [this](juce::Label& label, float size)
    {
        label.setJustificationType(juce::Justification::centred);
        label.setFont(voidchamber::voidFont(size, juce::Font::plain));
        label.setColour(juce::Label::textColourId, brightText);
        addAndMakeVisible(label);
    };
    styleValue(sizeValue, 14.0f);
    styleValue(preDelayValue, 14.0f);
    styleValue(dampingValue, 14.0f);
    styleValue(modulationValue, 14.0f);
    styleValue(diffusionValue, 12.0f);
    styleValue(widthValue, 12.0f);
    styleValue(airValue, 12.0f);
    styleValue(mixValue, 16.0f);
    styleValue(characterValue, 11.0f);

    // Meters
    inputMeter.setAccent(cyan);
    outputMeter.setAccent(magenta);
    inputMeter.setLevelSource([this] { return processorRef.getInputLevel(); });
    outputMeter.setLevelSource([this] { return processorRef.getOutputLevel(); });
    addAndMakeVisible(inputMeter);
    addAndMakeVisible(outputMeter);

    // Space type orbs
    const std::array<juce::Image, 5> orbs { orbVoid, orbHall, orbPlate, orbCavern, orbAmbient };
    const char* const spaceNames[] = { "Void", "Hall", "Plate", "Cavern", "Ambient" };
    for (int i = 0; i < 5; ++i)
    {
        spaceButtons[(size_t) i] = std::make_unique<voidchamber::SpaceTypeButton>(orbs[(size_t) i], spaceNames[i]);
        spaceButtons[(size_t) i]->onClick = [this, i]
        {
            if (spaceAttachment)
                spaceAttachment->setValueAsCompleteGesture((float) i);
        };
        addAndMakeVisible(*spaceButtons[(size_t) i]);
    }

    spaceAttachment = std::make_unique<ParameterAttachment>(
        *p.getValueTreeState().getParameter(Processor::spaceTypeID),
        [this](float choice)
        {
            const auto index = juce::roundToInt(choice);
            for (size_t i = 0; i < spaceButtons.size(); ++i)
                spaceButtons[i]->setSelected(static_cast<int>(i) == index);
        });
    spaceAttachment->sendInitialUpdate();

    // Channel mode (MONO / STEREO)
    monoButton.setClickingTogglesState(true);
    stereoButton.setClickingTogglesState(true);
    monoButton.setColour(juce::TextButton::buttonOnColourId, cyan);
    stereoButton.setColour(juce::TextButton::buttonOnColourId, cyan);
    monoButton.onClick = [this]
    {
        if (channelModeAttachment)
            channelModeAttachment->setValueAsCompleteGesture(1.0f);
    };
    stereoButton.onClick = [this]
    {
        if (channelModeAttachment)
            channelModeAttachment->setValueAsCompleteGesture(0.0f);
    };
    addAndMakeVisible(monoButton);
    addAndMakeVisible(stereoButton);

    channelModeAttachment = std::make_unique<ParameterAttachment>(
        *p.getValueTreeState().getParameter(Processor::channelModeID),
        [this](float value)
        {
            const bool mono = value >= 0.5f;
            monoButton.setToggleState(mono, juce::dontSendNotification);
            stereoButton.setToggleState(! mono, juce::dontSendNotification);
        });
    channelModeAttachment->sendInitialUpdate();

    // Character (Dark / Neutral / Bright)
    characterButton.onClick = [this]
    {
        const auto current = (int) std::lround(*processorRef.getValueTreeState().getRawParameterValue(Processor::characterID));
        if (characterAttachment)
            characterAttachment->setValueAsCompleteGesture((float) ((current + 1) % 3));
    };
    addAndMakeVisible(characterButton);
    addAndMakeVisible(characterValue);

    characterAttachment = std::make_unique<ParameterAttachment>(
        *p.getValueTreeState().getParameter(Processor::characterID),
        [this](float value)
        {
            static const char* const names[] = { "DARK", "NEUTRAL", "BRIGHT" };
            const auto index = juce::jlimit(0, 2, (int) std::lround(value));
            characterValue.setText(names[index], juce::dontSendNotification);
        });
    characterAttachment->sendInitialUpdate();

    // Frequency decay chart
    addAndMakeVisible(freqChart);

    // Preset browser
    presetPrevButton.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    presetNextButton.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    presetMenuButton.setMouseCursor(juce::MouseCursor::PointingHandCursor);
    presetPrevButton.onClick = [this] { processorRef.loadPreset(processorRef.getCurrentPresetIndex() - 1); };
    presetNextButton.onClick = [this] { processorRef.loadPreset(processorRef.getCurrentPresetIndex() + 1); };
    presetMenuButton.onClick = [this] { showPresetMenu(); };
    presetPrevButton.setColour(juce::TextButton::textColourOffId, juce::Colour (0xffb9bfd2));
    presetNextButton.setColour(juce::TextButton::textColourOffId, juce::Colour (0xffb9bfd2));
    presetMenuButton.setColour(juce::TextButton::textColourOffId, juce::Colour (0xffb9bfd2));
    addAndMakeVisible(presetPrevButton);
    addAndMakeVisible(presetNextButton);
    addAndMakeVisible(presetMenuButton);

    presetNameLabel.setJustificationType(juce::Justification::centred);
    presetNameLabel.setFont(voidchamber::voidFont(13.5f, juce::Font::plain));
    presetNameLabel.setColour(juce::Label::textColourId, brightText);
    addAndMakeVisible(presetNameLabel);

    setSize(1440, 960);
    updateValueLabels();
    updatePresetName();
    startTimerHz(30);
}

VoidChamberReverbAudioProcessorEditor::~VoidChamberReverbAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

//==============================================================================
void VoidChamberReverbAudioProcessorEditor::showPresetMenu()
{
    juce::PopupMenu menu;
    menu.addSectionHeader("PRESETS");
    const auto current = processorRef.getCurrentPresetIndex();
    for (int i = 0; i < processorRef.getNumPresets(); ++i)
    {
        const auto index = i;
        menu.addItem(i + 1, processorRef.getProgramName(index), true, index == current);
    }
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(presetMenuButton)
                                               .withMaximumNumColumns(1),
                       [this](int result)
                       {
                           if (result > 0)
                               processorRef.loadPreset(result - 1);
                       });
}

void VoidChamberReverbAudioProcessorEditor::updateValueLabels()
{
    auto raw = [this](const char* id) -> float { return *processorRef.getValueTreeState().getRawParameterValue(id); };
    sizeValue.setText(juce::String((int) (raw(Processor::sizeID) * 100.0f)) + " %", juce::dontSendNotification);
    preDelayValue.setText(juce::String((int) std::lround(raw(Processor::preDelayID))) + " ms", juce::dontSendNotification);
    dampingValue.setText(juce::String((int) (raw(Processor::dampingID) * 100.0f)) + " %", juce::dontSendNotification);
    modulationValue.setText(juce::String((int) (raw(Processor::modulationID) * 100.0f)) + " %", juce::dontSendNotification);
    diffusionValue.setText(juce::String((int) raw(Processor::diffusionID)) + " %", juce::dontSendNotification);
    widthValue.setText(juce::String((int) raw(Processor::widthID)) + " %", juce::dontSendNotification);
    airValue.setText(juce::String((int) raw(Processor::airID)) + " %", juce::dontSendNotification);
    mixValue.setText(juce::String((int) (raw(Processor::mixID) * 100.0f)) + " %", juce::dontSendNotification);
}

void VoidChamberReverbAudioProcessorEditor::updatePresetName()
{
    presetNameLabel.setText(processorRef.getCurrentPresetName(), juce::dontSendNotification);
}

void VoidChamberReverbAudioProcessorEditor::timerCallback()
{
    inputMeter.tick();
    outputMeter.tick();
    const auto phase = std::fmod(juce::Time::getMillisecondCounterHiRes() * 0.00042, 1.0);
    decayArc.tick(phase);
    freqChart.repaint();
    updateValueLabels();
    updatePresetName();
}

//==============================================================================
void VoidChamberReverbAudioProcessorEditor::paint(juce::Graphics& g)
{
    const auto w = (float) getWidth();

    // Background scene
    if (background.isValid())
        g.drawImage(background, getLocalBounds().toFloat());
    else
        g.fillAll(juce::Colour(0xff05060a));

    // Top bar scrim + hairline
    {
        juce::ColourGradient scrim(juce::Colour(0xcc05060c), 0.0f, 0.0f,
                                   juce::Colour(0x0005060c), 0.0f, 46.0f, false);
        g.setGradientFill(scrim);
        g.fillRect(0, 0, getWidth(), 46);
        g.setColour(juce::Colour(0x22ffffff));
        g.drawHorizontalLine(45, 0.0f, w);
    }

    // D·A Production logo (bars + wordmark)
    {
        const int bx = 24, by = 14;
        const float barW = 3.4f, barGap = 4.6f;
        const float heights[] = { 9.0f, 15.0f, 6.0f, 18.0f, 12.0f };
        for (int i = 0; i < 5; ++i)
        {
            const float bh = heights[i];
            g.setColour(i == 2 ? cyan : juce::Colour(0xfff2f4fc));
            g.fillRoundedRectangle(bx + i * barGap, by + (20.0f - bh) * 0.5f, barW, bh, 1.6f);
        }
        g.setColour(juce::Colour(0xfff2f4fc));
        g.setFont(voidchamber::voidFont(15.0f, juce::Font::bold));
        g.drawText("D·A Production", 66, 10, 220, 28, juce::Justification::centredLeft);
    }

    // Centre eyebrow: SPACE BEYOND SOUND with flanking hairlines
    {
        const int cy = 23;
        g.setColour(juce::Colour(0x2effffff));
        g.drawLine(410.0f, (float) cy + 0.5f, 606.0f, (float) cy + 0.5f, 1.0f);
        g.drawLine(834.0f, (float) cy + 0.5f, 1030.0f, (float) cy + 0.5f, 1.0f);
        voidchamber::drawTrackedText(g, "SPACE BEYOND SOUND", { 440, cy - 9, 560, 18 },
                                     3.2f, juce::Colour(0xff9aa0b4), 10.5f, juce::Font::plain);
    }

    // Preset pill + hamburger pill (backgrounds; components draw their glyphs)
    {
        const auto pill = [](juce::Graphics& gg, juce::Rectangle<int> bounds)
        {
            const auto r = bounds.toFloat();
            gg.setColour(juce::Colour(0x6610141d));
            gg.fillRoundedRectangle(r, r.getHeight() / 2.0f);
            gg.setColour(juce::Colour(0x3affffff));
            gg.drawRoundedRectangle(r, r.getHeight() / 2.0f, 1.0f);
        };
        pill(g, { 1024, 9, 332, 28 });
        pill(g, { 1364, 9, 44, 28 });
        // Hamburger (three lines) inside the right pill
        g.setColour(juce::Colour(0xffb9bfd2));
        const float hx = 1378.0f;
        for (int i = 0; i < 3; ++i)
            g.fillRoundedRectangle(hx, 17.0f + i * 5.0f, 16.0f, 1.6f, 0.8f);
    }

    // Title block
    voidchamber::drawTrackedText(g, "VOID CHAMBER", { 0, 56, getWidth(), 62 },
                                 22.0f, brightText, 44.0f, juce::Font::bold);
    voidchamber::drawTrackedText(g, "REVERB", { 0, 112, getWidth(), 22 },
                                 17.0f, juce::Colour(0xff9aa0b4), 13.0f, juce::Font::plain);

    // Right-side tagline, stacked
    {
        const int tx = 1132, tw = 156;
        voidchamber::drawTrackedText(g, "INFINITE",  { tx, 150, tw, 18 }, 2.6f, dimText, 10.5f, juce::Font::plain, juce::Justification::right);
        voidchamber::drawTrackedText(g, "SPACES.",   { tx, 168, tw, 18 }, 2.6f, dimText, 10.5f, juce::Font::plain, juce::Justification::right);
        voidchamber::drawTrackedText(g, "REAL",      { tx, 194, tw, 18 }, 2.6f, dimText, 10.5f, juce::Font::plain, juce::Justification::right);
        voidchamber::drawTrackedText(g, "EMOTIONS",  { tx, 212, tw, 18 }, 2.6f, dimText, 10.5f, juce::Font::plain, juce::Justification::right);
    }

    // Panels
    voidchamber::drawCapsulePanel(g, leftPanelBounds);
    voidchamber::drawCapsulePanel(g, rightPanelBounds);
    voidchamber::drawCapsulePanel(g, spacePanelBounds);
    voidchamber::drawCapsulePanel(g, freqPanelBounds);
    voidchamber::drawCapsulePanel(g, advPanelBounds);

    // Meter captions
    voidchamber::drawTrackedText(g, "INPUT",  { inputMeterBounds.getX(), 124, inputMeterBounds.getWidth(), 18 },
                                 3.0f, juce::Colour(0xffb9bfd2), 11.0f, juce::Font::bold);
    voidchamber::drawTrackedText(g, "OUTPUT", { outputMeterBounds.getX(), 124, outputMeterBounds.getWidth(), 18 },
                                 3.0f, juce::Colour(0xffb9bfd2), 11.0f, juce::Font::bold);
    g.setColour(dimText);
    g.setFont(voidchamber::voidFont(10.0f));
    g.drawText("0.0 dB", inputMeterBounds.getX(), 452, inputMeterBounds.getWidth(), 16, juce::Justification::centred);
    g.drawText("0.0 dB", outputMeterBounds.getX(), 452, outputMeterBounds.getWidth(), 16, juce::Justification::centred);

    // Knob name captions
    const auto caption = [this, &g](const char* text, int cx, int y, int widthPx = 150)
    {
        voidchamber::drawTrackedText(g, text, { cx - widthPx / 2, y, widthPx, 16 },
                                     1.6f, greyText, 10.5f, juce::Font::bold);
    };
    caption("SIZE",       150, 626);
    caption("PRE DELAY",  334, 626);
    caption("DAMPING",   1106, 626);
    caption("MODULATION", 1290, 626);

    // Panel section headings
    voidchamber::drawTrackedText(g, "SPACE TYPE", { 36, spacePanelBounds.getY() + 10, 220, 16 },
                                 2.0f, greyText, 10.0f, juce::Font::bold, juce::Justification::left);
    voidchamber::drawTrackedText(g, "FREQUENCY DECAY", { freqPanelBounds.getX() + 16, freqPanelBounds.getY() + 10, 240, 16 },
                                 2.0f, greyText, 10.0f, juce::Font::bold, juce::Justification::left);
    voidchamber::drawTrackedText(g, "ADVANCED", { advPanelBounds.getX() + 16, advPanelBounds.getY() + 10, 200, 16 },
                                 2.0f, greyText, 10.0f, juce::Font::bold, juce::Justification::left);

    // FREQUENCY DECAY legend — LOW / MID / HIGH
    {
        const int ly = freqPanelBounds.getY() + 18;
        struct Legend { int x; const char* text; juce::Colour colour; };
        const Legend items[] = { { 872, "LOW", magenta }, { 931, "MID", purple }, { 987, "HIGH", cyan } };
        g.setFont(voidchamber::voidFont(8.5f, juce::Font::bold));
        for (const auto& item : items)
        {
            g.setColour(item.colour);
            g.fillEllipse((float) item.x, (float) ly + 1.0f, 7.0f, 7.0f);
            g.setColour(greyText);
            g.drawSingleLineText(item.text, item.x + 10, ly + 8);
        }
    }

    // Chart x-axis labels (log spaced 20 Hz .. 20 kHz)
    {
        const int chartX = freqPanelBounds.getX() + 16;
        const int chartW = freqPanelBounds.getWidth() - 32;
        const int labelY = freqPanelBounds.getBottom() - 18;
        g.setColour(dimText);
        g.setFont(voidchamber::voidFont(8.5f));
        const char* const freqLabels[] = { "20", "100", "1K", "10K", "20K" };
        const float positions[] = { 0.0f, 0.233f, 0.5f, 0.767f, 1.0f };
        for (int i = 0; i < 5; ++i)
        {
            const float fx = chartX + positions[i] * (float) chartW;
            if (i == 0)      g.drawText(freqLabels[i], (int) fx - 4,   labelY, 26, 12, juce::Justification::left);
            else if (i == 4) g.drawText(freqLabels[i], (int) fx - 22,  labelY, 26, 12, juce::Justification::right);
            else             g.drawText(freqLabels[i], (int) fx - 13,  labelY, 26, 12, juce::Justification::centred);
        }
    }

    // ADVANCED mini-knob captions
    const auto miniCaption = [this, &g](const char* text, int cx)
    {
        voidchamber::drawTrackedText(g, text, { cx - 46, advPanelBounds.getY() + 62, 92, 12 },
                                     0.8f, greyText, 6.8f, juce::Font::bold);
    };
    miniCaption("DIFFUSION", 1092);
    miniCaption("WIDTH",     1180);
    miniCaption("AIR",       1268);
    voidchamber::drawTrackedText(g, "CHARACTER", { 1320, advPanelBounds.getY() + 64, 88, 12 },
                                 0.8f, greyText, 6.8f, juce::Font::bold);

    // OUTPUT MIX caption
    voidchamber::drawTrackedText(g, "OUTPUT MIX", { mixKnobBounds.getX() - 10, 434, mixKnobBounds.getWidth() + 20, 16 },
                                 2.0f, greyText, 10.0f, juce::Font::bold);
}

void VoidChamberReverbAudioProcessorEditor::resized()
{
    inputMeter.setBounds(inputMeterBounds);
    outputMeter.setBounds(outputMeterBounds);

    monoButton.setBounds(monoBounds);
    stereoButton.setBounds(stereoBounds);

    // Preset pill row (top right)
    presetPrevButton.setBounds(1030, 11, 26, 24);
    presetNameLabel.setBounds(1056, 11, 256, 24);
    presetNextButton.setBounds(1312, 11, 26, 24);
    presetMenuButton.setBounds(1366, 11, 38, 24);

    // Central decay arc
    decayArc.setBounds(decayArcBounds);

    // Control-panel knobs
    sizeKnob.setBounds(103, 646, 94, 94);
    preDelayKnob.setBounds(287, 646, 94, 94);
    dampingKnob.setBounds(1059, 646, 94, 94);
    modulationKnob.setBounds(1243, 646, 94, 94);

    sizeValue.setBounds(103, 744, 94, 16);
    preDelayValue.setBounds(287, 744, 94, 16);
    dampingValue.setBounds(1059, 744, 94, 16);
    modulationValue.setBounds(1243, 744, 94, 16);

    // Output mix knob (right edge, below the meter)
    mixKnob.setBounds(mixKnobBounds.reduced(14, 8));
    mixValue.setBounds(mixKnobBounds.getX() - 10, 588,
                       mixKnobBounds.getWidth() + 20, 16);

    // Space type orbs
    const int orbSize = 74, orbGap = 1;
    for (int i = 0; i < 5; ++i)
        spaceButtons[(size_t) i]->setBounds(28 + i * (orbSize + orbGap), 832, orbSize, 104);

    // Frequency decay chart
    freqChart.setBounds(freqPanelBounds.getX() + 16, freqPanelBounds.getY() + 28,
                        freqPanelBounds.getWidth() - 32, 54);

    // Advanced cells
    diffusionKnob.setBounds(1092 - 23, advPanelBounds.getY() + 14, 46, 46);
    widthKnob.setBounds(1180 - 23,     advPanelBounds.getY() + 14, 46, 46);
    airKnob.setBounds(1268 - 23,       advPanelBounds.getY() + 14, 46, 46);
    diffusionValue.setBounds(1092 - 44, advPanelBounds.getY() + 78, 88, 14);
    widthValue.setBounds(1180 - 44,     advPanelBounds.getY() + 78, 88, 14);
    airValue.setBounds(1268 - 44,       advPanelBounds.getY() + 78, 88, 14);

    characterButton.setBounds(1364 - 24, advPanelBounds.getY() + 12, 48, 46);
    characterValue.setBounds(1320, advPanelBounds.getY() + 78, 88, 14);
}
