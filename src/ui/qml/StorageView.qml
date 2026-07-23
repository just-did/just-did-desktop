import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    spacing: 16
    anchors.margins: 16

    Text {
        text: "存储管理"
        font.pixelSize: 18
        font.bold: true
    }

    // Overview
    GroupBox {
        title: "存储概览"
        Layout.fillWidth: true

        ColumnLayout {
            Text { text: "总占用空间: " + (storageVM.totalSize / 1024).toFixed(1) + " KB" }

            Repeater {
                model: storageVM.statsByYear
                delegate: Text {
                    text: modelData.year + "年: " + (modelData.size / 1024).toFixed(1) + " KB"
                }
            }
        }
    }

    Button {
        text: "刷新统计"
        onClicked: storageVM.refreshStats()
    }

    // Placeholder for backup/restore UI
    Text {
        text: "备份和恢复功能即将上线"
        color: "#999"
        font.pixelSize: 12
    }

    Component.onCompleted: storageVM.refreshStats()
}
