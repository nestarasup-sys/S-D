import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    color: "#151c23"
    signal importImage()
    signal loadDemo()
    signal recover()
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 16; spacing: 12
        Label { text: "BIBLIOTECA"; font.pixelSize: 10; font.letterSpacing: 2; color: "#8c9aa7" }
        Label { text: "Sua matéria-prima"; font.pixelSize: 18; font.weight: Font.DemiBold; color: "#e1e8ed" }
        Label { text: "Comece com um elemento.\nTransforme na sua composição."; color: "#83919e"; font.pixelSize: 11; lineHeight: 1.4 }
        StudioButton { text: "+   Importar imagem"; Layout.fillWidth: true; accent: true; onClicked: importImage() }
        RowLayout {
            StudioButton { text: "T   Texto"; Layout.fillWidth: true; onClicked: editor.addNode("text") }
            StudioButton { text: "▰   Forma"; Layout.fillWidth: true; onClicked: editor.addNode("shape") }
        }
        Rectangle { Layout.fillWidth: true; height: 1; color: "#2b3540"; Layout.topMargin: 8; Layout.bottomMargin: 6 }
        Label { text: "OBJETOS NA COMPOSIÇÃO"; color: "#83919e"; font.pixelSize: 10; font.letterSpacing: 1 }
        ListView {
            Layout.fillWidth: true; Layout.fillHeight: true; clip: true; spacing: 6
            model: editor.nodes
            delegate: Rectangle {
                required property var modelData
                width: ListView.view.width; height: 48; radius: 6
                color: editor.selectedId === modelData.id ? "#29372d" : "#1c252e"
                border.color: editor.selectedId === modelData.id ? "#557445" : "#2a3540"
                RowLayout {
                    anchors.fill: parent; anchors.margins: 9; spacing: 9
                    Label { text: modelData.kind === "text" ? "T" : modelData.kind === "image" ? "▧" : "▰"; color: modelData.color; font.pixelSize: 19; Layout.preferredWidth: 22 }
                    ColumnLayout {
                        spacing: 2; Layout.fillWidth: true
                        Label { text: modelData.name; color: "#d5dfe6"; font.pixelSize: 11; elide: Text.ElideRight; Layout.fillWidth: true }
                        Label { text: (modelData.duration / editor.fps).toFixed(1) + " s  ·  " + (modelData.animated ? "animado" : "estático"); color: "#7d8c99"; font.pixelSize: 10 }
                    }
                }
                MouseArea { anchors.fill: parent; onPressed: parent.forceActiveFocus(); onClicked: editor.selectNode(modelData.id) }
            }
        }
        StudioButton { text: "Carregar demonstração"; Layout.fillWidth: true; quiet: true; onClicked: loadDemo() }
        StudioButton { text: "Recuperar última edição"; Layout.fillWidth: true; visible: editor.recoveryAvailable; onClicked: recover() }
        Label { text: "Neste teste: texto, formas e imagens.\nVídeo e áudio entram na próxima etapa."; color: "#677987"; font.pixelSize: 10; lineHeight: 1.4; Layout.topMargin: 4 }
    }
}
