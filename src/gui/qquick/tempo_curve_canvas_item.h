// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "structure/arrangement/tempo_object_manager.h"

#include <QColor>
#include <QItemSelectionModel>
#include <QPointer>
#include <QtCanvasPainter/qcanvaspainteritem.h>

namespace zrythm::gui::qquick
{

class TempoCurveCanvasRenderer;

/**
 * @brief QML-visible canvas item that renders the tempo automation curve in
 * the background of the tempo lane.
 *
 * Draws the continuous tempo curve (constant steps and linear ramps) behind
 * the tempo objects using the GPU-accelerated QCanvasPainter API. The data is
 * read from the TempoObjectManager (the same objects rendered as pills), so the
 * curve always matches what is shown and edited. The vertical range is the
 * project-wide min/max of all tempo objects.
 */
class TempoCurveCanvasItem : public QCanvasPainterItem
{
  Q_OBJECT
  QML_NAMED_ELEMENT (TempoCurveCanvas)

  Q_PROPERTY (
    zrythm::structure::arrangement::TempoObjectManager * tempoObjectManager READ
      tempoObjectManager WRITE setTempoObjectManager NOTIFY
        tempoObjectManagerChanged)
  Q_PROPERTY (
    qreal pxPerTick READ pxPerTick WRITE setPxPerTick NOTIFY pxPerTickChanged)
  Q_PROPERTY (qreal scrollX READ scrollX WRITE setScrollX NOTIFY scrollXChanged)
  Q_PROPERTY (
    qreal scrollXPlusWidth READ scrollXPlusWidth WRITE setScrollXPlusWidth
      NOTIFY scrollXPlusWidthChanged)
  Q_PROPERTY (
    QColor curveColor READ curveColor WRITE setCurveColor NOTIFY
      curveColorChanged)
  Q_PROPERTY (
    QItemSelectionModel * selectionModel READ selectionModel WRITE
      setSelectionModel NOTIFY selectionModelChanged)
  Q_PROPERTY (
    bool dragActive READ dragActive WRITE setDragActive NOTIFY dragActiveChanged)
  Q_PROPERTY (
    qreal dragDeltaPx READ dragDeltaPx WRITE setDragDeltaPx NOTIFY
      dragDeltaPxChanged)

public:
  explicit TempoCurveCanvasItem (QQuickItem * parent = nullptr);

  QCanvasPainterItemRenderer * createItemRenderer () const override;

  structure::arrangement::TempoObjectManager * tempoObjectManager () const
  {
    return manager_;
  }
  void
  setTempoObjectManager (structure::arrangement::TempoObjectManager * manager);
  qreal  pxPerTick () const { return px_per_tick_; }
  void   setPxPerTick (qreal px);
  qreal  scrollX () const { return scroll_x_; }
  void   setScrollX (qreal x);
  qreal  scrollXPlusWidth () const { return scroll_x_plus_width_; }
  void   setScrollXPlusWidth (qreal w);
  QColor curveColor () const { return curve_color_; }
  void   setCurveColor (const QColor &color);
  QItemSelectionModel * selectionModel () const { return selection_model_; }
  void                  setSelectionModel (QItemSelectionModel * model);
  bool                  dragActive () const { return drag_active_; }
  void                  setDragActive (bool active);
  qreal                 dragDeltaPx () const { return drag_delta_px_; }
  void                  setDragDeltaPx (qreal px);

Q_SIGNALS:
  void tempoObjectManagerChanged ();
  void pxPerTickChanged ();
  void scrollXChanged ();
  void scrollXPlusWidthChanged ();
  void curveColorChanged ();
  void selectionModelChanged ();
  void dragActiveChanged ();
  void dragDeltaPxChanged ();

private:
  QPointer<structure::arrangement::TempoObjectManager> manager_;
  qreal                                                px_per_tick_ = 0.0;
  qreal                                                scroll_x_ = 0.0;
  qreal                         scroll_x_plus_width_ = 0.0;
  QColor                        curve_color_ = "#009DFF";
  QPointer<QItemSelectionModel> selection_model_;
  bool                          drag_active_ = false;
  qreal                         drag_delta_px_ = 0.0;
};

} // namespace zrythm::gui::qquick
