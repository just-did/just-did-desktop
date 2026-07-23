import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    color: "#ffffff"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 6

        // Main content: left timeline + right calendar
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 8

            // Left: timeline / records
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                border.color: "#e0e0e0"
                border.width: 1
                radius: 4

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 4
                    spacing: 4

                    Text {
                        text: timelineVM.selectedYear > 0
                              ? (timelineVM.selectedYear + "-" + timelineVM.selectedMonth + "-" + timelineVM.selectedDay)
                              : "选择日期查看日报"
                        font.pixelSize: 14
                        font.bold: true
                    }

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

                        Text {
                            anchors.centerIn: parent
                            text: timelineVM.selectedYear > 0 ? "暂无记录" : ""
                            color: "#999"
                            font.pixelSize: 13
                            visible: listView.count === 0
                        }
                    }
                }
            }

            // Right: calendar + status + mode
            ColumnLayout {
                width: 160
                spacing: 6

                // Calendar
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 200
                    border.color: "#e0e0e0"
                    border.width: 1
                    radius: 4

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 4
                        spacing: 2

                        // Month navigation
                        RowLayout {
                            Layout.fillWidth: true
                            Button {
                                text: "<"
                                implicitWidth: 24; implicitHeight: 18
                                font.pixelSize: 10
                                onClicked: calendarVM.prevMonth()
                            }
                            Text {
                                Layout.fillWidth: true
                                text: calendarVM.currentYear + "-" + calendarVM.currentMonth
                                horizontalAlignment: Text.AlignHCenter
                                font.pixelSize: 11
                                font.bold: true
                            }
                            Button {
                                text: ">"
                                implicitWidth: 24; implicitHeight: 18
                                font.pixelSize: 10
                                onClicked: calendarVM.nextMonth()
                            }
                        }

                        // Day-of-week headers
                        Row {
                            anchors.horizontalCenter: parent.horizontalCenter
                            spacing: 1
                            Repeater {
                                model: ["日", "一", "二", "三", "四", "五", "六"]
                                delegate: Rectangle {
                                    width: 18; height: 14
                                    color: "transparent"
                                    Text {
                                        anchors.centerIn: parent
                                        text: modelData
                                        font.pixelSize: 9
                                        color: "#999"
                                    }
                                }
                            }
                        }

                        // Calendar grid
                        GridView {
                            id: grid
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            cellWidth: 18; cellHeight: 18
                            model: calendarModel
                            interactive: false

                            delegate: Rectangle {
                                width: 17; height: 17
                                radius: 2
                                color: hasReport ? "#4A90D9" : (isCurrentMonth ? "#f5f5f5" : "transparent")

                                Text {
                                    anchors.centerIn: parent
                                    text: dayNumber
                                    color: hasReport ? "white" : (isCurrentMonth ? "#333" : "#ccc")
                                    font.pixelSize: 9
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    onClicked: {
                                        if (isCurrentMonth) {
                                            timelineVM.selectDate(calendarVM.currentYear, calendarVM.currentMonth, dayNumber)
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // Mode switch: 加载模式 / 清空模式
                Row {
                    anchors.horizontalCenter: parent.horizontalCenter
                    spacing: 2
                    property bool clearMode: false

                    Rectangle {
                        width: 60; height: 20; radius: 4
                        color: !parent.clearMode ? "#4A90D9" : "#e0e0e0"
                        Text {
                            anchors.centerIn: parent
                            text: "加载"
                            color: !parent.clearMode ? "white" : "#666"
                            font.pixelSize: 10
                        }
                        MouseArea {
                            anchors.fill: parent
                            onClicked: parent.parent.clearMode = false
                        }
                    }
                    Rectangle {
                        width: 60; height: 20; radius: 4
                        color: parent.clearMode ? "#E74C3C" : "#e0e0e0"
                        Text {
                            anchors.centerIn: parent
                            text: "清空"
                            color: parent.clearMode ? "white" : "#666"
                            font.pixelSize: 10
                        }
                        MouseArea {
                            anchors.fill: parent
                            onClicked: parent.parent.clearMode = true
                        }
                    }
                }

                // Connection status
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    Rectangle {
                        width: 8; height: 8; radius: 4
                        color: {
                            switch (connectionVM.connectionState) {
                                case 0: return "#999"
                                case 1: return "#E74C3C"
                                case 2: return "#2ECC71"
                                case 3: return "#F39C12"
                            }
                        }
                    }
                    Text {
                        text: {
                            switch (connectionVM.connectionState) {
                                case 0: return "未启动"
                                case 1: return "未连接"
                                case 2: return "已连接"
                                case 3: return "同步中"
                            }
                        }
                        font.pixelSize: 10
                        color: "#666"
                    }
                }

                // QR code placeholder + start button
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 4

                    Rectangle {
                        width: 30; height: 30
                        color: "#f0f0f0"
                        border.color: "#e0e0e0"
                        radius: 2
                        Text {
                            anchors.centerIn: parent
                            text: "QR"
                            font.pixelSize: 8
                            color: "#999"
                        }
                    }

                    Button {
                        text: connectionVM.connectionState === 0 ? "启动" : "停止"
                        implicitWidth: 50; implicitHeight: 22
                        font.pixelSize: 10
                        onClicked: {
                            if (connectionVM.connectionState === 0)
                                connectionVM.startServer()
                            else
                                connectionVM.stopServer()
                        }
                    }
                }

                Item { Layout.fillHeight: true }
            }
        }

        // Bottom: input area + submit button + clear button
        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            TextArea {
                id: inputArea
                Layout.fillWidth: true
                Layout.preferredHeight: 40
                font.pixelSize: 12
                placeholderText: "输入今天做了什么..."
                wrapMode: TextArea.Wrap
            }

            Button {
                text: "提交"
                implicitWidth: 50; implicitHeight: 40
                font.pixelSize: 12
                onClicked: {
                    if (inputArea.text.trim() !== "") {
                        floatingInputVM.inputText = inputArea.text
                        floatingInputVM.submitRecord()
                        inputArea.text = ""
                    }
                }
            }

            Button {
                text: "清空当天"
                implicitWidth: 60; implicitHeight: 40
                font.pixelSize: 11
                visible: timelineVM.hasContent
                onClicked: confirmDialog.open()
            }
        }
    }

    // Confirm clear dialog
    Dialog {
        id: confirmDialog
        title: "确认清空"
        standardButtons: Dialog.Ok | Dialog.Cancel
        anchors.centerIn: parent
        Text {
            text: "确定要清空 " + timelineVM.selectedYear + "-"
                  + timelineVM.selectedMonth + "-" + timelineVM.selectedDay + " 的日报吗？"
            font.pixelSize: 13
            width: 250
            wrapMode: Text.Wrap
        }
        onAccepted: timelineVM.clearSelectedDate()
    }
}
