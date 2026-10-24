#include "preview_item.hpp"
#include <QMouseEvent>
#include <algorithm>
#include <vector>

namespace {
QTransform nodeTransform(const studio::Project &project, const studio::Node &node, std::int64_t frame,
                         bool includeNode) {
    const auto camera = studio::evaluateCamera(project, frame);
    QTransform transform;
    transform.translate(project.width / 2.0, project.height / 2.0);
    transform.scale(camera.zoom, camera.zoom);
    transform.rotate(camera.rotation);
    transform.translate(-camera.x, -camera.y);
    std::vector<const studio::Node *> chain;
    auto parentId = includeNode ? node.id : node.parentId;
    while (!parentId.empty()) {
        const auto parent = std::find_if(project.nodes.begin(), project.nodes.end(),
                                         [&](const auto &candidate) { return candidate.id == parentId; });
        if (parent == project.nodes.end())
            break;
        chain.push_back(&*parent);
        parentId = parent->parentId;
    }
    std::reverse(chain.begin(), chain.end());
    for (const auto *part : chain) {
        const auto state = studio::evaluate(*part, frame);
        transform.translate(state.x, state.y);
        transform.translate(part->width / 2, part->height / 2);
        transform.rotate(state.rotation);
        transform.scale(state.scale, state.scale);
        transform.translate(-part->width / 2, -part->height / 2);
    }
    return transform;
}
void applyNodeTransform(QPainter &painter, const studio::Project &project, const studio::Node &node,
                        std::int64_t frame) {
    const auto camera = studio::evaluateCamera(project, frame);
    painter.translate(project.width / 2.0, project.height / 2.0);
    painter.scale(camera.zoom, camera.zoom);
    painter.rotate(camera.rotation);
    painter.translate(-camera.x, -camera.y);
    std::vector<const studio::Node *> chain{&node};
    auto parentId = node.parentId;
    while (!parentId.empty()) {
        const auto parent = std::find_if(project.nodes.begin(), project.nodes.end(),
                                         [&](const auto &candidate) { return candidate.id == parentId; });
        if (parent == project.nodes.end())
            break;
        chain.push_back(&*parent);
        parentId = parent->parentId;
    }
    std::reverse(chain.begin(), chain.end());
    for (const auto *part : chain) {
        const auto state = studio::evaluate(*part, frame);
        painter.translate(state.x, state.y);
        painter.translate(part->width / 2, part->height / 2);
        painter.rotate(state.rotation);
        painter.scale(state.scale, state.scale);
        painter.translate(-part->width / 2, -part->height / 2);
    }
}
} // namespace

PreviewItem::PreviewItem(QQuickItem *parent) : QQuickPaintedItem(parent) {
    setAcceptedMouseButtons(Qt::LeftButton);
    setAntialiasing(true);
}
void PreviewItem::setController(EditorController *controller) {
    if (controller_ == controller)
        return;
    if (controller_)
        disconnect(controller_, nullptr, this, nullptr);
    controller_ = controller;
    if (controller_) {
        connect(controller_, &EditorController::frameChanged, this, [this] { update(); });
        connect(controller_, &EditorController::documentChanged, this, [this] { update(); });
    }
    emit controllerChanged();
    update();
}
void PreviewItem::paint(QPainter *painter) {
    if (!controller_)
        return;
    const auto &p = controller_->project();
    painter->save();
    painter->scale(width() / p.width, height() / p.height);
    painter_.paint(*painter, p, controller_->frame());
    auto n = studio::findNode(p, controller_->selectedId().toStdString());
    if (n && studio::active(*n, controller_->frame())) {
        QPen pen(QColor("#b4f078"));
        pen.setWidthF(1.5 * p.width / width());
        pen.setStyle(Qt::DashLine);
        painter->setOpacity(1);
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);
        applyNodeTransform(*painter, p, *n, controller_->frame());
        painter->drawRect(QRectF(0, 0, n->width, n->height));
    }
    painter->restore();
}
QPointF PreviewItem::toProject(QPointF point) const {
    return {point.x() * controller_->canvasWidth() / width(),
            point.y() * controller_->canvasHeight() / height()};
}
void PreviewItem::mousePressEvent(QMouseEvent *event) {
    if (!controller_)
        return;
    forceActiveFocus(Qt::MouseFocusReason);
    auto point = toProject(event->position());
    const auto &nodes = controller_->project().nodes;
    for (auto it = nodes.rbegin(); it != nodes.rend(); ++it) {
        if (!studio::active(*it, controller_->frame()) || it->kind == studio::NodeKind::Group)
            continue;
        if (it->role == "background")
            continue;
        const auto transform = nodeTransform(controller_->project(), *it, controller_->frame(), true);
        if (QRectF(0, 0, it->width, it->height).contains(transform.inverted().map(point))) {
            if (controller_->playing())
                controller_->togglePlayback();
            controller_->selectNode(QString::fromStdString(it->id));
            dragParentTransform_ = nodeTransform(controller_->project(), *it, controller_->frame(), false);
            origin_ = dragParentTransform_.inverted().map(point);
            nodeOrigin_ = {it->x, it->y};
            dragging_ = true;
            controller_->beginGesture();
            event->accept();
            return;
        }
    }
    controller_->selectNode({});
    event->accept();
}
void PreviewItem::mouseMoveEvent(QMouseEvent *event) {
    if (!dragging_ || !controller_)
        return;
    const auto localPoint = dragParentTransform_.inverted().map(toProject(event->position()));
    auto delta = localPoint - origin_;
    controller_->moveSelected(qRound(nodeOrigin_.x() + delta.x()), qRound(nodeOrigin_.y() + delta.y()));
}
void PreviewItem::mouseReleaseEvent(QMouseEvent *) {
    if (dragging_ && controller_)
        controller_->endGesture();
    dragging_ = false;
}
