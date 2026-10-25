import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Rectangle {
    id: panel
    color: "#151c23"
    property var node: editor.selected
    FileDialog {
        id: fontDialog
        title: "Adicionar fonte à biblioteca"
        nameFilters: ["Fontes TrueType/OpenType (*.ttf *.otf)"]
        onAccepted: workspace.addFont(selectedFile)
    }
    Dialog {
        id: stylePresetDialog
        anchors.centerIn: parent
        width: 430
        modal: true
        title: "Salvar estilo visual"
        ColumnLayout {
            width: parent.width
            spacing: 14
            StudioField { id: stylePresetName; Layout.fillWidth: true; placeholderText: "Ex.: Noite neon, Jornal…"; maximumLength: 120 }
            RowLayout {
                Layout.alignment: Qt.AlignRight
                StudioButton { text: "Cancelar"; onClicked: stylePresetDialog.close() }
                StudioButton {
                    text: "Salvar estilo"
                    accent: true
                    onClicked: if (workspace.saveVisualStylePreset(stylePresetName.text)) stylePresetDialog.close()
                }
            }
        }
    }
    function activeShot() {
        for (let i = 0; i < editor.shots.length; ++i)
            if (editor.shots[i].id === editor.camera.shotId)
                return editor.shots[i]
        return ({})
    }
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
    component StyleField: ColumnLayout {
        id: styleField
        property string caption
        property string propertyKey
        property var currentValue
        property bool number: false
        spacing: 5
        Layout.fillWidth: true
        Label { text: styleField.caption; color: "#8193a2"; font.pixelSize: 10; elide: Text.ElideRight; Layout.fillWidth: true }
        TextField {
            id: styleInput
            Layout.fillWidth: true; implicitHeight: 32
            Binding { target: styleInput; property: "text"; value: styleField.currentValue === undefined ? "" : String(styleField.currentValue); when: !styleInput.activeFocus; restoreMode: Binding.RestoreNone }
            color: "#dce6ee"; font.pixelSize: 12; selectByMouse: true
            background: Rectangle { color: "#10171e"; radius: 5; border.color: styleInput.activeFocus ? "#b4f078" : "#303e49" }
            onEditingFinished: {
                const value = styleField.number ? Number(text) : text.trim()
                if ((styleField.number && (!text.trim() || !isFinite(value))) || (!styleField.number && !value)) {
                    text = String(styleField.currentValue)
                    return
                }
                editor.updateArtDirection(styleField.propertyKey, value)
                text = String(editor.artDirection[styleField.propertyKey])
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
            Label { text: "ESCOPO DA DIREÇÃO DE ARTE"; color: "#8193a2"; font.pixelSize: 10; font.letterSpacing: 1; Layout.topMargin: 8 }
            StudioComboBox {
                Layout.fillWidth: true
                model: ["global", "scene", "shot"]
                currentIndex: Math.max(0, model.indexOf(editor.artDirectionScope))
                onActivated: editor.setArtDirectionScope(currentText)
            }
            Label {
                text: editor.artDirectionScope === "global" ? "ESTILO GLOBAL" :
                      editor.artDirectionScope === "scene" ? "ESTILO DA CENA ATUAL" : "ESTILO DO SHOT ATUAL"
                color: "#8193a2"; font.pixelSize: 10; font.letterSpacing: 1
            }
            RowLayout {
                StyleField { caption: "Fundo · #RRGGBB"; propertyKey: "backgroundColor"; currentValue: editor.artDirection.backgroundColor }
                StyleField { caption: "Luz · #RRGGBB"; propertyKey: "glowColor"; currentValue: editor.artDirection.glowColor }
            }
            RowLayout {
                StyleField { caption: "Primária · #RRGGBB"; propertyKey: "primaryColor"; currentValue: editor.artDirection.primaryColor }
                StyleField { caption: "Secundária · #RRGGBB"; propertyKey: "secondaryColor"; currentValue: editor.artDirection.secondaryColor }
            }
            RowLayout {
                StyleField { caption: "Texto · #RRGGBB"; propertyKey: "textColor"; currentValue: editor.artDirection.textColor }
                StyleField { caption: "Cartões · #RRGGBB"; propertyKey: "cardColor"; currentValue: editor.artDirection.cardColor }
            }
            RowLayout {
                StyleField { caption: "Legenda · #RRGGBB"; propertyKey: "captionBackgroundColor"; currentValue: editor.artDirection.captionBackgroundColor }
                StyleField { caption: "Fonte instalada"; propertyKey: "fontFamily"; currentValue: editor.artDirection.fontFamily }
            }
            StudioComboBox {
                Layout.fillWidth: true
                model: workspace.fontFamilies
                currentIndex: Math.max(0, model.indexOf(editor.artDirection.fontFamily))
                onActivated: editor.updateArtDirection("fontFamily", currentText)
                ToolTip.text: "Fontes instaladas no sistema e fontes importadas para a biblioteca"
                ToolTip.visible: hovered
            }
            StudioButton {
                text: "+  Importar fonte TTF / OTF"
                Layout.fillWidth: true
                quiet: true
                onClicked: fontDialog.open()
            }
            RowLayout {
                StyleField { caption: "Título · px"; propertyKey: "titleSize"; currentValue: editor.artDirection.titleSize; number: true }
                StyleField { caption: "Texto · px"; propertyKey: "bodySize"; currentValue: editor.artDirection.bodySize; number: true }
            }
            RowLayout {
                StyleField { caption: "Legenda · px"; propertyKey: "captionSize"; currentValue: editor.artDirection.captionSize; number: true }
                StyleField { caption: "Margem · px"; propertyKey: "safeMargin"; currentValue: editor.artDirection.safeMargin; number: true }
            }
            Label { text: "Posição das legendas"; color: "#8193a2"; font.pixelSize: 10 }
            StudioComboBox {
                Layout.fillWidth: true; model: ["bottom", "top"]
                currentIndex: Math.max(0, model.indexOf(editor.artDirection.captionPosition))
                onActivated: editor.updateArtDirection("captionPosition", currentText)
            }
            Label { text: "O estilo local herda os valores do escopo acima. Valores de objetos definidos explicitamente permanecem preservados. Os estilos são salvos no projeto e usados na prévia e na exportação."; color: "#708393"; font.pixelSize: 10; wrapMode: Text.WordWrap; Layout.fillWidth: true }
            Label { text: "PRESETS VISUAIS DA BIBLIOTECA"; color: "#8193a2"; font.pixelSize: 10; font.letterSpacing: 1; Layout.topMargin: 8 }
            StudioComboBox {
                id: visualStylePicker
                Layout.fillWidth: true
                model: workspace.visualStylePresets
                textRole: "name"
                valueRole: "id"
                currentIndex: workspace.visualStylePresets.length > 0 ? 0 : -1
            }
            RowLayout {
                Layout.fillWidth: true
                StudioButton { text: "Salvar estilo atual"; Layout.fillWidth: true; quiet: true; onClicked: stylePresetDialog.open() }
                StudioButton { text: "Aplicar"; Layout.fillWidth: true; accent: true; enabled: !!visualStylePicker.currentValue; onClicked: workspace.useAsset(visualStylePicker.currentValue) }
            }
            Label { text: "O estilo salvo contém paleta, tipografia e parâmetros de legenda; aplicar respeita o escopo escolhido acima."; color: "#708393"; font.pixelSize: 10; wrapMode: Text.WordWrap; Layout.fillWidth: true }
            Label { text: "INTENSIDADE DE EDIÇÃO · CENA ATUAL"; color: "#8193a2"; font.pixelSize: 10; font.letterSpacing: 1; Layout.topMargin: 8 }
            Slider { id: intensitySlider; Layout.fillWidth: true; from: 0; to: 100; stepSize: 1; value: 50 }
            Label { text: Math.round(intensitySlider.value) + " / 100 · " + (intensitySlider.value === 0 ? "sem movimento automático" : intensitySlider.value <= 30 ? "movimento discreto" : intensitySlider.value <= 70 ? "movimento moderado" : "movimento intenso"); color: "#708393"; font.pixelSize: 10 }
            StudioButton {
                text: "Aplicar intensidade aos objetos da cena"
                Layout.fillWidth: true
                quiet: true
                onClicked: editor.applyEditingIntensity(Math.round(intensitySlider.value))
            }
            Label { text: "Esta ação ajusta os movimentos de entrada e saída da cena atual em uma única edição reversível."; color: "#708393"; font.pixelSize: 10; wrapMode: Text.WordWrap; Layout.fillWidth: true }
            Label { text: "TRANSIÇÃO DO SHOT ATUAL"; visible: !!panel.activeShot().id; color: "#8193a2"; font.pixelSize: 10; font.letterSpacing: 1; Layout.topMargin: 8 }
            Label { text: "Shot: " + (panel.activeShot().name || ""); visible: !!panel.activeShot().id; color: "#c5d0d8"; font.pixelSize: 11; elide: Text.ElideRight; Layout.fillWidth: true }
            StudioComboBox {
                id: transitionPicker
                visible: !!panel.activeShot().id
                enabled: panel.activeShot().hasPrevious
                Layout.fillWidth: true
                model: ["cut", "fade", "dissolve", "slide", "zoom"]
                currentIndex: Math.max(0, model.indexOf(panel.activeShot().transitionIn || "cut"))
                onActivated: editor.updateShotTransition(panel.activeShot().id, currentText, panel.activeShot().transitionDurationFrames || 0, panel.activeShot().transitionDirection || "left")
            }
            Label { text: panel.activeShot().hasPrevious ? "Tipo de transição de entrada deste shot." : "O primeiro shot da cena começa sem transição."; visible: !!panel.activeShot().id; color: "#708393"; font.pixelSize: 10; wrapMode: Text.WordWrap; Layout.fillWidth: true }
            RowLayout {
                visible: !!panel.activeShot().id && panel.activeShot().transitionIn !== "cut"
                Layout.fillWidth: true
                Label { text: "Duração · quadros"; color: "#8193a2"; font.pixelSize: 10 }
                TextField {
                    id: transitionDuration
                    Layout.preferredWidth: 82
                    text: String(panel.activeShot().transitionDurationFrames || 12)
                    validator: IntValidator { bottom: 2; top: 300 }
                    color: "#dce6ee"; font.pixelSize: 12; selectByMouse: true
                    background: Rectangle { color: "#10171e"; radius: 5; border.color: transitionDuration.activeFocus ? "#b4f078" : "#303e49" }
                    onEditingFinished: {
                        editor.updateShotTransition(panel.activeShot().id, panel.activeShot().transitionIn, Number(text), panel.activeShot().transitionDirection || "left")
                        text = String(panel.activeShot().transitionDurationFrames || 12)
                    }
                }
            }
            StudioComboBox {
                visible: !!panel.activeShot().id && panel.activeShot().transitionIn === "slide"
                Layout.fillWidth: true
                model: ["left", "right", "up", "down"]
                currentIndex: Math.max(0, model.indexOf(panel.activeShot().transitionDirection || "left"))
                onActivated: editor.updateShotTransition(panel.activeShot().id, panel.activeShot().transitionIn, panel.activeShot().transitionDurationFrames, currentText)
            }
            Label { text: editor.camera.shotId ? "CÂMERA 2D · SHOT " + editor.camera.shotId : "CÂMERA 2D"; color: "#8193a2"; font.pixelSize: 10; font.letterSpacing: 1; Layout.topMargin: 8; elide: Text.ElideRight; Layout.fillWidth: true }
            RowLayout {
                CameraField { caption: "Centro X"; propertyKey: "x"; currentValue: editor.camera.x }
                CameraField { caption: "Centro Y"; propertyKey: "y"; currentValue: editor.camera.y }
            }
            RowLayout {
                CameraField { caption: "Zoom"; propertyKey: "zoom"; currentValue: editor.camera.zoom }
                CameraField { caption: "Rotação"; propertyKey: "rotation"; currentValue: editor.camera.rotation }
            }
            Label { text: editor.camera.shotId ? "Os valores gravam um keyframe vinculado ao shot no quadro atual." : "Os valores gravam um keyframe no quadro atual."; color: "#708393"; font.pixelSize: 10; wrapMode: Text.WordWrap; Layout.fillWidth: true }
            Label { text: "Curva da câmera até este keyframe"; visible: !!editor.camera.keyframeEasing; color: "#8193a2"; font.pixelSize: 10 }
            StudioComboBox {
                Layout.fillWidth: true; visible: !!editor.camera.keyframeEasing
                model: ["linear", "ease-in", "ease-out", "ease-in-out"]
                currentIndex: Math.max(0, model.indexOf(editor.camera.keyframeEasing))
                onActivated: editor.setCameraKeyframeEasing(currentText)
            }
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
                Label { text: "ANIMAÇÃO POR PALAVRA"; color: "#8193a2"; font.pixelSize: 10; font.letterSpacing: 1; visible: panel.node.role === "caption" && panel.node.wordAligned }
                StudioComboBox {
                    Layout.fillWidth: true
                    visible: panel.node.role === "caption" && panel.node.wordAligned
                    model: ["highlight", "none", "pop", "rise", "fade", "pop-rise"]
                    currentIndex: Math.max(0, model.indexOf(panel.node.wordAnimationPreset || "highlight"))
                    onActivated: editor.updateSelected("wordAnimationPreset", currentText)
                }
                RowLayout {
                    visible: panel.node.role === "caption" && panel.node.wordAligned && panel.node.wordAnimationPreset !== "highlight" && panel.node.wordAnimationPreset !== "none"
                    Field { caption: "Duração · quadros"; propertyKey: "wordAnimationDurationFrames"; currentValue: panel.node.wordAnimationDurationFrames }
                    Field { caption: "Escala inicial"; propertyKey: "wordAnimationStartScale"; currentValue: panel.node.wordAnimationStartScale }
                }
                Field { caption: "Subida · px"; propertyKey: "wordAnimationTravelY"; currentValue: panel.node.wordAnimationTravelY; visible: panel.node.role === "caption" && panel.node.wordAligned && (panel.node.wordAnimationPreset === "rise" || panel.node.wordAnimationPreset === "pop-rise") }
                Label { text: "Esta legenda tem alinhamento real por palavra. A animação acompanha a voz na prévia e na exportação."; color: "#708393"; font.pixelSize: 10; wrapMode: Text.WordWrap; Layout.fillWidth: true; visible: panel.node.role === "caption" && panel.node.wordAligned }
                Label { text: "Esta legenda não tem timestamps reais por palavra; importe uma voz que forneça alinhamento para animá-la."; color: "#708393"; font.pixelSize: 10; wrapMode: Text.WordWrap; Layout.fillWidth: true; visible: panel.node.role === "caption" && !panel.node.wordAligned }
                Label {
                    text: "ALINHAMENTO MANUAL · QUADROS"
                    color: "#8193a2"; font.pixelSize: 10; font.letterSpacing: 1; Layout.topMargin: 4
                    visible: panel.node.role === "caption" && panel.node.wordAligned
                }
                Label {
                    text: "Ajuste os limites com precisão de um quadro. Cada limite compartilhado move as duas palavras juntas."
                    color: "#708393"; font.pixelSize: 10; wrapMode: Text.WordWrap; Layout.fillWidth: true
                    visible: panel.node.role === "caption" && panel.node.wordAligned
                }
                RowLayout {
                    visible: panel.node.role === "caption" && panel.node.wordAligned
                    Layout.fillWidth: true; spacing: 8
                    Label { text: "PALAVRA"; color: "#708393"; font.pixelSize: 9; Layout.fillWidth: true }
                    Label { text: "INÍCIO"; color: "#708393"; font.pixelSize: 9; Layout.preferredWidth: 68; horizontalAlignment: Text.AlignHCenter }
                    Label { text: "FIM"; color: "#708393"; font.pixelSize: 9; Layout.preferredWidth: 68; horizontalAlignment: Text.AlignHCenter }
                }
                ListView {
                    id: captionWordList
                    objectName: "captionWordAlignmentList"
                    visible: panel.node.role === "caption" && panel.node.wordAligned
                    Layout.fillWidth: true
                    Layout.preferredHeight: Math.min(240, contentHeight)
                    clip: true; spacing: 2
                    model: panel.node.captionWords || []
                    delegate: Rectangle {
                        required property var modelData
                        required property int index
                        width: captionWordList.width; height: 36; radius: 4
                        color: editor.frame >= modelData.start && editor.frame < modelData.end ? "#22342d" : "transparent"
                        RowLayout {
                            anchors.fill: parent; anchors.leftMargin: 5; anchors.rightMargin: 3; spacing: 8
                            Label { text: modelData.text; color: "#dce6ee"; font.pixelSize: 11; elide: Text.ElideRight; Layout.fillWidth: true }
                            SpinBox {
                                Layout.preferredWidth: 68; implicitHeight: 30; editable: true
                                from: index === 0 ? panel.node.start : panel.node.captionWords[index - 1].start + 1
                                to: modelData.end - 1; value: modelData.start
                                onValueModified: editor.setCaptionWordBoundary(index, "start", value)
                            }
                            SpinBox {
                                Layout.preferredWidth: 68; implicitHeight: 30; editable: true
                                from: modelData.start + 1
                                to: index + 1 < panel.node.captionWords.length
                                    ? panel.node.captionWords[index + 1].end - 1
                                    : panel.node.start + panel.node.duration
                                value: modelData.end
                                onValueModified: editor.setCaptionWordBoundary(index, "end", value)
                            }
                        }
                    }
                }
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
                Label { text: "FONTE DE VÍDEO"; color: "#8193a2"; font.pixelSize: 10; font.letterSpacing: 1; visible: panel.node.kind === "video" }
                StudioButton {
                    objectName: "addVideoAudioButton"
                    text: "Adicionar áudio do vídeo à timeline"
                    Layout.fillWidth: true
                    visible: panel.node.kind === "video"
                    onClicked: editor.addAudioFromSelectedVideo()
                    ToolTip.visible: hovered
                    ToolTip.text: "Cria uma pista editável alinhada ao trim e à velocidade deste clipe"
                }
                RowLayout {
                    visible: panel.node.kind === "video"
                    Field { caption: "Início da fonte · s"; propertyKey: "videoSourceInSeconds"; currentValue: panel.node.videoSourceInSeconds }
                    Field { caption: "Velocidade · ×"; propertyKey: "videoPlaybackRate"; currentValue: panel.node.videoPlaybackRate }
                }
                RowLayout {
                    visible: panel.node.kind === "video"
                    Field { caption: "Recorte X · 0–1"; propertyKey: "videoCropX"; currentValue: panel.node.videoCropX }
                    Field { caption: "Recorte Y · 0–1"; propertyKey: "videoCropY"; currentValue: panel.node.videoCropY }
                }
                RowLayout {
                    visible: panel.node.kind === "video"
                    Field { caption: "Largura · 0–1"; propertyKey: "videoCropWidth"; currentValue: panel.node.videoCropWidth }
                    Field { caption: "Altura · 0–1"; propertyKey: "videoCropHeight"; currentValue: panel.node.videoCropHeight }
                }
                Label { text: "O trim de saída é a duração do objeto na timeline. Recorte usa frações da imagem original."; color: "#708393"; font.pixelSize: 10; wrapMode: Text.WordWrap; Layout.fillWidth: true; visible: panel.node.kind === "video" }
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
                Label { text: "Curva até este keyframe"; visible: !!panel.node.keyframeEasing; color: "#8193a2"; font.pixelSize: 10 }
                StudioComboBox {
                    Layout.fillWidth: true; visible: !!panel.node.keyframeEasing
                    model: ["linear", "ease-in", "ease-out", "ease-in-out"]
                    currentIndex: Math.max(0, model.indexOf(panel.node.keyframeEasing))
                    onActivated: editor.setSelectedKeyframeEasing(currentText)
                }
            }
            Item { Layout.preferredHeight: 12 }
        }
    }
}
