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
    bits_        = 16;
    bit_scale_   = 65536.0f;
    decimate_    = 1.0f;
}

void LofiDelay::Prepare(const ParamSet& params) {
    // bits range: 16 (grit=0) down to 4 (grit=1)
    bits_ = 16 - static_cast<int>(params.grit * 12.0f);
    if (bits_ < 1) bits_ = 1;
    bit_scale_ = powf(2.0f, static_cast<float>(bits_));

    // grit=0: decimation factor=1 (passthrough), grit=1: factor=16
    decimate_ = 1.0f + params.grit * 15.0f;
}

StereoFrame LofiDelay::Process(float input, const ParamSet& params) {
    const float delay_samps = params.time * SAMPLE_RATE;
    lofi_line.SetDelay(delay_samps);

    float wet = lofi_line.Read();

    // --- Bit crush ---
    if (bits_ < 16) {
        wet = roundf(wet * bit_scale_) / bit_scale_;
    }

    // --- Sample-rate reduction (decimation) ---
    // sr_counter_ accumulates; when >= factor, update held sample
    sr_counter_ += 1.0f;
    if (sr_counter_ >= decimate_) {
        sr_counter_ -= decimate_;
        held_sample_ = wet;
    }
    wet = held_sample_;

    const float feedback = wet * params.repeats;
    lofi_line.Write(input + feedback);

    wet = dc_.Process(wet);

    return StereoFrame{wet, wet};
}

} // namespace pedal
