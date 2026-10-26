#include "gpu_effect_item.hpp"

#include <QFile>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QVector4D>
#include <rhi/qrhi.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <memory>

namespace {
constexpr qsizetype kGpuBudgetBytes = 64 * 1024 * 1024;
constexpr int kMaximumBlurRadius = 24;

enum class Pass { Mask, BlurHorizontal, BlurVertical, GlowComposite };

struct alignas(16) EffectUniforms {
    float params[4]{1.0f, 1.0f, 0.0f, 0.0f};
    float color[4]{1.0f, 1.0f, 1.0f, 1.0f};
};
static_assert(sizeof(EffectUniforms) == 32);

struct Vertex {
    float x, y, u, v;
};
constexpr std::array<Vertex, 4> kQuad{{
    {-1.0f, 1.0f, 0.0f, 0.0f}, {-1.0f, -1.0f, 0.0f, 1.0f},
    {1.0f, 1.0f, 1.0f, 0.0f}, {1.0f, -1.0f, 1.0f, 1.0f},
}};

QShader loadShader(const QString &path) {
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? QShader::fromSerialized(file.readAll()) : QShader{};
}

QString fragmentPath(Pass pass) {
    switch (pass) {
    case Pass::Mask: return QStringLiteral(":/gpu-effects/alpha_mask.frag.qsb");
    case Pass::BlurHorizontal: return QStringLiteral(":/gpu-effects/gaussian_blur_horizontal.frag.qsb");
    case Pass::BlurVertical: return QStringLiteral(":/gpu-effects/gaussian_blur_vertical.frag.qsb");
    case Pass::GlowComposite: return QStringLiteral(":/gpu-effects/glow_composite.frag.qsb");
    }
    return {};
}

} // namespace

class GpuEffectItemRenderer final : public QQuickRhiItemRenderer {
  public:
    ~GpuEffectItemRenderer() override = default;

    void initialize(QRhiCommandBuffer *) override {
        if (rhi_ == rhi() && vertexBuffer_)
            return;
        releaseResources();
        rhi_ = rhi();
        if (!rhi_)
            return;
        vertexBuffer_.reset(rhi_->newBuffer(QRhiBuffer::Immutable, QRhiBuffer::VertexBuffer,
                                            sizeof(kQuad)));
        for (auto &uniformBuffer : uniformBuffers_)
            uniformBuffer.reset(rhi_->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer,
                                                sizeof(EffectUniforms)));
        vertexBuffer_->create();
        for (auto &uniformBuffer : uniformBuffers_)
            uniformBuffer->create();
        auto *updates = rhi_->nextResourceUpdateBatch();
        updates->uploadStaticBuffer(vertexBuffer_.get(), kQuad.data());
        // Keep the batch until render(), where the command buffer can consume it.
        pendingUpdates_.reset(updates);
    }

    void synchronize(QQuickRhiItem *base) override {
        auto *item = static_cast<GpuEffectItem *>(base);
        if (readback_ != item->readback_)
            readbackQueued_ = false;
        source_ = item->source_;
        mask_ = item->mask_;
        kind_ = item->kind_;
        radius_ = item->radius_;
        color_ = item->color_;
        strength_ = item->strength_;
        revision_ = item->revision_;
        readback_ = item->readback_;
    }

    void render(QRhiCommandBuffer *cb) override {
        if (!rhi_ || source_.isNull()) {
            if (readback_)
                readback_->publish({}, {}, QStringLiteral("GPU effect has no source image"));
            return;
        }
        QString error;
        if (!ensureResources(error)) {
            if (readback_)
                readback_->publish({}, {}, error);
            return;
        }

        auto *updates = pendingUpdates_ ? pendingUpdates_.release() : rhi_->nextResourceUpdateBatch();
        QElapsedTimer uploadPreparationTimer;
        uploadPreparationTimer.start();
        qint64 uploadPreparationNanoseconds = 0;
        if (uploadedRevision_ != revision_) {
            updates->uploadTexture(sourceTexture_.get(), source_);
            QImage opaqueMask;
            if (mask_.isNull()) {
                opaqueMask = QImage(1, 1, QImage::Format_RGBA8888_Premultiplied);
                opaqueMask.fill(Qt::white);
            }
            updates->uploadTexture(maskTexture_.get(), mask_.isNull() ? opaqueMask : mask_);
            uploadedRevision_ = revision_;
            uploadPreparationNanoseconds = uploadPreparationTimer.nsecsElapsed();
        }
        QElapsedTimer submissionTimer;
        submissionTimer.start();
        if (kind_ == GpuEffectItem::Kind::GaussianBlur) {
            writeUniformsToBatch(updates, uniformBuffers_[1].get(), Pass::BlurHorizontal);
            runPass(cb, updates, horizontalPipeline_.get(), horizontalSrb_.get(), scratchTarget_[0].get(),
                    Pass::BlurHorizontal);
            updates = nullptr;
            auto *secondUpdates = rhi_->nextResourceUpdateBatch();
            writeUniformsToBatch(secondUpdates, uniformBuffers_[2].get(), Pass::BlurVertical);
            runPass(cb, secondUpdates, verticalPipeline_.get(), verticalSrb_.get(), renderTarget(),
                    Pass::BlurVertical);
        } else if (kind_ == GpuEffectItem::Kind::OuterGlow) {
            writeUniformsToBatch(updates, uniformBuffers_[1].get(), Pass::BlurHorizontal);
            runPass(cb, updates, horizontalPipeline_.get(), alphaHorizontalSrb_.get(), scratchTarget_[0].get(),
                    Pass::BlurHorizontal);
            updates = nullptr;
            auto *verticalUpdates = rhi_->nextResourceUpdateBatch();
            writeUniformsToBatch(verticalUpdates, uniformBuffers_[2].get(), Pass::BlurVertical);
            runPass(cb, verticalUpdates, verticalPipeline_.get(), alphaVerticalSrb_.get(), scratchTarget_[1].get(),
                    Pass::BlurVertical);
            auto *compositeUpdates = rhi_->nextResourceUpdateBatch();
            writeUniformsToBatch(compositeUpdates, uniformBuffers_[3].get(), Pass::GlowComposite);
            runPass(cb, compositeUpdates, glowPipeline_.get(), glowSrb_.get(), renderTarget(), Pass::GlowComposite);
        } else {
            writeUniformsToBatch(updates, uniformBuffers_[0].get(), Pass::Mask);
            runPass(cb, updates, maskPipeline_.get(), maskSrb_.get(), renderTarget(), Pass::Mask);
            updates = nullptr;
        }

        if (readback_ && !readbackQueued_) {
            auto result = std::make_shared<QRhiReadbackResult>();
            const auto state = readback_;
            const QSize readbackSize = source_.size();
            const auto submissionNanoseconds = submissionTimer.nsecsElapsed();
            const auto submittedAt = std::chrono::steady_clock::now();
            auto *commandBuffer = cb;
            const bool timestampSupported = rhi_->isFeatureSupported(QRhi::Timestamps);
            auto *resultData = result.get();
            result->completed = [resultData, state, readbackSize, submittedAt,
                                 uploadPreparationNanoseconds, submissionNanoseconds, commandBuffer,
                                 timestampSupported] {
                GpuEffectTimings timings;
                timings.textureUploadPreparationNanoseconds = uploadPreparationNanoseconds;
                timings.renderPassSubmissionNanoseconds = submissionNanoseconds;
                timings.readbackLatencyNanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::steady_clock::now() - submittedAt).count();
                if (timestampSupported)
                    timings.completedGpuFrameNanoseconds = qint64(commandBuffer->lastCompletedGpuTime() * 1e9);
                if (resultData->data.size() < qsizetype(readbackSize.width()) * readbackSize.height() * 4) {
                    state->publish({}, timings, QStringLiteral("QRhi returned an incomplete texture readback"));
                    return;
                }
                QImage image(reinterpret_cast<const uchar *>(resultData->data.constData()), readbackSize.width(),
                             readbackSize.height(), QImage::Format_RGBA8888_Premultiplied);
                state->publish(image.copy(), timings);
            };
            pendingReadback_ = result;
            auto *readbackUpdates = rhi_->nextResourceUpdateBatch();
            readbackUpdates->readBackTexture(QRhiReadbackDescription(colorTexture()), resultData);
            cb->resourceUpdate(readbackUpdates);
            readbackQueued_ = true;
            update();
        }
    }

  private:
    void releaseResources() {
        pipeline_.reset();
        maskPipeline_.reset();
        horizontalPipeline_.reset();
        verticalPipeline_.reset();
        glowPipeline_.reset();
        maskSrb_.reset();
        horizontalSrb_.reset();
        verticalSrb_.reset();
        alphaHorizontalSrb_.reset();
        alphaVerticalSrb_.reset();
        glowSrb_.reset();
        vertexBuffer_.reset();
        for (auto &uniformBuffer : uniformBuffers_)
            uniformBuffer.reset();
        sampler_.reset();
        sourceTexture_.reset();
        maskTexture_.reset();
        for (int i = 0; i < 2; ++i) {
            scratchTarget_[i].reset();
            scratchPass_[i].reset();
            scratchTexture_[i].reset();
        }
        pendingUpdates_.reset();
        rhi_ = nullptr;
        pipelineOutputSize_ = {};
    }

    bool ensureResources(QString &error) {
        if (radius_ < 1 || radius_ > kMaximumBlurRadius) {
            if (kind_ != GpuEffectItem::Kind::AlphaMask) {
                error = QStringLiteral("GPU blur radius must be in [1, 24]");
                return false;
            }
        }
        if (kind_ == GpuEffectItem::Kind::AlphaMask && mask_.isNull()) {
            error = QStringLiteral("Alpha mask is missing");
            return false;
        }
        const auto pixelCount = qint64(source_.width()) * source_.height();
        const auto maskPixelCount = mask_.isNull() ? qint64(1) :
            qint64(mask_.width()) * mask_.height();
        const auto requiredBytes = pixelCount * 4 * 4 + maskPixelCount * 4;
        if (pixelCount <= 0 || requiredBytes > kGpuBudgetBytes) {
            error = QStringLiteral("GPU effect exceeds the 64 MiB texture budget");
            return false;
        }
        const QSize requestedMaskSize = mask_.isNull() ? QSize(1, 1) : mask_.size();
        if (!sourceTexture_ || sourceTexture_->pixelSize() != source_.size() ||
            !maskTexture_ || maskTexture_->pixelSize() != requestedMaskSize) {
            releaseTargetsAndPipelines();
            sourceTexture_.reset(rhi_->newTexture(QRhiTexture::RGBA8, source_.size(), 1));
            maskTexture_.reset(rhi_->newTexture(QRhiTexture::RGBA8, requestedMaskSize, 1));
            scratchTexture_[0].reset(rhi_->newTexture(QRhiTexture::RGBA8, source_.size(), 1,
                                                       QRhiTexture::RenderTarget));
            scratchTexture_[1].reset(rhi_->newTexture(QRhiTexture::RGBA8, source_.size(), 1,
                                                       QRhiTexture::RenderTarget));
            for (auto *texture : {sourceTexture_.get(), maskTexture_.get(), scratchTexture_[0].get(),
                                  scratchTexture_[1].get()}) {
                if (!texture || !texture->create()) {
                    error = QStringLiteral("QRhi could not allocate GPU effect textures");
                    return false;
                }
            }
            for (int i = 0; i < 2; ++i) {
                auto *target = rhi_->newTextureRenderTarget(QRhiTextureRenderTargetDescription(
                    QRhiColorAttachment(scratchTexture_[i].get())));
                scratchTarget_[i].reset(target);
                scratchPass_[i].reset(target->newCompatibleRenderPassDescriptor());
                target->setRenderPassDescriptor(scratchPass_[i].get());
                if (!target->create()) {
                    error = QStringLiteral("QRhi could not create an intermediate render target");
                    return false;
                }
            }
            uploadedRevision_ = std::numeric_limits<quint64>::max();
            if (!createPipelines(error))
                return false;
        }
        if (pipelineOutputSize_ != renderTarget()->pixelSize()) {
            pipelineOutputSize_ = renderTarget()->pixelSize();
            if (!createPipelineForTarget(renderTarget()->renderPassDescriptor(), Pass::Mask, maskPipeline_, error) ||
                !createPipelineForTarget(renderTarget()->renderPassDescriptor(), Pass::BlurVertical,
                                         verticalPipeline_, error) ||
                !createPipelineForTarget(renderTarget()->renderPassDescriptor(), Pass::GlowComposite,
                                         glowPipeline_, error))
                return false;
        }
        return true;
    }

    void releaseTargetsAndPipelines() {
        pipeline_.reset();
        maskPipeline_.reset();
        horizontalPipeline_.reset();
        verticalPipeline_.reset();
        glowPipeline_.reset();
        maskSrb_.reset();
        horizontalSrb_.reset();
        verticalSrb_.reset();
        alphaHorizontalSrb_.reset();
        alphaVerticalSrb_.reset();
        glowSrb_.reset();
        for (int i = 0; i < 2; ++i) {
            scratchTarget_[i].reset();
            scratchPass_[i].reset();
            scratchTexture_[i].reset();
        }
        pipelineOutputSize_ = {};
    }

    bool createPipelines(QString &error) {
        sampler_.reset(rhi_->newSampler(QRhiSampler::Nearest, QRhiSampler::Nearest,
                                        QRhiSampler::None, QRhiSampler::ClampToEdge,
                                        QRhiSampler::ClampToEdge));
        if (!sampler_->create()) {
            error = QStringLiteral("QRhi could not create the effect sampler");
            return false;
        }
        auto makeSrb = [&](QRhiBuffer *uniformBuffer, QRhiTexture *first, QRhiTexture *second,
                           std::unique_ptr<QRhiShaderResourceBindings> &binding) {
            binding.reset(rhi_->newShaderResourceBindings());
            binding->setBindings({
                QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::FragmentStage,
                                                         uniformBuffer),
                QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage,
                                                          first, sampler_.get()),
                QRhiShaderResourceBinding::sampledTexture(2, QRhiShaderResourceBinding::FragmentStage,
                                                          second, sampler_.get()),
            });
            return binding->create();
        };
        if (!makeSrb(uniformBuffers_[0].get(), sourceTexture_.get(), maskTexture_.get(), maskSrb_) ||
            !makeSrb(uniformBuffers_[1].get(), sourceTexture_.get(), sourceTexture_.get(), horizontalSrb_) ||
            !makeSrb(uniformBuffers_[2].get(), scratchTexture_[0].get(), sourceTexture_.get(), verticalSrb_) ||
            !makeSrb(uniformBuffers_[1].get(), sourceTexture_.get(), sourceTexture_.get(), alphaHorizontalSrb_) ||
            !makeSrb(uniformBuffers_[2].get(), scratchTexture_[0].get(), sourceTexture_.get(), alphaVerticalSrb_) ||
            !makeSrb(uniformBuffers_[3].get(), sourceTexture_.get(), scratchTexture_[1].get(), glowSrb_)) {
            error = QStringLiteral("QRhi could not create effect texture bindings");
            return false;
        }
        if (!createPipelineForTarget(scratchPass_[0].get(), Pass::BlurHorizontal,
                                     horizontalPipeline_, error))
            return false;
        return true;
    }

    bool createPipelineForTarget(QRhiRenderPassDescriptor *pass, Pass kind,
                                 std::unique_ptr<QRhiGraphicsPipeline> &pipeline, QString &error) {
        QRhiShaderResourceBindings *bindings = nullptr;
        switch (kind) {
        case Pass::Mask: bindings = maskSrb_.get(); break;
        case Pass::BlurHorizontal: bindings = horizontalSrb_.get(); break;
        case Pass::BlurVertical: bindings = verticalSrb_.get(); break;
        case Pass::GlowComposite: bindings = glowSrb_.get(); break;
        }
        const auto vertexShader = loadShader(QStringLiteral(":/gpu-effects/fullscreen.vert.qsb"));
        const auto fragmentShader = loadShader(fragmentPath(kind));
        if (!vertexShader.isValid() || !fragmentShader.isValid()) {
            error = QStringLiteral("Could not load the serialized GPU effect shaders");
            return false;
        }
        pipeline.reset(rhi_->newGraphicsPipeline());
        pipeline->setShaderStages({{QRhiShaderStage::Vertex, vertexShader},
                                   {QRhiShaderStage::Fragment, fragmentShader}});
        QRhiVertexInputLayout layout;
        layout.setBindings({{sizeof(Vertex)}});
        layout.setAttributes({{0, 0, QRhiVertexInputAttribute::Float2, 0},
                              {0, 1, QRhiVertexInputAttribute::Float2, 2 * sizeof(float)}});
        pipeline->setVertexInputLayout(layout);
        pipeline->setShaderResourceBindings(bindings);
        pipeline->setRenderPassDescriptor(pass);
        pipeline->setTopology(QRhiGraphicsPipeline::TriangleStrip);
        if (!pipeline->create()) {
            error = QStringLiteral("QRhi failed to create a graphics pipeline for an effect pass");
            return false;
        }
        return true;
    }

    void writeUniformsToBatch(QRhiResourceUpdateBatch *batch, QRhiBuffer *buffer, Pass pass) const {
        EffectUniforms uniforms;
        uniforms.params[0] = float(radius_);
        uniforms.params[1] = 1.0f / float(source_.width());
        uniforms.params[2] = 1.0f / float(source_.height());
        uniforms.params[3] = 1.0f;
        if (pass == Pass::Mask) {
            uniforms.params[0] = 1.0f;
            uniforms.params[1] = strength_;
        } else if (pass == Pass::GlowComposite) {
            uniforms.params[0] = 1.0f;
            uniforms.params[1] = strength_;
        }
        uniforms.color[0] = float(color_.redF());
        uniforms.color[1] = float(color_.greenF());
        uniforms.color[2] = float(color_.blueF());
        uniforms.color[3] = float(color_.alphaF());
        batch->updateDynamicBuffer(buffer, 0, sizeof(uniforms), &uniforms);
    }

    void runPass(QRhiCommandBuffer *cb, QRhiResourceUpdateBatch *updates, QRhiGraphicsPipeline *pipeline,
                 QRhiShaderResourceBindings *bindings, QRhiRenderTarget *target, Pass) {
        cb->beginPass(target, Qt::transparent, {1.0f, 0}, updates);
        cb->setGraphicsPipeline(pipeline);
        cb->setViewport(QRhiViewport(0, 0, target->pixelSize().width(), target->pixelSize().height()));
        cb->setShaderResources(bindings);
        const QRhiCommandBuffer::VertexInput vertexBinding(vertexBuffer_.get(), 0);
        cb->setVertexInput(0, 1, &vertexBinding);
        cb->draw(4);
        cb->endPass();
    }

    QRhi *rhi_{nullptr};
    QImage source_;
    QImage mask_;
    GpuEffectItem::Kind kind_{GpuEffectItem::Kind::AlphaMask};
    int radius_{0};
    QColor color_{Qt::white};
    float strength_{1.0f};
    quint64 revision_{0};
    quint64 uploadedRevision_{std::numeric_limits<quint64>::max()};
    QSize pipelineOutputSize_;
    std::shared_ptr<GpuEffectReadbackState> readback_;
    bool readbackQueued_{false};
    std::shared_ptr<QRhiReadbackResult> pendingReadback_;
    std::unique_ptr<QRhiBuffer> vertexBuffer_;
    std::unique_ptr<QRhiBuffer> uniformBuffers_[4];
    std::unique_ptr<QRhiSampler> sampler_;
    std::unique_ptr<QRhiTexture> sourceTexture_;
    std::unique_ptr<QRhiTexture> maskTexture_;
    std::unique_ptr<QRhiTexture> scratchTexture_[2];
    std::unique_ptr<QRhiTextureRenderTarget> scratchTarget_[2];
    std::unique_ptr<QRhiRenderPassDescriptor> scratchPass_[2];
    std::unique_ptr<QRhiShaderResourceBindings> maskSrb_;
    std::unique_ptr<QRhiShaderResourceBindings> horizontalSrb_;
    std::unique_ptr<QRhiShaderResourceBindings> verticalSrb_;
    std::unique_ptr<QRhiShaderResourceBindings> alphaHorizontalSrb_;
    std::unique_ptr<QRhiShaderResourceBindings> alphaVerticalSrb_;
    std::unique_ptr<QRhiShaderResourceBindings> glowSrb_;
    std::unique_ptr<QRhiGraphicsPipeline> pipeline_;
    std::unique_ptr<QRhiGraphicsPipeline> maskPipeline_;
    std::unique_ptr<QRhiGraphicsPipeline> horizontalPipeline_;
    std::unique_ptr<QRhiGraphicsPipeline> verticalPipeline_;
    std::unique_ptr<QRhiGraphicsPipeline> glowPipeline_;
    std::unique_ptr<QRhiResourceUpdateBatch> pendingUpdates_;
};

void GpuEffectReadbackState::publish(QImage image, GpuEffectTimings timings, QString error) {
    QMutexLocker lock(&mutex_);
    image_ = std::move(image);
    timings_ = timings;
    error_ = std::move(error);
    complete_ = true;
    ready_.wakeAll();
}

bool GpuEffectReadbackState::wait(QImage &image, GpuEffectTimings &timings, QString &error, int timeoutMs) {
    QMutexLocker lock(&mutex_);
    if (!complete_)
        ready_.wait(&mutex_, timeoutMs);
    image = image_;
    timings = timings_;
    error = error_;
    return complete_;
}

GpuEffectItem::GpuEffectItem(QQuickItem *parent) : QQuickRhiItem(parent) {
    setAlphaBlending(true);
}

void GpuEffectItem::setInputs(QImage source, QImage mask) {
    source_ = source.convertToFormat(QImage::Format_RGBA8888_Premultiplied);
    mask_ = mask.isNull() ? QImage{} : mask.convertToFormat(QImage::Format_RGBA8888_Premultiplied);
    ++revision_;
    update();
}

void GpuEffectItem::setEffect(Kind kind, int radius, QColor color, double strength) {
    kind_ = kind;
    radius_ = radius;
    color_ = std::move(color);
    strength_ = float(std::clamp(strength, 0.0, 1.0));
    update();
}

void GpuEffectItem::setReadbackState(std::shared_ptr<GpuEffectReadbackState> state) {
    readback_ = std::move(state);
    update();
}

QQuickRhiItemRenderer *GpuEffectItem::createRenderer() { return new GpuEffectItemRenderer; }
