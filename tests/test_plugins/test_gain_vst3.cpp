// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <atomic>

#include "base/source/fstreamer.h"
#include "gain_dsp.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "public.sdk/source/main/pluginfactory.h"
#include "public.sdk/source/vst/vstsinglecomponenteffect.h"

namespace zrythm_test_plugins
{

using namespace Steinberg;
using namespace Steinberg::Vst;

static const FUID TestGainUID (0x8A3F2E10, 0x4C5D6E7F, 0x9A0B1C2D, 0x3E4F5A6B);

class TestGain : public SingleComponentEffect
{
public:
  static constexpr ParamID kLevelParamId = 0;

  tresult PLUGIN_API initialize (FUnknown * context) SMTG_OVERRIDE
  {
    const auto res = SingleComponentEffect::initialize (context);
    if (res != kResultOk)
      return res;

    addAudioInput (STR16 ("Input"), SpeakerArr::kStereo);
    addAudioOutput (STR16 ("Output"), SpeakerArr::kStereo);
    parameters.addParameter (
      STR16 ("Level"), STR16 (""), 0, 1.0, ParameterInfo::kCanAutomate,
      kLevelParamId);
    return kResultOk;
  }

  tresult PLUGIN_API setBusArrangements (
    SpeakerArrangement * inputs,
    int32                numIns,
    SpeakerArrangement * outputs,
    int32                numOuts) SMTG_OVERRIDE
  {
    if (
      numIns == 1 && numOuts == 1 && inputs[0] == SpeakerArr::kStereo
      && outputs[0] == SpeakerArr::kStereo)
      return kResultOk;
    return kResultFalse;
  }

  tresult PLUGIN_API
  canProcessSampleSize (int32 symbolicSampleSize) SMTG_OVERRIDE
  {
    return symbolicSampleSize == kSample32 ? kResultTrue : kResultFalse;
  }

  tresult PLUGIN_API
  setParamNormalized (ParamID tag, ParamValue value) SMTG_OVERRIDE
  {
    const auto res = EditControllerEx1::setParamNormalized (tag, value);
    if (res == kResultOk && tag == kLevelParamId)
      gain_.store (value);
    return res;
  }

  tresult PLUGIN_API setEditorState (IBStream * state) SMTG_OVERRIDE
  {
    IBStreamer streamer (state, kLittleEndian);
    double     saved_gain = 0.0;
    if (!streamer.readDouble (saved_gain))
      return kResultFalse;
    setParamNormalized (kLevelParamId, saved_gain);
    return kResultOk;
  }

  tresult PLUGIN_API getEditorState (IBStream * state) SMTG_OVERRIDE
  {
    IBStreamer streamer (state, kLittleEndian);
    streamer.writeDouble (gain_.load ());
    return kResultOk;
  }

  tresult PLUGIN_API process (ProcessData &data) SMTG_OVERRIDE
  {
    if (data.inputParameterChanges != nullptr)
      {
        const auto num_changes =
          data.inputParameterChanges->getParameterCount ();
        for (int32 i = 0; i < num_changes; ++i)
          {
            auto * queue = data.inputParameterChanges->getParameterData (i);
            if (queue == nullptr || queue->getParameterId () != kLevelParamId)
              continue;
            const auto num_points = queue->getPointCount ();
            if (num_points > 0)
              {
                int32      offset = 0;
                ParamValue value = 0.0;
                if (queue->getPoint (num_points - 1, offset, value) == kResultOk)
                  gain_.store (value);
              }
          }
      }

    if (data.numSamples <= 0 || data.numInputs < 1 || data.numOutputs < 1)
      return kResultOk;

    const auto num_channels =
      std::min (data.inputs[0].numChannels, data.outputs[0].numChannels);
    for (int32 ch = 0; ch < num_channels; ++ch)
      {
        const auto * in = data.inputs[0].channelBuffers32[ch];
        auto *       out = data.outputs[0].channelBuffers32[ch];
        if ((data.inputs[0].silenceFlags & (uint64{ 1 } << ch)) != 0)
          {
            std::fill_n (out, data.numSamples, 0.0f);
            data.outputs[0].silenceFlags |= (uint64{ 1 } << ch);
            continue;
          }
        apply_gain (
          in, out, static_cast<uint32_t> (data.numSamples), gain_.load ());
      }
    return kResultOk;
  }

  static FUnknown * createInstance (void *)
  {
    return static_cast<IAudioProcessor *> (new TestGain ());
  }

private:
  std::atomic<double> gain_{ 1.0 };
};

} // namespace zrythm_test_plugins

using zrythm_test_plugins::TestGain;
using zrythm_test_plugins::TestGainUID;

BEGIN_FACTORY_DEF ("Zrythm", "https://zrythm.org", "mailto:contact@zrythm.org")
DEF_CLASS2 (
  INLINE_UID_FROM_FUID (TestGainUID),
  PClassInfo::kManyInstances,
  kVstAudioEffectClass,
  "Test Gain",
  0,
  Vst::PlugType::kFx,
  "1.0.0",
  kVstVersionString,
  TestGain::createInstance)
END_FACTORY
