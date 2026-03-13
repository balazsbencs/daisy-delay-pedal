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

void FilterDelay::Prepare(const ParamSet& params) {
    lfo_.SetRate(params.mod_spd);
}

StereoFrame FilterDelay::Process(float input, const ParamSet& params) {
    const float delay_samps = params.time * SAMPLE_RATE;
    filter_line.SetDelay(delay_samps);

    const float lfo_val = lfo_.Process(); // -1..+1

    // Sweep cutoff: 200 Hz base + LFO * mod_dep * 2000 Hz
    float cutoff_hz = 400.0f + lfo_val * params.mod_dep * 2000.0f;
    if (cutoff_hz < 40.0f)           cutoff_hz = 40.0f;
    if (cutoff_hz > 10000.0f)        cutoff_hz = 10000.0f;

    // SVF coefficients
    // f = 2 * sin(pi * cutoff / sr)  -- cheap approximation
    float f = 2.0f * sinf(3.14159265f * cutoff_hz * INV_SAMPLE_RATE);
    if (f > 1.7f) f = 1.7f; // Stability limit for Chamberlin SVF

    // Damping q: 0 = self-oscillate, 2 = critically damped.
    // Map mod_dep to increase resonance (decrease damping).
    const float q = 2.0f - params.mod_dep * 1.95f;

    float wet = filter_line.Read();

    // State-variable filter (Chamberlin)
    const float hp = wet - q * z1_ - z2_;
    const float bp = f * hp + z1_;
    z1_            = bp;
    const float lp = f * bp + z2_;
    z2_            = lp;

    // Output is low-pass
    wet = lp;

    // Hard-clamp before feedback: near self-oscillation (low q) the SVF output
    // can far exceed ±1.0, and writing that back into the delay line without a
    // guard causes exponential runaway.
    if (wet >  1.0f) wet =  1.0f;
    if (wet < -1.0f) wet = -1.0f;

    const float feedback = wet * params.repeats;
    filter_line.Write(input + feedback);

    wet = dc_.Process(wet);

    return StereoFrame{wet, wet};
}

} // namespace pedal
