// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import Zrythm
import ZrythmGui

Loader {
  id: root

  enum DragMode {
    None,
    Move,
    ResizeFromStart,
    ResizeFromEnd
  }

  // GPU-composited visual drag feedback — no x/width property changes, so no
  // compute_peaks(), MIDI re-layout, or re-serialization. Short-circuits on
  // selectionTracker.isSelected so non-selected delegates don't re-evaluate.
  readonly property bool _dragActive: selectionTracker.isSelected && dragMode !== ArrangerObjectLoader.DragMode.None
  required property ArrangerObject arrangerObject
  required property ItemSelectionModel arrangerSelectionModel
  property real dragDeltaPx: 0
  property real dragDeltaY: 0

  // Drag visual feedback (driven by Arranger mouse handlers via Repeater).
  // Uses ArrangerObjectLoader.DragMode enum.
  property int dragMode: ArrangerObjectLoader.DragMode.None
  required property int index
  required property var model
  readonly property real objectEndX: x + width
  readonly property real objectX: posTracker.timelineTicks * pxPerTick
  required property real pxPerTick
  required property real scrollViewWidth
  required property real scrollX
  readonly property alias selectionTracker: selectionTracker
  required property UnifiedProxyModel unifiedObjectsModel
  property bool useCustomWidth: false // use `width` instead of bounds-based
  readonly property bool useParentPosition: arrangerObject.parentObject !== null

  // Viewport culling
  active: objectEndX + ZrythmTheme.scrollLoaderBufferPx >= scrollX && objectX <= (scrollX + scrollViewWidth + ZrythmTheme.scrollLoaderBufferPx)
  asynchronous: true
  visible: status === Loader.Ready
  x: objectX

  transform: [
    Translate {
      x: (root._dragActive && root.dragMode === ArrangerObjectLoader.DragMode.Move) ? root.dragDeltaPx : 0
      y: (root._dragActive && root.dragMode === ArrangerObjectLoader.DragMode.Move) ? root.dragDeltaY : 0
    },
    Scale {
      origin.x: (root._dragActive && root.dragMode === ArrangerObjectLoader.DragMode.ResizeFromStart) ? root.width : 0
      xScale: {
        if (!root._dragActive || root.width <= 0)
          return 1;
        if (root.dragMode === ArrangerObjectLoader.DragMode.ResizeFromEnd)
          return (root.width + root.dragDeltaPx) / root.width;
        if (root.dragMode === ArrangerObjectLoader.DragMode.ResizeFromStart)
          return (root.width - root.dragDeltaPx) / root.width;
        return 1;
      }
    }
  ]
  Binding on width {
    value: Math.max((posTracker.timelineEndTicks - posTracker.timelineTicks) * root.pxPerTick, 2)
    when: !root.useCustomWidth && root.arrangerObject.length
  }

  TimelinePositionTracker {
    id: posTracker

    arrangerObject: root.arrangerObject
  }

  SelectionTracker {
    id: selectionTracker

    // Dummy property to force the modelIndex to be recalculated
    property bool dummy: false

    modelIndex: {
      root.unifiedObjectsModel.addSourceModel(root.model);
      const ret = root.unifiedObjectsModel.mapFromSource(root.model.index(root.index, 0));
      dummy;
      return ret;
    }
    selectionModel: root.arrangerSelectionModel
  }

  // When objects are added to previous source models in the unified model, the modelIndex above is not updated, so we force an update here when objects are added/removed
  Connections {
    function onRowsInserted() {
      selectionTracker.dummy = !selectionTracker.dummy;
    }

    function onRowsRemoved() {
      selectionTracker.dummy = !selectionTracker.dummy;
    }

    target: root.unifiedObjectsModel
  }
}
