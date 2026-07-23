import QtQuick
import QtQuick.Controls

Window {
    id: floatingWin
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.Tool
    color: "transparent"
    visible: false
    x: Screen.width - width - 20
    y: Screen.height / 2 - height / 2

    property bool expanded: false
    property var mainWindow: null
    property int clickCount: 0

    width: 180
    height: 36

    onExpandedChanged: {
        if (expanded) { width = 300; height = 200 }
        else { width = 180; height = 36 }
    }

    // Double-click detection timer
    Timer {
        id: clickTimer
        interval: 300
        onTriggered: {
            if (floatingWin.clickCount === 1) {
                // Single click → expand
                floatingWin.expanded = true
            }
            floatingWin.clickCount = 0
        }
    }

    // Collapsed state
    Rectangle {
        anchors.fill: parent
        color: "#4A90D9"
        radius: 8
        visible: !floatingWin.expanded

        Text {
            anchors.centerIn: parent
            text: "刚刚做了什么？"
            color: "white"
            font.pixelSize: 13
        }

        MouseArea {
            anchors.fill: parent
            onClicked: {
                floatingWin.clickCount++
                if (floatingWin.clickCount === 1) {
                    clickTimer.start()
                } else if (floatingWin.clickCount >= 2) {
                    // Double click → back to main window
                    clickTimer.stop()
                    floatingWin.clickCount = 0
                    floatingWin.visible = false
                    if (floatingWin.mainWindow) {
                        floatingWin.mainWindow.visible = true
                    }
                }
            }
        }
    }

    // Expanded state
    Rectangle {
        anchors.fill: parent
        color: "white"
        radius: 8
        border.color: "#4A90D9"
        border.width: 1
        visible: floatingWin.expanded

        Text {
            anchors.top: parent.top
            anchors.topMargin: 12
            anchors.horizontalCenter: parent.horizontalCenter
            text: "记录今天做了什么"
            font.pixelSize: 13
            font.bold: true
        }

        TextArea {
            id: inputArea
            anchors.top: parent.top
            anchors.topMargin: 36
            anchors.left: parent.left
            anchors.leftMargin: 12
            anchors.right: parent.right
            anchors.rightMargin: 12
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 12
            placeholderText: "输入内容..."
            wrapMode: TextArea.Wrap
            focus: true

            Keys.onReturnPressed: function(event) {
                if (inputArea.text.trim() !== "") {
                    floatingInputVM.inputText = inputArea.text
                    floatingInputVM.submitRecord()
                    inputArea.text = ""
                    floatingWin.expanded = false
                }
            }
        }
    }

    // Click outside → collapse
    onActiveChanged: {
        if (!active && expanded) {
            expanded = false
        }
    }
}
