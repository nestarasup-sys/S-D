#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QImage>
#include <QImageReader>
#include <QRegularExpression>
#include <QJsonArray>
#include <QJsonDocument>
#include <QPainter>
#include <QSet>
#include <QTemporaryDir>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <studio/composition/native_painter.hpp>
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
        check(studio::premiumSlotRole("premium:quote:text"),
              "legacy premium role cache can initialize before declarative manifests load");
        QString premiumCatalogError;
        check(studio::initializePremiumCatalog(&premiumCatalogError) && premiumCatalogError.isEmpty() &&
                  studio::premiumSlotRole("accent-rule") &&
                  studio::premiumSlotRole("premium:question-hook:question") &&
                  studio::premiumSlotRole("illustrative-bar"),
              "declarative semantic roles become valid after the workspace-first initialization order");
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
        auto cameraPresetJson = shotRecipeJson;
        auto presetScenes = cameraPresetJson["scenes"].toArray();
        auto presetScene = presetScenes[0].toObject();
        auto presetShots = presetScene["shots"].toArray();
        auto presetFirstShot = presetShots[0].toObject();
        auto presetFirstCamera = presetFirstShot["camera"].toObject();
        auto presetFirstFocus = presetFirstCamera["focus"].toObject();
        presetFirstFocus.remove("follow");
        presetFirstCamera["focus"] = presetFirstFocus;
        presetFirstShot["camera"] = presetFirstCamera;
        presetFirstShot["cameraPreset"] = "steady-push";
        presetShots[0] = presetFirstShot;
        auto presetShot = presetShots[1].toObject();
        auto presetBeats = presetShot["beats"].toArray();
        presetBeats.removeLast(); // Camera beats cannot compete with a reusable camera trajectory.
        presetShot["beats"] = presetBeats;
        presetShot["cameraPreset"] = "steady-push";
        presetShots[1] = presetShot;
        presetScene["shots"] = presetShots;
        presetScenes[0] = presetScene;
        cameraPresetJson["scenes"] = presetScenes;
        cameraPresetJson["cameraPresets"] = QJsonObject{
            {"steady-push", QJsonArray{QJsonObject{{"offsetSeconds", 0}, {"x", 640}, {"y", 360},
                                                    {"zoom", 1}, {"rotation", 0}},
                                        QJsonObject{{"offsetSeconds", .033333333}, {"x", 660}, {"y", 360},
                                                    {"zoom", 1.4}, {"rotation", 0},
                                                    {"easing", "ease-in-out"}}}}};
        const auto parsedCameraPreset =
            studio::parseRecipe(QJsonDocument(cameraPresetJson).toJson(), directory.path());
        check(parsedCameraPreset.error.isEmpty() &&
                  parsedCameraPreset.recipe.cameraPresets.contains("steady-push") &&
                  parsedCameraPreset.recipe.scenes.front().shots.back().cameraPreset == "steady-push",
              "v3 validates named camera presets and shot references");
        auto automaticRhythmJson = shotRecipeJson;
        automaticRhythmJson["editing"] = QJsonObject{{"intensity", 50}, {"rhythm", "narration"}};
        auto automaticScenes = automaticRhythmJson["scenes"].toArray();
        auto automaticScene = automaticScenes[0].toObject();
        auto automaticShots = automaticScene["shots"].toArray();
        for (int i = 0; i < automaticShots.size(); ++i) {
            auto shot = automaticShots[i].toObject();
            shot.remove("start");
            automaticShots[i] = shot;
        }
        automaticScene["shots"] = automaticShots;
        automaticScenes[0] = automaticScene;
        automaticRhythmJson["scenes"] = automaticScenes;
        const auto parsedAutomaticRhythm =
            studio::parseRecipe(QJsonDocument(automaticRhythmJson).toJson(), directory.path());
        check(parsedAutomaticRhythm.error.isEmpty() && parsedAutomaticRhythm.recipe.editingRhythm == "narration" &&
                  std::all_of(parsedAutomaticRhythm.recipe.scenes.front().shots.begin(),
                              parsedAutomaticRhythm.recipe.scenes.front().shots.end(),
                              [](const auto &shot) { return shot.autoStart; }),
              "v3 narration rhythm accepts omitted shot starts as an explicit automatic direction");
        auto mixedRhythmJson = automaticRhythmJson;
        auto mixedScenes = mixedRhythmJson["scenes"].toArray();
        auto mixedScene = mixedScenes[0].toObject();
        auto mixedShots = mixedScene["shots"].toArray();
        auto mixedShot = mixedShots[1].toObject();
        mixedShot["start"] = QJsonObject{{"cue", 0}, {"cueProgress", .5}};
        mixedShots[1] = mixedShot;
        mixedScene["shots"] = mixedShots;
        mixedScenes[0] = mixedScene;
        mixedRhythmJson["scenes"] = mixedScenes;
        check(!studio::parseRecipe(QJsonDocument(mixedRhythmJson).toJson(), directory.path()).error.isEmpty(),
              "reject mixing automatic rhythm with manually timed shots");
        auto missingCameraPreset = cameraPresetJson;
        auto missingPresetScenes = missingCameraPreset["scenes"].toArray();
        auto missingPresetScene = missingPresetScenes[0].toObject();
        auto missingPresetShots = missingPresetScene["shots"].toArray();
        auto missingPresetShot = missingPresetShots[1].toObject();
        missingPresetShot["cameraPreset"] = "unknown-preset";
        missingPresetShots[1] = missingPresetShot;
        missingPresetScene["shots"] = missingPresetShots;
        missingPresetScenes[0] = missingPresetScene;
        missingCameraPreset["scenes"] = missingPresetScenes;
        check(!studio::parseRecipe(QJsonDocument(missingCameraPreset).toJson(), directory.path()).error.isEmpty(),
              "reject references to undefined camera presets");
        QFile portraitFixture(QString(STUDIO_SOURCE_DIR) + "/examples/retrato-1080x1920-v3.production.json");
        check(portraitFixture.open(QIODevice::ReadOnly), "portrait production example exists");
        const auto portraitBytes = portraitFixture.readAll();
        const auto portraitRecipe = studio::parseRecipe(portraitBytes, directory.path());
        check(portraitRecipe.error.isEmpty() && portraitRecipe.recipe.width == 1080 &&
                  portraitRecipe.recipe.height == 1920 &&
                  portraitRecipe.recipe.scenes.front().shots.front().cameraX == 540 &&
                  portraitRecipe.recipe.scenes.front().shots.front().cameraY == 960,
              "v3 portrait canvas is parsed and omitted camera coordinates center on its dimensions");
        auto invalidCanvas = QJsonDocument::fromJson(portraitBytes).object();
        invalidCanvas["canvas"] = QJsonObject{{"width", 1080}, {"height", 1080}};
        check(!studio::parseRecipe(QJsonDocument(invalidCanvas).toJson(), directory.path()).error.isEmpty(),
              "reject unsupported canvas dimensions");
        auto verticalVoice = QMap<QString, studio::PcmAudio>{};
        verticalVoice[portraitRecipe.recipe.scenes.front().narration.front().id].samples.resize(96000, .08f);
        const auto compiledPortrait = studio::compileProduction(portraitRecipe.recipe, verticalVoice,
                                                               directory.filePath("portrait.wav"));
        const auto *portraitCaption = studio::findNode(compiledPortrait.project, "caption-retrato-demo-u1");
        const auto *portraitBackground = studio::findNode(compiledPortrait.project, "fundo-estudio");
        check(compiledPortrait.error.isEmpty() && studio::validate(compiledPortrait.project).empty() &&
                  compiledPortrait.project.width == 1080 && compiledPortrait.project.height == 1920 &&
                  portraitCaption && portraitCaption->y == 1804 && portraitCaption->width == 1008 &&
                  portraitBackground && portraitBackground->width == 1080 && portraitBackground->height == 1920,
              "vertical compilation preserves canvas, safe captions, and background dimensions");
        studio::CompositionPainter portraitPainter;
        const auto portraitFrame = portraitPainter.frame(compiledPortrait.project, 30);
        check(portraitFrame.size() == QSize(1080, 1920) &&
                  portraitFrame.pixelColor(540, 960) != portraitFrame.pixelColor(10, 10),
              "shared composition renderer paints the vertical frame");
        QFile soundFixture(directory.filePath("beat-chime.wav"));
        check(soundFixture.open(QIODevice::WriteOnly) && soundFixture.write("fixture") == 7,
              "Visual Beat sound fixture exists");
        soundFixture.close();
        auto soundRecipeJson = shotRecipeJson;
        auto soundScene = soundRecipeJson["scenes"].toArray().at(0).toObject();
        auto soundShots = soundScene["shots"].toArray();
        auto soundShot = soundShots.at(1).toObject();
        auto soundBeats = soundShot["beats"].toArray();
        soundBeats.append(QJsonObject{{"id", "price-chime"},
                                      {"at", QJsonObject{{"cue", 0}, {"progress", 0.75}}},
                                      {"action", "sound"},
                                      {"source", "beat-chime.wav"},
                                      {"gain", 0.65}});
        soundShot["beats"] = soundBeats;
        soundShots[1] = soundShot;
        soundScene["shots"] = soundShots;
        soundRecipeJson["scenes"] = QJsonArray{soundScene};
        const auto parsedSoundRecipe =
            studio::parseRecipe(QJsonDocument(soundRecipeJson).toJson(), directory.path());
        auto invalidSoundRecipe = soundRecipeJson;
        auto invalidSoundScene = invalidSoundRecipe["scenes"].toArray().at(0).toObject();
        auto invalidSoundShots = invalidSoundScene["shots"].toArray();
        auto invalidSoundShot = invalidSoundShots.at(1).toObject();
        auto invalidSoundBeats = invalidSoundShot["beats"].toArray();
        auto invalidSoundBeat = invalidSoundBeats.at(invalidSoundBeats.size() - 1).toObject();
        invalidSoundBeat["gain"] = 4.1;
        invalidSoundBeats[invalidSoundBeats.size() - 1] = invalidSoundBeat;
        invalidSoundShot["beats"] = invalidSoundBeats;
        invalidSoundShots[1] = invalidSoundShot;
        invalidSoundScene["shots"] = invalidSoundShots;
        invalidSoundRecipe["scenes"] = QJsonArray{invalidSoundScene};
        check(parsedSoundRecipe.error.isEmpty() &&
                  parsedSoundRecipe.recipe.scenes.front().shots.back().beats.back().soundSource ==
                      directory.filePath("beat-chime.wav") &&
                  !studio::parseRecipe(QJsonDocument(invalidSoundRecipe).toJson(), directory.path()).error.isEmpty(),
              "sound Visual Beat accepts a local file and rejects out-of-range gain");
        QMap<QString, studio::PcmAudio> soundAudio;
        soundAudio[parsedSoundRecipe.recipe.scenes.front().narration.front().id].samples.resize(48000, .1f);
        const auto compiledSound = studio::compileProduction(parsedSoundRecipe.recipe, soundAudio, "sound.wav");
        const auto compiledSoundBeat =
            std::find_if(compiledSound.project.visualBeats.begin(), compiledSound.project.visualBeats.end(),
                         [](const auto &beat) { return beat.action == "sound"; });
        check(compiledSound.error.isEmpty() && compiledSound.project.schemaVersion == 9 &&
                  compiledSound.project.visualBeats.size() == 4 &&
                  compiledSoundBeat != compiledSound.project.visualBeats.end() &&
                  compiledSoundBeat->targetId == "sfx-price-chime" &&
                  compiledSound.project.audioTracks.size() == 1 &&
                  compiledSound.project.audioTracks.front().start == compiledSoundBeat->frame &&
                  compiledSound.project.audioTracks.front().source == directory.filePath("beat-chime.wav") &&
                  studio::validate(compiledSound.project).empty(),
              "sound Visual Beat compiles into a linked, timed, editable schema 9 audio track");
        QFile videoFixture(directory.filePath("broll.mp4"));
        check(videoFixture.open(QIODevice::WriteOnly) && videoFixture.write("fixture") == 7,
              "video recipe fixture exists");
        videoFixture.close();
        auto videoRecipeJson = shotRecipeJson;
        auto videoScene = videoRecipeJson["scenes"].toArray().at(0).toObject();
        auto videoShots = videoScene["shots"].toArray();
        auto videoShot = videoShots.at(0).toObject();
        auto videoElements = videoShot["elements"].toArray();
        videoElements.append(QJsonObject{{"id", "broll"},
                                         {"kind", "video"},
                                         {"source", "broll.mp4"},
                                         {"x", 100},
                                         {"y", 100},
                                         {"width", 500},
                                         {"height", 300},
                                         {"video", QJsonObject{{"sourceInSeconds", .25},
                                                                {"durationFrames", 15},
                                                                {"speed", 1.5},
                                                                {"crop", QJsonObject{{"x", .1},
                                                                                      {"y", .2},
                                                                                      {"width", .8},
                                                                                      {"height", .6}}}}}});
        videoShot["elements"] = videoElements;
        videoShots[0] = videoShot;
        videoScene["shots"] = videoShots;
        videoRecipeJson["scenes"] = QJsonArray{videoScene};
        const auto parsedVideoRecipe =
            studio::parseRecipe(QJsonDocument(videoRecipeJson).toJson(), directory.path());
        auto invalidVideoRecipe = videoRecipeJson;
        auto invalidVideoScene = invalidVideoRecipe["scenes"].toArray().at(0).toObject();
        auto invalidVideoShots = invalidVideoScene["shots"].toArray();
        auto invalidVideoShot = invalidVideoShots.at(0).toObject();
        auto invalidVideoElements = invalidVideoShot["elements"].toArray();
        auto invalidVideoElement = invalidVideoElements.at(invalidVideoElements.size() - 1).toObject();
        auto invalidVideoSettings = invalidVideoElement["video"].toObject();
        auto invalidVideoCrop = invalidVideoSettings["crop"].toObject();
        invalidVideoCrop["x"] = .4;
        invalidVideoCrop["width"] = .8;
        invalidVideoSettings["crop"] = invalidVideoCrop;
        invalidVideoElement["video"] = invalidVideoSettings;
        invalidVideoElements[invalidVideoElements.size() - 1] = invalidVideoElement;
        invalidVideoShot["elements"] = invalidVideoElements;
        invalidVideoShots[0] = invalidVideoShot;
        invalidVideoScene["shots"] = invalidVideoShots;
        invalidVideoRecipe["scenes"] = QJsonArray{invalidVideoScene};
        check(parsedVideoRecipe.error.isEmpty() &&
                  parsedVideoRecipe.recipe.scenes.front().shots.front().elements.back().node.videoSourceInSeconds ==
                      .25 && parsedVideoRecipe.recipe.scenes.front().shots.front().elements.back()
                                     .videoDurationFrames == 15 &&
                  !studio::parseRecipe(QJsonDocument(invalidVideoRecipe).toJson(), directory.path()).error.isEmpty(),
              "v3 parses a video trim, speed and normalized crop while rejecting an invalid crop");
        const auto compiledVideo =
            studio::compileProduction(parsedVideoRecipe.recipe, soundAudio, "video.wav");
        const auto *compiledBroll = studio::findNode(compiledVideo.project, "broll");
        check(compiledVideo.error.isEmpty() && compiledVideo.project.schemaVersion == 10 && compiledBroll &&
                  compiledBroll->duration == 15 && compiledBroll->videoPlaybackRate == 1.5 &&
                  compiledBroll->videoCropY == .2 &&
                  studio::validate(compiledVideo.project).empty(),
              "v3 video controls compile into an editable schema 10 project");
        auto objectCutJson = shotRecipeJson;
        auto objectCutScenes = objectCutJson["scenes"].toArray();
        auto objectCutScene = objectCutScenes[0].toObject();
        auto objectCutShots = objectCutScene["shots"].toArray();
        auto objectCutShot = objectCutShots[1].toObject();
        objectCutShot["transitionIn"] = QJsonObject{{"type", "cut"}, {"durationFrames", 8}};
        objectCutShots[1] = objectCutShot;
        objectCutScene["shots"] = objectCutShots;
        objectCutScenes[0] = objectCutScene;
        objectCutJson["scenes"] = objectCutScenes;
        const auto objectCutRecipe = studio::parseRecipe(QJsonDocument(objectCutJson).toJson(), directory.path());
        check(objectCutRecipe.error.isEmpty() &&
                  objectCutRecipe.recipe.scenes.front().shots.back().transitionIn == "cut" &&
                  objectCutRecipe.recipe.scenes.front().shots.back().transitionDurationFrames == 0,
              "cut transition objects with an optional duration are normalized to an immediate cut");
        auto styledRecipeJson = shotRecipeJson;
        styledRecipeJson["artDirection"] = QJsonObject{{"backgroundColor", "#101820"},
                                                        {"glowColor", "#29434a"},
                                                        {"primaryColor", "#f2b84b"},
                                                        {"secondaryColor", "#65d6c4"},
                                                        {"textColor", "#fff3d6"},
                                                        {"cardColor", "#24323b"},
                                                        {"captionBackgroundColor", "#101820"},
                                                        {"captionHighlightColor", "#ff21aa"},
                                                        {"fontFamily", "Segoe UI"},
                                                        {"titleSize", 60},
                                                        {"bodySize", 42},
                                                        {"captionSize", 29},
                                                        {"captionPosition", "top"},
                                                        {"safeMargin", 40}};
        auto styledScene = styledRecipeJson["scenes"].toArray().at(0).toObject();
        styledScene["artDirection"] = QJsonObject{{"backgroundColor", "#14202d"},
                                                  {"textColor", "#ca6af0"}};
        QFile trackFixture(directory.filePath("music-track.wav"));
        check(trackFixture.open(QIODevice::WriteOnly) && trackFixture.write("fixture") == 7,
              "audio track fixture is available");
        trackFixture.close();
        styledScene["audioTracks"] = QJsonArray{QJsonObject{{"id", "music-bed"},
                                                            {"name", "Trilha"},
                                                            {"source", "music-track.wav"},
                                                            {"start", QJsonObject{{"cue", 0}, {"progress", 0.1}}},
                                                            {"gain", 0.4},
                                                            {"fadeInFrames", 2},
                                                            {"fadeOutFrames", 3},
                                                            {"sourceInSeconds", 1.25},
                                                            {"playbackRate", 1.5},
                                                            {"gainAutomation", QJsonArray{
                                                                                  QJsonObject{{"frame", 0}, {"value", 0.25}},
                                                                                  QJsonObject{{"frame", 2}, {"value", 1.0},
                                                                                              {"easing", "ease-in-out"}}}},
                                                            {"duckUnderVoice", true},
                                                            {"duckAmount", 0.8}}};
        auto styledShots = styledScene["shots"].toArray();
        auto styledFirstShot = styledShots[0].toObject();
        auto styledFirstElements = styledFirstShot["elements"].toArray();
        styledFirstElements.append(QJsonObject{{"id", "scene-style-text"}, {"kind", "text"},
                                               {"text", "Cena"}, {"x", 60}, {"y", 80},
                                               {"width", 300}, {"height", 70}});
        styledFirstShot["elements"] = styledFirstElements;
        styledShots[0] = styledFirstShot;
        auto styledSecondShot = styledShots[1].toObject();
        styledSecondShot["artDirection"] = QJsonObject{{"backgroundColor", "#070a0c"},
                                                       {"textColor", "#ffffff"}};
        auto styledElements = styledSecondShot["elements"].toArray();
        styledElements.append(QJsonObject{{"id", "shot-style-text"}, {"kind", "text"},
                                          {"text", "Shot"}, {"x", 60}, {"y", 180},
                                          {"width", 300}, {"height", 70}});
        styledElements.append(QJsonObject{{"id", "card-default"}, {"kind", "shape"},
                                          {"x", 700}, {"y", 300}, {"width", 300}, {"height", 180}});
        styledSecondShot["elements"] = styledElements;
        styledShots[1] = styledSecondShot;
        styledScene["shots"] = styledShots;
        QJsonArray styledScenes;
        styledScenes.append(styledScene);
        styledRecipeJson["scenes"] = styledScenes;
        const auto styledRecipe =
            studio::parseRecipe(QJsonDocument(styledRecipeJson).toJson(), directory.path());
        check(styledRecipe.error.isEmpty() &&
                  styledRecipe.recipe.artDirection.backgroundColor == "#101820" &&
                  styledRecipe.recipe.artDirection.captionHighlightColor == "#ff21aa" &&
                  styledRecipe.recipe.artDirection.captionPosition == "top" &&
                  styledRecipe.recipe.scenes.front().artDirection->backgroundColor == "#14202d" &&
                  styledRecipe.recipe.scenes.front().shots.back().artDirection->backgroundColor == "#070a0c" &&
                  styledRecipe.recipe.scenes.front().shots.back().artDirection->primaryColor == "#f2b84b" &&
                  styledRecipe.recipe.scenes.front().shots.back().elements.back().node.color == "#24323b" &&
                  styledRecipe.recipe.scenes.front().shots.back().elements.front().children.front().node.color ==
                      "#ffffff" &&
                  styledRecipe.recipe.scenes.front().shots.front().elements.front().node.color == "#b4f078" &&
                  styledRecipe.recipe.scenes.front().shots.front().elements.back().node.color == "#ca6af0" &&
                  styledRecipe.recipe.scenes.front().audioTracks.size() == 1 &&
                  styledRecipe.recipe.scenes.front().audioTracks.front().duckUnderVoice &&
                  styledRecipe.recipe.scenes.front().audioTracks.front().fadeInFrames == 2 &&
                  styledRecipe.recipe.scenes.front().audioTracks.front().fadeOutFrames == 3 &&
                  styledRecipe.recipe.scenes.front().audioTracks.front().sourceInSeconds == 1.25 &&
                  styledRecipe.recipe.scenes.front().audioTracks.front().playbackRate == 1.5 &&
                  styledRecipe.recipe.scenes.front().audioTracks.front().gainAutomation.size() == 2 &&
                  styledRecipe.recipe.scenes.front().audioTracks.front().gainAutomation.back().easing ==
                      "ease-in-out",
              "v3 parses global art direction and independent, voice-ducked audio tracks");
        auto presetRecipe = shotRecipeJson;
        presetRecipe["artDirectionPreset"] = "style-global";
        auto stylePresetScene = presetRecipe["scenes"].toArray().at(0).toObject();
        stylePresetScene["artDirectionPreset"] = "style-scene";
        auto stylePresetShots = stylePresetScene["shots"].toArray();
        auto stylePresetShot = stylePresetShots.at(1).toObject();
        stylePresetShot["artDirectionPreset"] = "style-shot";
        stylePresetShot["artDirection"] = QJsonObject{{"textColor", "#ffffff"}};
        stylePresetShots[1] = stylePresetShot;
        stylePresetScene["shots"] = stylePresetShots;
        presetRecipe["scenes"] = QJsonArray{stylePresetScene};
        const auto styleLibrary = QJsonObject{{"assets", QJsonArray{
            QJsonObject{{"id", "style-global"}, {"kind", "visual-style"}, {"deleted", false},
                        {"name", "Global"}, {"style", QJsonObject{{"primaryColor", "#112233"}}}},
            QJsonObject{{"id", "style-scene"}, {"kind", "visual-style"}, {"deleted", false},
                        {"name", "Cena"}, {"style", QJsonObject{{"primaryColor", "#445566"}}}},
            QJsonObject{{"id", "style-shot"}, {"kind", "visual-style"}, {"deleted", false},
                        {"name", "Shot"}, {"style", QJsonObject{{"primaryColor", "#778899"}}}}
        }}};
        const auto presetParsed = studio::parseRecipe(QJsonDocument(presetRecipe).toJson(), directory.path(), styleLibrary);
        check(presetParsed.error.isEmpty() && presetParsed.recipe.artDirection.primaryColor == "#112233" &&
                  presetParsed.recipe.scenes.front().artDirection->primaryColor == "#445566" &&
                  presetParsed.recipe.scenes.front().shots.back().artDirection->primaryColor == "#778899" &&
                  presetParsed.recipe.scenes.front().shots.back().artDirection->textColor == "#ffffff",
              "named visual style presets inherit from root to scene to shot and inline fields override presets");
        auto missingStyle = presetRecipe;
        missingStyle["artDirectionPreset"] = "deleted-style";
        check(!studio::parseRecipe(QJsonDocument(missingStyle).toJson(), directory.path(), styleLibrary).error.isEmpty(),
              "recipes reject missing or inactive named visual styles");
        const auto styleGuide = studio::aiGuide(styleLibrary);
        check(styleGuide.contains("style-global") && styleGuide.contains("artDirectionPreset") &&
                  studio::capabilities()["productionCapabilities"].toArray().contains("named-visual-style-presets") &&
                  studio::capabilities()["productionCapabilities"].toArray().contains("external-library-fonts"),
              "generated AI guide and capability manifest expose only the supported local styles and fonts");
        const auto visualBeatGuide = studio::aiGuide({});
        check(visualBeatGuide.contains("opacity` inicial seja 0") &&
                  visualBeatGuide.contains("não interpolam antes do quadro marcado") &&
                  visualBeatGuide.contains("revelação sequencial de três objetos") &&
                  studio::capabilities()["productionCapabilities"].toArray().contains(
                      "instant-visual-beat-show-hide"),
              "AI guide and capability catalog describe exact-frame Visual Beat visibility");
        auto wordRecipeJson = shotRecipeJson;
        auto wordScene = wordRecipeJson["scenes"].toArray().at(0).toObject();
        auto wordShots = wordScene["shots"].toArray();
        auto wordShot = wordShots.at(1).toObject();
        auto wordElements = wordShot["elements"].toArray();
        auto wordGroup = wordElements.at(0).toObject();
        auto wordChildren = wordGroup["children"].toArray();
        auto wordPrice = wordChildren.at(0).toObject();
        wordPrice["wordIndex"] = 2;
        wordPrice["keyframes"] = QJsonArray{
            QJsonObject{{"wordIndex", 3}, {"x", 180}}, QJsonObject{{"wordIndex", 4}, {"x", 260}}};
        wordChildren[0] = wordPrice;
        wordGroup["children"] = wordChildren;
        wordElements[0] = wordGroup;
        wordShot["elements"] = wordElements;
        auto wordBeats = wordShot["beats"].toArray();
        auto wordBeat = wordBeats.at(0).toObject();
        wordBeat["at"] = QJsonObject{{"cue", 0}, {"wordIndex", 3}};
        wordBeats[0] = wordBeat;
        wordShot["beats"] = wordBeats;
        wordShots[1] = wordShot;
        wordScene["shots"] = wordShots;
        QJsonArray wordScenes;
        wordScenes.append(wordScene);
        wordRecipeJson["scenes"] = wordScenes;
        const auto wordRecipe = studio::parseRecipe(QJsonDocument(wordRecipeJson).toJson(), directory.path());
        check(wordRecipe.error.isEmpty() &&
                  wordRecipe.recipe.scenes.front().shots.back().beats.front().at.wordIndex == 3 &&
                  wordRecipe.recipe.scenes.front().shots.back().elements.front().children.front().wordIndex == 2 &&
                  wordRecipe.recipe.scenes.front().shots.back().elements.front().children.front().keyframes[0].wordIndex == 3,
              "v3 parses word-index timing for Visual Beats, elements and keyframes");
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
        QJsonArray stressScenes;
        for (int sceneIndex = 0; sceneIndex < 20; ++sceneIndex) {
            QJsonArray stressShots;
            for (int shotIndex = 0; shotIndex < 6; ++shotIndex) {
                const auto shotId = QString("stress-%1-%2").arg(sceneIndex).arg(shotIndex);
                const auto elementId = shotId + "-element";
                const double shotStart = double(shotIndex) / 6.;
                const double beatProgress = shotIndex == 0 ? .02 : shotIndex == 5 ? .95
                                                                                   : (shotIndex + .5) / 6.;
                QJsonObject shot{{"id", shotId},
                                 {"start", QJsonObject{{"cue", 0}, {"cueProgress", shotStart}}},
                                 {"camera", QJsonObject{{"x", 640}, {"y", 360}, {"zoom", 1}}},
                                 {"elements", QJsonArray{QJsonObject{{"id", elementId},
                                                                    {"kind", "shape"},
                                                                    {"x", 100 + shotIndex * 20},
                                                                    {"y", 200},
                                                                    {"width", 180},
                                                                    {"height", 100},
                                                                    {"color", "#55e2c4"}}}}};
                if (shotIndex > 0)
                    shot["transitionIn"] = QJsonObject{{"type", "dissolve"}, {"durationFrames", 2}};
                shot["beats"] = QJsonArray{
                    QJsonObject{{"id", shotId + "-move"},
                                 {"at", QJsonObject{{"cue", 0}, {"cueProgress", beatProgress}}},
                                 {"action", "transform"},
                                 {"target", elementId},
                                 {"durationFrames", 2},
                                 {"x", 250 + shotIndex * 20},
                                 {"scale", 1.08}},
                    QJsonObject{{"id", shotId + "-camera"},
                                 {"at", QJsonObject{{"cue", 0}, {"cueProgress", beatProgress}}},
                                 {"action", "camera"},
                                 {"durationFrames", 2},
                                 {"zoom", 1.05}}};
                stressShots.append(shot);
            }
            stressScenes.append(QJsonObject{{"id", QString("stress-scene-%1").arg(sceneIndex)},
                                            {"narration", QJsonArray{"Uma narração de teste para conferir muitos cortes."}},
                                            {"shots", stressShots}});
        }
        const QJsonObject stressJson{{"format", "studio-dark-production"},
                                     {"version", 3},
                                     {"catalogVersion", 1},
                                     {"title", "Teste temporal de sessenta segundos"},
                                     {"scenes", stressScenes}};
        const auto stressRecipe = studio::parseRecipe(QJsonDocument(stressJson).toJson(), directory.path());
        QMap<QString, studio::PcmAudio> stressAudio;
        if (stressRecipe.error.isEmpty())
            for (const auto &scene : stressRecipe.recipe.scenes)
                for (const auto &utterance : scene.narration)
                    stressAudio[utterance.id].samples.resize(72000, .08f);
        const auto stressCompiled = stressRecipe.error.isEmpty()
                                        ? studio::compileProduction(stressRecipe.recipe, stressAudio,
                                                                    directory.filePath("stress.wav"))
                                        : studio::CompileResult{};
        bool stressBoundariesValid = stressRecipe.error.isEmpty() && stressCompiled.error.isEmpty() &&
                                     stressCompiled.project.scenes.size() == 20 &&
                                     stressCompiled.project.shots.size() == 120 &&
                                     stressCompiled.project.visualBeats.size() == 240 &&
                                     stressCompiled.project.nodes.size() == 140 &&
                                     stressCompiled.project.duration >= 1800;
        for (const auto &scene : stressCompiled.project.scenes) {
            std::vector<studio::ShotMarker> sceneShots;
            for (const auto &shot : stressCompiled.project.shots)
                if (shot.sceneId == scene.id)
                    sceneShots.push_back(shot);
            std::sort(sceneShots.begin(), sceneShots.end(),
                      [](const auto &left, const auto &right) { return left.start < right.start; });
            stressBoundariesValid = stressBoundariesValid && sceneShots.size() == 6 &&
                                    sceneShots.front().start == scene.start &&
                                    sceneShots.back().start + sceneShots.back().duration ==
                                        scene.start + scene.duration;
            for (std::size_t i = 1; i < sceneShots.size(); ++i)
                stressBoundariesValid = stressBoundariesValid &&
                                        sceneShots[i - 1].start + sceneShots[i - 1].duration ==
                                            sceneShots[i].start &&
                                        sceneShots[i].transitionDurationFrames == 2;
        }
        for (std::size_t i = 0; i + 1 < stressCompiled.project.visualBeats.size(); i += 2)
            stressBoundariesValid = stressBoundariesValid &&
                                    stressCompiled.project.visualBeats[i].frame ==
                                        stressCompiled.project.visualBeats[i + 1].frame;
        if (!stressBoundariesValid)
            std::cerr << "stress parse='" << stressRecipe.error.toStdString() << "' compile='"
                      << stressCompiled.error.toStdString() << "' scenes="
                      << stressCompiled.project.scenes.size() << " shots="
                      << stressCompiled.project.shots.size() << " beats="
                      << stressCompiled.project.visualBeats.size() << " nodes="
                      << stressCompiled.project.nodes.size() << " duration="
                      << stressCompiled.project.duration << " validation='"
                      << studio::validate(stressCompiled.project) << "'\n";
        check(stressBoundariesValid && studio::validate(stressCompiled.project).empty(),
              "long v3 production compiles 120 contiguous shots, near-edge cuts, and concurrent beats");
        QMap<QString, studio::PcmAudio> shotAudio;
        studio::PcmAudio shortSpeech;
        shortSpeech.samples.resize(4800, .1f);
        shotAudio.insert(parsedShots.recipe.scenes.front().narration.front().id, shortSpeech);
        auto automaticRhythmAudio = shotAudio;
        automaticRhythmAudio[parsedAutomaticRhythm.recipe.scenes.front().narration.front().id].samples.resize(48000,
                                                                                                               .1f);
        studio::WordTimingMap automaticRhythmWords;
        automaticRhythmWords.insert(parsedAutomaticRhythm.recipe.scenes.front().narration.front().id,
                                    {0, 6000, 12000, 18000, 24000, 30000, 36000, 42000});
        const auto missingAutomaticRhythmWords =
            studio::compileProduction(parsedAutomaticRhythm.recipe, automaticRhythmAudio, "auto-rhythm-no-words.wav");
        const auto compiledAutomaticRhythm = studio::compileProduction(
            parsedAutomaticRhythm.recipe, automaticRhythmAudio, "auto-rhythm.wav", nullptr,
            &automaticRhythmWords);
        const auto autoGroup = std::find_if(compiledAutomaticRhythm.project.nodes.begin(),
                                            compiledAutomaticRhythm.project.nodes.end(),
                                            [](const auto &node) { return node.id == "detail-group"; });
        const auto autoPrice = std::find_if(compiledAutomaticRhythm.project.nodes.begin(),
                                            compiledAutomaticRhythm.project.nodes.end(),
                                            [](const auto &node) { return node.id == "price"; });
        check(!missingAutomaticRhythmWords.error.isEmpty() && compiledAutomaticRhythm.error.isEmpty() &&
                  compiledAutomaticRhythm.project.shots.size() == 2 &&
                  compiledAutomaticRhythm.project.shots.front().start == 0 &&
                  compiledAutomaticRhythm.project.shots.back().start == 30 &&
                  compiledAutomaticRhythm.project.shots.back().duration == 41 &&
                  autoGroup != compiledAutomaticRhythm.project.nodes.end() && autoGroup->start == 30 &&
                  autoPrice != compiledAutomaticRhythm.project.nodes.end() && autoPrice->start == 30 &&
                  studio::validate(compiledAutomaticRhythm.project).empty(),
              "narration rhythm uses measured phrase pivots and anchors implicit shot elements to the cut");
        auto compiledShots = studio::compileProduction(parsedShots.recipe, shotAudio, "shots.wav");
        check(compiledShots.error.isEmpty() && compiledShots.project.schemaVersion == 7 &&
                  compiledShots.project.shots.size() == 2 && compiledShots.project.visualBeats.size() == 3,
              "shots and beats compile into editable schema 4 project markers");
        auto titledSceneJson = shotRecipeJson;
        auto titledScenes = titledSceneJson["scenes"].toArray();
        auto titledSceneObject = titledScenes.at(0).toObject();
        titledSceneObject["title"] = "Rótulo da cena na timeline";
        titledScenes[0] = titledSceneObject;
        titledSceneJson["scenes"] = titledScenes;
        const auto parsedTitledScene =
            studio::parseRecipe(QJsonDocument(titledSceneJson).toJson(), directory.path());
        auto compiledTitledScene = parsedTitledScene.error.isEmpty()
                                       ? studio::compileProduction(parsedTitledScene.recipe, shotAudio,
                                                                   "titled-scene.wav")
                                       : studio::CompileResult{};
        const bool duplicateSceneHeading = std::any_of(
            compiledTitledScene.project.nodes.begin(), compiledTitledScene.project.nodes.end(),
            [](const auto &node) { return node.id == "heading-car-story"; });
        check(parsedTitledScene.error.isEmpty() && compiledTitledScene.error.isEmpty() &&
                  compiledTitledScene.project.scenes.front().name == "Rótulo da cena na timeline" &&
                  !duplicateSceneHeading,
              "v3 scene title is retained as timeline metadata without duplicating a visible heading over shots");
        auto unknownSceneJson = titledSceneJson;
        titledScenes = unknownSceneJson["scenes"].toArray();
        titledSceneObject = titledScenes.at(0).toObject();
        titledSceneObject["inventedSceneField"] = true;
        titledScenes[0] = titledSceneObject;
        unknownSceneJson["scenes"] = titledScenes;
        check(studio::parseRecipe(QJsonDocument(unknownSceneJson).toJson(), directory.path())
                      .error.contains("inventedSceneField"),
              "unknown v3 scene fields name the unsupported key in the parser error");
        const auto compiledCameraPreset =
            studio::compileProduction(parsedCameraPreset.recipe, shotAudio, "camera-preset.wav");
        const auto cameraPresetShot = std::find_if(
            compiledCameraPreset.project.shots.begin(), compiledCameraPreset.project.shots.end(),
            [](const auto &shot) { return shot.id == "price-cut"; });
        check(compiledCameraPreset.error.isEmpty() && cameraPresetShot != compiledCameraPreset.project.shots.end() &&
                  std::any_of(compiledCameraPreset.project.camera.begin(), compiledCameraPreset.project.camera.end(),
                              [&](const auto &key) {
                                  return key.frame == cameraPresetShot->start && key.zoom == 1.0;
                              }) &&
                  std::count_if(compiledCameraPreset.project.camera.begin(), compiledCameraPreset.project.camera.end(),
                                [](const auto &key) { return key.shotId == "wide"; }) >= 2 &&
                  std::count_if(compiledCameraPreset.project.camera.begin(), compiledCameraPreset.project.camera.end(),
                                [](const auto &key) { return key.shotId == "price-cut"; }) >= 2 &&
                  studio::evaluateCamera(compiledCameraPreset.project, cameraPresetShot->start + 2).zoom > 1.1,
              "a reusable camera preset expands into editable timed keyframes in multiple shots");
        auto overlongCameraPreset = parsedCameraPreset.recipe;
        overlongCameraPreset.cameraPresets["steady-push"][1].offsetSeconds = 600;
        const auto overlongCameraResult =
            studio::compileProduction(overlongCameraPreset, shotAudio, "camera-preset-overlong.wav");
        auto conflictingCameraPreset = parsedCameraPreset.recipe;
        conflictingCameraPreset.scenes.front().shots.front().focusFollow = true;
        const auto conflictingCameraResult =
            studio::compileProduction(conflictingCameraPreset, shotAudio, "camera-preset-conflict.wav");
        check(overlongCameraResult.error.contains("ultrapassa a duração do shot") &&
                  conflictingCameraResult.error.contains("cameraPreset não pode ser combinado"),
              "camera presets cannot exceed shot duration or compete with continuous follow");
        const auto missingWordAlignment =
            studio::compileProduction(wordRecipe.recipe, shotAudio, "word-shots.wav");
        studio::WordTimingMap wordMap;
        const auto cueId = wordRecipe.recipe.scenes.front().narration.front().id;
        wordMap.insert(cueId, {0, 1200, 2400, 4800, 6000, 7200, 8400, 9600, 10800});
        const auto alignedWordShots =
            studio::compileProduction(wordRecipe.recipe, shotAudio, "word-shots.wav", nullptr, &wordMap);
        check(!missingWordAlignment.error.isEmpty() && alignedWordShots.error.isEmpty() &&
                  std::any_of(alignedWordShots.project.visualBeats.begin(),
                              alignedWordShots.project.visualBeats.end(), [](const auto &beat) {
                                  return beat.id == "reveal-price" && beat.frame == 6;
                              }),
              "word-index beats require real alignment and resolve to measured word timing");
        studio::WordTimingMap captionWordMap;
        captionWordMap.insert(cueId, {0, 800, 1600, 2400, 3200, 4000, 4800, 5600});
        auto captionAudio = shotAudio;
        captionAudio[cueId].samples.resize(6400, .1f);
        const auto timedCaptions = studio::compileProduction(parsedShots.recipe, captionAudio, "word-captions.wav",
                                                             nullptr, &captionWordMap);
        const auto *timedCaption = studio::findNode(timedCaptions.project, "caption-car-story-u1");
        check(timedCaptions.error.isEmpty() && timedCaption && timedCaption->captionWords.size() == 8 &&
                  timedCaption->captionWords[3].start == 3 && timedCaption->captionWords[3].end > 3,
              "measured speech alignment compiles into per-word caption highlight intervals");
        const auto savedTimedCaptions = studio::deserialize(studio::serialize(timedCaptions.project));
        check(savedTimedCaptions.error.isEmpty() &&
                  studio::findNode(savedTimedCaptions.project, "caption-car-story-u1")->captionWords ==
                      timedCaption->captionWords,
              "word-highlight timings survive project save and load");
        auto wordAnimationJson = shotRecipeJson;
        wordAnimationJson["wordAnimation"] =
            QJsonObject{{"preset", "pop-rise"}, {"durationFrames", 6}, {"startScale", .72}, {"travelY", 18}};
        auto parsedWordAnimation =
            studio::parseRecipe(QJsonDocument(wordAnimationJson).toJson(), directory.path());
        check(parsedWordAnimation.error.isEmpty() &&
                  parsedWordAnimation.recipe.wordAnimationPreset == "pop-rise" &&
                  parsedWordAnimation.recipe.wordAnimationDurationFrames == 6 &&
                  parsedWordAnimation.recipe.wordAnimationStartScale == .72 &&
                  parsedWordAnimation.recipe.wordAnimationTravelY == 18,
              "v3 parses word-level caption animation presets and limits");
        auto unsupportedWordAnimation = wordAnimationJson;
        unsupportedWordAnimation["wordAnimation"] = QJsonObject{{"preset", "bounce"}};
        check(!studio::parseRecipe(QJsonDocument(unsupportedWordAnimation).toJson(), directory.path())
                   .error.isEmpty(),
              "unknown word-level caption animation presets are rejected");
        const auto animatedCaptions = studio::compileProduction(
            parsedWordAnimation.recipe, captionAudio, "word-animation.wav", nullptr, &captionWordMap);
        const auto *animatedCaption = studio::findNode(animatedCaptions.project, "caption-car-story-u1");
        const auto persistedAnimatedCaptions =
            studio::deserialize(studio::serialize(animatedCaptions.project));
        check(animatedCaptions.error.isEmpty() && animatedCaptions.project.schemaVersion == 12 &&
                  animatedCaption && animatedCaption->wordAnimationPreset == "pop-rise" &&
                  animatedCaption->captionWords.size() == 8 && persistedAnimatedCaptions.error.isEmpty() &&
                  studio::findNode(persistedAnimatedCaptions.project, "caption-car-story-u1")
                          ->wordAnimationPreset == "pop-rise",
              "word animation compiles to a timed editable caption and persists in schema 12");
        auto karaokeProject = studio::makeEmpty();
        karaokeProject.schemaVersion = 7;
        karaokeProject.duration = 10;
        karaokeProject.scenes = {{"car-story", "Car Story", "", 0, 10}};
        auto karaokeCaption = *timedCaption;
        karaokeCaption.start = 0;
        karaokeCaption.duration = 8;
        karaokeCaption.captionWords.clear();
        const auto karaokeText = QString::fromStdString(karaokeCaption.text).split(QRegularExpression("\\s+"),
                                                                                     Qt::SkipEmptyParts);
        for (int index = 0; index < karaokeText.size(); ++index)
            karaokeCaption.captionWords.push_back(
                {karaokeText[index].toStdString(), index, index + 1});
        karaokeProject.nodes.push_back(karaokeCaption);
        studio::CompositionPainter karaokePainter;
        const auto firstSpokenWord = karaokePainter.frame(karaokeProject, 0);
        const auto fourthSpokenWord = karaokePainter.frame(karaokeProject, 3);
        const auto highlightedPixels = [](const QImage &image) {
            int count = 0;
            for (int y = 610; y < 710; ++y)
                for (int x = 70; x < image.width() - 70; ++x) {
                    const auto color = image.pixelColor(x, y);
                    if (color.red() > 150 && color.green() > 210 && color.blue() < 155)
                        ++count;
                }
            return count;
        };
        check(!firstSpokenWord.isNull() && highlightedPixels(firstSpokenWord) > 0 &&
                  highlightedPixels(firstSpokenWord) != highlightedPixels(fourthSpokenWord),
              "caption painter changes the active word color on its measured frame");
        auto animationStaticProject = karaokeProject;
        auto animationProject = karaokeProject;
        animationProject.schemaVersion = 12;
        animationProject.nodes.front().wordAnimationPreset = "pop-rise";
        animationProject.nodes.front().wordAnimationDurationFrames = 4;
        animationProject.nodes.front().wordAnimationStartScale = .72;
        animationProject.nodes.front().wordAnimationTravelY = 18;
        studio::CompositionPainter animationPainter;
        const auto animatedWordImage = animationPainter.frame(animationProject, 0);
        const auto staticWordImage = animationPainter.frame(animationStaticProject, 0);
        int animatedPixelChanges = 0;
        for (int y = 580; y < 720; ++y)
            for (int x = 60; x < 1220; ++x)
                animatedPixelChanges += animatedWordImage.pixel(x, y) != staticWordImage.pixel(x, y);
        check(animatedPixelChanges > 0, "word animation changes pixels around the caption baseline");
        check(studio::validate(animationProject).empty() &&
                  animatedWordImage != staticWordImage &&
                  animationPainter.frame(animationProject, 2) != animationPainter.frame(animationStaticProject, 2),
              "caption motion and scale animate each aligned word in the shared compositor");
        auto alignedWordElement = studio::compileProduction(wordRecipe.recipe, shotAudio, "word-elements.wav",
                                                            nullptr, &wordMap);
        const auto *wordTimedPrice = studio::findNode(alignedWordElement.project, "price");
        check(alignedWordElement.error.isEmpty() && wordTimedPrice && wordTimedPrice->start == 3 &&
                  wordTimedPrice->keyframes.size() == 4 && wordTimedPrice->keyframes[0].frame == 4 &&
                  wordTimedPrice->keyframes[1].frame == 5 && wordTimedPrice->keyframes[2].frame == 6 &&
                  wordTimedPrice->keyframes[3].frame == 8,
              "word alignment preserves a pre-reveal hold and editable transform keyframes");
        const auto persistedShots = studio::deserialize(studio::serialize(compiledShots.project));
        check(persistedShots.error.isEmpty() &&
                  persistedShots.project.shots.back().transitionIn == "dissolve" &&
                  persistedShots.project.shots.back().transitionDurationFrames == 3,
              "shot transition settings survive project save and load");
        const auto styledAudio =
            studio::compileProduction(styledRecipe.recipe, shotAudio, "styled-shots.wav", nullptr, nullptr,
                                      "voice-stem.wav");
        check(styledAudio.error.isEmpty() && styledAudio.project.schemaVersion == 14 &&
                  styledAudio.project.artDirection.backgroundColor == "#101820" &&
                  styledAudio.project.scenes.front().artDirection->backgroundColor == "#14202d" &&
                  styledAudio.project.shots.back().artDirection->backgroundColor == "#070a0c" &&
                  studio::findNode(styledAudio.project, "caption-car-story-u1")->y == 40 &&
                  studio::findNode(styledAudio.project, "car-shape")->color == "#b4f078" &&
                  styledAudio.project.audioTracks.size() == 1 &&
                  styledAudio.project.audioTracks.front().duckUnderVoice &&
                  styledAudio.project.audioTracks.front().fadeInFrames == 2 &&
                  styledAudio.project.audioTracks.front().fadeOutFrames == 3 &&
                  styledAudio.project.audioTracks.front().sourceInSeconds == 1.25 &&
                  styledAudio.project.audioTracks.front().playbackRate == 1.5 &&
                  styledAudio.project.audioTracks.front().gainAutomation.size() == 2 &&
                  styledAudio.project.audioTracks.front().gain == .4 &&
                  styledAudio.project.voiceDuckingSource == "voice-stem.wav",
              "global art direction and audio track settings reach the compiled project");
        const auto persistedStyle = studio::deserialize(studio::serialize(styledAudio.project));
        check(persistedStyle.error.isEmpty() &&
                  persistedStyle.project.artDirection.backgroundColor == "#101820" &&
                  persistedStyle.project.artDirection.captionPosition == "top" &&
                  persistedStyle.project.audioTracks.size() == 1 &&
                  persistedStyle.project.audioTracks.front().duckUnderVoice &&
                  persistedStyle.project.audioTracks.front().gainAutomation.size() == 2 &&
                  persistedStyle.project.voiceDuckingSource == "voice-stem.wav",
              "global art direction and audio tracks survive project save and load");
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
        // Use a high-contrast layer to make this transition assertion stable across
        // Windows and Qt's offscreen font rasterizers; sparse full-frame sampling
        // should not depend on a particular system font's antialiasing.
        auto transitionProject = compiledShots.project;
        studio::Node transitionMarker;
        transitionMarker.id = "transition-regression-marker";
        transitionMarker.kind = studio::NodeKind::Shape;
        transitionMarker.name = "Transition regression marker";
        transitionMarker.x = 360;
        transitionMarker.y = 190;
        transitionMarker.width = 560;
        transitionMarker.height = 340;
        transitionMarker.color = "#ff20a0";
        transitionMarker.shotId = "price-cut";
        transitionMarker.animated = false;
        transitionProject.nodes.push_back(transitionMarker);
        const auto beforeDissolve = transitionPainter.frame(transitionProject, 2);
        const auto dissolveStart = transitionPainter.frame(transitionProject, 3);
        const auto dissolveMiddle = transitionPainter.frame(transitionProject, 4);
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
        check(studio::nativeLibrary().size() == 72,
              "documentary native catalog has expected number of assets");
        const auto imageBackedCount = std::count_if(studio::nativeLibrary().begin(), studio::nativeLibrary().end(),
                                                     [](const auto &asset) { return !asset.resourcePath.empty(); });
        const auto scenePreviewCount = std::count_if(studio::nativeLibrary().begin(), studio::nativeLibrary().end(),
                                                      [](const auto &asset) { return asset.previewOnly; });
        check(imageBackedCount == 61 && scenePreviewCount == 12,
              "60 original AI assets and the 12-pose presenter use embedded PNG artwork without new IDs");
        for (const auto *id : {"bars", "chart-line", "chart-pie", "chart-donut", "chart-area",
                               "timeline", "flowchart", "network", "dashboard", "progress-ring", "data-table"})
            check(studio::libraryItem(id) && studio::libraryItem(id)->resourcePath.empty(),
                  "dynamic graph/data natives retain their procedural renderer");
        const auto *personAsset = studio::libraryItem("person");
        check(personAsset && personAsset->name == "Apresentador" &&
                  personAsset->fallbackId == "person" && personAsset->poseArtworks.size() == 12 &&
                  personAsset->resourcePath.ends_with("/neutral.png"),
              "the existing person ID now resolves to twelve embedded pose PNGs with the native fallback");
        check(personAsset && personAsset->poseArtwork("point") == personAsset->poseArtwork("point-right") &&
                  personAsset->poseArtwork("worried") == personAsset->poseArtwork("thinking"),
              "legacy point and worried names map to the documented presenter PNGs");
        check(studio::libraryItem("illustrated:car") == studio::libraryItem("car"),
              "legacy illustrated IDs resolve to the same original native entry");
        check(studio::libraryItem("rocket") && studio::libraryItem("night-city") &&
                  studio::libraryItem("chart-donut"),
              "new objects, data graphics and backgrounds exist in native catalog");
        studio::CompositionPainter painter;
        auto artworkFixture = studio::makeEmpty();
        auto artworkNode = [&](const std::string &id, double x, double y, double width, double height) {
            studio::Node node;
            node.id = id; node.kind = studio::NodeKind::Native; node.nativeId = id;
            node.name = id; node.x = x; node.y = y; node.width = width; node.height = height;
            node.animated = false; node.motion = "none";
            artworkFixture.nodes.push_back(node);
        };
        artworkNode("car", 24, 400, 460, 210);
        artworkNode("chip", 360, 80, 220, 220);
        artworkNode("flask", 660, 68, 220, 255);
        artworkNode("server", 940, 70, 215, 227);
        artworkNode("satellite", 645, 365, 340, 255);
        const auto artworkFrame = painter.frame(artworkFixture, 30);
        check(!artworkFrame.isNull() && artworkFrame.size() == QSize(1280, 720),
              "QPainter composes AI artwork when the recipe uses ordinary original native IDs");
        std::vector<std::pair<std::string, QImage>> presenterFrames;
        QImage presenterSheet(1280, 1200, QImage::Format_ARGB32_Premultiplied);
        presenterSheet.fill(QColor("#202b34"));
        QPainter sheetPainter(&presenterSheet);
        sheetPainter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform |
                                    QPainter::TextAntialiasing);
        const auto &poses = studio::poseNames();
        for (std::size_t index = 0; index < poses.size(); ++index) {
            const auto &pose = poses[index];
            const int column = int(index % 4), row = int(index / 4);
            auto sample = studio::makeEmpty();
            sample.width = 320; sample.height = 400; sample.duration = 360;
            sample.artDirection.backgroundColor = "#202b34";
            sample.artDirection.glowColor = "#202b34";
            studio::Node presenter;
            presenter.id = "presenter"; presenter.name = "Apresentador";
            presenter.kind = studio::NodeKind::Native; presenter.nativeId = "person";
            presenter.pose = pose; presenter.x = 35; presenter.y = 8;
            presenter.width = personAsset->width; presenter.height = personAsset->height;
            presenter.animated = false; presenter.motion = "none";
            sample.nodes.push_back(presenter);
            const auto rendered = painter.frame(sample, 48);
            check(!rendered.isNull() && rendered.size() == QSize(320, 400),
                  "each registered presenter pose renders through the shared QPainter compositor");
            presenterFrames.emplace_back(pose, rendered);
            sheetPainter.drawImage(column * 320, row * 400, rendered);
            sheetPainter.setPen(QColor("#e4f5e9"));
            sheetPainter.drawText(QRect(column * 320 + 12, row * 400 + 365, 296, 24),
                                  Qt::AlignLeft | Qt::AlignVCenter, QString::fromStdString(pose));
        }
        sheetPainter.end();
        for (std::size_t index = 0; index < presenterFrames.size(); ++index) {
            const auto *pose = personAsset->poseArtwork(presenterFrames[index].first);
            check(pose && !pose->resourcePath.empty(), "pose resolver returns a packaged image and anchor");
            const auto path = QString::fromStdString(pose->resourcePath);
            QImageReader reader(path);
            const auto decoded = reader.read();
            check(!decoded.isNull() && decoded.size() == QSize(410, 410) && decoded.hasAlphaChannel(),
                  "every source pose remains a transparent 410x410 RGBA PNG");
            bool transparent = false, opaque = false;
            for (int y = 0; y < decoded.height() && !(transparent && opaque); y += 5)
                for (int x = 0; x < decoded.width(); x += 5) {
                    const auto alpha = decoded.pixelColor(x, y).alpha();
                    transparent |= alpha == 0;
                    opaque |= alpha > 0;
                    if (transparent && opaque) break;
                }
            check(transparent && opaque, "transparent pixels and presenter pixels are both retained");
            check(index == 0 || sampledImageDifference(presenterFrames.front().second,
                                                       presenterFrames[index].second) > .5,
                  "each pose visibly differs while using the same person node geometry");
        }
        const auto presenterArtifactDir = QStringLiteral(STUDIO_SOURCE_DIR) + "/artifacts/presenter-01";
        check(QDir().mkpath(presenterArtifactDir) &&
                  presenterSheet.save(presenterArtifactDir + "/contact-sheet.png"),
              "contact sheet of all twelve unmodified poses is generated from the production compositor");
        QFile aiAssetManifest(QStringLiteral(STUDIO_SOURCE_DIR) + "/assets/ai-library/asset-manifest.json");
        check(aiAssetManifest.open(QIODevice::ReadOnly), "integrated AI asset manifest is present");
        const auto aiManifest = QJsonDocument::fromJson(aiAssetManifest.readAll()).object();
        check(aiManifest["importStatus"].toString().startsWith("INSTALLED") &&
                  aiManifest["studioDarkIntegration"].toObject()["defaultArtworkReplaced"].toBool() &&
                  !aiManifest["studioDarkIntegration"].toObject()["duplicateLibraryItems"].toBool(),
              "asset manifest records original-ID replacement without duplicate library rows");
        int transparentAssets = 0, previewAssets = 0;
        QSet<QString> nativeIds;
        for (const auto &entryValue : aiManifest["assets"].toArray()) {
            const auto entry = entryValue.toObject();
            const auto nativeId = entry["nativeId"].toString().toStdString();
            const auto expectedId = nativeId;
            const auto *asset = studio::libraryItem(expectedId);
            const auto path = QString::fromStdString(asset ? asset->resourcePath : std::string{});
            const auto idUnique = !nativeIds.contains(QString::fromStdString(expectedId));
            nativeIds.insert(QString::fromStdString(expectedId));
            QImageReader reader(path);
            const auto decoded = reader.read();
            check(asset && idUnique &&
                      asset->fallbackId == nativeId && !decoded.isNull() &&
                      entry["studioDarkAssetId"].toString() == QString::fromStdString(nativeId) &&
                      decoded.size() == QSize(entry["size"].toArray()[0].toInt(),
                                              entry["size"].toArray()[1].toInt()),
                  "original manifest ID, embedded Qt resource, internal fallback and dimensions agree");
            if (entry["kind"] == "transparent-illustration") {
                ++transparentAssets;
                check(decoded.hasAlphaChannel(), "each transparent native PNG retains its alpha channel");
            }
            if (entry["kind"] == "scene-preview") {
                ++previewAssets;
                check(asset->previewOnly, "low-resolution scene on its original ID remains preview-only");
            }
        }
        check(transparentAssets == 48 && previewAssets == 12,
              "the 60 replacements include 48 transparent objects and 12 scene previews");
        const auto originalCarProject = [&] {
            auto project = studio::makeEmpty();
            studio::Node node; node.id = "car"; node.kind = studio::NodeKind::Native; node.nativeId = "car";
            node.x = 410; node.y = 220; node.width = 460; node.height = 210; node.animated = false;
            project.nodes.push_back(node);
            return project;
        }();
        auto legacyAliasProject = originalCarProject;
        legacyAliasProject.nodes.front().nativeId = "illustrated:car";
        check(painter.frame(originalCarProject, 30) == painter.frame(legacyAliasProject, 30),
              "legacy illustrated project IDs render the same default PNG as the original native ID");
        auto proceduralReference = painter.frame(studio::makeEmpty(), 30);
        {
            QPainter referencePainter(&proceduralReference);
            referencePainter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
            referencePainter.translate(originalCarProject.nodes.front().x, originalCarProject.nodes.front().y);
            studio::paintNative(referencePainter, originalCarProject.nodes.front(), 30);
        }
        auto pngReference = painter.frame(studio::makeEmpty(), 30);
        {
            QPainter referencePainter(&pngReference);
            referencePainter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
            referencePainter.translate(originalCarProject.nodes.front().x, originalCarProject.nodes.front().y);
            const auto *asset = studio::libraryItem("car");
            QImageReader reader(QString::fromStdString(asset->resourcePath));
            const auto image = reader.read();
            QSizeF fitted = image.size();
            fitted.scale(QSizeF(originalCarProject.nodes.front().width,
                                originalCarProject.nodes.front().height), Qt::KeepAspectRatio);
            referencePainter.drawImage(QRectF((originalCarProject.nodes.front().width - fitted.width()) / 2.0,
                                              (originalCarProject.nodes.front().height - fitted.height()) / 2.0,
                                              fitted.width(), fitted.height()), image);
        }
        check(painter.frame(originalCarProject, 30) == pngReference &&
                  painter.frame(originalCarProject, 30) != proceduralReference,
              "ordinary car ID paints the embedded PNG pixels, not the old procedural default");
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

        // Simulate a catalogue saved by a previous release: older native registry
        // plus personal content. Reload must merge new entries, not replace data.
        QTemporaryDir migrationDir;
        check(migrationDir.isValid(), "temporary workspace for native migration");
        studio::WorkspaceStore migrationSeed(migrationDir.path());
        auto legacyCatalog = migrationSeed.data();
        QJsonArray oldBuiltins;
        for (const auto &entry : legacyCatalog["assets"].toArray()) {
            auto item = entry.toObject();
            if (item["nativeId"] == "car" || item["nativeId"] == "person")
                oldBuiltins.append(item);
        }
        oldBuiltins.append(QJsonObject{{"id", "illustrated:car"}, {"nativeId", "illustrated:car"},
                                       {"name", "Carro · ilustração IA"}, {"kind", "native"},
                                       {"categoryId", "objects"}, {"deleted", false}, {"builtin", true}});
        oldBuiltins.append(QJsonObject{{"id", "illustrated:street"}, {"nativeId", "illustrated:street"},
                                       {"name", "Rua · prévia IA"}, {"kind", "native"},
                                       {"categoryId", "scenes"}, {"deleted", false}, {"builtin", true}});
        oldBuiltins.append(QJsonObject{{"id", "personal:retained"},
                                      {"name", "Elemento pessoal"},
                                      {"kind", "image"},
                                      {"categoryId", "objects"},
                                      {"deleted", false},
                                      {"builtin", false}});
        legacyCatalog["assets"] = oldBuiltins;
        QJsonArray legacyCategories;
        for (const auto &entry : legacyCatalog["categories"].toArray()) {
            const auto id = entry.toObject()["id"].toString();
            if (id != "technology" && id != "communication" &&
                id != "finance" && id != "science")
                legacyCategories.append(entry);
        }
        legacyCatalog["categories"] = legacyCategories;
        {
            QFile f(migrationDir.filePath("catalogue.json"));
            check(f.open(QIODevice::WriteOnly), "write legacy catalogue");
            f.write(QJsonDocument(legacyCatalog).toJson());
        }
        studio::WorkspaceStore upgraded(migrationDir.path());
        check(upgraded.error().isEmpty() && !upgraded.asset("native:rocket").isEmpty() &&
                  !upgraded.asset("native:newsroom").isEmpty() &&
                  upgraded.asset("illustrated:car")["deleted"].toBool() &&
                  upgraded.asset("illustrated:street")["deleted"].toBool(),
              "upgrade retires old duplicate rows while registering missing original native IDs");
        int activeNativeRows = 0;
        for (const auto &entry : upgraded.data()["assets"].toArray())
            if (entry.toObject()["kind"] == "native" && !entry.toObject()["deleted"].toBool())
                ++activeNativeRows;
        check(activeNativeRows == 72, "workspace upgrade leaves exactly one active row for each native ID");
        check(upgraded.asset("personal:retained")["name"] == "Elemento pessoal" &&
                  upgraded.asset("native:person")["name"] == "Apresentador",
              "upgrade preserves personal files and previous native assets");
        check(upgraded.addCategory("Depois da migração").error.isEmpty(),
              "updated library categories and natives remain valid for new writes");
        studio::WorkspaceStore persistedUpgrade(migrationDir.path());
        check(persistedUpgrade.error().isEmpty() &&
                  !persistedUpgrade.asset("native:rocket").isEmpty() &&
                  persistedUpgrade.asset("illustrated:car")["deleted"].toBool() &&
                  persistedUpgrade.asset("personal:retained")["name"] == "Elemento pessoal",
              "retired duplicate tombstone and personal assets persist safely");
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
        QFile manifestFile(QStringLiteral(STUDIO_SOURCE_DIR) + "/assets/premium/smartphone-premium-01.manifest.json");
        check(manifestFile.open(QIODevice::ReadOnly), "read the versioned premium package manifest");
        auto manifest = QJsonDocument::fromJson(manifestFile.readAll()).object();
        check(studio::validatePremiumManifest(manifest, QStringLiteral(STUDIO_SOURCE_DIR) + "/assets/premium").isEmpty(),
              "version 1 premium manifest validates against its package root");
        auto internalPathManifest = manifest;
        auto internalLayers = internalPathManifest["layers"].toArray();
        auto internalLayer = internalLayers[0].toObject();
        internalLayer["source"] = "smartphone-premium-01.manifest.json";
        internalLayers[0] = internalLayer;
        internalPathManifest["layers"] = internalLayers;
        check(studio::validatePremiumManifest(internalPathManifest, QStringLiteral(STUDIO_SOURCE_DIR) + "/assets/premium").isEmpty(),
              "premium manifest accepts an existing relative file inside its package");
        auto traversalManifest = manifest;
        auto traversalLayers = traversalManifest["layers"].toArray();
        auto traversalLayer = traversalLayers[0].toObject();
        traversalLayer["source"] = "../../outside.png";
        traversalLayers[0] = traversalLayer;
        traversalManifest["layers"] = traversalLayers;
        check(!studio::validatePremiumManifest(traversalManifest, QStringLiteral(STUDIO_SOURCE_DIR) + "/assets/premium").isEmpty(),
              "premium manifests reject paths escaping their package");
        auto duplicateManifest = manifest;
        auto duplicateLayers = duplicateManifest["layers"].toArray();
        duplicateLayers.append(duplicateLayers[0]);
        duplicateManifest["layers"] = duplicateLayers;
        check(!studio::validatePremiumManifest(duplicateManifest, QStringLiteral(STUDIO_SOURCE_DIR) + "/assets/premium").isEmpty(),
              "premium manifests reject duplicate layer IDs");
        auto futureManifest = manifest;
        futureManifest["manifestVersion"] = 3;
        check(!studio::validatePremiumManifest(futureManifest, QStringLiteral(STUDIO_SOURCE_DIR) + "/assets/premium").isEmpty(),
              "unknown future manifest versions fail clearly without corrupting older packages");
        auto premiumScene = [](QString sceneId, QString narration, QString elementId, QString assetId) {
            return QJsonObject{{"id", sceneId}, {"narration", QJsonArray{narration}},
                               {"elements", QJsonArray{QJsonObject{{"id", elementId},
                                                                   {"kind", "library"},
                                                                   {"assetId", assetId},
                                                                   {"cue", 0}}}}};
        };
        auto premiumRecipe = QJsonObject{
            {"format", "studio-dark-production"}, {"version", 2}, {"catalogVersion", 1},
            {"title", "Templates editoriais demonstrativos"}, {"captions", false},
            {"musicGain", 0}, {"effectsGain", 0},
            {"scenes", QJsonArray{
                premiumScene("abertura", "Uma ideia começa com um título que se revela aos poucos.",
                             "title", "premium:title-reveal-01"),
                premiumScene("objeto", "O smartphone premium apresenta uma composição editável.",
                             "phone", "premium:smartphone-premium-01"),
                premiumScene("comparacao", "Compare cenários com informação verificada.",
                             "comparison", "premium:comparison-pack-01")}}};
        auto premiumParsed = studio::parseRecipe(QJsonDocument(premiumRecipe).toJson(), directory.path(), catalog.data());
        std::string premiumDiagnostic = "premium recipe parse: " + premiumParsed.error.toStdString();
        const auto subtreeNodes = [](const auto &self, const studio::DirectedElement &element) -> int {
            int total = 1;
            for (const auto &child : element.children)
                total += self(self, child);
            return total;
        };
        for (const auto &scene : premiumParsed.recipe.scenes)
            if (!scene.elements.empty())
                premiumDiagnostic += " [" + scene.id.toStdString() + ":" +
                                     std::to_string(subtreeNodes(subtreeNodes, scene.elements.front())) + "]";
        check(premiumParsed.error.isEmpty() && premiumParsed.recipe.scenes.size() == 3 &&
                  subtreeNodes(subtreeNodes, premiumParsed.recipe.scenes[0].elements.front()) == 6 &&
                  subtreeNodes(subtreeNodes, premiumParsed.recipe.scenes[1].elements.front()) == 19 &&
                  subtreeNodes(subtreeNodes, premiumParsed.recipe.scenes[2].elements.front()) == 17,
              premiumDiagnostic.c_str());
        QMap<QString, studio::PcmAudio> shortPremiumVoice;
        for (const auto &scene : premiumParsed.recipe.scenes)
            for (const auto &utterance : scene.narration) {
                studio::PcmAudio audio;
                audio.samples.resize(2 * 48000, 0);
                shortPremiumVoice.insert(utterance.id, std::move(audio));
            }
        const auto compiledPremium = studio::compileProduction(
            premiumParsed.recipe, shortPremiumVoice, directory.filePath("premium-short-voice.wav"));
        const auto *shortTitleRoot = studio::findNode(compiledPremium.project, "title");
        const auto shortTitleValid = !compiledPremium.error.isEmpty()
                                         ? compiledPremium.error.toStdString()
                                         : studio::validate(compiledPremium.project);
        check(compiledPremium.error.isEmpty() && shortTitleValid.empty() && shortTitleRoot &&
                  shortTitleRoot->duration < 360 && !shortTitleRoot->keyframes.empty() &&
                  shortTitleRoot->keyframes.back().frame <= shortTitleRoot->start + shortTitleRoot->duration,
              ("registered premium animations scale to measured narration spans without invalid keyframes: " +
               shortTitleValid + ", duration=" + (shortTitleRoot ? std::to_string(shortTitleRoot->duration) : "missing") +
               ", keys=" + (shortTitleRoot ? std::to_string(shortTitleRoot->keyframes.size()) : "0") +
               ", premium=" + std::to_string(premiumParsed.recipe.scenes.front().elements.front().premiumTemplate) +
               ", preset=" + std::to_string(premiumParsed.recipe.scenes.front().elements.front().presetTiming) +
               ", sceneDuration=" + std::to_string(compiledPremium.project.scenes.front().duration)).c_str());
        check(studio::aiGuide(catalog.data()).contains("premium:smartphone-premium-01") &&
                  studio::aiGuide(catalog.data()).contains("premium:title-reveal-01") &&
                  studio::aiGuide(catalog.data()).contains("premium:comparison-pack-01") &&
                  studio::aiGuide(catalog.data()).contains("premium:dramatic-quote-01") &&
                  studio::aiGuide(catalog.data()).contains("premium:lower-third-editorial-01") &&
                  studio::aiGuide(catalog.data()).contains("premium:source-label-01") &&
                  studio::aiGuide(catalog.data()).contains("premium:number-emphasis-01") &&
                  studio::aiGuide(catalog.data()).contains("premium:question-hook-01") &&
                  studio::aiGuide(catalog.data()).contains("premium:statistic-callout-pack-01") &&
                  studio::capabilities()["premiumAssets"].toArray().size() == 9,
              "AI guide and generated capability catalogue match the nine implemented premium assets");
        auto quotePaletteScene = QJsonObject{
            {"id", "quote"},
            {"narration", QJsonArray{"Uma frase forte ganha espaço e uma referência clara."}},
            {"elements", QJsonArray{QJsonObject{
                {"id", "quote-template"}, {"kind", "library"},
                {"assetId", "premium:dramatic-quote-01"}, {"cue", 0},
                {"palette", QJsonObject{{"accent", "#5CD6C2"}, {"text", "#F4E1A1"},
                                         {"muted", "#AEC3D4"}}}}}}};
        auto paletteRecipe = QJsonObject{{"format", "studio-dark-production"}, {"version", 2},
                                         {"catalogVersion", 1}, {"title", "Paleta premium"},
                                         {"captions", false}, {"musicGain", 0}, {"effectsGain", 0},
                                         {"scenes", QJsonArray{quotePaletteScene}}};
        const auto paletteParsed = studio::parseRecipe(QJsonDocument(paletteRecipe).toJson(),
                                                       directory.path(), catalog.data());
        const auto findRole = [](const auto &self, const studio::DirectedElement &element,
                                 const std::string &role) -> const studio::Node * {
            if (element.node.role == role)
                return &element.node;
            for (const auto &child : element.children)
                if (const auto *found = self(self, child, role))
                    return found;
            return nullptr;
        };
        const QString slotImagePath = directory.filePath("premium-slot-media/screen.png");
        check(QDir().mkpath(QFileInfo(slotImagePath).absolutePath()), "create isolated premium slot fixture folder");
        QImage screenFixture(240, 480, QImage::Format_ARGB32_Premultiplied);
        screenFixture.fill(QColor("#10223a"));
        {
            QPainter painter(&screenFixture);
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor("#4c83a4"));
            painter.drawRoundedRect(QRect(24, 48, 192, 84), 18, 18);
            painter.setBrush(QColor("#b4f078"));
            painter.drawEllipse(QRect(84, 172, 72, 72));
            painter.setBrush(QColor("#dbe8ef"));
            painter.drawRoundedRect(QRect(34, 282, 172, 16), 8, 8);
        }
        check(screenFixture.save(slotImagePath), "save a local original smartphone screen fixture");
        const auto customQuote = [](QString elementId, QString quote, QString attribution, QString source,
                                   QString color) {
            return QJsonObject{{"id", elementId}, {"kind", "library"},
                               {"assetId", "premium:dramatic-quote-01"}, {"cue", 0},
                               {"slots", QJsonObject{{"quote", quote}, {"attribution", attribution},
                                                       {"source", source}}},
                               {"palette", QJsonObject{{"accent", color}, {"text", "#f5ebca"},
                                                        {"muted", "#b9c6cb"}}}};
        };
        const QJsonObject customizedPhone{
            {"id", "phone-custom"}, {"kind", "library"},
            {"assetId", "premium:smartphone-premium-01"}, {"cue", 0},
            {"slots", QJsonObject{{"screen-title", "14:28"},
                                    {"notification", "Sua análise semanal está disponível."},
                                    {"screen", "premium-slot-media/screen.png"}}},
            {"palette", QJsonObject{{"surface", "#18222b"}, {"screen", "#101d2f"},
                                     {"accent", "#67d8b7"}, {"notification", "#e9f2f4"},
                                     {"frame", "#8ea3ad"}, {"highlight", "#dbbd71"},
                                     {"muted", "#c7d6dc"}, {"text", "#f7fbfc"}}}};
        auto personalizedScene = QJsonObject{
            {"id", "personalizados"},
            {"narration", QJsonArray{"Duas citações e um smartphone mostram slots preenchidos pela receita."}},
            {"elements", QJsonArray{customQuote("quote-one", "A mudança começa\ncom uma pergunta.",
                                                "CONTEXTO EDITORIAL", "FONTE EXEMPLO · 2026", "#54c9af"),
                                    customQuote("quote-two", "O dado precisa\nde uma fonte.",
                                                "NOTA DE PESQUISA", "REFERÊNCIA DEMONSTRATIVA", "#dc9b57"),
                                    customizedPhone}}};
        const auto personalizedRecipe = QJsonObject{
            {"format", "studio-dark-production"}, {"version", 2}, {"catalogVersion", 1},
            {"title", "Slots premium por receita"}, {"captions", false}, {"musicGain", 0},
            {"effectsGain", 0}, {"scenes", QJsonArray{personalizedScene}}};
        const auto personalizedParsed = studio::parseRecipe(QJsonDocument(personalizedRecipe).toJson(),
                                                            directory.path(), catalog.data());
        const auto *quoteOneText = personalizedParsed.error.isEmpty()
                                       ? findRole(findRole, personalizedParsed.recipe.scenes.front().elements[0],
                                                  "premium:quote:text")
                                       : nullptr;
        const auto *quoteTwoText = personalizedParsed.error.isEmpty()
                                       ? findRole(findRole, personalizedParsed.recipe.scenes.front().elements[1],
                                                  "premium:quote:text")
                                       : nullptr;
        const auto *phoneText = personalizedParsed.error.isEmpty()
                                    ? findRole(findRole, personalizedParsed.recipe.scenes.front().elements[2],
                                               "premium:text:notification")
                                    : nullptr;
        const auto *quoteOneAccent = personalizedParsed.error.isEmpty()
                                         ? findRole(findRole, personalizedParsed.recipe.scenes.front().elements[0],
                                                    "palette:accent")
                                         : nullptr;
        const auto *quoteTwoAccent = personalizedParsed.error.isEmpty()
                                         ? findRole(findRole, personalizedParsed.recipe.scenes.front().elements[1],
                                                    "palette:accent")
                                         : nullptr;
        const auto *phoneImage = personalizedParsed.error.isEmpty()
                                     ? findRole(findRole, personalizedParsed.recipe.scenes.front().elements[2],
                                                "premium:image:screen")
                                     : nullptr;
        check(personalizedParsed.error.isEmpty() && quoteOneText && quoteTwoText && phoneText && phoneImage &&
                  quoteOneText->text == "A mudança começa\ncom uma pergunta." &&
                  quoteTwoText->text == "O dado precisa\nde uma fonte." &&
                  quoteOneAccent && quoteTwoAccent && quoteOneAccent->color == "#54c9af" &&
                  quoteTwoAccent->color == "#dc9b57" &&
                  phoneText->text == "Sua análise semanal está disponível." &&
                  phoneImage->kind == studio::NodeKind::Image &&
                  phoneImage->source == QFileInfo(slotImagePath).canonicalFilePath().toStdString() &&
                  personalizedParsed.recipe.scenes.front().elements[0].children.size() > 0 &&
                  personalizedParsed.recipe.scenes.front().elements[1].children.size() > 0,
              "manifest slot IDs populate separate text/image layers without removing their animation hierarchy");
        auto unknownSlotRecipe = personalizedRecipe;
        auto unknownSlotScenes = unknownSlotRecipe["scenes"].toArray();
        auto unknownSlotScene = unknownSlotScenes[0].toObject();
        auto unknownSlotElements = unknownSlotScene["elements"].toArray();
        auto unknownSlotElement = unknownSlotElements[0].toObject();
        auto unknownSlots = unknownSlotElement["slots"].toObject();
        unknownSlots["internal-layer-id"] = "Do not allow direct internal layer addressing.";
        unknownSlotElement["slots"] = unknownSlots;
        unknownSlotElements[0] = unknownSlotElement;
        unknownSlotScene["elements"] = unknownSlotElements;
        unknownSlotScenes[0] = unknownSlotScene;
        unknownSlotRecipe["scenes"] = unknownSlotScenes;
        const auto unknownSlotParsed = studio::parseRecipe(QJsonDocument(unknownSlotRecipe).toJson(),
                                                           directory.path(), catalog.data());
        check(unknownSlotParsed.error.contains("premium:dramatic-quote-01") &&
                  unknownSlotParsed.error.contains("internal-layer-id"),
              "unknown premium layer/slot overrides are rejected with asset and slot IDs");
        auto unsafeImageRecipe = personalizedRecipe;
        auto unsafeScenes = unsafeImageRecipe["scenes"].toArray();
        auto unsafeScene = unsafeScenes[0].toObject();
        auto unsafeElements = unsafeScene["elements"].toArray();
        auto unsafePhone = unsafeElements[2].toObject();
        auto unsafeSlots = unsafePhone["slots"].toObject();
        unsafeSlots["screen"] = "../../outside.png";
        unsafePhone["slots"] = unsafeSlots;
        unsafeElements[2] = unsafePhone;
        unsafeScene["elements"] = unsafeElements;
        unsafeScenes[0] = unsafeScene;
        unsafeImageRecipe["scenes"] = unsafeScenes;
        const auto unsafeImageParsed = studio::parseRecipe(QJsonDocument(unsafeImageRecipe).toJson(),
                                                            directory.path(), catalog.data());
        check(unsafeImageParsed.error.contains("premium:smartphone-premium-01") &&
                  unsafeImageParsed.error.contains("screen"),
              "premium image slots reject traversal and identify the offending slot");
        QMap<QString, studio::PcmAudio> personalizedAudio;
        studio::PcmAudio personalizedCue;
        personalizedCue.samples.resize(48000, .05f);
        personalizedAudio.insert("personalizados-u1", std::move(personalizedCue));
        auto personalizedCompiled = studio::compileProduction(
            personalizedParsed.recipe, personalizedAudio, directory.filePath("personalized.wav"));
        const auto serializedPersonalized = studio::deserialize(studio::serialize(personalizedCompiled.project));
        const auto findSavedSlot = [](const studio::Project &project, const std::string &rootId,
                                      const std::string &role) -> const studio::Node * {
            const auto found = std::find_if(project.nodes.begin(), project.nodes.end(), [&](const auto &node) {
                return node.id.rfind(rootId + "-preset-", 0) == 0 && node.role == role;
            });
            return found == project.nodes.end() ? nullptr : &*found;
        };
        const auto *savedQuoteOne = findSavedSlot(serializedPersonalized.project, "quote-one", "premium:quote:text");
        const auto *savedQuoteTwo = findSavedSlot(serializedPersonalized.project, "quote-two", "premium:quote:text");
        const auto *savedAccentOne = findSavedSlot(serializedPersonalized.project, "quote-one", "palette:accent");
        const auto *savedAccentTwo = findSavedSlot(serializedPersonalized.project, "quote-two", "palette:accent");
        studio::WorkspaceStore managedSlotWorkspace(directory.filePath("slot-media-workspace"));
        const auto importedScreen = managedSlotWorkspace.importSlotMedia(slotImagePath);
        check(personalizedCompiled.error.isEmpty() && serializedPersonalized.error.isEmpty() &&
                  savedQuoteOne && savedQuoteTwo &&
                  savedQuoteOne->text == "A mudança começa\ncom uma pergunta." &&
                  savedQuoteTwo->text == "O dado precisa\nde uma fonte." &&
                  savedQuoteOne->color == "#f5ebca" && savedQuoteTwo->color == "#f5ebca" &&
                  savedAccentOne && savedAccentTwo && savedAccentOne->color == "#54c9af" &&
                  savedAccentTwo->color == "#dc9b57" &&
                  !importedScreen.id.isEmpty() && QFileInfo::exists(importedScreen.id),
              "slot values and per-instance palette survive production compilation and project reopen; image copy is managed");
        QFile::remove(slotImagePath);
        check(QFileInfo::exists(importedScreen.id),
              "managed premium image remains available after the original recipe image is removed");
        const auto *quoteColorNode = !paletteParsed.error.isEmpty() || paletteParsed.recipe.scenes.empty()
                                         ? nullptr
                                         : findRole(findRole, paletteParsed.recipe.scenes.front().elements.front(),
                                                    "premium:quote:text");
        const auto *quoteAccentNode = !paletteParsed.error.isEmpty() || paletteParsed.recipe.scenes.empty()
                                          ? nullptr
                                          : findRole(findRole, paletteParsed.recipe.scenes.front().elements.front(),
                                                     "palette:accent");
        check(paletteParsed.error.isEmpty() && !paletteParsed.recipe.scenes.empty() &&
                  quoteColorNode && quoteColorNode->color == "#F4E1A1" && quoteAccentNode &&
                  quoteAccentNode->color == "#5CD6C2",
              "validated JSON palette recolors real premium slot and reusable accent component");
        auto comparisonPaletteScene = QJsonObject{
            {"id", "comparison-palette"},
            {"narration", QJsonArray{"Cores de painel e títulos têm papéis independentes."}},
            {"elements", QJsonArray{QJsonObject{
                {"id", "comparison-template"}, {"kind", "library"},
                {"assetId", "premium:comparison-pack-01"}, {"cue", 0},
                {"palette", QJsonObject{{"left", "#205C50"}, {"right", "#5B3A20"},
                                         {"accent", "#E9D08C"}, {"text", "#F3F4EE"}}}}}}};
        auto comparisonPaletteRecipe = paletteRecipe;
        comparisonPaletteRecipe["scenes"] = QJsonArray{comparisonPaletteScene};
        const auto comparisonPaletteParsed = studio::parseRecipe(
            QJsonDocument(comparisonPaletteRecipe).toJson(), directory.path(), catalog.data());
        const auto *comparisonLeftTitle = comparisonPaletteParsed.error.isEmpty()
                                              ? findRole(findRole,
                                                         comparisonPaletteParsed.recipe.scenes.front().elements.front(),
                                                         "premium:comparison:left-title")
                                              : nullptr;
        const auto *comparisonRightTitle = comparisonPaletteParsed.error.isEmpty()
                                               ? findRole(findRole,
                                                          comparisonPaletteParsed.recipe.scenes.front().elements.front(),
                                                          "premium:comparison:right-title")
                                               : nullptr;
        const auto *comparisonLeftSurface = comparisonPaletteParsed.error.isEmpty()
                                                ? findRole(findRole,
                                                           comparisonPaletteParsed.recipe.scenes.front().elements.front(),
                                                           "palette:left")
                                                : nullptr;
        const auto *comparisonRightSurface = comparisonPaletteParsed.error.isEmpty()
                                                 ? findRole(findRole,
                                                            comparisonPaletteParsed.recipe.scenes.front().elements.front(),
                                                            "palette:right")
                                                 : nullptr;
        check(comparisonPaletteParsed.error.isEmpty() && comparisonLeftTitle && comparisonRightTitle &&
                  comparisonLeftTitle->color == "#E9D08C" && comparisonRightTitle->color == "#E9D08C" &&
                  comparisonLeftSurface && comparisonLeftSurface->color == "#205C50" &&
                  comparisonRightSurface && comparisonRightSurface->color == "#5B3A20",
              "comparison palette keeps panel surfaces distinct from high-contrast headings");
        bool visualReviewExamplesParse = true;
        QString visualReviewExampleError;
        for (const auto &example : {QStringLiteral("visual-quality-review-demo-v3.production.json"),
                                    QStringLiteral("visual-quality-problematic-v3.production.json")}) {
            QFile exampleFile(QStringLiteral(STUDIO_SOURCE_DIR) + "/examples/" + example);
            if (!exampleFile.open(QIODevice::ReadOnly)) {
                visualReviewExamplesParse = false;
                visualReviewExampleError = "missing " + example;
                break;
            }
            const auto parsedExample = studio::parseRecipe(exampleFile.readAll(), directory.path(), catalog.data());
            if (!parsedExample.error.isEmpty()) {
                visualReviewExamplesParse = false;
                visualReviewExampleError = example + ": " + parsedExample.error;
                break;
            }
        }
        check(visualReviewExamplesParse,
              ("visual-quality review examples parse with the real production parser: " +
               visualReviewExampleError).toStdString().c_str());
        const auto findColor = [](const auto &self, const studio::DirectedElement &element,
                                  const std::string &color) -> bool {
            if (element.node.color == color)
                return true;
            for (const auto &child : element.children)
                if (self(self, child, color))
                    return true;
            return false;
        };
        for (const auto &asset : studio::premiumLibrary()) {
            for (const auto tokenView : asset.paletteTokens) {
                const auto token = QString::fromUtf8(tokenView.data(), int(tokenView.size()));
                const auto assetId = QString::fromUtf8(asset.id.data(), int(asset.id.size()));
                const auto scene = QJsonObject{
                    {"id", "palette-token-test"}, {"narration", QJsonArray{"Teste de paleta."}},
                    {"elements", QJsonArray{QJsonObject{
                        {"id", "premium-palette-test"}, {"kind", "library"}, {"assetId", assetId},
                        {"cue", 0}, {"palette", QJsonObject{{token, "#C85CB4"}}}}}}};
                auto recipe = paletteRecipe;
                recipe["scenes"] = QJsonArray{scene};
                const auto parsed = studio::parseRecipe(QJsonDocument(recipe).toJson(),
                                                        directory.path(), catalog.data());
                if (!parsed.error.isEmpty())
                    throw std::runtime_error((assetId + ".palette." + token + ": " + parsed.error).toStdString());
                const auto &elements = parsed.recipe.scenes.front().elements;
                check(!elements.empty() && findColor(findColor, elements.front(), "#C85CB4"),
                      "each manifest palette token recolors an actual registered premium layer");
            }
        }
        const auto premiumCatalog = studio::capabilities()["premiumAssets"].toArray();
        bool catalogSlotsAreEditableOnly = premiumCatalog.size() == 9;
        for (const auto &assetValue : premiumCatalog) {
            const auto asset = assetValue.toObject();
            const auto registered = studio::premiumAsset(asset["id"].toString().toStdString());
            const auto roles = asset["slots"].toArray();
            catalogSlotsAreEditableOnly = catalogSlotsAreEditableOnly && registered &&
                                          roles.size() == int(registered->slotRoles.size());
            for (const auto &role : roles)
                catalogSlotsAreEditableOnly = catalogSlotsAreEditableOnly &&
                                               role.toString().startsWith("premium:");
        }
        check(catalogSlotsAreEditableOnly,
              "AI catalog separates editable premium slots from palette component roles");
        auto badPaletteRecipe = paletteRecipe;
        auto badPaletteScene = badPaletteRecipe["scenes"].toArray().at(0).toObject();
        auto badPaletteElement = badPaletteScene["elements"].toArray().at(0).toObject();
        badPaletteElement["palette"] = QJsonObject{{"invented", "#123456"}};
        QJsonArray badPaletteElements;
        badPaletteElements.append(badPaletteElement);
        badPaletteScene["elements"] = badPaletteElements;
        QJsonArray badPaletteScenes;
        badPaletteScenes.append(badPaletteScene);
        badPaletteRecipe["scenes"] = badPaletteScenes;
        check(!studio::parseRecipe(QJsonDocument(badPaletteRecipe).toJson(), directory.path(), catalog.data())
                   .error.isEmpty(),
              "palette rejects unknown semantic tokens rather than silently ignoring them");
        auto nativePaletteScene = QJsonObject{
            {"id", "native-colors"}, {"narration", QJsonArray{"A forma recebe uma paleta."}},
            {"elements", QJsonArray{QJsonObject{{"id", "chip"}, {"kind", "native"}, {"asset", "chip"},
                                                {"palette", QJsonObject{{"primary", "#34AADC"},
                                                                         {"secondary", "#F2B64B"}}}}}}};
        auto nativePaletteRecipe = paletteRecipe;
        nativePaletteRecipe["scenes"] = QJsonArray{nativePaletteScene};
        const auto nativePaletteParsed = studio::parseRecipe(QJsonDocument(nativePaletteRecipe).toJson(),
                                                             directory.path());
        check(nativePaletteParsed.error.isEmpty() &&
                  nativePaletteParsed.recipe.scenes.front().elements.front().node.color == "#34AADC" &&
                  nativePaletteParsed.recipe.scenes.front().elements.front().node.secondaryColor == "#F2B64B",
              "legacy color palette fields remain parseable and are preserved on PNG-backed native nodes");
        auto paletteNodeProject = studio::makeEmpty();
        auto paletteNode = nativePaletteParsed.recipe.scenes.front().elements.front().node;
        paletteNode.id = "chip-palette";
        paletteNode.x = 420; paletteNode.y = 200; paletteNode.width = 320; paletteNode.height = 240;
        paletteNode.animated = false;
        paletteNodeProject.nodes.push_back(paletteNode);
        auto noPaletteNodeProject = paletteNodeProject;
        noPaletteNodeProject.nodes.front().color = "#010203";
        noPaletteNodeProject.nodes.front().secondaryColor = "#AABBCC";
        check(painter.frame(paletteNodeProject, 30) == painter.frame(noPaletteNodeProject, 30),
              "color properties do not recolor static PNG pixels");
        auto pngRecipe = nativePaletteRecipe;
        auto pngScene = nativePaletteScene;
        auto pngElement = pngScene["elements"].toArray().at(0).toObject();
        pngElement["asset"] = "car";
        pngElement["motion"] = "rise";
        pngScene["elements"] = QJsonArray{pngElement};
        pngRecipe["scenes"] = QJsonArray{pngScene};
        const auto parsedPngRecipe = studio::parseRecipe(QJsonDocument(pngRecipe).toJson(), directory.path());
        check(parsedPngRecipe.error.isEmpty() &&
                  parsedPngRecipe.recipe.scenes.front().elements.front().node.nativeId == "car" &&
                  parsedPngRecipe.recipe.scenes.front().elements.front().node.color == "#34AADC",
              "AI recipes use the original native ID and retain old color fields without claiming PNG recoloring");
        auto legacyAliasRecipe = pngRecipe;
        auto legacyAliasScene = legacyAliasRecipe["scenes"].toArray().at(0).toObject();
        auto legacyAliasElement = legacyAliasScene["elements"].toArray().at(0).toObject();
        legacyAliasElement["asset"] = "illustrated:car";
        legacyAliasScene["elements"] = QJsonArray{legacyAliasElement};
        legacyAliasRecipe["scenes"] = QJsonArray{legacyAliasScene};
        const auto parsedLegacyAlias = studio::parseRecipe(QJsonDocument(legacyAliasRecipe).toJson(), directory.path());
        check(parsedLegacyAlias.error.isEmpty() &&
                  parsedLegacyAlias.recipe.scenes.front().elements.front().node.nativeId == "illustrated:car",
              "previously published illustrated IDs remain accepted when importing old recipes");
        QMap<QString, studio::PcmAudio> pngAudio;
        for (const auto &scene : parsedPngRecipe.recipe.scenes)
            for (const auto &utterance : scene.narration) {
                studio::PcmAudio audio; audio.samples.resize(24000, .02f);
                pngAudio.insert(utterance.id, std::move(audio));
            }
        const auto compiledPngRecipe = studio::compileProduction(
            parsedPngRecipe.recipe, pngAudio, directory.filePath("native-artwork-library.wav"));
        check(compiledPngRecipe.error.isEmpty() &&
                  std::any_of(compiledPngRecipe.project.nodes.begin(), compiledPngRecipe.project.nodes.end(),
                              [](const auto &node) { return node.nativeId == "car"; }) &&
                  studio::validate(compiledPngRecipe.project).empty(),
              "production compiler creates a valid editable project using the original native PNG ID");
        auto pngProjectRoundtrip = studio::deserialize(studio::serialize(compiledPngRecipe.project));
        check(pngProjectRoundtrip.error.isEmpty() &&
                  std::any_of(pngProjectRoundtrip.project.nodes.begin(), pngProjectRoundtrip.project.nodes.end(),
                              [](const auto &node) { return node.nativeId == "car"; }),
              "native ID and animation survive project save and reopen");
        studio::CompositionPainter pngPreviewPainter;
        check(!pngPreviewPainter.frame(compiledPngRecipe.project, 30).isNull() &&
                  pngPreviewPainter.frame(compiledPngRecipe.project, 30) !=
                      pngPreviewPainter.frame(compiledPngRecipe.project, 5),
              "QPainter preview renders the new default PNG and preserves its external motion");
        const QStringList premiumFactoryFixtures{
            "camera-travelling-horizontal-v3.production.json",
            "camera-travelling-vertical-v3.production.json",
            "premium-palette-v2.production.json",
            "premium-factory-motion-showcase-v3.production.json",
            "visual-beats-sequential-reveal-v3.production.json",
            "premium-dramatic-quote-slots-v2.production.json",
            "premium-comparison-slots-v2.production.json",
            "premium-smartphone-slots-v2.production.json",
            "premium-question-hook-slots-v2.production.json",
            "premium-statistic-callout-slots-v2.production.json",
            "presenter-poses-walk-cycle-v3.production.json",
            "camera-path-spatial-v3.production.json",
            "camera-path-combined-v3.production.json",
            "premium-factory-stage14-demo-v3.production.json"};
        for (const auto &fixtureName : premiumFactoryFixtures) {
            QFile fixture(QStringLiteral(STUDIO_SOURCE_DIR) + "/examples/" + fixtureName);
            check(fixture.open(QIODevice::ReadOnly), "read a published small recipe example");
            const auto parsedFixture = studio::parseRecipe(fixture.readAll(),
                QFileInfo(fixture.fileName()).absolutePath(), catalog.data());
            if (!parsedFixture.error.isEmpty())
                throw std::runtime_error((fixtureName + ": " + parsedFixture.error).toStdString());
            QMap<QString, studio::PcmAudio> measuredFixtures;
            studio::WordTimingMap fixtureWordStarts;
            for (const auto &scene : parsedFixture.recipe.scenes)
                for (const auto &utterance : scene.narration) {
                    studio::PcmAudio audio;
                    audio.samples.resize(2 * 48000, .02f);
                    measuredFixtures.insert(utterance.id, std::move(audio));
                    if (fixtureName == "presenter-poses-walk-cycle-v3.production.json") {
                        std::vector<std::int64_t> starts;
                        const auto words = utterance.text.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
                        for (int word = 0; word < words.size(); ++word)
                            starts.push_back(std::min<std::int64_t>(word * 4000, 42000));
                        fixtureWordStarts.insert(utterance.id, std::move(starts));
                    }
                }
            const auto fixtureProject = studio::compileProduction(
                parsedFixture.recipe, measuredFixtures, directory.filePath(fixtureName + ".wav"), nullptr,
                fixtureWordStarts.isEmpty() ? nullptr : &fixtureWordStarts);
            if (!fixtureProject.error.isEmpty())
                throw std::runtime_error((fixtureName + " compile: " + fixtureProject.error).toStdString());
            check(studio::validate(fixtureProject.project).empty() && fixtureProject.project.duration > 0,
                  "official example parses, compiles and validates into an editable project");
            studio::CompositionPainter fixturePainter;
            const auto proofFrame = fixturePainter.frame(fixtureProject.project,
                std::min<std::int64_t>(fixtureProject.project.duration - 1,
                                       fixtureProject.project.duration / 2));
            check(!proofFrame.isNull() && proofFrame.size() ==
                      QSize(fixtureProject.project.width, fixtureProject.project.height),
                  "official camera, reveal and color examples render a real QPainter frame");
            if (fixtureName == "presenter-poses-walk-cycle-v3.production.json") {
                const auto *person = studio::findNode(fixtureProject.project, "presenter");
                auto invalidPoseRecipe = parsedFixture.recipe.original;
                auto invalidScenes = invalidPoseRecipe["scenes"].toArray();
                auto invalidScene = invalidScenes[0].toObject();
                auto invalidShots = invalidScene["shots"].toArray();
                auto invalidShot = invalidShots[0].toObject();
                auto invalidElements = invalidShot["elements"].toArray();
                auto invalidPerson = invalidElements.at(invalidElements.size() - 1).toObject();
                invalidPerson["pose"] = "teleport";
                invalidElements[invalidElements.size() - 1] = invalidPerson;
                invalidShot["elements"] = invalidElements;
                invalidShots[0] = invalidShot;
                invalidScene["shots"] = invalidShots;
                invalidScenes[0] = invalidScene;
                invalidPoseRecipe["scenes"] = invalidScenes;
                check(!studio::parseRecipe(QJsonDocument(invalidPoseRecipe).toJson(),
                                           QFileInfo(fixture.fileName()).absolutePath(), catalog.data())
                               .error.isEmpty(),
                      "recipe parser rejects unregistered presenter pose names");
                check(person && person->nativeId == "person" && person->walkCycle.has_value() &&
                          person->poseKeyframes.size() == 4 && fixtureProject.project.schemaVersion == 16,
                      "presenter production compiles timed poses and a walk cycle into one editable native identity");
                check(person && studio::validate(fixtureProject.project).empty() &&
                          studio::evaluatePose(*person, person->poseKeyframes[0].frame - 1) == "neutral" &&
                          studio::evaluatePose(*person, person->poseKeyframes[0].frame) == "smile" &&
                          studio::evaluatePose(*person, person->poseKeyframes[1].frame) == "wave" &&
                          studio::evaluatePose(*person, person->poseKeyframes[2].frame) == "explain" &&
                          studio::evaluatePose(*person, person->poseKeyframes[3].frame) == "point-right" &&
                          studio::evaluatePose(*person, person->walkCycle->startFrame) == "walk-a" &&
                          studio::evaluatePose(*person, person->walkCycle->startFrame + person->walkCycle->stepFrames) == "walk-b" &&
                          studio::evaluatePose(*person, person->walkCycle->endFrame) == "smile",
                      "pose cues persist until the next event and the walk alternates deterministically before stopping");
                check(person && studio::evaluate(*person, person->walkCycle->startFrame).x !=
                                      studio::evaluate(*person, person->walkCycle->endFrame - 1).x,
                      "presenter recipe keyframes move the character across the scene while walking");
                const auto unalignedCompile = studio::compileProduction(
                    parsedFixture.recipe, measuredFixtures, directory.filePath("presenter-no-alignment.wav"));
                check(unalignedCompile.error.contains("wordIndex"),
                      "wordIndex pose timing is rejected when the voice has no word-boundary alignment");
                const auto poseFolder = QStringLiteral(STUDIO_SOURCE_DIR) + "/artifacts/presenter-01/demo-frames";
                QDir().mkpath(poseFolder);
                studio::CompositionPainter presenterPainter;
                QJsonArray poseFrames;
                auto savePoseFrame = [&](std::int64_t frame, QString label) {
                    const auto image = presenterPainter.frame(fixtureProject.project, frame);
                    const auto path = QDir(poseFolder).filePath(label + QString("-%1.png").arg(frame));
                    check(!image.isNull() && image.save(path), "presenter production frame saved for visual review");
                    poseFrames.append(QJsonObject{{"frame", qint64(frame)}, {"pose", label},
                                                  {"x", studio::evaluate(*person, frame).x},
                                                  {"png", QFileInfo(path).fileName()}});
                };
                for (std::size_t i = 0; i < person->poseKeyframes.size(); ++i) {
                    const auto &key = person->poseKeyframes[i];
                    if (key.frame > 0)
                        savePoseFrame(key.frame - 1, QString::fromStdString(studio::evaluatePose(*person, key.frame - 1)));
                    savePoseFrame(key.frame, QString::fromStdString(key.pose));
                }
                savePoseFrame(person->walkCycle->startFrame, "walk-a");
                savePoseFrame(person->walkCycle->startFrame + person->walkCycle->stepFrames, "walk-b");
                savePoseFrame(person->walkCycle->endFrame, "smile-stop");
                QFile frameReport(QDir(poseFolder).filePath("pose-states.json"));
                check(frameReport.open(QIODevice::WriteOnly) &&
                          frameReport.write(QJsonDocument(QJsonObject{{"frames", poseFrames}})
                                                .toJson(QJsonDocument::Indented)) > 0,
                      "presenter recipe records the tested pose frames and positions");
                const auto savedProject = studio::deserialize(studio::serialize(fixtureProject.project));
                const auto originalWalkFrame = presenterPainter.frame(
                    fixtureProject.project,
                    person->walkCycle->startFrame + person->walkCycle->stepFrames);
                const auto reopenedWalkFrame = presenterPainter.frame(
                    savedProject.project,
                    person->walkCycle->startFrame + person->walkCycle->stepFrames);
                check(savedProject.error.isEmpty() && studio::validate(savedProject.project).empty() &&
                          originalWalkFrame == reopenedWalkFrame &&
                          studio::evaluatePose(*studio::findNode(savedProject.project, "presenter"),
                                               person->walkCycle->startFrame + person->walkCycle->stepFrames) == "walk-b",
                      "presenter pose rendering and walk cycle survive project save and reopen identically");
            }
            if (fixtureName == "premium-dramatic-quote-slots-v2.production.json" ||
                fixtureName == "premium-comparison-slots-v2.production.json" ||
                fixtureName == "premium-smartphone-slots-v2.production.json" ||
                fixtureName == "premium-question-hook-slots-v2.production.json" ||
                fixtureName == "premium-statistic-callout-slots-v2.production.json") {
                const auto projectRoundTrip = studio::deserialize(studio::serialize(fixtureProject.project));
                check(projectRoundTrip.error.isEmpty() &&
                          studio::validate(projectRoundTrip.project).empty() &&
                          projectRoundTrip.project.nodes.size() == fixtureProject.project.nodes.size(),
                      "slot/palette example survives project save and reopen without losing expanded layers");
            }
            if (fixtureName == "premium-dramatic-quote-slots-v2.production.json")
                check(std::any_of(fixtureProject.project.nodes.begin(), fixtureProject.project.nodes.end(),
                                  [](const auto &node) {
                                      return node.text == "Toda mudança\ncomeça com uma pergunta.";
                                  }),
                      "Dramatic Quote JSON replaces the actual quote text layer");
            if (fixtureName == "premium-comparison-slots-v2.production.json")
                check(std::any_of(fixtureProject.project.nodes.begin(), fixtureProject.project.nodes.end(),
                                  [](const auto &node) { return node.text == "CENÁRIO A"; }) &&
                          std::any_of(fixtureProject.project.nodes.begin(), fixtureProject.project.nodes.end(),
                                      [](const auto &node) { return node.text == "12,4"; }),
                      "Comparison Pack JSON replaces panel headings and values in real nodes");
            if (fixtureName == "premium-question-hook-slots-v2.production.json")
                check(std::any_of(fixtureProject.project.nodes.begin(), fixtureProject.project.nodes.end(),
                                  [](const auto &node) {
                                      return node.text == "QUEM DECIDE\nO QUE MERECE RESPOSTA?";
                                  }),
                      "Question Hook JSON populates its declared question slot");
            if (fixtureName == "premium-statistic-callout-slots-v2.production.json")
                check(std::any_of(fixtureProject.project.nodes.begin(), fixtureProject.project.nodes.end(),
                                  [](const auto &node) { return node.text == "12,4"; }) &&
                          std::any_of(fixtureProject.project.nodes.begin(), fixtureProject.project.nodes.end(),
                                      [](const auto &node) {
                                          return node.text == "NÃO É UM DADO REAL · SUBSTITUA POR FONTE VERIFICADA";
                                      }),
                      "Statistic Callout JSON populates value and source slots while marking its example");
            if (fixtureName == "premium-smartphone-slots-v2.production.json") {
                const auto imageNode = std::find_if(fixtureProject.project.nodes.begin(),
                    fixtureProject.project.nodes.end(), [](const auto &node) {
                        return node.role == "premium:image:screen";
                    });
                check(imageNode != fixtureProject.project.nodes.end() &&
                          imageNode->kind == studio::NodeKind::Image &&
                          QFileInfo::exists(QString::fromStdString(imageNode->source)),
                      "official smartphone slot recipe resolves its bundled local demo image");
            }
            if (fixtureName == "camera-path-spatial-v3.production.json" ||
                fixtureName == "camera-path-combined-v3.production.json") {
                const bool combined = fixtureName == "camera-path-combined-v3.production.json";
                const auto &cameraProject = fixtureProject.project;
                const auto pathKeys = std::vector<studio::CameraKeyframe>(cameraProject.camera.begin(),
                                                                           cameraProject.camera.end());
                check(pathKeys.size() >= 2 &&
                          pathKeys.back().x != pathKeys.front().x,
                      "camera.path compiles target framing to editable camera keyframes");
                const auto colorPixels = [](const QImage &image, const QColor &color) {
                    qsizetype count = 0;
                    for (int y = 0; y < image.height(); ++y)
                        for (int x = 0; x < image.width(); ++x)
                            if (image.pixelColor(x, y) == color)
                                ++count;
                    return count;
                };
                studio::CompositionPainter cameraPainter;
                const QColor chipColor("#4CC9A9");
                const QColor serverColor("#DC9B57");
                const auto startFrame = pathKeys.front().frame;
                const auto endFrame = pathKeys.back().frame;
                const auto startImage = cameraPainter.frame(cameraProject, startFrame);
                const auto endImage = cameraPainter.frame(cameraProject, endFrame);
                check(colorPixels(startImage, chipColor) > 0 && colorPixels(endImage, chipColor) == 0 &&
                          colorPixels(startImage, serverColor) == 0 &&
                          colorPixels(endImage, serverColor) > 0,
                      "camera path moves from the first target to the next, framing one at a time");
                if (!combined) {
                    bool spatialEntryObserved = false;
                    for (auto frame = startFrame + 1; frame < endFrame; ++frame)
                        if (colorPixels(cameraPainter.frame(cameraProject, frame), serverColor) > 0) {
                            spatialEntryObserved = true;
                            break;
                        }
                    check(spatialEntryObserved && cameraProject.visualBeats.empty(),
                          "spatial reveal enters during camera travel without a Visual Beat show");
                } else {
                    const auto beat = std::find_if(cameraProject.visualBeats.begin(),
                        cameraProject.visualBeats.end(), [](const auto &item) {
                            return item.id == "reveal-server-during-travel";
                        });
                    check(beat != cameraProject.visualBeats.end() &&
                              colorPixels(cameraPainter.frame(cameraProject, beat->frame - 1), serverColor) == 0 &&
                              colorPixels(cameraPainter.frame(cameraProject, beat->frame), serverColor) > 0,
                          "combined spatial and temporal reveal stays hidden until its exact Visual Beat frame");
                }
            }
        }
        check(studio::capabilities()["productionCapabilities"].toArray().contains("personal-library-video-assets"),
              "AI capability catalogue distinguishes real library video from premium slots");
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
        const auto expandedShortPlan = studio::planNarrationTiming(shortRecipe.recipe, timedRaw);
        check(shortRecipe.error.isEmpty() && expandedShortPlan.error.isEmpty() &&
                  expandedShortPlan.targetFrames > 3 * 30 && !expandedShortPlan.warnings.isEmpty(),
              "unattainable target expands to measured speech duration and reports a warning");
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
        check(studio::capabilities()["supportedCanvases"].toArray().size() == 3 &&
                  studio::capabilities()["productionCapabilities"].toArray().contains(
                      "landscape-and-portrait-canvas-presets") &&
                  studio::aiGuide().contains("\"canvas\":{\"width\":1080,\"height\":1920}"),
              "capabilities and AI guide expose only the supported vertical and landscape canvases");
        auto g = studio::aiGuide();
        for (const auto &asset : studio::nativeLibrary())
            check(g.contains("`" + QString::fromStdString(asset.id) + "`"),
                  "guide derives all real library IDs");
        check(studio::capabilities()["nativeAssets"].toArray().size() == int(studio::nativeLibrary().size()),
              "machine catalogue matches renderer registry");
        const auto nativeCatalogue = studio::capabilities()["nativeAssets"].toArray();
        int pngDefaultCount = 0, lowResolutionSceneCount = 0, proceduralCount = 0;
        for (const auto &assetValue : nativeCatalogue) {
            const auto asset = assetValue.toObject();
            check(studio::libraryItem(asset["id"].toString().toStdString()) &&
                      g.contains("`" + asset["id"].toString() + "`"),
                  "AI guide and catalogue expose only the registered original native IDs");
            if (asset["rendering"] == "embedded-ai-png") {
                ++pngDefaultCount;
                if (asset["previewOnly"].toBool())
                    ++lowResolutionSceneCount;
                check(!asset["pixelRecoloring"].toBool(),
                      "machine catalogue does not promise recoloring of embedded PNG pixels");
            } else if (asset["rendering"] == "procedural") {
                ++proceduralCount;
            }
        }
        check(nativeCatalogue.size() == 72 && pngDefaultCount == 61 && lowResolutionSceneCount == 12 &&
                  proceduralCount == 11 && !studio::capabilities().contains("illustratedAssets") &&
                  g.contains("asset: \"car\"") && g.contains("não alteram os pixels desses 60 PNGs") &&
                  g.contains("prévias rasterizadas de baixa resolução") &&
                  g.contains("person"),
              "AI catalogue exposes 61 default PNGs and 11 dynamic natives under 72 original IDs without duplicates");
        std::cout << "PASS: " << checks
                  << " production checks (recipe, audio timing, mix, migration, catalogue, rendering)\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "FAIL after " << checks << ": " << e.what() << '\n';
        return 1;
    }
}
