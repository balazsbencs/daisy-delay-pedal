#include "param_map.h"

namespace pedal {

const ParamRange& get_param_range(DelayModeId /*mode*/, ParamId param) {
    // All modes share default ranges for now
    // Per-mode customization can be added here later
    switch (param) {
        case ParamId::Time:    return default_ranges::TIME;
        case ParamId::Repeats: return default_ranges::REPEATS;
        case ParamId::Mix:     return default_ranges::MIX;
        case ParamId::Filter:  return default_ranges::FILTER;
        case ParamId::Grit:    return default_ranges::GRIT;
        case ParamId::ModSpd:  return default_ranges::MOD_SPD;
        case ParamId::ModDep:  return default_ranges::MOD_DEP;
        default:               return default_ranges::MIX;
    }
}

} // namespace pedal
