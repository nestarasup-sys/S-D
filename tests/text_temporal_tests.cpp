#include <QDateTime>
#include <QFontDatabase>
#include <QFileInfo>
#include <QDir>
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <studio/composition/painter.hpp>
#include <studio/core/project.hpp>
#include <studio/storage/project_store.hpp>
#include <studio/storage/workspace_store.hpp>

namespace {
struct Profile { int width, height; const char *name; };
struct Motion { const char *name; double x, y, zoom, rotation; };

void require(bool condition, const QString &message) {
    if (!condition)
        throw std::runtime_error(message.toStdString());
}

std::size_t changedPixels(const QImage &a, const QImage &b, const QRect &region) {
    std::size_t changed = 0;
    const auto bounded = region.intersected(a.rect()).intersected(b.rect());
    for (int y = bounded.top(); y <= bounded.bottom(); ++y)
        for (int x = bounded.left(); x <= bounded.right(); ++x)
            if (a.pixel(x, y) != b.pixel(x, y))
                ++changed;
    return changed;
}

QTransform transform(const studio::Project &project, const studio::CameraState &camera) {
    QTransform value;
    value.translate(project.width / 2.0, project.height / 2.0);
    value.scale(camera.zoom, camera.zoom);
    value.rotate(camera.rotation);
    value.translate(-camera.x, -camera.y);
    return value;
}

studio::Project makeProject(const Profile &profile, const Motion &motion, bool lightBackground = false) {
    auto project = studio::makeEmpty();
    project.width = profile.width;
    project.height = profile.height;
    project.duration = 60;
    project.schemaVersion = 14;
    project.name = std::string("Temporal text — ") + motion.name;
    const std::string background = lightBackground ? "#F3F5F7" : "#101820";
    const std::string textColor = lightBackground ? "#101820" : "#F4F7FF";
    project.artDirection.backgroundColor = background;
    project.artDirection.glowColor = background;
    project.artDirection.textColor = textColor;
    project.artDirection.captionBackgroundColor = lightBackground ? "#DCE3E8" : "#080E14";
    project.camera = {{0, profile.width / 2.0, profile.height / 2.0, 1, 0, "linear", {}}};
    if (motion.x != 0 || motion.y != 0 || motion.zoom != 1 || motion.rotation != 0)
        project.camera.push_back({59, profile.width / 2.0 + motion.x,
                                  profile.height / 2.0 + motion.y, motion.zoom,
                                  motion.rotation, "linear", {}});
    const double margin = profile.width * .11;
    studio::Node title;
    title.id = "normal-title";
    title.name = title.text = "A HISTÓRIA DA TECNOLOGIA";
    title.kind = studio::NodeKind::Text;
    title.x = margin;
    title.y = profile.height * .22;
    title.width = profile.width - margin * 2;
    title.height = std::max(100.0, profile.height * .12);
    title.fontSize = std::min(72.0, profile.width * .055);
    title.color = textColor;
    title.animated = false;
    title.motion = "none";
    title.start = 0;
    title.duration = project.duration;
    project.nodes.push_back(title);
    auto subtitle = title;
    subtitle.id = "normal-subtitle";
    subtitle.name = subtitle.text = "Como o mundo mudou";
    subtitle.y = profile.height * .37;
    subtitle.fontSize = std::min(42.0, profile.width * .032);
    subtitle.color = lightBackground ? "#31414C" : "#9DB5C9";
    subtitle.animated = false;
    subtitle.motion = "none";
    project.nodes.push_back(subtitle);
    studio::Node shape;
    shape.id = "camera-shape";
    shape.kind = studio::NodeKind::Shape;
    shape.x = profile.width * .72;
    shape.y = profile.height * .56;
    shape.width = profile.width * .10;
    shape.height = profile.height * .09;
    shape.color = "#58F4DA";
    shape.animated = false;
    shape.motion = "none";
    shape.start = 0;
    shape.duration = project.duration;
    project.nodes.push_back(shape);
    studio::Node caption = subtitle;
    caption.id = "scene-caption";
    caption.name = "Legenda de referência";
    caption.text = "Como o mundo mudou";
    caption.role = "caption";
    caption.x = profile.width * .12;
    caption.y = profile.height * .85;
    caption.width = profile.width * .76;
    caption.height = std::max(60.0, profile.height * .08);
    caption.fontSize = std::min(30.0, profile.width * .024);
    caption.animated = false;
    caption.motion = "none";
    project.nodes.push_back(caption);
    return project;
}

QImage warpReference(const studio::Project &project, const QImage &reference, std::int64_t frame) {
    const auto camera = studio::evaluateCamera(project, frame);
    QImage expected(project.width, project.height, QImage::Format_ARGB32_Premultiplied);
    expected.fill(Qt::transparent);
    QPainter painter(&expected);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.setWorldTransform(transform(project, camera));
    painter.drawImage(QPointF(0, 0), reference);
    painter.end();
    return expected;
}

QRect textRegion(const studio::Project &project, std::int64_t frame) {
    const auto camera = studio::evaluateCamera(project, frame);
    const auto *node = studio::findNode(project, "normal-title");
    return transform(project, camera).mapRect(QRectF(node->x, node->y, node->width, node->height))
        .adjusted(-8, -8, 8, 8).toAlignedRect().intersected(QRect(0, 0, project.width, project.height));
}

QImage renderTransparent(studio::CompositionPainter &painter, const studio::Project &project,
                         std::int64_t frame) {
    QImage image(project.width, project.height, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter target(&image);
    painter.paint(target, project, frame);
    target.end();
    return image;
}

QJsonObject compare(const QImage &actual, const QImage &expected, const QRect &region) {
    double mae = 0, alphaMae = 0;
    std::size_t n = 0, intersection = 0, unionCount = 0;
    double ax = 0, ay = 0, ex = 0, ey = 0;
    std::size_t actualCount = 0, expectedCount = 0;
    for (int y = region.top(); y <= region.bottom(); ++y) {
        for (int x = region.left(); x <= region.right(); ++x) {
            const auto a = actual.pixelColor(x, y);
            const auto e = expected.pixelColor(x, y);
            mae += std::abs(a.red() - e.red()) + std::abs(a.green() - e.green()) +
                   std::abs(a.blue() - e.blue());
            n += 3;
            alphaMae += std::abs(a.alpha() - e.alpha());
            const bool af = a.alpha() > 16, ef = e.alpha() > 16;
            intersection += af && ef;
            unionCount += af || ef;
            if (af) { ax += x; ay += y; ++actualCount; }
            if (ef) { ex += x; ey += y; ++expectedCount; }
        }
    }
    const double actualCx = actualCount ? ax / actualCount : 0;
    const double actualCy = actualCount ? ay / actualCount : 0;
    const double expectedCx = expectedCount ? ex / expectedCount : 0;
    const double expectedCy = expectedCount ? ey / expectedCount : 0;
    return {{"meanAbsoluteErrorRgb", n ? mae / n : 0},
            {"meanAbsoluteErrorAlpha", n ? alphaMae / (n / 3) : 0},
            {"binaryMaskIoU", unionCount ? double(intersection) / unionCount : 1},
            {"centroidErrorPixels", std::hypot(actualCx - expectedCx, actualCy - expectedCy)},
            {"foregroundPixelsActual", double(actualCount)},
            {"foregroundPixelsExpected", double(expectedCount)}};
}
}

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    app.setOrganizationName("StudioDarkPrototype");
    app.setApplicationName("Studio Dark");
    studio::loadWorkspaceFonts(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) +
                               "/workspace");
    const QString systemSegoe = QStringLiteral("C:/Windows/Fonts/segoeui.ttf");
    if (QFileInfo::exists(systemSegoe))
        QFontDatabase::addApplicationFont(systemSegoe);
    try {
        const QString sourceRoot = QStringLiteral(STUDIO_SOURCE_DIR);
        const QString outputRoot = sourceRoot + "/artifacts/text-temporal-" +
                                   QDateTime::currentDateTime().toString("yyyyMMdd-HHmmsszzz");
        require(QDir().mkpath(outputRoot), "create text temporal artifact root");
        const Profile profiles[]{{1280, 720, "1280x720"}, {1920, 1080, "1920x1080"},
                                  {1080, 1920, "1080x1920"}};
        const Motion motions[]{{"still", 0, 0, 1, 0}, {"pan-horizontal", 12, 0, 1, 0},
                               {"pan-vertical", 0, 10, 1, 0}, {"zoom-in", 0, 0, 1.15, 0},
                               {"zoom-out", 0, 0, 1, 0}, {"pan-zoom", 12, 8, 1.12, 0},
                               {"rotation", 0, 0, 1, 2}};
        QJsonArray reports;
        for (const auto &profile : profiles) {
            for (const auto &motion : motions) {
                auto project = makeProject(profile, motion);
                if (QString::fromLatin1(motion.name) == "zoom-out")
                    project.camera.front().zoom = 1.15;
                QDir().mkpath(outputRoot + "/" + profile.name + "/" + motion.name);
                studio::CompositionPainter painter, staticPainter;
                auto staticProject = project;
                staticProject.artDirection.backgroundColor = "#00000000";
                staticProject.artDirection.glowColor = "#00000000";
                project.artDirection.backgroundColor = "#00000000";
                project.artDirection.glowColor = "#00000000";
                staticProject.camera = {{0, profile.width / 2.0, profile.height / 2.0, 1, 0,
                                         "linear", {}}};
                const auto reference = renderTransparent(staticPainter, staticProject, 0);
                QJsonArray samples;
                QImage prior;
                std::size_t stationaryChanged = 0;
                const int count = 60;
                const bool exportFixture = QString::fromLatin1(profile.name) == "1280x720" &&
                                           QString::fromLatin1(motion.name) == "pan-zoom";
                if (exportFixture) {
                    auto exportProject = makeProject(profile, motion);
                    const auto fixturePath = outputRoot + "/text-pan-zoom.studio.json";
                    const auto saved = studio::saveProject(fixturePath, exportProject, {});
                    require(saved.error.isEmpty(), "save text pan/zoom MP4 fixture: " + saved.error);
                }
                for (int frame = 0; frame < count; ++frame) {
                    const auto actual = renderTransparent(painter, project, frame);
                    const auto expected = warpReference(project, reference, frame);
                    const auto region = textRegion(project, frame);
                    auto metrics = compare(actual, expected, region);
                    metrics["frame"] = frame;
                    samples.append(metrics);
                    if (frame == 0 || frame == 15 || frame == 30 || frame == 45 || frame == 59 ||
                        (QString::fromLatin1(motion.name) == "pan-zoom")) {
                        const auto crop = actual.copy(region);
                        crop.save(outputRoot + "/" + profile.name + "/" + motion.name +
                                  QString("/frame-%1.png").arg(frame, 3, 10, QChar('0')));
                    }
                    if (exportFixture && (frame == 0 || frame == 15 || frame == 30 || frame == 45 || frame == 59)) {
                        const auto fullFrame = painter.frame(makeProject(profile, motion), frame);
                        require(fullFrame.save(outputRoot +
                                               QString("/export-reference-%1.png").arg(frame, 3, 10, QChar('0'))),
                                "save full-size PNG reference for MP4 comparison");
                    }
                    if (!prior.isNull() && QString::fromLatin1(motion.name) == "still")
                        stationaryChanged += std::size_t(changedPixels(actual, prior, actual.rect()));
                    prior = actual;
                }
                QJsonObject summary{{"canvas", QString::fromLatin1(profile.name)},
                                    {"motion", QString::fromLatin1(motion.name)},
                                    {"frames", count}, {"sampleMetrics", samples},
                                    {"stationaryChangedPixels", double(stationaryChanged)},
                                    {"referenceMethod", "QPainter reference frame warped with the resolved camera transform"}};
                if (QString::fromLatin1(motion.name) == "still")
                    require(stationaryChanged == 0, "static camera produces byte-identical text frames");
                reports.append(summary);
            }
            const Motion still{"still", 0, 0, 1, 0};
            auto light = makeProject(profile, still, true);
            studio::CompositionPainter lightPainter;
            const auto lightFrame = lightPainter.frame(light, 0);
            const auto lightNextFrame = lightPainter.frame(light, 1);
            const auto lightDifference = changedPixels(lightFrame, lightNextFrame, lightFrame.rect());
            require(!lightFrame.isNull() && lightDifference == 0,
                    "light-background text fixture renders nonempty deterministic frames; changed pixels=" +
                        QString::number(qulonglong(lightDifference)));
            lightFrame.save(outputRoot + "/" + profile.name + "/light-background.png");
        }
        auto transitionProject = makeProject(profiles[0], Motion{"transition-pan", 18, 0, 1.08, 0});
        transitionProject.scenes.push_back({"text-scene", "Text camera test", "", 0, 60});
        transitionProject.shots = {{"text-shot-a", "text-scene", "Shot A", "cut", 0, 30, 0},
                                   {"text-shot-b", "text-scene", "Shot B", "fade", 30, 30, 8}};
        studio::CompositionPainter transitionPainter;
        QJsonArray transitionFrames;
        for (int frame = 25; frame < 40; ++frame) {
            const auto image = transitionPainter.frame(transitionProject, frame);
            require(!image.isNull() && image.size() == QSize(1280, 720),
                    "camera-moving shot transition renders text frame");
            if (frame == 25 || frame == 29 || frame == 30 || frame == 34 || frame == 38)
                image.save(outputRoot + QString("/transition-frame-%1.png").arg(frame));
            transitionFrames.append(frame);
        }
        QJsonObject report{{"status", "passed"}, {"resolutionProfiles", QJsonArray{"1280x720", "1920x1080", "1080x1920"}},
                           {"motions", QJsonArray{"still", "pan-horizontal", "pan-vertical", "zoom-in", "zoom-out", "pan-zoom", "rotation"}},
                           {"textSamples", QJsonArray{"A HISTÓRIA DA TECNOLOGIA", "Como o mundo mudou", "scene caption"}},
                           {"method", "Transparent text-only and scene-element sequences are compared with a no-camera QPainter reference warped using the resolved camera transform. The text ROI excludes the background, so metrics measure raster resampling and edge coverage rather than camera motion or background gradients. Metrics are per-frame RGB/alpha MAE, binary alpha-mask IoU and centroid displacement; they are not interpreted alone as a jitter verdict."},
                           {"lightBackgroundProfiles", QJsonArray{"1280x720", "1920x1080", "1080x1920"}},
                           {"transitionFrames", transitionFrames},
                           {"scenarios", reports}, {"artifacts", outputRoot}};
        QFile file(outputRoot + "/report.json");
        require(file.open(QIODevice::WriteOnly | QIODevice::Truncate), "open text temporal report");
        file.write(QJsonDocument(report).toJson(QJsonDocument::Indented));
        file.close();
        std::cout << "Text temporal baseline passed. Evidence: " << outputRoot.toStdString() << '\n';
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
