#pragma once
#include "recipe.hpp"
#include <QMap>
#include <studio/media/wave.hpp>

namespace studio {
struct CompileResult {
    Project project;
    PcmAudio voice, mix;
    QString subtitles, error;
};
CompileResult compileProduction(const ProductionRecipe &, const QMap<QString, PcmAudio> &utterances,
                                const QString &audioOutput);
} // namespace studio
