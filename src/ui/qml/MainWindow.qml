import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    color: "#ffffff"

    RowLayout {
        anchors.fill: parent
        anchors.margins: 12
        anchors.bottomMargin: 8
        spacing: 10

        // ============ LEFT: Records Timeline ============
        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: "#ffffff"
            border.color: "#e0e0e0"
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 4

                // Header
                Text {
                    text: timelineVM.selectedYear > 0
                          ? (timelineVM.selectedYear + "年" + timelineVM.selectedMonth + "月" + timelineVM.selectedDay + "日")
                          : "点击日历选择日期"
                    font.pixelSize: 13
                    font.bold: true
                    color: "#333"
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

                    // Empty placeholder
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

        // ============ RIGHT: Calendar + Controls ============
        ColumnLayout {
            width: 150
            spacing: 6

            // Calendar panel
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 180
                color: "#ffffff"
                border.color: "#d0d0d0"
                border.width: 1
                radius: 4

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 6
                    spacing: 2

                    // Month navigation
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Text {
                            text: "<"
                            font.pixelSize: 14; font.bold: true
                            color: "#008cff"
                            MouseArea {
                                anchors.fill: parent
                                anchors.margins: -4
                                onClicked: calendarVM.prevMonth()
                            }
                        }
                        Text {
                            Layout.fillWidth: true
                            text: calendarVM.currentYear + "年" + calendarVM.currentMonth + "月"
                            horizontalAlignment: Text.AlignHCenter
                            font.pixelSize: 10
                            color: "#666"
                        }
                        Text {
                            text: ">"
                            font.pixelSize: 14; font.bold: true
                            color: "#008cff"
                            MouseArea {
                                anchors.fill: parent
                                anchors.margins: -4
                                onClicked: calendarVM.nextMonth()
                            }
                        }
                    }

                    // Day-of-week headers
                    Row {
                        anchors.horizontalCenter: parent.horizontalCenter
                        spacing: 1
                        Repeater {
                            model: ["日", "一", "二", "三", "四", "五", "六"]
                            delegate: Rectangle {
                                width: 20; height: 16
                                color: "transparent"
                                Text {
                                    anchors.centerIn: parent
                                    text: modelData
                                    font.pixelSize: 9
                                    color: "#aaa"
                                }
                            }
                        }
                    }

                    // Calendar grid
                    GridView {
                        id: grid
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        cellWidth: 20; cellHeight: 20
                        model: calendarModel
                        interactive: false

                        delegate: Rectangle {
                            width: 19; height: 19
                            radius: 2
                            color: {
                                if (hasReport && isCurrentMonth) return "#ddeeff"
                                if (isCurrentMonth) return "#f5f5f5"
                                return "transparent"
                            }
                            border.color: hasReport && isCurrentMonth ? "#008cff" : "transparent"
                            border.width: hasReport && isCurrentMonth ? 1 : 0

                            Text {
                                anchors.centerIn: parent
                                text: dayNumber
                                color: hasReport && isCurrentMonth ? "#008cff" : (isCurrentMonth ? "#333" : "#ccc")
                                font.pixelSize: 10
                                font.bold: hasReport && isCurrentMonth
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

            // Mode toggle
            Rectangle {
                Layout.fillWidth: true
                height: 24
                radius: 3
                color: "#f0f0f0"
                border.color: "#e0e0e0"

                property bool clearMode: false

                Row {
                    anchors.fill: parent
                    Rectangle {
                        width: parent.width / 2; height: parent.height
                        color: !parent.parent.clearMode ? "#008cff" : "transparent"
                        radius: 3
                        Text {
                            anchors.centerIn: parent
                            text: "加载"
                            color: !parent.parent.clearMode ? "white" : "#666"
                            font.pixelSize: 10
                        }
                        MouseArea {
                            anchors.fill: parent
                            onClicked: parent.parent.parent.clearMode = false
                        }
                    }
                    Rectangle {
                        width: parent.width / 2; height: parent.height
                        color: parent.parent.clearMode ? "#008cff" : "transparent"
                        radius: 3
                        Text {
                            anchors.centerIn: parent
                            text: "清空"
                            color: parent.parent.clearMode ? "white" : "#666"
                            font.pixelSize: 10
                        }
                        MouseArea {
                            anchors.fill: parent
                            onClicked: parent.parent.parent.clearMode = true
                        }
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

            // QR code placeholder
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 60
                color: "#f5f5f5"
                border.color: "#e0e0e0"
                radius: 2
                Text {
                    anchors.centerIn: parent
                    text: "二维码"
                    font.pixelSize: 11
                    color: "#999"
                }
            }

            // Start button
            Rectangle {
                Layout.fillWidth: true
                height: 24
                radius: 12
                color: connectionVM.connectionState === 0 ? "#1ba1e2" : "#E74C3C"

                Text {
                    anchors.centerIn: parent
                    text: connectionVM.connectionState === 0 ? "启动" : "停止"
                    color: "white"
                    font.pixelSize: 11; font.bold: true
                }

                MouseArea {
                    anchors.fill: parent
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

    // ============ BOTTOM: Input + Submit ============
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 12
        anchors.bottomMargin: 8
        height: 44
        color: "#ffffff"
        border.color: "#e0e0e0"
        border.width: 1
        radius: 4

        RowLayout {
            anchors.fill: parent
            anchors.margins: 4
            spacing: 6

            TextArea {
                id: inputArea
                Layout.fillWidth: true
                Layout.fillHeight: true
                font.pixelSize: 12
                placeholderText: "输入今天做了什么..."
                wrapMode: TextArea.Wrap
                background: Rectangle {
                    color: "transparent"
                    border.width: 0
                }
            }

            // Submit button
            Rectangle {
                width: 60; height: 34
                radius: 17
                color: "#008cff"

                Text {
                    anchors.centerIn: parent
                    text: "提交"
                    color: "white"
                    font.pixelSize: 12; font.bold: true
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        if (inputArea.text.trim() !== "") {
                            floatingInputVM.inputText = inputArea.text
                            floatingInputVM.submitRecord()
                            inputArea.text = ""
                        }
                    }
                }
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
