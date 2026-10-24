import QtQuick
import QtQuick.Controls

TextField {
    id: control
    implicitHeight: 40
    color: "#e3ebee"
    placeholderTextColor: "#71838f"
    selectByMouse: true
    font.pixelSize: 13
    leftPadding: 13
    background: Rectangle { radius: 7; color: "#111b23"; border.color: control.activeFocus ? "#b4f078" : "#34434f" }
}
