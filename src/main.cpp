#include "daisy_seed.h"
#include "config/constants.h"
#include "config/delay_mode_id.h"
#include "config/pin_map.h"
#include "hardware/controls.h"
#include "audio/audio_engine.h"
#include "audio/bypass.h"
#include "params/param_set.h"
#include "params/param_map.h"
#include "params/param_range.h"
#include "params/param_id.h"
#include "modes/mode_registry.h"
#include "midi/midi_handler.h"
#include "display/display_manager.h"
#include "tempo/tempo_sync.h"

using namespace daisy;

// ── Hardware objects ──────────────────────────────────────────────────────────
static DaisySeed hw;
static GPIO      relay;
static GPIO      led_bypass;

// ── Subsystem singletons ──────────────────────────────────────────────────────
static pedal::Controls        controls;
static pedal::AudioEngine     audio_engine;
static pedal::ModeRegistry    mode_registry;
static pedal::MidiHandlerPedal midi_handler;
static pedal::DisplayManager  display;
static pedal::TempoSync       tempo_sync;

// ── Mutable main-loop state ───────────────────────────────────────────────────
static pedal::DelayModeId current_mode  = pedal::DelayModeId::Digital;
static pedal::Bypass      bypass_disp;  // authoritative bypass state object

// MIDI CC override table: one slot per pot, -1.0f signals "no override active".
static float midi_cc_val[pedal::NUM_POTS];
static bool  midi_cc_active[pedal::NUM_POTS];

// ── Helpers ───────────────────────────────────────────────────────────────────

/// Build an immutable ParamSet from the current control and MIDI state.
/// MIDI CC values take priority over pot positions when active.
/// @param ctrl           Smoothed pot + button readings from this iteration.
/// @param mode           Active delay mode (used for per-mode param ranges).
/// @param time_override  Seconds from TempoSync, or -1 when no sync is active.
static pedal::ParamSet BuildParams(const pedal::ControlState& ctrl,
                                   pedal::DelayModeId          mode,
                                   float                       time_override) {
    using namespace pedal;

    // Resolve the effective normalised value for each pot.
    float norm[NUM_POTS];
    for (int i = 0; i < NUM_POTS; ++i) {
        norm[i] = midi_cc_active[i] ? midi_cc_val[i] : ctrl.pot_normalized[i];
    }

    // Map through per-mode, per-parameter curves and ranges.
    ParamSet ps;
    ps.time    = map_param(norm[0], get_param_range(mode, ParamId::Time));
    ps.repeats = map_param(norm[1], get_param_range(mode, ParamId::Repeats));
    ps.mix     = map_param(norm[2], get_param_range(mode, ParamId::Mix));
    ps.filter  = map_param(norm[3], get_param_range(mode, ParamId::Filter));
    ps.grit    = map_param(norm[4], get_param_range(mode, ParamId::Grit));
    ps.mod_spd = map_param(norm[5], get_param_range(mode, ParamId::ModSpd));
    ps.mod_dep = map_param(norm[6], get_param_range(mode, ParamId::ModDep));

    // Tap/MIDI tempo overrides the time pot when active.
    if (time_override > 0.0f) {
        // Clamp to the maximum supported delay length.
        ps.time = (time_override < 3.0f) ? time_override : 3.0f;
    }

    return ps;
}

/// Switch to a new mode, resetting its DSP state and informing the engine.
static void ActivateMode(pedal::DelayModeId new_id) {
    current_mode = new_id;
    pedal::DelayMode* m = mode_registry.get(new_id);
    m->Reset();
    audio_engine.SetMode(m);
}

// ── Entry point ───────────────────────────────────────────────────────────────

int main() {
    // ── Hardware init ─────────────────────────────────────────────────────────
    hw.Init();
    hw.SetAudioBlockSize(pedal::BLOCK_SIZE);
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);

    relay.Init(pedal::pins::RELAY,      GPIO::Mode::OUTPUT);
    led_bypass.Init(pedal::pins::LED_BYPASS, GPIO::Mode::OUTPUT);

    // ── Subsystem init ────────────────────────────────────────────────────────
    controls.Init(hw);
    mode_registry.Init();
    audio_engine.Init(&hw);
    midi_handler.Init();
    display.Init();
    tempo_sync.Init();

    // ── MIDI CC override initialisation ──────────────────────────────────────
    for (int i = 0; i < pedal::NUM_POTS; ++i) {
        midi_cc_val[i]    = 0.0f;
        midi_cc_active[i] = false;
    }

    // ── Initial bypass state: pedal starts Active (processing) ───────────────
    bypass_disp.Init(); // Init() sets state to Active
    audio_engine.SetBypass(false); // false = not bypassed = processing
    relay.Write(true);             // relay energised → signal routed through
    led_bypass.Write(true);        // LED on → pedal is active

    // ── Set initial mode ──────────────────────────────────────────────────────
    ActivateMode(current_mode);

    // ── Start audio interrupt ─────────────────────────────────────────────────
    hw.StartAudio(pedal::AudioEngine::AudioCallback);

    // ── Main loop ─────────────────────────────────────────────────────────────
    while (true) {
        const uint32_t now = System::GetNow();

        // -- Poll controls ----------------------------------------------------
        controls.Poll();
        const pedal::ControlState& ctrl = controls.state();

        // -- Encoder: cycle through delay modes --------------------------------
        if (ctrl.encoder_increment != 0) {
            int idx = static_cast<int>(current_mode) + ctrl.encoder_increment;
            if (idx < 0)                      idx = pedal::NUM_MODES - 1;
            if (idx >= pedal::NUM_MODES)       idx = 0;
            ActivateMode(static_cast<pedal::DelayModeId>(idx));
        }

        // -- Bypass footswitch: toggle on rising edge -------------------------
        if (ctrl.bypass_pressed) {
            bypass_disp.Toggle();
            const bool active = bypass_disp.IsActive();
            // audio_engine: true = bypassed (dry through), false = processing
            audio_engine.SetBypass(!active);
            relay.Write(active);      // relay on when pedal is active
            led_bypass.Write(active); // LED on when pedal is active
        }

        // -- Tap tempo footswitch ---------------------------------------------
        if (ctrl.tap_pressed) {
            tempo_sync.OnTap(now);
        }

        // -- Poll MIDI --------------------------------------------------------
        pedal::MidiState midi_state{};
        midi_handler.Poll(midi_state);

        if (midi_state.clock_tick) {
            tempo_sync.OnMidiClock(now);
        }
        if (midi_state.clock_stop) {
            tempo_sync.OnMidiStop();
        }

        // Program change: select mode by program number (0-9).
        if (midi_state.program_change >= 0 &&
            midi_state.program_change < pedal::NUM_MODES) {
            ActivateMode(
                static_cast<pedal::DelayModeId>(midi_state.program_change));
        }

        // Latch incoming CC values; once a CC arrives for a slot, the pot for
        // that slot is superseded until the next power cycle.
        // Use cc_received (not cc_normalized > 0) so a CC value of 0 also latches.
        for (int i = 0; i < pedal::NUM_POTS; ++i) {
            if (midi_state.cc_received[i]) {
                midi_cc_val[i]    = midi_state.cc_normalized[i];
                midi_cc_active[i] = true;
            }
        }

        // -- Tempo sync process (handles MIDI clock timeout) ------------------
        tempo_sync.Process(now);

        // -- Build and push parameters to audio engine ------------------------
        const float time_override = tempo_sync.GetOverrideSeconds();
        const pedal::ParamSet params = BuildParams(ctrl, current_mode, time_override);
        audio_engine.SetParams(params);

        // -- Update display at ~30 fps ----------------------------------------
        display.Update(current_mode, params, bypass_disp, tempo_sync, now);

        // Yield a millisecond so the idle loop does not starve the USB stack.
        System::Delay(1);
    }
}
