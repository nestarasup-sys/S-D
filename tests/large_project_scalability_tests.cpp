#include <QDir>
#include <QDateTime>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <studio/composition/painter.hpp>
#include <studio/composition/render_snapshot.hpp>
#include <studio/core/editor.hpp>
#include <studio/production/compiler.hpp>
#include <studio/storage/project_store.hpp>
#include <studio/storage/workspace_store.hpp>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#ifdef Q_OS_WIN
#include <windows.h>
#include <psapi.h>
#endif

namespace {
void require(bool value, const QString &message) {
    if (!value)
        throw std::runtime_error(message.toStdString());
}
quint64 privateBytes() {
#ifdef Q_OS_WIN
    PROCESS_MEMORY_COUNTERS_EX counters{};
    if (GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS *>(&counters),
                             sizeof(counters)))
        return counters.PrivateUsage;
#endif
    return 0;
}
quint64 peakWorkingSetBytes() {
#ifdef Q_OS_WIN
    PROCESS_MEMORY_COUNTERS_EX counters{};
    if (GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS *>(&counters),
                             sizeof(counters)))
        return counters.PeakWorkingSetSize;
#endif
    return 0;
}
double elapsedMs(const QElapsedTimer &timer) {
    return double(timer.nsecsElapsed()) / 1'000'000.0;
}
QJsonObject layer(QString id, QString kind = "text") {
    QJsonObject node{{"id", id}, {"kind", kind}, {"x", 24}, {"y", 24}, {"width", 520}, {"height", 62},
                     {"fontSize", 26}, {"cue", 0}};
    if (kind == "text")
        node["text"] = QString(1200, QChar('A'));
    else if (kind == "native")
        node["asset"] = "car";
    return node;
}
QJsonObject nestedGroup() {
    QJsonObject child = layer("deep-leaf");
    for (int depth = 7; depth >= 0; --depth)
        child = QJsonObject{{"id", QString("deep-group-%1").arg(depth)}, {"kind", "group"}, {"cue", 0},
                            {"children", QJsonArray{child}}};
    return child;
}
QJsonObject largeRecipe(int requestedNodes) {
    constexpr int premiumLayers = 42; // 6 + 19 + 17 layers in current registered builders.
    constexpr int nestedGroupLayers = 9; // Eight nested group levels and one content layer.
    const int ordinaryLayers = requestedNodes - premiumLayers - nestedGroupLayers - 1; // one caption.
    require(ordinaryLayers > 0, "stress fixture size must fit its premium composition");
    QJsonArray elements{
        QJsonObject{{"id", "phone"}, {"kind", "library"}, {"assetId", "premium:smartphone-premium-01"}, {"cue", 0}},
        QJsonObject{{"id", "title"}, {"kind", "library"}, {"assetId", "premium:title-reveal-01"}, {"cue", 0}},
        QJsonObject{{"id", "comparison"}, {"kind", "library"}, {"assetId", "premium:comparison-pack-01"}, {"cue", 0}},
        nestedGroup()};
    for (int i = 0; i < ordinaryLayers; ++i) {
        const auto kind = i % 97 == 0 ? "native" : i % 11 == 0 ? "shape" : "text";
        auto node = layer(QString("ordinary-%1").arg(i), kind);
        if (i < 3)
            node["keyframes"] = QJsonArray{QJsonObject{{"cue", 0}, {"x", 24}, {"y", 24}},
                                           QJsonObject{{"cue", 0}, {"offsetSeconds", .25}, {"x", 36}, {"y", 30}}};
        elements.append(node);
    }
    // One deliberately long caption-like line pushes the largest valid recipe beyond the historical 1 MiB ceiling.
    QJsonObject shot{{"id", "shot-a"}, {"name", "Composição completa"},
                     {"start", QJsonObject{{"cue", 0}, {"cueProgress", 0}}},
                     {"transitionIn", "cut"},
                     {"camera", QJsonObject{{"x", 640}, {"y", 360}, {"zoom", 1}, {"rotation", 0}}},
                     {"elements", elements},
                     {"beats", QJsonArray{
                         QJsonObject{{"id", "move-camera"}, {"at", QJsonObject{{"cue", 0}, {"cueProgress", .45}}},
                                      {"action", "camera"}, {"durationFrames", 8}, {"x", 650}, {"y", 360}, {"zoom", 1.08}},
                         QJsonObject{{"id", "move-layer"}, {"at", QJsonObject{{"cue", 0}, {"cueProgress", .7}}},
                                      {"action", "transform"}, {"target", "ordinary-1"}, {"durationFrames", 6},
                                      {"x", 42}, {"y", 38}}}}};
    QJsonObject scene{{"id", "large-scene"}, {"narration", QJsonArray{"Teste estrutural de produção complexa."}},
                      {"shots", QJsonArray{shot}}};
    return {{"format", "studio-dark-production"}, {"version", 3}, {"catalogVersion", 1},
            {"title", "Fixture progressiva de escalabilidade"}, {"voice", "auto"}, {"captions", true},
            {"editing", QJsonObject{{"intensity", 35}, {"rhythm", "manual"}}},
            {"musicGain", 0}, {"effectsGain", 0}, {"scenes", QJsonArray{scene}}};
}
}

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    try {
        QTemporaryDir temp;
        require(temp.isValid(), "temporary directory available");
        studio::WorkspaceStore workspace(temp.filePath("workspace"));
        require(workspace.error().isEmpty(), "built-in library loads for real premium expansion");
        QJsonArray measurements;
        quint64 peakBytes = privateBytes();
        const int cases[] = {201, 217, 300, 500, 1000, 3000};
        for (const int requested : cases) {
            const quint64 beforeBytes = privateBytes();
            QJsonObject measurement;
            measurement["requestedEffectiveNodes"] = requested;
            measurement["privateBytesBefore"] = double(beforeBytes);
            measurement["peakWorkingSetBytesBefore"] = double(peakWorkingSetBytes());
            {
                const auto recipeBytes = QJsonDocument(largeRecipe(requested)).toJson(QJsonDocument::Compact);
                measurement["recipeBytes"] = recipeBytes.size();
                if (requested == 3000)
                    require(recipeBytes.size() > 1024 * 1024, "large production recipe exceeds old 1 MiB cutoff");
                QElapsedTimer timer;
                timer.start();
                auto parsed = studio::parseRecipe(recipeBytes, temp.path(), workspace.data());
                measurement["parseMs"] = elapsedMs(timer);
                require(parsed.error.isEmpty(), QString("%1-node production parses: %2").arg(requested).arg(parsed.error));
                peakBytes = std::max(peakBytes, privateBytes());

                QMap<QString, studio::PcmAudio> audio;
                studio::PcmAudio voice;
                voice.samples.resize(2 * 48000, .04f);
                audio.insert(parsed.recipe.scenes.front().narration.front().id, std::move(voice));
                timer.restart();
                auto compiled = studio::compileProduction(parsed.recipe, audio, temp.filePath("large-voice.wav"));
                measurement["compileMs"] = elapsedMs(timer);
                require(compiled.error.isEmpty(), QString("%1-node production compiles: %2").arg(requested).arg(compiled.error));
                measurement["effectiveNodes"] = double(compiled.project.nodes.size());
                require(compiled.project.nodes.size() == std::size_t(requested),
                        QString("premium/group/caption expansion yields exactly %1 nodes, got %2")
                            .arg(requested).arg(compiled.project.nodes.size()));
                require(std::count_if(compiled.project.nodes.begin(), compiled.project.nodes.end(),
                                      [](const auto &node) { return node.role == "caption"; }) == 1,
                        "automatic caption is present in large production");
                require(compiled.project.shots.size() == 1 && compiled.project.visualBeats.size() == 2 &&
                            !compiled.project.camera.empty(),
                        "shots, camera and Visual Beats survive large compilation");
                timer.restart();
                const auto validation = studio::validate(compiled.project);
                measurement["validationMs"] = elapsedMs(timer);
                require(validation.empty(), QString("%1-node project validates: %2").arg(requested).arg(QString::fromStdString(validation)));
                peakBytes = std::max(peakBytes, privateBytes());

                const auto projectPath = temp.filePath(QString("large-%1.studio.json").arg(requested));
                timer.restart();
                const auto saved = studio::saveProject(projectPath, compiled.project, {});
                measurement["saveMs"] = elapsedMs(timer);
                require(saved.error.isEmpty(), QString("%1-node project saves: %2").arg(requested).arg(saved.error));
                measurement["projectBytes"] = QFileInfo(projectPath).size();
                if (requested == 3000)
                    require(QFileInfo(projectPath).size() > 4 * 1024 * 1024,
                            "large saved project exceeds old 4 MiB cutoff");
                timer.restart();
                auto reopened = studio::loadProject(projectPath);
                measurement["reopenMs"] = elapsedMs(timer);
                require(reopened.error.isEmpty() && reopened.project.nodes.size() == std::size_t(requested),
                        QString("%1-node project reopens intact: %2").arg(requested).arg(reopened.error));
                peakBytes = std::max(peakBytes, privateBytes());

                timer.restart();
                const auto snapshot = studio::buildRenderFrameSnapshot(reopened.project, 12);
                measurement["snapshotMs"] = elapsedMs(timer);
                require(snapshot.nodes.size() == std::size_t(requested), "snapshot includes all expanded layers");
                studio::CompositionPainter painter;
                timer.restart();
                const auto rendered = painter.frame(reopened.project, 12);
                measurement["renderMs"] = elapsedMs(timer);
                require(!rendered.isNull() && rendered.size() == QSize(1280, 720), "one reference frame renders");
                studio::Editor history(reopened.project);
                const auto *historyNode = studio::findNode(history.project(), "ordinary-1");
                require(historyNode != nullptr, "large project node is addressable for editing");
                const double originalX = historyNode->x;
                timer.restart();
                const auto editError = history.execute([](studio::Project &project) {
                    const auto node = std::find_if(project.nodes.begin(), project.nodes.end(),
                                                   [](const auto &item) { return item.id == "ordinary-1"; });
                    if (node != project.nodes.end())
                        node->x += 2;
                });
                require(editError.empty() && history.undo() &&
                            studio::findNode(history.project(), "ordinary-1")->x == originalX &&
                            history.redo() && studio::findNode(history.project(), "ordinary-1")->x == originalX + 2,
                        QString("edit/undo/redo works at %1 nodes").arg(requested));
                measurement["editUndoRedoMs"] = elapsedMs(timer);
                peakBytes = std::max(peakBytes, privateBytes());
                measurement["privateBytesDuring"] = double(privateBytes());
                measurement["peakWorkingSetBytesDuring"] = double(peakWorkingSetBytes());
            }
            measurement["privateBytesAfterRelease"] = double(privateBytes());
            measurement["peakWorkingSetBytesAfterRelease"] = double(peakWorkingSetBytes());
            measurements.append(measurement);
        }
        QJsonObject report{{"format", "studio-dark-large-project-benchmark"}, {"timestamp", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},
                           {"policy", "resource-bound; no fixed node or JSON/project byte ceiling"},
                           {"maxSampledPrivateBytes", double(peakBytes)}, {"cases", measurements}};
        const auto artifactDir = QStringLiteral(STUDIO_SOURCE_DIR) + "/artifacts/large-project-scaling";
        require(QDir().mkpath(artifactDir), "create ignored artifact directory");
        QFile output(artifactDir + "/report.json");
        require(output.open(QIODevice::WriteOnly | QIODevice::Truncate), "open scaling report");
        output.write(QJsonDocument(report).toJson(QJsonDocument::Indented));
        output.close();
        std::cout << "Large project scaling passed: 201, 217, 300, 500, 1000, 3000 nodes. Report: "
                  << (artifactDir + "/report.json").toStdString() << '\n';
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
