import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: panel
    color: "#151c23"
    property var node: editor.selected
    component Field: ColumnLayout {
        id: field
        property string caption
        property string propertyKey
        property var currentValue
        property bool number: true
        spacing: 5
        Layout.fillWidth: true
        Label { text: field.caption; color: "#8193a2"; font.pixelSize: 10 }
        TextField {
            id: input
            Layout.fillWidth: true; implicitHeight: 32
            text: field.currentValue === undefined ? "" : String(field.currentValue)
            color: "#dce6ee"; font.pixelSize: 12; selectByMouse: true
            background: Rectangle { color: "#10171e"; radius: 5; border.color: input.activeFocus ? "#b4f078" : "#303e49" }
            onEditingFinished: {
                const value = field.number ? Number(text) : text
                if (field.number && (!text.trim() || !isFinite(value))) {
                    text = String(field.currentValue)
                    return
                }
                editor.updateSelected(field.propertyKey, value)
                text = String(panel.node[field.propertyKey])
            }
        }
    }
    ScrollView {
        anchors.fill: parent; anchors.margins: 17; clip: true
        contentWidth: availableWidth
        ColumnLayout {
            width: parent.width; spacing: 12
            Label { text: "INSPETOR"; font.pixelSize: 10; font.letterSpacing: 2; color: "#8c9aa7" }
            Label { text: panel.node.name || "Nenhum objeto"; font.pixelSize: 17; font.weight: Font.DemiBold; color: "#e1e8ed"; elide: Text.ElideRight; Layout.fillWidth: true }
            Label { text: "Selecione um objeto na biblioteca\nou diretamente na prévia."; visible: !panel.node.id; color: "#83919e"; font.pixelSize: 11 }
            ColumnLayout {
                visible: !!panel.node.id
                Layout.fillWidth: true; spacing: 12
                Label { text: "TRANSFORMAÇÃO"; color: "#8193a2"; font.pixelSize: 10; font.letterSpacing: 1; Layout.topMargin: 8 }
                RowLayout {
                    Field { caption: "Posição X"; propertyKey: "x"; currentValue: panel.node.x }
                    Field { caption: "Posição Y"; propertyKey: "y"; currentValue: panel.node.y }
                }
                RowLayout {
                    Field { caption: "Largura"; propertyKey: "width"; currentValue: panel.node.width }
                    Field { caption: "Altura"; propertyKey: "height"; currentValue: panel.node.height }
                }
                Field { caption: "Opacidade · 0 a 1"; propertyKey: "opacity"; currentValue: panel.node.opacity }
                Rectangle { Layout.fillWidth: true; height: 1; color: "#2b3540" }
                Label { text: "APARÊNCIA"; color: "#8193a2"; font.pixelSize: 10; font.letterSpacing: 1 }
                Field { caption: "Cor · #RRGGBB"; propertyKey: "color"; currentValue: panel.node.color; number: false; visible: panel.node.kind !== "image" }
                Field { caption: "Tamanho do texto"; propertyKey: "fontSize"; currentValue: panel.node.fontSize; visible: panel.node.kind === "text" }
                Label { text: "Conteúdo"; color: "#8193a2"; font.pixelSize: 10; visible: panel.node.kind === "text" }
                TextArea {
                    id: textEditor
                    property string editingId: ""
                    visible: panel.node.kind === "text"; Layout.fillWidth: true; Layout.preferredHeight: 80
                    text: panel.node.text || ""; wrapMode: TextEdit.Wrap; color: "#dae5ed"; font.pixelSize: 12; selectByMouse: true
                    background: Rectangle { color: "#10171e"; radius: 5; border.color: textEditor.activeFocus ? "#b4f078" : "#303e49" }
                    onActiveFocusChanged: {
                        if (activeFocus) { editingId = editor.selectedId; editor.beginGesture() }
                        else { editor.endGesture(); editingId = "" }
                    }
                    onTextChanged: if (activeFocus && editingId === editor.selectedId) editor.updateSelected("text", text)
                    Connections {
                        target: editor
                        function onDocumentChanged() { if (textEditor.activeFocus && textEditor.editingId !== editor.selectedId) textEditor.focus = false }
                    }
                }
                Rectangle { Layout.fillWidth: true; height: 1; color: "#2b3540" }
                Label { text: "TEMPO E MOVIMENTO"; color: "#8193a2"; font.pixelSize: 10; font.letterSpacing: 1 }
                RowLayout {
                    Field { caption: "Início · frames"; propertyKey: "start"; currentValue: panel.node.start }
                    Field { caption: "Duração · frames"; propertyKey: "duration"; currentValue: panel.node.duration }
                }
                CheckBox {
                    text: "Entrada suave"; checked: panel.node.animated || false; font.pixelSize: 12
                    onToggled: editor.updateSelected("animated", checked)
                }
                Label { text: "Movimento de entrada de 24 frames.\nAjustes de tempo são validados."; font.pixelSize: 10; color: "#6d8191"; lineHeight: 1.3 }
                StudioButton { text: "Remover objeto"; Layout.fillWidth: true; quiet: true; onClicked: editor.removeSelected() }
            }
            Item { Layout.preferredHeight: 12 }
        }
    }
}
