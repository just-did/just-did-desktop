import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: appWindow
    visible: true
    width: 800
    height: 600
    title: "刚刚做了什么"

    // Main window and floating window visibility
    property bool mainWindowVisible: true

    MainWindow {
        id: mainWindow
        visible: appWindow.mainWindowVisible
        anchors.fill: parent
    }

    FloatingWindow {
        id: floatingWindow
        visible: !appWindow.mainWindowVisible
    }

    Connections {
        target: floatingInputVM
        function onRequestShowMainWindow() {
            appWindow.mainWindowVisible = true
        }
    }

    // Close to tray instead of quitting
    onClosing: function(close) {
        close.accepted = false
        appWindow.mainWindowVisible = false
    }
}
