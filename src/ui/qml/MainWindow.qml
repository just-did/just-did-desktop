import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    color: "#ffffff"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        // ==========================================
        // Row 1: Left (reports 70%) + Right (calendar/status 30%)
        // ==========================================
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 8

            // ---- Left: Report detail panel (70%) ----
            Rectangle {
                Layout.preferredWidth: root.width * 0.6
                Layout.fillHeight: true
                color: "#ffffff"
                border.color: "#e0e0e0"
                border.width: 1
                radius: 4

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 8
                    spacing: 4

                    // Date header
                    Text {
                        id: dateHeader
                        text: timelineVM.selectedYear + "年" + timelineVM.selectedMonth + "月" + timelineVM.selectedDay + "日"
                        font.pixelSize: 13
                        font.bold: true
                        color: "#333"
                    }

                    // Report list with scrollbar
                    ListView {
                        id: listView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        model: recordListModel
                        clip: true

                        ScrollBar.vertical: ScrollBar {
                            policy: ScrollBar.AsNeeded
                        }

                        delegate: RecordItem {
                            width: listView.width
                            timeText: time
                            contentText: content
                        }
                    }
                }
            }

            // ---- Right: Calendar + Storage + HTTP status (30%) ----
            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 8

                // Row 1: Calendar
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
                                color: calendarVM.canGoNext ? "#008cff" : "#ccc"
                                MouseArea {
                                    anchors.fill: parent
                                    anchors.margins: -4
                                    onClicked: {
                                        if (calendarVM.canGoNext) calendarVM.nextMonth()
                                    }
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
                                    if (!isCurrentMonth) return "transparent"
                                    if (isToday) return "#1a73e8"
                                    if (isFuture) return "#f0f0f0"
                                    if (hasReport) return "#ddeeff"
                                    return "#f5f5f5"
                                }
                                border.color: {
                                    if (isSelected && isCurrentMonth && !isFuture) return "#008cff"
                                    return "transparent"
                                }
                                border.width: isSelected && isCurrentMonth && !isFuture ? 1.5 : 0

                                Text {
                                    anchors.centerIn: parent
                                    text: dayNumber
                                    color: {
                                        if (!isCurrentMonth) return "#ccc"
                                        if (isToday) return "#ffffff"
                                        if (isFuture) return "#bbb"
                                        if (hasReport) return "#008cff"
                                        return "#333"
                                    }
                                    font.pixelSize: 10
                                    font.bold: isToday || (hasReport && isCurrentMonth && !isFuture)
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

                // Row 2: Storage usage (progress bar style)
                Rectangle {
                    Layout.fillWidth: true
                    height: 36
                    color: "#f5f5f5"
                    border.color: "#e0e0e0"
                    border.width: 1
                    radius: 4

                    Column {
                        anchors.centerIn: parent
                        width: parent.width - 16
                        spacing: 4

                        RowLayout {
                            width: parent.width
                            Text {
                                text: "存储使用"
                                font.pixelSize: 10
                                color: "#666"
                            }
                            Text {
                                Layout.fillWidth: true
                                text: storageVM.totalSizeText
                                font.pixelSize: 10
                                color: "#999"
                                horizontalAlignment: Text.AlignRight
                            }
                        }

                        Rectangle {
                            width: parent.width
                            height: 6
                            radius: 3
                            color: "#e0e0e0"

                            Rectangle {
                                width: parent.width * storageVM.usageRatio
                                height: parent.height
                                radius: 3
                                color: storageVM.usageRatio > 0.8 ? "#E74C3C" : "#2ECC71"
                            }
                        }
                    }
                }

                // Row 3: HTTP panel (QR + status)
                Rectangle {
                    Layout.fillWidth: true
                    height: 100
                    color: "#f5f5f5"
                    border.color: "#e0e0e0"
                    border.width: 1
                    radius: 4

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 8

                        // Left: QR code
                        Rectangle {
                            width: 84; height: 84
                            color: "white"
                            border.color: "#e0e0e0"
                            border.width: 1

                            Image {
                                anchors.fill: parent
                                anchors.margins: 2
                                source: "image://qrcode/current?v=" + connectionVM.qrVersion
                                cache: false
                                fillMode: Image.PreserveAspectFit
                            }

                            Text {
                                anchors.centerIn: parent
                                text: "QR"
                                font.pixelSize: 12
                                color: "#ccc"
                                visible: connectionVM.connectionState < 1
                            }
                        }

                        // Right: Status + button
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 6

                            // Connection status indicator
                            RowLayout {
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

                            // Start / Stop button
                            Rectangle {
                                Layout.fillWidth: true
                                height: 26
                                radius: 13
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
                        }
                    }
                }
            }
        }

        // ==========================================
        // Row 2: Record module (input + submit)
        // ==========================================
        Rectangle {
            Layout.fillWidth: true
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
    }
}
