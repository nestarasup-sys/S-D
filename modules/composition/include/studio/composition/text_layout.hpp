#pragma once

#include <QFont>
#include <QRectF>
#include <QString>
#include <studio/core/project.hpp>

namespace studio {

struct TextLayoutResult {
    int pixelSize{12};
    QRectF contentBounds;
    QRectF measuredBounds;
    bool fits{true};
    bool reduced{false};
};

// Uses the same QFontMetricsF word-wrap calculation for drawing and visual QA.
TextLayoutResult resolvePremiumTextLayout(const Node &node, const ArtDirection &art);
QRectF measureWrappedText(const QString &text, const QFont &font, const QRectF &bounds,
                          Qt::Alignment alignment);

} // namespace studio
