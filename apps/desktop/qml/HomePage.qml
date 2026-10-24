import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    color: "#0e151d"
    signal createRequested()
    signal importRequested()
    signal productionRequested()
    signal exampleRequested()
    signal openRequested(string projectId)
    property var selectedProject: ({})
    Dialog {
        id: renameDialog; anchors.centerIn: parent; width: 440; modal: true; title: "Renomear projeto"
        onOpened: { renameInput.text = selectedProject.name; renameInput.forceActiveFocus(); renameInput.selectAll() }
        ColumnLayout {
            width: parent.width; spacing: 18
            StudioField { id: renameInput; Layout.fillWidth: true; maximumLength: 120; onAccepted: renameButton.clicked() }
            RowLayout {
                Layout.alignment: Qt.AlignRight
                StudioButton { text: "Cancelar"; onClicked: renameDialog.close() }
                StudioButton { id: renameButton; text: "Salvar nome"; accent: true; onClicked: if (workspace.renameProject(selectedProject.id, renameInput.text)) renameDialog.close() }
            }
        }
    }
    Dialog {
        id: deleteDialog; anchors.centerIn: parent; width: 460; modal: true; title: "Excluir projeto?"
        ColumnLayout {
            width: parent.width; spacing: 18
            Label { text: selectedProject.name || ""; textFormat: Text.PlainText; color: "#e3eaf0"; font.pixelSize: 17; wrapMode: Text.WordWrap; Layout.fillWidth: true }
            Label { text: "O projeto vai para a lixeira. Seus arquivos serão preservados e você poderá restaurá-lo."; color: "#97abb9"; wrapMode: Text.WordWrap; Layout.fillWidth: true }
            RowLayout {
                Layout.alignment: Qt.AlignRight
                StudioButton { text: "Cancelar"; onClicked: deleteDialog.close() }
                StudioButton { text: "Mover para lixeira"; danger: true; onClicked: if (workspace.deleteProject(selectedProject.id)) deleteDialog.close() }
            }
        }
    }
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 32; spacing: 22
        RowLayout {
            Layout.fillWidth: true; spacing: 22
            ColumnLayout {
                spacing: 8
                Label { text: "SEU ESTÚDIO, SUAS HISTÓRIAS"; color: "#8da5b4"; font.pixelSize: 10; font.letterSpacing: 2 }
                Label { text: "Dê espaço à próxima ideia."; color: "#edf3f4"; font.pixelSize: 32; font.weight: Font.DemiBold }
                Label { text: "Seus projetos e seus elementos, organizados em um só lugar."; color: "#8fa3b2"; font.pixelSize: 13 }
            }
            Item { Layout.fillWidth: true }
            StudioButton { text: "+  Novo projeto"; accent: true; implicitHeight: 42; onClicked: createRequested() }
            StudioButton { text: "Importar projeto"; implicitHeight: 42; onClicked: importRequested() }
        }
        RowLayout {
            Layout.fillWidth: true; spacing: 16
            Repeater {
                model: [ { title: "PROJETOS ATIVOS", value: workspace.projectCount, note: "Continue de onde parou", action: "projects" }, { title: "ELEMENTOS NA BIBLIOTECA", value: workspace.assetCount, note: "Objetos, cenários e imagens", action: "library" }, { title: "CATEGORIAS", value: workspace.categories.length, note: "Organize do seu jeito", action: "library" } ]
                Rectangle {
                    required property var modelData
                    Layout.fillWidth: true; implicitHeight: 104; radius: 10; color: "#17232d"; border.color: "#283a47"
                    ColumnLayout {
                        anchors.fill: parent; anchors.margins: 17; spacing: 6
                        Label { text: modelData.title; color: "#839aaa"; font.pixelSize: 10; font.letterSpacing: 1 }
                        RowLayout {
                            Layout.fillWidth: true
                            Label { text: String(modelData.value); color: "#b4f078"; font.pixelSize: 30; font.weight: Font.DemiBold }
                            Item { Layout.fillWidth: true }
                            Label { text: modelData.note; color: "#8da3b3"; font.pixelSize: 11 }
                        }
                    }
                    MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: workspace.navigate(modelData.action) }
                }
            }
        }
        RowLayout {
            Layout.fillWidth: true; spacing: 12
            Label { text: workspace.projectTrash ? "Lixeira de projetos" : "Seus projetos"; color: "#e3edf1"; font.pixelSize: 21; font.weight: Font.DemiBold }
            Item { Layout.fillWidth: true }
            StudioField { Layout.preferredWidth: 260; placeholderText: "Buscar projeto…"; text: workspace.projectSearch; onTextEdited: workspace.projectSearch = text }
            StudioButton { text: workspace.projectTrash ? "← Projetos ativos" : "Lixeira"; quiet: true; onClicked: workspace.projectTrash = !workspace.projectTrash }
            StudioButton { text: "Atualizar"; quiet: true; onClicked: workspace.refresh() }
        }
        Item {
            Layout.fillWidth: true; Layout.fillHeight: true
            GridView {
                id: projectGrid; anchors.fill: parent; clip: true
                cellWidth: width / Math.max(1, Math.floor(width / 340)); cellHeight: 304
                model: workspace.projects
                ScrollBar.vertical: ScrollBar {}
                delegate: Item {
                    required property var modelData
                    width: projectGrid.cellWidth; height: projectGrid.cellHeight
                    Rectangle {
                        anchors.fill: parent; anchors.rightMargin: 16; anchors.bottomMargin: 16
                        color: "#17232c"; radius: 10; border.color: "#2d414e"
                        ColumnLayout {
                            anchors.fill: parent; anchors.margins: 12; spacing: 9
                            Rectangle {
                                Layout.fillWidth: true; Layout.preferredHeight: 142; color: "#102027"; radius: 6; clip: true
                                Image { anchors.fill: parent; source: modelData.thumbnail; fillMode: Image.PreserveAspectCrop; cache: false; sourceSize.width: 480; sourceSize.height: 270 }
                                Label { anchors.centerIn: parent; text: "STUDIO DARK"; visible: !modelData.thumbnail; font.letterSpacing: 4; color: "#638e8b"; font.pixelSize: 17 }
                                Rectangle {
                                    anchors.left: parent.left; anchors.top: parent.top; anchors.margins: 9; height: 23; width: coverBadge.implicitWidth + 16; color: "#ce13222c"; radius: 4
                                    Label { id: coverBadge; anchors.centerIn: parent; text: modelData.deleted ? "NA LIXEIRA" : modelData.missing ? "ARQUIVO AUSENTE" : "PROJETO LOCAL"; color: modelData.missing ? "#efb29a" : "#bfd5dc"; font.pixelSize: 9; font.letterSpacing: 1 }
                                }
                            }
                            Label { text: modelData.name; textFormat: Text.PlainText; elide: Text.ElideRight; Layout.fillWidth: true; color: "#e1edf0"; font.pixelSize: 17; font.weight: Font.DemiBold }
                            Label { text: "Atualizado em " + modelData.dateLabel; color: "#8097a7"; font.pixelSize: 10 }
                            Item { Layout.fillHeight: true }
                            RowLayout {
                                Layout.fillWidth: true; spacing: 6
                                StudioButton { text: modelData.deleted ? "Restaurar" : "Abrir"; accent: true; Layout.fillWidth: true; enabled: modelData.deleted || !modelData.missing; onClicked: modelData.deleted ? workspace.restoreProject(modelData.id) : openRequested(modelData.id) }
                                StudioButton { text: "Duplicar"; visible: !modelData.deleted; quiet: true; onClicked: workspace.duplicateProject(modelData.id) }
                                StudioButton { text: "Renomear"; visible: !modelData.deleted; quiet: true; onClicked: { selectedProject = modelData; renameDialog.open() } }
                                StudioButton { text: "Excluir"; visible: !modelData.deleted; danger: true; onClicked: { selectedProject = modelData; deleteDialog.open() } }
                            }
                        }
                    }
                }
            }
            ColumnLayout {
                anchors.centerIn: parent; visible: projectGrid.count === 0; spacing: 16
                Label { text: workspace.projectTrash ? "Nada por aqui. Ainda bem." : workspace.projectSearch ? "Nenhum projeto encontrado." : "Toda história começa com uma ideia."; color: "#c6d5de"; font.pixelSize: 22; Layout.alignment: Qt.AlignHCenter }
                Label { text: workspace.projectTrash ? "Projetos excluídos aparecerão aqui para restauração." : "Crie uma composição vazia ou comece com o exemplo narrado."; color: "#7d97a8"; font.pixelSize: 12; Layout.alignment: Qt.AlignHCenter }
                RowLayout {
                    visible: !workspace.projectTrash; Layout.alignment: Qt.AlignHCenter; spacing: 12
                    StudioButton { text: "+  Criar projeto"; accent: true; onClicked: createRequested() }
                    StudioButton { text: "Testar vídeo de carros"; onClicked: exampleRequested() }
                    StudioButton { text: "Importar produção IA"; onClicked: productionRequested() }
                }
            }
        }
    }
}
