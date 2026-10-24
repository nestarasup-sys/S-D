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
            Binding {
                target: input; property: "text"
                value: field.currentValue === undefined ? "" : String(field.currentValue)
                when: !input.activeFocus; restoreMode: Binding.RestoreNone
            }
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
    component CameraField: ColumnLayout {
        id: cameraField
        property string caption
        property string propertyKey
        property real currentValue
        spacing: 5
        Layout.fillWidth: true
        Label { text: cameraField.caption; color: "#8193a2"; font.pixelSize: 10 }
        TextField {
            id: cameraInput
            Layout.fillWidth: true; implicitHeight: 32
            Binding { target: cameraInput; property: "text"; value: Number(cameraField.currentValue).toFixed(2); when: !cameraInput.activeFocus; restoreMode: Binding.RestoreNone }
            color: "#dce6ee"; font.pixelSize: 12; selectByMouse: true
            background: Rectangle { color: "#10171e"; radius: 5; border.color: cameraInput.activeFocus ? "#b4f078" : "#303e49" }
            onEditingFinished: {
                const value = Number(text)
                if (!text.trim() || !isFinite(value)) { text = Number(cameraField.currentValue).toFixed(2); return }
                editor.updateCamera(cameraField.propertyKey, value)
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
            Label { text: "CÂMERA 2D"; color: "#8193a2"; font.pixelSize: 10; font.letterSpacing: 1; Layout.topMargin: 8 }
            RowLayout {
                CameraField { caption: "Centro X"; propertyKey: "x"; currentValue: editor.camera.x }
                CameraField { caption: "Centro Y"; propertyKey: "y"; currentValue: editor.camera.y }
            }
            RowLayout {
                CameraField { caption: "Zoom"; propertyKey: "zoom"; currentValue: editor.camera.zoom }
                CameraField { caption: "Rotação"; propertyKey: "rotation"; currentValue: editor.camera.rotation }
            }
            Label { text: "Os valores gravam um keyframe no quadro atual."; color: "#708393"; font.pixelSize: 10; wrapMode: Text.WordWrap; Layout.fillWidth: true }
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
                RowLayout {
                    Field { caption: "Rotação · graus"; propertyKey: "rotation"; currentValue: panel.node.rotation }
                    Field { caption: "Escala"; propertyKey: "scale"; currentValue: panel.node.scale }
                }
                Rectangle { Layout.fillWidth: true; height: 1; color: "#2b3540" }
                Label { text: "APARÊNCIA"; color: "#8193a2"; font.pixelSize: 10; font.letterSpacing: 1; visible: panel.node.kind !== "group" }
                Field { caption: "Cor · #RRGGBB"; propertyKey: "color"; currentValue: panel.node.color; number: false; visible: panel.node.kind !== "image" && panel.node.kind !== "group" }
                Field { caption: "Cor da pele · #RRGGBB"; propertyKey: "secondaryColor"; currentValue: panel.node.secondaryColor; number: false; visible: panel.node.nativeId === "person" }
                StudioComboBox {
                    Layout.fillWidth: true; visible: panel.node.nativeId === "person"
                    model: ["neutral", "point", "wave", "worried"]
                    currentIndex: Math.max(0, model.indexOf(panel.node.pose))
                    onActivated: editor.updateSelected("pose", currentText)
                }
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
                StudioComboBox {
                    Layout.fillWidth: true; model: ["none", "fade", "rise", "slide-left", "slide-right", "pop"]
                    currentIndex: Math.max(0, model.indexOf(panel.node.motion))
                    onActivated: editor.updateSelected("motion", currentText)
                }
                RowLayout {
                    Field { caption: "Deslocamento X"; propertyKey: "travelX"; currentValue: panel.node.travelX }
                    Field { caption: "Deslocamento Y"; propertyKey: "travelY"; currentValue: panel.node.travelY }
                }
                Field { caption: "Giro no intervalo · graus"; propertyKey: "spinDegrees"; currentValue: panel.node.spinDegrees }
                StudioButton { text: panel.node.kind === "group" ? "Remover grupo e seus filhos" : "Remover objeto"; Layout.fillWidth: true; quiet: true; onClicked: editor.removeSelected() }
                StudioButton { text: "Criar grupo com este objeto"; Layout.fillWidth: true; visible: panel.node.kind !== "group"; onClicked: editor.groupSelected() }
                StudioButton { text: "Gravar keyframe · quadro " + editor.frame + " · " + (panel.node.keyframeCount || 0); Layout.fillWidth: true; accent: true; onClicked: editor.recordSelectedKeyframe() }
            }
            Item { Layout.preferredHeight: 12 }
        }
    }
}
