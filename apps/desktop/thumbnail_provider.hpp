#pragma once
#include <QQuickImageProvider>
#include <algorithm>
#include <studio/composition/painter.hpp>
#include <studio/core/library.hpp>

class ThumbnailProvider : public QQuickImageProvider {
  public:
    ThumbnailProvider() : QQuickImageProvider(QQuickImageProvider::Image) {}
    QImage requestImage(const QString &id, QSize *size, const QSize &requested) override {
        auto nativeId = id.startsWith("native/") ? id.mid(7) : QString{};
        auto asset = studio::libraryItem(nativeId.toStdString());
        if (!asset)
            return {};
        auto project = studio::makeEmpty();
        project.width = 640;
        project.height = 360;
        studio::Node n;
        n.id = "thumbnail";
        n.kind = studio::NodeKind::Native;
        n.nativeId = asset->id;
        n.color = "#efb873";
        n.animated = false;
        double ratio = std::min(560. / asset->width, 290. / asset->height);
        n.width = asset->width * ratio;
        n.height = asset->height * ratio;
        n.x = (640 - n.width) / 2;
        n.y = (360 - n.height) / 2;
        if (asset->category == "cenários") {
            n.x = n.y = 0;
            n.width = 640;
            n.height = 360;
        }
        project.nodes = {n};
        studio::CompositionPainter painter;
        auto result = painter.frame(project, 60);
        if (size)
            *size = result.size();
        return requested.isValid() ? result.scaled(requested, Qt::KeepAspectRatio, Qt::SmoothTransformation)
                                   : result;
    }
};
