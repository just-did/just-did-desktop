import QtQuick
import QtQuick.Controls

Window {
    id: floatingWin
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.Tool
    visible: true
    x: Screen.width - width - 20
    y: Screen.height / 2 - height / 2

    property bool expanded: floatingInputVM.isExpanded

    // Collapsed state
    Rectangle {
        id: collapsedState
        width: 180; height: 36
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
            onClicked: floatingInputVM.toggleExpand()
        }
    }

    // Expanded state
    Rectangle {
        id: expandedState
        width: 300; height: 200
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
                width: parent.width
                height: 100
                text: floatingInputVM.inputText
                onTextChanged: floatingInputVM.inputText = text
                placeholderText: "输入内容..."
                wrapMode: TextArea.Wrap
            }

            Row {
                spacing: 8
                anchors.horizontalCenter: parent.horizontalCenter

                Button {
                    text: "提交"
                    onClicked: floatingInputVM.submitRecord()
                }

                Button {
                    text: "收起"
                    onClicked: floatingInputVM.toggleExpand()
                }
            }
        }

        // Double-click to show main window
        MouseArea {
            anchors.fill: parent
            z: -1
            onDoubleClicked: floatingInputVM.showMainWindow()
        }
    }

    // Bind size to expanded state
    width: floatingWin.expanded ? 300 : 180
    height: floatingWin.expanded ? 200 : 36

    Connections {
        target: floatingInputVM
        function onRecordSubmitted() {
            // Feedback handled in QML
        }
    }
}
