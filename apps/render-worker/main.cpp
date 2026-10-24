#include <QCommandLineParser>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QImageReader>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QTemporaryDir>
#include <QTextStream>
#include <studio/composition/painter.hpp>
#include <studio/storage/project_store.hpp>

static void report(int progress, QString error = {}) {
    QTextStream stream(stdout);
    stream.setEncoding(QStringConverter::Utf8);
    stream
        << QJsonDocument(QJsonObject{{"progress", progress}, {"error", error}}).toJson(QJsonDocument::Compact)
        << Qt::endl;
}
int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    QCommandLineParser parser;
    parser.addHelpOption();
    for (auto key : {"project", "output", "ffmpeg", "cancel-file", "still"})
        parser.addOption({key, key, key});
    parser.process(app);
    auto loaded = studio::loadProject(parser.value("project"));
    auto fail = [](QString message) {
        report(0, message);
        return 1;
    };
    if (!loaded.error.isEmpty())
        return fail(loaded.error);
    studio::CompositionPainter painter;
    if (parser.isSet("still")) {
        bool ok;
        int frame = parser.value("still").toInt(&ok);
        if (!ok || frame < 0 || frame >= loaded.project.duration)
            return fail("Frame inválido.");
        return painter.frame(loaded.project, frame).save(parser.value("output"))
                   ? 0
                   : fail("Falha ao salvar PNG.");
    }
    const auto output = parser.value("output");
    if (output.isEmpty() || QFile::exists(output))
        return fail("Escolha um nome de arquivo novo para a exportação.");
    for (const auto &n : loaded.project.nodes)
        if (n.kind == studio::NodeKind::Image) {
            QImageReader reader(QString::fromStdString(n.source));
            if (!reader.canRead())
                return fail("Imagem ausente ou ilegível: " + QString::fromStdString(n.source));
        }
    QTemporaryDir temp(QFileInfo(output).absolutePath() + "/.studio-XXXXXX");
    if (!temp.isValid())
        return fail("Não foi possível criar a pasta temporária.");
    auto tempPath = temp.filePath("render.mp4");
    const auto &p = loaded.project;
    QProcess encoder;
    encoder.setProcessChannelMode(QProcess::SeparateChannels);
    encoder.start(parser.value("ffmpeg"),
                  {"-y",         "-hide_banner",
                   "-loglevel",  "error",
                   "-f",         "rawvideo",
                   "-pix_fmt",   "rgba",
                   "-s",         QString::number(p.width) + "x" + QString::number(p.height),
                   "-r",         QString::number(p.fps.numerator) + "/" + QString::number(p.fps.denominator),
                   "-i",         "pipe:0",
                   "-an",        "-c:v",
                   "libx264",    "-preset",
                   "veryfast",   "-crf",
                   "20",         "-pix_fmt",
                   "yuv420p",    "-movflags",
                   "+faststart", tempPath});
    if (!encoder.waitForStarted(10000))
        return fail("Não foi possível iniciar o FFmpeg: " + encoder.errorString());
    int last = -1;
    for (std::int64_t f = 0; f < p.duration; ++f) {
        if (!parser.value("cancel-file").isEmpty() && QFile::exists(parser.value("cancel-file"))) {
            encoder.kill();
            encoder.waitForFinished(5000);
            report(0, "Exportação cancelada; projeto preservado.");
            return 130;
        }
        auto image = painter.frame(p, f);
        const auto bytes = image.sizeInBytes();
        if (encoder.write(reinterpret_cast<const char *>(image.constBits()), bytes) != bytes) {
            encoder.kill();
            encoder.waitForFinished(5000);
            return fail("Falha ao enviar frame ao codificador.");
        }
        while (encoder.bytesToWrite() > 0) {
            if (!encoder.waitForBytesWritten(30000)) {
                auto detail = encoder.readAllStandardError();
                encoder.kill();
                encoder.waitForFinished(5000);
                return fail("Codificador interrompido: " + QString::fromUtf8(detail));
            }
        }
        int progress = int((f + 1) * 99 / p.duration);
        if (progress != last) {
            report(progress);
            last = progress;
        }
    }
    encoder.closeWriteChannel();
    if (!encoder.waitForFinished(120000)) {
        encoder.kill();
        encoder.waitForFinished(5000);
        return fail("Tempo de codificação excedido.");
    }
    if (encoder.exitStatus() != QProcess::NormalExit || encoder.exitCode() != 0)
        return fail(QString::fromUtf8(encoder.readAllStandardError()));
    if (!QFile::rename(tempPath, output))
        return fail("O MP4 foi codificado, mas não pôde ser movido ao destino.");
    report(100);
    return 0;
}
