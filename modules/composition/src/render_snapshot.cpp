#include <algorithm>
#include <studio/composition/render_snapshot.hpp>
#include <unordered_map>

namespace studio {
namespace {
const ArtDirection &artDirectionFor(const Project &project, const std::string &sceneId,
                                    const std::string &shotId = {}) {
    if (!shotId.empty())
        for (const auto &shot : project.shots)
            if (shot.id == shotId && shot.artDirection)
                return *shot.artDirection;
    if (!sceneId.empty())
        for (const auto &scene : project.scenes)
            if (scene.id == sceneId && scene.artDirection)
                return *scene.artDirection;
    return project.artDirection;
}

const ShotMarker *shotAt(const std::vector<ShotMarker> &shots, std::int64_t frame) {
    const ShotMarker *result = nullptr;
    for (const auto &shot : shots)
        if (frame >= shot.start && frame < shot.start + shot.duration &&
            (!result || shot.start > result->start))
            result = &shot;
    return result;
}

ArtDirection backgroundAt(const Project &project, std::int64_t frame) {
    if (const auto *shot = shotAt(project.shots, frame))
        return artDirectionFor(project, shot->sceneId, shot->id);
    for (const auto &scene : project.scenes)
        if (frame >= scene.start && frame < scene.start + scene.duration)
            return artDirectionFor(project, scene.id);
    return project.artDirection;
}

QTransform cameraTransform(const Project &project, CameraState camera) {
    QTransform transform;
    transform.translate(project.width / 2.0, project.height / 2.0);
    transform.scale(camera.zoom, camera.zoom);
    transform.rotate(camera.rotation);
    transform.translate(-camera.x, -camera.y);
    return transform;
}

RenderFrameSnapshot buildFrame(const Project &project, std::int64_t frame,
                               std::optional<std::string> shotId) {
    RenderFrameSnapshot result;
    result.frame = frame;
    result.camera = evaluateCamera(project, frame);
    result.cameraTransform = cameraTransform(project, result.camera);
    result.backgroundArt = backgroundAt(project, frame);
    result.shotId = std::move(shotId);
    for (const auto &scene : project.scenes)
        if (frame >= scene.start && frame < scene.start + scene.duration) {
            result.sceneId = scene.id;
            break;
        }

    std::unordered_map<std::string, std::size_t> indices;
    indices.reserve(project.nodes.size());
    for (std::size_t i = 0; i < project.nodes.size(); ++i)
        indices.emplace(project.nodes[i].id, i);
    std::vector<std::optional<std::size_t>> parentIndices(project.nodes.size());
    for (std::size_t i = 0; i < project.nodes.size(); ++i) {
        const auto parent = indices.find(project.nodes[i].parentId);
        if (!project.nodes[i].parentId.empty() && parent != indices.end() && parent->second != i)
            parentIndices[i] = parent->second;
    }
    result.nodes.reserve(project.nodes.size());
    for (std::size_t i = 0; i < project.nodes.size(); ++i) {
        const auto &node = project.nodes[i];
        RenderNodeSnapshot item;
        item.node = node;
        if (node.kind == NodeKind::Native && node.nativeId == "person")
            item.node.pose = evaluatePose(node, frame);
        item.drawOrder = i;
        item.state = evaluate(node, frame);
        item.localBounds = QRectF(0, 0, node.width, node.height);
        item.parentIndex = parentIndices[i];
        item.hierarchy.push_back(i);
        auto parentIndex = item.parentIndex;
        while (parentIndex && item.hierarchy.size() <= project.nodes.size()) {
            item.hierarchy.push_back(*parentIndex);
            parentIndex = parentIndices[*parentIndex];
        }
        if (parentIndex)
            item.visible = false;
        std::reverse(item.hierarchy.begin(), item.hierarchy.end());

        if (!parentIndex)
            item.visible = true;
        QTransform local;
        for (const auto ancestorIndex : item.hierarchy) {
            const auto &part = project.nodes[ancestorIndex];
            const auto state = evaluate(part, frame);
            item.transformChain.push_back({state, part.width, part.height});
            item.inheritedOpacity *= state.opacity;
            if (!active(part, frame) || state.opacity <= 0)
                item.visible = false;
            local.translate(state.x, state.y);
            local.translate(part.width / 2.0, part.height / 2.0);
            local.rotate(state.rotation);
            local.scale(state.scale, state.scale);
            local.translate(-part.width / 2.0, -part.height / 2.0);
        }
        item.sceneTransform = local;
        const auto localBounds = local.mapRect(item.localBounds);
        item.sceneBounds = node.role == "caption" ? localBounds
                                                    : result.cameraTransform.mapRect(localBounds);

        auto styleSceneId = node.sceneId;
        auto styleShotId = node.shotId;
        if (styleShotId.empty() && result.shotId)
            styleShotId = *result.shotId;
        if (styleSceneId.empty() && !styleShotId.empty())
            for (const auto &shot : project.shots)
                if (shot.id == styleShotId) {
                    styleSceneId = shot.sceneId;
                    break;
                }
        if (styleSceneId.empty())
            styleSceneId = result.sceneId;
        if (node.role == "caption" && styleShotId.empty())
            if (const auto *shot = shotAt(project.shots, frame); shot && shot->sceneId == styleSceneId)
                styleShotId = shot->id;
        item.artDirection = artDirectionFor(project, styleSceneId, styleShotId);
        result.nodes.push_back(std::move(item));
    }
    return result;
}
} // namespace

RenderFrameSnapshot buildRenderFrameSnapshot(const Project &project, std::int64_t frame,
                                             std::optional<std::string> shotId) {
    return buildFrame(project, frame, std::move(shotId));
}

RenderSnapshot RenderSnapshot::build(const Project &project, std::int64_t frame) {
    RenderSnapshot result;
    result.width_ = project.width;
    result.height_ = project.height;
    result.fps_ = project.fps;
    result.frame_ = frame;
    result.durationFrames_ = project.duration;
    result.hasShotTimeline_ = !project.shots.empty();
    result.scenes_ = project.scenes;
    result.shots_ = project.shots;
    const auto *current = shotAt(project.shots, frame);
    result.primary_ = buildRenderFrameSnapshot(
        project, frame, current ? std::optional<std::string>(current->id) : std::nullopt);
    if (!current)
        return result;
    const ShotMarker *previous = nullptr;
    if (current->transitionIn != "cut" && frame < current->start + current->transitionDurationFrames)
        for (const auto &shot : project.shots)
            if (shot.sceneId == current->sceneId && shot.start + shot.duration == current->start)
                previous = &shot;
    if (!previous)
        return result;
    const auto outgoingFrame = previous->start + previous->duration - 1;
    result.outgoing_ = buildRenderFrameSnapshot(project, outgoingFrame, previous->id);
    result.transition_ = RenderTransitionSnapshot{
        current->transitionIn,
        current->transitionDirection,
        current->id,
        previous->id,
        outgoingFrame,
        current->transitionDurationFrames,
        std::clamp(double(frame - current->start) / std::max(1, current->transitionDurationFrames - 1), 0.0,
                   1.0)};
    return result;
}
} // namespace studio
