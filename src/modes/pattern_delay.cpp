#include "pattern_delay.h"
#include "../dsp/delay_line_sdram.h"
#include "../config/constants.h"

namespace pedal {

// Must define the constexpr static data member in exactly one TU
constexpr float PatternDelay::PATTERNS[3][3];

static float DSY_SDRAM_BSS pattern_buf[MAX_DELAY_SAMPLES];
static DelayLineSdram       pattern_line;

void PatternDelay::Init() {
    pattern_line.Init(pattern_buf, MAX_DELAY_SAMPLES);
    lfo_.Init(1.0f, LfoWave::Sine);
    filter_.Init();
    filter_.SetKnob(0.5f);
    dc_.Init();
}

void PatternDelay::Reset() {
    pattern_line.Reset();
    dc_.Init();
}

void PatternDelay::Prepare(const ParamSet& params) {
    lfo_.SetRate(params.mod_spd);
    lfo_out_ = lfo_.PrepareBlock();
    filter_.SetKnob(params.filter);
}

StereoFrame PatternDelay::Process(float input, const ParamSet& params) {
    const float lfo_val    = lfo_out_;
    const float base_samps = params.time * SAMPLE_RATE
                           + lfo_val * (params.mod_dep * 25.0f);

    // Select pattern: grit 0..0.333 -> 0, 0.333..0.667 -> 1, 0.667..1 -> 2
    const int pat_idx = [&]() -> int {
        const int idx = static_cast<int>(params.grit * 3.0f);
        if (idx < 0) return 0;
        if (idx > 2) return 2;
        return idx;
    }();

    // Sum three rhythmic taps
    float wet = 0.0f;
    for (int i = 0; i < 3; ++i) {
        float tap_samps = base_samps * PATTERNS[pat_idx][i];
        if (tap_samps < 1.0f)
            tap_samps = 1.0f;
        if (tap_samps > static_cast<float>(MAX_DELAY_SAMPLES - 1))
            tap_samps = static_cast<float>(MAX_DELAY_SAMPLES - 1);
        wet += pattern_line.ReadAt(tap_samps);
    }
    wet *= (1.0f / 3.0f); // normalise sum of taps

    wet = filter_.Process(wet);

    // Feedback driven from first tap only
    const float first_tap_samps = base_samps * PATTERNS[pat_idx][0];
    const float first_tap       = pattern_line.ReadAt(
        (first_tap_samps < 1.0f) ? 1.0f : first_tap_samps);

    const float feedback = first_tap * params.repeats;
    pattern_line.Write(input + feedback);

    wet = dc_.Process(wet);

    return StereoFrame{wet, wet};
}

} // namespace pedal
