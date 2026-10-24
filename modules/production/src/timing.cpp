#include <algorithm>
#include <cmath>
#include <numeric>
#include <studio/production/compiler.hpp>

namespace studio {
namespace {
constexpr int samplesPerFrame = ProductionSampleRate / 30;
constexpr double maxVoiceSpeed = 1.15;
void distribute(std::vector<int> &result, const std::vector<int> &indices,
                const std::vector<int> &weights, int extra) {
    if (indices.empty() || extra <= 0)
        return;
    std::int64_t total = 0;
    for (int index : indices)
        total += std::max(1, weights[index]);
    std::int64_t carry = 0;
    for (int index : indices) {
        carry += std::int64_t(extra) * std::max(1, weights[index]);
        const int amount = int(carry / total);
        result[index] += amount;
        carry %= total;
    }
}
} // namespace
TimingPlan planNarrationTiming(const ProductionRecipe &recipe, const QMap<QString, PcmAudio> &utterances) {
    TimingPlan plan;
    if (recipe.timingMode == "natural")
        return plan;
    if (recipe.timingMode != "fit-to-duration" || recipe.targetDurationSeconds <= 0) {
        plan.error = "Modo de tempo não suportado.";
        return plan;
    }
    plan.targetFrames = int(std::llround(recipe.targetDurationSeconds * 30));
    if (plan.targetFrames <= 0 || plan.targetFrames > 18000) {
        plan.error = "Duração-alvo excede dez minutos.";
        return plan;
    }
    std::vector<int> sceneFrames, sceneWeights, openScenes;
    plan.scenes.resize(recipe.scenes.size());
    for (std::size_t si = 0; si < recipe.scenes.size(); ++si) {
        const auto &scene = recipe.scenes[si];
        auto &target = plan.scenes[si];
        int minimumScene = 0, preferredScene = 6;
        for (const auto &u : scene.narration) {
            if (!utterances.contains(u.id) || utterances[u.id].samples.empty()) {
                plan.error = "Narração ausente: " + u.id;
                return plan;
            }
            const auto count = std::int64_t(utterances[u.id].samples.size());
            if (count > ProductionSampleRate * 30) {
                plan.error = "Trecho de narração excede 30 segundos: " + u.id;
                return plan;
            }
            const int minSpeech = std::max(1, int(std::ceil(count / (samplesPerFrame * maxVoiceSpeed))));
            const int natural = int((count + samplesPerFrame - 1) / samplesPerFrame);
            const int fixed = u.targetDurationSeconds > 0
                                  ? int(std::llround(u.targetDurationSeconds * 30)) : 0;
            if (fixed > 0 && fixed < minSpeech) {
                plan.error = QString("Fala %1 pede %2 frames; precisa de %3 no mínimo (limite de 15%%).")
                                 .arg(u.id).arg(fixed).arg(minSpeech);
                return plan;
            }
            minimumScene += fixed > 0 ? fixed : minSpeech;
            preferredScene += std::max(natural + 4, fixed);
            target.cues.push_back({u.id, fixed, 0});
        }
        const int fixed = scene.targetDurationSeconds > 0
                              ? int(std::llround(scene.targetDurationSeconds * 30)) : 0;
        if (fixed > 0 && fixed < minimumScene) {
            plan.error = QString("Cena %1 requer no mínimo %2 frames.")
                             .arg(scene.id).arg(minimumScene);
            return plan;
        }
        sceneFrames.push_back(fixed > 0 ? fixed : minimumScene);
        sceneWeights.push_back(std::max(1, preferredScene - minimumScene));
        if (fixed == 0)
            openScenes.push_back(int(si));
    }
    const int allocatedScenes = std::accumulate(sceneFrames.begin(), sceneFrames.end(), 0);
    if (allocatedScenes > plan.targetFrames ||
        (openScenes.empty() && allocatedScenes != plan.targetFrames)) {
        plan.error = "As durações das cenas não cabem no total planejado.";
        return plan;
    }
    distribute(sceneFrames, openScenes, sceneWeights, plan.targetFrames - allocatedScenes);
    for (std::size_t si = 0; si < recipe.scenes.size(); ++si) {
        const auto &scene = recipe.scenes[si];
        auto &target = plan.scenes[si];
        target.durationFrames = sceneFrames[si];
        std::vector<int> slots, weights, openCues;
        for (std::size_t ci = 0; ci < scene.narration.size(); ++ci) {
            const auto &u = scene.narration[ci];
            const int count = int(utterances[u.id].samples.size());
            const int natural = (count + samplesPerFrame - 1) / samplesPerFrame;
            const int minimum = std::max(1, int(std::ceil(count / (samplesPerFrame * maxVoiceSpeed))));
            const int fixed = target.cues[ci].slotFrames;
            slots.push_back(fixed > 0 ? fixed : minimum);
            weights.push_back(std::max(1, natural + 4 - minimum));
            if (fixed == 0)
                openCues.push_back(int(ci));
        }
        const int allocated = std::accumulate(slots.begin(), slots.end(), 0);
        if (allocated > target.durationFrames) {
            plan.error = "Falas excedem a cena " + scene.id + ". Amplie a cena ou reduza o texto.";
            return plan;
        }
        distribute(slots, openCues, weights, target.durationFrames - allocated);
        int assignedToCues = 0;
        for (std::size_t ci = 0; ci < scene.narration.size(); ++ci) {
            auto &cue = target.cues[ci];
            const auto count = std::int64_t(utterances[cue.id].samples.size());
            const int natural = int((count + samplesPerFrame - 1) / samplesPerFrame);
            cue.slotFrames = slots[ci];
            cue.speechFrames = std::min(natural, cue.slotFrames);
            assignedToCues += cue.slotFrames;
            if (cue.slotFrames - cue.speechFrames > 90)
                plan.warnings.push_back(
                    QString("Pausa de mais de 3 segundos após a fala %1. Reveja ritmo e roteiro.")
                        .arg(cue.id));
            if (double(count) / (cue.speechFrames * samplesPerFrame) > maxVoiceSpeed + 1e-9) {
                plan.error = "Fala " + cue.id + " exigiria aceleração acima de 15%.";
                return plan;
            }
        }
        if (target.durationFrames - assignedToCues > 150)
            plan.warnings.push_back(
                QString("Cena %1 contém mais de 5 segundos de pausa final.").arg(scene.id));
    }
    return plan;
}
} // namespace studio
