import QtQuick
import QtQuick.Controls

Rectangle {
    id: statusBar
    height: 48
    color: "#f5f5f5"
    border.color: "#e0e0e0"
    border.width: 1

    Row {
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.leftMargin: 12
        spacing: 12

        // Status indicator
        Rectangle {
            width: 10; height: 10
            radius: 5
            anchors.verticalCenter: parent.verticalCenter
            color: {
                switch (connectionVM.connectionState) {
                    case 0: return "#999"      // Unstarted
                    case 1: return "#E74C3C"   // Disconnected
                    case 2: return "#2ECC71"   // Connected
                    case 3: return "#F39C12"   // Syncing
                }
            }
        }

        Text {
            text: {
                switch (connectionVM.connectionState) {
                    case 0: return "未启动"
                    case 1: return "未连接"
                    case 2: return "已连接"
                    case 3: return "同步中..."
                }
            }
            font.pixelSize: 12
            anchors.verticalCenter: parent.verticalCenter
        }

        Text {
            text: connectionVM.qrCodeUrl
            font.pixelSize: 11
            color: "#888"
            anchors.verticalCenter: parent.verticalCenter
            visible: connectionVM.connectionState >= 1
        }
    }

    Row {
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        anchors.rightMargin: 12
        spacing: 8

        Button {
            text: connectionVM.connectionState === 0 ? "启动服务" : "停止服务"
            height: 28
            onClicked: {
                if (connectionVM.connectionState === 0)
                    connectionVM.startServer()
                else
                    connectionVM.stopServer()
            }
        }
    }
}
