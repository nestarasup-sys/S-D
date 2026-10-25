#include <QFileInfo>
#include <QFont>
#include <QImageReader>
#include <QLinearGradient>
#include <QProcess>
#include <QProcessEnvironment>
#include <QSvgRenderer>
#include <QFontMetricsF>
#include <QRegularExpression>
#include <algorithm>
#include <functional>
#include <optional>
#include <QCoreApplication>
#include <QDir>
#include <studio/composition/native_painter.hpp>
#include <studio/composition/painter.hpp>
#include <vector>

namespace studio {
namespace {
enum class NodePass { All, ShotComposition, Captions };

void paintBackground(QPainter &painter, const Project &project, bool black = false) {
    if (black) {
        painter.fillRect(QRect(0, 0, project.width, project.height), Qt::black);
        return;
    }
    const auto background = QColor(QString::fromStdString(project.artDirection.backgroundColor));
    const auto glowColor = QColor(QString::fromStdString(project.artDirection.glowColor));
    painter.fillRect(QRect(0, 0, project.width, project.height), background);
    QRadialGradient glow(project.width * .82, project.height * .2, project.width * .8);
    glow.setColorAt(0, glowColor);
    glow.setColorAt(1, background);
    painter.fillRect(QRect(0, 0, project.width, project.height), glow);
}

void paintNodes(QPainter &painter, const Project &project, std::int64_t frame,
                const std::optional<std::string> &shotId, NodePass pass, QCache<QString, QImage> &images,
                const std::function<QImage(const Node &, std::int64_t)> &videoFrame) {
    const auto camera = evaluateCamera(project, frame);
    painter.save();
    painter.translate(project.width / 2.0, project.height / 2.0);
    painter.scale(camera.zoom, camera.zoom);
    painter.rotate(camera.rotation);
    painter.translate(-camera.x, -camera.y);
    for (const auto &node : project.nodes) {
        if (pass == NodePass::Captions && (node.role != "caption" || !node.shotId.empty()))
            continue;
        if (pass == NodePass::ShotComposition) {
            const bool belongsToShot = shotId && node.shotId == *shotId;
            const bool isShared = node.shotId.empty() && node.role != "caption";
            if (!belongsToShot && !isShared)
                continue;
        }
        std::vector<const Node *> chain{&node};
        auto parentId = node.parentId;
        while (!parentId.empty()) {
            const auto parent = std::find_if(project.nodes.begin(), project.nodes.end(),
                                             [&](const Node &candidate) { return candidate.id == parentId; });
            if (parent == project.nodes.end())
                break;
            chain.push_back(&*parent);
            parentId = parent->parentId;
        }
        std::reverse(chain.begin(), chain.end());
        double opacity = 1;
        bool visible = true;
        for (const auto *part : chain) {
            const auto state = evaluate(*part, frame);
            if (state.opacity <= 0 || !active(*part, frame)) {
                visible = false;
                break;
            }
            opacity *= state.opacity;
        }
        if (!visible || node.kind == NodeKind::Group)
            continue;
        painter.save();
        painter.setOpacity(opacity);
        for (const auto *part : chain) {
            const auto state = evaluate(*part, frame);
            painter.translate(state.x, state.y);
            painter.translate(part->width / 2, part->height / 2);
            painter.rotate(state.rotation);
            painter.scale(state.scale, state.scale);
            painter.translate(-part->width / 2, -part->height / 2);
        }
        const QRectF bounds(0, 0, node.width, node.height);
        if (node.kind == NodeKind::Shape) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(QString::fromStdString(node.color)));
            painter.drawRoundedRect(bounds, std::min(12., node.height / 2), std::min(12., node.height / 2));
        } else if (node.kind == NodeKind::Native) {
            paintNative(painter, node, frame);
        } else if (node.kind == NodeKind::Svg) {
            QSvgRenderer svg(QString::fromStdString(node.source));
            if (svg.isValid())
                svg.render(&painter, bounds);
        } else if (node.kind == NodeKind::Video) {
            const auto image = videoFrame(node, frame);
            if (!image.isNull()) {
                QSizeF size = image.size();
                size.scale(bounds.size(), Qt::KeepAspectRatioByExpanding);
                const QRectF target((node.width - size.width()) / 2.0, (node.height - size.height()) / 2.0,
                                    size.width(), size.height());
                painter.save();
                painter.setClipRect(bounds);
                painter.drawImage(target, image);
                painter.restore();
            } else {
                painter.fillRect(bounds, QColor("#18232b"));
                painter.setPen(Qt::white);
                painter.drawText(bounds, Qt::AlignCenter, "Vídeo indisponível\n" +
                                                            QFileInfo(QString::fromStdString(node.source)).fileName());
            }
        } else if (node.kind == NodeKind::Text) {
            const QRectF textBounds = node.role == "caption" ? bounds.adjusted(20, 8, -20, -8) : bounds;
            if (node.role == "caption") {
                painter.setPen(Qt::NoPen);
                auto captionBackground = QColor(QString::fromStdString(project.artDirection.captionBackgroundColor));
                captionBackground.setAlpha(225);
                painter.setBrush(captionBackground);
                painter.drawRoundedRect(bounds, 12, 12);
            }
            QFont font(QString::fromStdString(project.artDirection.fontFamily));
            font.setPixelSize(qRound(node.fontSize));
            font.setWeight(QFont::DemiBold);
            painter.setFont(font);
            if (node.role == "caption" && !node.captionWords.empty()) {
                struct CaptionPart {
                    QString text;
                    QColor color;
                    qreal width;
                };
                struct CaptionLine {
                    std::vector<CaptionPart> parts;
                    qreal width{0};
                };
                const QFontMetricsF metrics(font);
                const auto baseColor = QColor(QString::fromStdString(node.color));
                const auto highlightColor = QColor(QString::fromStdString(project.artDirection.primaryColor));
                const auto words = QString::fromStdString(node.text).split(QRegularExpression("\\s+"),
                                                                           Qt::SkipEmptyParts);
                std::vector<CaptionLine> lines(1);
                std::size_t cueIndex = 0;
                const qreal wordGap = metrics.horizontalAdvance(QLatin1Char(' '));
                for (const auto &word : words) {
                    const bool highlighted = cueIndex < node.captionWords.size() &&
                                             frame >= node.captionWords[cueIndex].start &&
                                             frame < node.captionWords[cueIndex].end;
                    const qreal width = metrics.horizontalAdvance(word);
                    auto *line = &lines.back();
                    const qreal addedWidth = width + (line->parts.empty() ? 0 : wordGap);
                    if (!line->parts.empty() && line->width + addedWidth > textBounds.width()) {
                        lines.emplace_back();
                        line = &lines.back();
                    }
                    if (!line->parts.empty())
                        line->width += wordGap;
                    line->parts.push_back({word, highlighted ? highlightColor : baseColor, width});
                    line->width += width;
                    if (cueIndex < node.captionWords.size())
                        ++cueIndex;
                }
                const qreal lineHeight = metrics.height();
                const qreal totalHeight = lineHeight * lines.size();
                qreal baseline = textBounds.y() + std::max<qreal>(metrics.ascent(),
                                                                  (textBounds.height() - totalHeight) / 2 +
                                                                      metrics.ascent());
                painter.save();
                for (const auto &line : lines) {
                    qreal x = textBounds.x() + (textBounds.width() - line.width) / 2;
                    for (std::size_t part = 0; part < line.parts.size(); ++part) {
                        if (part > 0)
                            x += wordGap;
                        painter.setPen(line.parts[part].color);
                        painter.drawText(QPointF(x, baseline), line.parts[part].text);
                        x += line.parts[part].width;
                    }
                    baseline += lineHeight;
                }
                painter.restore();
            } else {
                painter.setPen(QColor(QString::fromStdString(node.color)));
                painter.drawText(textBounds, (node.role == "caption" ? Qt::AlignCenter : Qt::AlignLeft | Qt::AlignTop) |
                                                  Qt::TextWordWrap,
                                 QString::fromStdString(node.text));
            }
        } else {
            const auto path = QString::fromStdString(node.source);
            if (!images.contains(path)) {
                QImageReader reader(path);
                reader.setAutoTransform(true);
                auto size = reader.size();
                if (size.width() > project.width || size.height() > project.height) {
                    size.scale(project.width, project.height, Qt::KeepAspectRatio);
                    reader.setScaledSize(size);
                }
                auto image = new QImage(reader.read());
                const auto cost = std::max<qsizetype>(1, image->sizeInBytes() / 1024);
                images.insert(path, image, int(cost));
            }
            const auto *cached = images.object(path);
            const auto image = cached ? *cached : QImage{};
            if (!image.isNull()) {
                QSizeF size = image.size();
                size.scale(bounds.size(), Qt::KeepAspectRatio);
                painter.drawImage(QRectF((node.width - size.width()) / 2, (node.height - size.height()) / 2,
                                         size.width(), size.height()),
                                  image);
            } else {
                painter.fillRect(bounds, QColor("#452832"));
                painter.setPen(Qt::white);
                painter.drawText(bounds, Qt::AlignCenter, "Imagem ausente\n" + QFileInfo(path).fileName());
            }
        }
        painter.restore();
    }
    painter.restore();
}

QImage shotLayer(const Project &project, std::int64_t frame, const ShotMarker &shot,
                 QCache<QString, QImage> &images,
                 const std::function<QImage(const Node &, std::int64_t)> &videoFrame) {
    QImage image(project.width, project.height, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    paintBackground(painter, project);
    paintNodes(painter, project, frame, shot.id, NodePass::ShotComposition, images, videoFrame);
    painter.end();
    return image;
}

} // namespace

struct CompositionPainter::VideoDecoder {
    QProcess process;
    QByteArray pending;
    QImage lastImage;
    std::int64_t decodedFrame{-1};
    std::size_t frameBytes{0};
    int width{0}, height{0};
    QString source;
    double sourceInSeconds{0}, playbackRate{1}, cropX{0}, cropY{0}, cropWidth{1}, cropHeight{1};
    bool endOfStream{false};
};

CompositionPainter::CompositionPainter() = default;
CompositionPainter::~CompositionPainter() = default;

void CompositionPainter::clearCache() {
    images_.clear();
    for (auto &[id, decoder] : videoDecoders_) {
        Q_UNUSED(id);
        if (decoder->process.state() != QProcess::NotRunning) {
            decoder->process.kill();
            decoder->process.waitForFinished(1000);
        }
    }
    videoDecoders_.clear();
}

QImage CompositionPainter::videoFrame(const Node &node, const Project &project, std::int64_t frameNumber) {
    const auto localFrame = frameNumber - node.start;
    if (localFrame < 0 || localFrame >= node.duration)
        return {};
    auto &slot = videoDecoders_[node.id];
    if (!slot)
        slot = std::make_unique<VideoDecoder>();
    auto &decoder = *slot;
    const QString source = QString::fromStdString(node.source);
    const int width = std::clamp(qCeil(node.width), 2, 1920);
    const int height = std::clamp(qCeil(node.height), 2, 1080);
    const auto frameBytes = std::size_t(width) * std::size_t(height) * 4;
    const auto stopDecoder = [&] {
        if (decoder.process.state() != QProcess::NotRunning) {
            decoder.process.kill();
            decoder.process.waitForFinished(1000);
        }
        decoder.pending.clear();
        decoder.lastImage = {};
        decoder.decodedFrame = -1;
        decoder.endOfStream = false;
    };
    if (decoder.source != source || decoder.width != width || decoder.height != height ||
        decoder.sourceInSeconds != node.videoSourceInSeconds || decoder.playbackRate != node.videoPlaybackRate ||
        decoder.cropX != node.videoCropX || decoder.cropY != node.videoCropY ||
        decoder.cropWidth != node.videoCropWidth || decoder.cropHeight != node.videoCropHeight ||
        localFrame < decoder.decodedFrame)
        stopDecoder();
    decoder.source = source;
    decoder.width = width;
    decoder.height = height;
    decoder.sourceInSeconds = node.videoSourceInSeconds;
    decoder.playbackRate = node.videoPlaybackRate;
    decoder.cropX = node.videoCropX;
    decoder.cropY = node.videoCropY;
    decoder.cropWidth = node.videoCropWidth;
    decoder.cropHeight = node.videoCropHeight;
    decoder.frameBytes = frameBytes;
    if (decoder.endOfStream)
        return decoder.lastImage;
    if (decoder.process.state() == QProcess::NotRunning) {
        auto ffmpeg = qEnvironmentVariable("STUDIO_FFMPEG");
        if (ffmpeg.isEmpty())
            ffmpeg = "ffmpeg";
        QStringList filters;
        filters << QString("trim=start=%1").arg(QString::number(node.videoSourceInSeconds, 'f', 6));
        filters << QString("setpts=(PTS-STARTPTS)/%1").arg(QString::number(node.videoPlaybackRate, 'f', 6));
        if (node.videoCropX != 0 || node.videoCropY != 0 || node.videoCropWidth != 1 ||
            node.videoCropHeight != 1)
            filters << QString("crop=iw*%1:ih*%2:iw*%3:ih*%4:exact=1")
                           .arg(QString::number(node.videoCropWidth, 'f', 6))
                           .arg(QString::number(node.videoCropHeight, 'f', 6))
                           .arg(QString::number(node.videoCropX, 'f', 6))
                           .arg(QString::number(node.videoCropY, 'f', 6));
        filters << QString("scale=%1:%2:force_original_aspect_ratio=decrease").arg(width).arg(height);
        filters << QString("pad=%1:%2:(ow-iw)/2:(oh-ih)/2").arg(width).arg(height);
        filters << QString("fps=%1/%2").arg(project.fps.numerator).arg(project.fps.denominator);
        decoder.process.setProcessChannelMode(QProcess::SeparateChannels);
        decoder.process.start(ffmpeg,
                              {"-nostdin", "-hide_banner", "-loglevel", "error", "-i", source,
                               "-vf", filters.join(','), "-an", "-sn", "-dn", "-f", "rawvideo",
                               "-pix_fmt", "rgba", "pipe:1"});
        if (!decoder.process.waitForStarted(5000))
            return {};
    }
    while (decoder.decodedFrame < localFrame) {
        while (decoder.pending.size() < qsizetype(frameBytes)) {
            if (decoder.process.bytesAvailable() == 0 && !decoder.process.waitForReadyRead(15000)) {
                if (decoder.process.state() != QProcess::NotRunning)
                    decoder.process.kill();
                decoder.process.waitForFinished(1000);
                decoder.endOfStream = true;
                return decoder.lastImage;
            }
            decoder.pending += decoder.process.readAllStandardOutput();
        }
        QByteArray bytes = decoder.pending.left(qsizetype(frameBytes));
        decoder.pending.remove(0, qsizetype(frameBytes));
        decoder.lastImage = QImage(reinterpret_cast<const uchar *>(bytes.constData()), width, height,
                                   width * 4, QImage::Format_RGBA8888).copy();
        ++decoder.decodedFrame;
    }
    return decoder.lastImage;
}

void CompositionPainter::paint(QPainter &painter, const Project &project, std::int64_t frame) {
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    if (project.shots.empty()) {
        paintBackground(painter, project);
        paintNodes(painter, project, frame, std::nullopt, NodePass::All, images_,
                   [this, &project](const Node &node, std::int64_t number) {
                       return videoFrame(node, project, number);
                   });
        return;
    }

    const ShotMarker *currentShot = nullptr;
    for (const auto &shot : project.shots)
        if (frame >= shot.start && frame < shot.start + shot.duration &&
            (!currentShot || shot.start > currentShot->start))
            currentShot = &shot;
    if (!currentShot) {
        paintBackground(painter, project);
        paintNodes(painter, project, frame, std::nullopt, NodePass::Captions, images_,
                   [this, &project](const Node &node, std::int64_t number) {
                       return videoFrame(node, project, number);
                   });
        return;
    }

    const ShotMarker *previousShot = nullptr;
    if (currentShot->transitionIn != "cut" &&
        frame < currentShot->start + currentShot->transitionDurationFrames) {
        for (const auto &shot : project.shots)
            if (shot.sceneId == currentShot->sceneId && shot.start + shot.duration == currentShot->start)
                previousShot = &shot;
    }

    if (!previousShot || currentShot->transitionIn == "cut") {
        paintBackground(painter, project);
        paintNodes(painter, project, frame, currentShot->id, NodePass::ShotComposition, images_,
                   [this, &project](const Node &node, std::int64_t number) {
                       return videoFrame(node, project, number);
                   });
    } else {
        const auto previousFrame = previousShot->start + previousShot->duration - 1;
        const auto decoder = [this, &project](const Node &node, std::int64_t number) {
            return videoFrame(node, project, number);
        };
        const auto outgoing = shotLayer(project, previousFrame, *previousShot, images_, decoder);
        const auto incoming = shotLayer(project, frame, *currentShot, images_, decoder);
        const double progress = std::clamp(double(frame - currentShot->start) /
                                               std::max(1, currentShot->transitionDurationFrames - 1),
                                           0.0, 1.0);
        if (currentShot->transitionIn == "dissolve") {
            painter.setCompositionMode(QPainter::CompositionMode_Plus);
            painter.setOpacity(1.0 - progress);
            painter.drawImage(0, 0, outgoing);
            painter.setOpacity(progress);
            painter.drawImage(0, 0, incoming);
            painter.setOpacity(1.0);
            painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        } else if (currentShot->transitionIn == "fade") {
            painter.fillRect(QRect(0, 0, project.width, project.height), Qt::black);
            const double outgoingOpacity = std::max(0.0, 1.0 - progress * 2.0);
            const double incomingOpacity = std::max(0.0, progress * 2.0 - 1.0);
            painter.setCompositionMode(QPainter::CompositionMode_Plus);
            painter.setOpacity(outgoingOpacity);
            painter.drawImage(0, 0, outgoing);
            painter.setOpacity(incomingOpacity);
            painter.drawImage(0, 0, incoming);
            painter.setOpacity(1.0);
            painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        } else if (currentShot->transitionIn == "slide") {
            painter.drawImage(0, 0, outgoing);
            painter.save();
            painter.setClipRect(QRect(0, 0, project.width, project.height));
            double x = 0, y = 0;
            if (currentShot->transitionDirection == "left")
                x = project.width * (1.0 - progress);
            else if (currentShot->transitionDirection == "right")
                x = -project.width * (1.0 - progress);
            else if (currentShot->transitionDirection == "up")
                y = project.height * (1.0 - progress);
            else
                y = -project.height * (1.0 - progress);
            painter.translate(x, y);
            painter.drawImage(0, 0, incoming);
            painter.restore();
        } else { // zoom
            painter.drawImage(0, 0, outgoing);
            const double scale = 1.25 - 0.25 * progress;
            const double width = project.width * scale;
            const double height = project.height * scale;
            painter.drawImage(
                QRectF((project.width - width) / 2.0, (project.height - height) / 2.0, width, height),
                incoming);
        }
    }
    paintNodes(painter, project, frame, std::nullopt, NodePass::Captions, images_,
               [this, &project](const Node &node, std::int64_t number) {
                   return videoFrame(node, project, number);
               });
}

QImage CompositionPainter::frame(const Project &project, std::int64_t frameNumber) {
    QImage image(project.width, project.height, QImage::Format_RGBA8888);
    image.fill(Qt::black);
    QPainter painter(&image);
    paint(painter, project, frameNumber);
    painter.end();
    return image;
}
} // namespace studio
