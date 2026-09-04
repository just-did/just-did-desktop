import QtQuick

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
    property string collapseText: "刚刚做了什么？"
    property color collapseTextColor: "white"
    property color collapseBgColor: "#4A90D9"

    width: 180
    height: 36

    onExpandedChanged: {
        if (expanded) {
            width = 300; height = 200
            // Keep within screen bounds
            if (x + width > Screen.width) x = Screen.width - width - 10
            if (y + height > Screen.height) y = Screen.height - height - 10
            if (x < 0) x = 10
            if (y < 0) y = 10
        }
        else { width = 180; height = 36 }
    }

    // Multi-click detection timer
    Timer {
        id: clickTimer
        interval: 300
        onTriggered: {
            if (floatingWin.clickCount >= 2) {
                // Double click → expand
                floatingWin.expanded = true
            }
            floatingWin.clickCount = 0
        }
    }

    // Single visual root item (Qt Window requires exactly one root child)
    Item {
        anchors.fill: parent

        // Collapsed state
        Rectangle {
            anchors.fill: parent
            color: floatingWin.collapseBgColor
            radius: 8
            visible: !floatingWin.expanded

            Text {
                anchors.centerIn: parent
                text: floatingWin.collapseText
                color: floatingWin.collapseTextColor
                font.pixelSize: 13
                horizontalAlignment: Text.AlignHCenter
            }

            MouseArea {
                id: collapseMouseArea
                anchors.fill: parent
                hoverEnabled: true
                property point dragStart: Qt.point(0, 0)
                property bool moved: false

                onEntered: {
                    hintTimer.start()
                }
                onExited: {
                    hintTimer.stop()
                    floatingWin.collapseText = "刚刚做了什么？"
                    floatingWin.collapseTextColor = "white"
                    floatingWin.collapseBgColor = "#4A90D9"
                }

                onPressed: {
                    dragStart = Qt.point(mouse.x, mouse.y)
                    moved = false
                }

                onPositionChanged: {
                    if (!pressed) return
                    if (!moved && (Math.abs(mouse.x - dragStart.x) > 3
                        || Math.abs(mouse.y - dragStart.y) > 3)) {
                        moved = true
                    }
                    floatingWin.x += mouse.x - dragStart.x
                    floatingWin.y += mouse.y - dragStart.y
                }

                onClicked: {
                    if (moved) return
                    floatingWin.clickCount++
                    if (floatingWin.clickCount === 1) {
                        clickTimer.start()
                    } else if (floatingWin.clickCount === 2) {
                        clickTimer.restart()
                    } else if (floatingWin.clickCount >= 3) {
                        // Triple click → back to main window
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

        // Hover hint timer
        Timer {
            id: hintTimer
            interval: 1000
            onTriggered: {
                floatingWin.collapseText = "双击进入展开模式\n三击进入主窗口"
                floatingWin.collapseBgColor = "#4CAF50"
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

            // Drag handle
            Rectangle {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                height: 30
                color: "transparent"

                Text {
                    anchors.centerIn: parent
                    text: "记录今天做了什么"
                    font.pixelSize: 13
                    font.bold: true
                }

                MouseArea {
                    anchors.fill: parent
                    property point dragStart: Qt.point(0, 0)

                    onPressed: {
                        dragStart = Qt.point(mouse.x, mouse.y)
                    }

                    onPositionChanged: {
                        floatingWin.x += mouse.x - dragStart.x
                        floatingWin.y += mouse.y - dragStart.y
                    }
                }
            }

            ScrollableTextArea {
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
                enterSubmits: true
                autoFocus: true
                onSubmitRequested: {
                    if (inputArea.text.trim() !== "") {
                        floatingInputVM.inputText = inputArea.text
                        floatingInputVM.submitRecord()
                        if (floatingInputVM.inputText === "") {
                            inputArea.text = ""
                            floatingWin.expanded = false
                        }
                    }
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

    Rectangle {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 8
        visible: toastVM.message !== ""
        color: "#cc000000"
        radius: 6
        width: floatingToastText.implicitWidth + 24
        height: floatingToastText.implicitHeight + 12
        z: 100

        Text {
            id: floatingToastText
            anchors.centerIn: parent
            text: toastVM.message
            color: "white"
            font.pixelSize: 11
        }
    }
}
