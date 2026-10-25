#pragma once
#include <QCache>
#include <QImage>
#include <QPainter>
#include <map>
#include <memory>
#include <studio/core/project.hpp>

namespace studio {
class CompositionPainter {
  public:
    CompositionPainter();
    ~CompositionPainter();
    CompositionPainter(const CompositionPainter &) = delete;
    CompositionPainter &operator=(const CompositionPainter &) = delete;
    void paint(QPainter &painter, const Project &, std::int64_t frame);
    QImage frame(const Project &, std::int64_t frame);
    void clearCache();

  private:
    struct VideoDecoder;
    QImage videoFrame(const Node &, const Project &, std::int64_t frame);
    QCache<QString, QImage> images_{128 * 1024}; // KiB; bounded decoded image memory.
    std::map<std::string, std::unique_ptr<VideoDecoder>> videoDecoders_;
};
} // namespace studio
