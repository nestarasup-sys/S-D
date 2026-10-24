#include <QFileInfo>
#include <QFont>
#include <QImageReader>
#include <QLinearGradient>
#include <QSvgRenderer>
#include <algorithm>
#include <studio/composition/native_painter.hpp>
#include <studio/composition/painter.hpp>
#include <vector>

namespace studio {
void CompositionPainter::paint(QPainter &painter, const Project &p, std::int64_t frame) {
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.fillRect(QRect(0, 0, p.width, p.height), QColor("#0b1014"));
    QRadialGradient glow(p.width * .82, p.height * .2, p.width * .8);
    glow.setColorAt(0, QColor("#1e3435"));
    glow.setColorAt(1, QColor("#0b1014"));
    painter.fillRect(QRect(0, 0, p.width, p.height), glow);
    const auto camera = evaluateCamera(p, frame);
    painter.save();
    painter.translate(p.width / 2.0, p.height / 2.0);
    painter.scale(camera.zoom, camera.zoom);
    painter.rotate(camera.rotation);
    painter.translate(-camera.x, -camera.y);
    for (const auto &n : p.nodes) {
        std::vector<const Node *> chain{&n};
        auto parentId = n.parentId;
        while (!parentId.empty()) {
            const auto parent = std::find_if(p.nodes.begin(), p.nodes.end(),
                                             [&](const Node &candidate) { return candidate.id == parentId; });
            if (parent == p.nodes.end())
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
        if (!visible || n.kind == NodeKind::Group)
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
        const QRectF bounds(0, 0, n.width, n.height);
        if (n.kind == NodeKind::Shape) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(QString::fromStdString(n.color)));
            painter.drawRoundedRect(bounds, std::min(12., n.height / 2), std::min(12., n.height / 2));
        } else if (n.kind == NodeKind::Native) {
            paintNative(painter, n, frame);
        } else if (n.kind == NodeKind::Svg) {
            QSvgRenderer svg(QString::fromStdString(n.source));
            if (svg.isValid())
                svg.render(&painter, bounds);
        } else if (n.kind == NodeKind::Text) {
            if (n.role == "caption") {
                painter.setPen(Qt::NoPen);
                painter.setBrush(QColor(8, 16, 24, 225));
                painter.drawRoundedRect(bounds, 12, 12);
            }
            QFont font("Segoe UI");
            font.setPixelSize(qRound(n.fontSize));
            font.setWeight(QFont::DemiBold);
            painter.setFont(font);
            painter.setPen(QColor(QString::fromStdString(n.color)));
            painter.drawText(n.role == "caption" ? bounds.adjusted(20, 8, -20, -8) : bounds,
                             (n.role == "caption" ? Qt::AlignCenter : Qt::AlignLeft | Qt::AlignTop) |
                                 Qt::TextWordWrap,
                             QString::fromStdString(n.text));
        } else {
            auto path = QString::fromStdString(n.source);
            if (!images_.contains(path)) {
                QImageReader reader(path);
                reader.setAutoTransform(true);
                auto size = reader.size();
                if (size.width() > p.width || size.height() > p.height) {
                    size.scale(p.width, p.height, Qt::KeepAspectRatio);
                    reader.setScaledSize(size);
                }
                auto image = new QImage(reader.read());
                auto cost = std::max<qsizetype>(1, image->sizeInBytes() / 1024);
                images_.insert(path, image, int(cost));
            }
            const auto cached = images_.object(path);
            const auto image = cached ? *cached : QImage{};
            if (!image.isNull()) {
                QSizeF size = image.size();
                size.scale(bounds.size(), Qt::KeepAspectRatio);
                painter.drawImage(QRectF((n.width - size.width()) / 2, (n.height - size.height()) / 2,
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
QImage CompositionPainter::frame(const Project &p, std::int64_t f) {
    QImage image(p.width, p.height, QImage::Format_RGBA8888);
    image.fill(Qt::black);
    QPainter painter(&image);
    paint(painter, p, f);
    painter.end();
    return image;
}
} // namespace studio
