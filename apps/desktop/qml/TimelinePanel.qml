import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    signal addAudioRequested()
    color: "#141c24"
    Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: "#2f3c48" }
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 16; spacing: 10
        RowLayout {
            Layout.fillWidth: true
            Label { text: "TIMELINE"; color: "#b9c7d1"; font.pixelSize: 11; font.letterSpacing: 1.5; font.bold: true }
            Label { text: " /   SEQUÊNCIA 01"; color: "#5e7485"; font.pixelSize: 10 }
            Item { Layout.fillWidth: true }
            Label { text: "Arraste um clipe para mover  ·  Clique na régua para navegar"; color: "#7b91a1"; font.pixelSize: 10 }
            StudioButton { text: "+ Áudio"; onClicked: root.addAudioRequested() }
            Label { text: (editor.duration / editor.fps).toFixed(1) + " s"; color: "#b4f078"; font.pixelSize: 12; Layout.leftMargin: 15 }
        }
        RowLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; spacing: 12
            ColumnLayout {
                Layout.preferredWidth: 175; Layout.minimumWidth: 175; Layout.maximumWidth: 175; Layout.fillHeight: true; spacing: 8
                Label { text: editor.shots.length ? "SHOTS + CAMADAS" : "CAMADAS VISUAIS"; color: "#677e90"; font.pixelSize: 9; Layout.preferredHeight: 25 }
                ListView {
                    id: trackLabels; Layout.fillWidth: true; Layout.fillHeight: true; clip: true; interactive: false
                    model: editor.nodes; spacing: 6
                    contentY: tracks.contentY
                    delegate: Label {
                        required property var modelData
                        width: ListView.view.width; height: 35; text: (modelData.parentId ? "↳ " : "") + modelData.name; elide: Text.ElideRight; verticalAlignment: Text.AlignVCenter
                        color: editor.selectedId === modelData.id ? "#b4f078" : "#9baebb"; font.pixelSize: 11
                    }
                }
            }
            ColumnLayout {
                Layout.fillWidth: true; Layout.minimumWidth: 100; Layout.fillHeight: true; spacing: 8
                Item {
                    Layout.fillWidth: true; Layout.preferredHeight: 25
                    Repeater {
                        model: 13
                        delegate: Label {
                            required property int index
                            x: index * (parent.width - 28) / 12
                            text: ((editor.duration / editor.fps) * index / 12).toFixed(0) + "s"
                            color: "#758d9e"; font.pixelSize: 10
                        }
                    }
                    MouseArea { anchors.fill: parent; onPressed: function(mouse) { editor.frame = Math.round(mouse.x / width * (editor.duration - 1)) }; onPositionChanged: function(mouse) { if (pressed) editor.frame = Math.round(mouse.x / width * (editor.duration - 1)) } }
                }
                Item {
                    id: shotTrackArea
                    Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                    Repeater {
                        model: 13
                        Rectangle { required property int index; x: index * (parent.width - 1) / 12; width: 1; height: parent.height; color: "#1e2b36" }
                    }
                    ListView {
                        id: tracks; anchors.fill: parent; anchors.topMargin: editor.shots.length ? 27 : 0
                        model: editor.nodes; clip: true; spacing: 6
                        ScrollBar.vertical: ScrollBar {}
                        delegate: Item {
                            id: track
                            required property var modelData
                            width: ListView.view.width; height: 35
                            Rectangle {
                                id: clip
                                x: track.modelData.start / editor.duration * track.width
                                width: Math.max(4, track.modelData.duration / editor.duration * track.width); height: 35; radius: 5
                                color: editor.selectedId === track.modelData.id ? "#4c673b" : track.modelData.kind === "text" ? "#2f4956" : "#384440"
                                border.width: 1; border.color: editor.selectedId === track.modelData.id ? "#b4f078" : "#526874"
                                Label { anchors.fill: parent; anchors.leftMargin: 10; text: (track.modelData.parentId ? "↳ " : "") + track.modelData.name; color: "#dce8ed"; verticalAlignment: Text.AlignVCenter; elide: Text.ElideRight; font.pixelSize: 10 }
                                MouseArea {
                                    anchors.fill: parent
                                    property real originX: 0
                                    property int startFrame: 0
                                    onPressed: function(mouse) {
                                        track.forceActiveFocus()
                                        if (editor.playing) editor.togglePlayback()
                                        originX = mapToItem(track, mouse.x, mouse.y).x
                                        startFrame = track.modelData.start
                                        editor.selectNode(track.modelData.id)
                                        editor.beginGesture()
                                    }
                                    onPositionChanged: function(mouse) {
                                        if (pressed) {
                                            const currentX = mapToItem(track, mouse.x, mouse.y).x
                                            editor.moveClip(track.modelData.id, startFrame + Math.round((currentX - originX) / track.width * editor.duration))
                                        }
                                    }
                                    onReleased: editor.endGesture()
                                    onCanceled: editor.endGesture()
                                }
                            }
                        }
                    }
                    Repeater {
                        model: editor.shots
                        delegate: Rectangle {
                            required property var modelData
                            required property int index
                            x: modelData.start / editor.duration * parent.width
                            y: 1
                            width: Math.max(6, modelData.duration / editor.duration * parent.width)
                            height: 22
                            radius: 4
                            color: "#304c46"
                            border.width: 1
                            border.color: "#70aa81"
                            z: 2
                            Label {
                                anchors.fill: parent; anchors.leftMargin: 6
                                text: modelData.transitionIn === "cut" ? modelData.name : modelData.name + " · " + modelData.transitionIn
                                color: "#d7eee1"
                                verticalAlignment: Text.AlignVCenter; elide: Text.ElideRight
                                font.pixelSize: 9
                                z: 2
                            }
                            Rectangle {
                                visible: modelData.transitionIn !== "cut"
                                x: 0; y: 0
                                width: Math.min(parent.width, modelData.transitionDurationFrames / editor.duration * shotTrackArea.width)
                                height: parent.height
                                color: "#d8b46b"
                                opacity: 0.35
                                z: 1
                                ToolTip.visible: transitionArea.containsMouse
                                ToolTip.text: modelData.transitionIn + " · " + modelData.transitionDurationFrames + " quadros · " + modelData.transitionDirection
                                MouseArea { id: transitionArea; anchors.fill: parent; hoverEnabled: true }
                            }
                            MouseArea {
                                anchors.fill: parent
                                onClicked: editor.frame = modelData.start
                            }
                            Rectangle {
                                visible: modelData.hasPrevious
                                x: 0; y: 0; width: 8; height: parent.height
                                color: "#b4f078"; opacity: boundaryArea.containsMouse || boundaryArea.pressed ? 1 : 0.55
                                z: 2
                                MouseArea {
                                    id: boundaryArea
                                    anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.SizeHorCursor
                                    property real originX: 0
                                    property int boundaryStart: 0
                                    onPressed: function(mouse) {
                                        if (editor.playing) editor.togglePlayback()
                                        originX = mapToItem(shotTrackArea, mouse.x, mouse.y).x
                                        boundaryStart = modelData.start
                                        editor.beginGesture()
                                    }
                                    onPositionChanged: function(mouse) {
                                        if (pressed) {
                                            const currentX = mapToItem(shotTrackArea, mouse.x, mouse.y).x
                                            const nextFrame = boundaryStart + Math.round((currentX - originX) / shotTrackArea.width * editor.duration)
                                            editor.moveShotBoundary(modelData.id, nextFrame)
                                        }
                                    }
                                    onReleased: editor.endGesture()
                                    onCanceled: editor.endGesture()
                                }
                            }
                        }
                    }
                    Repeater {
                        model: editor.visualBeats
                        delegate: Rectangle {
                            required property var modelData
                            x: modelData.frame / editor.duration * parent.width - 3
                            y: 20
                            width: 7; height: 7; radius: 2
                            color: modelData.action === "camera" ? "#e9bd69" : "#b4f078"
                            border.width: 1; border.color: "#101820"
                            z: 3
                            ToolTip.visible: beatArea.containsMouse
                            ToolTip.text: modelData.action + " · " + modelData.targetId
                            MouseArea { id: beatArea; anchors.fill: parent; hoverEnabled: true; onClicked: editor.frame = modelData.frame }
                        }
                    }
                    Rectangle {
                        x: editor.frame / editor.duration * (parent.width - 1); y: 0; width: 1; height: parent.height; color: "#b4f078"
                        Rectangle { anchors.horizontalCenter: parent.horizontalCenter; width: 7; height: 7; radius: 2; color: "#b4f078" }
                    }
                }
            }
        }
        ListView {
            visible: editor.audioTracks.length > 0
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(234, editor.audioTracks.length * 78)
            Layout.minimumHeight: Layout.preferredHeight
            Layout.maximumHeight: Layout.preferredHeight
            clip: true
            model: editor.audioTracks
            delegate: ColumnLayout {
                required property var modelData
                property real automationValue: 1
                property string automationEasing: "linear"
                width: ListView.view.width; height: 76; spacing: 0
                RowLayout {
                    Layout.fillWidth: true; Layout.preferredHeight: 48; spacing: 6
                Label { text: modelData.name; color: "#91b4c2"; Layout.preferredWidth: 135; elide: Text.ElideRight; font.pixelSize: 10 }
                Label { text: "in"; color: "#718d9e"; font.pixelSize: 9 }
                SpinBox {
                    from: 0; to: Math.max(0, editor.duration - modelData.duration); value: modelData.start
                    editable: true; Layout.preferredWidth: 92
                    onValueModified: editor.setAudioTrackTiming(modelData.id, value, modelData.duration)
                    ToolTip.visible: hovered; ToolTip.text: "Início da faixa em frames"
                }
                Label { text: "duração"; color: "#718d9e"; font.pixelSize: 9 }
                SpinBox {
                    from: 1; to: Math.max(1, editor.duration - modelData.start); value: modelData.duration
                    editable: true; Layout.preferredWidth: 92
                    onValueModified: editor.setAudioTrackTiming(modelData.id, modelData.start, value)
                    ToolTip.visible: hovered; ToolTip.text: "Duração da faixa em frames"
                }
                Slider {
                    Layout.preferredWidth: 160; from: 0; to: 2; value: modelData.gain
                    onMoved: editor.setAudioTrackGain(modelData.id, value)
                    ToolTip.visible: hovered; ToolTip.text: "Ganho da pista"
                }
                Label { text: Math.round(modelData.gain * 100) + "%"; color: "#9baebb"; Layout.preferredWidth: 40; font.pixelSize: 9 }
                Label { text: "fade in"; color: "#718d9e"; font.pixelSize: 9 }
                SpinBox {
                    from: 0; to: Math.min(1800, modelData.duration); value: modelData.fadeInFrames
                    editable: true; Layout.preferredWidth: 72
                    onValueModified: editor.setAudioTrackFades(modelData.id, value, modelData.fadeOutFrames)
                    ToolTip.visible: hovered; ToolTip.text: "Fade de entrada em frames"
                }
                Label { text: "out"; color: "#718d9e"; font.pixelSize: 9 }
                SpinBox {
                    from: 0; to: Math.min(1800, modelData.duration); value: modelData.fadeOutFrames
                    editable: true; Layout.preferredWidth: 72
                    onValueModified: editor.setAudioTrackFades(modelData.id, modelData.fadeInFrames, value)
                    ToolTip.visible: hovered; ToolTip.text: "Fade de saída em frames"
                }
                CheckBox {
                    text: "duck na voz"; checked: modelData.duckUnderVoice
                    onToggled: editor.setAudioTrackDucking(modelData.id, checked)
                    ToolTip.text: "Reduz esta pista durante a fala na prévia e na exportação"; ToolTip.visible: hovered
                }
                CheckBox {
                    text: "mudo"; checked: modelData.muted
                    onToggled: editor.setAudioTrackMuted(modelData.id, checked)
                }
                Item { Layout.fillWidth: true }
                StudioButton { text: "Remover"; onClicked: editor.removeAudioTrack(modelData.id) }
                }
                RowLayout {
                    Layout.fillWidth: true; Layout.preferredHeight: 28; spacing: 8
                    Label {
                        text: "Automação · " + modelData.gainAutomation.length + " pontos"
                        color: "#82bebd"; font.pixelSize: 9; Layout.preferredWidth: 135
                    }
                    Slider {
                        from: 0; to: 2; value: parent.parent.automationValue
                        Layout.preferredWidth: 120
                        onMoved: parent.parent.automationValue = value
                    }
                    Label { text: Math.round(parent.parent.automationValue * 100) + "%"; color: "#9baebb"; Layout.preferredWidth: 34; font.pixelSize: 9 }
                    ComboBox {
                        model: ["linear", "ease-in", "ease-out", "ease-in-out"]
                        currentIndex: Math.max(0, model.indexOf(parent.parent.automationEasing))
                        Layout.preferredWidth: 116
                        onActivated: parent.parent.automationEasing = currentText
                    }
                    StudioButton {
                        text: "+ ponto no playhead"
                        enabled: editor.frame >= modelData.start && editor.frame < modelData.start + modelData.duration
                        onClicked: editor.setAudioTrackGainPoint(modelData.id, parent.parent.automationValue,
                                                                 parent.parent.automationEasing)
                        ToolTip.visible: hovered; ToolTip.text: "Adiciona ou atualiza um ponto na posição atual da sequência"
                    }
                    StudioButton {
                        text: "Limpar automação"; quiet: true
                        enabled: modelData.gainAutomation.length > 0
                        onClicked: editor.clearAudioTrackGainAutomation(modelData.id)
                    }
                    Item { Layout.fillWidth: true }
                }
            }
        }
        RowLayout {
            visible: editor.hasAudio && editor.waveform.length > 0; Layout.fillWidth: true; Layout.preferredHeight: 28; Layout.minimumHeight: 28; Layout.maximumHeight: 28; spacing: 12
            Label { text: "VOZ + TRILHA"; Layout.preferredWidth: 175; color: "#82bebd"; font.pixelSize: 10 }
            Rectangle {
                Layout.fillWidth: true; Layout.fillHeight: true; color: "#1a333b"; radius: 4; clip: true
                Repeater {
                    model: editor.waveform
                    Rectangle {
                        required property real modelData
                        required property int index
                        x: index * parent.width / 256; width: Math.max(1, parent.width / 256 - 1)
                        height: Math.min(24, Math.max(2, modelData * 130)); y: (parent.height - height) / 2
                        color: "#82bebd"
                    }
                }
            }
        }
    }
}
