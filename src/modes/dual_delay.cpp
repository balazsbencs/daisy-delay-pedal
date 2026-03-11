#include "dual_delay.h"
#include "../dsp/delay_line_sdram.h"
#include "../config/constants.h"

namespace pedal {

static float DSY_SDRAM_BSS dual_buf_l[MAX_DELAY_SAMPLES];
static float DSY_SDRAM_BSS dual_buf_r[MAX_DELAY_SAMPLES];
static DelayLineSdram       dual_line_l;
static DelayLineSdram       dual_line_r;

void DualDelay::Init() {
    dual_line_l.Init(dual_buf_l, MAX_DELAY_SAMPLES);
    dual_line_r.Init(dual_buf_r, MAX_DELAY_SAMPLES);
    filter_l_.Init();
    filter_r_.Init();
    filter_l_.SetKnob(0.5f);
    filter_r_.SetKnob(0.5f);
    dc_l_.Init();
    dc_r_.Init();
}

void DualDelay::Reset() {
    dual_line_l.Reset();
    dual_line_r.Reset();
    dc_l_.Init();
    dc_r_.Init();
}

void DualDelay::Prepare(const ParamSet& params) {
    filter_l_.SetKnob(params.filter);
    filter_r_.SetKnob(params.filter);
}

StereoFrame DualDelay::Process(float input, const ParamSet& params) {
    // Left: base delay time
    // Right: detuned by mod_dep (0 = same, 1 = 50% longer)
    const float delay_l = params.time * SAMPLE_RATE;
    const float delay_r = params.time * SAMPLE_RATE * (1.0f + params.mod_dep * 0.5f);

    dual_line_l.SetDelay(delay_l);
    dual_line_r.SetDelay(delay_r);

    float wet_l = dual_line_l.Read();
    float wet_r = dual_line_r.Read();

    wet_l = filter_l_.Process(wet_l);
    wet_r = filter_r_.Process(wet_r);

    const float feedback_l = wet_l * params.repeats;
    const float feedback_r = wet_r * params.repeats;

    dual_line_l.Write(input + feedback_l);
    dual_line_r.Write(input + feedback_r);

    wet_l = dc_l_.Process(wet_l);
    wet_r = dc_r_.Process(wet_r);

    return StereoFrame{wet_l, wet_r};
}

} // namespace pedal
