#include "digital_delay.h"
#include "../dsp/delay_line_sdram.h"
#include "../config/constants.h"

namespace pedal {

static float DSY_SDRAM_BSS digital_buf[MAX_DELAY_SAMPLES];
static DelayLineSdram       digital_line;

void DigitalDelay::Init() {
    digital_line.Init(digital_buf, MAX_DELAY_SAMPLES);
    filter_.Init();
    filter_.SetKnob(0.5f);
    dc_.Init();
}

void DigitalDelay::Reset() {
    digital_line.Reset();
    dc_.Init();
}

StereoFrame DigitalDelay::Process(float input, const ParamSet& params) {
    const float delay_samps = params.time * SAMPLE_RATE;
    digital_line.SetDelay(delay_samps);

    float wet = digital_line.Read();
    filter_.SetKnob(params.filter);
    wet = filter_.Process(wet);

    const float feedback = wet * params.repeats;
    digital_line.Write(input + feedback);

    wet = dc_.Process(wet);

    return StereoFrame{wet, wet};
}

} // namespace pedal
