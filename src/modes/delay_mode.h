#pragma once
#include "../audio/stereo_frame.h"
#include "../params/param_set.h"

namespace pedal {

class DelayMode {
public:
    virtual ~DelayMode() = default;
    virtual void Init()  = 0;
    virtual void Reset() = 0;
    // Process one input sample, return stereo output (wet only, before mix)
    virtual StereoFrame Process(float input, const ParamSet& params) = 0;
    virtual const char* Name() const = 0;
};

} // namespace pedal
