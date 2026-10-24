#pragma once
#include <QCache>
#include <QImage>
#include <QPainter>
#include <studio/core/project.hpp>

namespace studio {
class CompositionPainter {
  public:
    void paint(QPainter &painter, const Project &, std::int64_t frame);
    QImage frame(const Project &, std::int64_t frame);
    void clearCache() {
        images_.clear();
    }

  private:
    QCache<QString, QImage> images_{128 * 1024}; // KiB; bounded decoded image memory.
};
} // namespace studio
