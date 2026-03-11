#include "lofi_delay.h"
#include "../dsp/delay_line_sdram.h"
#include "../config/constants.h"
#include <cmath>

namespace pedal {

static float DSY_SDRAM_BSS lofi_buf[MAX_DELAY_SAMPLES];
static DelayLineSdram       lofi_line;

void LofiDelay::Init() {
    lofi_line.Init(lofi_buf, MAX_DELAY_SAMPLES);
    dc_.Init();
    held_sample_ = 0.0f;
    sr_counter_  = 0.0f;
}

void LofiDelay::Reset() {
    lofi_line.Reset();
    dc_.Init();
    held_sample_ = 0.0f;
    sr_counter_  = 0.0f;
}

StereoFrame LofiDelay::Process(float input, const ParamSet& params) {
    const float delay_samps = params.time * SAMPLE_RATE;
    lofi_line.SetDelay(delay_samps);

    float wet = lofi_line.Read();

    // --- Bit crush ---
    // bits range: 16 (grit=0) down to 4 (grit=1)
    const int bits = 16 - static_cast<int>(params.grit * 12.0f);
    if (bits >= 1) {
        const float scale = powf(2.0f, static_cast<float>(bits));
        wet = roundf(wet * scale) / scale;
    }

    // --- Sample-rate reduction (decimation) ---
    // grit=0: decimation factor=1 (passthrough), grit=1: factor=16
    // sr_counter_ accumulates; when >= factor, update held sample
    const float decimate = 1.0f + params.grit * 15.0f; // 1..16
    sr_counter_ += 1.0f;
    if (sr_counter_ >= decimate) {
        sr_counter_ -= decimate;
        held_sample_ = wet;
    }
    wet = held_sample_;

    const float feedback = wet * params.repeats;
    lofi_line.Write(input + feedback);

    wet = dc_.Process(wet);

    return StereoFrame{wet, wet};
}

} // namespace pedal
