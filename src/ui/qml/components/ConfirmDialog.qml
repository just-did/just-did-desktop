import QtQuick
import QtQuick.Controls

Dialog {
    id: dialog
    title: "确认操作"
    property string message: ""
    standardButtons: Dialog.Ok | Dialog.Cancel

    Text {
        text: dialog.message
        font.pixelSize: 14
        wrapMode: Text.Wrap
        width: 280
    }
}
