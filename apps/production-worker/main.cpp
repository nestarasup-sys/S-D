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
    for (auto value : manifest["utterances"].toArray()) {
        auto line = value.toObject();
        auto audio = studio::readWave(line["path"].toString());
        if (!audio.error.isEmpty())
            return fail(audio.error);
        utterances.insert(line["id"].toString(), std::move(audio.audio));
    }
    if (!parser.value("cancel-file").isEmpty() && QFile::exists(parser.value("cancel-file")))
        return fail("Produção cancelada; projeto anterior preservado.");
    report("Montando cenas e sincronizando", 75);
    auto compiled = studio::compileProduction(parsed.recipe, utterances, output + "/media/master.wav");
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
                                    {"scenes", timings},
                                    {"alignment", "measured-utterance-audio"},
                                    {"audio", "voice + procedural music + scene chime"}})
              .toJson());
    report("Pronto", 100);
    return 0;
}
