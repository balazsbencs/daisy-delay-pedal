#pragma once
#include "delay_mode.h"
#include "../dsp/dc_blocker.h"

namespace pedal {

class LofiDelay : public DelayMode {
public:
    void Init()  override;
    void Reset() override;
    StereoFrame Process(float input, const ParamSet& params) override;
    const char* Name() const override { return "Lofi"; }

private:
    DcBlocker dc_;

    // Sample-rate reduction state: hold output until decimation counter expires
    float    held_sample_  = 0.0f;
    float    sr_counter_   = 0.0f;  // accumulator for decimation
};

} // namespace pedal
