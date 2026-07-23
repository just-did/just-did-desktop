import QtQuick

Rectangle {
    id: root
    property string timeText: ""
    property string contentText: ""
    height: 30
    color: "transparent"

    Row {
        anchors.left: parent.left
        anchors.leftMargin: 4
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        anchors.rightMargin: 4
        spacing: 8

        Text {
            text: root.timeText
            font.pixelSize: 12
            font.bold: true
            color: "#008cff"
            width: 40
        }

        Text {
            text: root.contentText
            font.pixelSize: 12
            color: "#333"
            elide: Text.ElideRight
            anchors.right: parent.right
        }
    }

    Rectangle {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 1
        color: "#f0f0f0"
    }
}
