#pragma once
#include <QPainter>
#include <studio/core/project.hpp>
namespace studio {
void paintNative(QPainter &, const Node &, std::int64_t frame);
}
