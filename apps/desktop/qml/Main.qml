import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

ApplicationWindow {
    id: window
    width: 1440; height: 920
    minimumWidth: 1150; minimumHeight: 760
    visible: true
    title: workspace.page === "projects" ? "Projetos — Studio Dark" : workspace.page === "library" ? "Biblioteca — Studio Dark" : editor.title + (editor.dirty ? " *" : "") + " — Studio Dark"
    color: "#0e1319"
    font.family: "Segoe UI"
    palette.window: "#151c23"
    palette.text: "#dce3e8"
    palette.buttonText: "#dce3e8"
    palette.base: "#10171e"
    palette.button: "#24343f"
    palette.highlight: "#b4f078"
    palette.highlightedText: "#14200e"
    property string pendingAction: ""
    property string pendingProjectId: ""
    property bool allowClose: false
    function dispatch(action) {
        if (action === "new") createDialog.open()
        if (action === "demo") { editor.newProject(true); workspace.navigate("editor") }
        if (action === "open") openDialog.open()
        if (action === "stored-open") workspace.openProject(pendingProjectId)
        if (action === "recover") { editor.recover(); workspace.navigate("editor") }
        if (action === "production") productionDialog.open()
        if (action === "car-example") { editor.importCarExample(); workspace.navigate("editor") }
        if (action === "latest") editor.open(editor.latestProduction)
        if (action === "close") { allowClose = true; close() }
    }
    function guard(action) {
        pendingAction = action
        if (editor.dirty) discardDialog.open()
        else dispatch(action)
    }
    function saveProject() { if (editor.savedBefore()) editor.save(); else saveDialog.open() }
    onClosing: function(event) {
        if (editor.exporting || editor.producing) { event.accepted = false; taskDialog.open(); return }
        if (!allowClose && editor.dirty) { event.accepted = false; guard("close") }
    }
    Shortcut { sequence: "Ctrl+N"; onActivated: window.guard("new") }
    Shortcut { sequence: "Ctrl+S"; enabled: workspace.page === "editor"; onActivated: window.saveProject() }
    Shortcut { sequence: "Ctrl+O"; onActivated: window.guard("open") }
    Shortcut { sequence: "Ctrl+Z"; enabled: workspace.page === "editor"; onActivated: editor.undo() }
    Shortcut { sequence: "Ctrl+Shift+Z"; enabled: workspace.page === "editor"; onActivated: editor.redo() }
    FileDialog {
        id: productionDialog; title: "Importar plano de produção da IA"; nameFilters: ["Produção IA (*.production.json)", "JSON (*.json)"]
        onAccepted: { editor.importProduction(selectedFile); workspace.navigate("editor") }
    }
    FileDialog {
        id: guideDialog; title: "Salvar guia para enviar à IA"; fileMode: FileDialog.SaveFile
        defaultSuffix: "md"; nameFilters: ["Guia Markdown (*.md)"]
        onAccepted: editor.saveAiGuide(selectedFile)
    }
    FileDialog {
        id: openDialog; title: "Importar ou abrir projeto"; nameFilters: ["Projeto Studio Dark (*.studio.json)", "JSON (*.json)"]
        onAccepted: workspace.importProject(selectedFile)
    }
    FileDialog {
        id: saveDialog; title: "Salvar projeto com um nome novo"; fileMode: FileDialog.SaveFile
        defaultSuffix: "studio.json"; nameFilters: ["Projeto Studio Dark (*.studio.json)"]
        onAccepted: editor.save(selectedFile)
    }
    FileDialog {
        id: imageDialog; title: "Importar imagem ou vetor"; nameFilters: ["Imagens e vetores (*.png *.jpg *.jpeg *.webp *.bmp *.svg)"]
        onAccepted: editor.addNode(selectedFile.toString().toLowerCase().endsWith(".svg") ? "svg" : "image", selectedFile)
    }
    FileDialog {
        id: exportDialog; title: "Exportar MP4 — escolha um nome novo"; fileMode: FileDialog.SaveFile
        defaultSuffix: "mp4"; nameFilters: ["Vídeo MP4 (*.mp4)"]
        onAccepted: editor.exportVideo(selectedFile)
    }
    Dialog {
        id: createDialog; anchors.centerIn: parent; width: 470; modal: true; title: "Uma nova história"
        onOpened: { projectName.text = "Novo projeto"; projectName.forceActiveFocus(); projectName.selectAll() }
        ColumnLayout {
            width: parent.width; spacing: 16
            Label { text: "Dê um nome ao seu projeto. Ele será salvo automaticamente na sua coleção."; color: "#a4b9c7"; wrapMode: Text.WordWrap; Layout.fillWidth: true }
            StudioField { id: projectName; Layout.fillWidth: true; maximumLength: 120; onAccepted: createButton.clicked() }
            Label { text: "Composição inicial · 1280 × 720 · 30 fps"; color: "#718d9e"; font.pixelSize: 11 }
            RowLayout {
                Layout.alignment: Qt.AlignRight
                StudioButton { text: "Cancelar"; onClicked: createDialog.close() }
                StudioButton { id: createButton; text: "Criar e abrir"; accent: true; onClicked: if (workspace.createProject(projectName.text)) createDialog.close() }
            }
        }
    }
    Dialog {
        id: discardDialog; anchors.centerIn: parent; width: 490; modal: true; title: "Há alterações sem salvar"
        ColumnLayout {
            width: parent.width; spacing: 18
            Label { text: "Salve o projeto antes de continuar se quiser preservar esta versão."; color: "#dce3e8"; wrapMode: Text.WordWrap; Layout.fillWidth: true }
            RowLayout {
                Layout.alignment: Qt.AlignRight
                StudioButton { text: "Cancelar"; onClicked: discardDialog.close() }
                StudioButton { text: "Continuar sem salvar"; onClicked: { discardDialog.close(); window.dispatch(window.pendingAction) } }
            }
        }
    }
    Dialog {
        id: taskDialog; anchors.centerIn: parent; modal: true; title: "Uma tarefa está em andamento"
        standardButtons: Dialog.Ok
        Label { text: "Aguarde a conclusão ou cancele a tarefa antes de fechar."; color: "#dce3e8" }
    }
    ColumnLayout {
        anchors.fill: parent; spacing: 0
        Rectangle {
            Layout.fillWidth: true; Layout.preferredHeight: 66; color: "#121c24"
            RowLayout {
                anchors.fill: parent; anchors.leftMargin: 24; anchors.rightMargin: 24; spacing: 12
                Rectangle {
                    width: 30; height: 30; radius: 8; color: "#b4f078"
                    Text { anchors.centerIn: parent; text: "S"; color: "#16210e"; font.bold: true; font.pixelSize: 20 }
                }
                Label { text: "STUDIO DARK"; font.pixelSize: 14; font.bold: true; font.letterSpacing: 2; color: "#eef4f5"; Layout.rightMargin: 25 }
                StudioButton { text: "Projetos"; accent: workspace.page === "projects"; quiet: workspace.page !== "projects"; onClicked: workspace.navigate("projects") }
                StudioButton { text: "Biblioteca completa"; accent: workspace.page === "library"; quiet: workspace.page !== "library"; onClicked: workspace.navigate("library") }
                StudioButton { text: "Voltar ao editor"; visible: workspace.hasDocument && workspace.page !== "editor"; quiet: true; onClicked: workspace.navigate("editor") }
                Item { Layout.fillWidth: true }
                Label { visible: editor.producing || editor.exporting; text: editor.producing ? "Montando " + editor.productionProgress + "%" : "Exportando " + editor.exportProgress + "%"; color: "#b4f078"; font.pixelSize: 12 }
                StudioButton { text: "Cancelar tarefa"; visible: editor.producing || editor.exporting; onClicked: editor.producing ? editor.cancelProduction() : editor.cancelExport() }
                StudioButton { text: "Guia para IA"; quiet: true; onClicked: guideDialog.open() }
                Label { text: "TUDO PRONTO PARA CRIAR"; visible: !editor.producing && !editor.exporting && workspace.page !== "editor"; color: "#6b8493"; font.pixelSize: 10; font.letterSpacing: 1 }
            }
        }
        Rectangle { Layout.fillWidth: true; height: 1; color: "#2b3b47" }
        StackLayout {
            Layout.fillWidth: true; Layout.fillHeight: true
            currentIndex: workspace.page === "projects" ? 0 : workspace.page === "library" ? 1 : 2
            HomePage {
                onCreateRequested: window.guard("new")
                onImportRequested: window.guard("open")
                onProductionRequested: window.guard("production")
                onExampleRequested: window.guard("car-example")
                onOpenRequested: function(projectId) { window.pendingProjectId = projectId; window.guard("stored-open") }
            }
            AssetLibraryPage {}
            ColumnLayout {
                spacing: 0
                Rectangle {
                    Layout.fillWidth: true; Layout.preferredHeight: 48; color: "#16222c"
                    RowLayout {
                        anchors.fill: parent; anchors.leftMargin: 18; anchors.rightMargin: 18; spacing: 9
                        StudioButton { text: "Novo"; quiet: true; onClicked: window.guard("new") }
                        StudioButton { text: "Abrir"; quiet: true; onClicked: window.guard("open") }
                        StudioButton { text: "Salvar"; onClicked: window.saveProject() }
                        StudioButton { text: "↶"; enabled: editor.canUndo; onClicked: editor.undo(); ToolTip.text: "Desfazer · Ctrl+Z"; ToolTip.visible: hovered }
                        StudioButton { text: "↷"; enabled: editor.canRedo; onClicked: editor.redo(); ToolTip.text: "Refazer · Ctrl+Shift+Z"; ToolTip.visible: hovered }
                        StudioButton { text: "Salvar objeto na biblioteca"; enabled: workspace.canSaveSelection; quiet: true; onClicked: workspace.navigate("library") }
                        Item { Layout.fillWidth: true }
                        StudioButton { text: "Importar produção IA"; enabled: !editor.producing && !editor.exporting; onClicked: window.guard("production") }
                        StudioButton { text: "Exportar MP4  ↗"; accent: true; enabled: !editor.exporting && !editor.producing; onClicked: exportDialog.open() }
                    }
                }
                RowLayout {
                    Layout.fillWidth: true; Layout.fillHeight: true; spacing: 1
                    LibraryPanel { Layout.preferredWidth: 226; Layout.fillHeight: true; onImportImage: imageDialog.open(); onLoadDemo: window.guard("demo"); onRecover: window.guard("recover"); onCarExample: window.guard("car-example"); onLatest: window.guard("latest") }
                    PreviewPanel { Layout.fillWidth: true; Layout.fillHeight: true }
                    InspectorPanel { Layout.preferredWidth: 268; Layout.fillHeight: true }
                }
                TimelinePanel { Layout.fillWidth: true; Layout.preferredHeight: 235 }
            }
        }
        Rectangle {
            Layout.fillWidth: true; Layout.preferredHeight: 30; color: "#121c24"
            RowLayout {
                anchors.fill: parent; anchors.leftMargin: 18; anchors.rightMargin: 18
                Rectangle { width: 5; height: 5; radius: 3; color: "#b4f078" }
                Label { text: workspace.page === "editor" || editor.producing || editor.exporting ? editor.message : workspace.message; color: "#9eb4c3"; font.pixelSize: 11; elide: Text.ElideRight; Layout.fillWidth: true }
                Label { text: "SEU ESTÚDIO LOCAL"; color: "#5e798a"; font.pixelSize: 10; font.letterSpacing: 1 }
            }
        }
    }
}
