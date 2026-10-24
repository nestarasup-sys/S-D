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
enum class NodeKind { Text, Shape, Image, Native, Svg };
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
    std::string nativeId, sceneId, role;
    std::string pose{"neutral"}, motion{"rise"}, secondaryColor{"#f3c49a"};
    double rotation{0}, scale{1}, travelX{0}, travelY{0}, spinDegrees{0};
    int motionFrames{24}, exitFrames{0};
    bool operator==(const Node &) const = default;
};
struct SceneMarker {
    std::string id, name, narration;
    std::int64_t start{0}, duration{1};
};
struct Project {
    std::string id;
    std::string name{"Sem título"};
    int schemaVersion{2};
    int width{1280}, height{720};
    FrameRate fps;
    std::int64_t duration{360};
    std::vector<Node> nodes;
    std::vector<SceneMarker> scenes;
    std::string audioSource;
};
struct VisualState {
    double x, y, opacity, rotation{0}, scale{1};
};
std::string validate(const Project &project);
Project makeDemo();
Project makeEmpty();
const Node *findNode(const Project &, const std::string &id);
VisualState evaluate(const Node &, std::int64_t frame);
bool active(const Node &, std::int64_t frame);
} // namespace studio
