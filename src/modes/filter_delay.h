#pragma once
#include "delay_mode.h"
#include "../dsp/lfo.h"
#include "../dsp/dc_blocker.h"

namespace pedal {

class FilterDelay : public DelayMode {
public:
    void Init()  override;
    void Reset() override;
    StereoFrame Process(float input, const ParamSet& params) override;
    const char* Name() const override { return "Filter"; }

private:
    Lfo       lfo_;
    DcBlocker dc_;

    // State-variable filter state
    float z1_ = 0.0f;
    float z2_ = 0.0f;
};

} // namespace pedal
