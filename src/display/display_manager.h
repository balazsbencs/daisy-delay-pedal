#pragma once
#include "st7789_driver.h"
#include "../config/delay_mode_id.h"
#include "../params/param_set.h"
#include "../audio/bypass.h"
#include "../tempo/tempo_sync.h"
#include <cstdint>

namespace pedal {

/// Owns the ST7789 display driver and exposes a single Update() call that
/// rate-limits redraws to approximately 30 fps (DISPLAY_UPDATE_MS).
/// Rendering writes to an SDRAM frame buffer; transfer to the display happens
/// asynchronously via SPI DMA so the main loop is never blocked.
class DisplayManager {
public:
    enum class PresetUiEvent {
        None,
        Loaded,
        Saved,
    };

    void Init();

    /// Call once per main loop iteration.
    /// Skips the redraw if the previous DMA transfer is still running or the
    /// minimum update interval has not elapsed.
    void Update(DelayModeId      mode,
                const ParamSet&  params,
                const Bypass&    bypass,
                const TempoSync& tempo,
                int              preset_slot,
                bool             shift_layer_active,
                PresetUiEvent    preset_event,
                uint32_t         now_ms);

private:
    void Render(DelayModeId      mode,
                const ParamSet&  params,
                const Bypass&    bypass,
                const TempoSync& tempo,
                int              preset_slot,
                bool             shift_layer_active,
                PresetUiEvent    preset_event);

    static const char* ModeName(DelayModeId id);

    St7789Driver driver_;
};

} // namespace pedal
