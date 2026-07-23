import QtQuick
import QtQuick.Controls

ApplicationWindow {
    id: appWindow
    visible: false
    width: 560
    height: 460
    title: "Just Did"
    flags: Qt.FramelessWindowHint | Qt.Window
    color: "#ffffff"

    property var floatingWin: null

    // Create floating window independently
    Component.onCompleted: {
        var comp = Qt.createComponent("FloatingWindow.qml")
        if (comp.status === Component.Ready) {
            floatingWin = comp.createObject(null, {"mainWindow": appWindow})
        }
    }

    // Title bar + content
    Column {
        anchors.fill: parent

        // Title bar
        Rectangle {
            width: parent.width
            height: 32
            color: "#4A90D9"

            // Drag to move
            MouseArea {
                anchors.fill: parent
                property point lastPos: Qt.point(0, 0)
                onPressed: lastPos = Qt.point(mouse.x, mouse.y)
                onPositionChanged: {
                    appWindow.x += mouse.x - lastPos.x
                    appWindow.y += mouse.y - lastPos.y
                }
            }

            Row {
                anchors.left: parent.left
                anchors.leftMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                Text {
                    text: "Just Did"
                    color: "white"
                    font.pixelSize: 13
                    font.bold: true
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            Row {
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                anchors.rightMargin: 4
                spacing: 2

                // Minimize → show floating window
                Rectangle {
                    width: 28; height: 22; radius: 3
                    color: minimizeBtn.containsMouse ? "#3A7BC8" : "transparent"
                    Text {
                        anchors.centerIn: parent
                        text: "—"
                        color: "white"
                        font.pixelSize: 14
                        font.bold: true
                    }
                    MouseArea {
                        id: minimizeBtn
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: {
                            appWindow.visible = false
                            appWindow.floatingWin.visible = true
                        }
                    }
                }

                // Close → quit
                Rectangle {
                    width: 28; height: 22; radius: 3
                    color: closeBtn.containsMouse ? "#C0392B" : "transparent"
                    Text {
                        anchors.centerIn: parent
                        text: "✕"
                        color: "white"
                        font.pixelSize: 14
                    }
                    MouseArea {
                        id: closeBtn
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: Qt.quit()
                    }
                }
            }
        }

        // Main content
        MainWindow {
            width: parent.width
            height: parent.height - 32
        }
    }
}
