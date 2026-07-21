// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <ranges>

#include "dsp/poly_voice_manager.h"
#include "utils/midi.h"

namespace zrythm::dsp
{

PolyVoiceManager::PolyVoiceManager ()
{
  last_pitch_bend_.fill (SynthVoice::kPitchBendCenter);
}

void
PolyVoiceManager::all_notes_off () noexcept
{
  for (auto &voice : voices_)
    voice->cut ();
}

void
PolyVoiceManager::note_on (int channel, int pitch, float velocity) noexcept
{
  // Find a free voice, or steal the oldest active one
  auto * voice = [&] () -> SynthVoice * {
    for (auto &v : voices_)
      {
        if (!v->is_active ())
          return v.get ();
      }
    const auto oldest = std::ranges::min_element (
      voices_, {}, [] (const auto &v) { return v->note_sequence (); });
    return oldest != voices_.end () ? oldest->get () : nullptr;
  }();
  if (voice == nullptr)
    return;

  if (voice->is_active ())
    voice->cut ();

  voice->note_on (channel, pitch, velocity, next_note_sequence_++);
  // Apply the channel's current bend so bent notes start bent
  voice->pitch_bend (last_pitch_bend_[channel]);
}

void
PolyVoiceManager::note_off (int channel, int pitch) noexcept
{
  for (auto &voice : voices_)
    {
      if (
        voice->is_active () && voice->current_note () == pitch
        && voice->current_channel () == channel)
        {
          voice->note_off ();
        }
    }
}

void
PolyVoiceManager::dispatch_event (std::span<const midi_byte_t> data) noexcept
{
  // Validate size against the status byte: this lets through only well-formed
  // messages of the relevant types (note on/off/pitch wheel are 3 bytes each)
  // and silently skips shorter messages that don't apply to voice allocation
  // (program change, channel pressure, active sensing, etc.). Future CC
  // support would slot in as another else-if branch with the same length
  // check.
  if (data.empty ())
    return;
  const auto expected_len = utils::midi::midi_get_msg_length (data[0]);
  if (data.size () < static_cast<size_t> (expected_len))
    return;

  const int channel =
    static_cast<int> (utils::midi::midi_get_channel_0_to_15 (data));
  if (utils::midi::midi_is_note_on (data))
    {
      note_on (
        channel, utils::midi::midi_get_note_number (data),
        static_cast<float> (utils::midi::midi_get_velocity (data)) / 127.f);
    }
  else if (utils::midi::midi_is_note_off (data))
    {
      note_off (channel, utils::midi::midi_get_note_number (data));
    }
  else if (utils::midi::midi_is_pitch_wheel (data))
    {
      const int value =
        static_cast<int> (utils::midi::midi_get_14_bit_value (data));
      last_pitch_bend_[channel] = value;
      for (auto &voice : voices_)
        voice->pitch_bend (value);
    }
}

void
PolyVoiceManager::render_active (
  juce::AudioBuffer<float> &output,
  int                       start_sample,
  int                       num_samples) noexcept
{
  if (num_samples <= 0)
    return;
  for (auto &voice : voices_)
    {
      if (voice->is_active ())
        voice->render (output, start_sample, num_samples);
    }
}

void
PolyVoiceManager::process (
  juce::AudioBuffer<float> &output,
  const MidiEventBuffer    &midi_events,
  units::sample_u32_t       offset,
  units::sample_u32_t       nframes) noexcept
{
  const int block_start = offset.in<int> (units::samples);
  const int block_end = block_start + nframes.in<int> (units::samples);

  int current = block_start;
  for (const auto &ev : midi_events)
    {
      const int ev_pos = ev.time ().in<int> (units::samples);
      if (ev_pos < block_start || ev_pos >= block_end)
        continue;
      render_active (output, current, ev_pos - current);
      current = ev_pos;
      dispatch_event (ev.data ());
    }
  render_active (output, current, block_end - current);
}

} // namespace zrythm::dsp
