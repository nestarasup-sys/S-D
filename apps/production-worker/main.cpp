#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QProcess>
#include <QSaveFile>
#include <QTemporaryDir>
#include <QTextStream>
#include <memory>
#include <cmath>
#include <algorithm>
#include <studio/production/compiler.hpp>
#include <studio/storage/project_store.hpp>
#include <studio/storage/workspace_store.hpp>

static bool write(const QString &path, const QByteArray &bytes) {
    QSaveFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size() && file.commit();
}
static void report(QString stage, int progress, QString error = {}) {
    QTextStream out(stdout);
    out.setEncoding(QStringConverter::Utf8);
    out << QJsonDocument(QJsonObject{{"stage", stage}, {"progress", progress}, {"error", error}})
               .toJson(QJsonDocument::Compact)
        << Qt::endl;
}
static QString missingMedia(const studio::DirectedElement &element) {
    if (element.node.kind == studio::NodeKind::Image || element.node.kind == studio::NodeKind::Svg) {
        const auto source = QString::fromStdString(element.node.source);
        if (!QFileInfo::exists(source))
            return source;
    }
    for (const auto &child : element.children)
        if (auto missing = missingMedia(child); !missing.isEmpty())
            return missing;
    return {};
}
int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    QCommandLineParser parser;
    parser.addHelpOption();
    for (auto key : {"recipe", "output-dir", "guide", "capabilities", "cancel-file", "workspace-root"})
        parser.addOption({key, key, key});
    parser.process(app);
    std::unique_ptr<studio::WorkspaceStore> workspace;
    if (parser.isSet("workspace-root"))
        workspace = std::make_unique<studio::WorkspaceStore>(parser.value("workspace-root"));
    if (parser.isSet("guide"))
        return write(parser.value("guide"),
                     studio::aiGuide(workspace ? workspace->data() : QJsonObject{}).toUtf8())
                   ? 0
                   : 1;
    if (parser.isSet("capabilities"))
        return write(parser.value("capabilities"), QJsonDocument(studio::capabilities()).toJson()) ? 0 : 1;
    auto fail = [](QString error) {
        report("failed", 0, error);
        return 1;
    };
    QFile input(parser.value("recipe"));
    if (!input.open(QIODevice::ReadOnly) || input.size() > 1024 * 1024)
        return fail("Não foi possível ler a receita (máximo 1 MiB).");
    auto bytes = input.readAll();
    auto parsed = studio::parseRecipe(bytes, QFileInfo(input).absolutePath(),
                                      workspace ? workspace->data() : QJsonObject{});
    if (!parsed.error.isEmpty())
        return fail(parsed.error);
    for (const auto &scene : parsed.recipe.scenes)
        for (const auto &element : scene.elements)
            if (auto missing = missingMedia(element); !missing.isEmpty())
                return fail("Mídia local ausente: " + missing);
    auto output = QDir(parser.value("output-dir")).absolutePath();
    if (parser.value("output-dir").isEmpty() || QDir(output).exists())
        return fail("Escolha uma pasta de produção nova.");
    if (!QDir().mkpath(output + "/media"))
        return fail("Não foi possível criar a pasta de produção.");
    QTemporaryDir temp;
    auto speechPath = temp.filePath("synthesize.ps1");
    if (!QFile::copy(":/production/synthesize.ps1", speechPath))
        return fail("Script de voz indisponível.");
    QJsonArray lines;
    for (const auto &scene : parsed.recipe.scenes)
        for (const auto &u : scene.narration)
            lines.append(QJsonObject{{"id", u.id}, {"text", u.text}});
    auto request = temp.filePath("speech.json");
    if (!write(request, QJsonDocument(QJsonObject{{"voice", parsed.recipe.voice},
                                                  {"speechRate", parsed.recipe.speechRate},
                                                  {"utterances", lines}})
                            .toJson()))
        return fail("Falha ao preparar narração.");
    report("Gerando voz local", 10);
    QProcess speech;
    speech.setProcessChannelMode(QProcess::MergedChannels);
    QStringList speechArgs{
        "-NoProfile", "-NonInteractive", "-ExecutionPolicy", "Bypass",         "-File", speechPath,
        "-InputFile", request,           "-OutputDirectory", output + "/media"};
    if (parser.isSet("cancel-file"))
        speechArgs << "-CancelFile" << parser.value("cancel-file");
    speech.start("powershell.exe", speechArgs);
    if (!speech.waitForStarted(10000))
        return fail("Não foi possível iniciar a voz do Windows.");
    if (!speech.waitForFinished(300000)) {
        speech.kill();
        speech.waitForFinished(5000);
        return fail("Tempo de geração de voz excedido.");
    }
    if (speech.exitCode() != 0 || speech.exitStatus() != QProcess::NormalExit)
        return fail("Falha na narração local: " + QString::fromLocal8Bit(speech.readAll()));
    QFile manifestFile(output + "/media/speech-manifest.json");
    if (!manifestFile.open(QIODevice::ReadOnly))
        return fail("Manifesto de voz ausente.");
    auto manifest = QJsonDocument::fromJson(manifestFile.readAll()).object();
    QMap<QString, studio::PcmAudio> utterances;
    QMap<QString, QString> wavPaths;
    QMap<QString, std::int64_t> measuredSamples;
    studio::WordTimingMap wordTimings;
    for (auto value : manifest["utterances"].toArray()) {
        auto line = value.toObject();
        auto audio = studio::readWave(line["path"].toString());
        if (!audio.error.isEmpty())
            return fail(audio.error);
        utterances.insert(line["id"].toString(), std::move(audio.audio));
        wavPaths.insert(line["id"].toString(), line["path"].toString());
        measuredSamples.insert(line["id"].toString(),
                               std::int64_t(utterances[line["id"].toString()].samples.size()));
        if (!line["words"].isArray() || line["words"].toArray().isEmpty())
            return fail("A voz pt-BR não forneceu timestamps de palavra para " + line["id"].toString());
        std::vector<std::int64_t> starts;
        std::int64_t previousSample = -1;
        for (const auto wordValue : line["words"].toArray()) {
            const auto word = wordValue.toObject();
            const auto sample = word["audioPositionSamples"].toDouble(-1);
            if (!word["text"].isString() || !word["characterPosition"].isDouble() ||
                !word["characterCount"].isDouble() || !std::isfinite(sample) || sample < 0 ||
                sample > utterances[line["id"].toString()].samples.size() * 2.0 || sample < previousSample)
                return fail("Alinhamento de palavra inválido na fala " + line["id"].toString());
            previousSample = std::llround(sample);
            starts.push_back(previousSample);
        }
        if (!starts.empty()) {
            const auto audioSamples = std::int64_t(utterances[line["id"].toString()].samples.size());
            const auto targetLastStart = std::max<std::int64_t>(
                0, audioSamples - std::min<std::int64_t>(studio::ProductionSampleRate / 3,
                                                         audioSamples / std::int64_t(starts.size() * 2)));
            const double clockCorrection = starts.back() > 0
                                               ? double(targetLastStart) / double(starts.back())
                                               : 1.0;
            for (auto &sample : starts)
                sample = std::clamp<std::int64_t>(
                    std::llround(double(sample) * clockCorrection), 0, audioSamples - 1);
            wordTimings.insert(line["id"].toString(), std::move(starts));
        }
    }
    if (!parser.value("cancel-file").isEmpty() && QFile::exists(parser.value("cancel-file")))
        return fail("Produção cancelada; projeto anterior preservado.");
    const auto plan = studio::planNarrationTiming(parsed.recipe, utterances);
    if (!plan.error.isEmpty())
        return fail(plan.error);
    for (const auto &warning : plan.warnings)
        report("Aviso de ritmo: " + warning, 65);
    if (parsed.recipe.timingMode == "fit-to-duration") {
        report("Conferindo duração e adaptando velocidade sem alterar a altura da voz", 65);
        const auto ffmpeg = qEnvironmentVariable("STUDIO_FFMPEG", "ffmpeg");
        int index = 0;
        for (const auto &scene : plan.scenes)
            for (const auto &cue : scene.cues) {
                if (!parser.value("cancel-file").isEmpty() &&
                    QFile::exists(parser.value("cancel-file")))
                    return fail("Produção cancelada; projeto anterior preservado.");
                auto &samples = utterances[cue.id].samples;
                const auto cap = std::int64_t(cue.speechFrames) * studio::ProductionSampleRate / 30;
                if (std::int64_t(samples.size()) <= cap)
                    continue;
                const double tempo = double(samples.size()) / cap;
                if (tempo > 1.15 + 1e-9)
                    return fail("Aceleração acima do limite de 15% na fala " + cue.id);
                const auto adjustedPath = temp.filePath(QString("retimed-%1.wav").arg(index++));
                QProcess process;
                process.start(ffmpeg,
                              {"-nostdin", "-y", "-hide_banner", "-loglevel", "error",
                               "-i", wavPaths.value(cue.id), "-af",
                               QString("atempo=%1").arg(tempo, 0, 'f', 9),
                               "-ac", "1", "-ar", "24000", "-c:a", "pcm_s16le", adjustedPath});
                if (!process.waitForStarted(10000))
                    return fail("FFmpeg indisponível para ajuste de fala: " + process.errorString());
                if (!process.waitForFinished(30000)) {
                    process.kill();
                    process.waitForFinished(5000);
                    return fail("FFmpeg excedeu o tempo no ajuste da fala " + cue.id);
                }
                if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0)
                    return fail("FFmpeg falhou ao ajustar a fala " + cue.id + ": " +
                                QString::fromLocal8Bit(process.readAllStandardError()).left(1000));
                auto adjusted = studio::readWave(adjustedPath);
                if (!adjusted.error.isEmpty())
                    return fail("Falha ao ler fala ajustada: " + adjusted.error);
                // atempo rounds individual packets; clamp to the exact frame budget.
                if (adjusted.audio.samples.empty() ||
                    std::abs(double(adjusted.audio.samples.size()) - cap) > cap * .08)
                    return fail("FFmpeg produziu duração inesperada para a fala " + cue.id);
                adjusted.audio.samples.resize(std::size_t(cap), 0);
                if (wordTimings.contains(cue.id))
                    for (auto &sample : wordTimings[cue.id])
                        sample = std::llround(double(sample) / tempo);
                samples = std::move(adjusted.audio.samples);
            }
    }
    report("Montando cenas e sincronizando", 75);
    auto compiled = studio::compileProduction(
        parsed.recipe, utterances, output + "/media/master.wav",
        parsed.recipe.timingMode == "fit-to-duration" ? &plan : nullptr, &wordTimings,
        output + "/media/narration.wav");
    if (!compiled.error.isEmpty())
        return fail(compiled.error);
    for (auto pair : {std::pair{output + "/media/narration.wav", &compiled.voice},
                      std::pair{output + "/media/master.wav", &compiled.mix}}) {
        auto error = studio::writeWave(pair.first, *pair.second);
        if (!error.isEmpty())
            return fail(error);
    }
    auto saved = studio::saveProject(output + "/project.studio.json", compiled.project, {});
    if (!saved.error.isEmpty())
        return fail(saved.error);
    if (!write(output + "/subtitles.srt", compiled.subtitles.toUtf8()) ||
        !write(output + "/recipe.production.json", bytes) ||
        !write(output + "/AI-GUIDE.md",
               studio::aiGuide(workspace ? workspace->data() : QJsonObject{}).toUtf8()))
        return fail("Falha ao registrar artefatos de produção.");
    QJsonArray plannedCues;
    if (parsed.recipe.timingMode == "fit-to-duration") {
        std::int64_t sceneFrame = 0;
        for (const auto &scene : plan.scenes) {
            QJsonArray cues;
            std::int64_t cursor = sceneFrame;
            for (const auto &cue : scene.cues) {
                const double speed = std::max(1., double(measuredSamples.value(cue.id)) /
                                                 (cue.speechFrames * (studio::ProductionSampleRate / 30)));
                cues.append(QJsonObject{{"id", cue.id},
                                        {"startFrame", double(cursor)},
                                        {"speechBudgetFrames", cue.speechFrames},
                                        {"slotFrames", cue.slotFrames},
                                        {"voiceSpeed", speed}});
                cursor += cue.slotFrames;
            }
            plannedCues.append(QJsonObject{{"startFrame", double(sceneFrame)},
                                           {"durationFrames", scene.durationFrames},
                                           {"cues", cues}});
            sceneFrame += scene.durationFrames;
        }
    }
    QJsonArray timings;
    for (const auto &s : compiled.project.scenes)
        timings.append(QJsonObject{{"id", QString::fromStdString(s.id)},
                                   {"startFrame", double(s.start)},
                                   {"durationFrames", double(s.duration)}});
    write(output + "/production-report.json",
          QJsonDocument(QJsonObject{{"status", "ready"},
                                    {"voice", manifest["voice"]},
                                    {"durationSeconds", double(compiled.project.duration) / 30},
                                    {"frameCount", double(compiled.project.duration)},
                                    {"nodes", int(compiled.project.nodes.size())},
                                    {"recipeVersion", parsed.recipe.version},
                                    {"timingMode", parsed.recipe.timingMode},
                                    {"targetDurationSeconds", parsed.recipe.targetDurationSeconds},
                                    {"maxVoiceAcceleration", 1.15},
                                    {"timingWarnings", QJsonArray::fromStringList(plan.warnings)},
                                    {"timingPlan", plannedCues},
                                    {"preserveVoicePitch", parsed.recipe.preserveVoicePitch},
                                    {"scenes", timings},
                                    {"alignment", parsed.recipe.timingMode == "fit-to-duration"
                                                       ? "measured-speech-and-frame-exact-slots"
                                                       : "measured-utterance-audio"},
                                    {"wordAlignment", "windows-speech-progress-events"},
                                    {"audio", "voice + procedural music + scene chime"}})
              .toJson());
    report("Pronto", 100);
    return 0;
}
