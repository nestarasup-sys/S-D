import QtQuick
import QtQuick.Controls

Button {
    id: control
    property bool accent: false
    property bool quiet: false
    implicitHeight: 34
    implicitWidth: Math.max(36, label.implicitWidth + 24)
    font.pixelSize: 12
    font.weight: Font.DemiBold
    padding: 10
    background: Rectangle {
        radius: 6
        color: !control.enabled ? "#171d23" : control.accent ? (control.hovered ? "#c4ff8e" : "#b4f078") : control.hovered ? "#303a43" : control.quiet ? "transparent" : "#242d35"
        border.color: control.accent ? "transparent" : "#34404a"
        border.width: control.quiet ? 0 : 1
    }
    contentItem: Text {
        id: label
        text: control.text
        font: control.font
        color: !control.enabled ? "#58636e" : control.accent ? "#182311" : "#dae1e6"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
}
