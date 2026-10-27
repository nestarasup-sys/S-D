#include "editor_controller.hpp"
#include <QElapsedTimer>
#include <QFile>
#include <QGuiApplication>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QThread>
#include <algorithm>
#include <iostream>
#include <stdexcept>

static int checks = 0;
static void check(bool condition, const char *message) {
    ++checks;
    if (!condition)
        throw std::runtime_error(message);
}
int main(int argc, char **argv) {
    QStandardPaths::setTestModeEnabled(true);
    QGuiApplication app(argc, argv);
    QTemporaryDir directory;
    try {
        EditorController c(nullptr, directory.filePath("recovery"));
        check(!c.dirty() && c.selectedId() == "title", "initial selection and clean baseline");
        const auto badRecipe = directory.filePath("bad.production.json");
        {
            QFile file(badRecipe);
            check(file.open(QIODevice::WriteOnly) && file.write("{bad json") > 0,
                  "write intentionally malformed production");
        }
        c.importProduction(QUrl::fromLocalFile(badRecipe));
        check(!c.producing() && !c.latestProductionLog().isEmpty(),
              "invalid production creates a diagnostic log without starting a worker");
        QFile diagnostic(c.latestProductionLog());
        check(diagnostic.open(QIODevice::ReadOnly), "diagnostic log exists");
        const auto logBytes = diagnostic.readAll();
        const auto entries = logBytes.split('\n');
        auto firstEntry = QJsonDocument::fromJson(entries[0]).object();
        auto secondEntry = QJsonDocument::fromJson(entries[1]).object();
        check(firstEntry["event"] == "import-start" && secondEntry["event"] == "recipe-invalid" &&
                  !logBytes.contains("{bad json"),
              "diagnostics contain event metadata and error, never raw recipe content");
        const auto *compositionGroup = studio::findNode(c.project(), "composition-board");
        const auto childLayers =
            std::count_if(c.project().nodes.begin(), c.project().nodes.end(),
                          [](const auto &node) { return node.parentId == "composition-board"; });
        check(c.nodeModel()->rowCount() == 4 && compositionGroup &&
                  compositionGroup->kind == studio::NodeKind::Group && childLayers == 3,
              "demo exposes three child layers and their composition group");
        int resets = 0;
        QObject::connect(c.nodeModel(), &QAbstractItemModel::modelReset, [&] { ++resets; });
        auto previous = c.selected()["x"].toDouble();
        c.beginGesture();
        for (int i = 1; i <= 15; ++i)
            c.moveSelected(previous + i, 250);
        c.endGesture();
        check(c.dirty() && c.canUndo(), "drag changes document");
        check(resets == 0, "moving objects preserves model row identity");
        c.undo();
        check(c.selected()["x"].toDouble() == previous && !c.dirty(), "undo gesture restores saved state");
        c.redo();
        check(c.selected()["x"].toDouble() == previous + 15, "redo controller gesture");
        c.selectNode("title");
        c.toggleNodeSelection("subtitle");
        check(c.selectedIds().size() == 2 && c.selectedId() == "subtitle",
              "timeline supports a primary item plus additive selection");
        c.beginGesture();
        c.moveSelectedClips("title", 15, 5);
        c.endGesture();
        check(studio::findNode(c.project(), "title")->start == 5 &&
                  studio::findNode(c.project(), "subtitle")->start == 35,
              "multi-selection moves clips together while preserving their offset");
        c.undo();
        check(studio::findNode(c.project(), "title")->start == 15 &&
                  studio::findNode(c.project(), "subtitle")->start == 45,
              "one undo restores all moved clips");
        c.redo();
        check(studio::findNode(c.project(), "title")->start == 5 &&
                  studio::findNode(c.project(), "subtitle")->start == 35,
              "one redo reapplies the complete multi-clip move");
        c.beginGesture();
        c.moveSelectedClips("title", 5, -1000);
        c.endGesture();
        check(studio::findNode(c.project(), "title")->start == 0 &&
                  studio::findNode(c.project(), "subtitle")->start == 30,
              "multi-selection clamps to the tightest clip boundary without changing offsets");
        c.undo();
        check(studio::findNode(c.project(), "title")->start == 5 &&
                  studio::findNode(c.project(), "subtitle")->start == 35,
              "undo restores the multi-selection from its clamped boundary");
        c.redo();
        c.selectNode("title");
        auto doc = studio::serialize(c.project());
        c.updateSelected("duration", -10);
        check(studio::serialize(c.project()) == doc, "invalid inspector edit preserves document");
        c.setFrame(10000);
        check(c.frame() == c.duration() - 1, "scrubbing clamps end");
        c.setFrame(-5);
        check(c.frame() == 0, "scrubbing clamps start");
        auto count = c.nodeModel()->rowCount();
        c.addNode("shape");
        check(c.nodeModel()->rowCount() == count + 1 && c.selected()["kind"] == "shape",
              "adding selects new object");
        c.removeSelected();
        check(c.nodeModel()->rowCount() == count && c.selected().isEmpty(), "deleting clears selection");
        c.undo();
        check(c.nodeModel()->rowCount() == count + 1, "undo deletion restores object");
        c.applyEditingIntensity(80);
        check(studio::findNode(c.project(), "title")->motion == "pop" &&
                  studio::findNode(c.project(), "title")->motionFrames == 16 &&
                  studio::findNode(c.project(), "title")->exitFrames == 6,
              "global intensity applies a validated motion profile to the scene");
        c.undo();
        const bool intensityUndoRestored = studio::findNode(c.project(), "title")->motion != "pop";
        c.redo();
        check(intensityUndoRestored && studio::findNode(c.project(), "title")->motion == "pop",
              "global intensity is one undoable editing action");
        c.updateArtDirection("backgroundColor", "#17202a");
        c.updateArtDirection("captionPosition", "top");
        check(c.artDirection()["backgroundColor"] == "#17202a" &&
                  c.artDirection()["captionPosition"] == "top",
              "global art direction inspector updates project styling");
        const auto artBeforeInvalid = studio::serialize(c.project());
        c.updateArtDirection("backgroundColor", "not-a-color");
        check(studio::serialize(c.project()) == artBeforeInvalid,
              "invalid global art direction preserves the project");
        auto path = QUrl::fromLocalFile(directory.filePath("saved.studio.json"));
        check(c.save(path) && !c.dirty(), "save controller project");
        c.selectNode("title");
        c.updateSelected("text", "edited title");
        check(c.save() && !c.dirty(), "save current path with expected revision");
        c.newProject();
        check(c.nodeModel()->rowCount() == 0, "new blank project");
        c.open(path);
        check(studio::findNode(c.project(), "title")->text == "edited title" &&
                  c.project().artDirection.backgroundColor == "#17202a" &&
                  c.project().artDirection.captionPosition == "top",
              "reopen retains content and global art direction");
        c.selectNode("title");
        c.updateSelected("text", "recovered title");
        QElapsedTimer elapsed;
        elapsed.start();
        while (elapsed.elapsed() < 1500) {
            app.processEvents();
            QThread::msleep(10);
        }
        check(c.recoveryAvailable(), "autosave writes recovery");
        c.newProject();
        c.recover();
        check(studio::findNode(c.project(), "title")->text == "recovered title" && c.dirty(),
              "recovery restores unsaved content");
        // Easing is editable at existing timeline keyframes and retains its choice on edit.
        c.selectNode("title");
        c.setFrame(45);
        c.recordSelectedKeyframe();
        c.setSelectedKeyframeEasing("ease-in-out");
        check(c.selected()["keyframeEasing"] == "ease-in-out", "inspector writes selected easing");
        c.updateSelected("x", 241);
        check(c.selected()["keyframeEasing"] == "ease-in-out",
              "editing animated position does not erase easing");
        c.updateCamera("x", 700);
        c.setCameraKeyframeEasing("ease-out");
        check(c.camera()["keyframeEasing"] == "ease-out", "inspector writes camera easing");
        c.updateCamera("zoom", 1.25);
        check(c.camera()["keyframeEasing"] == "ease-out", "camera update preserves easing");
        auto shotProject = studio::makeEmpty();
        shotProject.schemaVersion = 9;
        shotProject.duration = 120;
        shotProject.scenes = {{"story", "História", "", 0, 120}};
        shotProject.shots = {{"wide", "story", "Aberto", "cut", 0, 60},
                             {"detail", "story", "Detalhe", "cut", 60, 60}};
        studio::Node wideNode;
        wideNode.id = "wide-node";
        wideNode.sceneId = "story";
        wideNode.shotId = "wide";
        wideNode.duration = 50;
        studio::Node detailNode;
        detailNode.id = "detail-node";
        detailNode.sceneId = "story";
        detailNode.shotId = "detail";
        detailNode.start = 60;
        detailNode.duration = 60;
        studio::Node shotCaption;
        shotCaption.id = "shot-caption";
        shotCaption.name = "Legenda da narração";
        shotCaption.sceneId = "story";
        shotCaption.role = "caption";
        shotCaption.text = "Uma palavra.";
        shotCaption.duration = 120;
        shotCaption.x = 80;
        shotCaption.y = 610;
        shotCaption.width = 1120;
        shotCaption.height = 80;
        shotCaption.captionWords = {{"Uma", 0, 45}, {"palavra.", 45, 120}};
        shotProject.nodes = {wideNode, detailNode, shotCaption};
        shotProject.visualBeats = {{"detail-beat", "detail", "detail-node", "show", 75, 0},
                                   {"detail-chime", "detail", "sfx-detail-chime", "sound", 75, 0}};
        shotProject.audioTracks = {{"sfx-detail-chime", "Efeito: detail-chime", "C:/media/chime.wav",
                                    75, 45, .65}};
        shotProject.camera = {{59, 640, 360, 1, 0, "linear", "detail"},
                              {60, 700, 360, 1.1, 0, "linear", "detail"},
                              {90, 720, 360, 1.2, 0, "linear", "detail"}};
        check(studio::validate(shotProject).empty(), "shot edit fixture validates");
        const auto shotPath = QUrl::fromLocalFile(directory.filePath("shots.studio.json"));
        check(studio::saveProject(shotPath.toLocalFile(), shotProject, {}).error.isEmpty() &&
                  c.open(shotPath),
              "open a schema 9 project with independent shots and a linked sound beat");
        c.selectNode("wide-node");
        c.beginGesture();
        const auto snappedClip = c.snapClipMove("wide-node", 0, 9, 2);
        const auto unsnappedClip = c.snapClipMove("wide-node", 0, 20, 2);
        check(snappedClip["start"].toInt() == 10 && snappedClip["guideFrame"].toInt() == 60,
              "timeline snap aligns a clip edge to a shot boundary within frame tolerance");
        check(unsnappedClip["start"].toInt() == 20 && unsnappedClip["guideFrame"].toInt() == -1,
              "timeline snap leaves distant clip moves untouched");
        const auto snappedAudio = c.snapAudioTrackMove("sfx-detail-chime", 75, 59, 45, 2);
        const auto unsnappedAudio = c.snapAudioTrackMove("sfx-detail-chime", 75, 20, 45, 2);
        check(snappedAudio["start"].toInt() == 60 && snappedAudio["guideFrame"].toInt() == 60,
              "audio clip snapping aligns to a shot edge and skips its own linked sound beat");
        check(unsnappedAudio["start"].toInt() == 20 && unsnappedAudio["guideFrame"].toInt() == -1,
              "audio clip snapping leaves distant moves untouched");
        const auto snappedAudioTrim = c.snapAudioTrackTrim("sfx-detail-chime", 75, 45, 43, 2);
        const auto unsnappedAudioTrim = c.snapAudioTrackTrim("sfx-detail-chime", 75, 45, 20, 2);
        check(snappedAudioTrim["length"].toInt() == 45 && snappedAudioTrim["guideFrame"].toInt() == 120,
              "audio trim snapping aligns its end to a shot boundary within tolerance");
        check(unsnappedAudioTrim["length"].toInt() == 20 && unsnappedAudioTrim["guideFrame"].toInt() == -1,
              "audio trim snapping leaves a distant end untouched");
        c.endGesture();
        c.beginGesture();
        c.setAudioTrackTiming("sfx-detail-chime", 60, 45);
        c.endGesture();
        check(c.project().audioTracks.front().start == 60 && c.project().visualBeats.back().frame == 60,
              "moving an audio clip keeps its linked sound beat at the same frame");
        c.undo();
        check(c.project().audioTracks.front().start == 75 && c.project().visualBeats.back().frame == 75,
              "audio clip movement and linked beat undo together");
        c.beginGesture();
        c.setAudioTrackTiming("sfx-detail-chime", 75, 43);
        c.endGesture();
        check(c.project().audioTracks.front().start == 75 && c.project().audioTracks.front().duration == 43 &&
                  c.project().visualBeats.back().frame == 75,
              "audio edge trim preserves its start and keeps its sound marker linked");
        c.undo();
        check(c.project().audioTracks.front().duration == 45,
              "audio edge trim is one undoable gesture");
        check(c.audioTracks().front().toMap()["gainEnvelope"].toList().size() == 97,
              "audio track model exposes a sampled envelope for the timeline");
        int audioTrackResets = 0;
        QObject::connect(c.audioTrackModel(), &QAbstractItemModel::modelReset,
                         [&] { ++audioTrackResets; });
        c.beginGesture();
        c.setAudioTrackGainPointAt("sfx-detail-chime", 10, .4, "ease-out");
        c.setAudioTrackGainPointAt("sfx-detail-chime", 15, .9, "ease-in");
        c.endGesture();
        check(c.project().audioTracks.front().gainAutomation.size() == 2 &&
                  c.project().audioTracks.front().gainAutomation[0].frame == 10 &&
                  c.project().audioTracks.front().gainAutomation[0].easing == "ease-out" &&
                  c.project().audioTracks.front().gainAutomation[1].frame == 15 &&
                  audioTrackResets == 0,
              "timeline envelope edits store sorted points without resetting the active audio row");
        c.undo();
        check(c.project().audioTracks.front().gainAutomation.empty(),
              "one envelope drag gesture is one undoable edit");
        c.redo();
        c.setFrame(80);
        c.setArtDirectionScope("scene");
        c.updateArtDirection("backgroundColor", "#112233");
        check(c.project().schemaVersion == 11 && c.project().scenes.front().artDirection &&
                  c.artDirection()["backgroundColor"] == "#112233",
              "art direction inspector edits the active scene scope");
        c.setArtDirectionScope("shot");
        c.updateArtDirection("primaryColor", "#ff33aa");
        check(c.project().shots.back().artDirection &&
                  c.project().shots.back().artDirection->backgroundColor == "#112233" &&
                  c.artDirection()["primaryColor"] == "#ff33aa",
              "shot style inherits the active scene and saves its local override");
        c.selectNode("shot-caption");
        check(c.selected()["role"] == "caption" && c.selected()["wordAligned"].toBool(),
              "inspector identifies generated captions with real word timings");
        const auto wordsBeforeEdit = c.selected()["captionWords"].toList();
        check(wordsBeforeEdit.size() == 2 && wordsBeforeEdit[0].toMap()["start"].toInt() == 0 &&
                  wordsBeforeEdit[0].toMap()["end"].toInt() == 45 &&
                  wordsBeforeEdit[1].toMap()["start"].toInt() == 45,
              "inspector exposes absolute frame intervals for aligned caption words");
        check(c.setCaptionWordBoundary(1, "start", 50) &&
                  studio::findNode(c.project(), "shot-caption")->captionWords[0].end == 50 &&
                  studio::findNode(c.project(), "shot-caption")->captionWords[1].start == 50,
              "moving a caption word start snaps the shared neighboring boundary together");
        c.undo();
        check(studio::findNode(c.project(), "shot-caption")->captionWords[0].end == 45 &&
                  studio::findNode(c.project(), "shot-caption")->captionWords[1].start == 45,
              "caption timing boundary edit is undoable as one edit");
        c.redo();
        c.beginGesture();
        c.setCaptionWordBoundary(1, "start", 52);
        c.setCaptionWordBoundary(1, "start", 55);
        c.endGesture();
        check(studio::findNode(c.project(), "shot-caption")->captionWords[0].end == 55 &&
                  studio::findNode(c.project(), "shot-caption")->captionWords[1].start == 55,
              "dragging a caption boundary updates its live snapped frame");
        c.undo();
        check(studio::findNode(c.project(), "shot-caption")->captionWords[1].start == 50,
              "an entire caption boundary drag is one undoable gesture");
        c.redo();
        check(!c.setCaptionWordBoundary(1, "start", 120) &&
                  studio::findNode(c.project(), "shot-caption")->captionWords[1].start == 55,
              "caption timing rejects boundaries that would erase a word interval");
        c.updateSelected("wordAnimationPreset", "pop-rise");
        c.updateSelected("wordAnimationDurationFrames", 6);
        c.updateSelected("wordAnimationStartScale", .72);
        c.updateSelected("wordAnimationDurationFrames", 6.5);
        check(c.project().schemaVersion == 12 &&
                  studio::findNode(c.project(), "shot-caption")->wordAnimationPreset == "pop-rise" &&
                  studio::findNode(c.project(), "shot-caption")->wordAnimationDurationFrames == 6 &&
                  studio::findNode(c.project(), "shot-caption")->wordAnimationStartScale == .72 &&
                  c.message().contains("inteiros"),
              "caption inspector stores editable word animation parameters in schema 12");
        c.setArtDirectionScope("global");
        c.updateShotTransition("detail", "dissolve", 15, "left");
        check(c.project().shots[1].transitionIn == "dissolve" &&
                  c.project().shots[1].transitionDurationFrames == 15,
              "shot inspector updates a validated incoming transition");
        c.beginGesture();
        c.resizeShotTransition("detail", 1);
        c.resizeShotTransition("detail", 28);
        c.endGesture();
        check(c.project().shots[1].transitionDurationFrames == 28,
              "timeline overlap resize clamps to valid frame durations");
        c.undo();
        check(c.project().shots[1].transitionDurationFrames == 15,
              "transition overlap drag undoes as one gesture");
        c.redo();
        c.selectNode("detail-node");
        c.groupSelected();
        const auto groupedShotNode = studio::findNode(c.project(), c.selectedId().toStdString());
        check(groupedShotNode && groupedShotNode->shotId == "detail",
              "grouping an element preserves its shot ownership");
        const auto groupedId = c.selectedId();
        c.setFrame(80);
        c.updateCamera("zoom", 1.3);
        const auto ownedCamera = std::find_if(c.project().camera.begin(), c.project().camera.end(),
                                              [](const auto &key) { return key.frame == 80; });
        check(ownedCamera != c.project().camera.end() && ownedCamera->shotId == "detail",
              "camera inspector edits create shot-owned camera keyframes");
        c.beginGesture();
        c.moveShotBoundary("detail", 70);
        c.endGesture();
        check(c.project().shots[0].duration == 70 && c.project().shots[1].start == 70 &&
                  c.project().shots[1].duration == 50 &&
                  studio::findNode(c.project(), "detail-node")->start == 70 &&
                  studio::findNode(c.project(), groupedId.toStdString())->start == 70 &&
                  std::any_of(c.project().camera.begin(), c.project().camera.end(),
                              [](const auto &key) { return key.frame == 90 && key.shotId == "detail"; }) &&
                  c.project().visualBeats.front().frame == 85 && c.project().camera[1].frame == 70,
              "dragging a shot cut moves its duration, contents, beats and camera together");
        c.undo();
        check(c.project().shots[1].start == 60 && c.project().visualBeats.front().frame == 75 &&
                  c.project().camera[1].frame == 60 &&
                  c.project().audioTracks.front().start == 75 && c.project().audioTracks.front().duration == 45 &&
                  std::any_of(c.project().camera.begin(), c.project().camera.end(),
                              [](const auto &key) { return key.frame == 80 && key.shotId == "detail"; }),
              "one shot-cut gesture undoes as one action");
        const auto soundBeatAt = [&] {
            const auto it = std::find_if(c.project().visualBeats.begin(), c.project().visualBeats.end(),
                                         [](const auto &beat) { return beat.action == "sound"; });
            return it == c.project().visualBeats.end() ? -1 : int(it->frame);
        };
        check(soundBeatAt() == 75, "undo restores the linked sound marker");
        c.setAudioTrackTiming("sfx-detail-chime", 80, 30);
        check(soundBeatAt() == 80 && c.project().audioTracks.front().start == 80,
              "moving a one-shot effect moves its Visual Beat marker");
        c.beginGesture();
        c.moveShotBoundary("detail", 70);
        c.endGesture();
        check(soundBeatAt() == 90 && c.project().audioTracks.front().start == 90 &&
                  c.project().audioTracks.front().duration == 20,
              "moving a shot boundary keeps its sound track and beat synchronized");
        c.undo();
        c.removeAudioTrack("sfx-detail-chime");
        check(soundBeatAt() == -1 && c.project().audioTracks.empty(),
              "removing a sound effect track also removes its Visual Beat marker");
        c.setFrame(c.duration() - 1);
        c.togglePlayback();
        check(c.playing() && c.frame() == 0, "play at end restarts");
        c.togglePlayback();
        check(!c.playing(), "pause stops playback");
        c.setFrame(0);
        c.togglePlayback();
        elapsed.restart();
        while (elapsed.elapsed() < 100) {
            app.processEvents();
            QThread::msleep(5);
        }
        check(c.playing() && c.frame() > 0, "playback advances with elapsed time");
        c.setFrame(90);
        check(!c.playing() && c.frame() == 90, "scrubbing pauses at requested frame");
        std::cout << "PASS: " << checks
                  << " controller checks (model identity, editing, persistence, recovery, playback)\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL after " << checks << " checks: " << error.what() << '\n';
        return 1;
    }
}
