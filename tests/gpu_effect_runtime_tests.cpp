#include "gpu_effect_item.hpp"

#include <studio/composition/effect_pipeline.hpp>

#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QQuickWindow>
#include <QQuickGraphicsConfiguration>
#include <QSGRendererInterface>
#include <QThread>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>

namespace {
struct Metrics {
    double psnr{0};
    double mae{0};
    double alphaMae{0};
    double edgeF1{0};
};

QImage makeSource(QSize size) {
    QImage image(size, QImage::Format_RGBA8888_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(QRect(size.width() / 12, size.height() / 8, size.width() / 3, size.height() / 2),
                     QColor(36, 117, 185, 205));
    painter.setBrush(QColor(242, 172, 69, 230));
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(QRect(size.width() / 3, size.height() / 3, size.width() / 3, size.height() / 3));
    painter.setPen(QPen(QColor(244, 244, 238, 238), std::max(2, size.width() / 160)));
    painter.drawText(QRect(size.width() / 10, size.height() * 2 / 3, size.width() * 4 / 5, size.height() / 5),
                     Qt::AlignCenter, QStringLiteral("GPU • TEXTO • PNG"));
    return image;
}

QImage makeMask(QSize size) {
    QImage image(size, QImage::Format_RGBA8888_Premultiplied);
    image.fill(Qt::transparent);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    QLinearGradient gradient(0, 0, size.width(), size.height());
    gradient.setColorAt(0.0, QColor(255, 255, 255, 30));
    gradient.setColorAt(0.35, QColor(255, 255, 255, 255));
    gradient.setColorAt(0.75, QColor(255, 255, 255, 150));
    gradient.setColorAt(1.0, QColor(255, 255, 255, 0));
    painter.setBrush(gradient);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(QRectF(size.width() * .08, size.height() * .08,
                                  size.width() * .84, size.height() * .84),
                            size.width() * .12, size.width() * .12);
    return image;
}

bool edgeAt(const QImage &image, int x, int y) {
    // Alpha 0.5 is the silhouette threshold; lower values are soft halo pixels.
    const auto alpha = [&](int px, int py) { return image.pixelColor(px, py).alpha() > 127; };
    const bool value = alpha(x, y);
    for (int dy = -1; dy <= 1; ++dy)
        for (int dx = -1; dx <= 1; ++dx)
            if ((dx || dy) && alpha(std::clamp(x + dx, 0, image.width() - 1),
                                    std::clamp(y + dy, 0, image.height() - 1)) != value)
                return true;
    return false;
}

Metrics compare(const QImage &actual, const QImage &expected) {
    Metrics result;
    if (actual.size() != expected.size() || actual.isNull() || expected.isNull())
        return result;
    double squared = 0;
    quint64 absolute = 0, alphaAbsolute = 0, matchedActualEdges = 0, matchedExpectedEdges = 0;
    quint64 actualEdges = 0, expectedEdges = 0;
    const auto a = actual.convertToFormat(QImage::Format_RGBA8888_Premultiplied);
    const auto b = expected.convertToFormat(QImage::Format_RGBA8888_Premultiplied);
    for (int y = 0; y < a.height(); ++y) {
        const auto *ar = a.constScanLine(y);
        const auto *br = b.constScanLine(y);
        for (int x = 0; x < a.width(); ++x) {
            for (int c = 0; c < 4; ++c) {
                const int delta = int(ar[x * 4 + c]) - int(br[x * 4 + c]);
                absolute += std::abs(delta);
                squared += double(delta) * delta;
                if (c == 3)
                    alphaAbsolute += std::abs(delta);
            }
            const bool actualEdge = edgeAt(a, x, y), expectedEdge = edgeAt(b, x, y);
            actualEdges += actualEdge;
            expectedEdges += expectedEdge;
            bool nearActual = false, nearExpected = false;
            for (int dy = -1; dy <= 1; ++dy)
                for (int dx = -1; dx <= 1; ++dx) {
                    const int sx = x + dx, sy = y + dy;
                    if (sx >= 0 && sy >= 0 && sx < a.width() && sy < a.height()) {
                        nearActual = nearActual || edgeAt(a, sx, sy);
                        nearExpected = nearExpected || edgeAt(b, sx, sy);
                    }
                }
            matchedActualEdges += actualEdge && nearExpected;
            matchedExpectedEdges += expectedEdge && nearActual;
        }
    }
    const double count = double(a.width()) * a.height() * 4.0;
    const double mse = squared / count;
    result.psnr = mse == 0 ? 99.0 : 10.0 * std::log10(255.0 * 255.0 / mse);
    result.mae = double(absolute) / count;
    result.alphaMae = double(alphaAbsolute) / (double(a.width()) * a.height());
    const double precision = actualEdges ? double(matchedActualEdges) / actualEdges : 1.0;
    const double recall = expectedEdges ? double(matchedExpectedEdges) / expectedEdges : 1.0;
    result.edgeF1 = precision + recall > 0 ? 2.0 * precision * recall / (precision + recall) : 1.0;
    return result;
}

QImage difference(const QImage &a, const QImage &b) {
    QImage result(a.size(), QImage::Format_RGBA8888_Premultiplied);
    result.fill(Qt::transparent);
    for (int y = 0; y < a.height(); ++y) {
        for (int x = 0; x < a.width(); ++x) {
            const auto ac = a.pixelColor(x, y), bc = b.pixelColor(x, y);
            result.setPixelColor(x, y, QColor(std::abs(ac.red() - bc.red()),
                                             std::abs(ac.green() - bc.green()),
                                             std::abs(ac.blue() - bc.blue()),
                                             std::abs(ac.alpha() - bc.alpha())));
        }
    }
    return result;
}

QString effectName(GpuEffectItem::Kind kind) {
    switch (kind) {
    case GpuEffectItem::Kind::AlphaMask: return QStringLiteral("alpha-mask");
    case GpuEffectItem::Kind::GaussianBlur: return QStringLiteral("gaussian-blur");
    case GpuEffectItem::Kind::OuterGlow: return QStringLiteral("outer-glow");
    case GpuEffectItem::Kind::EffectStack: return QStringLiteral("stack-glow-blur-mask");
    }
    return QStringLiteral("unknown");
}

bool waitForResult(const std::shared_ptr<GpuEffectReadbackState> &state, QImage &image,
                   GpuEffectTimings &timings, QString &error) {
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < 20000) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
        if (state->wait(image, timings, error, 50))
            return true;
    }
    return false;
}

QImage cpuEffect(QImage source, QImage mask, GpuEffectItem::Kind kind, QSize size, int radius) {
    std::vector<studio::RenderEffect> effects;
    if (kind == GpuEffectItem::Kind::EffectStack) {
        effects = {{studio::RenderEffectKind::OuterGlow, radius, QColor(93, 165, 255, 220), {}, .75},
                   {studio::RenderEffectKind::GaussianBlur, radius},
                   {studio::RenderEffectKind::AlphaMask, 0, Qt::white, mask, .75}};
    } else {
        studio::RenderEffect effect;
        effect.radius = radius;
        effect.alphaMask = mask;
        effect.color = QColor(93, 165, 255, 220);
        effect.strength = .75;
        if (kind == GpuEffectItem::Kind::AlphaMask)
            effect.kind = studio::RenderEffectKind::AlphaMask;
        else if (kind == GpuEffectItem::Kind::GaussianBlur)
            effect.kind = studio::RenderEffectKind::GaussianBlur;
        else
            effect.kind = studio::RenderEffectKind::OuterGlow;
        effects.push_back(std::move(effect));
    }
    studio::RenderEffectPipeline pipeline(64 * 1024 * 1024);
    studio::RenderEffectContext context;
    context.frame = 0;
    context.fps = {30, 1};
    context.canvasSize = size;
    const auto result = pipeline.execute(source, effects, context);
    if (!result.succeeded())
        return {};
    return result.image;
}

} // namespace

int main(int argc, char **argv) {
    QQuickWindow::setGraphicsApi(QSGRendererInterface::Direct3D11);
    QGuiApplication app(argc, argv);
    QDir().mkpath(QStringLiteral(STUDIO_GPU_EFFECT_ARTIFACT_DIR));
    QDir().mkpath(QStringLiteral(STUDIO_GPU_EFFECT_REFERENCE_DIR));

    QQuickWindow window;
    QQuickGraphicsConfiguration graphicsConfiguration;
    graphicsConfiguration.setTimestamps(true);
    window.setGraphicsConfiguration(graphicsConfiguration);
    window.setTitle(QStringLiteral("Studio Dark GPU effect verification"));
    window.setColor(Qt::transparent);
    auto *item = new GpuEffectItem(window.contentItem());
    window.setWidth(640);
    window.setHeight(360);
    item->setWidth(640);
    item->setHeight(360);
    window.show();

    const QVector<QSize> sizes{{1280, 720}, {1920, 1080}, {1080, 1920}};
    const QVector<GpuEffectItem::Kind> effects{GpuEffectItem::Kind::AlphaMask,
                                               GpuEffectItem::Kind::GaussianBlur,
                                               GpuEffectItem::Kind::OuterGlow,
                                               GpuEffectItem::Kind::EffectStack};
    QJsonArray results;
    bool passed = true;
    const bool updateGoldens = qEnvironmentVariableIsSet("STUDIO_UPDATE_GPU_EFFECT_GOLDENS");
    for (const auto size : sizes) {
        QElapsedTimer sourceRasterTimer;
        sourceRasterTimer.start();
        const QImage source = makeSource(size);
        const QImage mask = makeMask(size);
        const auto sourceRasterNanoseconds = sourceRasterTimer.nsecsElapsed();
        item->setFixedColorBufferWidth(size.width());
        item->setFixedColorBufferHeight(size.height());
        item->setWidth(640);
        item->setHeight(360);
        window.setWidth(640);
        window.setHeight(360);
        for (const auto effect : effects) {
            const int radius = effect == GpuEffectItem::Kind::AlphaMask ? 0 : 12;
            const auto state = std::make_shared<GpuEffectReadbackState>();
            QElapsedTimer inputPreparationTimer;
            inputPreparationTimer.start();
            item->setInputs(source, mask);
            item->setEffect(effect, radius, QColor(93, 165, 255, 220), .75);
            item->setReadbackState(state);
            const auto inputPreparationNanoseconds = inputPreparationTimer.nsecsElapsed();
            QImage gpu;
            GpuEffectTimings timings;
            QString error;
            if (!waitForResult(state, gpu, timings, error)) {
                std::cerr << "GPU effect timed out at " << size.width() << 'x' << size.height() << " "
                          << effectName(effect).toStdString() << ": " << error.toStdString() << '\n';
                return 2;
            }
            if (!error.isEmpty() || gpu.size() != size) {
                std::cerr << "GPU effect failed at " << size.width() << 'x' << size.height() << " "
                          << effectName(effect).toStdString() << ": " << error.toStdString() << " size="
                          << gpu.width() << 'x' << gpu.height() << '\n';
                return 3;
            }
            QElapsedTimer cpuReferenceTimer;
            cpuReferenceTimer.start();
            const auto cpu = cpuEffect(source, mask, effect, size, radius);
            const auto cpuReferenceNanoseconds = cpuReferenceTimer.nsecsElapsed();
            if (cpu.isNull())
                return 4;
            const auto name = effectName(effect) + QStringLiteral("-%1x%2").arg(size.width()).arg(size.height());
            const auto goldenPath = QStringLiteral(STUDIO_GPU_EFFECT_REFERENCE_DIR) + '/' + name + QStringLiteral(".png");
            if (updateGoldens && !cpu.save(goldenPath, "PNG"))
                return 5;
            const QImage golden(goldenPath);
            if (golden.isNull()) {
                std::cerr << "Missing CPU golden " << goldenPath.toStdString()
                          << "; run with STUDIO_UPDATE_GPU_EFFECT_GOLDENS=1 to create it after review.\n";
                return 6;
            }
            const auto metrics = compare(gpu, golden);
            const auto artifactPath = QStringLiteral(STUDIO_GPU_EFFECT_ARTIFACT_DIR) + '/' + name;
            gpu.save(artifactPath + QStringLiteral("-gpu.png"));
            difference(gpu, golden).save(artifactPath + QStringLiteral("-difference.png"));
            const bool quality = metrics.psnr >= 38.0 && metrics.mae <= 1.25 &&
                                 metrics.alphaMae <= 1.0 && metrics.edgeF1 >= .96;
            passed = passed && quality;
            QJsonObject row{{"effect", effectName(effect)},
                            {"width", size.width()}, {"height", size.height()},
                            {"gpuTextureAllocationEstimateBytes",
                             double(qint64(size.width()) * size.height() * 4 * 5)},
                            {"gpuTextureBudgetBytes", 64 * 1024 * 1024},
                            {"psnrDb", metrics.psnr}, {"maeRgba", metrics.mae},
                            {"alphaMae", metrics.alphaMae}, {"edgeF1", metrics.edgeF1},
                            {"readbackBytes", double(gpu.sizeInBytes())},
                            {"inputPreparationNanoseconds", double(inputPreparationNanoseconds)},
                            {"sourceRasterizationNanoseconds", double(sourceRasterNanoseconds)},
                            {"cpuReferenceNanoseconds", double(cpuReferenceNanoseconds)},
                            {"textureUploadPreparationNanoseconds", double(timings.textureUploadPreparationNanoseconds)},
                            {"renderPassSubmissionNanoseconds", double(timings.renderPassSubmissionNanoseconds)},
                            {"readbackLatencyNanoseconds", double(timings.readbackLatencyNanoseconds)},
                            {"completedGpuFrameNanoseconds", double(timings.completedGpuFrameNanoseconds)},
                            {"gpuTimestampAvailable", timings.completedGpuFrameNanoseconds > 0},
                            {"qualityPassed", quality}};
            results.append(row);
            std::cout << size.width() << 'x' << size.height() << ' ' << effectName(effect).toStdString()
                      << " PSNR=" << metrics.psnr << " MAE=" << metrics.mae
                      << " alphaMAE=" << metrics.alphaMae << " edgeF1=" << metrics.edgeF1 << '\n';
        }
    }
    const QSize validationSize(64, 64);
    item->setFixedColorBufferWidth(validationSize.width());
    item->setFixedColorBufferHeight(validationSize.height());
    item->setInputs(makeSource(validationSize), makeMask(validationSize));
    item->setEffect(GpuEffectItem::Kind::GaussianBlur, 25);
    auto invalidRadiusState = std::make_shared<GpuEffectReadbackState>();
    item->setReadbackState(invalidRadiusState);
    QImage ignoredImage;
    GpuEffectTimings ignoredTimings;
    QString invalidRadiusError;
    if (!waitForResult(invalidRadiusState, ignoredImage, ignoredTimings, invalidRadiusError) ||
        !invalidRadiusError.contains(QStringLiteral("[1, 24]"))) {
        std::cerr << "GPU blur radius validation failed: " << invalidRadiusError.toStdString() << '\n';
        return 8;
    }
    item->setInputs(makeSource(validationSize), {});
    item->setEffect(GpuEffectItem::Kind::AlphaMask, 0);
    auto missingMaskState = std::make_shared<GpuEffectReadbackState>();
    item->setReadbackState(missingMaskState);
    QString missingMaskError;
    if (!waitForResult(missingMaskState, ignoredImage, ignoredTimings, missingMaskError) ||
        !missingMaskError.contains(QStringLiteral("Alpha mask is missing"))) {
        std::cerr << "GPU mask validation failed: " << missingMaskError.toStdString() << '\n';
        return 9;
    }
    QJsonObject report{{"status", passed ? "passed" : "failed"},
                        {"backend", window.rendererInterface()->graphicsApi() == QSGRendererInterface::Direct3D11
                                        ? "Direct3D11" : "other"},
                        {"runtimeGpuEffects", true},
                        {"invalidRadiusRejected", true},
                        {"missingAlphaMaskRejected", true},
                        {"sourceContains", QJsonArray{"transparent image", "vector shapes", "text"}},
                        {"results", results}};
    QFile reportFile(QStringLiteral(STUDIO_GPU_EFFECT_ARTIFACT_DIR) + QStringLiteral("/report.json"));
    if (reportFile.open(QIODevice::WriteOnly | QIODevice::Truncate))
        reportFile.write(QJsonDocument(report).toJson(QJsonDocument::Indented));
    return passed ? 0 : 7;
}
