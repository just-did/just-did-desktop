import QtQuick
import QtQuick.Controls

Rectangle {
    id: root
    property string timeText: ""
    property string contentText: ""
    height: rowLayout.implicitHeight + 12
    color: "transparent"

    Row {
        id: rowLayout
        anchors.left: parent.left
        anchors.leftMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        anchors.rightMargin: 12
        spacing: 12

        Text {
            text: root.timeText
            font.pixelSize: 13
            font.bold: true
            color: "#4A90D9"
            width: 45
        }

        Text {
            text: root.contentText
            font.pixelSize: 13
            color: "#333"
            wrapMode: Text.Wrap
            anchors.right: parent.right
            leftPadding: 4
        }
    }

    Rectangle {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.leftMargin: 12
        anchors.right: parent.right
        anchors.rightMargin: 12
        height: 1
        color: "#eee"
    }
}
