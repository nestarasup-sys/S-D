#pragma once
#include "recipe.hpp"
#include <QMap>
#include <QStringList>
#include <map>
#include <studio/media/wave.hpp>

namespace studio {
struct TimedCue {
    QString id;
    int slotFrames{0};
    int speechFrames{0};
};
struct TimedScene {
    int durationFrames{0};
    std::vector<TimedCue> cues;
};
struct TimingPlan {
    std::vector<TimedScene> scenes;
    int targetFrames{0};
    QStringList warnings;
    QString error;
};
using WordTimingMap = QMap<QString, std::vector<std::int64_t>>;
// Run after the voice is measured. Reject requests requiring >15% acceleration.
TimingPlan planNarrationTiming(const ProductionRecipe &, const QMap<QString, PcmAudio> &utterances);
struct CompileResult {
    Project project;
    PcmAudio voice, mix;
    QString subtitles, error;
};
CompileResult compileProduction(const ProductionRecipe &, const QMap<QString, PcmAudio> &utterances,
                                const QString &audioOutput, const TimingPlan *timing = nullptr,
                                const WordTimingMap *wordTimings = nullptr);
} // namespace studio
