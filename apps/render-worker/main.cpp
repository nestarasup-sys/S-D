#include <QCommandLineParser>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QImageReader>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QSvgRenderer>
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
    if (parser.isSet("ffmpeg"))
        qputenv("STUDIO_FFMPEG", parser.value("ffmpeg").toUtf8());
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
    for (const auto &n : loaded.project.nodes)
        if (n.kind == studio::NodeKind::Svg) {
            QSvgRenderer svg(QString::fromStdString(n.source));
            if (!svg.isValid())
                return fail("SVG ausente ou inválido: " + QString::fromStdString(n.source));
        }
    for (const auto &n : loaded.project.nodes)
        if (n.kind == studio::NodeKind::Video && !QFile::exists(QString::fromStdString(n.source)))
            return fail("Arquivo de vídeo ausente: " + QString::fromStdString(n.source));
    if (!loaded.project.audioSource.empty() &&
        !QFile::exists(QString::fromStdString(loaded.project.audioSource)))
        return fail("Arquivo de áudio ausente.");
    for (const auto &track : loaded.project.audioTracks)
        if (!track.muted && !QFile::exists(QString::fromStdString(track.source)))
            return fail("Arquivo ausente na pista de áudio '" + QString::fromStdString(track.name) + "'.");
    const bool hasUnmutedDuckingTrack = std::any_of(
        loaded.project.audioTracks.begin(), loaded.project.audioTracks.end(),
        [](const auto &track) { return !track.muted && track.duckUnderVoice; });
    if (hasUnmutedDuckingTrack && loaded.project.audioSource.empty() &&
        loaded.project.voiceDuckingSource.empty())
        return fail("Ducking exige uma faixa de voz de referência.");
    if (hasUnmutedDuckingTrack && !loaded.project.voiceDuckingSource.empty() &&
        !QFile::exists(QString::fromStdString(loaded.project.voiceDuckingSource)))
        return fail("Arquivo de voz usado para ducking está ausente.");
    QTemporaryDir temp(QFileInfo(output).absolutePath() + "/.studio-XXXXXX");
    if (!temp.isValid())
        return fail("Não foi possível criar a pasta temporária.");
    auto tempPath = temp.filePath("render.mp4");
    const auto &p = loaded.project;
    QProcess encoder;
    encoder.setProcessChannelMode(QProcess::SeparateChannels);
    QStringList encoderArgs{
        "-y",        "-hide_banner",
        "-loglevel", "error",
        "-f",        "rawvideo",
        "-pix_fmt",  "rgba",
        "-s",        QString::number(p.width) + "x" + QString::number(p.height),
        "-r",        QString::number(p.fps.numerator) + "/" + QString::number(p.fps.denominator),
        "-i",        "pipe:0"};
    const bool hasMainAudio = !p.audioSource.empty();
    if (hasMainAudio)
        encoderArgs << "-i" << QString::fromStdString(p.audioSource);
    std::vector<const studio::AudioTrack *> activeTracks;
    for (const auto &track : p.audioTracks)
        if (!track.muted)
            activeTracks.push_back(&track);
    const bool needsDucking = std::any_of(activeTracks.begin(), activeTracks.end(),
                                          [](const auto *track) { return track->duckUnderVoice; });
    const bool separateVoiceStem = needsDucking && !p.voiceDuckingSource.empty() &&
                                   p.voiceDuckingSource != p.audioSource;
    if (separateVoiceStem)
        encoderArgs << "-i" << QString::fromStdString(p.voiceDuckingSource);
    const int voiceInput = separateVoiceStem ? (hasMainAudio ? 2 : 1) : (hasMainAudio ? 1 : -1);
    int nextInput = 1 + int(hasMainAudio) + int(separateVoiceStem);
    for (const auto *track : activeTracks)
        encoderArgs << "-i" << QString::fromStdString(track->source);
    if (hasMainAudio || !activeTracks.empty()) {
        const double sequenceSeconds = double(p.duration) / p.fps.value();
        const auto seconds = [](double value) { return QString::number(value, 'f', 6); };
        QStringList filters, inputs;
        const bool needsSidechain = needsDucking && voiceInput >= 0;
        if (hasMainAudio) {
            filters << "[1:a:0]atrim=duration=" + seconds(sequenceSeconds) + ",asetpts=PTS-STARTPTS[main]";
            inputs << "[main]";
        }
        if (needsSidechain)
            filters << QString("[%1:a:0]apad,atrim=duration=%2,asetpts=PTS-STARTPTS[sidechain]")
                           .arg(voiceInput).arg(seconds(sequenceSeconds));
        for (std::size_t i = 0; i < activeTracks.size(); ++i) {
            const auto &track = *activeTracks[i];
            const int input = nextInput++;
            const auto delay = qRound64(double(track.start) * 1000. / p.fps.value());
            const auto length = double(track.duration) / p.fps.value();
            const auto gain = QString::number(track.gain, 'f', 6);
            const auto pre = QString("pre%1").arg(i);
            const auto out = QString("track%1").arg(i);
            filters << QString("[%1:a:0]atrim=duration=%2,asetpts=PTS-STARTPTS,volume=%3,adelay=delays=%4|%4:all=1[%5]")
                           .arg(input).arg(seconds(length), gain).arg(delay).arg(pre);
            if (track.duckUnderVoice && needsSidechain) {
                const auto ratio = QString::number(1.0 + track.duckAmount * 19.0, 'f', 4);
                filters << QString("[%1][sidechain]sidechaincompress=threshold=0.025:ratio=%2:attack=20:release=280[%3]")
                               .arg(pre, ratio, out);
            } else
                filters << QString("[%1]anull[%2]").arg(pre, out);
            inputs << "[" + out + "]";
        }
        filters << inputs.join("") + QString("amix=inputs=%1:duration=longest:normalize=0,alimiter=limit=0.98[aout]")
                                            .arg(inputs.size());
        encoderArgs << "-filter_complex" << filters.join(";") << "-map" << "0:v:0" << "-map" << "[aout]"
                    << "-c:a" << "aac" << "-b:a" << "160k";
    } else
        encoderArgs << "-an";
    encoderArgs << QStringList{
        "-c:v",      "libx264",    "-preset",  "veryfast",
        "-crf",      "20",         "-pix_fmt", "yuv420p",
        "-movflags", "+faststart", "-t",       QString::number(double(p.duration) / p.fps.value(), 'f', 6),
        tempPath};
    encoder.start(parser.value("ffmpeg"), encoderArgs);
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
