import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    spacing: 4

    // Month navigation
    RowLayout {
        Layout.fillWidth: true

        Button {
            text: "<"
            onClicked: calendarVM.prevMonth()
        }

        Text {
            Layout.fillWidth: true
            text: calendarVM.currentYear + "年" + calendarVM.currentMonth + "月"
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: 16
            font.bold: true
        }

        Button {
            text: ">"
            onClicked: calendarVM.nextMonth()
        }
    }

    // Day-of-week headers
    Row {
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 2
        Repeater {
            model: ["日", "一", "二", "三", "四", "五", "六"]
            delegate: Rectangle {
                width: 34; height: 24
                color: "transparent"
                Text {
                    anchors.centerIn: parent
                    text: modelData
                    font.pixelSize: 12
                    color: "#888"
                }
            }
        }
    }

    // Calendar grid
    GridView {
        id: grid
        Layout.fillWidth: true
        Layout.preferredHeight: 34 * 6 + 12
        cellWidth: 34; cellHeight: 34
        model: calendarModel
        interactive: false

        delegate: Rectangle {
            width: 32; height: 32
            radius: 4
            color: hasReport ? "#4A90D9" : (isCurrentMonth ? "#f0f0f0" : "transparent")

            Text {
                anchors.centerIn: parent
                text: dayNumber
                color: hasReport ? "white" : (isCurrentMonth ? "#333" : "#ccc")
                font.pixelSize: 13
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
