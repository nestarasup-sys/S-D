import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Rectangle {
    id: page
    color: "#0e151d"
    property var selectedAsset: ({})
    property string categoryAction: "create"
    property string editedCategoryId: ""
    property url pendingFile
    property string itemAction: "import"
    function categoryIndex(id) { return Math.max(0, workspace.categories.findIndex(function(c) { return c.id === id })) }
    function categoryName(id) { const c = workspace.categories.find(function(c) { return c.id === id }); return c ? c.name : "Categoria" }
    FileDialog {
        id: mediaDialog; title: "Adicionar imagem ou vetor à biblioteca"
        nameFilters: ["Imagens e vetores (*.png *.jpg *.jpeg *.webp *.bmp *.svg)"]
        onAccepted: { pendingFile = selectedFile; itemAction = "import"; selectedAsset = ({}); itemDialog.open() }
    }
    Dialog {
        id: itemDialog; anchors.centerIn: parent; width: 470; modal: true
        title: itemAction === "edit" ? "Organizar item" : itemAction === "preset" ? "Salvar composição reutilizável" : "Adicionar à biblioteca"
        onOpened: {
            itemName.text = itemAction === "edit" ? selectedAsset.name : itemAction === "preset" ? (editor.selected.name || "Meu objeto") : workspace.suggestedAssetName(pendingFile)
            itemCategory.currentIndex = categoryIndex(itemAction === "edit" ? selectedAsset.categoryId : (workspace.categoryFilter || "uncategorized"))
            itemName.forceActiveFocus(); itemName.selectAll()
        }
        ColumnLayout {
            width: parent.width; spacing: 12
            Label { text: "Nome"; color: "#9cb0bf"; font.pixelSize: 12 }
            StudioField { id: itemName; Layout.fillWidth: true; maximumLength: 120 }
            Label { text: "Categoria"; color: "#9cb0bf"; font.pixelSize: 12 }
            StudioComboBox { id: itemCategory; Layout.fillWidth: true; model: workspace.categories; textRole: "name"; valueRole: "id" }
            Label { text: itemAction === "import" ? "Uma cópia do arquivo será guardada na biblioteca." : itemAction === "preset" ? "Grupos, filhos, keyframes e mídias serão preservados no preset." : "Mudar a categoria não altera projetos já criados."; color: "#7f97a8"; wrapMode: Text.WordWrap; Layout.fillWidth: true; font.pixelSize: 11 }
            RowLayout {
                Layout.alignment: Qt.AlignRight; Layout.topMargin: 8
                StudioButton { text: "Cancelar"; onClicked: itemDialog.close() }
                StudioButton {
                    text: "Salvar"; accent: true
                    onClicked: {
                        let ok = false
                        if (itemAction === "edit") ok = workspace.updateAsset(selectedAsset.id, itemName.text, itemCategory.currentValue)
                        else if (itemAction === "preset") ok = workspace.saveSelection(itemName.text, itemCategory.currentValue)
                        else ok = workspace.addAsset(pendingFile, itemName.text, itemCategory.currentValue)
                        if (ok) itemDialog.close()
                    }
                }
            }
        }
    }
    Dialog {
        id: categoryDialog; anchors.centerIn: parent; width: 440; modal: true
        title: categoryAction === "create" ? "Nova categoria" : "Renomear categoria"
        onOpened: { categoryInput.text = categoryAction === "create" ? "" : categoryName(editedCategoryId); categoryInput.forceActiveFocus(); categoryInput.selectAll() }
        ColumnLayout {
            width: parent.width; spacing: 18
            StudioField { id: categoryInput; Layout.fillWidth: true; placeholderText: "Ex.: Meu canal, veículos, personagens…"; maximumLength: 120 }
            RowLayout {
                Layout.alignment: Qt.AlignRight
                StudioButton { text: "Cancelar"; onClicked: categoryDialog.close() }
                StudioButton { text: "Salvar categoria"; accent: true; onClicked: if (categoryAction === "create" ? workspace.addCategory(categoryInput.text) : workspace.renameCategory(editedCategoryId, categoryInput.text)) categoryDialog.close() }
            }
        }
    }
    Dialog {
        id: deleteCategoryDialog; anchors.centerIn: parent; width: 460; modal: true; title: "Excluir categoria?"
        ColumnLayout {
            width: parent.width; spacing: 18
            Label { text: "Os itens de “" + categoryName(editedCategoryId) + "” serão movidos para Sem categoria. Nenhum item será excluído."; textFormat: Text.PlainText; wrapMode: Text.WordWrap; Layout.fillWidth: true; color: "#b5c7d3" }
            RowLayout { Layout.alignment: Qt.AlignRight
                StudioButton { text: "Cancelar"; onClicked: deleteCategoryDialog.close() }
                StudioButton { text: "Excluir categoria"; danger: true; onClicked: if (workspace.deleteCategory(editedCategoryId)) deleteCategoryDialog.close() }
            }
        }
    }
    Dialog {
        id: deleteItemDialog; anchors.centerIn: parent; width: 460; modal: true; title: "Remover da biblioteca?"
        ColumnLayout {
            width: parent.width; spacing: 18
            Label { text: selectedAsset.name || ""; textFormat: Text.PlainText; color: "#e4eef1"; font.pixelSize: 17; wrapMode: Text.WordWrap; Layout.fillWidth: true }
            Label { text: "O item poderá ser restaurado em Excluídos. Elementos já usados em projetos e seus arquivos serão preservados."; color: "#91a7b7"; wrapMode: Text.WordWrap; Layout.fillWidth: true }
            RowLayout { Layout.alignment: Qt.AlignRight
                StudioButton { text: "Cancelar"; onClicked: deleteItemDialog.close() }
                StudioButton { text: "Remover item"; danger: true; onClicked: if (workspace.deleteAsset(selectedAsset.id)) deleteItemDialog.close() }
            }
        }
    }
    RowLayout {
        anchors.fill: parent; spacing: 0
        Rectangle {
            Layout.preferredWidth: 232; Layout.fillHeight: true; color: "#141f28"
            ColumnLayout {
                anchors.fill: parent; anchors.margins: 20; spacing: 14
                Label { text: "CATEGORIAS"; color: "#829aa9"; font.pixelSize: 10; font.letterSpacing: 2 }
                StudioButton { text: "Todos os itens  ·  " + workspace.assetCount; Layout.fillWidth: true; accent: workspace.categoryFilter === ""; onClicked: workspace.categoryFilter = "" }
                ListView {
                    Layout.fillWidth: true; Layout.fillHeight: true; clip: true; spacing: 7; model: workspace.categories
                    delegate: StudioButton {
                        required property var modelData
                        width: ListView.view.width
                        text: modelData.name + "   ·   " + modelData.count
                        accent: workspace.categoryFilter === modelData.id
                        quiet: workspace.categoryFilter !== modelData.id
                        onClicked: workspace.categoryFilter = modelData.id
                    }
                }
                StudioButton { text: "+  Nova categoria"; Layout.fillWidth: true; onClicked: { categoryAction = "create"; categoryDialog.open() } }
                StudioButton { text: "Renomear categoria"; Layout.fillWidth: true; quiet: true; enabled: workspace.categoryFilter !== ""; onClicked: { categoryAction = "rename"; editedCategoryId = workspace.categoryFilter; categoryDialog.open() } }
                StudioButton { text: "Excluir categoria"; Layout.fillWidth: true; quiet: true; enabled: workspace.categoryFilter !== "" && workspace.categoryFilter !== "uncategorized"; onClicked: { editedCategoryId = workspace.categoryFilter; deleteCategoryDialog.open() } }
                Rectangle { Layout.fillWidth: true; height: 1; color: "#2d414e" }
                StudioButton { text: workspace.assetTrash ? "← Itens ativos" : "Excluídos / restaurar"; Layout.fillWidth: true; onClicked: workspace.assetTrash = !workspace.assetTrash }
            }
        }
        ColumnLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; Layout.margins: 28; spacing: 18
            RowLayout {
                Layout.fillWidth: true
                ColumnLayout { spacing: 6
                    Label { text: workspace.assetTrash ? "Itens excluídos" : "Sua biblioteca criativa"; color: "#edf3f5"; font.pixelSize: 29; font.weight: Font.DemiBold }
                    Label { text: "Objetos, cenários, personagens, imagens e as suas próprias variações."; color: "#819aaa"; font.pixelSize: 12 }
                }
                Item { Layout.fillWidth: true }
                StudioButton { text: "+  Adicionar arquivo"; accent: true; implicitHeight: 40; onClicked: mediaDialog.open() }
            }
            RowLayout {
                Layout.fillWidth: true; spacing: 12
                StudioField { Layout.fillWidth: true; placeholderText: "Buscar na biblioteca…"; text: workspace.assetSearch; onTextEdited: workspace.assetSearch = text }
                StudioButton { text: "Salvar seleção do editor"; enabled: workspace.canSaveSelection; onClicked: { itemAction = "preset"; itemDialog.open() } ToolTip.text: "Selecione um objeto ou grupo raiz para salvar a composição"; ToolTip.visible: hovered }
                StudioButton { text: "Atualizar"; quiet: true; onClicked: workspace.refresh() }
            }
            Label { visible: !workspace.hasDocument; text: "Para usar um item, crie ou abra um projeto na tela Projetos."; color: "#a4c2b1"; font.pixelSize: 12 }
            Item {
                Layout.fillWidth: true; Layout.fillHeight: true
                GridView {
                    id: assetGrid; anchors.fill: parent; clip: true
                    cellWidth: width / Math.max(1, Math.floor(width / 265)); cellHeight: 280
                    model: workspace.assets
                    ScrollBar.vertical: ScrollBar {}
                    delegate: Item {
                        required property var modelData
                        width: assetGrid.cellWidth; height: assetGrid.cellHeight
                        Rectangle {
                            anchors.fill: parent; anchors.rightMargin: 14; anchors.bottomMargin: 14
                            color: "#17232d"; radius: 9; border.color: "#2b414e"
                            ColumnLayout {
                                anchors.fill: parent; anchors.margins: 12; spacing: 8
                                Rectangle {
                                    Layout.fillWidth: true; Layout.preferredHeight: 140; radius: 5; color: "#102028"; clip: true
                                    Image { anchors.fill: parent; anchors.margins: 4; source: modelData.thumbnail; fillMode: Image.PreserveAspectFit; autoTransform: true; sourceSize.width: 480; sourceSize.height: 270 }
                                    Label { anchors.left: parent.left; anchors.top: parent.top; anchors.margins: 7; text: modelData.typeLabel; color: "#b0c7cb"; font.pixelSize: 9; font.letterSpacing: 1 }
                                }
                                Label { text: modelData.name; textFormat: Text.PlainText; Layout.fillWidth: true; color: "#e2edf0"; elide: Text.ElideRight; font.pixelSize: 16; font.weight: Font.DemiBold }
                                Label { text: modelData.categoryName; textFormat: Text.PlainText; color: "#809aaa"; font.pixelSize: 10 }
                                RowLayout {
                                    Layout.fillWidth: true; spacing: 5
                                    StudioButton { text: modelData.deleted ? "Restaurar" : "Usar"; Layout.fillWidth: true; accent: true; enabled: modelData.deleted || (workspace.hasDocument && !modelData.missing); onClicked: modelData.deleted ? workspace.restoreAsset(modelData.id) : workspace.useAsset(modelData.id); ToolTip.text: "Adicionar ao projeto e abrir no editor"; ToolTip.visible: hovered }
                                    StudioButton { text: "Organizar"; visible: !modelData.deleted; quiet: true; onClicked: { selectedAsset = modelData; itemAction = "edit"; itemDialog.open() } }
                                    StudioButton { text: "Excluir"; visible: !modelData.deleted; danger: true; onClicked: { selectedAsset = modelData; deleteItemDialog.open() } }
                                }
                            }
                        }
                    }
                }
                ColumnLayout {
                    anchors.centerIn: parent; visible: assetGrid.count === 0; spacing: 10
                    Label { text: workspace.assetTrash ? "Nenhum item excluído nesta categoria." : "Uma categoria esperando suas ideias."; color: "#c2d3dd"; font.pixelSize: 21; Layout.alignment: Qt.AlignHCenter }
                    Label { text: workspace.assetSearch ? "Tente outro nome na busca." : "Adicione um arquivo ou salve uma seleção do editor."; color: "#7f97a9"; font.pixelSize: 12; Layout.alignment: Qt.AlignHCenter }
                    StudioButton { visible: !workspace.assetTrash; text: "+  Adicionar arquivo"; Layout.alignment: Qt.AlignHCenter; onClicked: mediaDialog.open() }
                }
            }
        }
    }
}
