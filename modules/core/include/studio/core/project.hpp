#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace studio {
struct FrameRate {
    int numerator{30};
    int denominator{1};
    double value() const;
};
enum class NodeKind { Text, Shape, Image };
struct Node {
    std::string id;
    std::string name;
    NodeKind kind{NodeKind::Text};
    std::string text;
    std::string color{"#e4f5e9"};
    std::string source; // Local image path; asset catalogue replaces this in the next slice.
    std::int64_t start{0};
    std::int64_t duration{360};
    double x{100}, y{100}, width{600}, height{100};
    double opacity{1};
    double fontSize{64};
    bool animated{true};
    bool operator==(const Node &) const = default;
};
struct Project {
    std::string id;
    std::string name{"Sem título"};
    int schemaVersion{1};
    int width{1280}, height{720};
    FrameRate fps;
    std::int64_t duration{360};
    std::vector<Node> nodes;
};
struct VisualState {
    double x, y, opacity;
};
std::string validate(const Project &project);
Project makeDemo();
Project makeEmpty();
const Node *findNode(const Project &, const std::string &id);
VisualState evaluate(const Node &, std::int64_t frame);
bool active(const Node &, std::int64_t frame);
} // namespace studio
