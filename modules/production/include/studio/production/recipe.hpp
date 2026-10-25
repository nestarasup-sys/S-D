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
    double targetDurationSeconds{0}; // Optional speech slot including pause in fit mode.
};
struct DirectedTime {
    int cue{0};
    double progress{-1}; // Relative to the measured speech, or -1 when offsetSeconds is used.
    double offsetSeconds{0};
};
struct DirectedKeyframe {
    int cue{0};
    double progress{0};
    double offsetSeconds{0};
    double cueProgress{-1}; // 0..1 follows the adjusted speech duration.
    double x{0}, y{0}, rotation{0}, scale{1}, opacity{1};
    std::string easing{"linear"};
};
struct DirectedCameraKeyframe {
    int cue{0};
    double progress{0};
    double offsetSeconds{0};
    double cueProgress{-1};
    double x{640}, y{360}, zoom{1}, rotation{0};
    std::string easing{"linear"};
};
struct DirectedElement {
    Node node;
    int cue{0};
    double progress{0};
    double offsetSeconds{0};
    double cueProgress{-1};
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
    QString transitionDirection{"left"};
    int transitionDurationFrames{0};
    DirectedTime start;
    double cameraX{640}, cameraY{360}, cameraZoom{1}, cameraRotation{0};
    QString focusTargetId;
    double focusPadding{1.25};
    bool focusFollow{false};
    std::vector<DirectedElement> elements;
    std::vector<DirectedBeat> beats;
};
struct DirectedScene {
    QString id, title, kicker, background;
    double targetDurationSeconds{0};
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
    QString timingMode{"natural"};
    double targetDurationSeconds{0};
    bool preserveVoicePitch{true};
    int editingIntensity{-1}; // Optional v3 preset for generated, otherwise-unspecified motion defaults.
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
