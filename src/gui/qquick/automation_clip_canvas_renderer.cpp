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

namespace zrythm::gui::qquick
{

namespace
{

/// Evaluates the automation value at a virtual tick position and returns the
/// value along with the AutomationPoint whose curve segment drives that
/// position (nullptr if before the first point).
auto
eval_at_virt_tick (
  const auto                                   &sorted_points,
  const structure::arrangement::AutomationClip &clip,
  double                                        virt_tick)
  -> std::pair<float, const structure::arrangement::AutomationPoint *>
{
  using AP = structure::arrangement::AutomationPoint;

  if (sorted_points.empty ())
    return { 0.0f, nullptr };

  const auto tick_of = [] (const AP * ap) {
    return ap->position ()->asTick ().asDouble ();
  };

  const double first_tick = tick_of (sorted_points.front ());
  const double last_tick = tick_of (sorted_points.back ());

  if (virt_tick <= first_tick)
    return { sorted_points.front ()->value (), sorted_points.front () };
  if (virt_tick >= last_tick)
    return { sorted_points.back ()->value (), sorted_points.back () };

  auto it = std::ranges::lower_bound (sorted_points, virt_tick, {}, tick_of);
  if (it == sorted_points.begin () || it == sorted_points.end ())
    return { sorted_points.back ()->value (), sorted_points.back () };

  const auto * prev_ap = *std::prev (it);
  const auto * next_ap = *it;

  const double prev_tick = tick_of (prev_ap);
  const double next_tick = tick_of (next_ap);
  if (next_tick <= prev_tick)
    return { prev_ap->value (), prev_ap };

  double t = (virt_tick - prev_tick) / (next_tick - prev_tick);
  t = std::clamp (t, 0.0, 1.0);

  const double curve_val = clip.get_normalized_value_in_curve (*prev_ap, t);

  const double diff =
    static_cast<double> (next_ap->value ())
    - static_cast<double> (prev_ap->value ());
  const double val =
    (diff > 0.0)
      ? prev_ap->value () + std::abs (diff) * curve_val
      : next_ap->value () + std::abs (diff) * curve_val;
  return { static_cast<float> (val), prev_ap };
}

} // anonymous namespace

void
AutomationClipCanvasRenderer::synchronize (QCanvasPainterItem * item)
{
  auto * canvas_item = static_cast<AutomationClipCanvasItem *> (item);

  curve_color_ = canvas_item->curveColor ();
  canvas_width_ = static_cast<float> (canvas_item->width ());
  canvas_height_ = static_cast<float> (canvas_item->height ());

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
  if (clip_ticks.asDouble () <= 0.0)
    return;

  const auto loop_start_ticks = clip->loopStartPosition ()->asTick ();
  const auto loop_end_ticks = clip->loopEndPosition ()->asTick ();
  const auto clip_start_ticks = clip->clipStartPosition ()->asTick ();
  const auto loop_length_ticks = max (
    dsp::ContentTick{ units::ticks (0.0) }, loop_end_ticks - loop_start_ticks);

  const double clip_ticks_d = clip_ticks.asDouble ();

  // Loop-walk: identical segment structure to MidiClipCanvasRenderer.
  auto loop_seg_virt_start = clip_start_ticks;
  auto loop_seg_virt_end = loop_end_ticks;
  auto loop_seg_abs_start = dsp::ContentTick{ units::ticks (0.0) };
  auto loop_seg_abs_end = loop_end_ticks - clip_start_ticks;
  if (loop_seg_abs_end > clip_ticks)
    {
      const auto diff = loop_seg_abs_end - clip_ticks;
      loop_seg_virt_end = loop_seg_virt_end - diff;
      loop_seg_abs_end = loop_seg_abs_end - diff;
    }

  const auto add_point =
    [&] (
      double abs_tick_d, float val,
      const structure::arrangement::AutomationPoint * source_ap) {
      if (abs_tick_d < 0.0 || abs_tick_d > clip_ticks_d)
        return;

      const auto * next_ap =
        (source_ap != nullptr) ? clip->get_next_ap (*source_ap, true) : nullptr;

      CachedControlPoint cp;
      const double       norm_x = abs_tick_d / clip_ticks_d;
      cp.px = static_cast<float> (norm_x * canvas_width_);
      cp.val = val;
      cp.py = canvas_height_ * (1.0f - val);
      cp.curve_opts =
        (source_ap != nullptr)
          ? dsp::CurveOptions (
              source_ap->curveOpts ()->curviness (),
              source_ap->curveOpts ()->algorithm ())
          : dsp::CurveOptions{};
      cp.start_higher =
        (next_ap != nullptr) ? next_ap->value () < source_ap->value () : false;

      points_.push_back (std::move (cp));
    };

  while (loop_seg_abs_start < clip_ticks)
    {
      const double vs = loop_seg_virt_start.asDouble ();
      const double ve = loop_seg_virt_end.asDouble ();
      const double as = loop_seg_abs_start.asDouble ();

      // Add a boundary point at the segment start so curves that cross loop
      // boundaries stay continuous.
      {
        auto [val, src_ap] = eval_at_virt_tick (sorted_points, *clip, vs);
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

      const auto current_len = loop_seg_abs_end - loop_seg_abs_start;
      if (current_len.asDouble () <= 0.0)
        break;

      loop_seg_virt_start = loop_start_ticks;
      loop_seg_virt_end = loop_end_ticks;
      loop_seg_abs_start = loop_seg_abs_start + current_len;
      loop_seg_abs_end = loop_seg_abs_end + loop_length_ticks;

      if (loop_seg_abs_end > clip_ticks)
        {
          const auto diff = loop_seg_abs_end - clip_ticks;
          loop_seg_virt_end = loop_seg_virt_end - diff;
          loop_seg_abs_end = loop_seg_abs_end - diff;
        }
    }

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
              const float  t = static_cast<float> (s) / steps;
              const double curve_val =
                a.curve_opts.get_normalized_y (t, a.start_higher);
              const float val =
                (diff > 0.0f)
                  ? a.val + std::abs (diff) * curve_val
                  : b.val + std::abs (diff) * curve_val;
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
