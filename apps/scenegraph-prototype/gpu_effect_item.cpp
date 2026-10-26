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
enum class TextureSlot { Source, Mask, Output, Scratch0, Scratch1 };

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
        effectStack_ = item->effectStack_;
        dynamicStackActive_ = item->dynamicStackActive_;
        stackRevision_ = item->stackRevision_;
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
            if (!dynamicStackActive_) {
                QImage opaqueMask;
                if (mask_.isNull()) {
                    opaqueMask = QImage(1, 1, QImage::Format_RGBA8888_Premultiplied);
                    opaqueMask.fill(Qt::white);
                }
                updates->uploadTexture(maskTexture_.get(), mask_.isNull() ? opaqueMask : mask_);
            }
            uploadedRevision_ = revision_;
            uploadPreparationNanoseconds = uploadPreparationTimer.nsecsElapsed();
        }
        QElapsedTimer submissionTimer;
        submissionTimer.start();
        if (kind_ == GpuEffectItem::Kind::EffectStack && dynamicStackActive_) {
            if (stackPlanRevision_ != stackRevision_ || stackPlanSize_ != source_.size()) {
                if (!createDynamicStackPlan(error)) {
                    if (readback_)
                        readback_->publish({}, {}, error);
                    return;
                }
            }
            if (!renderDynamicStack(cb, updates, uploadPreparationNanoseconds, error)) {
                if (readback_)
                    readback_->publish({}, {}, error);
                return;
            }
            updates = nullptr;
        } else if (kind_ == GpuEffectItem::Kind::GaussianBlur) {
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
        } else if (kind_ == GpuEffectItem::Kind::EffectStack) {
            writeUniformsToBatch(updates, uniformBuffers_[1].get(), Pass::BlurHorizontal);
            runPass(cb, updates, horizontalPipeline_.get(), horizontalSrb_.get(), scratchTarget_[0].get(),
                    Pass::BlurHorizontal);
            updates = nullptr;
            auto *verticalUpdates = rhi_->nextResourceUpdateBatch();
            writeUniformsToBatch(verticalUpdates, uniformBuffers_[2].get(), Pass::BlurVertical);
            runPass(cb, verticalUpdates, verticalPipeline_.get(), verticalSrb_.get(), scratchTarget_[1].get(),
                    Pass::BlurVertical);
            auto *glowUpdates = rhi_->nextResourceUpdateBatch();
            writeUniformsToBatch(glowUpdates, uniformBuffers_[3].get(), Pass::GlowComposite);
            runPass(cb, glowUpdates, glowPipeline_.get(), glowSrb_.get(), renderTarget(), Pass::GlowComposite);

            auto *stackBlurUpdates = rhi_->nextResourceUpdateBatch();
            writeUniformsToBatch(stackBlurUpdates, uniformBuffers_[4].get(), Pass::BlurHorizontal);
            runPass(cb, stackBlurUpdates, horizontalPipeline_.get(), stackHorizontalSrb_.get(),
                    scratchTarget_[0].get(), Pass::BlurHorizontal);
            auto *stackVerticalUpdates = rhi_->nextResourceUpdateBatch();
            writeUniformsToBatch(stackVerticalUpdates, uniformBuffers_[5].get(), Pass::BlurVertical);
            runPass(cb, stackVerticalUpdates, verticalPipeline_.get(), stackVerticalSrb_.get(),
                    scratchTarget_[1].get(), Pass::BlurVertical);
            auto *maskUpdates = rhi_->nextResourceUpdateBatch();
            writeUniformsToBatch(maskUpdates, uniformBuffers_[6].get(), Pass::Mask);
            runPass(cb, maskUpdates, maskPipeline_.get(), stackMaskSrb_.get(), renderTarget(), Pass::Mask);
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
            auto *resultData = result.get();
            result->completed = [resultData, state, readbackSize, submittedAt,
                                 uploadPreparationNanoseconds, submissionNanoseconds] {
                GpuEffectTimings timings;
                timings.textureUploadPreparationNanoseconds = uploadPreparationNanoseconds;
                timings.renderPassSubmissionNanoseconds = submissionNanoseconds;
                timings.readbackLatencyNanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::steady_clock::now() - submittedAt).count();
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
        dynamicPasses_.clear();
        stackPlanRevision_ = std::numeric_limits<quint64>::max();
        stackPlanSize_ = {};
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
        stackHorizontalSrb_.reset();
        stackVerticalSrb_.reset();
        stackMaskSrb_.reset();
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
        if (!(kind_ == GpuEffectItem::Kind::EffectStack && dynamicStackActive_) &&
            (radius_ < 1 || radius_ > kMaximumBlurRadius)) {
            if (kind_ != GpuEffectItem::Kind::AlphaMask) {
                error = QStringLiteral("GPU blur radius must be in [1, 24]");
                return false;
            }
        }
        if ((kind_ == GpuEffectItem::Kind::AlphaMask ||
             (kind_ == GpuEffectItem::Kind::EffectStack && !dynamicStackActive_)) && mask_.isNull()) {
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
        const QSize requestedMaskSize = dynamicStackActive_ ? source_.size()
                                             : mask_.isNull() ? QSize(1, 1) : mask_.size();
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

    struct DynamicPass {
        Pass pass{Pass::Mask};
        studio::RenderEffect effect;
        TextureSlot first{TextureSlot::Source};
        TextureSlot second{TextureSlot::Source};
        TextureSlot target{TextureSlot::Output};
        std::unique_ptr<QRhiBuffer> uniforms;
        std::unique_ptr<QRhiShaderResourceBindings> bindings;
    };

    QRhiTexture *textureFor(TextureSlot slot) const {
        switch (slot) {
        case TextureSlot::Source: return sourceTexture_.get();
        case TextureSlot::Mask: return maskTexture_.get();
        case TextureSlot::Output: return colorTexture();
        case TextureSlot::Scratch0: return scratchTexture_[0].get();
        case TextureSlot::Scratch1: return scratchTexture_[1].get();
        }
        return nullptr;
    }

    QRhiRenderTarget *targetFor(TextureSlot slot) const {
        switch (slot) {
        case TextureSlot::Output: return renderTarget();
        case TextureSlot::Scratch0: return scratchTarget_[0].get();
        case TextureSlot::Scratch1: return scratchTarget_[1].get();
        case TextureSlot::Mask: return nullptr;
        case TextureSlot::Source: return nullptr;
        }
        return nullptr;
    }

    bool createDynamicStackPlan(QString &error) {
        dynamicPasses_.clear();
        if (effectStack_.empty() || effectStack_.size() > 8) {
            error = QStringLiteral("Dynamic effect stack must contain between 1 and 8 effects");
            return false;
        }
        auto append = [&](Pass pass, const studio::RenderEffect &effect, TextureSlot first,
                          TextureSlot second, TextureSlot target) {
            DynamicPass command;
            command.pass = pass;
            command.effect = effect;
            command.first = first;
            command.second = second;
            command.target = target;
            command.uniforms.reset(rhi_->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer,
                                                    sizeof(EffectUniforms)));
            if (!command.uniforms || !command.uniforms->create())
                return false;
            command.bindings.reset(rhi_->newShaderResourceBindings());
            command.bindings->setBindings({
                QRhiShaderResourceBinding::uniformBuffer(0, QRhiShaderResourceBinding::FragmentStage,
                                                         command.uniforms.get()),
                QRhiShaderResourceBinding::sampledTexture(1, QRhiShaderResourceBinding::FragmentStage,
                                                          textureFor(first), sampler_.get()),
                QRhiShaderResourceBinding::sampledTexture(2, QRhiShaderResourceBinding::FragmentStage,
                                                          textureFor(second), sampler_.get()),
            });
            if (!command.bindings->create())
                return false;
            dynamicPasses_.push_back(std::move(command));
            return true;
        };
        const std::array<TextureSlot, 3> targets{TextureSlot::Output, TextureSlot::Scratch0,
                                                  TextureSlot::Scratch1};
        TextureSlot current = TextureSlot::Source;
        for (const auto &effect : effectStack_) {
            if (effect.radius < 0 || effect.radius > kMaximumBlurRadius || effect.strength < 0.0 ||
                effect.strength > 1.0 ||
                (effect.kind == studio::RenderEffectKind::AlphaMask && effect.alphaMask.isNull()) ||
                (effect.kind == studio::RenderEffectKind::OuterGlow && !effect.color.isValid())) {
                error = QStringLiteral("Dynamic effect has an invalid radius or missing alpha mask");
                return false;
            }
            if (effect.kind == studio::RenderEffectKind::GaussianBlur) {
                auto horizontal = std::find_if(targets.begin() + 1, targets.end(), [&](TextureSlot slot) {
                    return slot != current;
                });
                if (horizontal == targets.end()) {
                    error = QStringLiteral("No intermediate target is available for horizontal blur");
                    return false;
                }
                const auto temp = *horizontal;
                const auto vertical = temp == TextureSlot::Output ? TextureSlot::Scratch0 : TextureSlot::Output;
                if (vertical == temp ||
                    !append(Pass::BlurHorizontal, effect, current, current, temp) ||
                    !append(Pass::BlurVertical, effect, temp, current, vertical)) {
                    error = QStringLiteral("Could not allocate resources for a Gaussian blur pass");
                    return false;
                }
                current = vertical;
            } else if (effect.kind == studio::RenderEffectKind::AlphaMask) {
                const auto destination = std::find_if(targets.begin(), targets.end(), [&](TextureSlot slot) {
                    return slot != current;
                });
                if (destination == targets.end() ||
                    !append(Pass::Mask, effect, current, TextureSlot::Mask, *destination)) {
                    error = QStringLiteral("Could not allocate resources for an alpha mask pass");
                    return false;
                }
                current = *destination;
            } else {
                std::array<TextureSlot, 2> temporary{};
                int found = 0;
                for (const auto candidate : targets)
                    if (candidate != current && found < 2)
                        temporary[found++] = candidate;
                const auto destination = temporary[0];
                if (found != 2 ||
                    !append(Pass::BlurHorizontal, effect, current, current, temporary[0]) ||
                    !append(Pass::BlurVertical, effect, temporary[0], current, temporary[1]) ||
                    !append(Pass::GlowComposite, effect, current, temporary[1], destination)) {
                    error = QStringLiteral("Could not allocate resources for an outer glow pass");
                    return false;
                }
                current = destination;
            }
        }
        if (current != TextureSlot::Output) {
            studio::RenderEffect copy;
            copy.kind = studio::RenderEffectKind::AlphaMask;
            copy.alphaMask = QImage(source_.size(), QImage::Format_RGBA8888_Premultiplied);
            copy.alphaMask.fill(Qt::white);
            if (!append(Pass::Mask, copy, current, TextureSlot::Mask, TextureSlot::Output)) {
                error = QStringLiteral("Could not allocate final composition output pass");
                return false;
            }
        }
        stackPlanRevision_ = stackRevision_;
        stackPlanSize_ = source_.size();
        return true;
    }

    bool renderDynamicStack(QRhiCommandBuffer *cb, QRhiResourceUpdateBatch *firstUpdates,
                            qint64 &uploadPreparationNanoseconds, QString &error) {
        if (dynamicPasses_.empty()) {
            error = QStringLiteral("Dynamic effect pass plan is empty");
            return false;
        }
        bool firstPass = true;
        QElapsedTimer maskUploadTimer;
        maskUploadTimer.start();
        for (auto &command : dynamicPasses_) {
            auto *updates = firstPass ? firstUpdates : rhi_->nextResourceUpdateBatch();
            firstPass = false;
            if (command.pass == Pass::Mask) {
                if (command.effect.alphaMask.size() != source_.size()) {
                    error = QStringLiteral("Dynamic alpha masks must match their source layer dimensions");
                    return false;
                }
                // Sequential passes share this texture; a later mask or final copy may
                // overwrite it, so upload the declared mask immediately before use.
                updates->uploadTexture(maskTexture_.get(), command.effect.alphaMask);
            }
            EffectUniforms uniforms;
            uniforms.params[0] = float(command.effect.radius);
            uniforms.params[1] = 1.0f / float(std::max(1, source_.width()));
            uniforms.params[2] = 1.0f / float(std::max(1, source_.height()));
            uniforms.params[3] = 1.0f;
            if (command.pass == Pass::Mask || command.pass == Pass::GlowComposite) {
                uniforms.params[0] = 1.0f;
                uniforms.params[1] = float(command.effect.strength);
            }
            uniforms.color[0] = float(command.effect.color.redF());
            uniforms.color[1] = float(command.effect.color.greenF());
            uniforms.color[2] = float(command.effect.color.blueF());
            uniforms.color[3] = float(command.effect.color.alphaF());
            updates->updateDynamicBuffer(command.uniforms.get(), 0, sizeof(uniforms), &uniforms);
            QRhiGraphicsPipeline *pipeline = nullptr;
            switch (command.pass) {
            case Pass::Mask: pipeline = maskPipeline_.get(); break;
            case Pass::BlurHorizontal: pipeline = horizontalPipeline_.get(); break;
            case Pass::BlurVertical: pipeline = verticalPipeline_.get(); break;
            case Pass::GlowComposite: pipeline = glowPipeline_.get(); break;
            }
            runPass(cb, updates, pipeline, command.bindings.get(), targetFor(command.target), command.pass);
        }
        uploadPreparationNanoseconds += maskUploadTimer.nsecsElapsed();
        return true;
    }

    void releaseTargetsAndPipelines() {
        dynamicPasses_.clear();
        stackPlanRevision_ = std::numeric_limits<quint64>::max();
        stackPlanSize_ = {};
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
            !makeSrb(uniformBuffers_[3].get(), sourceTexture_.get(), scratchTexture_[1].get(), glowSrb_) ||
            !makeSrb(uniformBuffers_[4].get(), colorTexture(), sourceTexture_.get(), stackHorizontalSrb_) ||
            !makeSrb(uniformBuffers_[5].get(), scratchTexture_[0].get(), sourceTexture_.get(), stackVerticalSrb_) ||
            !makeSrb(uniformBuffers_[6].get(), scratchTexture_[1].get(), maskTexture_.get(), stackMaskSrb_)) {
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
    std::vector<studio::RenderEffect> effectStack_;
    GpuEffectItem::Kind kind_{GpuEffectItem::Kind::AlphaMask};
    int radius_{0};
    QColor color_{Qt::white};
    float strength_{1.0f};
    bool dynamicStackActive_{false};
    quint64 revision_{0};
    quint64 stackRevision_{0};
    quint64 uploadedRevision_{std::numeric_limits<quint64>::max()};
    quint64 stackPlanRevision_{std::numeric_limits<quint64>::max()};
    QSize stackPlanSize_;
    std::vector<DynamicPass> dynamicPasses_;
    QSize pipelineOutputSize_;
    std::shared_ptr<GpuEffectReadbackState> readback_;
    bool readbackQueued_{false};
    std::shared_ptr<QRhiReadbackResult> pendingReadback_;
    std::unique_ptr<QRhiBuffer> vertexBuffer_;
    std::unique_ptr<QRhiBuffer> uniformBuffers_[7];
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
    std::unique_ptr<QRhiShaderResourceBindings> stackHorizontalSrb_;
    std::unique_ptr<QRhiShaderResourceBindings> stackVerticalSrb_;
    std::unique_ptr<QRhiShaderResourceBindings> stackMaskSrb_;
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
    dynamicStackActive_ = false;
    effectStack_.clear();
    ++stackRevision_;
    radius_ = radius;
    color_ = std::move(color);
    strength_ = float(std::clamp(strength, 0.0, 1.0));
    update();
}

void GpuEffectItem::setEffectStack(std::vector<studio::RenderEffect> effects) {
    kind_ = Kind::EffectStack;
    dynamicStackActive_ = true;
    effectStack_ = std::move(effects);
    if (!source_.isNull())
        for (auto &effect : effectStack_)
            if (effect.kind == studio::RenderEffectKind::AlphaMask && !effect.alphaMask.isNull())
                effect.alphaMask = effect.alphaMask.convertToFormat(QImage::Format_RGBA8888_Premultiplied)
                                       .scaled(source_.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    ++stackRevision_;
    update();
}

void GpuEffectItem::setReadbackState(std::shared_ptr<GpuEffectReadbackState> state) {
    readback_ = std::move(state);
    update();
}

QQuickRhiItemRenderer *GpuEffectItem::createRenderer() { return new GpuEffectItemRenderer; }
