#include <QFileInfo>
#include <QFont>
#include <QImageReader>
#include <QLinearGradient>
#include <studio/composition/painter.hpp>

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
    for (const auto &n : p.nodes) {
        auto state = evaluate(n, frame);
        if (state.opacity <= 0)
            continue;
        painter.save();
        painter.setOpacity(state.opacity);
        painter.translate(state.x, state.y);
        const QRectF bounds(0, 0, n.width, n.height);
        if (n.kind == NodeKind::Shape) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(QColor(QString::fromStdString(n.color)));
            painter.drawRoundedRect(bounds, std::min(12., n.height / 2), std::min(12., n.height / 2));
        } else if (n.kind == NodeKind::Text) {
            QFont font("Segoe UI");
            font.setPixelSize(qRound(n.fontSize));
            font.setWeight(QFont::DemiBold);
            painter.setFont(font);
            painter.setPen(QColor(QString::fromStdString(n.color)));
            painter.drawText(bounds, Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
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
