#pragma once

#include <QColor>
#include <QImage>
#include <QString>
#include <cstdint>
#include <vector>
#include <studio/composition/render_snapshot.hpp>

namespace studio {

enum class RenderEffectKind { GaussianBlur, OuterGlow, AlphaMask };

// Effects operate on a rasterized layer, independent of whether it came from
// text, a native asset, media, a group or a future 2.5D plane.
struct RenderEffect {
    RenderEffectKind kind{RenderEffectKind::GaussianBlur};
    int radius{0};
    QColor color{Qt::white};
    QImage alphaMask;
    double strength{1.0};
};

struct RenderEffectTiming {
    RenderEffectKind kind{RenderEffectKind::GaussianBlur};
    std::int64_t elapsedNanoseconds{0};
};

struct RenderEffectContext {
    std::int64_t frame{0};
    FrameRate fps{};
    QSize canvasSize;
    QRectF localBounds;
    QTransform sceneTransform;
    QTransform cameraTransform;
    double inheritedOpacity{1.0};

    static RenderEffectContext fromSnapshot(const RenderSnapshot &, const RenderNodeSnapshot &);
};

struct RenderEffectResult {
    QImage image;
    std::vector<RenderEffectTiming> passTimings;
    qsizetype scratchBytes{0};
    qsizetype workingSetLimitBytes{0};
    std::int64_t frame{0};
    QString error;

    bool succeeded() const { return error.isEmpty() && !image.isNull(); }
};

// One pipeline instance belongs to one render thread. Its three intermediate
// QImages are reused, and the pool plus returned image stay inside the byte
// limit. It is a deterministic CPU fallback for the experimental effect graph.
class RenderEffectPipeline {
  public:
    explicit RenderEffectPipeline(qsizetype workingSetLimitBytes = 64 * 1024 * 1024);
    RenderEffectPipeline(const RenderEffectPipeline &) = delete;
    RenderEffectPipeline &operator=(const RenderEffectPipeline &) = delete;

    RenderEffectResult execute(const QImage &source, const std::vector<RenderEffect> &effects,
                               const RenderEffectContext &context);

  private:
    bool prepareBuffers(QSize size, qsizetype bytesPerBuffer, QString &error);
    QImage *availableBuffer(const QImage *input, const QImage *alsoUsed = nullptr);

    qsizetype workingSetLimitBytes_{0};
    QImage buffers_[3];
};

} // namespace studio
