#include "dbucket_delay.h"
#include "../dsp/delay_line_sdram.h"
#include "../config/constants.h"
#include <cstdint>

namespace pedal {

static float DSY_SDRAM_BSS dbucket_buf[MAX_DELAY_SAMPLES];
static DelayLineSdram       dbucket_line;

// LCG noise: returns -1..+1
float DbucketDelay::next_noise() {
    noise_seed_ = noise_seed_ * 1664525u + 1013904223u;
    // Map top 16 bits to -1..+1
    const int16_t s = static_cast<int16_t>(noise_seed_ >> 16u);
    return static_cast<float>(s) * (1.0f / 32768.0f);
}

void DbucketDelay::Init() {
    dbucket_line.Init(dbucket_buf, MAX_DELAY_SAMPLES);
    filter_.Init();
    filter_.SetKnob(0.4f); // slightly LP to emulate BBD default character
    dc_.Init();
    noise_seed_ = 12345u;
}

void DbucketDelay::Reset() {
    dbucket_line.Reset();
    dc_.Init();
    noise_seed_ = 12345u;
}

StereoFrame DbucketDelay::Process(float input, const ParamSet& params) {
    const float delay_samps = params.time * SAMPLE_RATE;
    dbucket_line.SetDelay(delay_samps);

    // BBDs progressively roll off HF per repeat; map grit to a lower filter knob
    // grit=0 -> knob=0.4 (gentle LP), grit=1 -> knob=0.1 (heavy LP)
    const float filter_knob = 0.4f - params.grit * 0.3f;
    filter_.SetKnob(filter_knob);

    float wet = dbucket_line.Read();
    wet       = filter_.Process(wet);

    // Add clock noise proportional to grit
    wet += next_noise() * params.grit * 0.003f;

    const float feedback = wet * params.repeats;
    dbucket_line.Write(input + feedback);

    wet = dc_.Process(wet);

    return StereoFrame{wet, wet};
}

} // namespace pedal
