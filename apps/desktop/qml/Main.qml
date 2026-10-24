import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

ApplicationWindow {
    id: window
    width: 1440; height: 920
    minimumWidth: 1150; minimumHeight: 760
    visible: true
    title: editor.title + (editor.dirty ? " *" : "") + " — Studio Dark"
    color: "#0e1319"
    font.family: "Segoe UI"
    palette.window: "#151c23"
    palette.text: "#dce3e8"
    palette.buttonText: "#dce3e8"
    palette.base: "#10171e"
    palette.highlight: "#b4f078"
    palette.highlightedText: "#14200e"
    property string pendingAction: ""
    property bool allowClose: false
    function dispatch(action) {
        if (action === "new") editor.newProject(false)
        if (action === "demo") editor.newProject(true)
        if (action === "open") openDialog.open()
        if (action === "recover") editor.recover()
        if (action === "close") { allowClose = true; close() }
    }
    function guard(action) {
        pendingAction = action
        if (editor.dirty) discardDialog.open()
        else dispatch(action)
    }
    function saveProject() { if (editor.savedBefore()) editor.save(); else saveDialog.open() }
    onClosing: function(event) {
        if (editor.exporting) { event.accepted = false; renderCloseDialog.open(); return }
        if (!allowClose && editor.dirty) { event.accepted = false; guard("close") }
    }
    Shortcut { sequence: "Ctrl+S"; onActivated: window.saveProject() }
    Shortcut { sequence: "Ctrl+O"; onActivated: window.guard("open") }
    Shortcut { sequence: "Ctrl+Z"; onActivated: editor.undo() }
    Shortcut { sequence: "Ctrl+Shift+Z"; onActivated: editor.redo() }
    FileDialog {
        id: openDialog; title: "Abrir projeto"; nameFilters: ["Projeto Studio Dark (*.studio.json)", "JSON (*.json)"]
        onAccepted: editor.open(selectedFile)
    }
    FileDialog {
        id: saveDialog; title: "Salvar projeto com um nome novo"; fileMode: FileDialog.SaveFile
        defaultSuffix: "studio.json"; nameFilters: ["Projeto Studio Dark (*.studio.json)"]
        onAccepted: editor.save(selectedFile)
    }
    FileDialog {
        id: imageDialog; title: "Importar imagem"; nameFilters: ["Imagens (*.png *.jpg *.jpeg *.webp *.bmp)"]
        onAccepted: editor.addNode("image", selectedFile)
    }
    FileDialog {
        id: exportDialog; title: "Exportar MP4 — escolha um nome novo"; fileMode: FileDialog.SaveFile
        defaultSuffix: "mp4"; nameFilters: ["Vídeo MP4 (*.mp4)"]
        onAccepted: editor.exportVideo(selectedFile)
    }
    Dialog {
        id: discardDialog; anchors.centerIn: parent; modal: true; title: "Há alterações sem salvar"
        ColumnLayout {
            spacing: 18
            Label { text: "Salve o projeto antes de continuar se quiser preservar esta versão."; color: "#dce3e8" }
            RowLayout {
                Layout.alignment: Qt.AlignRight
                StudioButton { text: "Cancelar"; onClicked: discardDialog.close() }
                StudioButton { text: "Continuar sem salvar"; onClicked: { discardDialog.close(); window.dispatch(window.pendingAction) } }
            }
        }
    }
    Dialog {
        id: renderCloseDialog; anchors.centerIn: parent; modal: true; title: "Exportação em andamento"
        standardButtons: Dialog.Ok
        Label { text: "Aguarde a conclusão ou cancele a exportação antes de fechar."; color: "#dce3e8" }
    }
    ColumnLayout {
        anchors.fill: parent; spacing: 0
        Rectangle {
            Layout.fillWidth: true; Layout.preferredHeight: 64; color: "#121a21"
            RowLayout {
                anchors.fill: parent; anchors.leftMargin: 22; anchors.rightMargin: 22; spacing: 12
                Rectangle {
                    width: 30; height: 30; radius: 8; color: "#b4f078"
                    Text { anchors.centerIn: parent; text: "S"; color: "#16210e"; font.bold: true; font.pixelSize: 20 }
                }
                Label { text: "STUDIO DARK"; font.pixelSize: 14; font.bold: true; font.letterSpacing: 2; color: "#eef4f5" }
                Label { text: " /   LAB 01"; font.pixelSize: 11; color: "#7b8996"; Layout.rightMargin: 22 }
                StudioButton { text: "Novo"; quiet: true; onClicked: window.guard("new") }
                StudioButton { text: "Abrir"; quiet: true; onClicked: window.guard("open") }
                StudioButton { text: "Salvar"; onClicked: window.saveProject() }
                StudioButton { text: "↶"; enabled: editor.canUndo; onClicked: editor.undo(); ToolTip.text: "Desfazer · Ctrl+Z"; ToolTip.visible: hovered }
                StudioButton { text: "↷"; enabled: editor.canRedo; onClicked: editor.redo(); ToolTip.text: "Refazer · Ctrl+Shift+Z"; ToolTip.visible: hovered }
                Item { Layout.fillWidth: true }
                Label { text: "C++  /  Qt Quick"; color: "#738392"; font.pixelSize: 11 }
                StudioButton { text: editor.exporting ? "Exportando " + editor.exportProgress + "%" : "Exportar MP4  ↗"; accent: true; enabled: !editor.exporting; onClicked: exportDialog.open() }
                StudioButton { text: "Cancelar"; visible: editor.exporting; onClicked: editor.cancelExport() }
            }
        }
        Rectangle { Layout.fillWidth: true; height: 1; color: "#2a333c" }
        RowLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; spacing: 1
            LibraryPanel { Layout.preferredWidth: 226; Layout.fillHeight: true; onImportImage: imageDialog.open(); onLoadDemo: window.guard("demo"); onRecover: window.guard("recover") }
            PreviewPanel { Layout.fillWidth: true; Layout.fillHeight: true }
            InspectorPanel { Layout.preferredWidth: 268; Layout.fillHeight: true }
        }
        TimelinePanel { Layout.fillWidth: true; Layout.preferredHeight: 252 }
        Rectangle {
            Layout.fillWidth: true; Layout.preferredHeight: 30; color: "#121a21"
            RowLayout {
                anchors.fill: parent; anchors.leftMargin: 16; anchors.rightMargin: 16
                Rectangle { width: 5; height: 5; radius: 3; color: editor.dirty ? "#d4ac6e" : "#b4f078" }
                Label { text: editor.message; color: "#a0aeba"; font.pixelSize: 11; elide: Text.ElideRight; Layout.fillWidth: true }
                Label { text: "PROTÓTIPO 0.1  ·  COMPOSIÇÃO 2D"; color: "#667583"; font.pixelSize: 10; font.letterSpacing: 1 }
            }
        }
    }
}
