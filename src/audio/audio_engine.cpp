#include "audio_engine.h"

using namespace daisy;

namespace pedal {

// ── Singleton ────────────────────────────────────────────────────────────────
AudioEngine* AudioEngine::instance_ = nullptr;

// ── Init ─────────────────────────────────────────────────────────────────────
void AudioEngine::Init(DaisySeed* hw) {
    hw_              = hw;
    mode_            = nullptr;
    bypassed_        = true;
    new_bypass_      = true;
    param_read_idx_  = 0;
    param_dirty_     = false;
    bypass_dirty_    = false;
    param_buf_[0]    = ParamSet::make_default();
    param_buf_[1]    = ParamSet::make_default();
    instance_        = this;
}

// ── Main-loop API ─────────────────────────────────────────────────────────────

void AudioEngine::SetParams(const ParamSet& params) {
    // Write to the buffer the ISR is NOT currently reading.
    const int write_idx    = 1 - param_read_idx_;
    param_buf_[write_idx]  = params;
    // Publish: the ISR will flip param_read_idx_ on the next block entry.
    param_dirty_ = true;
}

void AudioEngine::SetMode(DelayMode* mode) {
    // The mode pointer is written as a single aligned word on Cortex-M, which
    // is inherently atomic.  The ISR will pick up the new pointer at the start
    // of the next block's loop iteration.
    mode_ = mode;
}

void AudioEngine::SetBypass(bool bypassed) {
    new_bypass_   = bypassed;
    bypass_dirty_ = true;
}

// ── ISR trampoline ────────────────────────────────────────────────────────────

void AudioEngine::AudioCallback(AudioHandle::InputBuffer  in,
                                AudioHandle::OutputBuffer out,
                                size_t                    size) {
    if (instance_ != nullptr) {
        instance_->ProcessBlock(in, out, size);
    }
}

// ── ProcessBlock (runs in ISR) ────────────────────────────────────────────────

void AudioEngine::ProcessBlock(AudioHandle::InputBuffer  in,
                               AudioHandle::OutputBuffer out,
                               size_t                    size) {
    // Consume pending parameter update: flip the read index so the next
    // iteration uses the freshly written buffer.
    if (param_dirty_) {
        param_read_idx_ ^= 1;
        param_dirty_     = false;
    }

    // Consume pending bypass change.
    if (bypass_dirty_) {
        bypassed_     = new_bypass_;
        bypass_dirty_ = false;
    }

    const ParamSet& params = param_buf_[param_read_idx_];

    for (size_t i = 0; i < size; ++i) {
        const float dry = IN_L[i]; // pedal is mono-input

        if (bypassed_ || mode_ == nullptr) {
            // True bypass: route input directly to both outputs.
            OUT_L[i] = dry;
            OUT_R[i] = dry;
        } else {
            // Wet-only result from the mode; mix is applied here so modes
            // never need to know about the dry path.
            const StereoFrame wet = mode_->Process(dry, params);

            // Constant-power-style mix: dry fades as wet rises.
            const float dry_gain = 1.0f - params.mix;
            OUT_L[i] = dry * dry_gain + wet.left  * params.mix;
            OUT_R[i] = dry * dry_gain + wet.right * params.mix;
        }
    }
}

} // namespace pedal
