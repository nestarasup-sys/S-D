#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QFontMetricsF>
#include <QImageReader>
#include <QLinearGradient>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QSvgRenderer>
#include <algorithm>
#include <functional>
#include <optional>
#include <studio/composition/native_painter.hpp>
#include <studio/composition/painter.hpp>
#include <studio/composition/render_snapshot.hpp>
#include <vector>

namespace studio {
namespace {
enum class NodePass { All, ShotComposition, Captions };
constexpr std::int64_t sequentialVideoDecodeWindow = 24;

QString videoFrameCacheKey(const Node &node, const Project &project, int width, int height,
                           std::int64_t localFrame) {
    const auto source = QString::fromStdString(node.source);
    const QFileInfo sourceInfo(source);
    const auto fingerprint = QStringList{QString::fromStdString(node.id),
                                         source,
                                         QString::number(sourceInfo.size()),
                                         QString::number(sourceInfo.lastModified().toMSecsSinceEpoch()),
                                         QString::number(width),
                                         QString::number(height),
                                         QString::number(node.videoSourceInSeconds, 'f', 6),
                                         QString::number(node.videoPlaybackRate, 'f', 6),
                                         QString::number(node.videoCropX, 'f', 6),
                                         QString::number(node.videoCropY, 'f', 6),
                                         QString::number(node.videoCropWidth, 'f', 6),
                                         QString::number(node.videoCropHeight, 'f', 6),
                                         QString::number(project.fps.numerator),
                                         QString::number(project.fps.denominator)}
                                 .join(QChar(0));
    const auto digest = QCryptographicHash::hash(fingerprint.toUtf8(), QCryptographicHash::Sha256).toHex();
    return QString::fromLatin1(digest) + ':' + QString::number(localFrame);
}

QString svgRendererCacheKey(const QString &source) {
    const QFileInfo info(source);
    return source + QChar(0) + QString::number(info.size()) + QChar(0) +
           QString::number(info.lastModified().toMSecsSinceEpoch());
}

double firstVideoFrameAtOrAfter(const QString &ffmpeg, const QString &source, double sourceInSeconds,
                                double fallbackFrameRate) {
    QProcess probe;
    probe.setProcessChannelMode(QProcess::SeparateChannels);
    probe.start(ffmpeg, {"-hide_banner", "-i", source});
    if (!probe.waitForStarted(2000))
        return 0.0;
    if (!probe.waitForFinished(5000)) {
        probe.kill();
        probe.waitForFinished(1000);
    }
    const auto diagnostic = QString::fromLocal8Bit(probe.readAllStandardError());
    static const QRegularExpression startExpression(R"(Duration:.*?start:\s*(-?\d+(?:\.\d+)?))");
    static const QRegularExpression frameRateExpression(R"((\d+(?:\.\d+)?)\s+fps\b)");
    const auto startMatch = startExpression.match(diagnostic);
    const auto frameRateMatch = frameRateExpression.match(diagnostic);
    const double startTime = startMatch.hasMatch() ? startMatch.captured(1).toDouble() : 0.0;
    const double frameRate =
        frameRateMatch.hasMatch() ? frameRateMatch.captured(1).toDouble() : fallbackFrameRate;
    if (frameRate <= 0.0)
        return startTime;
    return startTime + std::ceil(std::max(0.0, sourceInSeconds - startTime) * frameRate - 1e-9) / frameRate;
}

const ArtDirection &artDirectionFor(const Project &project, const std::string &sceneId,
                                    const std::string &shotId = {}) {
    if (!shotId.empty())
        for (const auto &shot : project.shots)
            if (shot.id == shotId && shot.artDirection)
                return *shot.artDirection;
    if (!sceneId.empty())
        for (const auto &scene : project.scenes)
            if (scene.id == sceneId && scene.artDirection)
                return *scene.artDirection;
    return project.artDirection;
}

void paintBackground(QPainter &painter, const Project &project, const ArtDirection &art) {
    const auto background = QColor(QString::fromStdString(art.backgroundColor));
    const auto glowColor = QColor(QString::fromStdString(art.glowColor));
    painter.fillRect(QRect(0, 0, project.width, project.height), background);
    QRadialGradient glow(project.width * .82, project.height * .2, project.width * .8);
    glow.setColorAt(0, glowColor);
    glow.setColorAt(1, background);
    painter.fillRect(QRect(0, 0, project.width, project.height), glow);
}

double animationEaseOut(double progress) {
    const auto t = std::clamp(progress, 0.0, 1.0);
    return 1.0 - (1.0 - t) * (1.0 - t) * (1.0 - t);
}

void paintNodes(QPainter &painter, const Project &project, const RenderFrameSnapshot &snapshot, NodePass pass,
                QCache<QString, QImage> &images, QCache<QString, QSvgRenderer> &svgRenderers,
                SvgCacheStats &svgCacheStats,
                const std::function<QImage(const Node &, std::int64_t)> &videoFrame,
                const std::string *nodeId = nullptr, bool localLayer = false) {
    const auto frame = snapshot.frame;
    painter.save();
    if (!localLayer) {
        painter.translate(project.width / 2.0, project.height / 2.0);
        painter.scale(snapshot.camera.zoom, snapshot.camera.zoom);
        painter.rotate(snapshot.camera.rotation);
        painter.translate(-snapshot.camera.x, -snapshot.camera.y);
    }
    for (const auto &resolved : snapshot.nodes) {
        const auto &node = resolved.node;
        if (nodeId && node.id != *nodeId)
            continue;
        if (pass == NodePass::Captions && (node.role != "caption" || !node.shotId.empty()))
            continue;
        if (pass == NodePass::ShotComposition) {
            const bool belongsToShot = snapshot.shotId && node.shotId == *snapshot.shotId;
            const bool isShared = node.shotId.empty() && node.role != "caption";
            if (!belongsToShot && !isShared)
                continue;
        }
        if (!resolved.visible || node.kind == NodeKind::Group)
            continue;
        const auto &art = resolved.artDirection;
        painter.save();
        painter.setOpacity(localLayer ? 1.0 : resolved.inheritedOpacity);
        if (!localLayer) {
            for (const auto &part : resolved.transformChain) {
                painter.translate(part.state.x, part.state.y);
                painter.translate(part.width / 2.0, part.height / 2.0);
                painter.rotate(part.state.rotation);
                painter.scale(part.state.scale, part.state.scale);
                painter.translate(-part.width / 2.0, -part.height / 2.0);
            }
        }
        const QRectF bounds(0, 0, node.width, node.height);
        if (node.kind == NodeKind::Shape) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(QString::fromStdString(node.color)));
            painter.drawRoundedRect(bounds, std::min(12., node.height / 2), std::min(12., node.height / 2));
        } else if (node.kind == NodeKind::Native) {
            paintNative(painter, node, frame);
        } else if (node.kind == NodeKind::Svg) {
            const auto source = QString::fromStdString(node.source);
            const auto key = svgRendererCacheKey(source);
            if (svgRenderers.contains(key))
                ++svgCacheStats.hits;
            else {
                ++svgCacheStats.misses;
                svgRenderers.insert(key, new QSvgRenderer(source));
            }
            auto *svg = svgRenderers.object(key);
            if (svg && svg->isValid())
                svg->render(&painter, bounds);
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
                painter.drawText(bounds, Qt::AlignCenter,
                                 "Vídeo indisponível\n" +
                                     QFileInfo(QString::fromStdString(node.source)).fileName());
            }
        } else if (node.kind == NodeKind::Text) {
            const QRectF textBounds = node.role == "caption" ? bounds.adjusted(20, 8, -20, -8) : bounds;
            if (node.role == "caption") {
                painter.setPen(Qt::NoPen);
                auto captionBackground = QColor(QString::fromStdString(art.captionBackgroundColor));
                captionBackground.setAlpha(225);
                painter.setBrush(captionBackground);
                painter.drawRoundedRect(bounds, 12, 12);
            }
            QFont font(QString::fromStdString(art.fontFamily));
            font.setPixelSize(qRound(node.fontSize));
            font.setWeight(QFont::DemiBold);
            painter.setFont(font);
            if (node.role == "caption" && !node.captionWords.empty()) {
                struct CaptionPart {
                    QString text;
                    QColor color;
                    qreal width;
                    qreal scale{1};
                    qreal opacity{1};
                    qreal offsetY{0};
                };
                struct CaptionLine {
                    std::vector<CaptionPart> parts;
                    qreal width{0};
                };
                const QFontMetricsF metrics(font);
                const auto baseColor =
                    QColor(QString::fromStdString(node.role == "caption" ? art.textColor : node.color));
                const auto highlightColor = QColor(QString::fromStdString(art.captionHighlightColor));
                const auto words =
                    QString::fromStdString(node.text).split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
                std::vector<CaptionLine> lines(1);
                std::size_t cueIndex = 0;
                const qreal wordGap = metrics.horizontalAdvance(QLatin1Char(' '));
                for (const auto &word : words) {
                    const bool hasCue = cueIndex < node.captionWords.size();
                    const auto cue = hasCue ? node.captionWords[cueIndex] : CaptionWordCue{};
                    const bool highlighted =
                        hasCue && frame >= cue.start && frame < cue.end && node.wordAnimationPreset != "none";
                    const qreal width = metrics.horizontalAdvance(word);
                    qreal wordScale = 1, wordOpacity = 1, wordOffsetY = 0;
                    if (hasCue && frame >= cue.start && frame < cue.end) {
                        const auto elapsed = std::max<std::int64_t>(0, frame - cue.start);
                        const auto progress =
                            animationEaseOut(double(elapsed) / std::max(1, node.wordAnimationDurationFrames));
                        if (node.wordAnimationPreset == "pop" || node.wordAnimationPreset == "pop-rise")
                            wordScale =
                                node.wordAnimationStartScale + (1 - node.wordAnimationStartScale) * progress;
                        if (node.wordAnimationPreset == "rise" || node.wordAnimationPreset == "pop-rise")
                            wordOffsetY = -node.wordAnimationTravelY * (1 - progress);
                        if (node.wordAnimationPreset == "fade")
                            wordOpacity = progress;
                    }
                    auto *line = &lines.back();
                    const qreal addedWidth = width + (line->parts.empty() ? 0 : wordGap);
                    if (!line->parts.empty() && line->width + addedWidth > textBounds.width()) {
                        lines.emplace_back();
                        line = &lines.back();
                    }
                    if (!line->parts.empty())
                        line->width += wordGap;
                    line->parts.push_back({word, highlighted ? highlightColor : baseColor, width, wordScale,
                                           wordOpacity, wordOffsetY});
                    line->width += width;
                    if (cueIndex < node.captionWords.size())
                        ++cueIndex;
                }
                const qreal lineHeight = metrics.height();
                const qreal totalHeight = lineHeight * lines.size();
                qreal baseline = textBounds.y() +
                                 std::max<qreal>(metrics.ascent(),
                                                 (textBounds.height() - totalHeight) / 2 + metrics.ascent());
                painter.save();
                for (const auto &line : lines) {
                    qreal x = textBounds.x() + (textBounds.width() - line.width) / 2;
                    for (std::size_t part = 0; part < line.parts.size(); ++part) {
                        if (part > 0)
                            x += wordGap;
                        const auto &captionPart = line.parts[part];
                        painter.save();
                        painter.setOpacity(captionPart.opacity);
                        painter.setPen(captionPart.color);
                        if (captionPart.scale != 1 || captionPart.offsetY != 0) {
                            const qreal centerX = x + captionPart.width / 2;
                            const qreal centerY = baseline - metrics.height() / 2 + captionPart.offsetY;
                            painter.translate(centerX, centerY);
                            painter.scale(captionPart.scale, captionPart.scale);
                            painter.drawText(QPointF(-captionPart.width / 2, metrics.height() / 2),
                                             captionPart.text);
                        } else
                            painter.drawText(QPointF(x, baseline), captionPart.text);
                        painter.restore();
                        x += line.parts[part].width;
                    }
                    baseline += lineHeight;
                }
                painter.restore();
            } else {
                painter.setPen(QColor(QString::fromStdString(node.color)));
                painter.drawText(textBounds,
                                 (node.role == "caption" ? Qt::AlignCenter : Qt::AlignLeft | Qt::AlignTop) |
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
                 QCache<QString, QImage> &images, QCache<QString, QSvgRenderer> &svgRenderers,
                 SvgCacheStats &svgCacheStats,
                 const std::function<QImage(const Node &, std::int64_t)> &videoFrame) {
    QImage image(project.width, project.height, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    paintBackground(painter, project, artDirectionFor(project, shot.sceneId, shot.id));
    // The requested layer can be the outgoing shot while the frame time has
    // advanced into the incoming shot. Resolve it explicitly for this layer.
    const auto layer = buildRenderFrameSnapshot(project, frame, shot.id);
    paintNodes(painter, project, layer, NodePass::ShotComposition, images, svgRenderers, svgCacheStats,
               videoFrame);
    painter.end();
    return image;
}

} // namespace

struct CompositionPainter::VideoDecoder {
    QProcess process;
    QByteArray pending;
    QImage lastImage;
    std::int64_t decodedFrame{-1};
    std::int64_t decodeStartFrame{0};
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
    videoFrames_.clear();
    svgRenderers_.clear();
    videoCacheStats_ = {};
    svgCacheStats_ = {};
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
    const auto cacheKey = videoFrameCacheKey(node, project, width, height, localFrame);
    if (const auto *cached = videoFrames_.object(cacheKey)) {
        ++videoCacheStats_.hits;
        return *cached;
    }
    ++videoCacheStats_.misses;
    const auto frameBytes = std::size_t(width) * std::size_t(height) * 4;
    const auto stopDecoder = [&] {
        if (decoder.process.state() != QProcess::NotRunning) {
            decoder.process.kill();
            decoder.process.waitForFinished(1000);
        }
        decoder.pending.clear();
        decoder.lastImage = {};
        decoder.decodedFrame = -1;
        decoder.decodeStartFrame = 0;
        decoder.endOfStream = false;
    };
    const bool decoderSettingsChanged =
        decoder.source != source || decoder.width != width || decoder.height != height ||
        decoder.sourceInSeconds != node.videoSourceInSeconds ||
        decoder.playbackRate != node.videoPlaybackRate || decoder.cropX != node.videoCropX ||
        decoder.cropY != node.videoCropY || decoder.cropWidth != node.videoCropWidth ||
        decoder.cropHeight != node.videoCropHeight;
    const bool jumpedAhead =
        decoder.decodedFrame >= 0 && localFrame - decoder.decodedFrame > sequentialVideoDecodeWindow;
    if (decoderSettingsChanged || localFrame < decoder.decodedFrame || jumpedAhead)
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
        decoder.decodeStartFrame = localFrame > sequentialVideoDecodeWindow ? localFrame : 0;
        decoder.decodedFrame = decoder.decodeStartFrame - 1;
        QStringList filters;
        if (decoder.decodeStartFrame == 0)
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
        QStringList arguments{"-nostdin", "-hide_banner", "-loglevel", "error"};
        if (decoder.decodeStartFrame > 0) {
            const double projectSecondsPerFrame = double(project.fps.denominator) / project.fps.numerator;
            const double fallbackFrameRate = double(project.fps.numerator) / project.fps.denominator;
            const double firstSourceFrame =
                firstVideoFrameAtOrAfter(ffmpeg, source, node.videoSourceInSeconds, fallbackFrameRate);
            const double sourceTime =
                firstSourceFrame + decoder.decodeStartFrame * projectSecondsPerFrame * node.videoPlaybackRate;
            arguments << "-ss" << QString::number(sourceTime, 'f', 6);
            ++videoCacheStats_.seekStarts;
        }
        arguments << "-i" << source << "-vf" << filters.join(',') << "-an" << "-sn" << "-dn" << "-f"
                  << "rawvideo" << "-pix_fmt" << "rgba" << "pipe:1";
        decoder.process.setProcessChannelMode(QProcess::SeparateChannels);
        decoder.process.start(ffmpeg, arguments);
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
                                   width * 4, QImage::Format_RGBA8888)
                                .copy();
        ++decoder.decodedFrame;
        ++videoCacheStats_.decodedFrames;
        const auto frameKey = videoFrameCacheKey(node, project, width, height, decoder.decodedFrame);
        const auto costKb = std::max(1, int(decoder.lastImage.sizeInBytes() / 1024));
        videoFrames_.insert(frameKey, new QImage(decoder.lastImage), costKb);
    }
    return decoder.lastImage;
}

void CompositionPainter::paint(QPainter &painter, const Project &project, std::int64_t frame) {
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    const auto snapshot = RenderSnapshot::build(project, frame);
    if (project.shots.empty()) {
        paintBackground(painter, project, snapshot.primary().backgroundArt);
        paintNodes(painter, project, snapshot.primary(), NodePass::All, images_, svgRenderers_,
                   svgCacheStats_, [this, &project](const Node &node, std::int64_t number) {
                       return videoFrame(node, project, number);
                   });
        return;
    }

    const ShotMarker *currentShot = nullptr;
    if (snapshot.primary().shotId)
        for (const auto &shot : snapshot.shots())
            if (shot.id == *snapshot.primary().shotId) {
                currentShot = &shot;
                break;
            }
    if (!currentShot) {
        paintBackground(painter, project, snapshot.primary().backgroundArt);
        paintNodes(painter, project, snapshot.primary(), NodePass::Captions, images_, svgRenderers_,
                   svgCacheStats_, [this, &project](const Node &node, std::int64_t number) {
                       return videoFrame(node, project, number);
                   });
        return;
    }

    const ShotMarker *previousShot = nullptr;
    if (snapshot.transition())
        for (const auto &shot : snapshot.shots())
            if (shot.id == snapshot.transition()->outgoingShotId) {
                previousShot = &shot;
                break;
            }

    if (!previousShot || currentShot->transitionIn == "cut") {
        paintBackground(painter, project, artDirectionFor(project, currentShot->sceneId, currentShot->id));
        paintNodes(painter, project, snapshot.primary(), NodePass::ShotComposition, images_, svgRenderers_,
                   svgCacheStats_, [this, &project](const Node &node, std::int64_t number) {
                       return videoFrame(node, project, number);
                   });
    } else {
        const auto previousFrame = snapshot.transition()->outgoingFrame;
        const auto decoder = [this, &project](const Node &node, std::int64_t number) {
            return videoFrame(node, project, number);
        };
        const auto outgoing =
            shotLayer(project, previousFrame, *previousShot, images_, svgRenderers_, svgCacheStats_, decoder);
        const auto incoming =
            shotLayer(project, frame, *currentShot, images_, svgRenderers_, svgCacheStats_, decoder);
        const double progress = snapshot.transition()->progress;
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
    paintNodes(painter, project, snapshot.primary(), NodePass::Captions, images_, svgRenderers_,
               svgCacheStats_, [this, &project](const Node &node, std::int64_t number) {
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

QImage CompositionPainter::rasterNodeLayer(const Project &project, const RenderFrameSnapshot &snapshot,
                                           const RenderNodeSnapshot &node, double supersamplingScale) {
    if (!node.visible || node.node.kind == NodeKind::Group || node.node.kind == NodeKind::Video)
        return {};
    const auto scale = std::clamp(supersamplingScale, 1.0, 4.0);
    const QSize size(std::clamp(qCeil(node.node.width * scale), 1, 4096),
                     std::clamp(qCeil(node.node.height * scale), 1, 4096));
    QImage image(size, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.scale(scale, scale);
    paintNodes(painter, project, snapshot, NodePass::All, images_, svgRenderers_, svgCacheStats_,
               [this, &project](const Node &current, std::int64_t frame) {
                   return videoFrame(current, project, frame);
               },
               &node.node.id, true);
    painter.end();
    return image;
}

QImage CompositionPainter::rasterBackgroundLayer(const Project &project, const ArtDirection &art) {
    QImage image(project.width, project.height, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    paintBackground(painter, project, art);
    painter.end();
    return image;
}
} // namespace studio
