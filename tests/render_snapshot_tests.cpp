#include <QColor>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QImage>
#include <QProcess>
#include <QTemporaryDir>
#include <QTextStream>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <studio/composition/painter.hpp>
#include <studio/composition/render_snapshot.hpp>
#include <studio/core/library.hpp>
#include <studio/core/project.hpp>

#ifndef STUDIO_REFERENCE_DIR
#define STUDIO_REFERENCE_DIR "tests/reference/render-snapshot"
#endif

namespace {
int checks = 0;
void check(bool condition, const char *message) {
    ++checks;
    if (!condition)
        throw std::runtime_error(message);
}

double psnr(const QImage &left, const QImage &right) {
    if (left.size() != right.size() || left.isNull() || right.isNull())
        return 0;
    const auto normalizedLeft = left.convertToFormat(QImage::Format_ARGB32);
    const auto normalizedRight = right.convertToFormat(QImage::Format_ARGB32);
    long double squaredError = 0;
    const auto samples = qint64(normalizedLeft.width()) * normalizedLeft.height() * 3;
    for (int y = 0; y < normalizedLeft.height(); ++y) {
        const auto *a = reinterpret_cast<const QRgb *>(normalizedLeft.constScanLine(y));
        const auto *b = reinterpret_cast<const QRgb *>(normalizedRight.constScanLine(y));
        for (int x = 0; x < normalizedLeft.width(); ++x) {
            const auto dr = qRed(a[x]) - qRed(b[x]);
            const auto dg = qGreen(a[x]) - qGreen(b[x]);
            const auto db = qBlue(a[x]) - qBlue(b[x]);
            squaredError += dr * dr + dg * dg + db * db;
        }
    }
    const double error = double(squaredError / samples);
    return error == 0 ? 99.0 : 10 * std::log10(255.0 * 255.0 / error);
}

void saveSvg(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        throw std::runtime_error("could not create SVG test fixture");
    file.write("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"160\" height=\"90\">"
               "<rect width=\"160\" height=\"90\" rx=\"10\" fill=\"#143644\"/>"
               "<circle cx=\"45\" cy=\"45\" r=\"24\" fill=\"#b4f078\"/>"
               "<path d=\"M82 60 112 28l27 32\" stroke=\"#efb873\" stroke-width=\"8\" fill=\"none\"/>"
               "</svg>");
}

void saveImage(const QString &path) {
    QImage image(96, 64, QImage::Format_ARGB32_Premultiplied);
    for (int y = 0; y < image.height(); ++y)
        for (int x = 0; x < image.width(); ++x)
            image.setPixelColor(x, y, QColor(28 + x * 2, 34 + y * 2, (x * 3 + y * 5) % 190 + 32));
    if (!image.save(path))
        throw std::runtime_error("could not create image test fixture");
}

QString createVideo(const QString &path) {
    auto ffmpeg = qEnvironmentVariable("STUDIO_FFMPEG", "ffmpeg");
    QProcess process;
    process.start(ffmpeg,
                  {"-y", "-hide_banner", "-loglevel", "error", "-f", "lavfi", "-i",
                   "testsrc2=size=96x64:rate=30:duration=3", "-c:v", "libx264", "-pix_fmt", "yuv420p", path});
    if (!process.waitForStarted(5000) || !process.waitForFinished(15000) || process.exitCode() != 0)
        throw std::runtime_error("could not create video test fixture");
    return path;
}

studio::Node node(QString id, studio::NodeKind kind, double x, double y, double w, double h,
                  std::string color = "#b4f078") {
    studio::Node result;
    result.id = id.toStdString();
    result.name = id.toStdString();
    result.kind = kind;
    result.x = x;
    result.y = y;
    result.width = w;
    result.height = h;
    result.duration = 90;
    result.animated = false;
    result.color = std::move(color);
    return result;
}

studio::Project makeMixedProject(const QString &imagePath, const QString &svgPath, const QString &videoPath) {
    auto project = studio::makeEmpty();
    project.id = "render-snapshot-mixed";
    project.schemaVersion = 14;
    project.name = "Render snapshot regression";
    project.width = 640;
    project.height = 360;
    project.duration = 90;
    project.artDirection.backgroundColor = "#101820";
    project.artDirection.glowColor = "#19333b";
    project.artDirection.primaryColor = "#b4f078";
    project.artDirection.textColor = "#e4f5e9";
    project.camera = {{0, 320, 180, 1, 0, "ease-in-out", {}}, {60, 340, 176, 1.08, 1.5, "ease-out", {}}};

    auto group = node("root-group", studio::NodeKind::Group, 40, 35, 420, 250);
    group.opacity = .8;
    group.rotation = -2;
    group.keyframes = {{0, 40, 35, -2, 1, .8, "ease-in-out"}, {60, 50, 28, 3, .96, .8, "ease-out"}};
    project.nodes.push_back(group);

    auto shape = node("shape", studio::NodeKind::Shape, 18, 22, 150, 76, "#b4f078");
    shape.parentId = "root-group";
    shape.opacity = .9;
    shape.keyframes = {{0, 18, 22, 0, 1, .9, "linear"}, {60, 38, 42, 12, 1.1, .9, "ease-in-out"}};
    project.nodes.push_back(shape);

    auto title = node("title", studio::NodeKind::Text, 20, 116, 260, 70, "#f0f1e9");
    title.parentId = "root-group";
    title.text = "Cena compartilhada";
    title.fontSize = 32;
    project.nodes.push_back(title);

    auto svg = node("svg", studio::NodeKind::Svg, 206, 24, 160, 90);
    svg.parentId = "root-group";
    svg.source = svgPath.toStdString();
    project.nodes.push_back(svg);

    auto image = node("image", studio::NodeKind::Image, 206, 128, 96, 64);
    image.parentId = "root-group";
    image.source = imagePath.toStdString();
    project.nodes.push_back(image);

    auto car = node("native-car", studio::NodeKind::Native, 328, 120, 110, 50, "#f2bd61");
    car.parentId = "root-group";
    car.nativeId = "car";
    project.nodes.push_back(car);

    auto person = node("native-person", studio::NodeKind::Native, 500, 62, 90, 175, "#c77759");
    person.nativeId = "person";
    person.pose = "wave";
    person.start = 0;
    project.nodes.push_back(person);

    auto video = node("video", studio::NodeKind::Video, 425, 240, 160, 90);
    video.source = videoPath.toStdString();
    video.videoSourceInSeconds = .1;
    video.videoPlaybackRate = 1.25;
    video.videoCropX = .05;
    video.videoCropY = .05;
    video.videoCropWidth = .9;
    video.videoCropHeight = .9;
    project.nodes.push_back(video);

    auto caption = node("caption", studio::NodeKind::Text, 56, 294, 430, 48, "#e4f5e9");
    caption.role = "caption";
    caption.text = "A prova começa agora";
    caption.fontSize = 24;
    caption.wordAnimationPreset = "pop-rise";
    caption.wordAnimationDurationFrames = 8;
    caption.captionWords = {{"A", 20, 31}, {"prova", 31, 44}, {"começa", 44, 58}, {"agora", 58, 72}};
    project.nodes.push_back(caption);
    return project;
}

studio::Project makeShotProject(studio::Project project) {
    project.id = "render-snapshot-shots";
    project.duration = 60;
    project.scenes = {{"scene-a", "Plano A", "", 0, 30, std::nullopt},
                      {"scene-b", "Plano B", "", 30, 30, std::nullopt}};
    project.shots = {{"shot-a", "scene-a", "A", "cut", 0, 30, 0, "left", std::nullopt},
                     {"shot-b", "scene-a", "B", "dissolve", 30, 30, 8, "left", std::nullopt}};
    for (auto &item : project.nodes) {
        if (item.id == "shape" || item.id == "title" || item.id == "svg" || item.id == "image")
            item.shotId = "shot-a";
        if (item.id == "native-car" || item.id == "native-person" || item.id == "video")
            item.shotId = "shot-b";
    }
    project.camera = {{0, 320, 180, 1, 0, "ease-in-out", "shot-a"},
                      {29, 300, 180, 1.12, 0, "ease-out", "shot-a"},
                      {30, 320, 180, 1, 0, "ease-in-out", "shot-b"},
                      {59, 335, 172, 1.06, 1, "linear", "shot-b"}};
    return project;
}

void writeOrCompare(const QString &name, const QImage &image, const QString &updatePath) {
    if (!updatePath.isEmpty()) {
        if (!image.save(QDir(updatePath).filePath(name)))
            throw std::runtime_error("could not write visual reference image");
        return;
    }
    const auto reference = QImage(QStringLiteral(STUDIO_REFERENCE_DIR) + QLatin1Char('/') + name);
    const auto score = psnr(image, reference);
    QTextStream(stdout) << name << ": PSNR=" << score << " dB\n";
    check(score >= 42.0, "render output diverges from the pre-snapshot visual reference");
}

} // namespace

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    try {
        const QString updatePath = argc == 3 && QString::fromLocal8Bit(argv[1]) == "--update-reference"
                                       ? QString::fromLocal8Bit(argv[2])
                                       : QString{};
        if (!updatePath.isEmpty() && !QDir().mkpath(updatePath))
            throw std::runtime_error("could not create reference output folder");
        QTemporaryDir assets;
        if (!assets.isValid())
            throw std::runtime_error("could not create temporary fixture folder");
        const auto imagePath = assets.filePath("pattern.png");
        const auto svgPath = assets.filePath("scene.svg");
        const auto videoPath = assets.filePath("motion.mp4");
        saveImage(imagePath);
        saveSvg(svgPath);
        createVideo(videoPath);

        auto mixed = makeMixedProject(imagePath, svgPath, videoPath);
        const auto snapshot = studio::RenderSnapshot::build(mixed, 30);
        check(snapshot.frame() == 30 && snapshot.durationFrames() == 90 && snapshot.width() == 640 &&
                  snapshot.height() == 360 && snapshot.fps().value() == mixed.fps.value(),
              "snapshot captures frame timing and canvas metadata");
        const auto findResolved = [&](const std::string &id) -> const studio::RenderNodeSnapshot * {
            const auto found = std::find_if(snapshot.primary().nodes.begin(), snapshot.primary().nodes.end(),
                                            [&](const auto &item) { return item.node.id == id; });
            return found == snapshot.primary().nodes.end() ? nullptr : &*found;
        };
        const auto *resolvedShape = findResolved("shape");
        check(resolvedShape && resolvedShape->drawOrder == 1 && resolvedShape->visible &&
                  resolvedShape->hierarchy.size() == 2 &&
                  std::abs(resolvedShape->inheritedOpacity - .72) < .02 &&
                  resolvedShape->transformChain.size() == 2 && !resolvedShape->sceneBounds.isEmpty(),
              "snapshot resolves group hierarchy, inherited opacity, transforms and bounds");
        check(std::abs(snapshot.primary().camera.zoom - studio::evaluateCamera(mixed, 30).zoom) < 1e-9 &&
                  snapshot.primary().camera.zoom > 1.0,
              "snapshot resolves camera keyframes at the requested frame");
        check(findResolved("svg") && findResolved("image") && findResolved("video") &&
                  findResolved("native-car") && findResolved("caption"),
              "snapshot retains image, SVG, video, native asset and caption references");
        studio::CompositionPainter painter;
        const auto legacy = studio::makeDemo();
        writeOrCompare("legacy-demo.png", painter.frame(legacy, 60), updatePath);
        const auto mixedFrame = painter.frame(mixed, 30);
        writeOrCompare("mixed-frame.png", mixedFrame, updatePath);
        check(painter.frame(mixed, 30) == mixedFrame, "re-rendering an identical frame is deterministic");
        check(studio::validate(mixed).empty(), "mixed fixture remains a valid project");

        auto shotProject = makeShotProject(mixed);
        const auto shotSnapshot = studio::RenderSnapshot::build(shotProject, 34);
        check(shotSnapshot.transition() && shotSnapshot.outgoing() &&
                  shotSnapshot.transition()->type == "dissolve" &&
                  shotSnapshot.transition()->outgoingFrame == 29 &&
                  std::abs(shotSnapshot.transition()->progress - 4.0 / 7.0) < .001,
              "snapshot resolves shot transition and outgoing frame");
        studio::CompositionPainter shotPainter;
        const auto transition = shotPainter.frame(shotProject, 34);
        writeOrCompare("shot-transition.png", transition, updatePath);
        check(shotPainter.frame(shotProject, 34) == transition,
              "shot transition output is deterministic at a fixed frame");

        check(studio::nativeLibrary().size() == 72, "the 72 native renderers remain registered");
        QTextStream(stdout) << "render snapshot checks: " << checks << " passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "render snapshot test failed: " << error.what() << '\n';
        return 1;
    }
}
