// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/position.h"
#include "gui/qquick/automation_clip_canvas_item.h"
#include "gui/qquick/automation_clip_canvas_renderer.h"
#include "structure/arrangement/arranger_object_all.h"
#include "structure/arrangement/clip.h"

namespace zrythm::gui::qquick
{

AutomationClipCanvasItem::AutomationClipCanvasItem (QQuickItem * parent)
    : ClipCanvasItemBase (parent)
{
  setFillColor (Qt::transparent);
  setAlphaBlending (true);
}

QCanvasPainterItemRenderer *
AutomationClipCanvasItem::createItemRenderer () const
{
  return new AutomationClipCanvasRenderer ();
}

void
AutomationClipCanvasItem::setAutomationClip (
  structure::arrangement::AutomationClip * clip)
{
  if (automation_clip_ == clip)
    return;
  automation_clip_ = clip;

  for (const auto &connection : clip_connections_)
    QObject::disconnect (connection);
  clip_connections_.clear ();

  if (automation_clip_ != nullptr)
    {
      clip_connections_.push_back (
        QObject::connect (
          automation_clip_, &structure::arrangement::Clip::contentChanged, this,
          [this] () { update (); }, Qt::ConnectionType::QueuedConnection));

      clip_connections_.push_back (
        QObject::connect (
          automation_clip_,
          &structure::arrangement::Clip::loopablePropertiesChanged, this,
          [this] () { update (); }, Qt::ConnectionType::QueuedConnection));

      if (automation_clip_->length () != nullptr)
        clip_connections_.push_back (
          QObject::connect (
            automation_clip_->length (), &dsp::Position::positionChanged, this,
            [this] () { update (); }, Qt::ConnectionType::QueuedConnection));
    }

  Q_EMIT automationClipChanged ();
  update ();
}

void
AutomationClipCanvasItem::setCurveColor (const QColor &color)
{
  if (curve_color_ == color)
    return;
  curve_color_ = color;
  Q_EMIT curveColorChanged ();
  update ();
}

} // namespace zrythm::gui::qquick
