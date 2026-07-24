import QtQuick

Rectangle {
    id: root
    property string timeText: ""
    property string contentText: ""
    height: column.implicitHeight + 12
    color: "transparent"

    Column {
        id: column
        anchors.left: parent.left
        anchors.leftMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        anchors.rightMargin: 12
        spacing: 2

        Text {
            text: root.timeText
            font.pixelSize: 11
            font.bold: true
            color: "#888"
        }

        Text {
            text: root.contentText
            font.pixelSize: 13
            color: "#333"
            wrapMode: Text.Wrap
            width: column.width
        }
    }

    Rectangle {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.leftMargin: 12
        anchors.right: parent.right
        anchors.rightMargin: 12
        height: 1
        color: "#f0f0f0"
    }
}
