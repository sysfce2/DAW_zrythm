// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import QtQuick
import Zrythm
import ZrythmGui

Item {
  id: root

  required property int contentHeight
  required property int contentWidth
  required property AutomationClip automationClip

  AutomationClipCanvas {
    height: root.contentHeight
    width: root.contentWidth
    automationClip: root.automationClip
    curveColor: ZrythmTheme.clipContentColor
  }
}
