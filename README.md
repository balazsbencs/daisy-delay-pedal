# Daisy Delay Pedal

A multi-mode digital delay guitar pedal firmware for the [Electrosmith Daisy Seed](https://electro-smith.com/products/daisy-seed) (ARM Cortex-M7 @ 480 MHz, 64 MB SDRAM, AKM 24-bit codec).

## Features

- **10 delay modes** selectable via rotary encoder
- **7 dedicated knobs** for real-time parameter control
- **SSD1306 OLED display** showing mode and parameter state at 30 fps
- **Tap tempo** footswitch with BPM averaging
- **MIDI control** via USB MIDI and TRS MIDI Type A
  - CC mapping for all 7 parameters
  - Program Change for mode selection
  - MIDI Clock sync for delay time
  - MIDI Learn (hold encoder + twist)
- **True bypass relay** with LED indicator
- **Mono in / Stereo out**, 48 kHz / 24-bit

---

## Delay Modes

| # | Name | Character |
|---|------|-----------|
| 0 | **Duck** | Wet signal ducks when input is loud, swells in silence |
| 1 | **Swell** | AD envelope on repeats creates pad-like volume swells |
| 2 | **Trem** | LFO amplitude modulates the wet signal (tremolo on repeats) |
| 3 | **Digital** | Clean, pristine repeats with tone control |
| 4 | **DBucket** | BBD emulation: HF rolloff + clock noise per repeat |
| 5 | **Tape** | Wow/flutter modulation, HF rolloff, soft saturation |
| 6 | **Dual** | Two independent delay lines panned L/R |
| 7 | **Pattern** | Multi-tap rhythmic patterns (straight / dotted / triplet) |
| 8 | **Filter** | LFO-modulated resonant SVF filter sweeps on repeats |
| 9 | **Lo-Fi** | Progressive bit-crush + sample rate reduction per repeat |

---

## Hardware

### Daisy Seed Pin Assignments

| Function | Pin | Notes |
|----------|-----|-------|
| Time pot | A0 (D15) | ADC channel 0 |
| Repeats pot | A1 (D16) | ADC channel 1 |
| Mix pot | A2 (D17) | ADC channel 2 |
| Filter pot | A3 (D18) | ADC channel 3 |
| Grit pot | A4 (D19) | ADC channel 4 |
| Mod Speed pot | A5 (D20) | ADC channel 5 |
| Mod Depth pot | A6 (D21) | ADC channel 6 |
| Encoder A | D0 | Quadrature input |
| Encoder B | D1 | Quadrature input |
| Encoder button | D2 | Mode select / MIDI learn |
| Bypass footswitch | D3 | Toggle effect in/out |
| Tap tempo footswitch | D4 | Tap BPM input |
| Relay control | D5 | True bypass relay |
| Bypass LED | D6 | Active indicator |
| OLED SCL | D11 | I2C1 clock |
| OLED SDA | D12 | I2C1 data |
| TRS MIDI TX | D13 | UART TX |
| TRS MIDI RX | D14 | UART RX |

### Knob Reference

| Knob | Parameter | Range |
|------|-----------|-------|
| Time | Delay time | 10 ms – 3 s (log curve) |
| Repeats | Feedback | 0 – 98% |
| Mix | Wet/dry blend | 0 – 100% |
| Filter | Tone | LP at 0 → Flat at 50% → HP at 100% |
| Grit | Mode-specific dirt/character | 0 – 100% |
| Mod Speed | Modulation rate | 0.05 – 10 Hz |
| Mod Depth | Modulation amount | 0 – 100% |

---

## MIDI

### Default CC Map

| CC | Parameter |
|----|-----------|
| 14 | Time |
| 15 | Repeats |
| 16 | Mix |
| 17 | Filter |
| 18 | Grit |
| 19 | Mod Speed |
| 20 | Mod Depth |

CC values 0–127 map to the full parameter range for that knob.

### Program Change

Program Change 0–9 selects the corresponding delay mode.

### MIDI Clock

When MIDI Clock is received, the delay time is overridden and locked to the beat tempo. The OLED displays `MIDI` as the tempo source. Clock lock expires 2 seconds after the last clock tick.

### MIDI Learn

1. Hold the encoder button and twist the encoder — the pedal enters MIDI Learn mode for the currently highlighted parameter.
2. Send any CC message from your controller.
3. That CC is now mapped to that parameter.

Mappings reset on power cycle (non-volatile storage not yet implemented).

---

## Building

### Requirements

- `arm-none-eabi-gcc` toolchain (≥ 13.x recommended)
  - macOS: `brew install --cask gcc-arm-embedded`
  - Linux: `sudo apt install gcc-arm-none-eabi`
- GNU Make

### First-time Setup

```bash
# Clone the repo (if not already done)
git clone <repo-url> delay
cd delay

# Initialize all submodules (libDaisy + its nested deps + DaisySP)
git submodule update --init --recursive

# Build libDaisy
make -C third_party/libDaisy -j4

# Build DaisySP
make -C third_party/DaisySP -j4
```

### Building the Firmware

```bash
make -j4
```

Output files are in `build/`:
- `delay.elf` — ELF with debug symbols
- `delay.bin` — raw binary for flashing
- `delay.hex` — Intel HEX format

### Flashing

**Via DFU (USB):**

Put the Daisy Seed into DFU mode (hold BOOT, tap RESET, release BOOT), then:

```bash
make program-dfu
```

**Via ST-Link:**

```bash
make program
```

---

## Project Structure

```
delay/
├── Makefile
├── src/
│   ├── main.cpp                  # Entry point, main loop
│   ├── config/
│   │   ├── pin_map.h             # GPIO/ADC pin constants
│   │   ├── constants.h           # Sample rate, buffer sizes, MIDI CCs
│   │   └── delay_mode_id.h       # enum class DelayModeId
│   ├── hardware/
│   │   ├── controls.h/.cpp       # Pot smoothing, encoder, switches
│   ├── audio/
│   │   ├── stereo_frame.h        # POD StereoFrame type
│   │   ├── audio_engine.h/.cpp   # Audio callback, double-buffered params
│   │   └── bypass.h/.cpp         # Bypass state machine
│   ├── params/
│   │   ├── param_id.h            # enum class ParamId
│   │   ├── param_range.h         # ParamRange + curve mapping
│   │   ├── param_set.h/.cpp      # Immutable parameter snapshot
│   │   └── param_map.h/.cpp      # Per-mode parameter ranges
│   ├── dsp/
│   │   ├── delay_line_sdram.h/.cpp   # SDRAM-backed delay line
│   │   ├── dc_blocker.h              # One-pole DC blocker
│   │   ├── envelope_follower.h/.cpp  # Attack/release envelope
│   │   ├── lfo.h/.cpp                # Sine/triangle LFO
│   │   ├── tone_filter.h/.cpp        # One-knob LP/HP filter
│   │   └── saturation.h              # Soft-clip saturation
│   ├── modes/
│   │   ├── delay_mode.h          # Abstract base class
│   │   ├── mode_registry.h/.cpp  # Owns all 10 mode instances
│   │   ├── digital_delay.h/.cpp
│   │   ├── duck_delay.h/.cpp
│   │   ├── swell_delay.h/.cpp
│   │   ├── trem_delay.h/.cpp
│   │   ├── dbucket_delay.h/.cpp
│   │   ├── tape_delay.h/.cpp
│   │   ├── dual_delay.h/.cpp
│   │   ├── pattern_delay.h/.cpp
│   │   ├── filter_delay.h/.cpp
│   │   └── lofi_delay.h/.cpp
│   ├── midi/
│   │   └── midi_handler.h/.cpp   # USB + TRS MIDI, CC routing, learn
│   ├── display/
│   │   ├── display_layout.h      # Screen layout constants
│   │   └── display_manager.h/.cpp # OLED rendering at 30 fps
│   └── tempo/
│       ├── tap_tempo.h/.cpp      # Tap detection + BPM averaging
│       └── tempo_sync.h/.cpp     # MIDI Clock > Tap > Pot priority
└── third_party/
    ├── libDaisy/                 # git submodule
    └── DaisySP/                  # git submodule
```

---

## Architecture Notes

### Parameter Flow

```
Pots (ADC) ──┐
Encoder ─────┼──► Controls::Poll() ──► ParamSet (immutable snapshot)
MIDI CC ─────┘                               │
                                             ▼
Tap/MIDI Clock ──► TempoSync ──► time override
                                             │
                                             ▼
                                    AudioEngine::SetParams()
                                             │
                                    (double-buffer swap)
                                             │
                                             ▼
                                    AudioCallback (ISR)
                                             │
                                    DelayMode::Process()
                                             │
                                    ┌────────┴────────┐
                                    ▼                 ▼
                                 Codec L out       Codec R out
```

### Key Design Decisions

1. **Immutable `ParamSet`** — A fresh snapshot is taken each main loop iteration. The audio ISR never touches ADC values directly, preventing data races.

2. **Static SDRAM allocation** — All delay buffers (`~6.3 MB total`) are declared with `DSY_SDRAM_BSS` at file scope. No heap, no `malloc`. The linker places them in SDRAM automatically.

3. **All 10 modes pre-constructed** — Mode switching is a pointer swap + `Reset()`, not allocation. Zero audio glitches on mode change beyond the reset clearing the buffer.

4. **Double-buffered parameters** — Main loop writes to the idle buffer and sets a dirty flag; the ISR swaps the read index at block entry. No mutex needed on Cortex-M7 for aligned word stores.

5. **Tempo priority** — `TempoSync` implements a priority chain: MIDI Clock (highest) → Tap Tempo → Pot. MIDI Clock lock expires after 2 s of silence.

### SDRAM Memory Budget

| Buffer | Size | Mode |
|--------|------|------|
| Primary delay (3 s) | 576 KB | Most modes |
| Dual L (3 s) | 576 KB | Dual mode |
| Dual R (3 s) | 576 KB | Dual mode |
| Pattern (3 s) | 576 KB | Pattern mode |
| Filter (3 s) | 576 KB | Filter mode |
| Lo-Fi (3 s) | 576 KB | Lo-Fi mode |
| + others | ~3.3 MB | Remaining modes |
| **Total** | **~6.3 MB** | < 10% of 64 MB SDRAM |

---

## Flash Usage

As of the initial build:

```
FLASH:   92.88%  (121 KB / 128 KB)
SRAM:    20.10%  (105 KB / 512 KB)
SDRAM:    9.44%  (6.3 MB / 64 MB)
```

Flash is tight. If adding features, consider:
- Enabling link-time optimization (`OPT = -O2 -flto` in Makefile)
- Moving string literals to flash with `const char* __attribute__((section(".rodata")))`
- Booting from QSPI flash (8 MB available) for much more room
