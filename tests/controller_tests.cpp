#include "editor_controller.hpp"
#include <QElapsedTimer>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QGuiApplication>
#include <QTemporaryDir>
#include <QThread>
#include <iostream>
#include <stdexcept>

static int checks = 0;
static void check(bool condition, const char *message) {
    ++checks;
    if (!condition)
        throw std::runtime_error(message);
}
int main(int argc, char **argv) {
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
        check(c.nodeModel()->rowCount() == 3, "three initial rows");
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
        auto path = QUrl::fromLocalFile(directory.filePath("saved.studio.json"));
        check(c.save(path) && !c.dirty(), "save controller project");
        c.selectNode("title");
        c.updateSelected("text", "edited title");
        check(c.save() && !c.dirty(), "save current path with expected revision");
        c.newProject();
        check(c.nodeModel()->rowCount() == 0, "new blank project");
        c.open(path);
        check(studio::findNode(c.project(), "title")->text == "edited title", "reopen retains content");
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
