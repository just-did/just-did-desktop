import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: appWindow
    visible: true
    width: 550
    height: 430
    title: "Just Did"

    // X button → quit app
    onClosing: Qt.quit()

    // Minimize button → show floating window instead of minimizing
    onWindowStateChanged: {
        if (windowState === Qt.WindowMinimized) {
            windowState = Qt.WindowNoState
            visible = false
            floatingWinTimer.start()
        }
    }

    Timer {
        id: floatingWinTimer
        interval: 50
        onTriggered: floatingWin.visible = true
    }

    MainWindow {
        anchors.fill: parent
    }

    FloatingWindow {
        id: floatingWin
    }
}
