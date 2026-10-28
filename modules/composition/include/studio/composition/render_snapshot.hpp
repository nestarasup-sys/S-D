#pragma once

#include <QRectF>
#include <QTransform>
#include <cstdint>
#include <optional>
#include <string>
#include <studio/core/project.hpp>
#include <vector>

namespace studio {

enum class RenderPrimitiveKind { RoundedRectangle, Rectangle, Ellipse, RectangleOutline, Line, ConvexPath };

// A frame-owned, resolved description shared by composition backends. It copies
// project data so consumers never retain pointers into an editable Project.
struct RenderNodeSnapshot {
    struct TransformPart {
        VisualState state;
        double width{0}, height{0};
    };
    Node node;
    std::size_t drawOrder{0};
    VisualState state;
    double inheritedOpacity{1.0};
    bool visible{false};
    std::optional<std::size_t> parentIndex;
    std::vector<std::size_t> hierarchy;
    std::vector<TransformPart> transformChain;
    QTransform sceneTransform;
    QRectF localBounds;
    QRectF sceneBounds;
    ArtDirection artDirection;
    RenderPrimitiveKind primitive{RenderPrimitiveKind::RoundedRectangle};
    std::vector<QPointF> path;
    double strokeWidth{2.0};
    bool filled{true};
};

struct RenderFrameSnapshot {
    std::int64_t frame{0};
    CameraState camera;
    QTransform cameraTransform;
    ArtDirection backgroundArt;
    std::optional<std::string> shotId;
    std::string sceneId;
    std::vector<RenderNodeSnapshot> nodes;
};

struct RenderTransitionSnapshot {
    std::string type;
    std::string direction;
    std::string incomingShotId;
    std::string outgoingShotId;
    std::int64_t outgoingFrame{0};
    int durationFrames{0};
    double progress{0};
};

RenderFrameSnapshot buildRenderFrameSnapshot(const Project &project, std::int64_t frame,
                                             std::optional<std::string> shotId = std::nullopt);

class RenderSnapshot final {
  public:
    static RenderSnapshot build(const Project &project, std::int64_t frame);

    int width() const {
        return width_;
    }
    int height() const {
        return height_;
    }
    FrameRate fps() const {
        return fps_;
    }
    std::int64_t durationFrames() const {
        return durationFrames_;
    }
    std::int64_t frame() const {
        return frame_;
    }
    bool hasShotTimeline() const {
        return hasShotTimeline_;
    }
    const RenderFrameSnapshot &primary() const {
        return primary_;
    }
    const std::optional<RenderFrameSnapshot> &outgoing() const {
        return outgoing_;
    }
    const std::optional<RenderTransitionSnapshot> &transition() const {
        return transition_;
    }
    const std::vector<SceneMarker> &scenes() const {
        return scenes_;
    }
    const std::vector<ShotMarker> &shots() const {
        return shots_;
    }

  private:
    int width_{0}, height_{0};
    FrameRate fps_;
    std::int64_t frame_{0};
    bool hasShotTimeline_{false};
    std::int64_t durationFrames_{0};
    std::vector<SceneMarker> scenes_;
    std::vector<ShotMarker> shots_;
    RenderFrameSnapshot primary_;
    std::optional<RenderFrameSnapshot> outgoing_;
    std::optional<RenderTransitionSnapshot> transition_;
};

} // namespace studio
