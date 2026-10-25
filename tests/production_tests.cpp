#include <QFile>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <studio/composition/painter.hpp>
#include <studio/core/library.hpp>
#include <studio/production/compiler.hpp>
#include <studio/storage/project_store.hpp>
#include <studio/storage/workspace_store.hpp>

static int checks = 0;
static void check(bool value, const char *reason) {
    ++checks;
    if (!value)
        throw std::runtime_error(reason);
}
static double sampledImageDifference(const QImage &left, const QImage &right) {
    if (left.size() != right.size())
        return std::numeric_limits<double>::infinity();
    double total = 0;
    int samples = 0;
    for (int y = 0; y < left.height(); y += 8)
        for (int x = 0; x < left.width(); x += 8) {
            const auto a = left.pixelColor(x, y);
            const auto b = right.pixelColor(x, y);
            total +=
                std::abs(a.red() - b.red()) + std::abs(a.green() - b.green()) + std::abs(a.blue() - b.blue());
            ++samples;
        }
    return total / std::max(1, samples * 3);
}
int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    QTemporaryDir directory;
    try {
        QFile fixture(QString(STUDIO_SOURCE_DIR) + "/examples/carros-caros.production.json");
        check(fixture.open(QIODevice::ReadOnly), "example exists");
        auto bytes = fixture.readAll();
        auto parsed = studio::parseRecipe(bytes, directory.path());
        check(parsed.error.isEmpty(), "example parses with omitted optional fields");
        check(parsed.recipe.scenes.size() == 6, "six narrative scenes");
        const QByteArray invalidSyntax = QByteArray("{") + '\n' + "bad:";
        const auto syntaxError = studio::parseRecipe(invalidSyntax, directory.path()).error;
        check(syntaxError.contains("linha 2") && syntaxError.contains("coluna"),
              "JSON syntax errors return the correct line and column");
        auto root = QJsonDocument::fromJson(bytes).object();
        auto reject = [&](QJsonObject value) {
            return !studio::parseRecipe(QJsonDocument(value).toJson(), directory.path()).error.isEmpty();
        };
        auto wrong = root;
        wrong["catalogVersion"] = 99;
        check(reject(wrong), "reject unknown catalogue");
        wrong = root;
        wrong["surprise"] = true;
        check(reject(wrong), "reject unknown root field");
        auto changeElement = [&](const QString &key, QJsonValue value) {
            auto o = root;
            auto scenes = o["scenes"].toArray();
            auto scene = scenes[0].toObject();
            auto elements = scene["elements"].toArray();
            auto e = elements[0].toObject();
            e[key] = value;
            elements[0] = e;
            scene["elements"] = elements;
            scenes[0] = scene;
            o["scenes"] = scenes;
            return o;
        };
        check(reject(changeElement("asset", "flying-car")), "reject invented library item");
        check(reject(changeElement("motion", "teleport")), "reject invented motion");
        check(reject(changeElement("pose", "walking")), "reject unsupported pose");
        check(reject(changeElement("cue", 44)), "reject out-of-range cue");
        check(reject(changeElement("cue", .5)), "reject fractional cue");
        check(reject(changeElement("span", "forever")), "reject unsupported span");
        check(reject(changeElement("opacity", 2)), "reject invalid visual property");
        check(reject(changeElement("x", "left")), "reject string coordinate");
        check(reject(changeElement("unknown", true)), "reject unknown element property");
        QMap<QString, studio::PcmAudio> speech;
        int index = 0;
        for (const auto &s : parsed.recipe.scenes)
            for (const auto &u : s.narration) {
                studio::PcmAudio a;
                a.samples.resize(4800 + (index++ % 2) * 2400);
                for (std::size_t i = 0; i < a.samples.size(); ++i)
                    a.samples[i] = float(.2 * std::sin(i * .08));
                speech.insert(u.id, a);
            }
        auto compiled = studio::compileProduction(parsed.recipe, speech, directory.filePath("master.wav"));
        check(compiled.error.isEmpty(), "compile with actual audio sample counts");
        check(studio::validate(compiled.project).empty(), "compiled project validates");
        check(compiled.project.scenes.size() == 6, "scene markers preserved");
        check(compiled.voice.samples.size() == std::size_t(compiled.project.duration * 800),
              "audio rounded to exact video frame boundary");
        check(compiled.mix.samples != compiled.voice.samples, "music and effects mixed");
        check(compiled.subtitles.contains("00:00:00,000 -->"), "SRT uses measured start");
        check(studio::findNode(compiled.project, "etiqueta")->start > compiled.project.scenes[0].start,
              "cue one enters after cue zero audio");
        auto mutedRecipe = parsed.recipe;
        mutedRecipe.musicGain = 0;
        mutedRecipe.effectsGain = 0;
        auto muted = studio::compileProduction(mutedRecipe, speech, directory.filePath("muted.wav"));
        check(muted.mix.samples == muted.voice.samples, "zero gains mute procedural soundtrack");
        const QJsonObject priceElement{{"id", "price"},      {"kind", "text"}, {"text", "BARATO?"},
                                       {"x", 100},           {"y", 50},        {"width", 400},
                                       {"height", 100},      {"fontSize", 64}, {"motion", "rise"},
                                       {"motionFrames", 42}, {"exitFrames", 9}};
        const QJsonObject detailGroup{{"id", "detail-group"},
                                      {"kind", "group"},
                                      {"x", 300},
                                      {"y", 100},
                                      {"width", 800},
                                      {"height", 300},
                                      {"children", QJsonArray{priceElement}}};
        const QJsonObject wideShot{
            {"id", "wide"},
            {"name", "Plano aberto"},
            {"start", QJsonObject{{"cue", 0}, {"progress", 0}}},
            {"camera", QJsonObject{{"x", 640},
                                   {"y", 360},
                                   {"zoom", 1},
                                   {"rotation", 0},
                                   {"focus", QJsonObject{{"target", "car-shape"}, {"follow", true}}}}},
            {"elements", QJsonArray{QJsonObject{{"id", "car-shape"},
                                                {"kind", "shape"},
                                                {"x", 140},
                                                {"y", 300},
                                                {"width", 500},
                                                {"height", 180},
                                                {"color", "#b4f078"}}}}};
        const QJsonObject detailShot{
            {"id", "price-cut"},
            {"name", "Revelação do preço"},
            {"start", QJsonObject{{"cue", 0}, {"progress", 0.5}}},
            {"transitionIn", QJsonObject{{"type", "dissolve"}, {"durationFrames", 3}}},
            {"camera", QJsonObject{{"x", 650},
                                   {"y", 350},
                                   {"zoom", 1.25},
                                   {"rotation", 30},
                                   {"focus", QJsonObject{{"target", "price"}, {"padding", 1.2}}}}},
            {"elements", QJsonArray{detailGroup}},
            {"beats", QJsonArray{QJsonObject{{"id", "reveal-price"},
                                             {"at", QJsonObject{{"cue", 0}, {"progress", 0.7}}},
                                             {"action", "show"},
                                             {"target", "price"}},
                                 QJsonObject{{"id", "emphasize-price"},
                                             {"at", QJsonObject{{"cue", 0}, {"progress", 0.8}}},
                                             {"action", "transform"},
                                             {"target", "price"},
                                             {"durationFrames", 2},
                                             {"scale", 1.2}},
                                 QJsonObject{{"id", "camera-push"},
                                             {"at", QJsonObject{{"cue", 0}, {"progress", 0.9}}},
                                             {"action", "camera"},
                                             {"durationFrames", 2},
                                             {"zoom", 1.4}}}}};
        QJsonObject shotRecipeJson{
            {"format", "studio-dark-production"},
            {"version", 3},
            {"catalogVersion", 1},
            {"title", "Cortes no mesmo trecho"},
            {"scenes", QJsonArray{QJsonObject{
                           {"id", "car-story"},
                           {"narration", QJsonArray{"O carro parecia barato, mas havia uma surpresa."}},
                           {"shots", QJsonArray{wideShot, detailShot}}}}}};
        auto parsedShots = studio::parseRecipe(QJsonDocument(shotRecipeJson).toJson(), directory.path());
        check(parsedShots.error.isEmpty() && parsedShots.recipe.scenes.front().shots.size() == 2 &&
                  parsedShots.recipe.scenes.front().shots.back().beats.size() == 3 &&
                  parsedShots.recipe.scenes.front().shots.back().transitionIn == "dissolve" &&
                  parsedShots.recipe.scenes.front().shots.back().transitionDurationFrames == 3,
              "v3 parses independent shots, Visual Beats, and transition timing");
        auto highIntensityJson = shotRecipeJson;
        highIntensityJson["editing"] = QJsonObject{{"intensity", 90}};
        const auto highIntensity =
            studio::parseRecipe(QJsonDocument(highIntensityJson).toJson(), directory.path());
        bool generatedMotionMatches = false;
        bool explicitMotionPreserved = false;
        if (highIntensity.error.isEmpty() && highIntensity.recipe.scenes.front().shots.size() == 2) {
            const auto &generatedMotion =
                highIntensity.recipe.scenes.front().shots.front().elements.front().node;
            const auto &explicitMotion =
                highIntensity.recipe.scenes.front().shots.back().elements.front().children.front().node;
            generatedMotionMatches = highIntensity.recipe.editingIntensity == 90 &&
                                     generatedMotion.motion == "pop" && generatedMotion.motionFrames == 12 &&
                                     generatedMotion.exitFrames == 5;
            explicitMotionPreserved = explicitMotion.motion == "rise" && explicitMotion.motionFrames == 42 &&
                                      explicitMotion.exitFrames == 9;
        }
        check(generatedMotionMatches, "editing intensity generates deterministic default motion settings");
        check(explicitMotionPreserved, "editing intensity preserves explicitly directed motion");
        highIntensityJson["editing"] = QJsonObject{{"intensity", 0}};
        const auto calmIntensity =
            studio::parseRecipe(QJsonDocument(highIntensityJson).toJson(), directory.path());
        bool calmDefaultsMatch = false;
        if (calmIntensity.error.isEmpty() && !calmIntensity.recipe.scenes.front().shots.empty()) {
            const auto &motion = calmIntensity.recipe.scenes.front().shots.front().elements.front().node;
            calmDefaultsMatch = motion.motion == "none" && !motion.animated && motion.exitFrames == 0;
        }
        check(calmDefaultsMatch, "editing intensity zero disables automatic motion and exit fade");
        highIntensityJson["editing"] = QJsonObject{{"intensity", 100}};
        const auto maxIntensity =
            studio::parseRecipe(QJsonDocument(highIntensityJson).toJson(), directory.path());
        bool maxDefaultsMatch = false;
        if (maxIntensity.error.isEmpty() && !maxIntensity.recipe.scenes.front().shots.empty()) {
            const auto &motion = maxIntensity.recipe.scenes.front().shots.front().elements.front().node;
            maxDefaultsMatch = motion.motion == "pop" && motion.motionFrames == 8 && motion.exitFrames == 3;
        }
        check(maxDefaultsMatch, "editing intensity one hundred uses the maximum motion profile");
        highIntensityJson["editing"] = QJsonObject{{"intensity", 101}};
        check(
            !studio::parseRecipe(QJsonDocument(highIntensityJson).toJson(), directory.path()).error.isEmpty(),
            "editing intensity rejects values outside 0 to 100");
        QFile shotFixture(QString(STUDIO_SOURCE_DIR) + "/examples/shots-visual-beats-v3.production.json");
        check(shotFixture.open(QIODevice::ReadOnly), "v3 shots example exists");
        const auto fileRecipe = studio::parseRecipe(shotFixture.readAll(), directory.path());
        check(fileRecipe.error.isEmpty() && fileRecipe.recipe.version == 3 &&
                  fileRecipe.recipe.scenes.front().shots.size() == 3,
              "documented multi-shot v3 fixture is importable");
        QMap<QString, studio::PcmAudio> shotAudio;
        studio::PcmAudio shortSpeech;
        shortSpeech.samples.resize(4800, .1f);
        shotAudio.insert(parsedShots.recipe.scenes.front().narration.front().id, shortSpeech);
        auto compiledShots = studio::compileProduction(parsedShots.recipe, shotAudio, "shots.wav");
        check(compiledShots.error.isEmpty() && compiledShots.project.schemaVersion == 4 &&
                  compiledShots.project.shots.size() == 2 && compiledShots.project.visualBeats.size() == 3,
              "shots and beats compile into editable schema 4 project markers");
        const auto persistedShots = studio::deserialize(studio::serialize(compiledShots.project));
        check(persistedShots.error.isEmpty() &&
                  persistedShots.project.shots.back().transitionIn == "dissolve" &&
                  persistedShots.project.shots.back().transitionDurationFrames == 3,
              "shot transition settings survive project save and load");
        check(studio::validate(compiledShots.project).empty() &&
                  studio::findNode(compiledShots.project, "price")->shotId == "price-cut" &&
                  studio::findNode(compiledShots.project, "price")->keyframes.size() >= 2,
              "beat transforms compile into shot-owned editable keyframes");
        check(std::abs(studio::evaluateCamera(compiledShots.project, 2).y -
                       studio::evaluateCamera(compiledShots.project, 0).y) > 1,
              "continuous shot focus follows animated target across frames");
        check(std::any_of(compiledShots.project.camera.begin(), compiledShots.project.camera.end(),
                          [](const auto &key) { return key.frame == 2; }),
              "continuous shot focus records the final frame of its span");
        studio::CompositionPainter transitionPainter;
        const auto beforeDissolve = transitionPainter.frame(compiledShots.project, 2);
        const auto dissolveStart = transitionPainter.frame(compiledShots.project, 3);
        const auto dissolveMiddle = transitionPainter.frame(compiledShots.project, 4);
        check(sampledImageDifference(beforeDissolve, dissolveStart) < 0.5,
              "dissolve starts on the outgoing shot without a visual jump");
        check(sampledImageDifference(dissolveStart, dissolveMiddle) > 0.5,
              "dissolve advances between its endpoints");
        double transitionBrightness = 0;
        for (int y = 0; y < dissolveMiddle.height(); y += 16)
            for (int x = 0; x < dissolveMiddle.width(); x += 16) {
                const auto color = dissolveMiddle.pixelColor(x, y);
                transitionBrightness += color.red() + color.green() + color.blue();
            }
        check(transitionBrightness > 100000,
              "dissolve keeps the composition visible instead of producing a black frame");
        auto fadeProject = compiledShots.project;
        fadeProject.shots.back().transitionIn = "fade";
        const auto fadeMiddle = transitionPainter.frame(fadeProject, 4);
        auto slideProject = compiledShots.project;
        slideProject.shots.back().transitionIn = "slide";
        const auto slideMiddle = transitionPainter.frame(slideProject, 4);
        slideProject.shots.back().transitionDirection = "right";
        const auto slideRightMiddle = transitionPainter.frame(slideProject, 4);
        auto zoomProject = compiledShots.project;
        zoomProject.shots.back().transitionIn = "zoom";
        const auto zoomMiddle = transitionPainter.frame(zoomProject, 4);
        check(sampledImageDifference(fadeMiddle, dissolveMiddle) > 0.5 &&
                  sampledImageDifference(slideMiddle, dissolveMiddle) > 0.5 &&
                  sampledImageDifference(zoomMiddle, dissolveMiddle) > 0.5 &&
                  sampledImageDifference(slideMiddle, slideRightMiddle) > 0.5,
              "fade, slide directions, zoom, and dissolve render distinct transition frames");
        const auto priceShot =
            std::find_if(compiledShots.project.shots.begin(), compiledShots.project.shots.end(),
                         [](const auto &shot) { return shot.id == "price-cut"; });
        const auto framedPrice = studio::evaluateCamera(compiledShots.project, priceShot->start);
        check(std::abs(framedPrice.x - 600) < .001 && std::abs(framedPrice.y - 256) < .001 &&
                  framedPrice.zoom > 2.0 && framedPrice.zoom < 2.2 &&
                  std::abs(framedPrice.rotation - 30) < .001,
              "auto reframe resolves the world bounds of a nested group element");
        check(studio::evaluateCamera(compiledShots.project, 7).zoom >= 1.4,
              "camera Visual Beat produces a timed zoom after its event");
        auto missing = speech;
        missing.remove(parsed.recipe.scenes[0].narration[0].id);
        check(!studio::compileProduction(parsed.recipe, missing, "audio.wav").error.isEmpty(),
              "missing narration blocks compilation");
        auto path = directory.filePath("audio.wav");
        check(studio::writeWave(path, compiled.mix).isEmpty(), "write PCM WAV");
        auto loaded = studio::readWave(path);
        check(loaded.error.isEmpty() && loaded.audio.samples.size() == compiled.mix.samples.size(),
              "read PCM WAV roundtrip");
        check(std::abs(loaded.audio.samples[100] - compiled.mix.samples[100]) < .0001,
              "PCM sample quantization bounded");
        auto serialized = studio::serialize(compiled.project);
        auto project = studio::deserialize(serialized);
        check(project.error.isEmpty() && studio::serialize(project.project) == serialized,
              "new schema roundtrip with scenes audio and native props");
        auto legacy = QJsonDocument::fromJson(studio::serialize(studio::makeEmpty())).object();
        legacy["schemaVersion"] = 1;
        legacy.remove("scenes");
        legacy.remove("audioSource");
        check(studio::deserialize(QJsonDocument(legacy).toJson()).project.schemaVersion == 2,
              "v1 migrates into v2 in memory");
        studio::CompositionPainter painter;
        for (const auto &asset : studio::nativeLibrary()) {
            auto p = studio::makeEmpty();
            studio::Node n;
            n.id = asset.id;
            n.kind = studio::NodeKind::Native;
            n.nativeId = asset.id;
            n.width = asset.width;
            n.height = asset.height;
            p.nodes.push_back(n);
            check(studio::validate(p).empty() && !painter.frame(p, 60).isNull(),
                  "native library asset renders");
        }
        studio::WorkspaceStore catalog(directory.filePath("catalog"));
        studio::Node group, part;
        group.id = "template-root";
        group.name = "Painel";
        group.kind = studio::NodeKind::Group;
        group.start = 30;
        group.duration = 90;
        group.animated = false;
        part.id = "template-part";
        part.name = "Selo";
        part.kind = studio::NodeKind::Shape;
        part.parentId = group.id;
        part.start = 40;
        part.duration = 70;
        part.keyframes = {{45, 15, 30, 0, 1, 1}, {65, 60, 30, 0, 1, 1}};
        auto grouped = catalog.addPreset("Painel IA", "objects", {group, part});
        check(grouped.error.isEmpty(), "store reusable group preset");
        auto production = QJsonObject{
            {"format", "studio-dark-production"},
            {"version", 2},
            {"catalogVersion", 1},
            {"title", "Composição de teste"},
            {"captions", false},
            {"musicGain", 0},
            {"effectsGain", 0},
            {"scenes", QJsonArray{QJsonObject{{"id", "cena"},
                                              {"narration", QJsonArray{"Testando uma composição animada."}},
                                              {"elements", QJsonArray{QJsonObject{{"id", "reuse"},
                                                                                  {"kind", "library"},
                                                                                  {"assetId", grouped.id},
                                                                                  {"cue", 0}}}}}}}};
        auto recipeWithGroup =
            studio::parseRecipe(QJsonDocument(production).toJson(), directory.path(), catalog.data());
        check(recipeWithGroup.error.isEmpty(), "parse JSON with grouped library asset");
        QMap<QString, studio::PcmAudio> groupVoice;
        studio::PcmAudio voice;
        voice.samples.resize(120000, 0);
        groupVoice.insert(recipeWithGroup.recipe.scenes[0].narration[0].id, voice);
        auto compiledGroup = studio::compileProduction(recipeWithGroup.recipe, groupVoice, "group.wav");
        check(compiledGroup.error.isEmpty() && compiledGroup.project.nodes.size() == 2,
              "compile grouped preset against measured audio");
        const auto *rootNode = studio::findNode(compiledGroup.project, "reuse");
        const auto *childNode = studio::findNode(compiledGroup.project, "reuse-preset-1");
        check(rootNode && childNode && childNode->parentId == rootNode->id && childNode->start == 10 &&
                  childNode->duration == 70 && childNode->keyframes.size() == 2 &&
                  childNode->keyframes.front().frame == 15,
              "group import retains parent, relative time and keyframes");
        check(studio::aiGuide(catalog.data()).contains("composição com 2 partes"),
              "guide includes grouped personal library item");
        auto shortened = groupVoice;
        shortened.begin().value().samples.resize(2400);
        check(!studio::compileProduction(recipeWithGroup.recipe, shortened, "short.wav").error.isEmpty(),
              "reject preset keyframe beyond measured narration scene");
        check(studio::capabilities()["productionCapabilities"].toArray().contains("reusable-group-presets"),
              "capabilities advertise reusable group assets");
        auto easedJson = QJsonObject{
            {"format", "studio-dark-production"},
            {"version", 2},
            {"catalogVersion", 1},
            {"title", "Curvas por cue"},
            {"captions", false},
            {"musicGain", 0},
            {"effectsGain", 0},
            {"scenes",
             QJsonArray{QJsonObject{
                 {"id", "curvas"},
                 {"narration", QJsonArray{"Uma curva suaviza a câmera."}},
                 {"elements", QJsonArray{QJsonObject{
                                  {"id", "shape-easing"},
                                  {"kind", "shape"},
                                  {"cue", 0},
                                  {"keyframes", QJsonArray{QJsonObject{{"cue", 0}, {"x", 0}, {"y", 30}},
                                                           QJsonObject{{"cue", 0},
                                                                       {"offsetSeconds", 1},
                                                                       {"x", 100},
                                                                       {"y", 30},
                                                                       {"easing", "ease-in"}}}}}}},
                 {"camera",
                  QJsonArray{QJsonObject{{"cue", 0}, {"x", 640}, {"y", 360}, {"zoom", 1}, {"rotation", 0}},
                             QJsonObject{{"cue", 0},
                                         {"offsetSeconds", 1},
                                         {"x", 740},
                                         {"y", 360},
                                         {"zoom", 1},
                                         {"rotation", 0},
                                         {"easing", "ease-out"}}}}}}}};
        auto easedRecipe = studio::parseRecipe(QJsonDocument(easedJson).toJson(), directory.path());
        check(easedRecipe.error.isEmpty(), "JSON accepts named easing curves");
        auto invalidEasing = easedJson;
        auto invalidScenes = invalidEasing["scenes"].toArray();
        auto invalidScene = invalidScenes[0].toObject();
        auto cameraKeys = invalidScene["camera"].toArray();
        auto badKey = cameraKeys[1].toObject();
        badKey["easing"] = "elastic-unknown";
        cameraKeys[1] = badKey;
        invalidScene["camera"] = cameraKeys;
        invalidScenes[0] = invalidScene;
        invalidEasing["scenes"] = invalidScenes;
        check(!studio::parseRecipe(QJsonDocument(invalidEasing).toJson(), directory.path()).error.isEmpty(),
              "unsupported easing rejected by strict JSON validator");
        QMap<QString, studio::PcmAudio> easedVoice;
        studio::PcmAudio enoughVoice;
        enoughVoice.samples.resize(120000, 0);
        easedVoice.insert(easedRecipe.recipe.scenes[0].narration[0].id, enoughVoice);
        auto easedProject = studio::compileProduction(easedRecipe.recipe, easedVoice, "easing.wav");
        check(easedProject.error.isEmpty() &&
                  studio::findNode(easedProject.project, "shape-easing")->keyframes[1].easing == "ease-in" &&
                  easedProject.project.camera[1].easing == "ease-out",
              "compiled visual and camera easing persisted");
        check(std::abs(studio::evaluate(*studio::findNode(easedProject.project, "shape-easing"), 15).x -
                       12.5) < .001,
              "production uses easing in compositor");
        // Fit-to-duration: two exact scene budgets, one utterance requiring <15% acceleration.
        QJsonObject timed = {
            {"format", "studio-dark-production"},
            {"version", 2},
            {"catalogVersion", 1},
            {"title", "Tempo fixo"},
            {"timingMode", "fit-to-duration"},
            {"targetDurationSeconds", 4.1},
            {"preserveVoicePitch", true},
            {"musicGain", 0},
            {"effectsGain", 0},
            {"captions", true},
            {"scenes",
             QJsonArray{
                 QJsonObject{
                     {"id", "intro"},
                     {"targetDurationSeconds", 2.0},
                     {"narration",
                      QJsonArray{QJsonObject{{"text", "Primeiro trecho."}, {"targetDurationSeconds", 2.0}}}},
                     {"elements",
                      QJsonArray{
                          QJsonObject{{"id", "animated-through-speech"},
                                      {"kind", "shape"},
                                      {"cue", 0},
                                      {"cueProgress", 0},
                                      {"keyframes",
                                       QJsonArray{QJsonObject{{"cue", 0}, {"cueProgress", 0}, {"x", 100}},
                                                  QJsonObject{{"cue", 0},
                                                              {"cueProgress", 1},
                                                              {"x", 500},
                                                              {"easing", "ease-in-out"}}}}},
                          QJsonObject{
                              {"id", "half-speech"}, {"kind", "shape"}, {"cue", 0}, {"cueProgress", 0.5}}}},
                     {"camera", QJsonArray{QJsonObject{{"cue", 0},
                                                       {"cueProgress", 0},
                                                       {"x", 640},
                                                       {"y", 360},
                                                       {"zoom", 1},
                                                       {"rotation", 0}},
                                           QJsonObject{{"cue", 0},
                                                       {"cueProgress", 1},
                                                       {"x", 690},
                                                       {"y", 360},
                                                       {"zoom", 1},
                                                       {"rotation", 0}}}}},
                 QJsonObject{{"id", "fim"},
                             {"targetDurationSeconds", 2.1},
                             {"narration", QJsonArray{QJsonObject{{"text", "Segundo trecho."},
                                                                  {"targetDurationSeconds", 2.1}}}},
                             {"elements", QJsonArray{}}}}}};
        auto timedRecipe = studio::parseRecipe(QJsonDocument(timed).toJson(), directory.path());
        check(timedRecipe.error.isEmpty() && timedRecipe.recipe.timingMode == "fit-to-duration",
              "parse v2 fixed target, scene/cue slots and proportional keyframes");
        QMap<QString, studio::PcmAudio> timedRaw;
        studio::PcmAudio firstSpeech, secondSpeech;
        firstSpeech.samples.resize(48000, .1f);  // exactly 2 seconds
        secondSpeech.samples.resize(57600, .1f); // 2.4 seconds; atempo about 1.143x
        timedRaw.insert(timedRecipe.recipe.scenes[0].narration[0].id, firstSpeech);
        timedRaw.insert(timedRecipe.recipe.scenes[1].narration[0].id, secondSpeech);
        auto timingPlan = studio::planNarrationTiming(timedRecipe.recipe, timedRaw);
        check(timingPlan.error.isEmpty() && timingPlan.targetFrames == 123 && timingPlan.scenes.size() == 2 &&
                  timingPlan.scenes[0].durationFrames == 60 && timingPlan.scenes[1].durationFrames == 63 &&
                  timingPlan.scenes[1].cues[0].speechFrames == 63,
              "speech planner creates exact 30-fps budgets within 15% speed cap");
        check(!studio::compileProduction(timedRecipe.recipe, timedRaw, "no-plan.wav").error.isEmpty(),
              "fixed production requires a verified timing plan");
        check(!studio::compileProduction(timedRecipe.recipe, timedRaw, "unadjusted.wav", &timingPlan)
                   .error.isEmpty(),
              "unadjusted audio cannot overflow the planned speech slot");
        auto simulatedRetimed = timedRaw;
        // The real worker uses FFmpeg atempo. Unit test supplies already adjusted sample count.
        simulatedRetimed[timedRecipe.recipe.scenes[1].narration[0].id].samples.resize(63 * 800);
        auto timedResult =
            studio::compileProduction(timedRecipe.recipe, simulatedRetimed, "timed.wav", &timingPlan);
        check(timedResult.error.isEmpty() && timedResult.project.duration == 123 &&
                  timedResult.voice.samples.size() == 123 * 800 &&
                  timedResult.project.scenes[0].duration == 60 &&
                  timedResult.project.scenes[1].duration == 63,
              "exact project, audio and scene duration at 30 fps");
        const auto *half = studio::findNode(timedResult.project, "half-speech");
        const auto *animatedSpeech = studio::findNode(timedResult.project, "animated-through-speech");
        check(half && half->start == 30 && animatedSpeech && animatedSpeech->keyframes.size() == 2 &&
                  animatedSpeech->keyframes.front().frame == 0 &&
                  animatedSpeech->keyframes.back().frame == 59 &&
                  timedResult.project.camera.back().frame == 59,
              "cueProgress resolves against adjusted speech, last frame is inclusive");
        check(timedResult.subtitles.contains("00:00:02,000 --> 00:00:04,100"),
              "subtitle timestamps follow the adjusted speech and exact scene starts");
        auto tooShort = timed;
        tooShort["targetDurationSeconds"] = 3.0;
        auto tooShortScenes = tooShort["scenes"].toArray();
        auto tooShortFirst = tooShortScenes[0].toObject();
        auto tooShortSecond = tooShortScenes[1].toObject();
        tooShortFirst.remove("targetDurationSeconds");
        tooShortSecond.remove("targetDurationSeconds");
        auto shortNarration = tooShortSecond["narration"].toArray()[0].toObject();
        shortNarration.remove("targetDurationSeconds");
        tooShortSecond["narration"] = QJsonArray{shortNarration};
        tooShortFirst["narration"] = QJsonArray{"Primeiro trecho."};
        tooShortScenes[0] = tooShortFirst;
        tooShortScenes[1] = tooShortSecond;
        tooShort["scenes"] = tooShortScenes;
        auto shortRecipe = studio::parseRecipe(QJsonDocument(tooShort).toJson(), directory.path());
        check(shortRecipe.error.isEmpty() &&
                  !studio::planNarrationTiming(shortRecipe.recipe, timedRaw).error.isEmpty(),
              "unattainable target produces an actionable error, not artificial voice");
        auto longPlanRecipe = timedRecipe.recipe;
        longPlanRecipe.targetDurationSeconds = 30;
        for (auto &scene : longPlanRecipe.scenes) {
            scene.targetDurationSeconds = 0;
            for (auto &cue : scene.narration)
                cue.targetDurationSeconds = 0;
        }
        const auto longPlan = studio::planNarrationTiming(longPlanRecipe, timedRaw);
        check(longPlan.error.isEmpty() && !longPlan.warnings.isEmpty(),
              "very long silent gaps trigger explicit pacing warnings");
        auto invalidTiming = timed;
        invalidTiming["preserveVoicePitch"] = false;
        check(!studio::parseRecipe(QJsonDocument(invalidTiming).toJson(), directory.path()).error.isEmpty(),
              "reject pitch shifting that was not implemented");
        invalidTiming = timed;
        auto conflictingScenes = invalidTiming["scenes"].toArray();
        auto conflicting = conflictingScenes[0].toObject();
        auto elements = conflicting["elements"].toArray();
        auto invalidElement = elements[0].toObject();
        invalidElement["offsetSeconds"] = 1;
        elements[0] = invalidElement;
        conflicting["elements"] = elements;
        conflictingScenes[0] = conflicting;
        invalidTiming["scenes"] = conflictingScenes;
        check(!studio::parseRecipe(QJsonDocument(invalidTiming).toJson(), directory.path()).error.isEmpty(),
              "reject ambiguous cueProgress plus offsetSeconds");
        check(studio::capabilities()["productionCapabilities"].toArray().contains("keyframe-easing-curves"),
              "easing remains advertised");
        auto g = studio::aiGuide();
        for (const auto &asset : studio::nativeLibrary())
            check(g.contains("`" + QString::fromStdString(asset.id) + "`"),
                  "guide derives all real library IDs");
        check(studio::capabilities()["nativeAssets"].toArray().size() == int(studio::nativeLibrary().size()),
              "machine catalogue matches renderer registry");
        std::cout << "PASS: " << checks
                  << " production checks (recipe, audio timing, mix, migration, catalogue, rendering)\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "FAIL after " << checks << ": " << e.what() << '\n';
        return 1;
    }
}
