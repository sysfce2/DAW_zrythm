// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <vector>

#include "gui/qquick/clip_canvas_item_base.h"
#include "structure/arrangement/automation_clip.h"

#include <QColor>
#include <QPointer>

namespace zrythm::gui::qquick
{

class AutomationClipCanvasRenderer;

class AutomationClipCanvasItem : public ClipCanvasItemBase
{
  Q_OBJECT
  QML_NAMED_ELEMENT (AutomationClipCanvas)

  Q_PROPERTY (
    zrythm::structure::arrangement::AutomationClip * automationClip READ
      automationClip WRITE setAutomationClip NOTIFY automationClipChanged)
  Q_PROPERTY (
    QColor curveColor READ curveColor WRITE setCurveColor NOTIFY
      curveColorChanged)

public:
  explicit AutomationClipCanvasItem (QQuickItem * parent = nullptr);

  QCanvasPainterItemRenderer * createItemRenderer () const override;

  structure::arrangement::AutomationClip * automationClip () const
  {
    return automation_clip_;
  }
  void setAutomationClip (structure::arrangement::AutomationClip * clip);

  QColor curveColor () const { return curve_color_; }
  void   setCurveColor (const QColor &color);

Q_SIGNALS:
  void automationClipChanged ();
  void curveColorChanged ();

private:
  QPointer<structure::arrangement::AutomationClip> automation_clip_;
  QColor                                           curve_color_;
  std::vector<QMetaObject::Connection>             clip_connections_;
};

} // namespace zrythm::gui::qquick
