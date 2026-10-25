#include <QFileInfo>
#include <QFont>
#include <QImageReader>
#include <QLinearGradient>
#include <QSvgRenderer>
#include <algorithm>
#include <optional>
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
    painter.fillRect(QRect(0, 0, project.width, project.height), QColor("#0b1014"));
    QRadialGradient glow(project.width * .82, project.height * .2, project.width * .8);
    glow.setColorAt(0, QColor("#1e3435"));
    glow.setColorAt(1, QColor("#0b1014"));
    painter.fillRect(QRect(0, 0, project.width, project.height), glow);
}

void paintNodes(QPainter &painter, const Project &project, std::int64_t frame,
                const std::optional<std::string> &shotId, NodePass pass, QCache<QString, QImage> &images) {
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
        } else if (node.kind == NodeKind::Text) {
            if (node.role == "caption") {
                painter.setPen(Qt::NoPen);
                painter.setBrush(QColor(8, 16, 24, 225));
                painter.drawRoundedRect(bounds, 12, 12);
            }
            QFont font("Segoe UI");
            font.setPixelSize(qRound(node.fontSize));
            font.setWeight(QFont::DemiBold);
            painter.setFont(font);
            painter.setPen(QColor(QString::fromStdString(node.color)));
            painter.drawText(node.role == "caption" ? bounds.adjusted(20, 8, -20, -8) : bounds,
                             (node.role == "caption" ? Qt::AlignCenter : Qt::AlignLeft | Qt::AlignTop) |
                                 Qt::TextWordWrap,
                             QString::fromStdString(node.text));
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
                 QCache<QString, QImage> &images) {
    QImage image(project.width, project.height, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    paintBackground(painter, project);
    paintNodes(painter, project, frame, shot.id, NodePass::ShotComposition, images);
    painter.end();
    return image;
}

} // namespace

void CompositionPainter::paint(QPainter &painter, const Project &project, std::int64_t frame) {
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    if (project.shots.empty()) {
        paintBackground(painter, project);
        paintNodes(painter, project, frame, std::nullopt, NodePass::All, images_);
        return;
    }

    const ShotMarker *currentShot = nullptr;
    for (const auto &shot : project.shots)
        if (frame >= shot.start && frame < shot.start + shot.duration &&
            (!currentShot || shot.start > currentShot->start))
            currentShot = &shot;
    if (!currentShot) {
        paintBackground(painter, project);
        paintNodes(painter, project, frame, std::nullopt, NodePass::Captions, images_);
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
        paintNodes(painter, project, frame, currentShot->id, NodePass::ShotComposition, images_);
    } else {
        const auto previousFrame = previousShot->start + previousShot->duration - 1;
        const auto outgoing = shotLayer(project, previousFrame, *previousShot, images_);
        const auto incoming = shotLayer(project, frame, *currentShot, images_);
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
    paintNodes(painter, project, frame, std::nullopt, NodePass::Captions, images_);
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
