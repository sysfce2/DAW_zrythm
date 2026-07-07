// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

import "../config.js" as Config
import QtQuick
import QtQuick.Controls
import Zrythm
import ZrythmStyle
import Qt.labs.synchronizer

MenuBar {
  id: root

  required property AboutDialog aboutDialog
  required property DeviceManager deviceManager
  required property ExportDialog exportDialog
  required property LoadController loadController
  readonly property Project project: session.project
  required property SaveController saveController
  required property ProjectSession session

  Menu {
    title: qsTr("&File")

    MenuItem {
      action: root.saveController.saveAction
    }

    MenuItem {
      action: root.saveController.saveAsAction
    }

    MenuItem {
      action: root.loadController.loadAction
    }

    Action {
      text: qsTr("Export…")

      onTriggered: {
        root.exportDialog.open();
      }
    }
  }

  Menu {
    title: qsTr("&Edit")

    Action {
      text: qsTr("Undo")
    }
  }

  Menu {
    title: qsTr("&View")

    MenuItem {
      checkable: true
      icon.source: ResourceManager.getIconUrl("gnome-icon-library", "dock-left-symbolic.svg")
      text: qsTr("Left Panel")

      Synchronizer on checked {
        sourceObject: GlobalState.application.appSettings
        sourceProperty: "leftPanelVisible"
      }
    }

    MenuItem {
      checkable: true
      icon.source: ResourceManager.getIconUrl("gnome-icon-library", "dock-bottom-symbolic.svg")
      text: qsTr("Bottom Panel")

      Synchronizer on checked {
        sourceObject: GlobalState.application.appSettings
        sourceProperty: "bottomPanelVisible"
      }
    }

    MenuItem {
      checkable: true
      icon.source: ResourceManager.getIconUrl("gnome-icon-library", "dock-right-symbolic.svg")
      text: qsTr("Right Panel")

      Synchronizer on checked {
        sourceObject: GlobalState.application.appSettings
        sourceProperty: "rightPanelVisible"
      }
    }

    MenuSeparator {
    }

    Menu {
      title: qsTr("Language")

      MenuItem {
        id: systemLocaleMenuItem

        checkable: true
        checked: GlobalState.application.appSettings.uiLocale === ""
        text: qsTr("System")

        onTriggered: {
          GlobalState.application.translationManager.loadTranslation("");
        }
      }

      MenuSeparator {
      }

      Repeater {
        model: Object.keys(Config.languageMap).map(code => {
          return ({
              "code": code,
              "name": Config.languageMap[code]
            });
        })

        delegate: MenuItem {
          id: localeMenuItem

          required property string code
          required property string name

          checkable: true
          checked: GlobalState.application.appSettings.uiLocale === code
          text: name

          onTriggered: {
            GlobalState.application.translationManager.loadTranslation(code);
          }
        }
      }
    }

    Menu {
      title: qsTr("Appearance")

      Action {
        icon.source: ResourceManager.getIconUrl("gnome-icon-library", "dark-mode-symbolic.svg")
        text: qsTr("Switch Light/Dark Theme")

        onTriggered: {
          ZrythmTheme.darkMode = !ZrythmTheme.darkMode;
        }
      }

      Menu {
        title: qsTr("Theme Color")

        Action {
          enabled: ZrythmTheme.darkMode
          text: qsTr("Zrythm Orange")

          onTriggered: {
            ZrythmTheme.primaryColor = ZrythmTheme.zrythmColor;
          }
        }

        Action {
          text: qsTr("Celestial Blue")

          onTriggered: {
            ZrythmTheme.primaryColor = ZrythmTheme.celestialBlueColor;
          }
        }

        Action {
          enabled: ZrythmTheme.darkMode
          text: qsTr("Jonquil Yellow")

          onTriggered: {
            ZrythmTheme.primaryColor = ZrythmTheme.jonquilYellowColor;
          }
        }

        Action {
          enabled: ZrythmTheme.darkMode
          text: qsTr("Spring Green")

          onTriggered: {
            ZrythmTheme.primaryColor = ZrythmTheme.springGreen;
          }
        }

        Action {
          enabled: ZrythmTheme.darkMode
          text: qsTr("Munsell Red")

          onTriggered: {
            ZrythmTheme.primaryColor = ZrythmTheme.munsellRed;
          }
        }

        Action {
          enabled: !ZrythmTheme.darkMode
          text: qsTr("Gunmetal")

          onTriggered: {
            ZrythmTheme.primaryColor = ZrythmTheme.gunmetalColor;
          }
        }

        Action {
          text: qsTr("Electric Purple")

          onTriggered: {
            ZrythmTheme.primaryColor = ZrythmTheme.electricPurple;
          }
        }
      }
    }

    MenuItem {
      action: ApplicationWindow.window?.fullScreenAction ?? null
    }

    Menu {
      title: qsTr("Debug")

      MenuItem {
        checkable: true
        text: qsTr("Show Cache Activity")

        Synchronizer on checked {
          sourceObject: GlobalState.application.appSettings
          sourceProperty: "showCacheActivity"
        }
      }
    }
  }

  Menu {
    title: qsTr("Devices")

    Action {
      text: qsTr("Audio/MIDI Setup")

      onTriggered: {
        root.deviceManager.showDeviceSelector();
      }
    }
  }

  Menu {
    title: qsTr("&Help")

    Action {
      text: qsTr("About Zrythm")

      onTriggered: root.aboutDialog.open()
    }
  }
}
