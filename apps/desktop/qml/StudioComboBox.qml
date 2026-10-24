import QtQuick
import QtQuick.Controls

ComboBox {
    id: control
    implicitHeight: 34
    implicitWidth: 180
    leftPadding: 11
    rightPadding: 28
    font.pixelSize: 12
    background: Rectangle { radius: 5; color: "#202b34"; border.color: control.activeFocus ? "#b4f078" : "#344550" }
    contentItem: Text {
        text: control.displayText; font: control.font; color: "#dce7ed"
        verticalAlignment: Text.AlignVCenter; elide: Text.ElideRight
    }
    indicator: Text { x: control.width - 23; y: (control.height - height) / 2; text: "⌄"; color: "#a4b5c1"; font.pixelSize: 16 }
    delegate: ItemDelegate {
        id: entry
        required property var modelData
        required property int index
        width: control.width - 8
        text: control.textRole ? modelData[control.textRole] : modelData
        contentItem: Text { text: entry.text; color: "#dce7ed"; font.pixelSize: 12; elide: Text.ElideRight; verticalAlignment: Text.AlignVCenter }
        background: Rectangle { radius: 4; color: entry.highlighted ? "#345044" : "transparent" }
        highlighted: control.highlightedIndex === index
    }
    popup: Popup {
        y: control.height + 4; width: control.width; padding: 4
        implicitHeight: Math.min(300, contentItem.implicitHeight + 8)
        background: Rectangle { radius: 6; color: "#1a252e"; border.color: "#3a4b57" }
        contentItem: ListView {
            clip: true; implicitHeight: contentHeight
            model: control.popup.visible ? control.delegateModel : null
            currentIndex: control.highlightedIndex
            ScrollBar.vertical: ScrollBar {}
        }
    }
}
