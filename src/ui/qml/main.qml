import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: appWindow
    visible: true
    width: 550
    height: 430
    title: "Just Did"

    // X button → quit. Minimize (-) → show floating window
    onClosing: Qt.quit()

    Timer {
        id: minimizeTimer
        interval: 0
        onTriggered: {
            appWindow.visible = false
            floatingWindow.show()
        }
    }

    onVisibilityChanged: {
        if (appWindow.visibility === Window.Minimized) {
            minimizeTimer.start()
        }
    }

    MainWindow {
        anchors.fill: parent
    }

    FloatingWindow {
        id: floatingWindow
    }
}
