#include <QFile>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <iostream>
#include <stdexcept>
#include <studio/composition/painter.hpp>
#include <studio/core/editor.hpp>
#include <studio/storage/project_store.hpp>

static int checks = 0;
static void check(bool condition, const char *message) {
    ++checks;
    if (!condition)
        throw std::runtime_error(message);
}
int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    try {
        auto p = studio::makeDemo();
        studio::Editor editor(p);
        check(studio::validate(p).empty(), "demo must validate");
        auto baseline = studio::serialize(editor.project());
        check(!editor.execute([](auto &c) { c.nodes[0].start = -1; }).empty(), "reject negative start");
        check(studio::serialize(editor.project()) == baseline && !editor.canUndo(),
              "invalid command must not mutate history");
        check(!editor.execute([](auto &c) { c.nodes[1].id = c.nodes[0].id; }).empty(), "reject duplicate ID");
        check(!editor.execute([](auto &c) { c.nodes[0].opacity = 2; }).empty(), "reject opacity overflow");
        editor.beginGesture();
        for (int i = 0; i < 20; ++i)
            editor.execute([i](auto &c) { c.nodes[0].x = 200 + i; });
        editor.endGesture();
        check(editor.undo(), "gesture undo");
        check(studio::serialize(editor.project()) == baseline, "one undo restores entire drag");
        check(editor.redo() && editor.project().nodes[0].x == 219, "redo entire drag");
        editor.undo();
        editor.execute([](auto &c) { c.nodes[0].y = 90; });
        check(!editor.canRedo(), "new edit invalidates redo");
        auto clip = p.nodes[1];
        check(!studio::active(clip, clip.start - 1), "clip start exclusive outside");
        check(studio::active(clip, clip.start), "clip starts at first frame");
        check(!studio::active(clip, clip.start + clip.duration), "clip end exclusive");
        check(studio::evaluate(clip, clip.start).opacity == 0, "animated entrance first frame");
        check(studio::evaluate(clip, clip.start + 24).opacity == clip.opacity, "animated entrance settles");
        // Incoming keyframe controls the segment easing; older files remain linear.
        studio::Node animated;
        animated.id = "curve-test";
        animated.animated = false;
        animated.duration = 90;
        animated.keyframes = {{0, 0, 0, 0, 1, 1}, {60, 100, 0, 0, 1, 1, "ease-in"}};
        p.nodes.push_back(animated);
        p.camera = {{0, 640, 360, 1, 0}, {60, 740, 360, 2, 0, "ease-out"}};
        check(studio::supportedEasing("linear") && studio::supportedEasing("ease-in") &&
                  studio::supportedEasing("ease-out") && studio::supportedEasing("ease-in-out") &&
                  !studio::supportedEasing("bounce"),
              "all four named keyframe easing curves are supported");
        check(std::abs(studio::evaluate(animated, 30).x - 12.5) < .001,
              "incoming ease-in uses cubic interpolation");
        check(std::abs(studio::evaluateCamera(p, 30).x - 727.5) < .001,
              "camera ease-out uses cubic interpolation");
        check(studio::serialize(studio::deserialize(studio::serialize(p)).project) == studio::serialize(p),
              "eased keyframes survive schema 3 roundtrip");
        auto badCurve = p;
        badCurve.camera[1].easing = "spring-unknown";
        check(!studio::validate(badCurve).empty(), "reject unsupported camera curve");
        badCurve = p;
        badCurve.nodes.back().keyframes[1].easing = "spring-unknown";
        check(!studio::validate(badCurve).empty(), "reject unsupported object curve");
        p.fps = {30000, 1001};
        check(studio::validate(p).empty(), "fractional fps supported");
        check(studio::serialize(studio::deserialize(studio::serialize(p)).project) == studio::serialize(p),
              "round trip rational fps and document");
        auto malformed = QJsonDocument::fromJson(studio::serialize(p)).object();
        malformed["schemaVersion"] = 5;
        check(!studio::deserialize(QJsonDocument(malformed).toJson()).error.isEmpty(),
              "reject future schema");
        auto shotProject = studio::makeEmpty();
        shotProject.schemaVersion = 4;
        shotProject.duration = 120;
        shotProject.scenes = {{"story", "História", "Uma fala", 0, 120}};
        shotProject.shots = {{"wide", "story", "Plano aberto", "cut", 0, 60},
                             {"detail", "story", "Detalhe", "cut", 60, 60}};
        studio::Node shotNode;
        shotNode.id = "price";
        shotNode.name = "Preço";
        shotNode.sceneId = "story";
        shotNode.shotId = "detail";
        shotNode.start = 60;
        shotNode.duration = 60;
        shotProject.nodes = {shotNode};
        shotProject.visualBeats = {{"reveal", "detail", "price", "show", 72, 0}};
        check(studio::validate(shotProject).empty(), "valid independent shots and beat");
        auto persistedShots = studio::deserialize(studio::serialize(shotProject));
        check(persistedShots.error.isEmpty() && persistedShots.project.shots.size() == 2 &&
                  persistedShots.project.visualBeats.size() == 1 &&
                  persistedShots.project.nodes.front().shotId == "detail",
              "shots, beat and node ownership survive schema 4 roundtrip");
        auto brokenShots = shotProject;
        brokenShots.shots[1].start = 59;
        check(!studio::validate(brokenShots).empty(), "reject overlapping visual shots");
        check(!studio::deserialize("{broken").error.isEmpty(), "reject broken JSON");
        QTemporaryDir directory;
        auto path = directory.filePath("project.studio.json");
        auto save = studio::saveProject(path, p, {});
        check(save.error.isEmpty(), "initial atomic save");
        check(!studio::saveProject(path, p, {}).error.isEmpty(), "new file save never silently overwrites");
        auto loaded = studio::loadProject(path);
        check(loaded.error.isEmpty() && loaded.revision == save.revision, "load revision");
        auto modified = p;
        modified.name = "Changed";
        auto updated = studio::saveProject(path, modified, save.revision);
        check(updated.error.isEmpty(), "save known revision");
        check(studio::loadProject(path + ".previous").project.name == p.name, "previous snapshot preserved");
        check(!studio::saveProject(path, p, save.revision).error.isEmpty(), "reject stale revision");
        check(studio::loadProject(path).project.name == "Changed", "stale save preserves current bytes");
        studio::CompositionPainter painter;
        auto first = painter.frame(p, 0), later = painter.frame(p, 90);
        check(first.size() == QSize(p.width, p.height) && first != later, "animated frames differ");
        auto same = painter.frame(p, 90);
        check(same == later, "frame evaluation deterministic");
        auto missing = p;
        studio::Node image;
        image.id = "missing";
        image.kind = studio::NodeKind::Image;
        image.source = "missing.png";
        missing.nodes.push_back(image);
        check(!painter.frame(missing, 90).isNull(), "missing image preview does not crash");
        std::cout << "PASS: " << checks
                  << " checks (commands, timing, persistence, conflicts, composition)\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL after " << checks << " checks: " << error.what() << '\n';
        return 1;
    }
}
