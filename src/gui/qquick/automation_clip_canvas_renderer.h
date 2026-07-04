// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <vector>

#include "dsp/curve.h"

#include <QColor>
#include <QtCanvasPainter/qcanvaslineargradient.h>
#include <QtCanvasPainter/qcanvaspainter.h>
#include <QtCanvasPainter/qcanvaspainteritemrenderer.h>

namespace zrythm::gui::qquick
{

class AutomationClipCanvasItem;

class AutomationClipCanvasRenderer : public QCanvasPainterItemRenderer
{
public:
  AutomationClipCanvasRenderer () = default;
  Q_DISABLE_COPY_MOVE (AutomationClipCanvasRenderer)

  void synchronize (QCanvasPainterItem * item) override;
  void paint (QCanvasPainter * painter) override;

private:
  struct CachedControlPoint
  {
    /** Pixel x coordinate (after loop unwrapping). */
    float px;
    /** Pixel y coordinate (pre-computed from val). */
    float py;
    /** Normalized automation value (0..1). */
    float val;
    /**
     * @brief Curve options for the segment starting at this point.
     *
     * Unused for the last point (no segment after it).
     */
    dsp::CurveOptions curve_opts;
  };

  std::vector<CachedControlPoint> points_;
  QColor                          curve_color_;
  float                           canvas_width_ = 0.0f;
  float                           canvas_height_ = 0.0f;
  qreal                           reference_width_ = 0;
  qreal                           reference_x_ = 0;
};

} // namespace zrythm::gui::qquick
