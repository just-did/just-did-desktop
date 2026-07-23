import QtQuick
import QtQuick.Controls

Window {
    id: floatingWin
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.Tool
    visible: false
    x: Screen.width - width - 20
    y: Screen.height / 2 - height / 2

    property bool expanded: false

    // Collapsed size: 180x36, expanded size: 300x200
    width: 180
    height: 36

    onExpandedChanged: {
        if (expanded) { width = 300; height = 200 }
        else { width = 180; height = 36 }
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
            onClicked: floatingWin.expanded = true
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
    }

    // Click empty area to collapse / double-click to show main
    MouseArea {
        anchors.fill: parent
        z: -1
        visible: floatingWin.expanded
        onClicked: floatingWin.expanded = false
        onDoubleClicked: {
            floatingWin.visible = false
            appWindow.visible = true
        }
    }
}
