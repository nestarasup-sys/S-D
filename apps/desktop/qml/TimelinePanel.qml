import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    objectName: "timelinePanelRoot"
    signal addAudioRequested()
    color: "#141c24"
    property real timelineZoom: 1
    property real timelineOffset: 0
    property bool snapEnabled: true
    property int snapGuideFrame: -1
    property bool selectedCaptionTrackAvailable: {
        const rows = editor.nodeItems
        for (let i = 0; i < rows.length; ++i)
            if (rows[i].id === editor.selectedId && rows[i].role === "caption" && rows[i].wordAligned)
                return true
        return false
    }
    property int selectedTrackIndex: {
        const rows = editor.nodeItems
        for (let i = 0; i < rows.length; ++i)
            if (rows[i].id === editor.selectedId)
                return i
        return -1
    }
    function visibleFrames() { return Math.max(1, (editor.duration - 1) / timelineZoom) }
    function maxStartFrame() { return Math.max(0, (editor.duration - 1) - visibleFrames()) }
    function viewStartFrame() { return timelineOffset * maxStartFrame() }
    function frameToX(frame, width) { return (frame - viewStartFrame()) / visibleFrames() * width }
    function xToFrame(x, width) { return viewStartFrame() + x / Math.max(1, width) * visibleFrames() }
    function revealSelectedTrack() {
        const rows = editor.nodeItems
        for (let i = 0; i < rows.length; ++i)
            if (rows[i].id === editor.selectedId) {
                tracks.positionViewAtIndex(i, ListView.Contain)
                return
            }
    }
    function setZoomAt(value, x, width) {
        const ratio = Math.max(0, Math.min(1, x / Math.max(1, width)))
        const anchorFrame = viewStartFrame() + ratio * visibleFrames()
        timelineZoom = Math.max(1, Math.min(8, value))
        const nextStart = Math.max(0, Math.min(maxStartFrame(), anchorFrame - ratio * visibleFrames()))
        timelineOffset = maxStartFrame() > 0 ? nextStart / maxStartFrame() : 0
    }
    Timer {
        id: revealSelectedTrackTimer
        interval: 100; repeat: false
        onTriggered: root.revealSelectedTrack()
    }
    Component.onCompleted: revealSelectedTrackTimer.start()
    Connections {
        target: editor
        function onSelectedChanged() { revealSelectedTrackTimer.restart() }
    }
    Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: "#2f3c48" }
    ColumnLayout {
        anchors.fill: parent; anchors.margins: 16; spacing: 10
        RowLayout {
            Layout.fillWidth: true
            Label { text: "TIMELINE"; color: "#b9c7d1"; font.pixelSize: 11; font.letterSpacing: 1.5; font.bold: true }
            Label { text: " /   SEQUÊNCIA 01"; color: "#5e7485"; font.pixelSize: 10 }
            Item { Layout.fillWidth: true }
            Label {
                text: editor.selectedIds.length > 1
                      ? editor.selectedIds.length + " selecionados · Arraste move juntos · Ctrl+clique altera"
                      : "Ctrl+clique seleciona · Arraste move · Régua navega · Ctrl+roda aproxima"
                color: "#7b91a1"; font.pixelSize: 10
                Layout.maximumWidth: 380
                elide: Text.ElideRight
            }
            Label { text: "Zoom"; color: "#7b91a1"; font.pixelSize: 9 }
            Slider {
                id: zoomSlider
                Layout.preferredWidth: 110
                from: 1; to: 8; stepSize: 0.25; value: root.timelineZoom
                onMoved: root.setZoomAt(value, width / 2, width)
                ToolTip.visible: hovered; ToolTip.text: "Ampliação temporal da timeline"
            }
            Label { text: root.timelineZoom.toFixed(1) + "×"; color: "#b4f078"; font.pixelSize: 10; Layout.preferredWidth: 30 }
            Label { text: "Pan"; color: "#7b91a1"; font.pixelSize: 9 }
            Slider {
                id: scrollSlider
                Layout.preferredWidth: 100
                from: 0; to: 1; value: root.timelineOffset; enabled: root.timelineZoom > 1.001
                onMoved: root.timelineOffset = value
                ToolTip.visible: hovered; ToolTip.text: "Navegação horizontal: início → fim"
            }
            StudioButton {
                text: root.snapEnabled ? "Snap ligado" : "Snap desligado"
                quiet: !root.snapEnabled
                onClicked: { root.snapEnabled = !root.snapEnabled; root.snapGuideFrame = -1 }
                ToolTip.visible: hovered
                ToolTip.text: "Alinha clipes a cenas, shots, camadas, beats, áudio e playhead"
            }
            StudioButton { text: "+ Áudio"; onClicked: root.addAudioRequested() }
            Label { text: (editor.duration / editor.fps).toFixed(1) + " s"; color: "#b4f078"; font.pixelSize: 12; Layout.leftMargin: 15 }
        }
        RowLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; spacing: 12
            ColumnLayout {
                Layout.preferredWidth: 175; Layout.minimumWidth: 175; Layout.maximumWidth: 175; Layout.fillHeight: true; spacing: 8
                Label { text: editor.shots.length ? "SHOTS + CAMADAS" : "CAMADAS VISUAIS"; color: "#677e90"; font.pixelSize: 9; Layout.preferredHeight: 25 }
                ListView {
                    id: trackLabels; Layout.fillWidth: true; Layout.fillHeight: true
                    Layout.topMargin: editor.shots.length ? 27 : 0; clip: true; interactive: false
                    model: editor.nodes; spacing: 6
                    contentY: tracks.contentY
                    delegate: Label {
                        required property var modelData
                        width: ListView.view.width; height: 35; text: (modelData.parentId ? "↳ " : "") + modelData.name; elide: Text.ElideRight; verticalAlignment: Text.AlignVCenter
                        color: editor.selectedIds.indexOf(modelData.id) >= 0 ? "#b4f078" : "#9baebb"; font.pixelSize: 11
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
                            text: ((root.viewStartFrame() + root.visibleFrames() * index / 12) / editor.fps).toFixed(1) + "s"
                            color: "#758d9e"; font.pixelSize: 10
                        }
                    }
                    MouseArea {
                        anchors.fill: parent
                        onPressed: function(mouse) { editor.frame = Math.round(root.xToFrame(mouse.x, width)) }
                        onPositionChanged: function(mouse) { if (pressed) editor.frame = Math.round(root.xToFrame(mouse.x, width)) }
                        onWheel: function(wheel) {
                            if (wheel.modifiers & Qt.ControlModifier) {
                                const factor = wheel.angleDelta.y > 0 ? 1.25 : 0.8
                                root.setZoomAt(root.timelineZoom * factor, wheel.x, width)
                                wheel.accepted = true
                            } else if (wheel.modifiers & Qt.ShiftModifier && root.timelineZoom > 1.001) {
                                root.timelineOffset = Math.max(0, Math.min(1,
                                    root.timelineOffset + (wheel.angleDelta.y < 0 ? 0.08 : -0.08)))
                                wheel.accepted = true
                            }
                        }
                    }
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
                                x: root.frameToX(track.modelData.start, track.width)
                                width: Math.max(4, track.modelData.duration / root.visibleFrames() * track.width); height: 35; radius: 5
                                color: editor.selectedIds.indexOf(track.modelData.id) >= 0 ? "#4c673b" : track.modelData.kind === "text" ? "#2f4956" : "#384440"
                                border.width: editor.selectedIds.indexOf(track.modelData.id) >= 0 ? 2 : 1
                                border.color: editor.selectedIds.indexOf(track.modelData.id) >= 0 ? "#b4f078" : "#526874"
                                Label { anchors.fill: parent; anchors.leftMargin: 10; text: (track.modelData.parentId ? "↳ " : "") + track.modelData.name; color: "#dce8ed"; verticalAlignment: Text.AlignVCenter; elide: Text.ElideRight; font.pixelSize: 10; z: 1 }
                                Repeater {
                                    model: track.modelData.captionWords || []
                                    delegate: Item {
                                        required property var modelData
                                        required property int index
                                        x: (modelData.start - track.modelData.start) / root.visibleFrames() * track.width
                                        width: Math.max(1, (modelData.end - modelData.start) / root.visibleFrames() * track.width)
                                        height: clip.height; z: 2
                                        Rectangle {
                                            x: 0; y: parent.height - 4; width: parent.width; height: 3
                                            color: editor.frame >= modelData.start && editor.frame < modelData.end ? "#b4f078" : "#82bebd"
                                            opacity: editor.frame >= modelData.start && editor.frame < modelData.end ? 1 : 0.72
                                        }
                                        Rectangle {
                                            x: -3; y: 12; width: 7; height: 11; radius: 2
                                            color: index === 0 ? "#d6e8e0" : "#b4f078"
                                            border.width: 1; border.color: "#20352e"
                                            ToolTip.visible: boundaryHint.containsMouse
                                            ToolTip.text: index === 0 ? "Arraste para ajustar o início desta legenda" : "Arraste para alinhar as duas palavras vizinhas"
                                            MouseArea { id: boundaryHint; anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.NoButton }
                                        }
                                        Rectangle {
                                            visible: index === track.modelData.captionWords.length - 1
                                            x: parent.width - 3; y: 12; width: 7; height: 11; radius: 2
                                            color: "#d6e8e0"; border.width: 1; border.color: "#20352e"
                                            ToolTip.visible: lastBoundaryHint.containsMouse
                                            ToolTip.text: "Arraste para ajustar o fim desta legenda"
                                            MouseArea { id: lastBoundaryHint; anchors.fill: parent; hoverEnabled: true; acceptedButtons: Qt.NoButton }
                                        }
                                    }
                                }
                                MouseArea {
                                    anchors.fill: parent
                                    property bool captionBoundaryEnabled: editor.selected.id === track.modelData.id && track.modelData.role === "caption" && track.modelData.wordAligned
                                    objectName: captionBoundaryEnabled ? "captionWordBoundaryDrag" : ""
                                    property real originX: 0
                                    property int startFrame: 0
                                    property bool selectionClick: false
                                    property int boundaryWordIndex: -1
                                    property string boundaryEdge: ""
                                    property int boundaryMinimum: 0
                                    property int boundaryMaximum: 0
                                    function boundaryAt(localX) {
                                        if (!captionBoundaryEnabled)
                                            return null
                                        const words = editor.selected.captionWords
                                        if (!words || !words.length)
                                            return null
                                        let nearest = null
                                        for (let i = 0; i < words.length; ++i) {
                                            const word = words[i]
                                            const x = (word.start - track.modelData.start) / root.visibleFrames() * track.width
                                            const distance = Math.abs(localX - x)
                                            if (distance <= 9 && (!nearest || distance < nearest.distance))
                                                nearest = {index: i, edge: "start", minimum: i === 0 ? track.modelData.start : words[i - 1].start + 1,
                                                           maximum: word.end - 1, distance: distance}
                                        }
                                        const last = words.length - 1
                                        const endX = (words[last].end - track.modelData.start) / root.visibleFrames() * track.width
                                        const endDistance = Math.abs(localX - endX)
                                        if (endDistance <= 9 && (!nearest || endDistance < nearest.distance))
                                            nearest = {index: last, edge: "end", minimum: words[last].start + 1,
                                                       maximum: track.modelData.start + track.modelData.duration, distance: endDistance}
                                        return nearest
                                    }
                                    onPressed: function(mouse) {
                                        root.snapGuideFrame = -1
                                        track.forceActiveFocus()
                                        if (editor.playing) editor.togglePlayback()
                                        selectionClick = (mouse.modifiers & Qt.ControlModifier) !== 0
                                        if (selectionClick) {
                                            editor.toggleNodeSelection(track.modelData.id)
                                            return
                                        }
                                        const boundary = boundaryAt(mouse.x)
                                        if (boundary) {
                                            boundaryWordIndex = boundary.index; boundaryEdge = boundary.edge
                                            boundaryMinimum = boundary.minimum; boundaryMaximum = boundary.maximum
                                            editor.beginGesture()
                                            return
                                        }
                                        originX = mapToItem(track, mouse.x, mouse.y).x
                                        startFrame = track.modelData.start
                                        if (editor.selectedIds.indexOf(track.modelData.id) < 0)
                                            editor.selectNode(track.modelData.id)
                                        editor.beginGesture()
                                    }
                                    onPositionChanged: function(mouse) {
                                        if (pressed && !selectionClick) {
                                            const currentX = mapToItem(track, mouse.x, mouse.y).x
                                            if (boundaryWordIndex >= 0) {
                                                const pointerX = mapToItem(shotTrackArea, mouse.x, mouse.y).x
                                                const frame = Math.max(boundaryMinimum, Math.min(boundaryMaximum,
                                                    Math.round(root.xToFrame(pointerX, shotTrackArea.width))))
                                                editor.setCaptionWordBoundary(boundaryWordIndex, boundaryEdge, frame)
                                            } else {
                                                const requestedStart = startFrame + Math.round((currentX - originX) / track.width * root.visibleFrames())
                                                let nextStart = requestedStart
                                                root.snapGuideFrame = -1
                                                if (root.snapEnabled) {
                                                    const tolerance = Math.max(1, Math.ceil(8 / Math.max(1, shotTrackArea.width) * root.visibleFrames()))
                                                    const snap = editor.snapClipMove(track.modelData.id, startFrame, requestedStart, tolerance)
                                                    nextStart = snap.start
                                                    root.snapGuideFrame = snap.guideFrame
                                                }
                                                editor.moveSelectedClips(track.modelData.id, startFrame, nextStart)
                                            }
                                        }
                                    }
                                    onReleased: {
                                        if (!selectionClick) editor.endGesture()
                                        root.snapGuideFrame = -1
                                        selectionClick = false; boundaryWordIndex = -1; boundaryEdge = ""
                                    }
                                    onCanceled: {
                                        if (!selectionClick) editor.endGesture()
                                        root.snapGuideFrame = -1
                                        selectionClick = false; boundaryWordIndex = -1; boundaryEdge = ""
                                    }
                                }
                            }
                        }
                    }
                    Repeater {
                        model: editor.shots
                        delegate: Rectangle {
                            required property var modelData
                            required property int index
                            x: root.frameToX(modelData.start, parent.width)
                            y: 1
                            width: Math.max(6, modelData.duration / root.visibleFrames() * parent.width)
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
                                width: Math.max(8, Math.min(parent.width, modelData.transitionDurationFrames / root.visibleFrames() * shotTrackArea.width))
                                height: parent.height
                                color: "#d8b46b"
                                opacity: 0.35
                                z: 1
                                ToolTip.visible: transitionArea.containsMouse
                                ToolTip.text: modelData.transitionIn + " · sobreposição de " + modelData.transitionDurationFrames + " quadros · arraste a alça âmbar para ajustar"
                                MouseArea { id: transitionArea; anchors.fill: parent; hoverEnabled: true }
                                Rectangle {
                                    x: parent.width - width / 2; y: 0; width: 8; height: parent.height
                                    color: resizeTransitionArea.containsMouse || resizeTransitionArea.pressed ? "#ffe0a0" : "#e8bd70"
                                    z: 3
                                    MouseArea {
                                        id: resizeTransitionArea
                                        anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.SizeHorCursor
                                        property real originX: 0
                                        property int originDuration: 0
                                        onPressed: function(mouse) {
                                            if (editor.playing) editor.togglePlayback()
                                            originX = mapToItem(shotTrackArea, mouse.x, mouse.y).x
                                            originDuration = modelData.transitionDurationFrames
                                            editor.beginGesture()
                                        }
                                        onPositionChanged: function(mouse) {
                                            if (pressed) {
                                                const currentX = mapToItem(shotTrackArea, mouse.x, mouse.y).x
                                                const nextDuration = originDuration + Math.round((currentX - originX) / shotTrackArea.width * root.visibleFrames())
                                                editor.resizeShotTransition(modelData.id, nextDuration)
                                            }
                                        }
                                        onReleased: editor.endGesture()
                                        onCanceled: editor.endGesture()
                                    }
                                }
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
                                            const nextFrame = boundaryStart + Math.round((currentX - originX) / shotTrackArea.width * root.visibleFrames())
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
                            x: root.frameToX(modelData.frame, parent.width) - 3
                            y: 20
                            width: 7; height: 7; radius: 2
                            color: modelData.action === "sound" ? "#e97868"
                                 : modelData.action === "camera" ? "#e9bd69" : "#b4f078"
                            border.width: 1; border.color: "#101820"
                            z: 3
                            ToolTip.visible: beatArea.containsMouse
                            ToolTip.text: (modelData.action === "sound" ? "efeito sonoro" : modelData.action) + " · " + modelData.targetId
                            MouseArea { id: beatArea; anchors.fill: parent; hoverEnabled: true; onClicked: editor.frame = modelData.frame }
                        }
                    }
                    Rectangle {
                        x: root.frameToX(editor.frame, parent.width - 1); y: 0; width: 1; height: parent.height; color: "#b4f078"
                        Rectangle { anchors.horizontalCenter: parent.horizontalCenter; width: 7; height: 7; radius: 2; color: "#b4f078" }
                    }
                    Rectangle {
                        visible: root.snapGuideFrame >= 0
                        x: root.frameToX(root.snapGuideFrame, parent.width) - 1
                        y: 0; width: 2; height: parent.height
                        color: "#f0bd63"
                        z: 10
                        Label {
                            anchors.left: parent.left; anchors.top: parent.top; anchors.leftMargin: 4; anchors.topMargin: 3
                            text: "ALINHADO"; color: "#f0bd63"; font.pixelSize: 8; font.bold: true
                        }
                    }
                }
            }
        }
        ListView {
            visible: editor.audioTracks.length > 0
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(138, editor.audioTracks.length * 138)
            Layout.minimumHeight: Layout.preferredHeight
            Layout.maximumHeight: Layout.preferredHeight
            clip: true
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
            model: editor.audioTrackModel
            delegate: ColumnLayout {
                id: audioTrackRow
                required property var modelData
                property real automationValue: 1
                property string automationEasing: "linear"
                width: ListView.view.width; height: 136; spacing: 1
                onModelDataChanged: envelopeCanvas.requestPaint()
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
                    Layout.fillWidth: true; Layout.preferredHeight: 26; spacing: 6
                    Label { text: "Fonte · s"; color: "#718d9e"; font.pixelSize: 9 }
                    TextField {
                        Layout.preferredWidth: 98
                        text: Number(audioTrackRow.modelData.sourceInSeconds).toFixed(2)
                        validator: DoubleValidator { locale: Qt.locale("en_US"); bottom: 0; top: 3600; decimals: 2; notation: DoubleValidator.StandardNotation }
                        onEditingFinished: editor.setAudioTrackSourceTiming(audioTrackRow.modelData.id,
                            Number(text), audioTrackRow.modelData.playbackRate)
                        ToolTip.visible: hovered; ToolTip.text: "Trim de entrada na fonte de áudio, em segundos"
                    }
                    Label { text: "Velocidade · ×"; color: "#718d9e"; font.pixelSize: 9 }
                    TextField {
                        Layout.preferredWidth: 82
                        text: Number(audioTrackRow.modelData.playbackRate).toFixed(2)
                        validator: DoubleValidator { locale: Qt.locale("en_US"); bottom: 0.25; top: 4; decimals: 2; notation: DoubleValidator.StandardNotation }
                        onEditingFinished: editor.setAudioTrackSourceTiming(audioTrackRow.modelData.id,
                            audioTrackRow.modelData.sourceInSeconds, Number(text))
                        ToolTip.visible: hovered; ToolTip.text: "Velocidade da fonte, entre 0,25× e 4×"
                    }
                    Item { Layout.fillWidth: true }
                }
                Item {
                    Layout.fillWidth: true; Layout.preferredHeight: 30
                    clip: true
                    Rectangle {
                        id: envelopeLane
                        x: root.frameToX(modelData.start, parent.width)
                        width: Math.max(24, modelData.duration / root.visibleFrames() * parent.width)
                        height: parent.height; radius: 3; color: "#102129"; clip: true
                        Canvas {
                            id: envelopeCanvas
                            anchors.fill: parent
                            onPaint: {
                                const context = getContext("2d")
                                context.clearRect(0, 0, width, height)
                                context.strokeStyle = "#29404a"
                                context.lineWidth = 1
                                context.beginPath()
                                const unityY = height - 4 - (height - 8) / 4
                                context.moveTo(0, unityY)
                                context.lineTo(width, unityY)
                                context.stroke()
                                const samples = audioTrackRow.modelData.gainEnvelope
                                if (!samples || samples.length < 2)
                                    return
                                context.strokeStyle = "#82bebd"
                                context.lineWidth = 2
                                context.beginPath()
                                for (let i = 0; i < samples.length; ++i) {
                                    const px = i * width / (samples.length - 1)
                                    const py = height - 4 - Math.max(0, Math.min(4, samples[i])) * (height - 8) / 4
                                    if (i === 0) context.moveTo(px, py)
                                    else context.lineTo(px, py)
                                }
                                context.stroke()
                            }
                        }
                        Repeater {
                            model: audioTrackRow.modelData.gainAutomation
                            delegate: Rectangle {
                                required property var modelData
                                x: modelData.frame / Math.max(1, audioTrackRow.modelData.duration - 1) * (parent.width - width)
                                y: parent.height - 4 - modelData.value * (parent.height - 8) / 4 - height / 2
                                width: 8; height: 8; radius: 4; color: "#b4f078"
                                border.width: 1; border.color: "#142027"
                            }
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            property int editedPointFrame: 0
                            property string editedPointEasing: "linear"
                            function updatePoint(mouseX, mouseY) {
                                const value = Math.max(0, Math.min(4,
                                    (height - 4 - mouseY) * 4 / Math.max(1, height - 8)))
                                editor.setAudioTrackGainPointAt(audioTrackRow.modelData.id, editedPointFrame,
                                                                 value, editedPointEasing)
                            }
                            onPressed: function(mouse) {
                                let nearest = -1
                                let nearestDistance = 12
                                const points = audioTrackRow.modelData.gainAutomation
                                for (let i = 0; i < points.length; ++i) {
                                    const point = points[i]
                                    const pointX = point.frame / Math.max(1, audioTrackRow.modelData.duration - 1) * width
                                    const pointY = height - 4 - point.value * (height - 8) / 4
                                    const distance = Math.hypot(mouse.x - pointX, mouse.y - pointY)
                                    if (distance < nearestDistance) {
                                        nearest = i
                                        nearestDistance = distance
                                    }
                                }
                                editedPointFrame = nearest >= 0 ? points[nearest].frame
                                    : Math.max(0, Math.min(audioTrackRow.modelData.duration - 1,
                                        Math.round(mouse.x / Math.max(1, width) * (audioTrackRow.modelData.duration - 1))))
                                editedPointEasing = nearest >= 0 ? points[nearest].easing
                                    : audioTrackRow.automationEasing
                                editor.beginGesture()
                                updatePoint(mouse.x, mouse.y)
                            }
                            onPositionChanged: function(mouse) {
                                if (pressed)
                                    updatePoint(mouse.x, mouse.y)
                            }
                            onReleased: editor.endGesture()
                            onCanceled: editor.endGesture()
                            hoverEnabled: true
                            ToolTip.visible: containsMouse
                            ToolTip.text: "Clique para criar um ponto ou arraste um ponto para ajustar tempo e ganho · escala ×0–×4"
                        }
                        Rectangle {
                            anchors.left: parent.left; width: 15; height: parent.height
                            color: audioMoveArea.pressed ? "#c7ee8b" : "#334b46"
                            border.width: 1; border.color: "#789b71"
                            z: 4
                            Label { anchors.centerIn: parent; text: "↔"; color: "#e5f2dd"; font.pixelSize: 10 }
                            MouseArea {
                                id: audioMoveArea
                                anchors.fill: parent
                                cursorShape: Qt.SizeHorCursor
                                property real originX: 0
                                property int originStart: 0
                                property int trackLength: 1
                                onPressed: function(mouse) {
                                    if (editor.playing) editor.togglePlayback()
                                    root.snapGuideFrame = -1
                                    originX = mapToItem(audioTrackRow, mouse.x, mouse.y).x
                                    originStart = audioTrackRow.modelData.start
                                    trackLength = audioTrackRow.modelData.duration
                                    editor.beginGesture()
                                }
                                onPositionChanged: function(mouse) {
                                    if (!pressed) return
                                    const currentX = mapToItem(audioTrackRow, mouse.x, mouse.y).x
                                    const requestedStart = originStart + Math.round(
                                        (currentX - originX) / Math.max(1, audioTrackRow.width) * root.visibleFrames())
                                    let nextStart = requestedStart
                                    root.snapGuideFrame = -1
                                    if (root.snapEnabled) {
                                        const tolerance = Math.max(1, Math.ceil(
                                            8 / Math.max(1, audioTrackRow.width) * root.visibleFrames()))
                                        const snap = editor.snapAudioTrackMove(audioTrackRow.modelData.id,
                                            originStart, requestedStart, trackLength, tolerance)
                                        nextStart = snap.start
                                        root.snapGuideFrame = snap.guideFrame
                                    }
                                    editor.setAudioTrackTiming(audioTrackRow.modelData.id, nextStart, trackLength)
                                }
                                onReleased: { editor.endGesture(); root.snapGuideFrame = -1 }
                                onCanceled: { editor.endGesture(); root.snapGuideFrame = -1 }
                                ToolTip.visible: containsMouse
                                ToolTip.text: "Arraste para mover a pista de áudio; Snap alinha a shots, camadas, beats e outras pistas"
                            }
                        }
                        Rectangle {
                            anchors.right: parent.right; width: 11; height: parent.height
                            color: audioTrimArea.pressed ? "#c7ee8b" : "#334b46"
                            border.width: 1; border.color: "#789b71"
                            z: 4
                            MouseArea {
                                id: audioTrimArea
                                anchors.fill: parent
                                cursorShape: Qt.SizeHorCursor
                                property real originX: 0
                                property int originStart: 0
                                property int originLength: 1
                                onPressed: function(mouse) {
                                    if (editor.playing) editor.togglePlayback()
                                    root.snapGuideFrame = -1
                                    originX = mapToItem(audioTrackRow, mouse.x, mouse.y).x
                                    originStart = audioTrackRow.modelData.start
                                    originLength = audioTrackRow.modelData.duration
                                    editor.beginGesture()
                                }
                                onPositionChanged: function(mouse) {
                                    if (!pressed) return
                                    const currentX = mapToItem(audioTrackRow, mouse.x, mouse.y).x
                                    const delta = Math.round(
                                        (currentX - originX) / Math.max(1, audioTrackRow.width) * root.visibleFrames())
                                    const requestedLength = Math.max(1, Math.min(editor.duration - originStart,
                                        originLength + delta))
                                    let nextLength = requestedLength
                                    root.snapGuideFrame = -1
                                    if (root.snapEnabled) {
                                        const tolerance = Math.max(1, Math.ceil(
                                            8 / Math.max(1, audioTrackRow.width) * root.visibleFrames()))
                                        const snap = editor.snapAudioTrackTrim(audioTrackRow.modelData.id,
                                            originStart, originLength, requestedLength, tolerance)
                                        nextLength = snap.length
                                        root.snapGuideFrame = snap.guideFrame
                                    }
                                    editor.setAudioTrackTiming(audioTrackRow.modelData.id, originStart, nextLength)
                                }
                                onReleased: { editor.endGesture(); root.snapGuideFrame = -1 }
                                onCanceled: { editor.endGesture(); root.snapGuideFrame = -1 }
                                ToolTip.visible: containsMouse
                                ToolTip.text: "Arraste para ajustar o fim da pista ao frame; Snap alinha a cenas, shots, camadas e áudio"
                            }
                        }
                    }
                    Rectangle {
                        visible: root.snapGuideFrame >= 0
                        x: root.frameToX(root.snapGuideFrame, parent.width) - 1
                        y: 0; width: 2; height: parent.height; color: "#f0bd63"; z: 6
                    }
                    Label {
                        anchors.left: parent.left; anchors.verticalCenter: parent.verticalCenter
                        leftPadding: 5; text: "Envelope ×0–×4"; color: "#78919d"; font.pixelSize: 8
                        visible: envelopeLane.width < 80
                    }
                }
                RowLayout {
                    Layout.fillWidth: true; Layout.preferredHeight: 28; spacing: 8
                    Label {
                        text: "Envelope · " + modelData.gainAutomation.length + " pontos"
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
