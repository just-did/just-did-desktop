import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    spacing: 4

    // Header
    RowLayout {
        Text {
            text: timelineVM.selectedYear > 0
                  ? (timelineVM.selectedYear + "-" + timelineVM.selectedMonth + "-" + timelineVM.selectedDay + " 日报")
                  : "选择日期查看日报"
            font.pixelSize: 14
            font.bold: true
        }

        Item { Layout.fillWidth: true }

        Button {
            text: "清空当天"
            visible: timelineVM.hasContent
            onClicked: {
                confirmDialog.message = "确定清空 " + timelineVM.selectedYear + "-"
                    + timelineVM.selectedMonth + "-" + timelineVM.selectedDay + " 的日报？"
                confirmDialog.open()
            }
        }
    }

    // Record list
    ListView {
        id: listView
        Layout.fillWidth: true
        Layout.fillHeight: true
        model: recordListModel
        clip: true

        delegate: RecordItem {
            width: listView.width
            timeText: time
            contentText: content
        }
    }

    // Empty state
    Text {
        visible: listView.count === 0 && timelineVM.selectedYear > 0
        text: "暂无记录"
        color: "#999"
        font.pixelSize: 14
        anchors.centerIn: parent
    }

    // Confirm dialog
    ConfirmDialog {
        id: confirmDialog
        onAccepted: timelineVM.clearSelectedDate()
    }
}
