import QtQuick
import QtQuick.Controls

Window {
    id: floatingWin
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.Tool
    visible: false
    x: Screen.width - width - 20
    y: Screen.height / 2 - height / 2

    property bool expanded: false

    // Bind size to expanded state
    width: expanded ? 300 : 180
    height: expanded ? 200 : 36

    // Collapsed state
    Rectangle {
        id: collapsedState
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
            onClicked: floatingWin.expanded = true
        }
    }

    // Expanded state
    Rectangle {
        id: expandedState
        anchors.fill: parent
        color: "white"
        radius: 8
        border.color: "#4A90D9"
        border.width: 1
        visible: floatingWin.expanded

        Column {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 8

            Text {
                text: "记录今天做了什么"
                font.pixelSize: 13
                font.bold: true
            }

            TextArea {
                id: inputArea
                width: parent.width - 24
                height: 100
                placeholderText: "输入内容..."
                wrapMode: TextArea.Wrap
            }

            Row {
                spacing: 8
                anchors.horizontalCenter: parent.horizontalCenter

                Button {
                    text: "提交"
                    onClicked: {
                        if (inputArea.text.trim() !== "") {
                            floatingInputVM.inputText = inputArea.text
                            floatingInputVM.submitRecord()
                            inputArea.text = ""
                            floatingWin.expanded = false
                        }
                    }
                }

                Button {
                    text: "收起"
                    onClicked: floatingWin.expanded = false
                }
            }
        }

        // Click blank area to collapse
        MouseArea {
            anchors.fill: parent
            z: -1
            onClicked: floatingWin.expanded = false
            onDoubleClicked: {
                floatingWin.hide()
                appWindow.visible = true
            }
        }
    }

    function show() {
        floatingWin.visible = true
    }

    function hide() {
        floatingWin.visible = false
    }
}
