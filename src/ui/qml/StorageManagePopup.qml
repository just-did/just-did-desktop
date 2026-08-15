import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// 存储管理弹窗：右键存储区域唤起（前置门槛通过后）。
// 模态 + 半透明遮罩（主窗口不可操作），点击遮罩任意位置销毁。
Popup {
    id: popup
    modal: true
    closePolicy: Popup.CloseOnPressOutside
    anchors.centerIn: Overlay.overlay
    width: 320
    padding: 12

    Overlay.modal: Rectangle {
        color: "#80000000"
    }

    background: Rectangle {
        color: "#ffffff"
        radius: 6
        border.color: "#d0d0d0"
        border.width: 1
    }

    // 待确认动作："selected"/"week"/"month"/"year"；空串 = 不显示确认层
    property string pendingAction: ""
    property int pendingCount: 0
    property string pendingLabel: ""

    ColumnLayout {
        anchors.fill: parent
        spacing: 6

        // 标题
        Text {
            Layout.alignment: Qt.AlignHCenter
            text: "存储管理"
            font.pixelSize: 13
            font.bold: true
            color: "#333"
        }

        // 月导航（与主日历一致，不可翻到未来月）
        RowLayout {
            Layout.fillWidth: true

            Text {
                text: "<"
                font.pixelSize: 14; font.bold: true
                color: "#008cff"
                MouseArea {
                    anchors.fill: parent
                    anchors.margins: -4
                    onClicked: storageCleanupVM.prevMonth()
                }
            }
            Text {
                Layout.fillWidth: true
                text: storageCleanupVM.currentYear + "年" + storageCleanupVM.currentMonth + "月"
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: 10
                color: "#666"
            }
            Text {
                text: ">"
                font.pixelSize: 14; font.bold: true
                color: storageCleanupVM.canGoNext ? "#008cff" : "#ccc"
                MouseArea {
                    anchors.fill: parent
                    anchors.margins: -4
                    onClicked: {
                        if (storageCleanupVM.canGoNext) storageCleanupVM.nextMonth()
                    }
                }
            }
        }

        // 星期表头
        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 1
            Repeater {
                model: ["日", "一", "二", "三", "四", "五", "六"]
                delegate: Rectangle {
                    width: 34; height: 16
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

        // 日历网格：绿点=有日报，未来灰显，isPicked 高亮；仅非未来可点选
        GridView {
            id: grid
            width: 238
            height: 156
            anchors.horizontalCenter: parent.horizontalCenter
            cellWidth: 34; cellHeight: 26
            model: storageCleanupVM.calendarModel
            interactive: false

            delegate: Rectangle {
                width: 33; height: 25
                radius: 2
                color: {
                    if (!isCurrentMonth) return "transparent"
                    if (isFuture) return "#f0f0f0"
                    return "#f5f5f5"
                }
                border.color: isPicked && isCurrentMonth ? "#008cff" : "transparent"
                border.width: isPicked && isCurrentMonth ? 1.5 : 0

                Rectangle {
                    width: 5; height: 5
                    radius: 2.5
                    color: "#4CAF50"
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.topMargin: 1
                    anchors.leftMargin: 1
                    visible: hasReport && isCurrentMonth
                }

                Text {
                    anchors.centerIn: parent
                    text: dayNumber
                    color: {
                        if (!isCurrentMonth) return "#ccc"
                        if (isFuture) return "#bbb"
                        return "#333"
                    }
                    font.pixelSize: 10
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        if (isCurrentMonth && !isFuture)
                            storageCleanupVM.toggleDay(dayNumber)
                    }
                }
            }
        }

        // 已选计数 + 全选
        RowLayout {
            Layout.fillWidth: true
            Text {
                text: "已选 " + storageCleanupVM.pickedCount + " 天"
                font.pixelSize: 10
                color: "#666"
            }
            Item { Layout.fillWidth: true }
            Rectangle {
                width: 56; height: 22
                radius: 11
                color: "#f0f0f0"
                border.color: "#d0d0d0"
                border.width: 1
                Text {
                    anchors.centerIn: parent
                    text: "全选"
                    font.pixelSize: 10
                    color: "#333"
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: storageCleanupVM.toggleSelectAllMonth()
                }
            }
        }

        // 四个清理动作
        GridLayout {
            Layout.fillWidth: true
            columns: 2
            rowSpacing: 6
            columnSpacing: 8

            // 清理所选
            Rectangle {
                Layout.preferredWidth: 140
                height: 30
                radius: 15
                color: storageCleanupVM.pickedCount > 0 ? "#008cff" : "#c8c8c8"
                Text {
                    anchors.centerIn: parent
                    text: "清理所选(" + storageCleanupVM.pickedCount + ")"
                    color: "white"
                    font.pixelSize: 11
                    font.bold: true
                }
                MouseArea {
                    anchors.fill: parent
                    enabled: storageCleanupVM.pickedCount > 0
                    onClicked: {
                        pendingAction = "selected"
                        pendingCount = storageCleanupVM.pickedCount
                        pendingLabel = "清理所选日期"
                    }
                }
            }

            // 清理一周前
            Rectangle {
                Layout.preferredWidth: 140
                height: 30
                radius: 15
                color: storageCleanupVM.weekCount > 0 ? "#008cff" : "#c8c8c8"
                Text {
                    anchors.centerIn: parent
                    text: "一周前(" + storageCleanupVM.weekCount + ")"
                    color: "white"
                    font.pixelSize: 11
                    font.bold: true
                }
                MouseArea {
                    anchors.fill: parent
                    enabled: storageCleanupVM.weekCount > 0
                    onClicked: {
                        pendingAction = "week"
                        pendingCount = storageCleanupVM.weekCount
                        pendingLabel = "清理一周前"
                    }
                }
            }

            // 清理一月前
            Rectangle {
                Layout.preferredWidth: 140
                height: 30
                radius: 15
                color: storageCleanupVM.monthCount > 0 ? "#008cff" : "#c8c8c8"
                Text {
                    anchors.centerIn: parent
                    text: "一月前(" + storageCleanupVM.monthCount + ")"
                    color: "white"
                    font.pixelSize: 11
                    font.bold: true
                }
                MouseArea {
                    anchors.fill: parent
                    enabled: storageCleanupVM.monthCount > 0
                    onClicked: {
                        pendingAction = "month"
                        pendingCount = storageCleanupVM.monthCount
                        pendingLabel = "清理一月前"
                    }
                }
            }

            // 清理一年前
            Rectangle {
                Layout.preferredWidth: 140
                height: 30
                radius: 15
                color: storageCleanupVM.yearCount > 0 ? "#008cff" : "#c8c8c8"
                Text {
                    anchors.centerIn: parent
                    text: "一年前(" + storageCleanupVM.yearCount + ")"
                    color: "white"
                    font.pixelSize: 11
                    font.bold: true
                }
                MouseArea {
                    anchors.fill: parent
                    enabled: storageCleanupVM.yearCount > 0
                    onClicked: {
                        pendingAction = "year"
                        pendingCount = storageCleanupVM.yearCount
                        pendingLabel = "清理一年前"
                    }
                }
            }
        }

        // 内嵌确认层：确认后执行清理、关弹窗并 toast；取消保留全部状态
        Rectangle {
            anchors.fill: parent
            color: "#ffffff"
            radius: 6
            visible: pendingAction !== ""

            ColumnLayout {
                anchors.centerIn: parent
                spacing: 10

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: pendingLabel + "：将删除 " + pendingCount + " 个日期，确认清理？"
                    font.pixelSize: 12
                    color: "#333"
                    horizontalAlignment: Text.AlignHCenter
                }

                RowLayout {
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 12

                    Rectangle {
                        width: 70; height: 28
                        radius: 14
                        color: "#f0f0f0"
                        border.color: "#d0d0d0"
                        border.width: 1
                        Text {
                            anchors.centerIn: parent
                            text: "取消"
                            font.pixelSize: 11
                            color: "#333"
                        }
                        MouseArea {
                            anchors.fill: parent
                            onClicked: pendingAction = ""
                        }
                    }

                    Rectangle {
                        width: 70; height: 28
                        radius: 14
                        color: "#E74C3C"
                        Text {
                            anchors.centerIn: parent
                            text: "确认清理"
                            font.pixelSize: 11
                            color: "white"
                            font.bold: true
                        }
                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                var n = storageCleanupVM.executePending(pendingAction)
                                toastVM.show("已清理 " + n + " 个日期")
                                pendingAction = ""
                                popup.close()
                            }
                        }
                    }
                }
            }
        }
    }
}
