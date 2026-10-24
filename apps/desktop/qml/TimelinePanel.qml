import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
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
            Label { text: (editor.duration / editor.fps).toFixed(1) + " s"; color: "#b4f078"; font.pixelSize: 12; Layout.leftMargin: 15 }
        }
        RowLayout {
            Layout.fillWidth: true; Layout.fillHeight: true; spacing: 12
            ColumnLayout {
                Layout.preferredWidth: 175; Layout.minimumWidth: 175; Layout.maximumWidth: 175; Layout.fillHeight: true; spacing: 8
                Label { text: "CAMADAS VISUAIS"; color: "#677e90"; font.pixelSize: 9; Layout.preferredHeight: 25 }
                ListView {
                    id: trackLabels; Layout.fillWidth: true; Layout.fillHeight: true; clip: true; interactive: false
                    model: editor.nodes; spacing: 6
                    contentY: tracks.contentY
                    delegate: Label {
                        required property var modelData
                        width: ListView.view.width; height: 35; text: modelData.name; elide: Text.ElideRight; verticalAlignment: Text.AlignVCenter
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
                    Layout.fillWidth: true; Layout.fillHeight: true; clip: true
                    Repeater {
                        model: 13
                        Rectangle { required property int index; x: index * (parent.width - 1) / 12; width: 1; height: parent.height; color: "#1e2b36" }
                    }
                    ListView {
                        id: tracks; anchors.fill: parent; model: editor.nodes; clip: true; spacing: 6
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
                                Label { anchors.fill: parent; anchors.leftMargin: 10; text: track.modelData.name; color: "#dce8ed"; verticalAlignment: Text.AlignVCenter; elide: Text.ElideRight; font.pixelSize: 10 }
                                MouseArea {
                                    anchors.fill: parent
                                    property real originX: 0
                                    property int startFrame: 0
                                    onPressed: function(mouse) {
                                        track.forceActiveFocus()
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
                    Rectangle {
                        x: editor.frame / editor.duration * (parent.width - 1); y: 0; width: 1; height: parent.height; color: "#b4f078"
                        Rectangle { anchors.horizontalCenter: parent.horizontalCenter; width: 7; height: 7; radius: 2; color: "#b4f078" }
                    }
                }
            }
        }
    }
}
