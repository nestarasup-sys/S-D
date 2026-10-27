#include <QElapsedTimer>
#include <QHash>
#include <QGuiApplication>
#include <QImage>
#include <QFontDatabase>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QRectF>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <studio/composition/painter.hpp>
#include <studio/core/library.hpp>
#include <studio/storage/workspace_store.hpp>

namespace {
void check(bool ok, const char *message) {
    if (!ok)
        throw std::runtime_error(message);
}
QRect contentBounds(const studio::Project &project) {
    QHash<QString, QPointF> origins;
    QRectF bounds;
    bool hasBounds = false;
    for (const auto &node : project.nodes) {
        auto origin = QPointF(node.x, node.y);
        if (!node.parentId.empty())
            origin += origins.value(QString::fromStdString(node.parentId));
        origins.insert(QString::fromStdString(node.id), origin);
        if (node.kind == studio::NodeKind::Group)
            continue;
        const auto rect = QRectF(origin, QSizeF(std::max(1.0, node.width), std::max(1.0, node.height)));
        bounds = hasBounds ? bounds.united(rect) : rect;
        hasBounds = true;
    }
    if (!hasBounds)
        return QRect(0, 0, project.width, project.height);
    return bounds.adjusted(-8, -8, 8, 8).toAlignedRect().intersected(QRect(0, 0, project.width, project.height));
}
double sampledDifference(const QImage &a, const QImage &b, QRect roi) {
    if (a.size() != b.size()) return 0;
    roi = roi.intersected(a.rect());
    double sum = 0;
    int samples = 0;
    for (int y = roi.top(); y <= roi.bottom(); y += 8)
        for (int x = roi.left(); x <= roi.right(); x += 8) {
            const auto xColor = a.pixelColor(x, y);
            const auto yColor = b.pixelColor(x, y);
            sum += std::abs(xColor.red() - yColor.red()) +
                   std::abs(xColor.green() - yColor.green()) +
                   std::abs(xColor.blue() - yColor.blue()) +
                   std::abs(xColor.alpha() - yColor.alpha());
            samples += 4;
        }
    return samples ? sum / samples : 0;
}
QJsonObject benchmark(const studio::PremiumAssetDefinition &definition, int width, int height) {
    auto project = studio::makeEmpty();
    project.name = std::string(definition.name);
    project.width = width;
    project.height = height;
    project.duration = 360;
    project.nodes = studio::makePremiumAsset(definition.id, width, height);
    check(studio::validate(project).empty(), "responsive premium composition validates");
    studio::CompositionPainter painter;
    QJsonArray repeats;
    constexpr int frames = 15;
    for (int run = 0; run < 3; ++run) {
        QElapsedTimer timer;
        timer.start();
        for (int frame = 0; frame < frames; ++frame) {
            const auto image = painter.frame(project, frame + 36);
            check(image.size() == QSize(width, height), "QPainter returns the selected canvas size");
        }
        repeats.append(double(timer.nsecsElapsed()) / 1'000'000.0);
    }
    auto values = std::vector<double>{repeats[0].toDouble(), repeats[1].toDouble(), repeats[2].toDouble()};
    std::sort(values.begin(), values.end());
    return {{"assetId", QString::fromUtf8(definition.id.data(), int(definition.id.size()))},
            {"resolution", QString::number(width) + "x" + QString::number(height)},
            {"nodeCount", int(project.nodes.size())}, {"framesPerRepeat", frames},
            {"repeatCount", 3}, {"medianTotalMs", values[1]},
            {"medianMsPerFrame", values[1] / frames},
            {"singleOutputFramebufferMiB", double(width) * height * 4 / (1024.0 * 1024.0)}};
}
} // namespace
int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    try {
        QString catalogError;
        check(studio::initializePremiumCatalog(&catalogError), "initialize declarative premium registry");
        const auto systemFontId = QFontDatabase::addApplicationFont(QStringLiteral("C:/Windows/Fonts/segoeui.ttf"));
        check(systemFontId >= 0, "load the existing Windows Segoe UI font for deterministic offscreen text rendering");
        const std::pair<int, int> canvases[]{{1280, 720}, {1920, 1080}, {1080, 1920}};
        const QString output = QStringLiteral(STUDIO_SOURCE_DIR) + "/artifacts/premium-factory-stage12-13";
        check(QDir().mkpath(output), "create premium visual report directory");
        QJsonArray results;
        for (const auto &definition : studio::premiumLibrary()) {
            for (const auto &[width, height] : canvases) {
                auto project = studio::makeEmpty();
                project.name = std::string(definition.name);
                project.width = width;
                project.height = height;
                project.duration = 360;
                project.nodes = studio::makePremiumAsset(definition.id, width, height);
                check(!project.nodes.empty() && studio::validate(project).empty(),
                      "registered premium factory and canvas dimensions validate");
                for (const auto &node : project.nodes) {
                    if (node.parentId.empty()) continue;
                    const auto parent = std::find_if(project.nodes.begin(), project.nodes.end(),
                        [&](const auto &candidate) { return candidate.id == node.parentId; });
                    check(parent != project.nodes.end() && node.x >= -1 && node.y >= -1 &&
                              node.x + node.width <= parent->width + 2 &&
                              node.y + node.height <= parent->height + 2,
                          "nested layer fits inside its local parent bounds");
                }
                studio::CompositionPainter painter;
                const auto entrance = painter.frame(project, 12);
                const auto settled = painter.frame(project, 56);
                const auto roi = contentBounds(project);
                const auto id = QString::fromUtf8(definition.id.data(), int(definition.id.size()))
                                    .section(':', 1).replace(':', '-');
                if (entrance.isNull() || settled.isNull() || sampledDifference(entrance, settled, roi) <= .08)
                    throw std::runtime_error(("premium animation has insufficient visual change: " +
                                              id + " at " + QString::number(width) + "x" +
                                              QString::number(height)).toStdString());
                if (width == 1280 || height == 1920) {
                    entrance.save(output + "/" + id + "-" + QString::number(width) + "x" +
                                  QString::number(height) + "-entrance.png");
                    settled.save(output + "/" + id + "-" + QString::number(width) + "x" +
                                 QString::number(height) + "-settled.png");
                }
                if (definition.id == "premium:title-reveal-01") {
                    auto longTitle = project;
                    auto title = std::find_if(longTitle.nodes.begin(), longTitle.nodes.end(),
                        [](const auto &node) { return node.role == "premium:title:main"; });
                    check(title != longTitle.nodes.end(), "title-reveal main-title slot is registered");
                    title->text = "A LONG DOCUMENTARY TITLE THAT SHOULD WRAP INSIDE THE SAFE AREA\nWITHOUT CLIPPING OR CROPPING ITS FINAL WORDS";
                    const auto longImage = painter.frame(longTitle, 56);
                    check(!longImage.isNull(), "long customizable title renders within the canvas");
                    if (width == 1080)
                        longImage.save(output + "/title-reveal-long-portrait.png");
                }
                results.append(benchmark(definition, width, height));
            }
        }
        QJsonObject report{{"status", "passed"},
                           {"backend", "CompositionPainter / QPainter"},
                           {"method", "3 repeats × 15 contiguous rendered frames; median; encoding excluded"},
                           {"canvases", QJsonArray{"1280x720", "1920x1080", "1080x1920"}},
                           {"scenarios", results}};
        QFile file(output + "/benchmark.json");
        check(file.open(QIODevice::WriteOnly) &&
                  file.write(QJsonDocument(report).toJson(QJsonDocument::Indented)) > 0,
              "save premium QPainter benchmark report");
        std::cout << QJsonDocument(report).toJson(QJsonDocument::Compact).constData() << '\n';
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
