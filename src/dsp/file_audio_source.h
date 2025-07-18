// SPDX-FileCopyrightText: © 2019-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "utils/audio.h"
#include "utils/audio_file.h"
#include "utils/icloneable.h"
#include "utils/monotonic_time_provider.h"
#include "utils/types.h"
#include "utils/uuid_identifiable_object.h"

namespace zrythm::dsp
{

/**
 * Audio clips for the pool.
 *
 * These should be loaded in the project's sample rate.
 */
class FileAudioSource final
    : public QObject,
      public utils::UuidIdentifiableObject<FileAudioSource>
{
  Q_OBJECT

public:
  using BitDepth = zrythm::utils::audio::BitDepth;
  using AudioFile = zrythm::utils::audio::AudioFile;

public:
  FileAudioSource (QObject * parent = nullptr) : QObject (parent) { }

  /**
   * Creates an audio clip from a file.
   *
   * The basename of the file will be used as the name of the clip.
   *
   * @param current_bpm Current BPM from TempoTrack. @ref bpm_ will be set to
   * this. FIXME: should this be optional? does "current" BPM make sense?
   *
   * @throw ZrythmException on error.
   */
  FileAudioSource (
    const fs::path &full_path,
    sample_rate_t   project_sample_rate,
    bpm_t           current_bpm,
    QObject *       parent = nullptr);

  /**
   * Creates an audio clip by copying the given buffer.
   *
   * @param buf Buffer to copy.
   * @param name A name for this clip.
   */
  FileAudioSource (
    const utils::audio::AudioBuffer &buf,
    utils::audio::BitDepth           bit_depth,
    sample_rate_t                    project_sample_rate,
    bpm_t                            current_bpm,
    const utils::Utf8String         &name,
    QObject *                        parent = nullptr);

  /**
   * Creates an audio clip by copying the given interleaved float array.
   *
   * @param arr Interleaved array.
   * @param nframes Number of frames per channel.
   * @param channels Number of channels.
   * @param name A name for this clip.
   */
  FileAudioSource (
    const float *                  arr,
    unsigned_frame_t               nframes,
    channels_t                     channels,
    zrythm::utils::audio::BitDepth bit_depth,
    sample_rate_t                  project_sample_rate,
    bpm_t                          current_bpm,
    const utils::Utf8String       &name,
    QObject *                      parent = nullptr)
      : FileAudioSource (
          *utils::audio::AudioBuffer::from_interleaved (arr, nframes, channels),
          bit_depth,
          project_sample_rate,
          current_bpm,
          name,
          parent)
  {
  }

  /**
   * Create an audio clip while recording.
   *
   * The frames will keep getting reallocated until the recording is
   * finished.
   *
   * @param nframes Number of frames to allocate. This should be the
   * current cycle's frames when called during recording.
   */
  FileAudioSource (
    channels_t               channels,
    unsigned_frame_t         nframes,
    sample_rate_t            project_sample_rate,
    bpm_t                    current_bpm,
    const utils::Utf8String &name,
    QObject *                parent = nullptr);

  // ========================================================================
  // QML Interface
  // ========================================================================

  /**
   * @brief Emitted when the source samples change.
   */
  Q_SIGNAL void samplesChanged ();

  // ========================================================================

  auto        get_bit_depth () const { return bit_depth_; }
  auto        get_name () const { return name_; }
  auto        get_bpm () const { return bpm_; }
  const auto &get_samples () const { return ch_frames_; }
  auto        get_samplerate () const { return samplerate_; }

  void set_name (const utils::Utf8String &name) { name_ = name; }

  /**
   * @brief Expands (appends to the end) the frames in the clip by the given
   * frames.
   *
   * @param frames Non-interleaved frames.
   */
  void expand_with_frames (const utils::audio::AudioBuffer &frames);

  /**
   * Replaces the clip's frames starting from @p start_frame with @p frames.
   *
   * @warning Not realtime safe.
   *
   * @param src_frames Frames to copy.
   * @param start_frame Frame to start copying to (@p src_frames are always
   * copied from the start).
   */
  void replace_frames (
    const utils::audio::AudioBuffer &src_frames,
    unsigned_frame_t                 start_frame);

  /**
   * Replaces the clip's frames starting from @p start_frame with @p frames.
   *
   * @warning Not realtime safe.
   *
   * @param frames Frames, interleaved.
   * @param start_frame Frame to start copying to (@p src_frames are always
   * copied from the start).
   */
  void replace_frames_from_interleaved (
    const float *    frames,
    unsigned_frame_t start_frame,
    unsigned_frame_t num_frames_per_channel,
    channels_t       channels);

  /**
   * @brief Unloads the clip's frames from memory.
   */
  void clear_frames ()
  {
    ch_frames_.setSize (ch_frames_.getNumChannels (), 0, false, true);
    Q_EMIT samplesChanged ();
  }

  auto get_num_channels () const { return ch_frames_.getNumChannels (); };
  auto get_num_frames () const { return ch_frames_.getNumSamples (); };

  /**
   * @brief Initializes members from an audio file.
   *
   * @param full_path Path to the file.
   * @param bpm_to_set BPM of the clip to set (File BPM or 0 will be used if
   * nullopt).
   *
   * @throw ZrythmException on I/O error.
   */
  void init_from_file (
    const fs::path      &full_path,
    sample_rate_t        project_sample_rate,
    std::optional<bpm_t> bpm_to_set);

private:
  friend void init_from (
    FileAudioSource       &obj,
    const FileAudioSource &other,
    utils::ObjectCloneType clone_type);

  static constexpr auto kNameKey = "name"sv;
  static constexpr auto kBpmKey = "bpm"sv;
  static constexpr auto kBitDepthKey = "bitDepth"sv;
  static constexpr auto kSamplerateKey = "samplerate"sv;
  friend void           to_json (nlohmann::json &j, const FileAudioSource &clip)
  {
    to_json (j, static_cast<const UuidIdentifiableObject &> (clip));
    j[kNameKey] = clip.name_;
    j[kBpmKey] = clip.bpm_;
    j[kBitDepthKey] = clip.bit_depth_;
    j[kSamplerateKey] = clip.samplerate_;
  }
  friend void from_json (const nlohmann::json &j, FileAudioSource &clip)
  {
    from_json (j, static_cast<UuidIdentifiableObject &> (clip));
    j.at (kNameKey).get_to (clip.name_);
    j.at (kBpmKey).get_to (clip.bpm_);
    j.at (kBitDepthKey).get_to (clip.bit_depth_);
    j.at (kSamplerateKey).get_to (clip.samplerate_);
  }

private:
  /** Name of the clip. */
  utils::Utf8String name_;

  /**
   * Per-channel frames.
   */
  utils::audio::AudioBuffer ch_frames_;

  /**
   * BPM of the clip, or BPM of the project when the clip was first loaded.
   */
  bpm_t bpm_{};

  /**
   * Samplerate of the clip, or samplerate when the clip was imported into the
   * project.
   */
  sample_rate_t samplerate_{};

  /**
   * Bit depth of the clip when the clip was imported into the project.
   */
  utils::audio::BitDepth bit_depth_{};
};

// ========================================================================

/**
 * Handles all file I/O operations for FileAudioSource
 */
class FileAudioSourceWriter : private utils::QElapsedTimeProvider
{
public:
  /**
   * Writes the given audio clip data to a file.
   *
   * @param parts If true, only write new data. @see
   * FileAudioSource.frames_written.
   *
   * @throw ZrythmException on error.
   */
  explicit FileAudioSourceWriter (
    const FileAudioSource &source,
    fs::path               path,
    bool                   parts);

  /**
   * @brief Write the file either in parts or whole (depending on constructor
   * param).
   *
   * If parts, this should be called periodically while the source is expanding
   * in memory to write any unwritten data to the file.
   */
  void write_to_file ();

  /**
   * @brief To be called after the file has been written via
   * write_file_buffered().
   */
  void finalize_buffered_write ();

  /**
   * @brief Returns whether enough time has elapsed since the last write to
   * file. This is used so that writing to file is done in chunks.
   */
  bool enough_time_elapsed_since_last_write () const;

private:
  /**
   * @brief Whether we are writing in parts.
   */
  bool parts_;

  const FileAudioSource                   &source_;
  std::unique_ptr<juce::AudioFormatWriter> writer_;
  fs::path                                 writer_path_;

  /**
   * Frames already written to the file, per channel.
   *
   * Used when writing in chunks/parts.
   */
  unsigned_frame_t frames_written_{};

  /**
   * Time the last write took place.
   *
   * This is used so that we can write every x ms instead of all the time.
   *
   * @see FileAudioSource.frames_written.
   */
  utils::MonotonicTime last_write_{};
};

using FileAudioSourcePtrVariant = std::variant<FileAudioSource *>;
using FileAudioSourceRegistry =
  utils::OwningObjectRegistry<FileAudioSourcePtrVariant, FileAudioSource>;
using FileAudioSourceUuidReference =
  utils::UuidReference<FileAudioSourceRegistry>;

} // namespace zrythm::dsp

DEFINE_UUID_HASH_SPECIALIZATION (zrythm::dsp::FileAudioSource::Uuid)
