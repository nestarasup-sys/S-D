#pragma once
#include <QCache>
#include <QImage>
#include <QPainter>
#include <QSvgRenderer>
#include <cstdint>
#include <map>
#include <memory>
#include <studio/core/project.hpp>
#include <studio/composition/render_snapshot.hpp>

namespace studio {
struct VideoCacheStats {
    std::uint64_t hits{0}, misses{0}, decodedFrames{0}, seekStarts{0};
};
struct SvgCacheStats {
    std::uint64_t hits{0}, misses{0};
};

class CompositionPainter {
  public:
    CompositionPainter();
    ~CompositionPainter();
    CompositionPainter(const CompositionPainter &) = delete;
    CompositionPainter &operator=(const CompositionPainter &) = delete;
    void paint(QPainter &painter, const Project &, std::int64_t frame);
    QImage frame(const Project &, std::int64_t frame);
    QImage rasterNodeLayer(const Project &, const RenderFrameSnapshot &, const RenderNodeSnapshot &,
                           double supersamplingScale = 1.0);
    QImage rasterBackgroundLayer(const Project &, const ArtDirection &);
    void clearCache();
    VideoCacheStats videoCacheStats() const {
        return videoCacheStats_;
    }
    SvgCacheStats svgCacheStats() const {
        return svgCacheStats_;
    }

  private:
    struct VideoDecoder;
    QImage videoFrame(const Node &, const Project &, std::int64_t frame);
    QCache<QString, QImage> images_{128 * 1024};      // KiB; bounded decoded image memory.
    QCache<QString, QImage> videoFrames_{64 * 1024};  // KiB; keeps recent video seeks within a fixed budget.
    QCache<QString, QSvgRenderer> svgRenderers_{128}; // Bounded parsed SVG documents.
    VideoCacheStats videoCacheStats_;
    SvgCacheStats svgCacheStats_;
    std::map<std::string, std::unique_ptr<VideoDecoder>> videoDecoders_;
};
} // namespace studio
