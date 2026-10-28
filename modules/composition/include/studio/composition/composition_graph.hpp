#pragma once

#include <QRectF>
#include <QTransform>
#include <optional>
#include <string>
#include <vector>
#include <studio/composition/effect_pipeline.hpp>

namespace studio {

enum class CompositionGraphOpKind { BeginGroupSurface, DrawLayer, ApplyEffectStack, CompositeGroup };
enum class CompositionEffectScope { Object, Group };

struct CompositionEffectBinding {
    std::string nodeId;
    CompositionEffectScope scope{CompositionEffectScope::Object};
    std::vector<RenderEffect> effects;
};

// Render-only instructions derived from one resolved frame. Node indices always
// refer to that frame; this graph is not another editable project representation.
struct CompositionGraphOp {
    CompositionGraphOpKind kind{CompositionGraphOpKind::DrawLayer};
    std::size_t nodeIndex{0};
    std::size_t drawOrder{0};
    std::optional<std::size_t> parentIndex;
    int surfaceId{-1};
    QRectF sourceBounds;
    QRectF outputBounds;
    QRectF clipBounds;
    QTransform transform;
    double inheritedOpacity{1.0};
    std::vector<RenderEffect> effects;
    std::vector<int> dependencies;
};

struct CompositionGraphResult {
    std::int64_t frame{0};
    FrameRate fps{};
    QSize canvas;
    QTransform cameraTransform;
    std::vector<CompositionGraphOp> operations;
    QString error;
    bool succeeded() const { return error.isEmpty(); }
};

class CompositionRenderGraph final {
  public:
    static CompositionGraphResult build(const RenderSnapshot &, const std::vector<CompositionEffectBinding> & = {});
};

} // namespace studio
