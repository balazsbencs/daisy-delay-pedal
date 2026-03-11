#include "tape_delay.h"
#include "../dsp/delay_line_sdram.h"
#include "../config/constants.h"

namespace pedal {

static float DSY_SDRAM_BSS tape_buf[MAX_DELAY_SAMPLES];
static DelayLineSdram       tape_line;

void TapeDelay::Init() {
    tape_line.Init(tape_buf, MAX_DELAY_SAMPLES);
    lfo_.Init(1.0f, LfoWave::Sine);
    filter_.Init();
    filter_.SetKnob(0.4f); // slight LP for tape warmth default
    sat_.Init();
    dc_.Init();
}

void TapeDelay::Reset() {
    tape_line.Reset();
    dc_.Init();
}

StereoFrame TapeDelay::Process(float input, const ParamSet& params) {
    lfo_.SetRate(params.mod_spd);
    const float lfo_val = lfo_.Process(); // -1..+1

    // wow/flutter: max deviation = mod_dep * 50 samples
    const float flutter     = params.mod_dep * 50.0f;
    const float base_samps  = params.time * SAMPLE_RATE;
    float delay_samps       = base_samps + lfo_val * flutter;
    if (delay_samps < 1.0f)                         delay_samps = 1.0f;
    if (delay_samps > static_cast<float>(MAX_DELAY_SAMPLES - 1))
        delay_samps = static_cast<float>(MAX_DELAY_SAMPLES - 1);

    tape_line.SetDelay(delay_samps);

    float wet = tape_line.Read();

    filter_.SetKnob(params.filter);
    wet = filter_.Process(wet);

    // Saturation on feedback path; drive scales with grit
    sat_.SetDrive(params.grit);
    wet = sat_.Process(wet * (1.0f + params.grit));

    const float feedback = wet * params.repeats;
    tape_line.Write(input + feedback);

    wet = dc_.Process(wet);

    return StereoFrame{wet, wet};
}

} // namespace pedal
