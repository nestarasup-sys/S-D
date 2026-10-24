#include <QFile>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <studio/composition/painter.hpp>
#include <studio/core/library.hpp>
#include <studio/production/compiler.hpp>
#include <studio/storage/project_store.hpp>

static int checks = 0;
static void check(bool value, const char *reason) {
    ++checks;
    if (!value)
        throw std::runtime_error(reason);
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
        auto legacy = QJsonDocument::fromJson(studio::serialize(studio::makeDemo())).object();
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
