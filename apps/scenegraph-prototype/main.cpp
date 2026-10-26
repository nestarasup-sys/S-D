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
#include <cmath>
#include <memory>
#include <vector>
#ifdef Q_OS_WIN
#include <psapi.h>
#include <windows.h>
#endif
#include <studio/composition/painter.hpp>
#include <studio/core/project.hpp>

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
    const int columns = std::max(1, int(std::sqrt(nodeCount * double(width) / height)));
    const int rows = (nodeCount + columns - 1) / columns;
    const double cellWidth = double(width) / columns;
    const double cellHeight = double(height) / rows;
    for (int index = 0; index < nodeCount; ++index) {
        const int column = index % columns;
        const int row = index / columns;
        studio::Node group;
        group.id = "group-" + std::to_string(index);
        group.kind = studio::NodeKind::Group;
        group.name = "Benchmark group";
        group.start = 0;
        group.duration = project.duration;
        group.x = column * cellWidth;
        group.y = row * cellHeight;
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
            {0, shape.x, shape.y, 1, 0, 1, "ease-in-out"},
            {60, shape.x + cellWidth * .06, shape.y + cellHeight * .06, 1, 4, .92, "ease-in-out"},
            {119, shape.x, shape.y, 1, 0, 1, "linear"}};
        project.nodes.push_back(shape);
    }
    project.camera = {{0, width / 2.0, height / 2.0, 1, 0, "ease-in-out", {}},
                      {119, width / 2.0 + width * .025, height / 2.0, 1.025, 0, "linear", {}}};
    return project;
}

QTransform nodeTransform(const studio::Project &project, const studio::Node &node, std::int64_t frame) {
    const auto camera = studio::evaluateCamera(project, frame);
    QTransform transform;
    transform.translate(project.width / 2.0, project.height / 2.0);
    transform.scale(camera.zoom, camera.zoom);
    transform.rotate(camera.rotation);
    transform.translate(-camera.x, -camera.y);
    std::vector<const studio::Node *> chain;
    const studio::Node *part = &node;
    while (part) {
        chain.push_back(part);
        part = part->parentId.empty() ? nullptr : studio::findNode(project, part->parentId);
    }
    std::reverse(chain.begin(), chain.end());
    for (const auto *entry : chain) {
        const auto state = studio::evaluate(*entry, frame);
        transform.translate(state.x, state.y);
        transform.translate(entry->width / 2.0, entry->height / 2.0);
        transform.rotate(state.rotation);
        transform.scale(state.scale, state.scale);
        transform.translate(-entry->width / 2.0, -entry->height / 2.0);
    }
    return transform;
}

std::vector<QPointF> roundedRect(const QRectF &rect) {
    const double radius = std::min({12.0, rect.width() / 4.0, rect.height() / 4.0});
    std::vector<QPointF> points;
    constexpr int steps = 5;
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

class SceneGraphItem final : public QQuickItem {
  public:
    explicit SceneGraphItem(studio::Project project) : project_(std::move(project)) {
        setFlag(ItemHasContents);
    }
    void setFrame(std::int64_t frame) {
        frame_ = frame;
        update();
    }
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) override {
        auto *node = static_cast<QSGGeometryNode *>(oldNode);
        if (!node) {
            node = new QSGGeometryNode;
            auto *geometry = new QSGGeometry(QSGGeometry::defaultAttributes_ColoredPoint2D(), 0);
            geometry->setDrawingMode(QSGGeometry::DrawTriangles);
            geometry->setVertexDataPattern(QSGGeometry::DynamicPattern);
            node->setGeometry(geometry);
            node->setFlag(QSGNode::OwnsGeometry);
            node->setMaterial(new QSGVertexColorMaterial);
            node->setFlag(QSGNode::OwnsMaterial);
        }
        std::vector<Vertex> vertices;
        const double sx = width() / project_.width;
        const double sy = height() / project_.height;
        for (const auto &shape : project_.nodes) {
            if (shape.kind != studio::NodeKind::Shape || !studio::active(shape, frame_))
                continue;
            const auto state = studio::evaluate(shape, frame_);
            if (state.opacity <= 0)
                continue;
            const auto polygon = roundedRect(QRectF(0, 0, shape.width, shape.height));
            const auto transform = nodeTransform(project_, shape, frame_);
            const QColor color(QString::fromStdString(shape.color));
            const auto append = [&](const QPointF &point) {
                const auto mapped = transform.map(point);
                vertices.push_back({float(mapped.x() * sx), float(mapped.y() * sy), uchar(color.red()),
                                    uchar(color.green()), uchar(color.blue()),
                                    uchar(std::clamp(qRound(color.alpha() * state.opacity), 0, 255))});
            };
            for (std::size_t i = 0; i < polygon.size(); ++i) {
                append({shape.width / 2.0, shape.height / 2.0});
                append(polygon[i]);
                append(polygon[(i + 1) % polygon.size()]);
            }
        }
        auto *geometry = node->geometry();
        geometry->allocate(int(vertices.size()));
        auto *destination = geometry->vertexDataAsColoredPoint2D();
        for (std::size_t index = 0; index < vertices.size(); ++index) {
            const auto &vertex = vertices[index];
            destination[index].set(vertex.x, vertex.y, vertex.r, vertex.g, vertex.b, vertex.a);
        }
        node->markDirty(QSGNode::DirtyGeometry);
        return node;
    }

  private:
    studio::Project project_;
    std::int64_t frame_{0};
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
    long double squaredError = 0;
    const auto pixels = qint64(left.width()) * left.height() * 3;
    for (int y = 0; y < left.height(); ++y) {
        const auto *a = reinterpret_cast<const QRgb *>(left.constScanLine(y));
        const auto *b = reinterpret_cast<const QRgb *>(right.constScanLine(y));
        for (int x = 0; x < left.width(); ++x) {
            const int dr = qRed(a[x]) - qRed(b[x]);
            const int dg = qGreen(a[x]) - qGreen(b[x]);
            const int db = qBlue(a[x]) - qBlue(b[x]);
            squaredError += dr * dr + dg * dg + db * db;
        }
    }
    const double mse = double(squaredError / pixels);
    return mse <= 0 ? 99.0 : 10.0 * std::log10(255.0 * 255.0 / mse);
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
    studio::CompositionPainter cpu;
    QElapsedTimer timer;
    QImage cpuImage;
    for (int warmup = 0; warmup < 3; ++warmup)
        cpuImage = cpu.frame(project, warmup);
    timer.start();
    for (int i = 0; i < frameCount; ++i)
        cpuImage = cpu.frame(project, i % int(project.duration));
    const double cpuMilliseconds = double(timer.nsecsElapsed()) / 1e6 / frameCount;
    cpuImage.save(output.filePath("cpu-reference.png"));
    const auto cpuWorkingSet = workingSetBytes();

    QQuickWindow window;
    window.setFlags(Qt::FramelessWindowHint);
    auto surfaceFormat = window.format();
    surfaceFormat.setSwapInterval(0);
    window.setFormat(surfaceFormat);
    window.setColor(QColor("#101820"));
    window.setGeometry(40, 40, width, height);
    auto *item = new SceneGraphItem(project);
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
        item->setFrame(warmup);
        QTimer::singleShot(5000, &loop, &QEventLoop::quit);
        loop.exec();
        QObject::disconnect(connection);
    }
    timer.restart();
    for (int i = 0; i < frameCount; ++i) {
        QEventLoop loop;
        const auto connection =
            QObject::connect(&window, &QQuickWindow::frameSwapped, &loop, &QEventLoop::quit);
        item->setFrame(i % int(project.duration));
        QTimer::singleShot(5000, &loop, &QEventLoop::quit);
        loop.exec();
        QObject::disconnect(connection);
    }
    const double sceneGraphMilliseconds = double(timer.nsecsElapsed()) / 1e6 / frameCount;
    const QImage gpuImage = window.grabWindow();
    if (gpuImage.isNull())
        return 5;
    gpuImage.save(output.filePath("scenegraph-prototype.png"));
    const auto sceneGraphWorkingSet = workingSetBytes();

    const auto api = apiName(window.rendererInterface()->graphicsApi());
    const auto framebufferBytes = qint64(width) * height * 4;
    const auto geometryBytes = qint64(nodes) * 24 * 3 * sizeof(Vertex);
    const double pixelsPerSecondCpu = (double(width) * height) / (cpuMilliseconds / 1000.0);
    const double pixelsPerSecondGpu = (double(width) * height) / (sceneGraphMilliseconds / 1000.0);
    const QJsonObject metrics{{"canvas", QJsonObject{{"width", width}, {"height", height}}},
                              {"shapeCount", nodes},
                              {"frames", frameCount},
                              {"sceneGraphApi", api},
                              {"cpuPainterMsPerFrame", cpuMilliseconds},
                              {"sceneGraphMsPerFrame", sceneGraphMilliseconds},
                              {"cpuPainterPixelsPerSecond", pixelsPerSecondCpu},
                              {"sceneGraphPixelsPerSecond", pixelsPerSecondGpu},
                              {"cpuWorkingSetBytes", cpuWorkingSet},
                              {"sceneGraphWorkingSetBytes", sceneGraphWorkingSet},
                              {"visualPsnrDb", psnr(cpuImage, gpuImage)},
                              {"visualThresholdDb", 24.0},
                              {"visualRegressionPassed", psnr(cpuImage, gpuImage) >= 24.0},
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
    return psnr(cpuImage, gpuImage) >= 24.0 ? 0 : 6;
}
