#include <QFontMetricsF>
#include <Qt>
#include <algorithm>
#include <studio/composition/text_layout.hpp>

namespace studio {
QRectF measureWrappedText(const QString &text, const QFont &font, const QRectF &bounds,
                          Qt::Alignment alignment) {
    const auto horizontalAlignment = alignment & Qt::AlignHorizontal_Mask;
    return QFontMetricsF(font).boundingRect(QRectF(0, 0, bounds.width(), 10000),
                                            horizontalAlignment | Qt::AlignTop | Qt::TextWordWrap,
                                            text);
}

TextLayoutResult resolvePremiumTextLayout(const Node &node, const ArtDirection &art) {
    TextLayoutResult result;
    const double padding = node.textAutoFit ? node.textPadding : 0;
    result.contentBounds = QRectF(0, 0, node.width, node.height).adjusted(padding, padding, -padding, -padding);
    const int requested = std::max(8, qRound(node.fontSize));
    // Preserve the historic premium behavior for older projects. New manifests
    // use their explicit minimum; no policy ever shrinks below its declared floor.
    const int minimum = node.textAutoFit ? std::max(8, qRound(node.textMinFontSize))
                                         : (node.role.rfind("premium:", 0) == 0 ? 12 : requested);
    QFont font(QString::fromStdString(art.fontFamily));
    font.setWeight(QFont::DemiBold);
    const auto text = QString::fromStdString(node.text);
    const auto alignment = (node.role == "caption" ? Qt::AlignCenter : Qt::AlignLeft | Qt::AlignTop);
    result.pixelSize = requested;
    auto measure = [&] {
        font.setPixelSize(result.pixelSize);
        result.measuredBounds = measureWrappedText(text, font, result.contentBounds, alignment);
        result.fits = result.measuredBounds.width() <= result.contentBounds.width() + .5 &&
                      result.measuredBounds.height() <= result.contentBounds.height() + .5;
    };
    measure();
    if (!result.fits && requested > minimum) {
        int low = minimum, high = requested - 1, largestFit = minimum;
        font.setPixelSize(minimum);
        result.measuredBounds = measureWrappedText(text, font, result.contentBounds, alignment);
        const bool minimumFits = result.measuredBounds.width() <= result.contentBounds.width() + .5 &&
                                 result.measuredBounds.height() <= result.contentBounds.height() + .5;
        if (minimumFits) {
            while (low <= high) {
                const int candidate = low + (high - low) / 2;
                font.setPixelSize(candidate);
                const auto measured = measureWrappedText(text, font, result.contentBounds, alignment);
                const bool fits = measured.width() <= result.contentBounds.width() + .5 &&
                                  measured.height() <= result.contentBounds.height() + .5;
                if (fits) {
                    largestFit = candidate;
                    low = candidate + 1;
                } else {
                    high = candidate - 1;
                }
            }
        }
        result.pixelSize = largestFit;
        measure();
    }
    result.reduced = result.pixelSize < requested;
    return result;
}
} // namespace studio
