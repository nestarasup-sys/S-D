#pragma once
#include <QByteArray>
#include <QJsonObject>
#include <QString>
#include <studio/core/project.hpp>
#include <vector>

namespace studio {
struct Utterance {
    QString id, text;
};
struct DirectedKeyframe {
    int cue{0};
    double offsetSeconds{0};
    double x{0}, y{0}, rotation{0}, scale{1}, opacity{1};
};
struct DirectedCameraKeyframe {
    int cue{0};
    double offsetSeconds{0};
    double x{640}, y{360}, zoom{1}, rotation{0};
};
struct DirectedElement {
    Node node;
    int cue{0};
    double offsetSeconds{0};
    bool utteranceOnly{false};
    std::vector<DirectedKeyframe> keyframes;
    std::vector<DirectedElement> children;
};
struct DirectedScene {
    QString id, title, kicker, background;
    std::vector<Utterance> narration;
    std::vector<DirectedElement> elements;
    std::vector<DirectedCameraKeyframe> camera;
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
