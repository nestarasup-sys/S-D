#include <studio/composition/effect_pipeline.hpp>

#include <QElapsedTimer>
#include <QPainter>
#include <algorithm>
#include <array>
#include <cmath>

namespace studio {
namespace {
using Kernel = std::vector<double>;

Kernel gaussianKernel(int radius) {
    const double sigma = std::max(0.5, radius / 3.0);
    Kernel weights(std::size_t(radius * 2 + 1));
    double total = 0;
    for (int offset = -radius; offset <= radius; ++offset) {
        const auto value = std::exp(-(offset * offset) / (2.0 * sigma * sigma));
        weights[std::size_t(offset + radius)] = value;
        total += value;
    }
    for (auto &weight : weights)
        weight /= total;
    return weights;
}

uchar channel(QRgb color, int component) {
    switch (component) {
    case 0: return qRed(color);
    case 1: return qGreen(color);
    case 2: return qBlue(color);
    default: return qAlpha(color);
    }
}

QImage *blurImage(const QImage &source, QImage &horizontal, QImage &output, int radius,
                  const Kernel &weights) {
    const int width = source.width(), height = source.height();
    for (int y = 0; y < height; ++y) {
        const auto *src = reinterpret_cast<const QRgb *>(source.constScanLine(y));
        auto *dst = reinterpret_cast<QRgb *>(horizontal.scanLine(y));
        for (int x = 0; x < width; ++x) {
            std::array<double, 4> sums{};
            for (int tap = -radius; tap <= radius; ++tap) {
                const auto pixel = src[std::clamp(x + tap, 0, width - 1)];
                const auto weight = weights[std::size_t(tap + radius)];
                for (int component = 0; component < 4; ++component)
                    sums[std::size_t(component)] += channel(pixel, component) * weight;
            }
            dst[x] = qRgba(qBound(0, qRound(sums[0]), 255), qBound(0, qRound(sums[1]), 255),
                            qBound(0, qRound(sums[2]), 255), qBound(0, qRound(sums[3]), 255));
        }
    }
    for (int y = 0; y < height; ++y) {
        auto *dst = reinterpret_cast<QRgb *>(output.scanLine(y));
        for (int x = 0; x < width; ++x) {
            std::array<double, 4> sums{};
            for (int tap = -radius; tap <= radius; ++tap) {
                const auto *row = reinterpret_cast<const QRgb *>(
                    horizontal.constScanLine(std::clamp(y + tap, 0, height - 1)));
                const auto pixel = row[x];
                const auto weight = weights[std::size_t(tap + radius)];
                for (int component = 0; component < 4; ++component)
                    sums[std::size_t(component)] += channel(pixel, component) * weight;
            }
            dst[x] = qRgba(qBound(0, qRound(sums[0]), 255), qBound(0, qRound(sums[1]), 255),
                            qBound(0, qRound(sums[2]), 255), qBound(0, qRound(sums[3]), 255));
        }
    }
    return &output;
}

QRgb premultipliedColor(const QColor &color, int alpha) {
    const int finalAlpha = qBound(0, qRound(alpha * color.alphaF()), 255);
    return qRgba(qRound(color.red() * finalAlpha / 255.0), qRound(color.green() * finalAlpha / 255.0),
                 qRound(color.blue() * finalAlpha / 255.0), finalAlpha);
}

QRgb sourceOver(QRgb source, QRgb destination) {
    const double inverseAlpha = 1.0 - qAlpha(source) / 255.0;
    return qRgba(qBound(0, qRound(qRed(source) + qRed(destination) * inverseAlpha), 255),
                 qBound(0, qRound(qGreen(source) + qGreen(destination) * inverseAlpha), 255),
                 qBound(0, qRound(qBlue(source) + qBlue(destination) * inverseAlpha), 255),
                 qBound(0, qRound(qAlpha(source) + qAlpha(destination) * inverseAlpha), 255));
}
} // namespace

RenderEffectPipeline::RenderEffectPipeline(qsizetype workingSetLimitBytes)
    : workingSetLimitBytes_(std::max<qsizetype>(1024, workingSetLimitBytes)) {}

RenderEffectContext RenderEffectContext::fromSnapshot(const RenderSnapshot &snapshot,
                                                      const RenderNodeSnapshot &node) {
    return {snapshot.frame(), snapshot.fps(), QSize(snapshot.width(), snapshot.height()), node.localBounds,
            node.sceneTransform, snapshot.primary().cameraTransform, node.inheritedOpacity};
}

bool RenderEffectPipeline::prepareBuffers(QSize size, qsizetype bytesPerBuffer, QString &error) {
    if (bytesPerBuffer <= 0 || bytesPerBuffer > workingSetLimitBytes_ / 4) {
        error = "O efeito excede o limite de memória da pilha raster.";
        return false;
    }
    for (auto &buffer : buffers_) {
        if (buffer.size() != size || buffer.format() != QImage::Format_ARGB32_Premultiplied)
            buffer = QImage(size, QImage::Format_ARGB32_Premultiplied);
        if (buffer.isNull()) {
            error = "Não foi possível reservar os buffers temporários do efeito.";
            return false;
        }
    }
    return true;
}

QImage *RenderEffectPipeline::availableBuffer(const QImage *input, const QImage *alsoUsed) {
    for (auto &buffer : buffers_)
        if (&buffer != input && &buffer != alsoUsed)
            return &buffer;
    return nullptr;
}

RenderEffectResult RenderEffectPipeline::execute(const QImage &source,
                                                 const std::vector<RenderEffect> &effects,
                                                 const RenderEffectContext &context) {
    RenderEffectResult result;
    result.workingSetLimitBytes = workingSetLimitBytes_;
    result.frame = context.frame;
    if (context.frame < 0 || context.fps.numerator <= 0 || context.fps.denominator <= 0 ||
        context.canvasSize.isEmpty()) {
        result.error = "O contexto de efeitos precisa de frame, FPS e canvas válidos do RenderSnapshot.";
        return result;
    }
    if (source.isNull()) {
        result.error = "A camada de origem do efeito está vazia.";
        return result;
    }
    const auto bytesPerBuffer = qsizetype(source.width()) * source.height() * 4;
    if (!prepareBuffers(source.size(), bytesPerBuffer, result.error))
        return result;
    result.scratchBytes = bytesPerBuffer * 3;
    const QImage *current = &source;
    if (source.format() != QImage::Format_ARGB32_Premultiplied) {
        auto &normalized = buffers_[0];
        normalized.fill(Qt::transparent);
        QPainter painter(&normalized);
        if (!painter.isActive()) {
            result.error = "A camada não pôde ser normalizada para alpha premultiplicado.";
            return result;
        }
        painter.drawImage(QPoint(0, 0), source);
        painter.end();
        current = &normalized;
    }

    for (const auto &effect : effects) {
        QElapsedTimer timer;
        timer.start();
        if (effect.kind == RenderEffectKind::GaussianBlur || effect.kind == RenderEffectKind::OuterGlow) {
            if (effect.radius < 1 || effect.radius > 64) {
                result.error = "O raio de desfoque deve estar entre 1 e 64 pixels.";
                return result;
            }
            const auto weights = gaussianKernel(effect.radius);
            auto *horizontal = availableBuffer(current);
            auto *output = availableBuffer(current, horizontal);
            if (!horizontal || !output) {
                result.error = "Não há buffers livres para executar o desfoque.";
                return result;
            }
            auto *filtered = blurImage(*current, *horizontal, *output, effect.radius, weights);
            if (effect.kind == RenderEffectKind::GaussianBlur) {
                current = filtered;
            } else {
                const double strength = std::clamp(effect.strength, 0.0, 1.0);
                const QColor glowColor = effect.color;
                for (int y = 0; y < source.height(); ++y) {
                    const auto *original = reinterpret_cast<const QRgb *>(current->constScanLine(y));
                    const auto *blurred = reinterpret_cast<const QRgb *>(filtered->constScanLine(y));
                    auto *composited = reinterpret_cast<QRgb *>(horizontal->scanLine(y));
                    for (int x = 0; x < source.width(); ++x) {
                        const int alpha = qRound(qAlpha(blurred[x]) * strength);
                        composited[x] = sourceOver(original[x], premultipliedColor(glowColor, alpha));
                    }
                }
                current = horizontal;
            }
        } else if (effect.kind == RenderEffectKind::AlphaMask) {
            if (effect.alphaMask.isNull()) {
                result.error = "A máscara alfa não contém pixels.";
                return result;
            }
            const double strength = std::clamp(effect.strength, 0.0, 1.0);
            auto *output = availableBuffer(current);
            if (!output) {
                result.error = "Não há buffer livre para aplicar a máscara alfa.";
                return result;
            }
            for (int y = 0; y < source.height(); ++y) {
                const auto *inputRow = reinterpret_cast<const QRgb *>(current->constScanLine(y));
                const int maskY = std::min(effect.alphaMask.height() - 1,
                                           int((qint64(y) * effect.alphaMask.height()) / source.height()));
                auto *outputRow = reinterpret_cast<QRgb *>(output->scanLine(y));
                for (int x = 0; x < source.width(); ++x) {
                    const int maskX = std::min(effect.alphaMask.width() - 1,
                                               int((qint64(x) * effect.alphaMask.width()) / source.width()));
                    const double maskAlpha = qAlpha(effect.alphaMask.pixel(maskX, maskY)) / 255.0;
                    const double factor = 1.0 - strength * (1.0 - maskAlpha);
                    outputRow[x] = qRgba(qRound(qRed(inputRow[x]) * factor),
                                         qRound(qGreen(inputRow[x]) * factor),
                                         qRound(qBlue(inputRow[x]) * factor),
                                         qRound(qAlpha(inputRow[x]) * factor));
                }
            }
            current = output;
        } else {
            result.error = "Tipo de efeito desconhecido.";
            return result;
        }
        result.passTimings.push_back({effect.kind, timer.nsecsElapsed()});
    }

    result.image = current->copy();
    if (result.image.isNull())
        result.error = "Falha ao preservar a imagem final do efeito.";
    return result;
}

} // namespace studio
