// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <string_view>

#include "dsp/midi_event.h"
#include "plugins/clap_plugin.h"
#include "plugins/plugin_configuration.h"
#include "plugins/plugin_descriptor.h"
#include "utils/audio.h"
#include "utils/object_registry.h"

#include "helpers/scoped_juce_qapplication.h"
#include "helpers/test_plugin_finder.h"

#include "unit/dsp/graph_helpers.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace zrythm::plugins
{
using namespace std::literals;

class ClapPluginTest
    : public ::testing::Test,
      public test_helpers::ScopedJuceQApplication,
      public testing::WithParamInterface<std::string_view>
{
protected:
  void SetUp () override
  {
    registry_ = std::make_unique<utils::ObjectRegistry> ();
    mock_transport_ = std::make_unique<dsp::graph_test::MockTransport> ();
    tempo_map_ = std::make_unique<dsp::TempoMap> (units::sample_rate (48000));
  }

  void TearDown () override
  {
    if (plugin_ != nullptr)
      {
        plugin_->release_resources ();
      }
    plugin_.reset ();
    registry_.reset ();
  }

  static PluginHostWindowFactory create_mock_window_provider ()
  {
    return [] (Plugin &) {
      class MockWindow : public IPluginHostWindow
      {
      public:
        void setJuceComponentContentNonOwned (juce::Component *) override { }
        void setSizeAndCenter (int, int) override { }
        void setSize (int, int) override { }
        void setVisible (bool) override { }
        WId  getEmbedWindowId () const override { return 0; }
      };
      return std::make_unique<MockWindow> ();
    };
  }

  void load_test_plugin (std::string_view name)
  {
    const auto juce_desc = test_helpers::find_test_clap_plugin_by_name (
      juce::String::fromUTF8 (name.data (), static_cast<int> (name.size ())));
    ASSERT_NE (juce_desc, nullptr)
      << "Test CLAP plugin '" << name << "' not found";

    auto config = std::make_unique<PluginConfiguration> ();
    config->descr_ = PluginDescriptor::from_juce_description (*juce_desc);
    ASSERT_NE (config->descr_, nullptr);

    plugin_ =
      std::make_unique<ClapPlugin> (*registry_, create_mock_window_provider ());
    // CLAP instantiation is synchronous
    plugin_->set_configuration (*config);
    ASSERT_FALSE (plugin_->get_output_ports ().empty ())
      << "Plugin failed to load";

    plugin_->prepare_for_processing (
      nullptr, units::sample_rate (48000), units::samples (256));
  }

  std::unique_ptr<utils::ObjectRegistry>          registry_;
  std::unique_ptr<dsp::graph_test::MockTransport> mock_transport_;
  std::unique_ptr<dsp::TempoMap>                  tempo_map_;
  std::unique_ptr<ClapPlugin>                     plugin_;
};

// A note-on sent to the plugin's MIDI input must produce audio on the
// plugin's audio outputs, regardless of the note dialect the plugin
// declares (CLAP notes or raw MIDI)
TEST_P (ClapPluginTest, NoteOnProducesAudio)
{
  load_test_plugin (GetParam ());

  dsp::MidiPort * midi_in = nullptr;
  for (const auto &port_ref : plugin_->get_input_ports ())
    {
      midi_in = port_ref.get_object_as<dsp::MidiPort> ();
      if (midi_in != nullptr)
        break;
    }
  ASSERT_NE (midi_in, nullptr);

  const auto note_on =
    dsp::midi_event::make_note_on (0, 60, 100, units::samples (0u));
  midi_in->buffer_.push_back (note_on.time_, note_on.data ());

  const dsp::graph::ProcessBlockInfo time_nfo{
    .transport_position_ = units::samples (0),
    .buffer_offset_ = units::samples (0),
    .nframes_ = units::samples (256),
  };
  for (int i = 0; i < 5; ++i)
    plugin_->process_block (time_nfo, *mock_transport_, *tempo_map_);

  bool has_audio = false;
  for (const auto &port_ref : plugin_->get_output_ports ())
    {
      if (auto * port = port_ref.get_object_as<dsp::AudioPort> ())
        {
          if (utils::audio::buffer_has_audio (*port->buffers (), 0, 256))
            {
              has_audio = true;
              break;
            }
        }
    }
  EXPECT_TRUE (has_audio)
    << "Plugin produced silent output for note-on (dialect mismatch?)";
}

INSTANTIATE_TEST_SUITE_P (
  NoteDialects,
  ClapPluginTest,
  testing::Values (
    // CLAP note dialect only
    "Test Synth"sv,
    // MIDI dialect only
    "Test Synth MIDI"sv));

} // namespace zrythm::plugins
