#include <QPointF>
#include <QJsonArray>
#include <QRegularExpression>
#include <QSet>
#include <QUuid>
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <map>
#include <numeric>
#include <unordered_map>
#include <studio/production/compiler.hpp>

namespace studio {
namespace {
std::int64_t cueFrame(const std::pair<std::int64_t, std::int64_t> &cue, double offsetSeconds,
                      double progress, int wordIndex = -1,
                      const std::vector<std::int64_t> *wordStarts = nullptr) {
    constexpr std::int64_t spf = ProductionSampleRate / 30;
    if (wordIndex >= 0) {
        if (!wordStarts || std::size_t(wordIndex) >= wordStarts->size())
            return -1;
        return cue.first / spf + std::llround(double((*wordStarts)[std::size_t(wordIndex)]) / spf);
    }
    if (progress >= 0) {
        const auto first = cue.first / spf;
        const auto last = std::max(first, (cue.second - 1) / spf);
        return first + std::llround(progress * (last - first));
    }
    return cue.first / spf + std::llround(offsetSeconds * 30);
}
QString srtTime(std::int64_t sample) {
    auto ms = sample * 1000 / ProductionSampleRate;
    return QString("%1:%2:%3,%4")
        .arg(ms / 3600000, 2, 10, QChar('0'))
        .arg(ms / 60000 % 60, 2, 10, QChar('0'))
        .arg(ms / 1000 % 60, 2, 10, QChar('0'))
        .arg(ms % 1000, 3, 10, QChar('0'));
}
Node textNode(std::string id, std::string text, double x, double y, double width, double height, double size,
              std::int64_t start, std::int64_t duration) {
    Node n;
    n.id = std::move(id);
    n.name = n.id;
    n.text = std::move(text);
    n.x = x;
    n.y = y;
    n.width = width;
    n.height = height;
    n.fontSize = size;
    n.start = start;
    n.duration = duration;
    return n;
}
bool compileElement(const DirectedElement &directed,
                    const std::vector<std::pair<std::int64_t, std::int64_t>> &cues,
                    const std::vector<std::vector<std::int64_t>> &wordStarts, std::int64_t sceneStart,
                    std::int64_t shotStart, std::int64_t sceneEnd, const std::string &sceneId,
                    const std::string &shotId,
                    std::int64_t samplesPerFrame, Project &project, QString &error) {
    auto node = directed.node;
    const auto cue = cues[directed.cue];
    const auto *words = directed.cue < int(wordStarts.size()) ? &wordStarts[std::size_t(directed.cue)] : nullptr;
    if (directed.wordIndex >= 0 && (!words || std::size_t(directed.wordIndex) >= words->size())) {
        error = "wordIndex sem timestamp de alinhamento disponível no elemento: " +
                QString::fromStdString(node.id);
        return false;
    }
    const auto anchor = directed.shotRelative
                            ? shotStart
                            : cueFrame(cue, directed.offsetSeconds, directed.cueProgress, directed.wordIndex, words);
    const auto presetLocalStart = node.start;
    const auto presetDuration = std::max<std::int64_t>(1, node.duration);
    node.start = anchor + (directed.presetTiming ? presetLocalStart : 0);
    const auto availableEnd =
        directed.utteranceOnly ? (cue.second + samplesPerFrame - 1) / samplesPerFrame : sceneEnd;
    auto end = directed.presetTiming ? std::min(availableEnd, node.start + node.duration) : availableEnd;
    if (directed.premiumTemplate && directed.presetTiming && end > node.start &&
        end - node.start < presetDuration) {
        const auto availableDuration = end - node.start;
        const double timeScale = double(availableDuration) / double(presetDuration);
        node.start = anchor + std::llround(double(presetLocalStart) * timeScale);
        node.duration = availableDuration;
        end = node.start + node.duration;
        for (auto &key : node.keyframes)
            key.frame = std::llround(double(key.frame) * timeScale);
        node.motionFrames = std::max(1, int(std::llround(double(node.motionFrames) * timeScale)));
        node.exitFrames = node.exitFrames == 0
                              ? 0
                              : std::max(1, int(std::llround(double(node.exitFrames) * timeScale)));
    }
    if (node.kind == NodeKind::Video && directed.videoDurationFrames > 0) {
        if (directed.videoDurationFrames > availableEnd - node.start) {
            error = "video.durationFrames ultrapassa o intervalo disponível: " + QString::fromStdString(node.id);
            return false;
        }
        end = node.start + directed.videoDurationFrames;
    }
    if (node.start >= end) {
        error = "Elemento começa depois do intervalo disponível: " + QString::fromStdString(node.id);
        return false;
    }
    node.duration = end - node.start;
    node.sceneId = sceneId;
    node.shotId = shotId;
    if (directed.presetTiming) {
        for (auto &key : node.keyframes)
            key.frame += anchor;
    } else {
        node.keyframes.clear();
        for (const auto &directedKey : directed.keyframes) {
            const auto *keyWords = directedKey.cue < int(wordStarts.size())
                                       ? &wordStarts[std::size_t(directedKey.cue)]
                                       : nullptr;
            if (directedKey.wordIndex >= 0 &&
                (!keyWords || std::size_t(directedKey.wordIndex) >= keyWords->size())) {
                error = "wordIndex sem timestamp de alinhamento disponível no keyframe de elemento: " +
                        QString::fromStdString(node.id);
                return false;
            }
            const auto frame = cueFrame(cues[directedKey.cue], directedKey.offsetSeconds,
                                        directedKey.cueProgress, directedKey.wordIndex, keyWords);
            node.keyframes.push_back({frame, directedKey.x, directedKey.y, directedKey.rotation,
                                      directedKey.scale, directedKey.opacity, directedKey.easing});
        }
    }
    for (const auto &key : node.keyframes)
        if (key.frame < node.start || key.frame >= node.start + node.duration) {
            error = "Keyframe fica fora do intervalo do elemento: " + QString::fromStdString(node.id);
            return false;
        }
    std::sort(node.keyframes.begin(), node.keyframes.end(),
              [](const auto &left, const auto &right) { return left.frame < right.frame; });
    for (std::size_t i = 1; i < node.keyframes.size(); ++i)
        if (node.keyframes[i - 1].frame == node.keyframes[i].frame) {
            error =
                "Dois keyframes do elemento resolvem para o mesmo quadro: " + QString::fromStdString(node.id);
            return false;
        }
    project.nodes.push_back(node);
    for (const auto &child : directed.children)
        if (!compileElement(child, cues, wordStarts, sceneStart, shotStart, sceneEnd, sceneId, shotId,
                            samplesPerFrame, project, error))
            return false;
    Q_UNUSED(sceneStart);
    return true;
}
std::int64_t resolveTime(const DirectedTime &time,
                         const std::vector<std::pair<std::int64_t, std::int64_t>> &cues,
                         const std::vector<std::vector<std::int64_t>> &wordStarts) {
    const auto *words = time.cue < int(wordStarts.size()) ? &wordStarts[std::size_t(time.cue)] : nullptr;
    return cueFrame(cues[time.cue], time.offsetSeconds, time.progress, time.wordIndex, words);
}
bool wordEndsPhrase(const QString &word, double &weight) {
    if (word.endsWith('.') || word.endsWith('?') || word.endsWith('!')) {
        weight = 3.0;
        return true;
    }
    if (word.endsWith(';') || word.endsWith(':')) {
        weight = 2.0;
        return true;
    }
    if (word.endsWith(',')) {
        weight = 0.75;
        return true;
    }
    return false;
}
QString normalizedWord(QString word) {
    word = word.toLower();
    while (!word.isEmpty() && !word.front().isLetterOrNumber())
        word.remove(0, 1);
    while (!word.isEmpty() && !word.back().isLetterOrNumber())
        word.chop(1);
    return word;
}
bool narrationShotStarts(const DirectedScene &scene,
                         const std::vector<std::pair<std::int64_t, std::int64_t>> &cues,
                         const std::vector<std::vector<std::int64_t>> &wordStarts,
                         std::int64_t sceneStart, std::int64_t sceneEnd, int editingIntensity,
                         std::vector<std::int64_t> &starts, QString &error) {
    constexpr std::int64_t samplesPerFrame = ProductionSampleRate / 30;
    if (scene.shots.size() < 2) {
        starts = {sceneStart};
        return true;
    }
    std::map<std::int64_t, double> candidates;
    const QSet<QString> semanticPivots{"mas", "porém", "contudo", "entretanto", "agora", "então",
                                      "portanto", "porque", "enquanto", "quando", "depois", "antes",
                                      "logo", "afinal", "todavia", "senão"};
    QString previousLastWord;
    for (std::size_t cueIndex = 0; cueIndex < scene.narration.size(); ++cueIndex) {
        const auto tokens = scene.narration[cueIndex].text.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (cueIndex >= wordStarts.size() || tokens.isEmpty() ||
            wordStarts[cueIndex].size() != std::size_t(tokens.size())) {
            error = "editing.rhythm narration exige timestamps reais para cada palavra da fala " +
                    scene.narration[cueIndex].id + ". Use uma voz que forneça alinhamento de palavras.";
            return false;
        }
        const auto cueStart = cues[cueIndex].first / samplesPerFrame;
        const auto addCandidate = [&](std::int64_t frame, double score) {
            if (frame <= sceneStart || frame >= sceneEnd)
                return;
            auto [it, inserted] = candidates.emplace(frame, score);
            if (!inserted)
                it->second = std::max(it->second, score);
        };
        if (cueIndex > 0) {
            double punctuation = 0;
            wordEndsPhrase(previousLastWord, punctuation);
            addCandidate(cueStart, 1.5 + punctuation);
        }
        for (int wordIndex = 1; wordIndex < tokens.size(); ++wordIndex) {
            double score = 0.05;
            wordEndsPhrase(tokens[wordIndex - 1], score);
            const auto nextWord = normalizedWord(tokens[wordIndex]);
            if (semanticPivots.contains(nextWord))
                score += 1.25;
            const auto frame = cueStart + std::llround(
                double(wordStarts[cueIndex][std::size_t(wordIndex)]) / samplesPerFrame);
            addCandidate(frame, score);
        }
        previousLastWord = tokens.back();
    }
    const int intensity = std::clamp(editingIntensity < 0 ? 50 : editingIntensity, 0, 100);
    const auto minimumShotFrames = std::max(6, 18 - int(std::lround(intensity * .12)));
    std::vector<std::int64_t> minimum(scene.shots.size(), minimumShotFrames);
    for (std::size_t i = 0; i < scene.shots.size(); ++i)
        minimum[i] = std::max<std::int64_t>(minimum[i], scene.shots[i].transitionDurationFrames);
    const auto required = std::accumulate(minimum.begin(), minimum.end(), std::int64_t{0});
    if (required > sceneEnd - sceneStart) {
        error = "A duração da cena é curta demais para distribuir os shots com editing.rhythm narration.";
        return false;
    }
    starts.clear();
    starts.reserve(scene.shots.size());
    starts.push_back(sceneStart);
    std::int64_t previous = sceneStart;
    const double targetDuration = double(sceneEnd - sceneStart) / scene.shots.size();
    const double timeWeight = .75 + intensity * .0225;
    const double phraseWeight = 2.5 - intensity * .015;
    for (std::size_t cut = 1; cut < scene.shots.size(); ++cut) {
        const auto lower = previous + minimum[cut - 1];
        const auto remaining = std::accumulate(minimum.begin() + std::ptrdiff_t(cut), minimum.end(),
                                               std::int64_t{0});
        const auto upper = sceneEnd - remaining;
        const auto ideal = sceneStart + std::llround(targetDuration * cut);
        auto chosen = candidates.end();
        double bestScore = -std::numeric_limits<double>::infinity();
        for (auto it = candidates.lower_bound(lower); it != candidates.end() && it->first <= upper; ++it) {
            const double timingCost = std::abs(double(it->first - ideal)) / std::max(1.0, targetDuration);
            const double score = it->second * phraseWeight - timingCost * timeWeight;
            if (score > bestScore || (score == bestScore && chosen != candidates.end() &&
                                      std::abs(it->first - ideal) < std::abs(chosen->first - ideal))) {
                chosen = it;
                bestScore = score;
            }
        }
        if (chosen == candidates.end()) {
            error = "Não há pontos de fala suficientes para distribuir os shots sem cortes rápidos demais. "
                    "Reduza a quantidade de shots ou aumente a duração da cena.";
            return false;
        }
        starts.push_back(chosen->first);
        previous = chosen->first;
    }
    return true;
}
VisualState beatState(const VisualState &current, const DirectedBeat &beat, double visibleOpacity) {
    auto next = current;
    if (beat.action == "show")
        next.opacity = visibleOpacity;
    else if (beat.action == "hide")
        next.opacity = 0;
    if (beat.x)
        next.x = *beat.x;
    if (beat.y)
        next.y = *beat.y;
    if (beat.rotation)
        next.rotation = *beat.rotation;
    if (beat.scale)
        next.scale = *beat.scale;
    if (beat.opacity)
        next.opacity = *beat.opacity;
    return next;
}
bool frameCameraOnTarget(const Project &project, const Node &target, std::int64_t frame, double padding,
                         CameraState &camera, const std::unordered_map<std::string, std::size_t> &nodeIndices,
                         double safeMargin = 0, double captionSafeBottom = 0) {
    std::vector<const Node *> chain{&target};
    auto parentId = target.parentId;
    while (!parentId.empty()) {
        const auto parent = nodeIndices.find(parentId);
        if (parent == nodeIndices.end() || parent->second >= project.nodes.size())
            return false;
        const auto &parentNode = project.nodes[parent->second];
        chain.push_back(&parentNode);
        parentId = parentNode.parentId;
    }
    const auto mapToWorld = [&](QPointF point) {
        for (const auto *part : chain) {
            const auto state = evaluate(*part, frame);
            const double angle = state.rotation * 3.14159265358979323846 / 180.0;
            const double dx = (point.x() - part->width / 2.0) * state.scale;
            const double dy = (point.y() - part->height / 2.0) * state.scale;
            point = {state.x + part->width / 2.0 + dx * std::cos(angle) - dy * std::sin(angle),
                     state.y + part->height / 2.0 + dx * std::sin(angle) + dy * std::cos(angle)};
        }
        return point;
    };
    const std::array<QPointF, 4> corners{mapToWorld({0, 0}), mapToWorld({target.width, 0}),
                                         mapToWorld({0, target.height}),
                                         mapToWorld({target.width, target.height})};
    double left = corners.front().x(), right = left, top = corners.front().y(), bottom = top;
    for (const auto &corner : corners) {
        left = std::min(left, corner.x());
        right = std::max(right, corner.x());
        top = std::min(top, corner.y());
        bottom = std::max(bottom, corner.y());
    }
    camera.x = (left + right) / 2.0;
    camera.y = (top + bottom) / 2.0;
    const double cameraAngle = camera.rotation * 3.14159265358979323846 / 180.0;
    double cameraLeft = std::numeric_limits<double>::infinity();
    double cameraRight = -cameraLeft;
    double cameraTop = cameraLeft;
    double cameraBottom = -cameraLeft;
    for (const auto &corner : corners) {
        const double dx = corner.x() - camera.x;
        const double dy = corner.y() - camera.y;
        const double x = dx * std::cos(cameraAngle) + dy * std::sin(cameraAngle);
        const double y = -dx * std::sin(cameraAngle) + dy * std::cos(cameraAngle);
        cameraLeft = std::min(cameraLeft, x);
        cameraRight = std::max(cameraRight, x);
        cameraTop = std::min(cameraTop, y);
        cameraBottom = std::max(cameraBottom, y);
    }
    const double boundsWidth = std::max(1.0, cameraRight - cameraLeft);
    const double boundsHeight = std::max(1.0, cameraBottom - cameraTop);
    const double availableWidth = project.width * (1 - safeMargin * 2);
    const double availableHeight = project.height * (1 - safeMargin * 2 - captionSafeBottom);
    camera.zoom = std::clamp(
        std::min(availableWidth / (boundsWidth * padding), availableHeight / (boundsHeight * padding)), 0.1,
        10.0);
    // Keep the focus inside the safe viewport above the reserved caption band.
    camera.y += captionSafeBottom * project.height / (2 * camera.zoom);
    return std::isfinite(camera.x) && std::isfinite(camera.y) && std::isfinite(camera.zoom);
}
} // namespace
CompileResult compileProduction(const ProductionRecipe &recipe, const QMap<QString, PcmAudio> &utterances,
                                const QString &audioOutput, const TimingPlan *timing,
                                const WordTimingMap *wordTimings, const QString &voiceDuckingSource) {
    CompileResult r;
    auto &p = r.project;
    p = makeEmpty();
    p.width = recipe.width;
    p.height = recipe.height;
    p.schemaVersion = recipe.version >= 3 ? 7 : 3;
    p.artDirection = recipe.artDirection;
    bool hasCaptionHighlight = recipe.original.value("artDirectionPreset").isString() ||
                               recipe.original.value("artDirection").toObject().contains("captionHighlightColor");
    for (const auto &sceneValue : recipe.original.value("scenes").toArray()) {
        const auto sceneObject = sceneValue.toObject();
        hasCaptionHighlight = hasCaptionHighlight ||
            sceneObject.value("artDirection").toObject().contains("captionHighlightColor") ||
            sceneObject.value("artDirectionPreset").isString();
        for (const auto &shotValue : sceneObject.value("shots").toArray()) {
            const auto shotObject = shotValue.toObject();
            hasCaptionHighlight = hasCaptionHighlight ||
                shotObject.value("artDirection").toObject().contains("captionHighlightColor") ||
                shotObject.value("artDirectionPreset").isString();
        }
    }
    if (hasCaptionHighlight)
        p.schemaVersion = 14;
    p.id = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
    p.name = recipe.title.toStdString();
    p.audioSource = audioOutput.toStdString();
    p.voiceDuckingSource = voiceDuckingSource.toStdString();
    constexpr std::int64_t samplesPerFrame = ProductionSampleRate / 30;
    const bool fixed = recipe.timingMode == "fit-to-duration";
    if (fixed && (!timing || !timing->error.isEmpty() || timing->scenes.size() != recipe.scenes.size() ||
                  timing->targetFrames <= 0)) {
        r.error = "Plano temporal ajustado ausente ou incompatível.";
        return r;
    }
    std::vector<std::int64_t> transitions;
    int captionIndex = 0, compiledCameraPresetKeys = 0, compiledCameraPathKeys = 0;
    CameraState previousShotCamera{p.width / 2.0, p.height / 2.0, 1, 0};
    std::unordered_map<std::string, std::size_t> nodeIndices;
    const auto indexNewNodes = [&](std::size_t first) {
        nodeIndices.reserve(p.nodes.size());
        for (std::size_t index = first; index < p.nodes.size(); ++index)
            nodeIndices.emplace(p.nodes[index].id, index);
    };
    for (std::size_t sceneIndex = 0; sceneIndex < recipe.scenes.size(); ++sceneIndex) {
        const auto &scene = recipe.scenes[sceneIndex];
        const ArtDirection &sceneArt = scene.artDirection ? *scene.artDirection : recipe.artDirection;
        const bool sceneThemed = recipe.version >= 3 &&
                                 (recipe.original.contains("artDirection") ||
                                  recipe.original.contains("artDirectionPreset") || scene.artDirection.has_value());
        const TimedScene *sceneTiming = fixed ? &timing->scenes[sceneIndex] : nullptr;
        if (fixed &&
            (sceneTiming->cues.size() != scene.narration.size() || sceneTiming->durationFrames <= 0)) {
            r.error = "Plano temporal de cena inválido.";
            return r;
        }
        const auto sceneSample = std::int64_t(r.voice.samples.size());
        const auto sceneStart = sceneSample / samplesPerFrame;
        transitions.push_back(sceneSample);
        std::vector<std::pair<std::int64_t, std::int64_t>> cues;
        std::vector<std::vector<std::int64_t>> wordStarts;
        for (std::size_t cueIndex = 0; cueIndex < scene.narration.size(); ++cueIndex) {
            const auto &utterance = scene.narration[cueIndex];
            const TimedCue *cueTiming = fixed ? &sceneTiming->cues[cueIndex] : nullptr;
            if (!utterances.contains(utterance.id) || utterances[utterance.id].samples.empty()) {
                r.error = "Narração ausente: " + utterance.id;
                return r;
            }
            const auto &audio = utterances[utterance.id].samples;
            if (fixed && (cueTiming->id != utterance.id || cueTiming->speechFrames <= 0 ||
                          cueTiming->slotFrames < cueTiming->speechFrames ||
                          audio.size() > std::size_t(cueTiming->speechFrames * samplesPerFrame))) {
                r.error = "Voz ajustada não corresponde ao plano: " + utterance.id;
                return r;
            }
            if (audio.size() > ProductionSampleRate * 30) {
                r.error = "Trecho de narração excede 30 segundos.";
                return r;
            }
            std::int64_t begin = r.voice.samples.size();
            r.voice.samples.insert(r.voice.samples.end(), audio.begin(), audio.end());
            std::int64_t end = r.voice.samples.size();
            cues.push_back({begin, end});
            wordStarts.push_back(wordTimings && wordTimings->contains(utterance.id)
                                     ? wordTimings->value(utterance.id)
                                     : std::vector<std::int64_t>{});
            r.subtitles += QString::number(++captionIndex) + "\n" + srtTime(begin) + " --> " + srtTime(end) +
                           "\n" + utterance.text + "\n\n";
            auto start = begin / samplesPerFrame, finish = (end + samplesPerFrame - 1) / samplesPerFrame;
            const auto captionMargin = sceneThemed ? sceneArt.safeMargin : 80;
            const auto captionHeight = sceneThemed ? std::max(80., sceneArt.captionSize * 2.5) : 80.;
            auto caption = textNode("caption-" + utterance.id.toStdString(), utterance.text.toStdString(),
                                    captionMargin,
                                    sceneThemed && sceneArt.captionPosition == "top"
                                        ? captionMargin
                                        : p.height - (sceneThemed ? captionMargin + captionHeight : 100),
                                    p.width - captionMargin * 2,
                                    captionHeight, sceneThemed ? sceneArt.captionSize : 27, start, finish - start);
            caption.color = sceneArt.textColor;
            caption.role = "caption";
            caption.sceneId = scene.id.toStdString();
            caption.animated = false;
            caption.motion = "none";
            caption.wordAnimationPreset = recipe.wordAnimationPreset.toStdString();
            caption.wordAnimationDurationFrames = recipe.wordAnimationDurationFrames;
            caption.wordAnimationStartScale = recipe.wordAnimationStartScale;
            caption.wordAnimationTravelY = recipe.wordAnimationTravelY;
            const auto &cueWords = wordStarts.back();
            const auto captionWords = QString::fromStdString(caption.text).split(QRegularExpression("\\s+"),
                                                                                 Qt::SkipEmptyParts);
            if (captionWords.size() == int(cueWords.size())) {
                for (int wordIndex = 0; wordIndex < captionWords.size(); ++wordIndex) {
                    const auto wordStart = begin / samplesPerFrame +
                                           std::llround(double(cueWords[std::size_t(wordIndex)]) / samplesPerFrame);
                    if (wordStart < finish) {
                        if (!caption.captionWords.empty() && caption.captionWords.back().start == wordStart)
                            caption.captionWords.back().text = captionWords[wordIndex].toStdString();
                        else
                            caption.captionWords.push_back({captionWords[wordIndex].toStdString(), wordStart, 0});
                    }
                }
                for (std::size_t wordIndex = 0; wordIndex < caption.captionWords.size(); ++wordIndex)
                    caption.captionWords[wordIndex].end =
                        wordIndex + 1 < caption.captionWords.size() ? caption.captionWords[wordIndex + 1].start
                                                                    : finish;
                if (!caption.captionWords.empty())
                    p.schemaVersion = std::max(p.schemaVersion, 7);
            }
            if (recipe.wordAnimationPreset != "highlight") {
                if (recipe.wordAnimationPreset != "none" && recipe.renderCaptions &&
                    caption.captionWords.empty() && !captionWords.empty()) {
                    r.error = "wordAnimation exige timestamps de voz para " + utterance.id;
                    return r;
                }
                p.schemaVersion = std::max(p.schemaVersion, 12);
            }
            if (recipe.renderCaptions)
                p.nodes.push_back(caption);
            if (fixed) {
                const auto desiredEnd = begin + std::int64_t(cueTiming->slotFrames) * samplesPerFrame;
                r.voice.samples.resize(std::size_t(desiredEnd), 0);
            } else
                r.voice.samples.resize(r.voice.samples.size() + ProductionSampleRate / 7, 0);
        }
        if (fixed) {
            const auto sceneEnd = sceneSample + std::int64_t(sceneTiming->durationFrames) * samplesPerFrame;
            if (r.voice.samples.size() > std::size_t(sceneEnd)) {
                r.error = "Falas planejadas ultrapassam a cena " + scene.id;
                return r;
            }
            r.voice.samples.resize(std::size_t(sceneEnd), 0);
        } else
            r.voice.samples.resize(r.voice.samples.size() + ProductionSampleRate / 5, 0);
        auto rounded = ((r.voice.samples.size() + samplesPerFrame - 1) / samplesPerFrame) * samplesPerFrame;
        r.voice.samples.resize(rounded, 0);
        if (rounded > ProductionSampleRate * 600) {
            r.error = "Produção excede dez minutos.";
            return r;
        }
        auto sceneDuration = std::int64_t(rounded / samplesPerFrame) - sceneStart;
        std::string narration;
        for (const auto &u : scene.narration) {
            if (!narration.empty())
                narration += " ";
            narration += u.text.toStdString();
        }
        p.scenes.push_back(
            {scene.id.toStdString(), scene.title.toStdString(), narration, sceneStart, sceneDuration});
        if (scene.artDirection) {
            p.scenes.back().artDirection = scene.artDirection;
            p.schemaVersion = std::max(p.schemaVersion, 11);
        }
        for (const auto &directedTrack : scene.audioTracks) {
            const auto trackStart = resolveTime(directedTrack.start, cues, wordStarts);
            const auto sceneEnd = sceneStart + sceneDuration;
            if (trackStart < sceneStart || trackStart >= sceneEnd) {
                r.error = "Início da pista de áudio fora da cena: " + directedTrack.id;
                return r;
            }
            p.audioTracks.push_back({directedTrack.id.toStdString(), directedTrack.name.toStdString(),
                                     directedTrack.source.toStdString(), trackStart, sceneEnd - trackStart,
                                     directedTrack.gain, false, directedTrack.duckUnderVoice,
                                     directedTrack.duckAmount, directedTrack.fadeInFrames,
                                     directedTrack.fadeOutFrames});
            p.audioTracks.back().gainAutomation = directedTrack.gainAutomation;
            p.audioTracks.back().sourceInSeconds = directedTrack.sourceInSeconds;
            p.audioTracks.back().playbackRate = directedTrack.playbackRate;
            if (directedTrack.sourceInSeconds != 0 || directedTrack.playbackRate != 1)
                p.schemaVersion = std::max(p.schemaVersion, 13);
            if (!directedTrack.gainAutomation.empty())
                p.schemaVersion = std::max(p.schemaVersion, 8);
            if (directedTrack.fadeInFrames > 0 || directedTrack.fadeOutFrames > 0)
                p.schemaVersion = std::max(p.schemaVersion, 6);
        }
        if (!scene.background.isEmpty()) {
            Node bg;
            bg.id = "background-" + scene.id.toStdString();
            bg.kind = NodeKind::Native;
            bg.nativeId = scene.background.toStdString();
            bg.name = "Cenário";
            bg.x = 0;
            bg.y = 0;
            bg.width = p.width;
            bg.height = p.height;
            bg.start = sceneStart;
            bg.duration = sceneDuration;
            bg.animated = false;
            bg.role = "background";
            bg.sceneId = scene.id.toStdString();
            // Insert backgrounds beneath previously collected caption overlays.
            p.nodes.insert(p.nodes.begin(), bg);
        }
        if (!scene.kicker.isEmpty()) {
            auto kicker = textNode("kicker-" + scene.id.toStdString(), scene.kicker.toStdString(),
                                   sceneThemed ? sceneArt.safeMargin : 70, 44,
                                   sceneThemed ? p.width - sceneArt.safeMargin * 2 : p.width - 140, 35,
                                   sceneThemed ? sceneArt.bodySize * .55 : 19, sceneStart, sceneDuration);
            kicker.color = sceneThemed ? sceneArt.secondaryColor : "#efb873";
            kicker.sceneId = scene.id.toStdString();
            kicker.role = "header";
            p.nodes.push_back(kicker);
        }
        if (!scene.title.isEmpty()) {
            auto title = textNode("heading-" + scene.id.toStdString(), scene.title.toStdString(),
                                  sceneThemed ? sceneArt.safeMargin : 70, 92,
                                  sceneThemed ? p.width - sceneArt.safeMargin * 2 : p.width - 140, 140,
                                  sceneThemed ? sceneArt.titleSize : 58, sceneStart, sceneDuration);
            if (sceneThemed)
                title.color = sceneArt.textColor;
            title.sceneId = scene.id.toStdString();
            title.role = "header";
            p.nodes.push_back(title);
        }
        if (recipe.version >= 3) {
            struct ResolvedShot {
                const DirectedShot *source;
                std::int64_t start;
            };
            std::vector<ResolvedShot> shots;
            shots.reserve(scene.shots.size());
            std::vector<std::int64_t> automaticStarts;
            if (recipe.editingRhythm == "narration" &&
                !narrationShotStarts(scene, cues, wordStarts, sceneStart, sceneStart + sceneDuration,
                                     recipe.editingIntensity, automaticStarts, r.error))
                return r;
            if (!automaticStarts.empty() && automaticStarts.size() != scene.shots.size()) {
                r.error = "A distribuição automática não corresponde à quantidade de shots.";
                return r;
            }
            for (std::size_t shotIndex = 0; shotIndex < scene.shots.size(); ++shotIndex) {
                const auto &shot = scene.shots[shotIndex];
                const auto start = shot.autoStart ? automaticStarts[shotIndex]
                                                  : resolveTime(shot.start, cues, wordStarts);
                if (start < 0) {
                    r.error = "wordIndex sem timestamp de alinhamento disponível no shot: " + shot.id;
                    return r;
                }
                shots.push_back({&shot, start});
            }
            std::sort(shots.begin(), shots.end(),
                      [](const auto &a, const auto &b) { return a.start < b.start; });
            if (shots.empty() || shots.front().start != sceneStart) {
                r.error = "O primeiro shot deve começar no início da cena: " + scene.id;
                return r;
            }
            for (std::size_t shotIndex = 0; shotIndex < shots.size(); ++shotIndex) {
                const auto &directedShot = *shots[shotIndex].source;
                const auto shotStart = shots[shotIndex].start;
                const auto shotEnd =
                    shotIndex + 1 < shots.size() ? shots[shotIndex + 1].start : sceneStart + sceneDuration;
                if (shotStart < sceneStart || shotStart >= shotEnd || shotEnd > sceneStart + sceneDuration) {
                    r.error = "Início de shot repetido ou fora dos limites da cena: " + directedShot.id;
                    return r;
                }
                if (directedShot.transitionIn != "cut" &&
                    (shotIndex == 0 || directedShot.transitionDurationFrames > shotEnd - shotStart)) {
                    r.error = "Transição precisa de um shot anterior e deve caber no intervalo do plano: " +
                              directedShot.id;
                    return r;
                }
                p.shots.push_back({directedShot.id.toStdString(), scene.id.toStdString(),
                                   directedShot.name.toStdString(), directedShot.transitionIn.toStdString(),
                                   shotStart, shotEnd - shotStart, directedShot.transitionDurationFrames,
                                   directedShot.transitionDirection.toStdString()});
                if (directedShot.artDirection) {
                    p.shots.back().artDirection = directedShot.artDirection;
                    p.schemaVersion = std::max(p.schemaVersion, 11);
                }
                for (const auto &element : directedShot.elements) {
                    const auto firstNode = p.nodes.size();
                    if (!compileElement(element, cues, wordStarts, shotStart, shotStart, shotEnd,
                                        scene.id.toStdString(), directedShot.id.toStdString(), samplesPerFrame, p,
                                        r.error))
                        return r;
                    indexNewNodes(firstNode);
                }

                CameraState shotCamera{directedShot.cameraX, directedShot.cameraY, directedShot.cameraZoom,
                                       directedShot.cameraRotation};
                struct ResolvedCameraTarget {
                    std::int64_t frame;
                    CameraState camera;
                    const DirectedCameraTarget *source;
                };
                std::vector<ResolvedCameraTarget> resolvedCameraPath;
                if (directedShot.cameraPath) {
                    if (!directedShot.cameraPreset.isEmpty() || !directedShot.focusTargetId.isEmpty() ||
                        directedShot.focusFollow ||
                        std::any_of(directedShot.beats.begin(), directedShot.beats.end(),
                                    [](const auto &beat) { return beat.action == "camera"; })) {
                        r.error = "camera.path é exclusivo de focus, cameraPreset e Visual Beats de câmera no shot: " +
                                  directedShot.id;
                        return r;
                    }
                    std::int64_t previousTargetFrame = -1;
                    for (std::size_t targetIndex = 0;
                         targetIndex < directedShot.cameraPath->targets.size(); ++targetIndex) {
                        const auto &point = directedShot.cameraPath->targets[targetIndex];
                        const auto targetFrame = resolveTime(point.at, cues, wordStarts);
                        if (targetFrame < shotStart || targetFrame >= shotEnd || targetFrame <= previousTargetFrame ||
                            (targetIndex == 0 && targetFrame != shotStart)) {
                            r.error = "camera.path precisa começar no corte e ter tempos crescentes dentro do shot: " +
                                      directedShot.id;
                            return r;
                        }
                        const auto nodeFound = nodeIndices.find(point.targetId.toStdString());
                        if (nodeFound == nodeIndices.end() || nodeFound->second >= p.nodes.size() ||
                            p.nodes[nodeFound->second].shotId != directedShot.id.toStdString()) {
                            r.error = "camera.path aponta para alvo ausente ou de outro shot: " + point.targetId;
                            return r;
                        }
                        const auto &target = p.nodes[nodeFound->second];
                        const auto hasShow = std::any_of(directedShot.beats.begin(), directedShot.beats.end(),
                            [&](const auto &beat) {
                                return beat.action == "show" && beat.targetId == point.targetId;
                            });
                        if (point.reveal == "spatial" && hasShow) {
                            r.error = "camera.path reveal spatial não aceita show para o mesmo alvo; use reveal combined: " +
                                      point.targetId;
                            return r;
                        }
                        if ((point.reveal == "temporal" || point.reveal == "combined") && !hasShow) {
                            r.error = "camera.path reveal temporal/combined exige Visual Beat show para o alvo: " +
                                      point.targetId;
                            return r;
                        }
                        const auto effectiveOpacity = [&](const Node &node, std::int64_t frame) {
                            double opacity = 1;
                            const Node *current = &node;
                            std::size_t depth = 0;
                            while (current && depth++ <= 8) {
                                opacity *= evaluate(*current, frame).opacity;
                                if (current->parentId.empty())
                                    break;
                                const auto parent = nodeIndices.find(current->parentId);
                                current = parent == nodeIndices.end() ? nullptr : &p.nodes[parent->second];
                            }
                            return opacity;
                        };
                        if ((point.reveal == "temporal" || point.reveal == "combined") &&
                            effectiveOpacity(target, shotStart) > 0) {
                            r.error = "camera.path reveal temporal/combined exige o alvo inicialmente invisível: " +
                                      point.targetId;
                            return r;
                        }
                        if (point.reveal == "spatial" &&
                            (!active(target, targetFrame) || effectiveOpacity(target, targetFrame) <= 0)) {
                            r.error = "camera.path reveal spatial exige o alvo visível no espaço da cena: " +
                                      point.targetId + " (frame=" + QString::number(targetFrame) +
                                      ", intervalo=" + QString::number(target.start) + ".." +
                                      QString::number(target.start + target.duration - 1) +
                                      ", opacity=" + QString::number(effectiveOpacity(target, targetFrame), 'f', 3) + ")";
                            return r;
                        }
                        CameraState targetCamera{directedShot.cameraX, directedShot.cameraY,
                                                 directedShot.cameraZoom, directedShot.cameraRotation};
                        if (!frameCameraOnTarget(p, target, targetFrame, point.padding, targetCamera, nodeIndices,
                                                 directedShot.cameraPath->safeMargin,
                                                 directedShot.cameraPath->captionSafeBottom)) {
                            r.error = "Não foi possível enquadrar o target de camera.path: " + point.targetId;
                            return r;
                        }
                        resolvedCameraPath.push_back({targetFrame, targetCamera, &point});
                        previousTargetFrame = targetFrame;
                    }
                    shotCamera = resolvedCameraPath.front().camera;
                }
                if (!directedShot.focusTargetId.isEmpty()) {
                    const auto focus = nodeIndices.find(directedShot.focusTargetId.toStdString());
                    if (focus == nodeIndices.end() || focus->second >= p.nodes.size() ||
                        p.nodes[focus->second].shotId != directedShot.id.toStdString() ||
                        !frameCameraOnTarget(p, p.nodes[focus->second], shotStart, directedShot.focusPadding,
                                             shotCamera, nodeIndices)) {
                        r.error = "Não foi possível enquadrar o alvo da câmera no shot: " + directedShot.id;
                        return r;
                    }
                    if (directedShot.focusFollow &&
                        std::any_of(directedShot.beats.begin(), directedShot.beats.end(),
                                    [](const auto &beat) { return beat.action == "camera"; })) {
                        r.error = "focus.follow não pode ser combinado com Visual Beats de câmera no shot: " +
                                  directedShot.id;
                        return r;
                    }
                }
                if (shotStart > 0)
                    p.camera.push_back({shotStart - 1, previousShotCamera.x, previousShotCamera.y,
                                        previousShotCamera.zoom, previousShotCamera.rotation, "linear",
                                        directedShot.id.toStdString()});
                CameraKeyframe initialCamera{shotStart, shotCamera.x, shotCamera.y, shotCamera.zoom,
                                             shotCamera.rotation, "linear", directedShot.id.toStdString()};
                if (!resolvedCameraPath.empty()) {
                    const auto &firstTarget = resolvedCameraPath.front();
                    initialCamera.targetId = firstTarget.source->targetId.toStdString();
                    initialCamera.revealMode = firstTarget.source->reveal.toStdString();
                    initialCamera.safeMargin = directedShot.cameraPath->safeMargin;
                    initialCamera.captionSafeBottom = directedShot.cameraPath->captionSafeBottom;
                    p.schemaVersion = 15;
                }
                p.camera.push_back(std::move(initialCamera));
                if (!resolvedCameraPath.empty()) {
                    for (std::size_t i = 1; i < resolvedCameraPath.size(); ++i) {
                        const auto &target = resolvedCameraPath[i];
                        if (++compiledCameraPathKeys > 4096) {
                            r.error = "A produção excede 4096 pontos compilados de camera.path.";
                            return r;
                        }
                        CameraKeyframe cameraKey{target.frame, target.camera.x, target.camera.y,
                                                 target.camera.zoom, target.camera.rotation,
                                                 target.source->easing, directedShot.id.toStdString()};
                        cameraKey.targetId = target.source->targetId.toStdString();
                        cameraKey.revealMode = target.source->reveal.toStdString();
                        cameraKey.safeMargin = directedShot.cameraPath->safeMargin;
                        cameraKey.captionSafeBottom = directedShot.cameraPath->captionSafeBottom;
                        p.camera.push_back(std::move(cameraKey));
                        shotCamera = target.camera;
                    }
                } else if (!directedShot.cameraPreset.isEmpty()) {
                    if (directedShot.focusFollow ||
                        std::any_of(directedShot.beats.begin(), directedShot.beats.end(),
                                    [](const auto &beat) { return beat.action == "camera"; })) {
                        r.error = "cameraPreset não pode ser combinado com focus.follow ou Visual Beats de câmera: " +
                                  directedShot.id;
                        return r;
                    }
                    const auto preset = recipe.cameraPresets.constFind(directedShot.cameraPreset);
                    if (preset == recipe.cameraPresets.cend()) {
                        r.error = "Preset de câmera ausente: " + directedShot.cameraPreset;
                        return r;
                    }
                    for (const auto &key : *preset) {
                        const auto frame = shotStart + qRound64(key.offsetSeconds * 30.0);
                        if (frame >= shotEnd) {
                            r.error = "cameraPreset ultrapassa a duração do shot: " + directedShot.id;
                            return r;
                        }
                        if (++compiledCameraPresetKeys > 4096) {
                            r.error = "A produção excede 4096 keyframes compilados de cameraPreset.";
                            return r;
                        }
                        p.camera.push_back({frame, key.x, key.y, key.zoom, key.rotation, key.easing,
                                            directedShot.id.toStdString()});
                        shotCamera = {key.x, key.y, key.zoom, key.rotation};
                    }
                }
                auto beats = directedShot.beats;
                for (const auto &beat : beats)
                    if (resolveTime(beat.at, cues, wordStarts) < 0) {
                        r.error = "wordIndex sem timestamp de alinhamento disponível no Visual Beat: " +
                                  beat.id;
                        return r;
                    }
                std::stable_sort(beats.begin(), beats.end(), [&](const auto &a, const auto &b) {
                    return resolveTime(a.at, cues, wordStarts) < resolveTime(b.at, cues, wordStarts);
                });
                std::unordered_map<std::string, double> visibleOpacityByNode;
                std::unordered_map<std::string, std::vector<TransformKeyframe>> opacityKeyframesByNode;
                for (const auto &beat : beats) {
                    const auto beatFrame = resolveTime(beat.at, cues, wordStarts);
                    if (beatFrame < shotStart || beatFrame >= shotEnd ||
                        beat.durationFrames > shotEnd - beatFrame - 1) {
                        r.error = "Visual Beat fica fora do shot: " + beat.id;
                        return r;
                    }
                    VisualBeat stored;
                    stored.id = beat.id.toStdString();
                    stored.shotId = directedShot.id.toStdString();
                    stored.action = beat.action.toStdString();
                    stored.frame = beatFrame;
                    stored.durationFrames = beat.durationFrames;
                    stored.x = beat.x;
                    stored.y = beat.y;
                    stored.rotation = beat.rotation;
                    stored.scale = beat.scale;
                    stored.opacity = beat.opacity;
                    stored.zoom = beat.zoom;
                    if (beat.action == "sound") {
                        stored.targetId = ("sfx-" + beat.id).toStdString();
                        const auto sceneEnd = sceneStart + sceneDuration;
                        p.audioTracks.push_back({stored.targetId, "Efeito: " + beat.id.toStdString(),
                                                 beat.soundSource.toStdString(), beatFrame,
                                                 sceneEnd - beatFrame, beat.soundGain, false, false, .65});
                        p.schemaVersion = std::max(p.schemaVersion, 9);
                    } else
                        stored.targetId = beat.targetId.toStdString();
                    p.visualBeats.push_back(std::move(stored));
                    if (beat.action == "sound") {
                        continue;
                    } else if (beat.action == "camera") {
                        CameraState target = shotCamera;
                        target.x = beat.x.value_or(target.x);
                        target.y = beat.y.value_or(target.y);
                        target.rotation = beat.rotation.value_or(target.rotation);
                        target.zoom = beat.zoom.value_or(target.zoom);
                        if (beat.durationFrames > 0)
                            p.camera.push_back({beatFrame, shotCamera.x, shotCamera.y, shotCamera.zoom,
                                                shotCamera.rotation, "linear",
                                                directedShot.id.toStdString()});
                        else if (beatFrame > shotStart)
                            p.camera.push_back({beatFrame - 1, shotCamera.x, shotCamera.y, shotCamera.zoom,
                                                shotCamera.rotation, "linear",
                                                directedShot.id.toStdString()});
                        p.camera.push_back({beatFrame + beat.durationFrames, target.x, target.y, target.zoom,
                                            target.rotation, "linear", directedShot.id.toStdString()});
                        shotCamera = target;
                    } else {
                        const auto nodeFound = nodeIndices.find(beat.targetId.toStdString());
                        if (nodeFound == nodeIndices.end() || nodeFound->second >= p.nodes.size() ||
                            p.nodes[nodeFound->second].shotId != directedShot.id.toStdString()) {
                            r.error =
                                "Visual Beat aponta para um elemento que não foi criado no shot: " + beat.id;
                            return r;
                        }
                        auto &node = p.nodes[nodeFound->second];
                        auto current = evaluate(node, beatFrame);
                        auto sourceKeyframes = opacityKeyframesByNode.find(node.id);
                        if (sourceKeyframes == opacityKeyframesByNode.end())
                            sourceKeyframes = opacityKeyframesByNode.emplace(node.id, node.keyframes).first;
                        auto visibleOpacity = visibleOpacityByNode.find(node.id);
                        if (visibleOpacity == visibleOpacityByNode.end()) {
                            double initialOpacity = node.opacity;
                            for (const auto &key : sourceKeyframes->second)
                                if (key.frame <= beatFrame && key.opacity > 0)
                                    initialOpacity = key.opacity;
                            visibleOpacity = visibleOpacityByNode.emplace(
                                node.id, initialOpacity > 0 ? initialOpacity : 1.0).first;
                        }
                        for (const auto &key : sourceKeyframes->second)
                            if (key.frame <= beatFrame && key.opacity > 0)
                                visibleOpacity->second = key.opacity;
                        if (beat.action == "transform" && beat.opacity && *beat.opacity > 0)
                            visibleOpacity->second = *beat.opacity;
                        const auto target = beatState(current, beat, visibleOpacity->second);
                        std::map<std::int64_t, TransformKeyframe> keysByFrame;
                        for (const auto &key : node.keyframes)
                            keysByFrame[key.frame] = key;
                        if ((beat.action == "show" || beat.action == "hide" || beat.durationFrames == 0) &&
                            node.start < beatFrame) {
                            const auto beforeFrame = beatFrame - 1;
                            const auto before = evaluate(node, beforeFrame);
                            keysByFrame[beforeFrame] = {beforeFrame, before.x, before.y, before.rotation,
                                                        before.scale, before.opacity};
                        }
                        if (beat.durationFrames > 0)
                            keysByFrame[beatFrame] = {beatFrame,        current.x,     current.y,
                                                      current.rotation, current.scale, current.opacity};
                        const auto targetFrame = beatFrame + beat.durationFrames;
                        keysByFrame[targetFrame] = {targetFrame,     target.x,     target.y,
                                                    target.rotation, target.scale, target.opacity};
                        node.keyframes.clear();
                        for (const auto &[frame, key] : keysByFrame) {
                            Q_UNUSED(frame);
                            node.keyframes.push_back(key);
                        }
                    }
                }
                previousShotCamera = shotCamera;
                if (directedShot.focusFollow) {
                    const auto focus = nodeIndices.find(directedShot.focusTargetId.toStdString());
                    if (focus == nodeIndices.end() || focus->second >= p.nodes.size() ||
                        p.nodes[focus->second].shotId != directedShot.id.toStdString() ||
                        !frameCameraOnTarget(p, p.nodes[focus->second], shotEnd - 1,
                                             directedShot.focusPadding, previousShotCamera, nodeIndices)) {
                        r.error =
                            "Não foi possível determinar o enquadramento final do shot: " + directedShot.id;
                        return r;
                    }
                    for (auto frame = shotStart + 1; frame < shotEnd - 1; ++frame) {
                        auto trackedCamera = previousShotCamera;
                        if (!frameCameraOnTarget(p, p.nodes[focus->second], frame, directedShot.focusPadding,
                                                 trackedCamera, nodeIndices)) {
                            r.error =
                                "Não foi possível acompanhar o alvo da câmera no shot: " + directedShot.id;
                            return r;
                        }
                        p.camera.push_back({frame, trackedCamera.x, trackedCamera.y, trackedCamera.zoom,
                                            trackedCamera.rotation, "linear", directedShot.id.toStdString()});
                    }
                    p.camera.push_back({shotEnd - 1, previousShotCamera.x, previousShotCamera.y,
                                        previousShotCamera.zoom, previousShotCamera.rotation, "linear",
                                        directedShot.id.toStdString()});
                }
            }
        } else {
            for (const auto &directed : scene.elements) {
                if (!compileElement(directed, cues, wordStarts, sceneStart, sceneStart,
                                    sceneStart + sceneDuration, scene.id.toStdString(), {}, samplesPerFrame, p,
                                    r.error))
                    return r;
            }
        }
        for (const auto &directedCamera : scene.camera) {
            const auto frame =
                cueFrame(cues[directedCamera.cue], directedCamera.offsetSeconds, directedCamera.cueProgress);
            p.camera.push_back({frame, directedCamera.x, directedCamera.y, directedCamera.zoom,
                                directedCamera.rotation, directedCamera.easing});
        }
    }
    p.duration = r.voice.samples.size() / samplesPerFrame;
    if (std::any_of(p.nodes.begin(), p.nodes.end(), [](const auto &node) { return node.kind == NodeKind::Video; }))
        p.schemaVersion = std::max(p.schemaVersion, 5);
    if (std::any_of(p.nodes.begin(), p.nodes.end(), [](const auto &node) {
            return node.kind == NodeKind::Video &&
                   (node.videoSourceInSeconds != 0 || node.videoPlaybackRate != 1 || node.videoCropX != 0 ||
                    node.videoCropY != 0 || node.videoCropWidth != 1 || node.videoCropHeight != 1);
        }))
        p.schemaVersion = std::max(p.schemaVersion, 10);
    if (std::any_of(p.nodes.begin(), p.nodes.end(), [](const auto &node) { return node.textAutoFit; }))
        p.schemaVersion = 15;
    if (fixed && p.duration != timing->targetFrames) {
        r.error = "A produção não alcançou o número de quadros solicitado.";
        return r;
    }
    std::map<std::int64_t, CameraKeyframe> cameraByFrame;
    for (const auto &key : p.camera)
        cameraByFrame[key.frame] = key;
    p.camera.clear();
    for (const auto &[frame, key] : cameraByFrame) {
        Q_UNUSED(frame);
        p.camera.push_back(key);
    }
    // Captions always remain above scenery and illustration layers.
    std::stable_partition(p.nodes.begin(), p.nodes.end(), [](const Node &n) { return n.role != "caption"; });
    if (auto error = validate(p); !error.empty()) {
        r.error = QString::fromStdString(error);
        return r;
    }
    r.mix = r.voice;
    addSoundtrack(r.mix, recipe.musicGain, recipe.effectsGain, transitions);
    return r;
}
} // namespace studio
