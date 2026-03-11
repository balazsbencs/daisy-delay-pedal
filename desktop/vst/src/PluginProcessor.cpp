#include "PluginProcessor.h"
#include "PluginEditor.h"

#include "params/param_map.h"
#include "params/param_id.h"
#include "params/param_range.h"
#include <cmath>

namespace {
float getParam(const juce::AudioProcessorValueTreeState& apvts, const juce::String& id) {
    if (auto* p = apvts.getRawParameterValue(id)) {
        return p->load();
    }
    return 0.0f;
}

float clamp01(float v) {
    if (v < 0.0f) {
        return 0.0f;
    }
    if (v > 1.0f) {
        return 1.0f;
    }
    return v;
}
} // namespace

DelayPluginProcessor::DelayPluginProcessor()
    : AudioProcessor(BusesProperties()
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true))
    , apvts_(*this, nullptr, "PARAMS", createParameterLayout()) {
    mode_registry_.Init();
    active_mode_ = mode_registry_.get(current_mode_);
    active_mode_->Reset();
}

void DelayPluginProcessor::prepareToPlay(double, int) {
    ensureModeFromParam();
    if (active_mode_ != nullptr) {
        active_mode_->Reset();
    }
}

void DelayPluginProcessor::releaseResources() {}

bool DelayPluginProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const {
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo()) {
        return false;
    }
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()) {
        return false;
    }
    return true;
}

pedal::ParamSet DelayPluginProcessor::buildParamsFromState() const {
    using namespace pedal;

    const float n_time = clamp01(getParam(apvts_, "time"));
    const float n_rep  = clamp01(getParam(apvts_, "repeats"));
    const float n_mix  = clamp01(getParam(apvts_, "mix"));
    const float n_fil  = clamp01(getParam(apvts_, "filter"));
    const float n_grit = clamp01(getParam(apvts_, "grit"));
    const float n_mspd = clamp01(getParam(apvts_, "modspd"));
    const float n_mdep = clamp01(getParam(apvts_, "moddep"));

    ParamSet ps;
    ps.time    = map_param(n_time, get_param_range(current_mode_, ParamId::Time));
    ps.repeats = map_param(n_rep, get_param_range(current_mode_, ParamId::Repeats));
    ps.mix     = map_param(n_mix, get_param_range(current_mode_, ParamId::Mix));
    ps.filter  = map_param(n_fil, get_param_range(current_mode_, ParamId::Filter));
    ps.grit    = map_param(n_grit, get_param_range(current_mode_, ParamId::Grit));
    ps.mod_spd = map_param(n_mspd, get_param_range(current_mode_, ParamId::ModSpd));
    ps.mod_dep = map_param(n_mdep, get_param_range(current_mode_, ParamId::ModDep));
    return ps;
}

void DelayPluginProcessor::ensureModeFromParam() {
    const int next_mode = juce::jlimit(0, pedal::NUM_MODES - 1, (int)std::lround(getParam(apvts_, "mode")));
    const auto next_id  = static_cast<pedal::DelayModeId>(next_mode);
    if (next_id == current_mode_) {
        return;
    }

    current_mode_ = next_id;
    active_mode_  = mode_registry_.get(current_mode_);
    if (active_mode_ != nullptr) {
        active_mode_->Reset();
    }
}

void DelayPluginProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) {
    juce::ScopedNoDenormals noDenormals;

    ensureModeFromParam();
    if (active_mode_ == nullptr) {
        buffer.clear();
        return;
    }

    const bool bypassed = getParam(apvts_, "bypass") >= 0.5f;
    const auto ps       = buildParamsFromState();
    active_mode_->Prepare(ps);

    auto* left  = buffer.getWritePointer(0);
    auto* right = buffer.getWritePointer(1);
    const int n = buffer.getNumSamples();

    if (bypassed) {
        return;
    }

    const float angle = ps.mix * 1.57079632679f;
    const float dry_g = std::cos(angle);
    const float wet_g = std::sin(angle);

    for (int i = 0; i < n; ++i) {
        const float dry = 0.5f * (left[i] + right[i]);
        const auto wet  = active_mode_->Process(dry, ps);

        left[i]  = dry * dry_g + wet.left * wet_g;
        right[i] = dry * dry_g + wet.right * wet_g;
    }
}

juce::AudioProcessorEditor* DelayPluginProcessor::createEditor() {
    return new DelayPluginEditor(*this);
}

bool DelayPluginProcessor::hasEditor() const { return true; }
const juce::String DelayPluginProcessor::getName() const { return JucePlugin_Name; }
bool DelayPluginProcessor::acceptsMidi() const { return false; }
bool DelayPluginProcessor::producesMidi() const { return false; }
bool DelayPluginProcessor::isMidiEffect() const { return false; }
double DelayPluginProcessor::getTailLengthSeconds() const { return 3.0; }

int DelayPluginProcessor::getNumPrograms() { return 1; }
int DelayPluginProcessor::getCurrentProgram() { return 0; }
void DelayPluginProcessor::setCurrentProgram(int) {}
const juce::String DelayPluginProcessor::getProgramName(int) { return {}; }
void DelayPluginProcessor::changeProgramName(int, const juce::String&) {}

void DelayPluginProcessor::getStateInformation(juce::MemoryBlock& destData) {
    auto state = apvts_.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void DelayPluginProcessor::setStateInformation(const void* data, int sizeInBytes) {
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState == nullptr) {
        return;
    }
    if (!xmlState->hasTagName(apvts_.state.getType())) {
        return;
    }
    apvts_.replaceState(juce::ValueTree::fromXml(*xmlState));
    ensureModeFromParam();
}

juce::AudioProcessorValueTreeState::ParameterLayout DelayPluginProcessor::createParameterLayout() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;

    auto addNorm = [&](const juce::String& id, const juce::String& name, float def) {
        p.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{id, 1}, name, juce::NormalisableRange<float>(0.0f, 1.0f), def));
    };

    addNorm("time", "Time", 0.5f);
    addNorm("repeats", "Repeats", 0.4f);
    addNorm("mix", "Mix", 0.5f);
    addNorm("filter", "Filter", 0.5f);
    addNorm("grit", "Grit", 0.0f);
    addNorm("modspd", "Mod Speed", 0.5f);
    addNorm("moddep", "Mod Depth", 0.0f);

    juce::StringArray modeNames;
    modeNames.add("Duck");
    modeNames.add("Swell");
    modeNames.add("Trem");
    modeNames.add("Digital");
    modeNames.add("DBucket");
    modeNames.add("Tape");
    modeNames.add("Dual");
    modeNames.add("Pattern");
    modeNames.add("Filter");
    modeNames.add("LoFi");
    p.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"mode", 1}, "Mode", modeNames, (int)pedal::DelayModeId::Digital));

    p.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"bypass", 1}, "Bypass", false));

    return {p.begin(), p.end()};
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() {
    return new DelayPluginProcessor();
}
