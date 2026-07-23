import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Tab bar
        TabBar {
            id: tabBar
            Layout.fillWidth: true
            TabButton { text: "日报" }
            TabButton { text: "存储管理" }
        }

        // Content
        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabBar.currentIndex

            // Daily report view
            ColumnLayout {
                spacing: 8
                anchors.fill: parent
                anchors.margins: 12

                CalendarView {
                    Layout.fillWidth: true
                }

                TimelineView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                }
            }

            // Storage management view
            StorageView {
                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }

        // Status bar at bottom
        StatusBar {
            Layout.fillWidth: true
            Layout.preferredHeight: 48
        }
    }
}
