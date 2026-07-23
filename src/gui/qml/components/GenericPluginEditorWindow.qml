// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Zrythm

Window {
  id: root

  property Plugin plugin: null

  color: palette.window
  // Tool windows are always kept on top of their (transient) parent while
  // leaving the parent focusable
  flags: Qt.Tool
  height: 480
  minimumHeight: 160
  minimumWidth: 280
  title: root.plugin ? root.plugin.configuration.descriptor.name : ""
  width: 420

  onClosing: {
    // Defer so this window isn't destroyed mid-signal when the model row
    // removal reaches the Instantiator
    Qt.callLater(() => {
      if (root.plugin)
        root.plugin.uiVisible = false;
    });
  }

  ColumnLayout {
    anchors.fill: parent
    anchors.margins: 8
    spacing: 4

    Label {
      Layout.fillWidth: true
      elide: Text.ElideRight
      font.bold: true
      text: root.plugin ? root.plugin.configuration.descriptor.name : ""
    }

    PluginParameterListView {
      Layout.fillHeight: true
      Layout.fillWidth: true
      plugin: root.plugin
    }
  }
}
