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
struct DirectedElement {
    Node node;
    int cue{0};
    double offsetSeconds{0};
    bool utteranceOnly{false};
};
struct DirectedScene {
    QString id, title, kicker, background;
    std::vector<Utterance> narration;
    std::vector<DirectedElement> elements;
};
struct ProductionRecipe {
    QString title, voice{"auto"};
    int speechRate{0};
    double musicGain{.025}, effectsGain{.035};
    std::vector<DirectedScene> scenes;
    QJsonObject original;
};
struct RecipeResult {
    ProductionRecipe recipe;
    QString error;
};
RecipeResult parseRecipe(const QByteArray &, const QString &baseDirectory);
QJsonObject capabilities();
QString aiGuide();
} // namespace studio
