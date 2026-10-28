#pragma once

#include <QImage>
#include <QMutex>
#include <QQuickRhiItem>
#include <QWaitCondition>
#include <chrono>
#include <memory>
#include <vector>
#include <studio/composition/effect_pipeline.hpp>

struct GpuEffectTimings {
    qint64 textureUploadPreparationNanoseconds{0};
    qint64 renderPassSubmissionNanoseconds{0};
    qint64 readbackLatencyNanoseconds{0};
    qint64 completedGpuFrameNanoseconds{0};
};

class GpuEffectReadbackState final {
  public:
    void publish(QImage image, GpuEffectTimings timings, QString error = {});
    bool wait(QImage &image, GpuEffectTimings &timings, QString &error, int timeoutMs);

  private:
    QMutex mutex_;
    QWaitCondition ready_;
    QImage image_;
    GpuEffectTimings timings_;
    QString error_;
    bool complete_{false};
};

class GpuEffectItem final : public QQuickRhiItem {
    Q_OBJECT
  public:
    enum class Kind { AlphaMask, GaussianBlur, OuterGlow, EffectStack };

    explicit GpuEffectItem(QQuickItem *parent = nullptr);
    void setInputs(QImage source, QImage mask = {});
    void setEffect(Kind kind, int radius = 0, QColor color = Qt::white, double strength = 1.0);
    void setEffectStack(std::vector<studio::RenderEffect> effects);
    void setReadbackState(std::shared_ptr<GpuEffectReadbackState> state);

  protected:
    QQuickRhiItemRenderer *createRenderer() override;

  private:
    friend class GpuEffectItemRenderer;
    QImage source_;
    QImage mask_;
    Kind kind_{Kind::AlphaMask};
    int radius_{0};
    QColor color_{Qt::white};
    float strength_{1.0f};
    std::vector<studio::RenderEffect> effectStack_;
    bool dynamicStackActive_{false};
    quint64 stackRevision_{0};
    quint64 revision_{0};
    std::shared_ptr<GpuEffectReadbackState> readback_;
};
