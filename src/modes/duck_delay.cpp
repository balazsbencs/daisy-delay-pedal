#include "duck_delay.h"
#include "../dsp/delay_line_sdram.h"
#include "../config/constants.h"

namespace pedal {

static float DSY_SDRAM_BSS duck_buf[MAX_DELAY_SAMPLES];
static DelayLineSdram       duck_line;

void DuckDelay::Init() {
    duck_line.Init(duck_buf, MAX_DELAY_SAMPLES);
    // Moderate attack, slower release for smooth ducking
    follower_.Init(10.0f, 150.0f);
    filter_.Init();
    filter_.SetKnob(0.5f);
    dc_.Init();
}

void DuckDelay::Reset() {
    duck_line.Reset();
    dc_.Init();
}

void DuckDelay::Prepare(const ParamSet& params) {
    filter_.SetKnob(params.filter);
}

StereoFrame DuckDelay::Process(float input, const ParamSet& params) {
    const float delay_samps = params.time * SAMPLE_RATE;
    duck_line.SetDelay(delay_samps);

    // grit controls duck threshold depth: 0=no duck, 1=full duck
    const float env        = follower_.Process(input);
    float duck_amount      = 1.0f - env * params.grit * 2.0f;
    if (duck_amount < 0.0f) duck_amount = 0.0f;
    if (duck_amount > 1.0f) duck_amount = 1.0f;

    float wet = duck_line.Read();
    wet = filter_.Process(wet);

    // Feedback is taken BEFORE ducking, so the delay history is preserved
    const float feedback = wet * params.repeats;
    duck_line.Write(input + feedback);

    // Duck the output only
    wet *= duck_amount;
    wet = dc_.Process(wet);

    return StereoFrame{wet, wet};
}

} // namespace pedal
