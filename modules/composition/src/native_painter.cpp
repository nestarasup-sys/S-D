#include <QFont>
#include <QPainterPath>
#include <algorithm>
#include <cmath>
#include <studio/composition/native_painter.hpp>

namespace studio {
namespace {
void box(QPainter &p, QRectF r, QColor c, double radius = 8) {
    p.setPen(Qt::NoPen);
    p.setBrush(c);
    p.drawRoundedRect(r, radius, radius);
}
void line(QPainter &p, QPointF a, QPointF b, QColor c, double w = 3) {
    p.setPen(QPen(c, w, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.drawLine(a, b);
}
void ellipse(QPainter &p, QRectF r, QColor c) {
    p.setPen(Qt::NoPen);
    p.setBrush(c);
    p.drawEllipse(r);
}
void label(QPainter &p, QRectF r, QString s, int px, QColor c = QColor("#dbe9ec")) {
    QFont f("Segoe UI");
    f.setPixelSize(px);
    f.setWeight(QFont::DemiBold);
    p.setFont(f);
    p.setPen(c);
    p.drawText(r, Qt::AlignCenter, s);
}
void building(QPainter &p, QRectF r, QColor c) {
    box(p, r, c, 3);
    for (int row = 0; row < 5; ++row)
        for (int col = 0; col < 3; ++col)
            box(p,
                {r.x() + r.width() * (.15 + col * .25), r.y() + r.height() * (.1 + row * .16),
                 r.width() * .12, r.height() * .07},
                QColor("#7fa9ac"), 1);
}
} // namespace
void paintNative(QPainter &p, const Node &n, std::int64_t frame) {
    const auto id = n.nativeId;
    const QColor color(QString::fromStdString(n.color)), skin(QString::fromStdString(n.secondaryColor));
    p.save();
    p.setClipRect(QRectF(0, 0, n.width, n.height), Qt::IntersectClip);
    if (id == "street" || id == "office" || id == "showroom" || id == "studio") {
        // Native backdrops were authored in 16:9. Fill the target canvas without
        // stretching them; portrait canvases crop the sides while preserving scale.
        const double scale = std::max(n.width / 1280.0, n.height / 720.0);
        p.translate((n.width - 1280 * scale) / 2, (n.height - 720 * scale) / 2);
        p.scale(scale, scale);
        QLinearGradient gradient(0, 0, 1280, 720);
        gradient.setColorAt(0, QColor("#101e2b"));
        gradient.setColorAt(1, QColor("#173d42"));
        p.fillRect(QRectF(0, 0, 1280, 720), gradient);
        if (id == "street") {
            ellipse(p, {1030, 74, 130, 130}, QColor("#d9ba73"));
            for (int i = 0; i < 8; ++i)
                building(p,
                         {double(i * 178 - 30), double(180 + (i % 3) * 45), 145., double(350 - (i % 3) * 45)},
                         QColor(i % 2 ? "#294a56" : "#213d4b"));
            box(p, {0, 522, 1280, 198}, QColor("#0e1923"), 0);
            line(p, {0, 554}, {1280, 554}, QColor("#517079"), 4);
            for (int i = 0; i < 9; ++i)
                box(p, {double(i * 180 - 40), 622, 86, 5}, QColor("#91a59e"), 2);
        } else if (id == "showroom") {
            for (int i = 0; i < 5; ++i) {
                box(p, {double(40 + i * 250), 125, 220, 350}, QColor("#26464f"), 0);
                line(p, {double(150 + i * 250), 125}, {double(150 + i * 250), 475}, QColor("#365e66"), 2);
            }
            box(p, {0, 510, 1280, 210}, QColor("#162630"), 0);
            ellipse(p, {180, 510, 900, 90}, QColor("#29434a"));
            for (int i = 0; i < 4; ++i) {
                line(p, {double(190 + i * 280), 25}, {double(190 + i * 280), 77}, QColor("#6d8990"), 3);
                box(p, {double(120 + i * 280), 75, 140, 8}, QColor("#dcebd5"), 4);
            }
        } else if (id == "office") {
            box(p, {810, 120, 350, 300}, QColor("#1c414e"), 14);
            for (int i = 0; i < 3; ++i)
                building(p, {double(835 + i * 110), double(215 - i * 30), 80., double(190 + i * 30)},
                         QColor("#396171"));
            box(p, {60, 530, 1160, 190}, QColor("#14242e"), 0);
            line(p, {95, 455}, {740, 455}, QColor("#9c8262"), 18);
            line(p, {130, 460}, {130, 650}, QColor("#6a5c4f"), 12);
            line(p, {700, 460}, {700, 650}, QColor("#6a5c4f"), 12);
        } else {
            for (int i = 0; i < 8; ++i)
                line(p, {double(i * 200), 0}, {double(i * 200 - 140), 720}, QColor("#1b3742"), 1);
            ellipse(p, {900, 80, 260, 260}, QColor("#254e50"));
            ellipse(p, {930, 110, 200, 200}, QColor("#1a3d43"));
        }
        p.restore();
        return;
    }
    if (id == "car") {
        p.scale(n.width / 460, n.height / 210);
        ellipse(p, {15, 175, 430, 27}, QColor(0, 0, 0, 75));
        QPainterPath body;
        body.moveTo(20, 112);
        body.lineTo(82, 91);
        body.lineTo(142, 30);
        body.quadTo(156, 17, 180, 17);
        body.lineTo(310, 17);
        body.quadTo(330, 20, 349, 49);
        body.lineTo(386, 92);
        body.lineTo(436, 112);
        body.quadTo(452, 118, 452, 137);
        body.lineTo(449, 163);
        body.lineTo(14, 163);
        body.lineTo(14, 130);
        body.quadTo(14, 118, 20, 112);
        p.fillPath(body, color);
        QPainterPath glass;
        glass.moveTo(105, 91);
        glass.lineTo(159, 37);
        glass.lineTo(233, 37);
        glass.lineTo(233, 91);
        glass.closeSubpath();
        p.fillPath(glass, QColor("#b7d5df"));
        glass = QPainterPath();
        glass.moveTo(246, 37);
        glass.lineTo(306, 37);
        glass.lineTo(351, 91);
        glass.lineTo(246, 91);
        glass.closeSubpath();
        p.fillPath(glass, QColor("#b7d5df"));
        line(p, {245, 102}, {245, 151}, color.darker(135), 2);
        box(p, {262, 105, 26, 5}, color.darker(170), 2);
        box(p, {19, 119, 35, 12}, QColor("#fff1b9"), 5);
        box(p, {420, 119, 24, 12}, QColor("#fa8a73"), 4);
        for (double x : {103., 361.}) {
            ellipse(p, {x - 35, 137, 70, 70}, QColor("#101b25"));
            ellipse(p, {x - 21, 151, 42, 42}, QColor("#a4bbc0"));
            ellipse(p, {x - 7, 165, 14, 14}, QColor("#516b78"));
        }
    } else if (id == "person") {
        p.scale(n.width / 180, n.height / 350);
        const double bob = n.pose == "wave" ? std::sin((frame - n.start) * .13) * 3 : 0;
        p.translate(0, bob);
        ellipse(p, {21, 332, 140, 14}, QColor(0, 0, 0, 60));
        line(p, {72, 230}, {62, 320}, QColor("#263d55"), 25);
        line(p, {109, 230}, {123, 320}, QColor("#263d55"), 25);
        box(p, {36, 319, 42, 17}, QColor("#a5bac0"), 6);
        box(p, {112, 319, 42, 17}, QColor("#a5bac0"), 6);
        box(p, {52, 114, 78, 135}, color, 22);
        box(p, {77, 96, 28, 34}, skin, 8);
        line(p, {56, 137}, {30, 209}, color, 21);
        line(p, {30, 209}, {43, 258}, skin, 15);
        if (n.pose == "point") {
            line(p, {124, 137}, {155, 154}, color, 21);
            line(p, {155, 154}, {177, 112}, skin, 15);
            line(p, {177, 112}, {177, 89}, skin, 7);
        } else if (n.pose == "wave") {
            line(p, {124, 137}, {155, 108}, color, 21);
            double hand = 57 + std::sin((frame - n.start) * .15) * 10;
            line(p, {155, 108}, {163, hand}, skin, 15);
            ellipse(p, {153, hand - 15, 21, 25}, skin);
        } else if (n.pose == "worried") {
            line(p, {124, 137}, {151, 122}, color, 21);
            line(p, {151, 122}, {130, 65}, skin, 15);
        } else {
            line(p, {124, 137}, {150, 211}, color, 21);
            line(p, {150, 211}, {143, 258}, skin, 15);
        }
        ellipse(p, {53, 22, 79, 88}, skin);
        QPainterPath hair;
        hair.moveTo(54, 64);
        hair.quadTo(41, 12, 83, 12);
        hair.quadTo(139, 4, 133, 62);
        hair.lineTo(116, 41);
        hair.quadTo(87, 58, 62, 43);
        hair.closeSubpath();
        p.fillPath(hair, QColor("#28333e"));
        ellipse(p, {73, 66, 5, 6}, QColor("#233544"));
        ellipse(p, {106, 66, 5, 6}, QColor("#233544"));
        p.setPen(QPen(QColor("#9e6453"), 2.5, Qt::SolidLine, Qt::RoundCap));
        p.setBrush(Qt::NoBrush);
        p.drawArc(QRectF(80, n.pose == "worried" ? 88 : 77, 25, 19), n.pose == "worried" ? 0 : 180 * 16,
                  180 * 16);
    } else if (id == "building") {
        p.scale(n.width / 160, n.height / 300);
        building(p, {0, 0, 160, 300}, color);
    } else if (id == "house") {
        p.scale(n.width / 240, n.height / 210);
        box(p, {30, 76, 180, 132}, color, 3);
        QPainterPath roof;
        roof.moveTo(0, 87);
        roof.lineTo(120, 0);
        roof.lineTo(240, 87);
        roof.closeSubpath();
        p.fillPath(roof, color.lighter(135));
        box(p, {100, 130, 43, 78}, QColor("#1c3543"), 3);
        box(p, {52, 110, 31, 38}, skin, 2);
        box(p, {158, 110, 31, 38}, skin, 2);
    } else if (id == "document") {
        p.scale(n.width / 200, n.height / 250);
        box(p, {4, 3, 190, 244}, QColor("#e4e5d9"), 12);
        box(p, {25, 27, 75, 10}, color, 4);
        for (int i = 0; i < 5; ++i)
            box(p, {25., double(65 + i * 30), double(i % 2 ? 110 : 145), 6}, QColor("#7c989c"), 3);
        box(p, {25, 217, 95, 10}, color, 3);
    } else if (id == "computer") {
        p.scale(n.width / 280, n.height / 210);
        box(p, {0, 0, 280, 168}, QColor("#9bb4bc"), 12);
        box(p, {10, 10, 260, 145}, QColor("#142d3e"), 7);
        box(p, {130, 168, 20, 24}, QColor("#6c8b99"), 2);
        box(p, {85, 193, 110, 10}, QColor("#9bb4bc"), 4);
        box(p, {37, 42, 95, 10}, color, 4);
        for (int i = 0; i < 3; ++i)
            box(p, {37., double(70 + i * 21), double(195 - i * 24), 6}, QColor("#5a7c8d"), 3);
    } else if (id == "phone") {
        p.scale(n.width / 120, n.height / 220);
        box(p, {1, 0, 118, 220}, QColor("#829da9"), 18);
        box(p, {8, 8, 104, 204}, QColor("#142c3b"), 12);
        box(p, {40, 16, 40, 5}, QColor("#829da9"), 2);
        box(p, {26, 64, 68, 12}, color, 4);
        box(p, {26, 91, 68, 6}, QColor("#5c7e88"), 3);
        ellipse(p, {48, 181, 23, 23}, color);
    } else if (id == "coins") {
        p.scale(n.width / 250, n.height / 200);
        for (int col = 0; col < 3; ++col)
            for (int row = 0; row < 3 + col; ++row) {
                box(p, {double(6 + col * 78), double(166 - row * 24), 68, 22}, color.darker(115), 5);
                ellipse(p, {double(6 + col * 78), double(160 - row * 24), 68, 18}, color.lighter(115));
            }
    } else if (id == "price-tag") {
        p.scale(n.width / 250, n.height / 150);
        QPainterPath tag;
        tag.moveTo(38, 8);
        tag.lineTo(235, 8);
        tag.quadTo(249, 8, 249, 23);
        tag.lineTo(249, 125);
        tag.quadTo(249, 140, 235, 140);
        tag.lineTo(38, 140);
        tag.lineTo(0, 75);
        tag.closeSubpath();
        p.fillPath(tag, color);
        ellipse(p, {19, 62, 20, 20}, QColor("#183342"));
        label(p, {55, 20, 170, 105}, "R$", 68, QColor("#132c32"));
    } else if (id == "bars") {
        p.scale(n.width / 320, n.height / 240);
        line(p, {20, 214}, {310, 214}, QColor("#698592"), 2);
        for (int i = 0; i < 3; ++i) {
            double h = (80 + i * 48);
            double grow = std::clamp(double(frame - n.start) / 35., 0., 1.);
            box(p, {double(38 + i * 91), 214 - h * grow, 56, h * grow}, i == 1 ? color.lighter(120) : color,
                5);
        }
    }
    p.restore();
}
} // namespace studio
