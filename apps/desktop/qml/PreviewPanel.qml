import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Studio.Dark

Rectangle {
    color: "#0e141b"
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 20; spacing: 12
        RowLayout {
            Layout.fillWidth: true
            ColumnLayout {
                spacing: 5
                Label { text: "MONITOR DE COMPOSIÇÃO"; color: "#778997"; font.pixelSize: 10; font.letterSpacing: 1.5 }
                Label { text: editor.title; color: "#e0e8ed"; font.pixelSize: 18; font.weight: Font.DemiBold }
            }
            Item { Layout.fillWidth: true }
            Label { text: editor.canvasWidth + " × " + editor.canvasHeight + "   /   " + editor.fps.toFixed(2) + " fps"; color: "#748896"; font.pixelSize: 11 }
        }
        Item {
            Layout.fillWidth: true; Layout.fillHeight: true
            Rectangle {
                anchors.centerIn: parent
                width: Math.min(parent.width, parent.height * editor.canvasWidth / editor.canvasHeight)
                height: width * editor.canvasHeight / editor.canvasWidth
                color: "#0b1014"; border.color: "#35414a"
                PreviewItem { anchors.fill: parent; anchors.margins: 1; controller: editor; clip: true }
            }
        }
        StudioComboBox {
            Layout.fillWidth: true; visible: editor.scenes.length > 0
            model: editor.scenes; textRole: "name"; valueRole: "start"
            currentIndex: Math.max(0, editor.scenes.findIndex(function(s) { return editor.frame >= s.start && editor.frame < s.start + s.duration }))
            onActivated: editor.frame = currentValue
        }
        RowLayout {
            Layout.fillWidth: true; spacing: 10
            Label { text: "Arraste os objetos na prévia para posicionar."; font.pixelSize: 10; color: "#6c7d8b" }
            Item { Layout.fillWidth: true }
            Label { text: "PRÉVIA  /  AJUSTAR"; color: "#6c7d8b"; font.pixelSize: 10 }
        }
        Rectangle { height: 1; Layout.fillWidth: true; color: "#26323d" }
        RowLayout {
            Layout.alignment: Qt.AlignHCenter; spacing: 12
            StudioButton { text: "|◀"; quiet: true; onClicked: editor.frame = 0 }
            StudioButton { text: "‹"; quiet: true; onClicked: editor.frame -= 1 }
            StudioButton { text: editor.playing ? "Ⅱ  Pausar" : "▶  Reproduzir"; accent: true; onClicked: editor.togglePlayback() }
            StudioButton { text: "›"; quiet: true; onClicked: editor.frame += 1 }
            Label { text: editor.timecode(editor.frame); color: "#d9e5eb"; font.family: "Consolas"; font.pixelSize: 14 }
            Label { text: "/  " + editor.timecode(editor.duration); color: "#697d8d"; font.family: "Consolas"; font.pixelSize: 12 }
        }
    }
}
