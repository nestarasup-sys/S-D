#include <QColor>
#include <QCommandLineParser>
#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSGGeometry>
#include <QSGGeometryNode>
#include <QSGRendererInterface>
#include <QSGVertexColorMaterial>
#include <QSurfaceFormat>
#include <QTextStream>
#include <QThread>
#include <QTimer>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <memory>
#include <numbers>
#include <optional>
#include <vector>
#ifdef Q_OS_WIN
#include <windows.h>

#include <psapi.h>
#endif
#include <studio/composition/painter.hpp>
#include <studio/composition/render_snapshot.hpp>
#include <studio/core/project.hpp>

#ifndef STUDIO_PRIMITIVE_REFERENCE_DIR
#define STUDIO_PRIMITIVE_REFERENCE_DIR "tests/reference/render-primitives"
#endif

namespace {
struct Vertex {
    float x, y;
    uchar r, g, b, a;
};

studio::Project referenceProject(int width, int height, int nodeCount) {
    auto project = studio::makeEmpty();
    project.width = width;
    project.height = height;
    project.duration = 120;
    project.artDirection.backgroundColor = "#101820";
    project.artDirection.glowColor = "#101820";
    studio::Node root;
    root.id = "root-group";
    root.kind = studio::NodeKind::Group;
    root.name = "Nested benchmark root";
    root.start = 0;
    root.duration = project.duration;
    root.x = 0;
    root.y = 0;
    root.width = width;
    root.height = height;
    root.animated = false;
    root.opacity = .94;
    project.nodes.push_back(root);
    const int columns = std::max(1, int(std::sqrt(nodeCount * double(width) / height)));
    const int rows = (nodeCount + columns - 1) / columns;
    const double cellWidth = double(width) / columns;
    const double cellHeight = double(height) / rows;
    for (int index = 0; index < nodeCount; ++index) {
        const int column = index % columns;
        const int row = index / columns;
        studio::Node group;
        group.id = "group-" + std::to_string(index);
        group.parentId = root.id;
        group.kind = studio::NodeKind::Group;
        group.name = "Benchmark group";
        group.start = 0;
        group.duration = project.duration;
        group.x = column * cellWidth;
        group.y = row * cellHeight;
        if (index % 19 == 0)
            group.x -= cellWidth * .55;
        if (index % 23 == 0)
            group.y += cellHeight * .6;
        group.opacity = .96;
        group.keyframes = {{0, group.x, group.y, 0, 1, .96, "ease-in-out"},
                           {60, group.x + cellWidth * .025, group.y, 1, 1, .92, "ease-in-out"},
                           {119, group.x, group.y, 0, 1, .96, "linear"}};
        group.animated = false;
        project.nodes.push_back(group);

        studio::Node shape;
        shape.id = "shape-" + std::to_string(index);
        shape.parentId = group.id;
        shape.kind = studio::NodeKind::Shape;
        shape.name = "Benchmark vector";
        shape.start = 0;
        shape.duration = project.duration;
        shape.x = cellWidth * .12;
        shape.y = cellHeight * .18;
        shape.width = cellWidth * .76;
        shape.height = cellHeight * .64;
        shape.animated = false;
        shape.color = index % 2 ? "#b4f078" : "#efb873";
        shape.keyframes = {
            {0, shape.x, shape.y, 0, 1, 1, "ease-in-out"},
            {60, shape.x + cellWidth * .06, shape.y + cellHeight * .06, 4, 1, .92, "ease-in-out"},
            {119, shape.x, shape.y, 0, 1, 1, "linear"}};
        project.nodes.push_back(shape);
    }
    project.camera = {{0, width / 2.0, height / 2.0, 1, 0, "ease-in-out", {}},
                      {119, width / 2.0 + width * .025, height / 2.0, 1.025, 0, "linear", {}}};
    return project;
}

std::vector<QPointF> roundedRect(const QRectF &rect) {
    const double radius = std::min({12.0, rect.width() / 2.0, rect.height() / 2.0});
    std::vector<QPointF> points;
    constexpr int steps = 12;
    const QPointF centers[] = {{rect.right() - radius, rect.top() + radius},
                               {rect.right() - radius, rect.bottom() - radius},
                               {rect.left() + radius, rect.bottom() - radius},
                               {rect.left() + radius, rect.top() + radius}};
    for (int corner = 0; corner < 4; ++corner) {
        const double start = -90.0 + corner * 90.0;
        for (int step = 0; step <= steps; ++step) {
            const double angle = qDegreesToRadians(start + step * 90.0 / steps);
            points.emplace_back(centers[corner].x() + std::cos(angle) * radius,
                                centers[corner].y() + std::sin(angle) * radius);
        }
    }
    return points;
}

std::vector<QPointF> ellipse(const QRectF &rect) {
    std::vector<QPointF> points;
    constexpr int segments = 64;
    points.reserve(segments);
    for (int index = 0; index < segments; ++index) {
        const auto angle = 2.0 * std::numbers::pi * index / segments;
        points.emplace_back(rect.center().x() + std::cos(angle) * rect.width() / 2.0,
                            rect.center().y() + std::sin(angle) * rect.height() / 2.0);
    }
    return points;
}

void appendPolygon(std::vector<Vertex> &vertices, const std::vector<QPointF> &points, const QColor &color,
                   double opacity, bool antialias = true) {
    if (points.size() < 3)
        return;
    QPointF center;
    for (const auto &point : points)
        center += point;
    center /= points.size();
    const auto add = [&](QPointF point, uchar alpha) {
        const auto effectiveAlpha = uchar(std::clamp(qRound(alpha * opacity), 0, 255));
        const auto premultiply = [effectiveAlpha](int channel) {
            return uchar(qRound(channel * effectiveAlpha / 255.0));
        };
        vertices.push_back({float(point.x()), float(point.y()), premultiply(color.red()),
                            premultiply(color.green()), premultiply(color.blue()), effectiveAlpha});
    };
    for (std::size_t index = 0; index < points.size(); ++index) {
        const auto &a = points[index];
        const auto &b = points[(index + 1) % points.size()];
        add(center, color.alpha());
        add(a, color.alpha());
        add(b, color.alpha());
        if (!antialias)
            continue;
        QPointF edge = b - a;
        const double length = std::hypot(edge.x(), edge.y());
        if (length <= 1e-6)
            continue;
        // The points are clockwise in screen coordinates; this is the outward normal.
        const QPointF normal(edge.y() / length, -edge.x() / length);
        const QPointF outerA = a + normal;
        const QPointF outerB = b + normal;
        add(a, color.alpha());
        add(b, color.alpha());
        add(outerB, 0);
        add(a, color.alpha());
        add(outerB, 0);
        add(outerA, 0);
    }
}

void appendStroke(std::vector<Vertex> &vertices, const std::vector<QPointF> &points, const QColor &color,
                  double opacity, double width, bool closed = false) {
    if (points.size() < 2 || width <= 0)
        return;
    const auto segmentCount = closed ? points.size() : points.size() - 1;
    for (std::size_t index = 0; index < segmentCount; ++index) {
        const auto a = points[index];
        const auto b = points[(index + 1) % points.size()];
        const QPointF delta = b - a;
        const double length = std::hypot(delta.x(), delta.y());
        if (length <= 1e-6)
            continue;
        const QPointF normal(-delta.y() / length * width / 2.0, delta.x() / length * width / 2.0);
        appendPolygon(vertices, {a + normal, b + normal, b - normal, a - normal}, color, opacity);
    }
}

std::vector<QPointF> primitivePoints(const studio::RenderNodeSnapshot &resolved) {
    const QRectF rect(0, 0, resolved.node.width, resolved.node.height);
    switch (resolved.primitive) {
    case studio::RenderPrimitiveKind::RoundedRectangle:
        return roundedRect(rect);
    case studio::RenderPrimitiveKind::Rectangle:
    case studio::RenderPrimitiveKind::RectangleOutline:
        return {{rect.left(), rect.top()},
                {rect.right(), rect.top()},
                {rect.right(), rect.bottom()},
                {rect.left(), rect.bottom()}};
    case studio::RenderPrimitiveKind::Ellipse:
        return ellipse(rect);
    case studio::RenderPrimitiveKind::Line:
        return resolved.path.size() >= 2
                   ? resolved.path
                   : std::vector<QPointF>{{rect.left(), rect.top()}, {rect.right(), rect.bottom()}};
    case studio::RenderPrimitiveKind::ConvexPath:
        return resolved.path;
    }
    return {};
}

std::optional<studio::RenderPrimitiveKind> parsePrimitiveKind(const QString &name) {
    if (name == "roundedRectangle")
        return studio::RenderPrimitiveKind::RoundedRectangle;
    if (name == "rectangle")
        return studio::RenderPrimitiveKind::Rectangle;
    if (name == "ellipse")
        return studio::RenderPrimitiveKind::Ellipse;
    if (name == "rectangleOutline")
        return studio::RenderPrimitiveKind::RectangleOutline;
    if (name == "line")
        return studio::RenderPrimitiveKind::Line;
    if (name == "convexPath")
        return studio::RenderPrimitiveKind::ConvexPath;
    return std::nullopt;
}

void applyPrimitiveSuite(studio::RenderFrameSnapshot &frame,
                         std::optional<studio::RenderPrimitiveKind> forcedKind = std::nullopt) {
    std::size_t shapeIndex = 0;
    for (auto &resolved : frame.nodes) {
        if (resolved.node.kind != studio::NodeKind::Shape)
            continue;
        if (forcedKind) {
            resolved.primitive = *forcedKind;
            if (*forcedKind == studio::RenderPrimitiveKind::RectangleOutline)
                resolved.filled = false;
            if (*forcedKind == studio::RenderPrimitiveKind::Line)
                resolved.path = {{0, resolved.node.height},
                                 {resolved.node.width * .28, 0},
                                 {resolved.node.width * .72, resolved.node.height},
                                 {resolved.node.width, 0}};
            if (*forcedKind == studio::RenderPrimitiveKind::ConvexPath)
                resolved.path = {{resolved.node.width * .5, 0},
                                 {resolved.node.width, resolved.node.height * .35},
                                 {resolved.node.width * .82, resolved.node.height},
                                 {resolved.node.width * .18, resolved.node.height},
                                 {0, resolved.node.height * .35}};
            resolved.strokeWidth = 3.0;
            continue;
        }
        switch (shapeIndex++ % 6) {
        case 0:
            resolved.primitive = studio::RenderPrimitiveKind::Rectangle;
            break;
        case 1:
            resolved.primitive = studio::RenderPrimitiveKind::RoundedRectangle;
            break;
        case 2:
            resolved.primitive = studio::RenderPrimitiveKind::Ellipse;
            break;
        case 3:
            resolved.primitive = studio::RenderPrimitiveKind::RectangleOutline;
            resolved.filled = false;
            break;
        case 4:
            resolved.primitive = studio::RenderPrimitiveKind::Line;
            resolved.path = {{0, resolved.node.height},
                             {resolved.node.width * .28, 0},
                             {resolved.node.width * .72, resolved.node.height},
                             {resolved.node.width, 0}};
            break;
        case 5:
            resolved.primitive = studio::RenderPrimitiveKind::ConvexPath;
            resolved.path = {{resolved.node.width * .5, 0},
                             {resolved.node.width, resolved.node.height * .35},
                             {resolved.node.width * .82, resolved.node.height},
                             {resolved.node.width * .18, resolved.node.height},
                             {0, resolved.node.height * .35}};
            break;
        }
        resolved.strokeWidth = 3.0;
    }
}

QImage paintPrimitiveReference(const studio::RenderFrameSnapshot &frame, int width, int height) {
    QImage image(width, height, QImage::Format_ARGB32_Premultiplied);
    image.fill(QColor("#101820"));
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    painter.translate(width / 2.0, height / 2.0);
    painter.scale(frame.camera.zoom, frame.camera.zoom);
    painter.rotate(frame.camera.rotation);
    painter.translate(-frame.camera.x, -frame.camera.y);
    for (const auto &resolved : frame.nodes) {
        if (resolved.node.kind != studio::NodeKind::Shape || !resolved.visible)
            continue;
        painter.save();
        painter.setOpacity(resolved.inheritedOpacity);
        for (const auto &part : resolved.transformChain) {
            painter.translate(part.state.x, part.state.y);
            painter.translate(part.width / 2.0, part.height / 2.0);
            painter.rotate(part.state.rotation);
            painter.scale(part.state.scale, part.state.scale);
            painter.translate(-part.width / 2.0, -part.height / 2.0);
        }
        const QRectF bounds(0, 0, resolved.node.width, resolved.node.height);
        QPen pen(QColor(QString::fromStdString(resolved.node.color)));
        pen.setWidthF(resolved.strokeWidth);
        painter.setPen(resolved.filled && resolved.primitive != studio::RenderPrimitiveKind::Line ? Qt::NoPen
                                                                                                  : pen);
        painter.setBrush(resolved.filled && resolved.primitive != studio::RenderPrimitiveKind::Line
                             ? QColor(QString::fromStdString(resolved.node.color))
                             : Qt::NoBrush);
        if (resolved.primitive == studio::RenderPrimitiveKind::RectangleOutline ||
            resolved.primitive == studio::RenderPrimitiveKind::Line)
            painter.setBrush(QBrush(Qt::NoBrush));
        switch (resolved.primitive) {
        case studio::RenderPrimitiveKind::RoundedRectangle:
            painter.drawRoundedRect(bounds, std::min({12.0, bounds.width() / 2, bounds.height() / 2}),
                                    std::min({12.0, bounds.width() / 2, bounds.height() / 2}));
            break;
        case studio::RenderPrimitiveKind::Rectangle:
        case studio::RenderPrimitiveKind::RectangleOutline:
            painter.drawRect(bounds);
            break;
        case studio::RenderPrimitiveKind::Ellipse:
            painter.drawEllipse(bounds);
            break;
        case studio::RenderPrimitiveKind::Line:
            if (resolved.path.size() > 1)
                painter.drawPolyline(resolved.path.data(), int(resolved.path.size()));
            break;
        case studio::RenderPrimitiveKind::ConvexPath:
            if (resolved.path.size() > 2)
                painter.drawPolygon(resolved.path.data(), int(resolved.path.size()));
            break;
        }
        painter.restore();
    }
    painter.end();
    return image;
}

class SceneGraphItem final : public QQuickItem {
  public:
    SceneGraphItem(studio::RenderFrameSnapshot snapshot, int canvasWidth, int canvasHeight)
        : snapshot_(std::move(snapshot)), canvasWidth_(canvasWidth), canvasHeight_(canvasHeight) {
        setFlag(ItemHasContents);
        setClip(true);
    }
    void setSnapshot(studio::RenderFrameSnapshot snapshot) {
        snapshot_ = std::move(snapshot);
        update();
    }
    const studio::RenderFrameSnapshot &snapshot() const {
        return snapshot_;
    }
    qint64 preparationNanos() const {
        return preparationNanos_.load();
    }
    qint64 geometryBytes() const {
        return geometryBytes_.load();
    }
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) override {
        QElapsedTimer preparationTimer;
        preparationTimer.start();
        // Qt Quick's D3D11 path uses 16-bit geometry indices. Keep each draw
        // batch below that limit; a single oversized geometry silently loses
        // later triangles on some Qt/driver combinations.
        constexpr std::size_t maxVerticesPerBatch = 60000;
        std::vector<std::vector<Vertex>> batches(1);
        const auto &frame = snapshot_;
        const double sx = width() / canvasWidth_;
        const double sy = height() / canvasHeight_;
        for (const auto &resolved : frame.nodes) {
            const auto &shape = resolved.node;
            if (shape.kind != studio::NodeKind::Shape || !resolved.visible || resolved.inheritedOpacity <= 0)
                continue;
            const QColor color(QString::fromStdString(shape.color));
            const auto localPoints = primitivePoints(resolved);
            std::vector<QPointF> points;
            points.reserve(localPoints.size());
            for (const auto &point : localPoints) {
                const auto mapped = frame.cameraTransform.map(resolved.sceneTransform.map(point));
                points.emplace_back(mapped.x() * sx, mapped.y() * sy);
            }
            std::vector<Vertex> shapeVertices;
            if (resolved.primitive == studio::RenderPrimitiveKind::Line)
                appendStroke(shapeVertices, points, color, resolved.inheritedOpacity, resolved.strokeWidth * sx);
            else if (resolved.primitive == studio::RenderPrimitiveKind::RectangleOutline)
                appendStroke(shapeVertices, points, color, resolved.inheritedOpacity, resolved.strokeWidth * sx,
                             true);
            else if (resolved.filled)
                appendPolygon(shapeVertices, points, color, resolved.inheritedOpacity);
            else
                appendStroke(shapeVertices, points, color, resolved.inheritedOpacity, resolved.strokeWidth * sx,
                             true);
            if (!batches.back().empty() && batches.back().size() + shapeVertices.size() > maxVerticesPerBatch)
                batches.emplace_back();
            batches.back().insert(batches.back().end(), shapeVertices.begin(), shapeVertices.end());
        }
        auto *root = new QSGNode;
        qint64 geometryBytes = 0;
        for (const auto &vertices : batches) {
            if (vertices.empty())
                continue;
            auto *node = new QSGGeometryNode;
            auto *geometry = new QSGGeometry(QSGGeometry::defaultAttributes_ColoredPoint2D(), int(vertices.size()));
            geometry->setDrawingMode(QSGGeometry::DrawTriangles);
            geometry->setVertexDataPattern(QSGGeometry::StaticPattern);
            auto *destination = geometry->vertexDataAsColoredPoint2D();
            for (std::size_t index = 0; index < vertices.size(); ++index) {
                const auto &vertex = vertices[index];
                destination[index].set(vertex.x, vertex.y, vertex.r, vertex.g, vertex.b, vertex.a);
            }
            node->setGeometry(geometry);
            node->setFlag(QSGNode::OwnsGeometry);
            auto *material = new QSGVertexColorMaterial;
            material->setFlag(QSGMaterial::Blending, true);
            node->setMaterial(material);
            node->setFlag(QSGNode::OwnsMaterial);
            root->appendChildNode(node);
            geometryBytes += qint64(vertices.size() * sizeof(Vertex));
        }
        delete oldNode;
        geometryBytes_.store(geometryBytes);
        preparationNanos_.fetch_add(preparationTimer.nsecsElapsed());
        return root;
    }

  private:
    studio::RenderFrameSnapshot snapshot_;
    int canvasWidth_{0}, canvasHeight_{0};
    std::atomic<qint64> preparationNanos_{0};
    std::atomic<qint64> geometryBytes_{0};
};

QString apiName(QSGRendererInterface::GraphicsApi api) {
    switch (api) {
    case QSGRendererInterface::Direct3D11:
        return "Direct3D11";
    case QSGRendererInterface::OpenGL:
        return "OpenGL";
    case QSGRendererInterface::Vulkan:
        return "Vulkan";
    case QSGRendererInterface::Metal:
        return "Metal";
    case QSGRendererInterface::Software:
        return "Software";
    default:
        return "Unknown";
    }
}

qint64 workingSetBytes() {
#ifdef Q_OS_WIN
    PROCESS_MEMORY_COUNTERS_EX counters{};
    if (GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS *>(&counters),
                             sizeof(counters)))
        return qint64(counters.WorkingSetSize);
#endif
    return -1;
}

double psnr(const QImage &left, const QImage &right) {
    if (left.size() != right.size() || left.isNull() || right.isNull())
        return 0;
    const auto normalizedLeft = left.convertToFormat(QImage::Format_ARGB32);
    const auto normalizedRight = right.convertToFormat(QImage::Format_ARGB32);
    long double squaredError = 0;
    const auto pixels = qint64(normalizedLeft.width()) * normalizedLeft.height() * 3;
    for (int y = 0; y < normalizedLeft.height(); ++y) {
        const auto *a = reinterpret_cast<const QRgb *>(normalizedLeft.constScanLine(y));
        const auto *b = reinterpret_cast<const QRgb *>(normalizedRight.constScanLine(y));
        for (int x = 0; x < normalizedLeft.width(); ++x) {
            const int dr = qRed(a[x]) - qRed(b[x]);
            const int dg = qGreen(a[x]) - qGreen(b[x]);
            const int db = qBlue(a[x]) - qBlue(b[x]);
            squaredError += dr * dr + dg * dg + db * db;
        }
    }
    const double mse = double(squaredError / pixels);
    return mse <= 0 ? 99.0 : 10.0 * std::log10(255.0 * 255.0 / mse);
}

QImage differenceImage(const QImage &left, const QImage &right) {
    const auto a = left.convertToFormat(QImage::Format_ARGB32);
    const auto b = right.convertToFormat(QImage::Format_ARGB32);
    QImage result(a.size(), QImage::Format_ARGB32);
    for (int y = 0; y < a.height(); ++y) {
        const auto *lineA = reinterpret_cast<const QRgb *>(a.constScanLine(y));
        const auto *lineB = reinterpret_cast<const QRgb *>(b.constScanLine(y));
        auto *lineOut = reinterpret_cast<QRgb *>(result.scanLine(y));
        for (int x = 0; x < a.width(); ++x) {
            const int dr = std::abs(qRed(lineA[x]) - qRed(lineB[x]));
            const int dg = std::abs(qGreen(lineA[x]) - qGreen(lineB[x]));
            const int db = std::abs(qBlue(lineA[x]) - qBlue(lineB[x]));
            lineOut[x] = qRgb(std::min(255, dr * 4), std::min(255, dg * 4), std::min(255, db * 4));
        }
    }
    return result;
}

QJsonObject imageMetrics(const QImage &left, const QImage &right) {
    if (left.isNull() || right.isNull() || left.size() != right.size())
        return {{"meanAbsoluteColorError", 255.0},
                {"foregroundIntersectionOverUnion", 0.0},
                {"edgeF1Within1Px", 0.0},
                {"cpuEdgePixels", 0.0},
                {"gpuEdgePixels", 0.0},
                {"sizeMismatch", true}};
    const auto a = left.convertToFormat(QImage::Format_ARGB32);
    const auto b = right.convertToFormat(QImage::Format_ARGB32);
    const QColor background("#101820");
    quint64 absoluteError = 0, intersection = 0, unionCount = 0;
    quint64 edgeA = 0, edgeB = 0, matchedA = 0, matchedB = 0;
    const auto foreground = [&](const QImage &image, int x, int y) {
        const auto color = QColor::fromRgb(image.pixel(x, y));
        return std::abs(color.red() - background.red()) + std::abs(color.green() - background.green()) +
                   std::abs(color.blue() - background.blue()) >
               32;
    };
    const auto edge = [&](const QImage &image, int x, int y) {
        const bool current = foreground(image, x, y);
        return (x > 0 && foreground(image, x - 1, y) != current) ||
               (x + 1 < image.width() && foreground(image, x + 1, y) != current) ||
               (y > 0 && foreground(image, x, y - 1) != current) ||
               (y + 1 < image.height() && foreground(image, x, y + 1) != current);
    };
    for (int y = 0; y < a.height(); ++y) {
        const auto *lineA = reinterpret_cast<const QRgb *>(a.constScanLine(y));
        const auto *lineB = reinterpret_cast<const QRgb *>(b.constScanLine(y));
        for (int x = 0; x < a.width(); ++x) {
            absoluteError += std::abs(qRed(lineA[x]) - qRed(lineB[x])) +
                             std::abs(qGreen(lineA[x]) - qGreen(lineB[x])) +
                             std::abs(qBlue(lineA[x]) - qBlue(lineB[x]));
            const bool fa = foreground(a, x, y), fb = foreground(b, x, y);
            intersection += fa && fb;
            unionCount += fa || fb;
            const bool ea = edge(a, x, y), eb = edge(b, x, y);
            edgeA += ea;
            edgeB += eb;
            if (ea || eb) {
                bool nearA = false, nearB = false;
                for (int dy = -1; dy <= 1; ++dy)
                    for (int dx = -1; dx <= 1; ++dx) {
                        const int nx = x + dx, ny = y + dy;
                        if (nx >= 0 && ny >= 0 && nx < a.width() && ny < a.height()) {
                            nearA |= edge(a, nx, ny);
                            nearB |= edge(b, nx, ny);
                        }
                    }
                matchedA += ea && nearB;
                matchedB += eb && nearA;
            }
        }
    }
    const double precision = edgeB ? double(matchedB) / edgeB : 1.0;
    const double recall = edgeA ? double(matchedA) / edgeA : 1.0;
    const double edgeF1 = precision + recall > 0 ? 2 * precision * recall / (precision + recall) : 1.0;
    const double meanAbsoluteError = double(absoluteError) / (qint64(a.width()) * a.height() * 3);
    return {{"meanAbsoluteColorError", meanAbsoluteError},
            {"foregroundIntersectionOverUnion", unionCount ? double(intersection) / unionCount : 1.0},
            {"edgeF1Within1Px", edgeF1},
            {"cpuEdgePixels", double(edgeA)},
            {"gpuEdgePixels", double(edgeB)}};
}
} // namespace

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addOption({"width", "Canvas width", "pixels", "1280"});
    parser.addOption({"height", "Canvas height", "pixels", "720"});
    parser.addOption({"frames", "Measured frames", "count", "60"});
    parser.addOption({"nodes", "Shape count", "count", "120"});
    parser.addOption({"output", "Output directory", "path", "artifacts/renderer-prototype"});
    parser.addOption({"primitive-suite", "Render all vector primitive classes against a QPainter oracle"});
    parser.addOption({"primitive-kind", "Force a primitive class for the reference test", "name"});
    parser.addOption({"update-reference", "Write a QPainter golden for --primitive-kind"});
    parser.process(app);
    const int width = parser.value("width").toInt();
    const int height = parser.value("height").toInt();
    const int frameCount = parser.value("frames").toInt();
    const int nodes = parser.value("nodes").toInt();
    if (width <= 0 || height <= 0 || frameCount < 2 || nodes < 1)
        return 2;
    const QDir output(parser.value("output"));
    if (!output.exists() && !QDir().mkpath(output.path()))
        return 3;

    auto project = referenceProject(width, height, nodes);
    const auto forcedPrimitive = parser.isSet("primitive-kind")
                                     ? parsePrimitiveKind(parser.value("primitive-kind"))
                                     : std::optional<studio::RenderPrimitiveKind>{};
    if (parser.isSet("primitive-kind") && !forcedPrimitive)
        return 2;
    if (parser.isSet("update-reference") && !forcedPrimitive)
        return 2;
    const bool primitiveSuite = parser.isSet("primitive-suite") || forcedPrimitive.has_value();
    studio::CompositionPainter cpu;
    QElapsedTimer timer;
    QImage cpuImage;
    double cpuMilliseconds = 0;
    qint64 cpuWorkingSet = -1;
    if (!primitiveSuite) {
        for (int warmup = 0; warmup < 3; ++warmup)
            cpuImage = cpu.frame(project, warmup);
        timer.start();
        for (int i = 0; i < frameCount; ++i)
            cpuImage = cpu.frame(project, i % int(project.duration));
        cpuMilliseconds = double(timer.nsecsElapsed()) / 1e6 / frameCount;
        cpuWorkingSet = workingSetBytes();
    } else {
        timer.start();
        for (int i = 0; i < frameCount; ++i) {
            auto snapshot = studio::RenderSnapshot::build(project, i % int(project.duration));
            auto resolved = snapshot.primary();
            applyPrimitiveSuite(resolved, forcedPrimitive);
            cpuImage = paintPrimitiveReference(resolved, width, height);
        }
        cpuMilliseconds = double(timer.nsecsElapsed()) / 1e6 / frameCount;
        cpuWorkingSet = workingSetBytes();
    }
    cpuImage.save(output.filePath("cpu-reference.png"));
    bool qPainterReferenceRegressionPassed = true;
    QJsonObject qPainterReferenceMetrics;
    QString goldenPath;
    if (forcedPrimitive) {
        const auto goldenName = parser.value("primitive-kind") + ".png";
        goldenPath = QDir(QStringLiteral(STUDIO_PRIMITIVE_REFERENCE_DIR)).filePath(goldenName);
        if (parser.isSet("update-reference")) {
            if (!QDir().mkpath(QFileInfo(goldenPath).absolutePath()) || !cpuImage.save(goldenPath))
                return 8;
        } else {
            const QImage golden(goldenPath);
            qPainterReferenceRegressionPassed = !golden.isNull();
            if (qPainterReferenceRegressionPassed) {
                qPainterReferenceMetrics = imageMetrics(golden, cpuImage);
                qPainterReferenceRegressionPassed =
                    qPainterReferenceMetrics.value("foregroundIntersectionOverUnion").toDouble() >= .9999 &&
                    qPainterReferenceMetrics.value("edgeF1Within1Px").toDouble() >= .9999 &&
                    qPainterReferenceMetrics.value("meanAbsoluteColorError").toDouble() <= .01;
            }
        }
    }

    QQuickWindow window;
    window.setFlags(Qt::FramelessWindowHint);
    auto surfaceFormat = window.format();
    surfaceFormat.setSwapInterval(0);
    window.setFormat(surfaceFormat);
    window.setColor(QColor("#101820"));
    window.setGeometry(40, 40, width, height);
    auto initial = studio::RenderSnapshot::build(project, 0).primary();
    if (primitiveSuite)
        applyPrimitiveSuite(initial, forcedPrimitive);
    auto *item = new SceneGraphItem(std::move(initial), width, height);
    item->setParentItem(window.contentItem());
    item->setSize(QSizeF(width, height));
    QObject::connect(
        &window, &QQuickWindow::sceneGraphError, &app,
        [](QQuickWindow::SceneGraphError, const QString &message) { qCritical().noquote() << message; });
    window.show();
    for (int attempt = 0; attempt < 50 && !window.isExposed(); ++attempt) {
        QCoreApplication::processEvents();
        QThread::msleep(100);
    }
    if (!window.isExposed())
        return 4;
    for (int warmup = 0; warmup < 3; ++warmup) {
        QEventLoop loop;
        const auto connection =
            QObject::connect(&window, &QQuickWindow::frameSwapped, &loop, &QEventLoop::quit);
        auto snapshot = studio::RenderSnapshot::build(project, warmup).primary();
        if (primitiveSuite)
            applyPrimitiveSuite(snapshot, forcedPrimitive);
        item->setSnapshot(std::move(snapshot));
        QTimer::singleShot(5000, &loop, &QEventLoop::quit);
        loop.exec();
        QObject::disconnect(connection);
    }
    timer.restart();
    qint64 snapshotBuildNanos = 0;
    qint64 preparationBefore = item->preparationNanos();
    for (int i = 0; i < frameCount; ++i) {
        QEventLoop loop;
        const auto connection =
            QObject::connect(&window, &QQuickWindow::frameSwapped, &loop, &QEventLoop::quit);
        QElapsedTimer snapshotTimer;
        snapshotTimer.start();
        auto snapshot = studio::RenderSnapshot::build(project, i % int(project.duration)).primary();
        if (primitiveSuite)
            applyPrimitiveSuite(snapshot, forcedPrimitive);
        snapshotBuildNanos += snapshotTimer.nsecsElapsed();
        item->setSnapshot(std::move(snapshot));
        QTimer::singleShot(5000, &loop, &QEventLoop::quit);
        loop.exec();
        QObject::disconnect(connection);
    }
    const double sceneGraphMilliseconds = double(timer.nsecsElapsed()) / 1e6 / frameCount;
    const double snapshotBuildMilliseconds = double(snapshotBuildNanos) / 1e6 / frameCount;
    const double preparationMilliseconds =
        double(item->preparationNanos() - preparationBefore) / 1e6 / frameCount;
    QElapsedTimer captureTimer;
    captureTimer.start();
    const QImage gpuImage = window.grabWindow();
    const double captureMilliseconds = double(captureTimer.nsecsElapsed()) / 1e6;
    if (gpuImage.isNull())
        return 5;
    gpuImage.save(output.filePath("scenegraph-prototype.png"));
    const auto sceneGraphWorkingSet = workingSetBytes();
    const double gpuCaptureMilliseconds = captureMilliseconds;
    const auto compare = imageMetrics(cpuImage, gpuImage);
    differenceImage(cpuImage, gpuImage).save(output.filePath("difference-amplified.png"));
    const bool thinStrokeClass =
        forcedPrimitive && (*forcedPrimitive == studio::RenderPrimitiveKind::RectangleOutline ||
                            *forcedPrimitive == studio::RenderPrimitiveKind::Line);
    const double minimumForegroundIou = thinStrokeClass ? 0.74 : 0.94;
    const bool visualRegressionPassed =
        qPainterReferenceRegressionPassed &&
        compare.value("foregroundIntersectionOverUnion").toDouble() >= minimumForegroundIou &&
        compare.value("edgeF1Within1Px").toDouble() >= 0.95 &&
        compare.value("meanAbsoluteColorError").toDouble() <= 3.0;

    const auto api = apiName(window.rendererInterface()->graphicsApi());
    const auto framebufferBytes = qint64(width) * height * 4;
    const auto geometryBytes = item->geometryBytes();
    QJsonObject primitiveCoverage;
    int primitiveCounts[6]{};
    for (const auto &resolved : item->snapshot().nodes) {
        if (resolved.node.kind != studio::NodeKind::Shape)
            continue;
        ++primitiveCounts[int(resolved.primitive)];
    }
    const char *primitiveNames[] = {"roundedRectangle", "rectangle", "ellipse",
                                    "rectangleOutline", "line",      "convexPath"};
    for (int index = 0; index < 6; ++index)
        primitiveCoverage.insert(primitiveNames[index], primitiveCounts[index]);
    const double pixelsPerSecondCpu = (double(width) * height) / (cpuMilliseconds / 1000.0);
    const double pixelsPerSecondGpu = (double(width) * height) / (sceneGraphMilliseconds / 1000.0);
    const QJsonObject metrics{
        {"canvas", QJsonObject{{"width", width}, {"height", height}}},
        {"shapeCount", nodes},
        {"scenario", primitiveSuite ? "primitive-suite-qPainter-oracle" : "project-shape-snapshot"},
        {"supportedVectorPrimitives", primitiveCoverage},
        {"unsupportedSceneFeatures",
         QJsonArray{"text/typography", "native assets", "images", "SVG", "video",
                    "radial art-direction background", "shot transition compositing"}},
        {"frames", frameCount},
        {"sceneGraphApi", api},
        {"cpuPainterMsPerFrame", cpuMilliseconds},
        {"sceneGraphMsPerFrame", sceneGraphMilliseconds},
        {"snapshotBuildMsPerFrame", snapshotBuildMilliseconds},
        {"sceneGraphPreparationMsPerFrame", preparationMilliseconds},
        {"sceneGraphCaptureMs", gpuCaptureMilliseconds},
        {"sceneGraphFrameAfterSnapshotAndPreparationMsPerFrame",
         std::max(0.0, sceneGraphMilliseconds - snapshotBuildMilliseconds - preparationMilliseconds)},
        {"cpuPainterPixelsPerSecond", pixelsPerSecondCpu},
        {"sceneGraphPixelsPerSecond", pixelsPerSecondGpu},
        {"cpuWorkingSetBytes", cpuWorkingSet},
        {"sceneGraphWorkingSetBytes", sceneGraphWorkingSet},
        {"visualPsnrDb", psnr(cpuImage, gpuImage)},
        {"visualMetrics", compare},
        {"qPainterReference", goldenPath},
        {"qPainterReferenceMetrics", qPainterReferenceMetrics},
        {"qPainterReferenceRegressionPassed", qPainterReferenceRegressionPassed},
        {"visualRegressionCriteria",
         QJsonObject{{"foregroundIntersectionOverUnionMin", minimumForegroundIou},
                     {"edgeF1Within1PxMin", 0.95},
                     {"meanAbsoluteColorErrorMax", 3.0},
                     {"thinStrokeIoUAllowance",
                      "Raster edge coverage over a 3-pixel outline is sensitive to subpixel center "
                      "placement; edge F1 and color MAE provide stricter companion checks."}}},
        {"visualRegressionPassed", visualRegressionPassed},
        {"gpuDedicatedMemoryMeasured", false},
        {"cpuRgbaFrameBytes", double(framebufferBytes)},
        {"sceneGraphGeometryBytesEstimate", double(geometryBytes)},
        {"sceneGraphFramebufferBytesEstimate", double(framebufferBytes)},
        {"cpuImage", "cpu-reference.png"},
        {"sceneGraphImage", "scenegraph-prototype.png"}};
    const auto metricsJson = QJsonDocument(metrics).toJson(QJsonDocument::Indented);
    QFile metricsFile(output.filePath("metrics.json"));
    if (!metricsFile.open(QIODevice::WriteOnly) || metricsFile.write(metricsJson) != metricsJson.size())
        return 7;
    QTextStream(stdout) << metricsJson << Qt::endl;
    window.close();
    return visualRegressionPassed ? 0 : 6;
}
