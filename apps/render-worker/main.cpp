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
#include <algorithm>
#include <cmath>
#include <vector>
#include <studio/composition/painter.hpp>
#include <studio/media/wave.hpp>
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
    for (auto key : {"project", "output", "ffmpeg", "cancel-file", "still", "still-sequence"})
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
    if (parser.isSet("still-sequence")) {
        const auto frameValues = parser.value("still-sequence").split(',', Qt::SkipEmptyParts);
        if (frameValues.isEmpty())
            return fail("A sequência precisa de pelo menos um quadro.");
        const auto directory = parser.value("output");
        if (directory.isEmpty() || !QDir().mkpath(directory))
            return fail("Escolha uma pasta válida para os quadros da sequência.");
        int index = 0;
        for (const auto &value : frameValues) {
            bool ok = false;
            const auto frame = value.toLongLong(&ok);
            if (!ok || frame < 0 || frame >= loaded.project.duration)
                return fail("A sequência contém um quadro fora do projeto.");
            const auto path = QDir(directory).filePath(QString("frame-%1-%2.png").arg(index++).arg(frame));
            if (!painter.frame(loaded.project, frame).save(path))
                return fail("Falha ao salvar quadro da sequência.");
        }
        const auto stats = painter.videoCacheStats();
        QTextStream stream(stdout);
        stream.setEncoding(QStringConverter::Utf8);
        stream << QJsonDocument(QJsonObject{{"progress", 100},
                                            {"videoCache", QJsonObject{{"hits", qint64(stats.hits)},
                                                                        {"misses", qint64(stats.misses)},
                                                                        {"decodedFrames", qint64(stats.decodedFrames)},
                                                                        {"seekStarts", qint64(stats.seekStarts)}}}})
                      .toJson(QJsonDocument::Compact)
               << Qt::endl;
        return 0;
    }
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
    const auto sidechainPath = !p.voiceDuckingSource.empty() ? QString::fromStdString(p.voiceDuckingSource)
                                                              : QString::fromStdString(p.audioSource);
    const auto sidechain = needsDucking && !sidechainPath.isEmpty() ? studio::readWave(sidechainPath)
                                                                    : studio::WaveResult{};
    const bool hasDuckingEnvelope = needsDucking && sidechain.error.isEmpty();
    const bool separateVoiceStem = needsDucking && !hasDuckingEnvelope && !p.voiceDuckingSource.empty() &&
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
        std::vector<int> duckEnvelopeInputs(activeTracks.size(), -1);
        if (hasDuckingEnvelope) {
            std::vector<float> duckLevels(std::size_t(p.duration), 0.f);
            double smoothed = 0;
            for (std::int64_t frame = 0; frame < p.duration; ++frame) {
                const auto begin = std::min<std::size_t>(sidechain.audio.samples.size(),
                    std::size_t(qRound64(double(frame) * studio::ProductionSampleRate / p.fps.value())));
                const auto end = std::min<std::size_t>(sidechain.audio.samples.size(),
                    std::size_t(qRound64(double(frame + 1) * studio::ProductionSampleRate / p.fps.value())));
                double sum = 0;
                for (auto at = begin; at < end; ++at)
                    sum += double(sidechain.audio.samples[at]) * sidechain.audio.samples[at];
                const auto rms = end > begin ? std::sqrt(sum / double(end - begin)) : 0.;
                const auto target = std::clamp((rms - .012) / .09, 0., 1.);
                smoothed += (target - smoothed) * (target > smoothed ? .65 : .18);
                duckLevels[std::size_t(frame)] = float(smoothed);
            }
            for (std::size_t i = 0; i < activeTracks.size(); ++i) {
                const auto &track = *activeTracks[i];
                if (!track.duckUnderVoice)
                    continue;
                studio::PcmAudio envelope;
                const auto sampleCount = qRound64(sequenceSeconds * studio::ProductionSampleRate);
                envelope.samples.resize(std::size_t(sampleCount), 1.f);
                for (std::int64_t frame = 0; frame < p.duration; ++frame) {
                    const auto gain = float(1. - track.duckAmount * duckLevels[std::size_t(frame)]);
                    const auto begin = std::min<std::size_t>(envelope.samples.size(),
                        std::size_t(qRound64(double(frame) * studio::ProductionSampleRate / p.fps.value())));
                    const auto end = std::min<std::size_t>(envelope.samples.size(),
                        std::size_t(qRound64(double(frame + 1) * studio::ProductionSampleRate / p.fps.value())));
                    std::fill(envelope.samples.begin() + std::ptrdiff_t(begin),
                              envelope.samples.begin() + std::ptrdiff_t(end), gain);
                }
                const auto envelopePath = temp.filePath(QString("duck-%1.wav").arg(i));
                if (const auto error = studio::writeWave(envelopePath, envelope); !error.isEmpty())
                    return fail(error);
                duckEnvelopeInputs[i] = nextInput;
                encoderArgs << "-i" << envelopePath;
                ++nextInput;
            }
        }
        const auto gainExpression = [&](const studio::AudioTrack &track) {
            const auto &points = track.gainAutomation;
            QString expression = seconds(points.back().value);
            for (std::size_t i = points.size(); i-- > 1;) {
                const auto &from = points[i - 1];
                const auto &to = points[i];
                const auto start = seconds(double(from.frame) / p.fps.value());
                const auto end = seconds(double(to.frame) / p.fps.value());
                const auto raw = QString("min(1,max(0,(t-%1)/%2))").arg(start, seconds((to.frame - from.frame) / p.fps.value()));
                QString progress = raw;
                if (to.easing == "ease-in")
                    progress = "pow(" + raw + ",3)";
                else if (to.easing == "ease-out")
                    progress = "1-pow(1-" + raw + ",3)";
                else if (to.easing == "ease-in-out")
                    progress = "(" + raw + ")*" + raw + "*(3-2*(" + raw + "))";
                const auto ramp = seconds(from.value) + "+(" + seconds(to.value - from.value) + ")*(" + progress + ")";
                expression = "if(lt(t," + end + ")," + ramp + "," + expression + ")";
            }
            const auto firstTime = seconds(double(points.front().frame) / p.fps.value());
            return "if(lt(t," + firstTime + ")," + seconds(points.front().value) + "," + expression + ")";
        };
        QStringList filters, inputs;
        if (hasMainAudio) {
            filters << "[1:a:0]atrim=duration=" + seconds(sequenceSeconds) + ",asetpts=PTS-STARTPTS[main]";
            inputs << "[main]";
        }
        if (needsDucking && !hasDuckingEnvelope && voiceInput >= 0)
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
            QString trackFilter = "[" + QString::number(input) + ":a:0]atrim=start=" +
                                  seconds(track.sourceInSeconds) + ":duration=" +
                                  seconds(length * track.playbackRate) + ",asetpts=PTS-STARTPTS";
            if (std::abs(track.playbackRate - 1.) > 1e-9) {
                double remaining = track.playbackRate;
                while (remaining < .5) {
                    trackFilter += ",atempo=0.5";
                    remaining /= .5;
                }
                while (remaining > 2.) {
                    trackFilter += ",atempo=2";
                    remaining /= 2.;
                }
                trackFilter += ",atempo=" + QString::number(remaining, 'f', 9);
            }
            trackFilter += ",apad,atrim=duration=" + seconds(length) + ",asetpts=PTS-STARTPTS";
            if (track.fadeInFrames > 0)
                trackFilter += ",afade=t=in:st=0:d=" + seconds(double(track.fadeInFrames) / p.fps.value());
            if (track.fadeOutFrames > 0)
                trackFilter += ",afade=t=out:st=" +
                               seconds(length - double(track.fadeOutFrames) / p.fps.value()) +
                               ":d=" + seconds(double(track.fadeOutFrames) / p.fps.value());
            trackFilter += ",volume=" + gain;
            if (!track.gainAutomation.empty())
                trackFilter += ",volume='" + gainExpression(track) + "':eval=frame";
            trackFilter += ",adelay=delays=" + QString::number(delay) + "|" + QString::number(delay) + ":all=1";
            trackFilter += "[" + pre + "]";
            filters << trackFilter;
            if (duckEnvelopeInputs[i] >= 0) {
                filters << QString("[%1][%2:a:0]amultiply[%3]")
                               .arg(pre).arg(duckEnvelopeInputs[i]).arg(out);
            } else if (track.duckUnderVoice && needsDucking && voiceInput >= 0) {
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
