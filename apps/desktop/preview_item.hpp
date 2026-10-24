#pragma once
#include "editor_controller.hpp"
#include <QQuickPaintedItem>
#include <QTransform>
#include <QtQml/qqmlregistration.h>
#include <studio/composition/painter.hpp>

class PreviewItem : public QQuickPaintedItem {
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(EditorController *controller READ controller WRITE setController NOTIFY controllerChanged)
  public:
    explicit PreviewItem(QQuickItem *parent = nullptr);
    EditorController *controller() const {
        return controller_;
    }
    void setController(EditorController *);
    void paint(QPainter *) override;
  signals:
    void controllerChanged();

  protected:
    void mousePressEvent(QMouseEvent *) override;
    void mouseMoveEvent(QMouseEvent *) override;
    void mouseReleaseEvent(QMouseEvent *) override;

  private:
    QPointF toProject(QPointF) const;
    EditorController *controller_{nullptr};
    studio::CompositionPainter painter_;
    QPointF origin_, nodeOrigin_;
    QTransform dragParentTransform_;
    bool dragging_{false};
};
