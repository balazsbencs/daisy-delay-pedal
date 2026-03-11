#include "filter_delay.h"
#include "../dsp/delay_line_sdram.h"
#include "../config/constants.h"
#include <cmath>

namespace pedal {

static float DSY_SDRAM_BSS filter_buf[MAX_DELAY_SAMPLES];
static DelayLineSdram       filter_line;

void FilterDelay::Init() {
    filter_line.Init(filter_buf, MAX_DELAY_SAMPLES);
    lfo_.Init(1.0f, LfoWave::Sine);
    dc_.Init();
    z1_ = 0.0f;
    z2_ = 0.0f;
}

void FilterDelay::Reset() {
    filter_line.Reset();
    dc_.Init();
    z1_ = 0.0f;
    z2_ = 0.0f;
}

StereoFrame FilterDelay::Process(float input, const ParamSet& params) {
    const float delay_samps = params.time * SAMPLE_RATE;
    filter_line.SetDelay(delay_samps);

    lfo_.SetRate(params.mod_spd);
    const float lfo_val = lfo_.Process(); // -1..+1

    // Sweep cutoff: 200 Hz base + LFO * mod_dep * 2000 Hz
    float cutoff_hz = 200.0f + lfo_val * params.mod_dep * 2000.0f;
    if (cutoff_hz < 20.0f)          cutoff_hz = 20.0f;
    if (cutoff_hz > 20000.0f)       cutoff_hz = 20000.0f;

    // SVF coefficients
    // f = 2 * sin(pi * cutoff / sr)  -- cheap approximation
    const float f = 2.0f * sinf(3.14159265f * cutoff_hz * INV_SAMPLE_RATE);
    // q resonance: 0.5 (damped) to 3.5 (near self-oscillation)
    const float q = 0.5f + params.mod_dep * 3.0f;

    float wet = filter_line.Read();

    // State-variable filter (Chamberlin)
    const float hp = wet - q * z1_ - z2_;
    const float bp = f * hp + z1_;
    z1_            = bp;
    const float lp = f * bp + z2_;
    z2_            = lp;

    // Output is low-pass
    wet = lp;

    const float feedback = wet * params.repeats;
    filter_line.Write(input + feedback);

    wet = dc_.Process(wet);

    return StereoFrame{wet, wet};
}

} // namespace pedal
