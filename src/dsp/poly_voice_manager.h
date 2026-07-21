// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <array>
#include <memory>
#include <vector>

#include "dsp/midi_event_buffer.h"
#include "dsp/synth_voice.h"
#include "utils/units.h"

namespace zrythm::dsp
{

/**
 * @brief Owns and drives a set of polyphonic synthesizer voices.
 *
 * Dispatches MIDI events (note on/off, pitch wheel) to voices with
 * sample-accurate timing and renders the active voices between events.
 * Voice allocation picks the first free voice, stealing the oldest active
 * voice when all are busy.
 *
 * Real-time safety: process() is non-blocking — it performs no allocations
 * or locking. Voices must be added and prepared before processing starts.
 *
 * TODO: handle sustain pedal (CC64) — defer note releases while the pedal
 * is down and flush them when it lifts (juce::Synthesiser, which this
 * replaces, handled it; currently sustained notes release immediately).
 */
class PolyVoiceManager
{
public:
  PolyVoiceManager ();
  /** Adds a voice (takes ownership). Not RT-safe; call before processing. */
  void add_voice (std::unique_ptr<SynthVoice> voice)
  {
    voices_.push_back (std::move (voice));
  }

  /** Removes all voices. Not RT-safe. */
  void clear_voices () { voices_.clear (); }

  /** Returns the voices (for per-voice setup such as control values). */
  std::span<const std::unique_ptr<SynthVoice>> voices () const
  {
    return voices_;
  }

  /** Releases all notes immediately (no tail-off). RT-safe. */
  void all_notes_off () noexcept [[clang::nonblocking]];

  /**
   * @brief Dispatches MIDI events and renders the block.
   *
   * Events outside [@p offset, @p offset + @p nframes) are ignored. Events
   * must be sorted by time. Active voices are rendered sample-accurately
   * between consecutive events.
   *
   * @param output Target buffer (voices add into it).
   * @param midi_events MIDI events for the current cycle (may be empty).
   * @param offset Sample offset of the block within the cycle.
   * @param nframes Number of samples to render.
   */
  void process (
    juce::AudioBuffer<float> &output,
    const MidiEventBuffer    &midi_events,
    units::sample_u32_t       offset,
    units::sample_u32_t       nframes) noexcept [[clang::nonblocking]];

private:
  void dispatch_event (std::span<const midi_byte_t> data) noexcept
    [[clang::nonblocking]];
  void note_on (int channel, int pitch, float velocity) noexcept
    [[clang::nonblocking]];
  void note_off (int channel, int pitch) noexcept [[clang::nonblocking]];
  void render_active (
    juce::AudioBuffer<float> &output,
    int                       start_sample,
    int                       num_samples) noexcept [[clang::nonblocking]];

private:
  std::vector<std::unique_ptr<SynthVoice>> voices_;

  /** Last pitch bend value per MIDI channel (for notes started later). */
  std::array<int, 16> last_pitch_bend_;

  /** Monotonic counter for voice age (stealing). */
  std::uint32_t next_note_sequence_ = 1;
};

} // namespace zrythm::dsp
