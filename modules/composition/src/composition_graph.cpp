#include <studio/composition/composition_graph.hpp>

#include <QSize>
#include <algorithm>
#include <cmath>
#include <functional>
#include <unordered_map>
#include <unordered_set>

namespace studio {
namespace {

const std::vector<RenderEffect> *effectsFor(const RenderNodeSnapshot &node,
                                            const std::vector<CompositionEffectBinding> &bindings) {
    for (const auto &binding : bindings)
        if (binding.nodeId == node.node.id)
            return &binding.effects;
    return nullptr;
}

double effectExpansion(const std::vector<RenderEffect> *effects, const QTransform &transform) {
    if (!effects)
        return 0.0;
    const double xScale = std::hypot(transform.m11(), transform.m12());
    const double yScale = std::hypot(transform.m21(), transform.m22());
    const double scale = std::max(xScale, yScale);
    double radius = 0.0;
    for (const auto &effect : *effects)
        if (effect.kind == RenderEffectKind::GaussianBlur || effect.kind == RenderEffectKind::OuterGlow)
            radius += std::max(0, effect.radius) * 3.0;
    return radius * scale;
}

QRectF expandedClipped(const QRectF &bounds, double margin, const QRectF &canvas) {
    if (bounds.isEmpty())
        return {};
    const QRectF expanded = bounds.adjusted(-margin, -margin, margin, margin);
    return expanded.intersected(canvas);
}

} // namespace

CompositionGraphResult CompositionRenderGraph::build(
    const RenderSnapshot &snapshot, const std::vector<CompositionEffectBinding> &bindings) {
    CompositionGraphResult graph;
    graph.frame = snapshot.frame();
    graph.fps = snapshot.fps();
    graph.canvas = QSize(snapshot.width(), snapshot.height());
    const auto &frame = snapshot.primary();
    graph.cameraTransform = frame.cameraTransform;
    if (graph.canvas.width() <= 0 || graph.canvas.height() <= 0) {
        graph.error = QStringLiteral("Composition graph requires a valid snapshot canvas");
        return graph;
    }

    const auto canvasBounds = QRectF(QPointF(0, 0), graph.canvas);
    std::vector<std::vector<std::size_t>> children(frame.nodes.size());
    std::vector<std::size_t> roots;
    for (std::size_t index = 0; index < frame.nodes.size(); ++index) {
        const auto &node = frame.nodes[index];
        if (!node.parentIndex) {
            roots.push_back(index);
            continue;
        }
        if (*node.parentIndex >= frame.nodes.size() || *node.parentIndex == index) {
            graph.error = QStringLiteral("Composition graph found an invalid parent reference");
            return graph;
        }
        children[*node.parentIndex].push_back(index);
    }
    const auto byDrawOrder = [&](std::size_t a, std::size_t b) {
        return frame.nodes[a].drawOrder < frame.nodes[b].drawOrder;
    };
    std::sort(roots.begin(), roots.end(), byDrawOrder);
    for (auto &list : children)
        std::sort(list.begin(), list.end(), byDrawOrder);

    std::unordered_map<std::size_t, const std::vector<RenderEffect> *> effectMap;
    std::unordered_map<std::string, std::size_t> indexById;
    for (std::size_t index = 0; index < frame.nodes.size(); ++index)
        indexById.emplace(frame.nodes[index].node.id, index);
    for (const auto &binding : bindings) {
        const auto found = indexById.find(binding.nodeId);
        if (found == indexById.end()) {
            graph.error = QStringLiteral("Composition graph effect references an unknown node");
            return graph;
        }
        const auto &resolved = frame.nodes[found->second];
        const bool isGroup = resolved.node.kind == NodeKind::Group;
        if ((binding.scope == CompositionEffectScope::Group) != isGroup) {
            graph.error = QStringLiteral("Composition graph effect scope does not match its node kind");
            return graph;
        }
        if (binding.effects.empty()) {
            graph.error = QStringLiteral("Composition graph effect stacks cannot be empty");
            return graph;
        }
        for (const auto &effect : binding.effects) {
            if (effect.radius < 0 || effect.radius > 24 ||
                (effect.kind == RenderEffectKind::AlphaMask && effect.alphaMask.isNull())) {
                graph.error = QStringLiteral("Composition graph effect parameters are invalid");
                return graph;
            }
        }
        effectMap.emplace(found->second, &binding.effects);
    }

    std::vector<int> visit(frame.nodes.size(), 0);
    int nextSurface = 1;
    std::function<QRectF(std::size_t)> emitNode = [&](std::size_t index) -> QRectF {
        if (visit[index] == 1) {
            graph.error = QStringLiteral("Composition graph found a cyclic group hierarchy");
            return {};
        }
        if (visit[index] == 2)
            return {};
        visit[index] = 1;
        const auto &resolved = frame.nodes[index];
        if (!resolved.visible || resolved.inheritedOpacity <= 0) {
            visit[index] = 2;
            return {};
        }
        const auto effectIt = effectMap.find(index);
        const auto *effects = effectIt == effectMap.end() ? nullptr : effectIt->second;
        const auto transform = resolved.sceneTransform * frame.cameraTransform;
        if (resolved.node.kind != NodeKind::Group) {
            const auto source = resolved.sceneBounds.intersected(canvasBounds);
            const auto output = expandedClipped(resolved.sceneBounds, effectExpansion(effects, transform),
                                                canvasBounds);
            CompositionGraphOp op;
            op.kind = CompositionGraphOpKind::DrawLayer;
            op.nodeIndex = index;
            op.drawOrder = resolved.drawOrder;
            op.parentIndex = resolved.parentIndex;
            op.sourceBounds = source;
            op.outputBounds = output;
            op.clipBounds = canvasBounds;
            op.transform = transform;
            op.inheritedOpacity = resolved.inheritedOpacity;
            if (effects)
                op.effects = *effects;
            graph.operations.push_back(std::move(op));
            visit[index] = 2;
            return output;
        }

        const int surface = effects ? nextSurface++ : -1;
        if (effects) {
            CompositionGraphOp begin;
            begin.kind = CompositionGraphOpKind::BeginGroupSurface;
            begin.nodeIndex = index;
            begin.drawOrder = resolved.drawOrder;
            begin.parentIndex = resolved.parentIndex;
            begin.surfaceId = surface;
            begin.sourceBounds = resolved.sceneBounds.intersected(canvasBounds);
            begin.clipBounds = canvasBounds;
            begin.transform = transform;
            begin.inheritedOpacity = resolved.inheritedOpacity;
            graph.operations.push_back(std::move(begin));
        }
        QRectF contentBounds;
        for (const auto child : children[index])
            contentBounds = contentBounds.united(emitNode(child));
        if (!graph.error.isEmpty())
            return {};

        QRectF output = contentBounds.united(resolved.sceneBounds.intersected(canvasBounds));
        if (effects) {
            const double margin = effectExpansion(effects, transform);
            output = expandedClipped(output, margin, canvasBounds);
            CompositionGraphOp apply;
            apply.kind = CompositionGraphOpKind::ApplyEffectStack;
            apply.nodeIndex = index;
            apply.drawOrder = resolved.drawOrder;
            apply.parentIndex = resolved.parentIndex;
            apply.surfaceId = surface;
            apply.sourceBounds = contentBounds.intersected(canvasBounds);
            apply.outputBounds = output;
            apply.clipBounds = canvasBounds;
            apply.transform = transform;
            apply.inheritedOpacity = resolved.inheritedOpacity;
            apply.effects = *effects;
            for (const auto child : children[index])
                apply.dependencies.push_back(int(child));
            graph.operations.push_back(std::move(apply));

            CompositionGraphOp composite;
            composite.kind = CompositionGraphOpKind::CompositeGroup;
            composite.nodeIndex = index;
            composite.drawOrder = resolved.drawOrder;
            composite.parentIndex = resolved.parentIndex;
            composite.surfaceId = surface;
            composite.sourceBounds = contentBounds.intersected(canvasBounds);
            composite.outputBounds = output;
            composite.clipBounds = canvasBounds;
            composite.transform = transform;
            composite.inheritedOpacity = resolved.inheritedOpacity;
            composite.dependencies.push_back(surface);
            graph.operations.push_back(std::move(composite));
        }
        visit[index] = 2;
        return output;
    };

    for (const auto root : roots)
        emitNode(root);
    if (graph.error.isEmpty())
        for (const auto state : visit)
            if (state == 0) {
                graph.error = QStringLiteral("Composition graph contains an unreachable or cyclic node");
                break;
            }
    if (!graph.error.isEmpty())
        graph.operations.clear();
    return graph;
}

} // namespace studio
