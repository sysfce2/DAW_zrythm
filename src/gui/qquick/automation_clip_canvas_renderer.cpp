// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <cmath>
#include <ranges>
#include <utility>

#include "dsp/tick_types.h"
#include "gui/qquick/automation_clip_canvas_item.h"
#include "gui/qquick/automation_clip_canvas_renderer.h"
#include "structure/arrangement/arranger_object_all.h"
#include "structure/arrangement/clip.h"
#include "structure/arrangement/loop_segment_iterator.h"

namespace zrythm::gui::qquick
{

namespace
{

/// Evaluates the automation value at a virtual content-tick position and
/// returns the value along with the AutomationPoint whose curve segment
/// drives that position (nullptr if before the first point).
auto
eval_at_virt_tick (const auto &sorted_points, dsp::ContentTick virt_tick)
  -> std::pair<float, const structure::arrangement::AutomationPoint *>
{
  using AP = structure::arrangement::AutomationPoint;

  if (sorted_points.empty ())
    return { 0.0f, nullptr };

  const auto tick_of = [] (const AP * ap) {
    return ap->position ()->asTick ();
  };

  const auto first_tick = tick_of (sorted_points.front ());
  const auto last_tick = tick_of (sorted_points.back ());

  if (virt_tick <= first_tick)
    return { sorted_points.front ()->value (), sorted_points.front () };
  if (virt_tick >= last_tick)
    return { sorted_points.back ()->value (), sorted_points.back () };

  auto it = std::ranges::lower_bound (sorted_points, virt_tick, {}, tick_of);
  if (it == sorted_points.begin () || it == sorted_points.end ())
    return { sorted_points.back ()->value (), sorted_points.back () };

  const auto * prev_ap = *std::ranges::prev (it);
  const auto * next_ap = *it;

  const auto prev_tick = tick_of (prev_ap);
  const auto next_tick = tick_of (next_ap);
  if (next_tick <= prev_tick)
    return { prev_ap->value (), prev_ap };

  double t = (virt_tick - prev_tick) / (next_tick - prev_tick);
  t = std::clamp (t, 0.0, 1.0);

  const float val = dsp::evaluate_curve (
    prev_ap->value (), next_ap->value (), prev_ap->curveOpts ()->algorithm (),
    static_cast<float> (prev_ap->curveOpts ()->curviness ()), t);
  return { val, prev_ap };
}

} // anonymous namespace

void
AutomationClipCanvasRenderer::synchronize (QCanvasPainterItem * item)
{
  auto * canvas_item = static_cast<AutomationClipCanvasItem *> (item);

  curve_color_ = canvas_item->curveColor ();
  canvas_width_ = static_cast<float> (canvas_item->width ());
  canvas_height_ = static_cast<float> (canvas_item->height ());
  reference_width_ = canvas_item->effectiveReferenceWidth ();
  reference_x_ = canvas_item->referenceX ();

  points_.clear ();

  auto * clip = canvas_item->automationClip ();
  if (clip == nullptr || canvas_width_ <= 0.0f || canvas_height_ <= 0.0f)
    return;

  const auto sorted_points = clip->structure::arrangement::ArrangerObjectOwner<
    structure::arrangement::AutomationPoint>::get_sorted_children_view ();
  if (sorted_points.empty ())
    return;

  if (clip->length () == nullptr)
    return;
  const auto clip_ticks = clip->length ()->asTick ();
  if (clip_ticks <= dsp::ContentTick{})
    return;

  const auto loop_start_ticks = clip->loopStartPosition ()->asTick ();
  const auto loop_end_ticks = clip->loopEndPosition ()->asTick ();
  const auto clip_start_ticks = clip->clipStartPosition ()->asTick ();

  const double clip_ticks_d = clip_ticks.asDouble ();
  const double px_per_tick =
    static_cast<double> (reference_width_) / clip_ticks_d;
  // Extend beyond clip_ticks for looped clips or during loop-resize of a
  // non-looped clip (loopPreview).
  const auto display_end_tick =
    (clip->looped () || canvas_item->loopPreview ())
      ? max (
          clip_ticks,
          dsp::ContentTick{ units::ticks (
            (static_cast<double> (canvas_width_) + reference_x_) / px_per_tick) })
      : clip_ticks;
  const double display_end_ticks_d = display_end_tick.asDouble ();

  const auto add_point =
    [&] (
      double abs_tick_d, float val,
      const structure::arrangement::AutomationPoint * source_ap) {
      if (abs_tick_d < 0.0 || abs_tick_d > display_end_ticks_d)
        return;

      CachedControlPoint cp;
      cp.px = static_cast<float> (abs_tick_d * px_per_tick - reference_x_);
      cp.val = val;
      cp.py = canvas_height_ * (1.0f - val);
      cp.curve_opts =
        (source_ap != nullptr)
          ? dsp::CurveOptions (
              source_ap->curveOpts ()->curviness (),
              source_ap->curveOpts ()->algorithm ())
          : dsp::CurveOptions{};

      points_.push_back (std::move (cp));
    };

  structure::arrangement::for_each_loop_segment (
    clip_start_ticks, loop_start_ticks, loop_end_ticks, display_end_tick,
    [&] (const structure::arrangement::LoopSegment &seg) {
      const double vs = seg.virt_start.asDouble ();
      const double ve = seg.virt_end.asDouble ();
      const double as = seg.abs_start.asDouble ();

      // Add a boundary point at the segment start so curves that cross loop
      // boundaries stay continuous.
      {
        auto [val, src_ap] = eval_at_virt_tick (sorted_points, seg.virt_start);
        if (src_ap != nullptr)
          add_point (as, val, src_ap);
      }

      // Add each automation point within this segment.
      for (const auto * ap : sorted_points)
        {
          const double ap_virt = ap->position ()->asTick ().asDouble ();
          if (ap_virt < vs || ap_virt > ve)
            continue;

          const double abs_tick = as + (ap_virt - vs);
          add_point (abs_tick, ap->value (), ap);
        }
    });

  std::ranges::sort (points_, {}, &CachedControlPoint::px);
}

void
AutomationClipCanvasRenderer::paint (QCanvasPainter * painter)
{
  if (points_.empty () || canvas_width_ <= 0.0f || canvas_height_ <= 0.0f)
    return;

  // ---- Build adaptive sample points from cached control points ----
  std::vector<float> sample_xs;
  std::vector<float> sample_ys;
  sample_xs.reserve (static_cast<size_t> (canvas_width_) + 16);
  sample_ys.reserve (static_cast<size_t> (canvas_width_) + 16);

  sample_xs.push_back (points_.front ().px);
  sample_ys.push_back (points_.front ().py);

  for (size_t i = 0; i + 1 < points_.size (); ++i)
    {
      const auto &a = points_[i];
      const auto &b = points_[i + 1];

      const float seg_px = b.px - a.px;
      if (seg_px <= 0.0f)
        continue;

      const float diff = b.val - a.val;

      if (std::abs (diff) < 1e-5f)
        {
          // Flat segment — single lineTo.
          sample_xs.push_back (b.px);
          sample_ys.push_back (b.py);
        }
      else if (a.curve_opts.algo_ == dsp::CurveOptions::Algorithm::Pulse)
        {
          // Step function: hold value then jump at the end.
          sample_xs.push_back (b.px);
          sample_ys.push_back (a.py);
          sample_xs.push_back (b.px);
          sample_ys.push_back (b.py);
        }
      else if (std::abs (a.curve_opts.curviness_) < 1e-4)
        {
          // Linear segment — single lineTo.
          sample_xs.push_back (b.px);
          sample_ys.push_back (b.py);
        }
      else
        {
          // Curved segment — adaptive sampling at ~2 px granularity.
          const int steps = std::clamp (static_cast<int> (seg_px * 0.5f), 4, 64);
          for (int s = 1; s <= steps; ++s)
            {
              const float t = static_cast<float> (s) / steps;
              const float val = dsp::evaluate_curve (
                a.val, b.val, a.curve_opts.algo_,
                static_cast<float> (a.curve_opts.curviness_), t);
              sample_xs.push_back (a.px + t * seg_px);
              sample_ys.push_back (canvas_height_ * (1.0f - val));
            }
        }
    }

  painter->setRenderHint (QCanvasPainter::RenderHint::Antialiasing, true);

  // ---- Fill below the curve with a vertical gradient ----
  painter->beginPath ();
  painter->moveTo (sample_xs.front (), sample_ys.front ());
  for (size_t i = 1; i < sample_xs.size (); ++i)
    painter->lineTo (sample_xs[i], sample_ys[i]);
  painter->lineTo (sample_xs.back (), canvas_height_);
  painter->lineTo (sample_xs.front (), canvas_height_);
  painter->closePath ();

  QCanvasLinearGradient gradient (0.0f, 0.0f, 0.0f, canvas_height_);
  QColor                top_fill = curve_color_;
  top_fill.setAlphaF (0.30f);
  QColor bottom_fill = curve_color_;
  bottom_fill.setAlphaF (0.05f);
  gradient.setStartColor (top_fill);
  gradient.setEndColor (bottom_fill);
  painter->setFillStyle (gradient);
  painter->fill ();

  // ---- Stroke the open curve ----
  painter->beginPath ();
  painter->moveTo (sample_xs.front (), sample_ys.front ());
  for (size_t i = 1; i < sample_xs.size (); ++i)
    painter->lineTo (sample_xs[i], sample_ys[i]);
  painter->setLineWidth (2.0f);
  painter->setStrokeStyle (curve_color_);
  painter->stroke ();

  // ---- Center line (0.5 value) ----
  QColor center_color = curve_color_;
  center_color.setAlphaF (0.3f);
  painter->setStrokeStyle (center_color);
  painter->setLineWidth (1.0f);
  painter->beginPath ();
  painter->moveTo (0.0f, canvas_height_ * 0.5f);
  painter->lineTo (canvas_width_, canvas_height_ * 0.5f);
  painter->stroke ();

  // ---- Automation point circles ----
  painter->setFillStyle (curve_color_);
  for (const auto &p : points_)
    {
      painter->beginPath ();
      painter->circle (p.px, p.py, 3.0f);
      painter->fill ();
    }
}

} // namespace zrythm::gui::qquick
