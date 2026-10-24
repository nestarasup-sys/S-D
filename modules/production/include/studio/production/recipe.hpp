#pragma once
#include <QByteArray>
#include <QJsonObject>
#include <QString>
#include <optional>
#include <studio/core/project.hpp>
#include <vector>

namespace studio {
struct Utterance {
    QString id, text;
};
struct DirectedTime {
    int cue{0};
    double progress{0};
    double offsetSeconds{0};
};
struct DirectedKeyframe {
    int cue{0};
    double progress{0};
    double offsetSeconds{0};
    double x{0}, y{0}, rotation{0}, scale{1}, opacity{1};
    std::string easing{"linear"};
};
struct DirectedCameraKeyframe {
    int cue{0};
    double progress{0};
    double offsetSeconds{0};
    double x{640}, y{360}, zoom{1}, rotation{0};
    std::string easing{"linear"};
};
struct DirectedElement {
    Node node;
    int cue{0};
    double progress{0};
    double offsetSeconds{0};
    bool utteranceOnly{false};
    bool presetTiming{false}; // Preserved frame offsets, lifetimes and keys from a stored composition.
    std::vector<DirectedKeyframe> keyframes;
    std::vector<DirectedElement> children;
};
struct DirectedBeat {
    QString id, action, targetId;
    DirectedTime at;
    int durationFrames{0};
    std::optional<double> x, y, rotation, scale, opacity, zoom;
};
struct DirectedShot {
    QString id, name, transitionIn{"cut"};
    DirectedTime start;
    double cameraX{640}, cameraY{360}, cameraZoom{1}, cameraRotation{0};
    std::vector<DirectedElement> elements;
    std::vector<DirectedBeat> beats;
};
struct DirectedScene {
    QString id, title, kicker, background;
    std::vector<Utterance> narration;
    std::vector<DirectedElement> elements;
    std::vector<DirectedCameraKeyframe> camera;
    std::vector<DirectedShot> shots;
};
struct ProductionRecipe {
    int version{1};
    QString title, voice{"auto"};
    int speechRate{0};
    bool renderCaptions{true};
    double musicGain{.025}, effectsGain{.035};
    std::vector<DirectedScene> scenes;
    QJsonObject original;
};
struct RecipeResult {
    ProductionRecipe recipe;
    QString error;
};
RecipeResult parseRecipe(const QByteArray &, const QString &baseDirectory, const QJsonObject &library = {});
QJsonObject capabilities();
QString aiGuide(const QJsonObject &library = {});
} // namespace studio
