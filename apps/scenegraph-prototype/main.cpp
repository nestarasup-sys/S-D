#include <QColor>
#include <QCommandLineParser>
#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QCryptographicHash>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QImage>
#include <QImageReader>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QPainter>
#include <QQuickItem>
#include <QQuickWindow>
#include <QRunnable>
#include <QSGGeometry>
#include <QSGGeometryNode>
#include <QSGOpacityNode>
#include <QSGRendererInterface>
#include <QSGSimpleTextureNode>
#include <QSGTextureMaterial>
#include <QSGTransformNode>
#include <QSGVertexColorMaterial>
#include <QSurfaceFormat>
#include <QTextStream>
#include <QThread>
#include <QTimer>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <functional>
#include <list>
#include <unordered_map>
#include <unordered_set>
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
#include <studio/core/library.hpp>
#include <studio/core/project.hpp>

#ifndef STUDIO_PRIMITIVE_REFERENCE_DIR
#define STUDIO_PRIMITIVE_REFERENCE_DIR "tests/reference/render-primitives"
#endif
#ifndef STUDIO_HYBRID_REFERENCE_DIR
#define STUDIO_HYBRID_REFERENCE_DIR "tests/reference/render-hybrid"
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

studio::Project hybridProject(int width, int height, const QString &fixtureDirectory, bool nativeAtlas,
                               bool staticScene = false, bool *webpAlphaOutput = nullptr) {
    QDir().mkpath(fixtureDirectory);
    auto project = studio::makeEmpty();
    project.id = nativeAtlas ? "scenegraph-native-atlas" : "scenegraph-hybrid-suite";
    project.name = project.id;
    project.width = width;
    project.height = height;
    project.duration = 120;
    project.artDirection.backgroundColor = "#101820";
    project.artDirection.glowColor = "#23404b";
    project.artDirection.fontFamily = "Segoe UI";
    studio::Node root;
    root.id = "animated-group";
    root.kind = studio::NodeKind::Group;
    root.name = "Hybrid test group";
    root.start = 0;
    root.duration = project.duration;
    root.x = 0;
    root.y = 0;
    root.width = width;
    root.height = height;
    root.opacity = .92;
    if (!staticScene)
        root.keyframes = {{0, 0, 0, 0, 1, .92, "ease-in-out"},
                          {60, width * .012, height * .008, 1.5, 1.015, .92, "ease-in-out"},
                          {119, 0, 0, 0, 1, .92, "linear"}};
    project.nodes.push_back(root);
    if (!staticScene)
        project.camera = {{0, width / 2.0, height / 2.0, 1, 0, "ease-in-out", {}},
                          {60, width / 2.0 + width * .008, height / 2.0, 1.015, .35, "ease-in-out", {}},
                          {119, width / 2.0, height / 2.0, 1, 0, "linear", {}}};

    if (nativeAtlas) {
        const auto &assets = studio::nativeLibrary();
        const int columns = 9;
        const int rows = int((assets.size() + columns - 1) / columns);
        const double cellWidth = double(width) / columns;
        const double cellHeight = double(height) / rows;
        for (std::size_t index = 0; index < assets.size(); ++index) {
            const auto &asset = assets[index];
            studio::Node node;
            node.id = "native-" + asset.id;
            node.parentId = root.id;
            node.kind = studio::NodeKind::Native;
            node.nativeId = asset.id;
            node.name = asset.name;
            node.start = 0;
            node.duration = project.duration;
            node.x = double(index % columns) * cellWidth + 3;
            node.y = double(index / columns) * cellHeight + 3;
            node.width = cellWidth - 6;
            node.height = cellHeight - 6;
            node.color = index % 2 ? "#b4f078" : "#efb873";
            node.secondaryColor = "#f3c49a";
            node.pose = asset.id == "person" ? "wave" : "neutral";
            node.animated = false;
            project.nodes.push_back(node);
        }
        return project;
    }

    studio::Node shape;
    shape.id = "gpu-shape";
    shape.parentId = root.id;
    shape.kind = studio::NodeKind::Shape;
    shape.name = "GPU vector behind text";
    shape.start = 0;
    shape.duration = project.duration;
    shape.x = width * .055;
    shape.y = height * .07;
    shape.width = width * .35;
    shape.height = height * .13;
    shape.color = "#284f56";
    shape.animated = false;
    project.nodes.push_back(shape);

    studio::Node title;
    title.id = "hybrid-title";
    title.parentId = root.id;
    title.kind = studio::NodeKind::Text;
    title.name = "Large wrapped title";
    title.text = "PREÇO DOS CARROS\nEM ALTA";
    title.color = "#f0f4e9";
    title.start = 0;
    title.duration = project.duration;
    title.x = width * .07;
    title.y = height * .075;
    title.width = width * .62;
    title.height = height * .3;
    title.fontSize = std::max(40.0, width * .045);
    title.animated = false;
    project.nodes.push_back(title);

    studio::Node native;
    native.id = "hybrid-native-person";
    native.parentId = root.id;
    native.kind = studio::NodeKind::Native;
    native.nativeId = "person";
    native.pose = staticScene ? "neutral" : "wave";
    native.name = "Animated native character";
    native.start = 0;
    native.duration = project.duration;
    native.x = width * .72;
    native.y = height * .18;
    native.width = width * .16;
    native.height = height * .42;
    native.color = "#c77759";
    native.secondaryColor = "#f3c49a";
    native.animated = false;
    project.nodes.push_back(native);

    studio::Node caption;
    caption.id = "hybrid-caption";
    caption.parentId = root.id;
    caption.kind = studio::NodeKind::Text;
    caption.name = "Word timed caption";
    caption.role = "caption";
    caption.text = "Os carros estão caros";
    caption.color = "#f2f7ec";
    caption.start = 0;
    caption.duration = project.duration;
    caption.x = width * .08;
    caption.y = height * .73;
    caption.width = width * .72;
    caption.height = height * .12;
    caption.fontSize = std::max(22.0, width * .022);
    if (!staticScene)
        caption.captionWords = {{"Os", 0, 15}, {"carros", 15, 33}, {"estão", 33, 50}, {"caros", 50, 75}};
    caption.wordAnimationPreset = staticScene ? "none" : "pop-rise";
    caption.wordAnimationDurationFrames = 8;
    caption.animated = false;
    project.nodes.push_back(caption);

    QImage transparent(144, 96, QImage::Format_ARGB32_Premultiplied);
    transparent.fill(Qt::transparent);
    {
        QPainter painter(&transparent);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setBrush(QColor("#b4f078"));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(QRectF(5, 5, 82, 82));
        painter.setBrush(QColor(239, 184, 115, 190));
        painter.drawRoundedRect(QRectF(62, 25, 78, 62), 10, 10);
    }
    transparent.save(QDir(fixtureDirectory).filePath("transparent.png"));
    QImage opaque(144, 96, QImage::Format_RGB32);
    opaque.fill(QColor("#263d52"));
    {
        QPainter painter(&opaque);
        painter.setPen(QPen(QColor("#e8bb72"), 6));
        painter.drawLine(10, 76, 130, 20);
    }
    opaque.save(QDir(fixtureDirectory).filePath("photo.jpg"), "JPEG", 90);
    QImage webp(144, 96, QImage::Format_ARGB32_Premultiplied);
    webp.fill(Qt::transparent);
    QPainter webpPainter(&webp);
    webpPainter.setPen(Qt::NoPen);
    webpPainter.setBrush(QColor("#b4f078"));
    webpPainter.drawEllipse(QRectF(30, 12, 82, 72));
    webpPainter.setBrush(QColor(239, 184, 115, 210));
    webpPainter.drawRoundedRect(QRectF(82, 42, 50, 42), 9, 9);
    webpPainter.end();
    const auto webpPath = QDir(fixtureDirectory).filePath("photo.webp");
    const bool webpFixtureWritten = webp.save(webpPath, "WEBP", 90);
    QImageReader webpFixtureReader(webpPath);
    const bool webpFixtureUsable = webpFixtureWritten && webpFixtureReader.canRead();
    const auto decodedWebp = webpFixtureUsable ? webpFixtureReader.read() : QImage{};
    const bool webpAlphaOk = webpFixtureUsable && decodedWebp.pixelColor(2, 2).alpha() == 0 &&
                             decodedWebp.pixelColor(50, 40).alpha() >= 240;
    if (webpAlphaOutput)
        *webpAlphaOutput = webpAlphaOk;
    QFile svgFile(QDir(fixtureDirectory).filePath("mark.svg"));
    if (svgFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
        svgFile.write("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"144\" height=\"96\"><rect x=\"3\" y=\"3\" width=\"138\" height=\"90\" rx=\"15\" fill=\"#284f56\"/><path d=\"M18 70 L55 20 L80 55 L105 30 L128 72 Z\" fill=\"#b4f078\"/><circle cx=\"112\" cy=\"27\" r=\"8\" fill=\"#efb873\"/></svg>");

    const auto addMedia = [&](const std::string &id, studio::NodeKind kind, const QString &source,
                              double x, double y) {
        studio::Node node;
        node.id = id;
        node.parentId = root.id;
        node.kind = kind;
        node.name = id;
        node.source = source.toStdString();
        node.start = 0;
        node.duration = project.duration;
        node.x = x;
        node.y = y;
        node.width = width * .16;
        node.height = height * .14;
        node.animated = false;
        project.nodes.push_back(node);
    };
    addMedia("hybrid-png", studio::NodeKind::Image, QDir(fixtureDirectory).filePath("transparent.png"),
             width * .08, height * .48);
    addMedia("hybrid-jpeg", studio::NodeKind::Image, QDir(fixtureDirectory).filePath("photo.jpg"),
             width * .27, height * .48);
    if (webpFixtureUsable)
        addMedia("hybrid-webp", studio::NodeKind::Image, webpPath, width * .46, height * .48);
    addMedia("hybrid-svg", studio::NodeKind::Svg, QDir(fixtureDirectory).filePath("mark.svg"),
             width * .65, height * .48);
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
    SceneGraphItem(studio::Project project, studio::RenderFrameSnapshot snapshot, int canvasWidth, int canvasHeight,
                   qsizetype textureBudgetBytes = 64 * 1024 * 1024)
        : project_(std::move(project)), snapshot_(std::move(snapshot)), canvasWidth_(canvasWidth),
          canvasHeight_(canvasHeight), textureCacheBudgetBytes_(std::max<qsizetype>(1024, textureBudgetBytes)) {
        setFlag(ItemHasContents);
        setClip(true);
    }
    void setSnapshot(studio::RenderFrameSnapshot snapshot, std::optional<studio::Project> project = std::nullopt) {
        snapshot_ = std::move(snapshot);
        if (project)
            project_ = std::move(*project);
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
    qint64 rasterizationNanos() const { return rasterizationNanos_.load(); }
    qint64 textureUploadNanos() const { return textureUploadNanos_.load(); }
    qint64 textureCacheBytes() const { return qint64(textureCacheBytes_.load()); }
    qint64 peakTextureCacheBytes() const { return qint64(peakTextureCacheBytes_.load()); }
    qint64 textureCacheBudget() const { return qint64(textureCacheBudgetBytes_); }
    qint64 textureCacheEvictions() const { return textureCacheEvictions_.load(); }
    qint64 textureCacheHits() const { return textureCacheHits_.load(); }
    qint64 textureCacheMisses() const { return textureCacheMisses_.load(); }
    qint64 textureUploads() const { return textureUploads_.load(); }
    qint64 rasterizedLayerCount() const { return rasterizedLayerCount_.load(); }
    qint64 transparentLayerCount() const { return transparentLayerCount_.load(); }
    qint64 unsupportedLayerCount() const { return unsupportedLayerCount_.load(); }
    qint64 nativeAssetCount() const { return nativeAssetCount_.load(); }
    bool textureCacheWithinBudget() const { return textureCacheBytes() <= textureCacheBudgetBytes_; }
    bool cacheInvalidationSelfTest() const {
        const studio::RenderNodeSnapshot *staticLayer = nullptr;
        const studio::RenderNodeSnapshot *animatedNative = nullptr;
        const studio::RenderNodeSnapshot *animatedCaption = nullptr;
        for (const auto &resolved : snapshot_.nodes) {
            const auto &node = resolved.node;
            const bool isAnimatedNative = node.kind == studio::NodeKind::Native &&
                                          ((node.nativeId == "person" && node.pose == "wave") ||
                                           node.nativeId == "bars");
            if (!staticLayer && node.kind != studio::NodeKind::Group && node.kind != studio::NodeKind::Shape &&
                node.kind != studio::NodeKind::Video && !isAnimatedNative)
                staticLayer = &resolved;
            if (!animatedNative && node.kind == studio::NodeKind::Native && isAnimatedNative)
                animatedNative = &resolved;
            if (!animatedCaption && node.kind == studio::NodeKind::Text && node.role == "caption" &&
                !node.captionWords.empty())
                animatedCaption = &resolved;
        }
        if (!staticLayer)
            return false;
        auto changed = *staticLayer;
        changed.node.color = changed.node.color == "#ffffff" ? "#000000" : "#ffffff";
        const bool staticStable = rasterKey(*staticLayer, snapshot_.frame) ==
                                  rasterKey(*staticLayer, snapshot_.frame + 1);
        const bool contentInvalidates = rasterKey(*staticLayer, snapshot_.frame) !=
                                        rasterKey(changed, snapshot_.frame);
        const bool nativeAnimationInvalidates = !animatedNative ||
                                                rasterKey(*animatedNative, snapshot_.frame) !=
                                                    rasterKey(*animatedNative, snapshot_.frame + 1);
        const bool wordAnimationInvalidates = !animatedCaption ||
                                              rasterKey(*animatedCaption, snapshot_.frame) !=
                                                  rasterKey(*animatedCaption, snapshot_.frame + 1);
        return staticStable && contentInvalidates && nativeAnimationInvalidates && wordAnimationInvalidates;
    }

    void clearTextureCacheOnRenderThread() {
        for (auto entry = textures_.cbegin(); entry != textures_.cend(); ++entry) {
            delete entry.value().texture;
        }
        textures_.clear();
        textureLru_.clear();
        textureCacheBytes_.store(0);
    }

    void releaseResources() override {
        if (textures_.empty())
            return;
        std::vector<QSGTexture *> pending;
        pending.reserve(textures_.size());
        for (auto entry = textures_.cbegin(); entry != textures_.cend(); ++entry)
            pending.push_back(entry.value().texture);
        textures_.clear();
        textureLru_.clear();
        textureCacheBytes_.store(0);
        if (auto *quickWindow = window())
            quickWindow->scheduleRenderJob(new TextureReleaseJob(std::move(pending)),
                                           QQuickWindow::AfterRenderingStage);
    }

    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) override {
        QElapsedTimer preparationTimer;
        preparationTimer.start();
        delete oldNode;
        activeTextureKeys_.clear();
        unsupportedLayerCount_.store(0);
        // Qt Quick's D3D11 path uses 16-bit geometry indices. Keep each draw
        // batch below that limit; a single oversized geometry silently loses
        // later triangles on some Qt/driver combinations.
        constexpr std::size_t maxVerticesPerBatch = 60000;
        std::vector<Vertex> shapeBatch;
        const auto &frame = snapshot_;
        const double sx = width() / canvasWidth_;
        const double sy = height() / canvasHeight_;
        auto *root = new QSGNode;
        appendTexture(root, QStringLiteral("background:") + artKey(frame.backgroundArt),
                      [&] { return rasterizer_.rasterBackgroundLayer(project_, frame.backgroundArt); },
                      QRectF(0, 0, project_.width, project_.height), QTransform{}, 1.0, sx, sy);

        const auto flushShapes = [&] {
            if (shapeBatch.empty())
                return;
            auto *node = new QSGGeometryNode;
            auto *geometry = new QSGGeometry(QSGGeometry::defaultAttributes_ColoredPoint2D(),
                                             int(shapeBatch.size()));
            geometry->setDrawingMode(QSGGeometry::DrawTriangles);
            geometry->setVertexDataPattern(QSGGeometry::StaticPattern);
            auto *destination = geometry->vertexDataAsColoredPoint2D();
            for (std::size_t index = 0; index < shapeBatch.size(); ++index) {
                const auto &vertex = shapeBatch[index];
                destination[index].set(vertex.x, vertex.y, vertex.r, vertex.g, vertex.b, vertex.a);
            }
            node->setGeometry(geometry);
            node->setFlag(QSGNode::OwnsGeometry);
            auto *material = new QSGVertexColorMaterial;
            material->setFlag(QSGMaterial::Blending, true);
            node->setMaterial(material);
            node->setFlag(QSGNode::OwnsMaterial);
            root->appendChildNode(node);
            geometryBytes_.fetch_add(qint64(shapeBatch.size() * sizeof(Vertex)));
            shapeBatch.clear();
        };
        geometryBytes_.store(0);
        std::unordered_set<std::string> nativeIds;
        for (const auto &resolved : frame.nodes) {
            const auto &node = resolved.node;
            if (!resolved.visible || resolved.inheritedOpacity <= 0 || node.kind == studio::NodeKind::Group)
                continue;
            if (node.kind == studio::NodeKind::Shape) {
                const QColor color(QString::fromStdString(node.color));
                const auto localPoints = primitivePoints(resolved);
                std::vector<QPointF> points;
                points.reserve(localPoints.size());
                for (const auto &point : localPoints) {
                    const auto mapped = frame.cameraTransform.map(resolved.sceneTransform.map(point));
                    points.emplace_back(mapped.x() * sx, mapped.y() * sy);
                }
                std::vector<Vertex> shapeVertices;
                if (resolved.primitive == studio::RenderPrimitiveKind::Line)
                    appendStroke(shapeVertices, points, color, resolved.inheritedOpacity,
                                 resolved.strokeWidth * sx);
                else if (resolved.primitive == studio::RenderPrimitiveKind::RectangleOutline)
                    appendStroke(shapeVertices, points, color, resolved.inheritedOpacity,
                                 resolved.strokeWidth * sx, true);
                else if (resolved.filled)
                    appendPolygon(shapeVertices, points, color, resolved.inheritedOpacity);
                else
                    appendStroke(shapeVertices, points, color, resolved.inheritedOpacity,
                                 resolved.strokeWidth * sx, true);
                if (!shapeBatch.empty() && shapeBatch.size() + shapeVertices.size() > maxVerticesPerBatch)
                    flushShapes();
                shapeBatch.insert(shapeBatch.end(), shapeVertices.begin(), shapeVertices.end());
                continue;
            }

            flushShapes();
            if (node.kind == studio::NodeKind::Video) {
                unsupportedLayerCount_.fetch_add(1);
                continue;
            }
            if (node.kind == studio::NodeKind::Native)
                nativeIds.insert(node.nativeId);
            const auto key = rasterKey(resolved, frame.frame);
            // QTransform composes in execution order: local scene transform, then camera.
            const auto composed = resolved.sceneTransform * frame.cameraTransform;
            appendTexture(root, key, [&] { return rasterizer_.rasterNodeLayer(project_, frame, resolved, 2.0); },
                          QRectF(0, 0, node.width, node.height), composed,
                          resolved.inheritedOpacity, sx, sy);
        }
        flushShapes();
        nativeAssetCount_.store(qint64(nativeIds.size()));
        preparationNanos_.fetch_add(preparationTimer.nsecsElapsed());
        return root;
    }

  private:
    struct CachedTexture {
        QSGTexture *texture{nullptr};
        qsizetype bytes{0};
        std::list<QString>::iterator lru;
    };
    class TextureReleaseJob final : public QRunnable {
      public:
        explicit TextureReleaseJob(std::vector<QSGTexture *> textures) : textures_(std::move(textures)) {}
        void run() override {
            for (auto *texture : textures_)
                delete texture;
        }
      private:
        std::vector<QSGTexture *> textures_;
    };

    static QString artKey(const studio::ArtDirection &art) {
        return QString::fromLatin1(QCryptographicHash::hash(
            QStringList{QString::fromStdString(art.backgroundColor), QString::fromStdString(art.glowColor),
                        QString::fromStdString(art.primaryColor), QString::fromStdString(art.secondaryColor),
                        QString::fromStdString(art.textColor), QString::fromStdString(art.cardColor),
                        QString::fromStdString(art.captionBackgroundColor),
                        QString::fromStdString(art.captionHighlightColor)}
                .join(QChar(0)).toUtf8(), QCryptographicHash::Sha256).toHex());
    }

    static QString rasterKey(const studio::RenderNodeSnapshot &resolved, std::int64_t frame) {
        const auto &node = resolved.node;
        QStringList parts{QString::fromStdString(node.id), QString::number(int(node.kind)),
                          QString::fromStdString(node.text), QString::fromStdString(node.color),
                          QString::fromStdString(node.secondaryColor), QString::fromStdString(node.source),
                          QString::fromStdString(node.nativeId), QString::fromStdString(node.pose),
                          QString::fromStdString(node.role), QString::fromStdString(node.wordAnimationPreset),
                          QString::fromStdString(resolved.artDirection.fontFamily),
                          QString::fromStdString(resolved.artDirection.textColor),
                          QString::fromStdString(resolved.artDirection.captionBackgroundColor),
                          QString::fromStdString(resolved.artDirection.captionHighlightColor),
                          QString::number(node.width, 'f', 4), QString::number(node.height, 'f', 4),
                          QString::number(node.fontSize, 'f', 4), QString::number(node.wordAnimationDurationFrames),
                          QString::number(node.start),
                          QString::number(node.wordAnimationStartScale, 'f', 4),
                          QString::number(node.wordAnimationTravelY, 'f', 4)};
        for (const auto &cue : node.captionWords)
            parts << QString::fromStdString(cue.text) << QString::number(cue.start) << QString::number(cue.end);
        if (node.kind == studio::NodeKind::Image || node.kind == studio::NodeKind::Svg) {
            const QFileInfo info(QString::fromStdString(node.source));
            parts << QString::number(info.size()) << QString::number(info.lastModified().toMSecsSinceEpoch());
        }
        const bool animatedCaption = node.kind == studio::NodeKind::Text && node.role == "caption" &&
                                     !node.captionWords.empty() && node.wordAnimationPreset != "none";
        const bool animatedNative = node.kind == studio::NodeKind::Native &&
                                    ((node.nativeId == "person" && node.pose == "wave") ||
                                     node.nativeId == "bars");
        if (animatedCaption || animatedNative)
            parts << QString::number(frame);
        return QString::fromLatin1(QCryptographicHash::hash(parts.join(QChar(0)).toUtf8(),
                                                             QCryptographicHash::Sha256).toHex());
    }

    void appendTexture(QSGNode *root, const QString &key, const std::function<QImage()> &rasterize,
                       const QRectF &localRect,
                       const QTransform &transform, double opacity, double sx, double sy) {
        bool isCached = false;
        auto *texture = textureFor(key, rasterize, isCached);
        if (!texture)
            return;
        auto *opacityNode = new QSGOpacityNode;
        opacityNode->setOpacity(float(std::clamp(opacity, 0.0, 1.0)));
        auto *transformNode = new QSGTransformNode;
        QMatrix4x4 matrix;
        matrix.scale(float(sx), float(sy));
        matrix *= QMatrix4x4(transform);
        transformNode->setMatrix(matrix);
        auto *textureNode = new QSGSimpleTextureNode;
        textureNode->setTexture(texture);
        textureNode->setOwnsTexture(!isCached);
        textureNode->setFiltering(QSGTexture::Linear);
        textureNode->setRect(localRect);
        transformNode->appendChildNode(textureNode);
        opacityNode->appendChildNode(transformNode);
        root->appendChildNode(opacityNode);
    }

    QSGTexture *textureFor(const QString &key, const std::function<QImage()> &rasterize, bool &isCached) {
        if (auto found = textures_.find(key); found != textures_.end()) {
            auto &entry = found.value();
            textureLru_.splice(textureLru_.begin(), textureLru_, entry.lru);
            entry.lru = textureLru_.begin();
            activeTextureKeys_.insert(key);
            isCached = true;
            textureCacheHits_.fetch_add(1);
            return entry.texture;
        }
        textureCacheMisses_.fetch_add(1);
        if (!window())
            return nullptr;
        QElapsedTimer rasterTimer;
        rasterTimer.start();
        const auto source = rasterize();
        rasterizationNanos_.fetch_add(rasterTimer.nsecsElapsed());
        if (source.isNull()) {
            unsupportedLayerCount_.fetch_add(1);
            return nullptr;
        }
        rasterizedLayerCount_.fetch_add(1);
        bool hasTransparentSamples = false;
        const int stepX = std::max(1, source.width() / 24);
        const int stepY = std::max(1, source.height() / 24);
        for (int y = 0; y < source.height() && !hasTransparentSamples; y += stepY)
            for (int x = 0; x < source.width(); x += stepX)
                if (qAlpha(source.pixel(x, y)) < 250) {
                    hasTransparentSamples = true;
                    break;
                }
        if (hasTransparentSamples)
            transparentLayerCount_.fetch_add(1);
        QElapsedTimer uploadTimer;
        uploadTimer.start();
        auto *texture = window()->createTextureFromImage(source);
        textureUploadNanos_.fetch_add(uploadTimer.nsecsElapsed());
        if (!texture)
            return nullptr;
        textureUploads_.fetch_add(1);
        const qsizetype bytes = qsizetype(source.width()) * source.height() * 4;
        if (bytes > textureCacheBudgetBytes_) {
            isCached = false;
            return texture;
        }
        while (textureCacheBytes_.load() + bytes > textureCacheBudgetBytes_) {
            auto candidate = textureLru_.rbegin();
            while (candidate != textureLru_.rend() && activeTextureKeys_.contains(*candidate))
                ++candidate;
            if (candidate == textureLru_.rend()) {
                isCached = false;
                return texture;
            }
            const auto oldKey = *candidate;
            const auto old = textures_.find(oldKey);
            if (old != textures_.end()) {
                textureCacheBytes_.fetch_sub(old.value().bytes);
                delete old.value().texture;
                textures_.remove(oldKey);
                textureCacheEvictions_.fetch_add(1);
            }
            textureLru_.erase(std::next(candidate).base());
        }
        textureLru_.push_front(key);
        textures_.insert(key, CachedTexture{texture, bytes, textureLru_.begin()});
        activeTextureKeys_.insert(key);
        const auto current = textureCacheBytes_.fetch_add(bytes) + bytes;
        auto peak = peakTextureCacheBytes_.load();
        while (current > peak && !peakTextureCacheBytes_.compare_exchange_weak(peak, current)) {}
        isCached = true;
        return texture;
    }

    studio::Project project_;
    studio::RenderFrameSnapshot snapshot_;
    int canvasWidth_{0}, canvasHeight_{0};
    qsizetype textureCacheBudgetBytes_{64 * 1024 * 1024};
    studio::CompositionPainter rasterizer_;
    QHash<QString, CachedTexture> textures_;
    std::list<QString> textureLru_;
    QSet<QString> activeTextureKeys_;
    std::atomic<qint64> preparationNanos_{0};
    std::atomic<qint64> geometryBytes_{0};
    std::atomic<qint64> rasterizationNanos_{0}, textureUploadNanos_{0};
    std::atomic<qint64> textureCacheBytes_{0}, peakTextureCacheBytes_{0};
    std::atomic<qint64> textureCacheHits_{0}, textureCacheMisses_{0}, textureUploads_{0};
    std::atomic<qint64> textureCacheEvictions_{0};
    std::atomic<qint64> rasterizedLayerCount_{0}, transparentLayerCount_{0}, unsupportedLayerCount_{0},
        nativeAssetCount_{0};
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
                {"meanAbsoluteAlphaError", 255.0},
                {"foregroundIntersectionOverUnion", 0.0},
                {"edgeF1Within1Px", 0.0},
                {"cpuEdgePixels", 0.0},
                {"gpuEdgePixels", 0.0},
                {"sizeMismatch", true}};
    const auto a = left.convertToFormat(QImage::Format_ARGB32);
    const auto b = right.convertToFormat(QImage::Format_ARGB32);
    const QColor background("#101820");
    quint64 absoluteError = 0, absoluteAlphaError = 0, intersection = 0, unionCount = 0;
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
            absoluteAlphaError += std::abs(qAlpha(lineA[x]) - qAlpha(lineB[x]));
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
    const double meanAbsoluteAlphaError = double(absoluteAlphaError) / (qint64(a.width()) * a.height());
    return {{"meanAbsoluteColorError", meanAbsoluteError},
            {"meanAbsoluteAlphaError", meanAbsoluteAlphaError},
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
    parser.addOption({"hybrid-suite", "Render the mixed text/native/image/SVG regression scene"});
    parser.addOption({"static-scene", "Use fixed text/native content and no animated camera for the hybrid benchmark"});
    parser.addOption({"native-atlas", "Render every native asset as a QPainter-backed GPU texture"});
    parser.addOption({"cache-pressure-test", "Use a 4 MiB texture cache to verify eviction and memory bounds"});
    parser.addOption({"font-file", "Register an external font file in QFontDatabase for this prototype run", "path"});
    parser.addOption({"update-hybrid-reference", "Write the QPainter hybrid golden image"});
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

    const bool hybridSuite = parser.isSet("hybrid-suite");
    const bool nativeAtlas = parser.isSet("native-atlas");
    const bool cachePressureTest = parser.isSet("cache-pressure-test");
    const bool staticScene = parser.isSet("static-scene");
    if ((hybridSuite && nativeAtlas) || (cachePressureTest && !hybridSuite) || (staticScene && !hybridSuite))
        return 2;
    const auto fixtureDirectory = output.filePath("fixtures");
    bool webpAlphaWasPreserved = false;
    auto project = (hybridSuite || nativeAtlas)
                       ? hybridProject(width, height, fixtureDirectory, nativeAtlas, staticScene,
                                       &webpAlphaWasPreserved)
                       : referenceProject(width, height, nodes);
    QString registeredFontFamily;
    if (parser.isSet("font-file")) {
        const int fontId = QFontDatabase::addApplicationFont(parser.value("font-file"));
        const auto families = QFontDatabase::applicationFontFamilies(fontId);
        if (fontId < 0 || families.isEmpty())
            return 9;
        registeredFontFamily = families.front();
        project.artDirection.fontFamily = registeredFontFamily.toStdString();
    }
    const auto forcedPrimitive = parser.isSet("primitive-kind")
                                     ? parsePrimitiveKind(parser.value("primitive-kind"))
                                     : std::optional<studio::RenderPrimitiveKind>{};
    if (parser.isSet("primitive-kind") && !forcedPrimitive)
        return 2;
    if (parser.isSet("update-reference") && !forcedPrimitive)
        return 2;
    const bool primitiveSuite = parser.isSet("primitive-suite") || forcedPrimitive.has_value();
    if ((hybridSuite || nativeAtlas) && primitiveSuite)
        return 2;
    const auto fixtureCanRead = [](const QString &path) {
        QImageReader reader(path);
        return reader.canRead();
    };
    const bool pngFixtureReadable = !hybridSuite || fixtureCanRead(QDir(fixtureDirectory).filePath("transparent.png"));
    const bool jpegFixtureReadable = !hybridSuite || fixtureCanRead(QDir(fixtureDirectory).filePath("photo.jpg"));
    const auto supportedFormats = QImageReader::supportedImageFormats();
    const bool webpAdvertised = std::any_of(supportedFormats.begin(), supportedFormats.end(),
                                            [](const QByteArray &format) {
                                                return format.toLower() == QByteArrayLiteral("webp");
                                            });
    const bool webpFixtureReadable = !hybridSuite ||
                                     fixtureCanRead(QDir(fixtureDirectory).filePath("photo.webp"));
    const bool webpAvailable = webpAdvertised || (hybridSuite && webpFixtureReadable);
    const bool svgFixtureReadable = !hybridSuite ||
                                    QSvgRenderer(QDir(fixtureDirectory).filePath("mark.svg")).isValid();
    const bool webpAlphaTested = hybridSuite && QFileInfo::exists(QDir(fixtureDirectory).filePath("photo.webp"));
    const bool webpAlphaPassed = !hybridSuite || !webpAvailable ||
                                 (webpAlphaTested && webpAlphaWasPreserved);
    const bool mediaFormatTestsPassed = !hybridSuite || (pngFixtureReadable && jpegFixtureReadable &&
                                        (!webpAvailable || webpFixtureReadable) && webpAlphaPassed &&
                                        svgFixtureReadable);
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
    if (parser.isSet("update-hybrid-reference") && !hybridSuite && !nativeAtlas)
        return 2;
    if (parser.isSet("update-hybrid-reference")) {
        if (width != 1280 || height != 720)
            return 2;
        const QString goldenName = nativeAtlas ? "native-atlas.png" : staticScene ? "hybrid-static.png" : "hybrid.png";
        const auto goldenPath = QDir(QStringLiteral(STUDIO_HYBRID_REFERENCE_DIR)).filePath(goldenName);
        if (!QDir().mkpath(QFileInfo(goldenPath).absolutePath()) || !cpuImage.save(goldenPath))
            return 8;
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
    const qsizetype textureBudget = cachePressureTest ? 4 * 1024 * 1024 : 64 * 1024 * 1024;
    auto *item = new SceneGraphItem(project, std::move(initial), width, height, textureBudget);
    item->setParentItem(window.contentItem());
    item->setSize(QSizeF(width, height));
    QObject::connect(&window, &QQuickWindow::sceneGraphInvalidated, item,
                     [item] { item->clearTextureCacheOnRenderThread(); }, Qt::DirectConnection);
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
    const qint64 rasterizationBefore = item->rasterizationNanos();
    const qint64 textureUploadBefore = item->textureUploadNanos();
    const qint64 textureUploadsBefore = item->textureUploads();
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
    QString hybridGoldenPath;
    if (hybridSuite || nativeAtlas) {
        if (width == 1280 && height == 720) {
            const QString goldenName =
                nativeAtlas ? "native-atlas.png" : staticScene ? "hybrid-static.png" : "hybrid.png";
            hybridGoldenPath = QDir(QStringLiteral(STUDIO_HYBRID_REFERENCE_DIR)).filePath(goldenName);
        }
        if (!parser.isSet("update-hybrid-reference") && !hybridGoldenPath.isEmpty() &&
            registeredFontFamily.isEmpty()) {
            const QImage golden(hybridGoldenPath);
            qPainterReferenceRegressionPassed = !golden.isNull();
            if (qPainterReferenceRegressionPassed) {
                const auto goldenMetrics = imageMetrics(golden, cpuImage);
                qPainterReferenceMetrics = goldenMetrics;
                qPainterReferenceRegressionPassed =
                    goldenMetrics.value("meanAbsoluteColorError").toDouble() <= 0.1 &&
                    goldenMetrics.value("foregroundIntersectionOverUnion").toDouble() >= .999;
            }
        }
    }
    const bool thinStrokeClass =
        forcedPrimitive && (*forcedPrimitive == studio::RenderPrimitiveKind::RectangleOutline ||
                            *forcedPrimitive == studio::RenderPrimitiveKind::Line);
    const double minimumForegroundIou = thinStrokeClass ? 0.74 : 0.94;
    const double minimumEdgeF1 = nativeAtlas ? 0.91 : 0.95;
    const bool nativeAtlasComplete = !nativeAtlas || item->nativeAssetCount() == qint64(studio::nativeLibrary().size());
    const bool cacheTestsPassed =
        (!hybridSuite && !nativeAtlas) ||
        (item->textureCacheWithinBudget() && item->textureCacheMisses() > 0 && item->textureUploads() > 0 &&
         (frameCount < 2 || item->textureCacheHits() > 0) && item->unsupportedLayerCount() == 0 &&
         item->cacheInvalidationSelfTest() && (!hybridSuite || item->transparentLayerCount() > 0) &&
         mediaFormatTestsPassed && (!cachePressureTest || item->textureCacheEvictions() > 0));
    const bool visualRegressionPassed =
        qPainterReferenceRegressionPassed &&
        compare.value("foregroundIntersectionOverUnion").toDouble() >= minimumForegroundIou &&
        compare.value("edgeF1Within1Px").toDouble() >= minimumEdgeF1 &&
        compare.value("meanAbsoluteColorError").toDouble() <= 3.0 &&
        compare.value("meanAbsoluteAlphaError").toDouble() <= 0.1 && cacheTestsPassed && nativeAtlasComplete;

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
        {"scenario", nativeAtlas ? "native-atlas" : hybridSuite ? "hybrid-text-native-image-svg" :
                                                primitiveSuite ? "primitive-suite-qPainter-oracle" :
                                                                 "project-shape-snapshot"},
        {"hybridSuite", hybridSuite},
        {"staticScene", staticScene},
        {"nativeAtlas", nativeAtlas},
        {"cachePressureTest", cachePressureTest},
        {"nativeAssetsSupported", item->nativeAssetCount()},
        {"nativeAssetsExpected", nativeAtlas ? int(studio::nativeLibrary().size()) : 0},
        {"nativeAtlasComplete", nativeAtlasComplete},
        {"rasterizedLayerCount", item->rasterizedLayerCount()},
        {"transparentTextureLayerCount", item->transparentLayerCount()},
        {"unsupportedLayerCount", item->unsupportedLayerCount()},
        {"cpuRasterizationMsTotal", double(item->rasterizationNanos()) / 1e6},
        {"cpuRasterizationMsTotalIncludesWarmup", true},
        {"cpuRasterizationMsPerMeasuredFrame",
         double(item->rasterizationNanos() - rasterizationBefore) / 1e6 / frameCount},
        {"textureUploadSubmissionMsTotal", double(item->textureUploadNanos()) / 1e6},
        {"textureUploadSubmissionMsPerMeasuredFrame",
         double(item->textureUploadNanos() - textureUploadBefore) / 1e6 / frameCount},
        {"textureUploadsDuringMeasuredFrames", item->textureUploads() - textureUploadsBefore},
        {"gpuTextureUploadCount", item->textureUploads()},
        {"textureCacheHits", item->textureCacheHits()},
        {"textureCacheMisses", item->textureCacheMisses()},
        {"textureCacheBytes", item->textureCacheBytes()},
        {"textureCachePeakBytes", item->peakTextureCacheBytes()},
        {"textureCacheBudgetBytes", item->textureCacheBudget()},
        {"textureCacheEvictions", item->textureCacheEvictions()},
        {"textureCacheWithinBudget", item->textureCacheWithinBudget()},
        {"textureCacheTestsPassed", cacheTestsPassed},
        {"cacheInvalidationChecksPassed", (!hybridSuite && !nativeAtlas) || item->cacheInvalidationSelfTest()},
        {"nativeRasterizationBackend", "QPainter CPU rasterization uploaded as GPU textures"},
        {"fontFamilyRequested", hybridSuite ? (registeredFontFamily.isEmpty() ? "Segoe UI" : registeredFontFamily) : ""},
        {"fontFamilyAvailable", hybridSuite && QFontDatabase().families().contains(
                                      registeredFontFamily.isEmpty() ? "Segoe UI" : registeredFontFamily,
                                      Qt::CaseInsensitive)},
        {"externalFontRegistered", !registeredFontFamily.isEmpty()},
        {"registeredFontFamily", registeredFontFamily},
        {"imageFormats", QJsonObject{{"png", pngFixtureReadable}, {"jpeg", jpegFixtureReadable},
                                     {"webpAvailable", webpAvailable}, {"webp", webpFixtureReadable},
                                     {"svg", svgFixtureReadable}}},
        {"mediaFormatTestsPassed", mediaFormatTestsPassed},
        {"webpAlphaTested", webpAlphaTested},
        {"webpAlphaPreserved", webpAlphaWasPreserved},
        {"rasterSupersamplingScale", 2.0},
        {"transparentLayerSamplesDetected", item->transparentLayerCount() > 0},
        {"supportedVectorPrimitives", primitiveCoverage},
        {"unsupportedSceneFeatures",
         QJsonArray{"video frames", "shot transition compositing", "premium effects"}},
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
        {"qPainterReference", forcedPrimitive ? goldenPath : hybridGoldenPath},
        {"qPainterReferenceMetrics", qPainterReferenceMetrics},
        {"qPainterReferenceRegressionPassed", qPainterReferenceRegressionPassed},
        {"visualRegressionCriteria",
         QJsonObject{{"foregroundIntersectionOverUnionMin", minimumForegroundIou},
                     {"edgeF1Within1PxMin", minimumEdgeF1},
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
