import QtQuick
import QtQuick.Controls

ScrollView {
    id: scrollView

    property alias text: editor.text
    property alias cursorPosition: editor.cursorPosition
    property string placeholderText: ""
    property bool enterSubmits: false
    property bool autoFocus: false
    property bool showVerticalScrollBar: true
    property bool sanitizing: false
    property var viewport: contentItem
    signal submitRequested()

    clip: true
    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
    ScrollBar.vertical.policy: showVerticalScrollBar ? ScrollBar.AsNeeded : ScrollBar.AlwaysOff

    function normalizeText(value) {
        return value.replace(/\r\n/g, "\n")
                    .replace(/\r/g, "\n")
                    .replace(/\n{2,}/g, "\n")
    }

    function ensureCursorVisible() {
        var flickable = scrollView.viewport
        if (!flickable || !editor.activeFocus)
            return
        var top = editor.cursorRectangle.y
        var bottom = top + editor.cursorRectangle.height
        if (top < flickable.contentY)
            flickable.contentY = Math.max(0, top)
        else if (bottom > flickable.contentY + scrollView.availableHeight)
            flickable.contentY = Math.min(
                        Math.max(0, flickable.contentHeight - scrollView.availableHeight),
                        bottom - scrollView.availableHeight)
    }

    TextArea {
        id: editor
        focus: scrollView.autoFocus
        width: scrollView.availableWidth
        implicitHeight: Math.max(scrollView.availableHeight, contentHeight)
        font.pixelSize: 12
        placeholderText: scrollView.placeholderText
        wrapMode: TextArea.Wrap
        selectByMouse: true
        background: Rectangle {
            color: "transparent"
            border.width: 0
        }

        onTextChanged: {
            if (scrollView.sanitizing)
                return
            var cleaned = scrollView.normalizeText(text)
            if (cleaned !== text) {
                var oldCursor = cursorPosition
                var newCursor = scrollView.normalizeText(text.substring(0, oldCursor)).length
                scrollView.sanitizing = true
                text = cleaned
                cursorPosition = Math.min(newCursor, text.length)
                scrollView.sanitizing = false
                toastVM.show("连续空行已自动移除")
            }
            Qt.callLater(scrollView.ensureCursorVisible)
        }
        onCursorRectangleChanged: Qt.callLater(scrollView.ensureCursorVisible)

        Keys.onReturnPressed: function(event) {
            if (scrollView.enterSubmits) {
                event.accepted = true
                scrollView.submitRequested()
            } else {
                event.accepted = false
            }
        }
    }
}
