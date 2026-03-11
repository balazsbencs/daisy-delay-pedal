#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <array>

class DelayPluginEditor : public juce::AudioProcessorEditor {
public:
    explicit DelayPluginEditor(DelayPluginProcessor&);
    ~DelayPluginEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    struct Knob {
        juce::Slider slider;
        juce::Label  label;
        std::unique_ptr<SliderAttachment> attach;
    };

    DelayPluginProcessor& processor_;

    std::array<Knob, 7> knobs_;
    juce::ComboBox mode_box_;
    juce::Label    mode_label_;
    std::unique_ptr<ComboAttachment> mode_attach_;

    juce::ToggleButton bypass_btn_{"Bypass"};
    std::unique_ptr<ButtonAttachment> bypass_attach_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DelayPluginEditor)
};
