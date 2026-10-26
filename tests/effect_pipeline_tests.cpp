#include <studio/composition/effect_pipeline.hpp>

#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QImageReader>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QTemporaryDir>
#include <cmath>
#include <iostream>
#include <stdexcept>

#ifndef STUDIO_EFFECT_REFERENCE
#define STUDIO_EFFECT_REFERENCE "tests/reference/render-effects/pipeline.png"
#endif

namespace {
int checks = 0;
void check(bool condition, const char *message) {
    ++checks;
    if (!condition)
        throw std::runtime_error(message);
}

QImage sourceImage(QSize size) {
    QImage image(size, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(QColor("#e7f3e5"));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(QRectF(size.width() * .33, size.height() * .30, size.width() * .34,
                                   size.height() * .40), 10, 10);
    painter.setBrush(QColor(60, 210, 245, 180));
    painter.drawEllipse(QRectF(size.width() * .52, size.height() * .42, size.width() * .25,
                               size.height() * .30));
    return image;
}

QImage alphaRamp(QSize size) {
    QImage mask(size, QImage::Format_ARGB32_Premultiplied);
    for (int y = 0; y < size.height(); ++y) {
        auto *row = reinterpret_cast<QRgb *>(mask.scanLine(y));
        for (int x = 0; x < size.width(); ++x) {
            const int alpha = qRound(255.0 * x / std::max(1, size.width() - 1));
            row[x] = qRgba(alpha, alpha, alpha, alpha);
        }
    }
    return mask;
}

studio::RenderEffectContext snapshotContext(QSize size, std::int64_t frame) {
    auto project = studio::makeEmpty();
    project.width = size.width();
    project.height = size.height();
    project.duration = std::max<std::int64_t>(120, frame + 1);
    studio::Node node;
    node.id = "effect-layer-context";
    node.name = "Effect layer";
    node.kind = studio::NodeKind::Shape;
    node.start = 0;
    node.duration = project.duration;
    node.width = size.width();
    node.height = size.height();
    node.animated = false;
    project.nodes.push_back(node);
    const auto snapshot = studio::RenderSnapshot::build(project, frame);
    return studio::RenderEffectContext::fromSnapshot(snapshot, snapshot.primary().nodes.front());
}

std::vector<studio::RenderEffect> effectStack(QImage mask) {
    return {{studio::RenderEffectKind::GaussianBlur, 3, Qt::white, {}, 1.0},
            {studio::RenderEffectKind::OuterGlow, 9, QColor("#6fe0ad"), {}, .8},
            {studio::RenderEffectKind::AlphaMask, 0, Qt::white, std::move(mask), .9}};
}

double meanAbsoluteError(const QImage &left, const QImage &right, bool alphaOnly = false) {
    if (left.size() != right.size())
        return 255;
    const auto aImage = left.convertToFormat(QImage::Format_ARGB32);
    const auto bImage = right.convertToFormat(QImage::Format_ARGB32);
    qint64 total = 0;
    const qint64 pixels = qint64(left.width()) * left.height();
    for (int y = 0; y < left.height(); ++y) {
        const auto *a = reinterpret_cast<const QRgb *>(aImage.constScanLine(y));
        const auto *b = reinterpret_cast<const QRgb *>(bImage.constScanLine(y));
        for (int x = 0; x < left.width(); ++x) {
            if (alphaOnly)
                total += std::abs(qAlpha(a[x]) - qAlpha(b[x]));
            else
                total += std::abs(qRed(a[x]) - qRed(b[x])) + std::abs(qGreen(a[x]) - qGreen(b[x])) +
                         std::abs(qBlue(a[x]) - qBlue(b[x]));
        }
    }
    return double(total) / (pixels * (alphaOnly ? 1 : 3));
}

QJsonObject benchmark(int width, int height) {
    if (width < 1 || height < 1)
        throw std::runtime_error("benchmark dimensions must be positive");
    const QSize size(width, height);
    auto source = sourceImage(size);
    auto mask = alphaRamp(QSize(std::max(1, width / 2), std::max(1, height / 2)));
    const auto effects = effectStack(mask);
    const auto context = snapshotContext(size, 30);
    studio::RenderEffectPipeline pipeline;
    const auto warmup = pipeline.execute(source, effects, context);
    if (!warmup.succeeded())
        throw std::runtime_error(warmup.error.toStdString());
    const auto result = pipeline.execute(source, effects, context);
    if (!result.succeeded())
        throw std::runtime_error(result.error.toStdString());
    QJsonArray passTimes;
    for (const auto &timing : result.passTimings)
        passTimes.append(QJsonObject{{"kind", int(timing.kind)},
                                     {"elapsedMilliseconds", double(timing.elapsedNanoseconds) / 1e6}});
    qint64 totalNanos = 0;
    for (const auto &timing : result.passTimings)
        totalNanos += timing.elapsedNanoseconds;
    return {{"width", width},
            {"height", height},
            {"iterations", 1},
            {"totalPassMilliseconds", double(totalNanos) / 1e6},
            {"scratchBytes", result.scratchBytes},
            {"workingSetLimitBytes", result.workingSetLimitBytes},
            {"passes", passTimes}};
}

void runTests(bool updateReference) {
    constexpr QSize size(192, 128);
    const auto source = sourceImage(size);
    const auto context = snapshotContext(size, 42);
    check(context.frame == 42 && context.fps.value() == 30.0 && context.canvasSize == size,
          "effect context carries frame, FPS and canvas resolved by RenderSnapshot");
    studio::RenderEffectPipeline pipeline;
    const auto blurred = pipeline.execute(source, {{studio::RenderEffectKind::GaussianBlur, 6}}, context);
    check(blurred.succeeded(), "Gaussian blur executes on premultiplied RGBA");
    check(qAlpha(blurred.image.pixel(60, 64)) > 0,
          "Gaussian blur softens the edge beyond the original hard rectangle");

    const auto glowing = pipeline.execute(source,
                                          {{studio::RenderEffectKind::OuterGlow, 8, QColor("#70ecb1"), {}, 1.0}},
                                          context);
    check(glowing.succeeded(), "outer glow executes as a reusable effect pass");
    const auto glowPixel = glowing.image.pixelColor(57, 64);
    check(glowPixel.alpha() > 0 && glowPixel.green() > glowPixel.blue(),
          "outer glow extends colored alpha outside the source silhouette");

    QImage opaque(size, QImage::Format_ARGB32_Premultiplied);
    opaque.fill(Qt::white);
    QImage mask(size, QImage::Format_ARGB32_Premultiplied);
    for (int y = 0; y < size.height(); ++y) {
        auto *row = reinterpret_cast<QRgb *>(mask.scanLine(y));
        for (int x = 0; x < size.width(); ++x) {
            const int alpha = x < size.width() / 2 ? 0 : 255;
            row[x] = qRgba(alpha, alpha, alpha, alpha);
        }
    }
    const auto masked = pipeline.execute(opaque,
                                        {{studio::RenderEffectKind::AlphaMask, 0, Qt::white, mask, 1.0}}, context);
    check(masked.succeeded() && qAlpha(masked.image.pixel(24, 64)) == 0 &&
              qAlpha(masked.image.pixel(168, 64)) == 255,
          "alpha mask controls transparent and opaque regions");

    const auto chain = pipeline.execute(source, effectStack(alphaRamp(size)), context);
    check(chain.succeeded() && chain.passTimings.size() == 3,
          "blur, glow and alpha mask share one ordered pass stack");
    check(chain.scratchBytes <= chain.workingSetLimitBytes,
          "reusable intermediate buffers remain within the explicit byte budget");
    for (const auto &timing : chain.passTimings)
        check(timing.elapsedNanoseconds > 0, "each effect pass reports its CPU preparation/render time");

    auto premultiplied = chain.image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    bool validPremultiplied = true;
    for (int y = 0; y < premultiplied.height() && validPremultiplied; ++y) {
        const auto *row = reinterpret_cast<const QRgb *>(premultiplied.constScanLine(y));
        for (int x = 0; x < premultiplied.width(); ++x)
            if (qRed(row[x]) > qAlpha(row[x]) || qGreen(row[x]) > qAlpha(row[x]) ||
                qBlue(row[x]) > qAlpha(row[x])) {
                validPremultiplied = false;
                break;
            }
    }
    check(validPremultiplied, "all effects preserve premultiplied-alpha channel invariants");

    const QString referencePath = QStringLiteral(STUDIO_EFFECT_REFERENCE);
    if (updateReference) {
        QDir().mkpath(QFileInfo(referencePath).absolutePath());
        check(chain.image.save(referencePath), "write the combined-effect golden image");
    } else {
        const QImage reference(referencePath);
        check(!reference.isNull(), "the combined-effect golden image exists");
        check(meanAbsoluteError(reference, chain.image) <= .1 &&
                  meanAbsoluteError(reference, chain.image, true) <= .1,
              "the combined-effect output remains visually consistent with its golden image");
    }
    studio::RenderEffectPipeline tinyBudget(64 * 1024);
    const auto rejected = tinyBudget.execute(QImage(192, 128, QImage::Format_ARGB32_Premultiplied),
                                             {{studio::RenderEffectKind::GaussianBlur, 2}}, context);
    check(!rejected.succeeded() && !rejected.error.isEmpty(),
          "oversized effect buffers fail with a clear error before allocation");
    const auto repeated = pipeline.execute(source, effectStack(alphaRamp(size)), context);
    check(repeated.succeeded() && repeated.image == chain.image &&
              repeated.scratchBytes == chain.scratchBytes,
          "reused scratch storage is deterministic across consecutive frames");

    std::cout << "effect pipeline checks: " << checks << " passed; rgb MAE="
              << meanAbsoluteError(chain.image, QImage(referencePath)) << "; alpha MAE="
              << meanAbsoluteError(chain.image, QImage(referencePath), true) << '\n';
}
} // namespace

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    try {
        if (argc == 4 && QString::fromLocal8Bit(argv[1]) == "--benchmark") {
            bool widthOk = false, heightOk = false;
            const int width = QString::fromLocal8Bit(argv[2]).toInt(&widthOk);
            const int height = QString::fromLocal8Bit(argv[3]).toInt(&heightOk);
            if (!widthOk || !heightOk)
                throw std::runtime_error("invalid benchmark dimensions");
            std::cout << QJsonDocument(benchmark(width, height)).toJson(QJsonDocument::Compact).constData()
                      << '\n';
            return 0;
        }
        const bool updateReference = argc == 2 && QString::fromLocal8Bit(argv[1]) == "--update-reference";
        runTests(updateReference);
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
