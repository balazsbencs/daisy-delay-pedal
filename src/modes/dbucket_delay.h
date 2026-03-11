#pragma once
#include "delay_mode.h"
#include "../dsp/tone_filter.h"
#include "../dsp/dc_blocker.h"

namespace pedal {

class DbucketDelay : public DelayMode {
public:
    void Init()  override;
    void Reset() override;
    StereoFrame Process(float input, const ParamSet& params) override;
    const char* Name() const override { return "Dbucket"; }

private:
    ToneFilter filter_;
    DcBlocker  dc_;

    // Simple LCG for deterministic noise
    uint32_t noise_seed_ = 12345u;

    float next_noise();
};

} // namespace pedal
