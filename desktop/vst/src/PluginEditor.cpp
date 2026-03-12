#include "PluginEditor.h"

namespace {
constexpr const char* kParamIds[7] = {
    "time", "repeats", "mix", "filter", "grit", "modspd", "moddep"
};

constexpr const char* kParamNames[7] = {
    "Time", "Repeats", "Mix", "Filter", "Grit", "Mod Spd", "Mod Dep"
};

constexpr const char* kModeNames[pedal::NUM_MODES] = {
    "Duck", "Swell", "Trem", "Digital", "DBucket", "Tape", "Dual", "Pattern", "Filter", "LoFi"
};
} // namespace

DelayPluginEditor::DelayPluginEditor(DelayPluginProcessor& p)
    : juce::AudioProcessorEditor(&p)
    , processor_(p) {
    auto& apvts = processor_.parameters();

    for (int i = 0; i < (int)knobs_.size(); ++i) {
        auto& k = knobs_[i];
        k.slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        k.slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 18);
        k.slider.setRange(0.0, 1.0, 0.001);
        addAndMakeVisible(k.slider);

        k.label.setText(kParamNames[i], juce::dontSendNotification);
        k.label.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(k.label);

        k.attach = std::make_unique<SliderAttachment>(apvts, kParamIds[i], k.slider);
    }

    mode_label_.setText("Mode", juce::dontSendNotification);
    mode_label_.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(mode_label_);

    for (int i = 0; i < pedal::NUM_MODES; ++i) {
        mode_box_.addItem(kModeNames[i], i + 1);
    }
    addAndMakeVisible(mode_box_);
    mode_attach_ = std::make_unique<ComboAttachment>(apvts, "mode", mode_box_);

    addAndMakeVisible(bypass_btn_);
    bypass_attach_ = std::make_unique<ButtonAttachment>(apvts, "bypass", bypass_btn_);

    setSize(760, 340);
}

void DelayPluginEditor::paint(juce::Graphics& g) {
    g.fillAll(juce::Colour::fromRGB(22, 24, 28));

    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(22.0f, juce::Font::bold));
    g.drawText("BB DigitDelay", 16, 10, 220, 30, juce::Justification::left);

    g.setColour(juce::Colour::fromRGB(70, 76, 90));
    g.drawLine(16.0f, 46.0f, (float)getWidth() - 16.0f, 46.0f, 1.0f);
}

void DelayPluginEditor::resized() {
    constexpr int pad = 16;
    constexpr int knobW = 100;
    constexpr int knobH = 122;

    // top row: 4 knobs
    for (int i = 0; i < 4; ++i) {
        const int x = pad + i * (knobW + 12);
        knobs_[(size_t)i].slider.setBounds(x, 64, knobW, knobH - 24);
        knobs_[(size_t)i].label.setBounds(x, 42, knobW, 20);
    }

    // bottom row: 3 knobs
    for (int i = 0; i < 3; ++i) {
        const int x = pad + i * (knobW + 12);
        knobs_[(size_t)(i + 4)].slider.setBounds(x, 202, knobW, knobH - 24);
        knobs_[(size_t)(i + 4)].label.setBounds(x, 180, knobW, 20);
    }

    mode_label_.setBounds(500, 72, 70, 20);
    mode_box_.setBounds(570, 68, 160, 28);

    bypass_btn_.setBounds(570, 112, 120, 28);
}
