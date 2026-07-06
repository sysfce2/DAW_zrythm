// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm
import ZrythmControllers
import ZrythmStyle

RowLayout {
  id: root

  required property AppSettings appSettings
  required property Metronome metronome
  readonly property real playheadBpm: root.tempoMap.tempoAtTick(root.transport.playhead.ticks)
  required property TempoMap tempoMap
  required property Transport transport
  required property TransportController transportController
  required property UndoStack undoStack

  LinkedButtons {
    Button {
      icon.source: ResourceManager.getIconUrl("gnome-icon-library", "seek-backward-large-symbolic.svg")

      onClicked: root.transportController.moveBackward()
    }

    Button {
      icon.source: ResourceManager.getIconUrl("gnome-icon-library", "seek-forward-large-symbolic.svg")

      onClicked: root.transportController.moveForward()
    }

    Button {
      icon.source: ResourceManager.getIconUrl("gnome-icon-library", "skip-backward-large-symbolic.svg")
    }

    Button {
      icon.source: ResourceManager.getIconUrl("gnome-icon-library", (root.transport.playState == 1 ? "pause" : "play") + "-large-symbolic.svg")

      onClicked: {
        root.transport.isRolling() ? root.transport.requestPause() : root.transport.requestRoll();
      }
    }

    Button {
      checkable: true
      checked: root.transport.loopEnabled
      icon.source: ResourceManager.getIconUrl("gnome-icon-library", "loop-arrow-symbolic.svg")

      onCheckedChanged: {
        root.transport.loopEnabled = checked;
      }
    }
  }

  RecordSplitButton {
    appSettings: root.appSettings
    transport: root.transport
  }

  MetronomeSplitButton {
    id: metronomeSplitButton

    metronome: root.metronome
  }

  RowLayout {
    spacing: 2

    // BPM readout: follows the tempo at the playhead. Click to edit the base
    // tempo (the permanent tempo anchored at tick 0). The dot appears when the
    // tempo at the playhead differs from the base tempo.
    RowLayout {
      id: bpmCellLayout

      spacing: 2

      EditableValueDisplay {
        label: "bpm"
        value: root.playheadBpm.toFixed(2)
      }

      Rectangle {
        Layout.alignment: Qt.AlignTop
        Layout.preferredHeight: 4
        Layout.preferredWidth: 4
        color: palette.accent
        radius: 3
        visible: Math.abs(root.playheadBpm - root.tempoMap.baseBpm) > 0.01

        ToolTip {
          text: qsTr("Tempo at playhead differs from base tempo (%1 BPM)").arg(root.tempoMap.baseBpm.toFixed(2))
        }
      }

      HoverHandler {
        cursorShape: Qt.PointingHandCursor
      }

      TapHandler {
        acceptedButtons: Qt.LeftButton

        onTapped: bpmEditDialog.open()
      }

      Dialog {
        id: bpmEditDialog

        modal: true
        popupType: Popup.Window
        standardButtons: Dialog.Ok | Dialog.Cancel
        title: qsTr("Edit Base Tempo")

        contentItem: ColumnLayout {
          spacing: ZrythmTheme.buttonPadding

          Label {
            text: qsTr("Base BPM (at tick 0):")
          }

          DoubleSpinBox {
            id: bpmSpinBox

            Layout.fillWidth: true
            decimals: 2
            editable: true
            from: 1.0
            stepSize: 1.0
            to: 999.0
            value: root.tempoMap.baseBpm
          }
        }

        onAboutToShow: bpmSpinBox.value = root.tempoMap.baseBpm
        onAccepted: bpmPropertyOperator.setValueAffectingTempoMap("baseBpm", bpmSpinBox.value)
      }
    }

    EditableValueDisplay {
      id: timeDisplay

      label: "time"
      minValueHeight: timeTextMetrics.height
      minValueWidth: timeTextMetrics.width
      value: root.tempoMap.getMusicalPositionString(root.transport.playhead.ticks)

      TextMetrics {
        id: timeTextMetrics

        font: ZrythmTheme.semiBoldTextFont
        text: "99.9.9.999"
      }
    }

    EditableValueDisplay {
      label: "sig"
      value: {
        const timeSigNumerator = root.tempoMap.timeSignatureNumeratorAtTick(root.transport.playhead.ticks);
        const timeSigDenominator = root.tempoMap.timeSignatureDenominatorAtTick(root.transport.playhead.ticks);
        return `${timeSigNumerator}/${timeSigDenominator}`;
      }
    }
  }

  QObjectPropertyOperator {
    id: bpmPropertyOperator

    currentObject: root.tempoMap
    undoStack: root.undoStack
  }
}
