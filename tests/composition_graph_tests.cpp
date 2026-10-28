#include <studio/composition/composition_graph.hpp>

#include <QGuiApplication>
#include <QImage>
#include <algorithm>
#include <iterator>
#include <iostream>
#include <stdexcept>

namespace {
int checks = 0;
void check(bool condition, const char *message) {
    ++checks;
    if (!condition)
        throw std::runtime_error(message);
}

studio::Node node(std::string id, std::string parent, studio::NodeKind kind, double x, double y,
                  double width = 80, double height = 48) {
    studio::Node value;
    value.id = std::move(id);
    value.parentId = std::move(parent);
    value.kind = kind;
    value.name = value.id;
    value.start = 0;
    value.duration = 120;
    value.x = x;
    value.y = y;
    value.width = width;
    value.height = height;
    value.animated = false;
    return value;
}

std::vector<studio::RenderEffect> glowMask(bool glowFirst, QSize size) {
    QImage mask(size, QImage::Format_ARGB32_Premultiplied);
    mask.fill(Qt::white);
    const studio::RenderEffect glow{studio::RenderEffectKind::OuterGlow, 6, QColor("#73dfd1"), {}, .8};
    const studio::RenderEffect alpha{studio::RenderEffectKind::AlphaMask, 0, Qt::white, mask, .9};
    return glowFirst ? std::vector<studio::RenderEffect>{glow, alpha}
                     : std::vector<studio::RenderEffect>{alpha, glow};
}

void run() {
    auto project = studio::makeEmpty();
    project.width = 640;
    project.height = 360;
    project.duration = 120;
    project.fps = {30, 1};
    project.camera = {{0, 320, 180, 1.1, 4, "linear", {}},
                      {119, 326, 176, 1.15, -2, "ease-in-out", {}}};
    auto root = node("root-scene", "", studio::NodeKind::Group, 0, 0, 640, 360);
    root.opacity = .96;
    root.keyframes = {{0, 0, 0, 0, 1, .96, "linear"}, {119, 0, 0, 0, 1, .96, "linear"}};
    project.nodes.push_back(root);
    auto groupA = node("group-a", root.id, studio::NodeKind::Group, 36, 40, 360, 220);
    groupA.opacity = .82;
    groupA.keyframes = {{0, 36, 40, -2, 1, .82, "linear"}, {119, 42, 34, 4, 1.04, .82, "linear"}};
    project.nodes.push_back(groupA);
    project.nodes.push_back(node("title", groupA.id, studio::NodeKind::Text, 8, 5));
    project.nodes.push_back(node("group-b", groupA.id, studio::NodeKind::Group, 12, 72, 220, 120));
    project.nodes.push_back(node("vector", "group-b", studio::NodeKind::Shape, 4, 6));
    project.nodes.push_back(node("blur-image", "group-b", studio::NodeKind::Image, 88, 8));
    project.nodes.push_back(node("phone", groupA.id, studio::NodeKind::Native, 260, 70, 50, 100));
    project.nodes.push_back(node("foreground-caption", root.id, studio::NodeKind::Text, 24, 300, 420, 38));

    const auto snapshot = studio::RenderSnapshot::build(project, 54);
    const auto imageMask = QSize(80, 48);
    const std::vector<studio::CompositionEffectBinding> bindings{
        {"group-a", studio::CompositionEffectScope::Group,
         {{studio::RenderEffectKind::OuterGlow, 9, QColor("#d7a85c"), {}, .7},
          {studio::RenderEffectKind::GaussianBlur, 2}}},
        {"group-b", studio::CompositionEffectScope::Group, glowMask(false, QSize(220, 120))},
        {"blur-image", studio::CompositionEffectScope::Object,
         {{studio::RenderEffectKind::GaussianBlur, 5}}},
        {"phone", studio::CompositionEffectScope::Object,
         {{studio::RenderEffectKind::OuterGlow, 6, QColor("#69d6d0"), {}, .8}}},
        {"title", studio::CompositionEffectScope::Object, glowMask(true, imageMask)}};
    const auto graph = studio::CompositionRenderGraph::build(snapshot, bindings);
    check(graph.succeeded(), "composition graph builds from the shared resolved snapshot");
    check(graph.frame == 54 && graph.canvas == QSize(640, 360) && graph.fps.value() == 30.0,
          "graph retains the snapshot frame, canvas, FPS and camera context");

    const auto &nodes = snapshot.primary().nodes;
    const auto idAt = [&](std::size_t index) { return nodes.at(index).node.id; };
    std::vector<std::string> operations;
    int groupSurfaceBegins = 0;
    int groupSurfaceComposites = 0;
    const studio::CompositionGraphOp *groupAApply = nullptr;
    const studio::CompositionGraphOp *groupBApply = nullptr;
    const studio::CompositionGraphOp *imageDraw = nullptr;
    for (const auto &op : graph.operations) {
        operations.push_back(idAt(op.nodeIndex));
        if (op.kind == studio::CompositionGraphOpKind::BeginGroupSurface)
            ++groupSurfaceBegins;
        if (op.kind == studio::CompositionGraphOpKind::CompositeGroup)
            ++groupSurfaceComposites;
        if (op.kind == studio::CompositionGraphOpKind::ApplyEffectStack && idAt(op.nodeIndex) == "group-a")
            groupAApply = &op;
        if (op.kind == studio::CompositionGraphOpKind::ApplyEffectStack && idAt(op.nodeIndex) == "group-b")
            groupBApply = &op;
        if (op.kind == studio::CompositionGraphOpKind::DrawLayer && idAt(op.nodeIndex) == "blur-image")
            imageDraw = &op;
    }
    const std::vector<std::string> expected{
        "group-a", "title", "group-b", "vector", "blur-image", "group-b", "group-b",
        "phone", "group-a", "group-a", "foreground-caption"};
    check(operations == expected, "nested groups and effected layers preserve hierarchy and sibling order");
    check(groupSurfaceBegins == 2 && groupSurfaceComposites == 2,
          "only groups with effects receive intermediate surfaces");
    check(groupAApply && groupBApply && groupAApply->effects.size() == 2 && groupBApply->effects.size() == 2,
          "nested group stacks remain attached to their own combined surfaces");
    check(groupBApply->effects[0].kind == studio::RenderEffectKind::AlphaMask &&
              groupBApply->effects[1].kind == studio::RenderEffectKind::OuterGlow,
          "declared group effect order is preserved");
    check(groupAApply->effects[0].kind == studio::RenderEffectKind::OuterGlow &&
              groupAApply->effects[1].kind == studio::RenderEffectKind::GaussianBlur,
          "outer group has a distinct ordered effect stack");
    check(imageDraw && imageDraw->effects.size() == 1 &&
              imageDraw->effects.front().kind == studio::RenderEffectKind::GaussianBlur,
          "object effects are separate from the enclosing group effects");
    check(groupAApply->dependencies.size() == 3 && groupBApply->dependencies.size() == 2,
          "group surfaces track child dependencies for pass scheduling");
    check(groupAApply->outputBounds.contains(groupAApply->sourceBounds) &&
              groupAApply->outputBounds.left() >= 0 && groupAApply->outputBounds.right() <= project.width,
          "effect bounds expand for glow and blur then clip safely to the canvas");
    check(!groupAApply->transform.isIdentity() && groupAApply->inheritedOpacity < 1.0,
          "graph carries animated group transform and inherited opacity");

    auto reordered = bindings;
    reordered[4].effects = glowMask(false, imageMask);
    const auto alternate = studio::CompositionRenderGraph::build(snapshot, reordered);
    check(alternate.succeeded(), "alternate stack order remains a valid graph");
    const auto titleIndex = std::size_t(std::distance(nodes.begin(), std::find_if(nodes.begin(), nodes.end(),
        [](const auto &nodeValue) { return nodeValue.node.id == "title"; })));
    const auto titleOp = std::find_if(alternate.operations.begin(), alternate.operations.end(), [&](const auto &op) {
        return op.kind == studio::CompositionGraphOpKind::DrawLayer && op.nodeIndex == titleIndex;
    });
    check(titleOp != alternate.operations.end() && titleOp->effects[0].kind == studio::RenderEffectKind::AlphaMask &&
              titleOp->effects[1].kind == studio::RenderEffectKind::OuterGlow,
          "changing only a stack order changes the graph instructions without changing document structure");

    auto invalid = bindings;
    invalid.push_back({"missing-node", studio::CompositionEffectScope::Object,
                       {{studio::RenderEffectKind::GaussianBlur, 1}}});
    check(!studio::CompositionRenderGraph::build(snapshot, invalid).succeeded(),
          "unknown effect target is rejected");
    invalid = bindings;
    invalid[0].scope = studio::CompositionEffectScope::Object;
    check(!studio::CompositionRenderGraph::build(snapshot, invalid).succeeded(),
          "object and group effect scopes cannot be confused");
    invalid = bindings;
    invalid[3].effects.front().radius = 25;
    check(!studio::CompositionRenderGraph::build(snapshot, invalid).succeeded(),
          "effect radii outside the bounded resource contract are rejected");

    std::cout << "composition graph checks: " << checks << " passed\n";
}
} // namespace

int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    try {
        run();
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
