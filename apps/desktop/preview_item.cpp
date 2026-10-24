#include "preview_item.hpp"
#include <QMouseEvent>

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
        auto state = studio::evaluate(*n, controller_->frame());
        QPen pen(QColor("#b4f078"));
        pen.setWidthF(1.5 * p.width / width());
        pen.setStyle(Qt::DashLine);
        painter->setOpacity(1);
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);
        painter->translate(state.x + n->width / 2, state.y + n->height / 2);
        painter->rotate(state.rotation);
        painter->scale(state.scale, state.scale);
        painter->drawRect(QRectF(-n->width / 2, -n->height / 2, n->width, n->height));
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
        if (!studio::active(*it, controller_->frame()))
            continue;
        if (it->role == "background")
            continue;
        auto state = studio::evaluate(*it, controller_->frame());
        QTransform transform;
        transform.translate(state.x + it->width / 2, state.y + it->height / 2);
        transform.rotate(state.rotation);
        transform.scale(state.scale, state.scale);
        transform.translate(-it->width / 2, -it->height / 2);
        if (QRectF(0, 0, it->width, it->height).contains(transform.inverted().map(point))) {
            if (controller_->playing())
                controller_->togglePlayback();
            controller_->selectNode(QString::fromStdString(it->id));
            origin_ = point;
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
    auto delta = toProject(event->position()) - origin_;
    controller_->moveSelected(qRound(nodeOrigin_.x() + delta.x()), qRound(nodeOrigin_.y() + delta.y()));
}
void PreviewItem::mouseReleaseEvent(QMouseEvent *) {
    if (dragging_ && controller_)
        controller_->endGesture();
    dragging_ = false;
}
