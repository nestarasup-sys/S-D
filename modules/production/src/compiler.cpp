#include <QPointF>
#include <QUuid>
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <map>
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
                    const std::vector<std::pair<std::int64_t, std::int64_t>> &cues, std::int64_t sceneStart,
                    std::int64_t sceneEnd, const std::string &sceneId, const std::string &shotId,
                    std::int64_t samplesPerFrame, Project &project, QString &error) {
    auto node = directed.node;
    const auto cue = cues[directed.cue];
    const auto anchor = cueFrame(cue, directed.offsetSeconds, directed.cueProgress);
    node.start = anchor + (directed.presetTiming ? node.start : 0);
    const auto availableEnd =
        directed.utteranceOnly ? (cue.second + samplesPerFrame - 1) / samplesPerFrame : sceneEnd;
    const auto end =
        directed.presetTiming ? std::min(availableEnd, node.start + node.duration) : availableEnd;
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
            const auto frame =
                cueFrame(cues[directedKey.cue], directedKey.offsetSeconds, directedKey.cueProgress);
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
        if (!compileElement(child, cues, sceneStart, sceneEnd, sceneId, shotId, samplesPerFrame, project,
                            error))
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
VisualState beatState(const VisualState &current, const DirectedBeat &beat, double baseOpacity) {
    auto next = current;
    if (beat.action == "show")
        next.opacity = baseOpacity;
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
                         CameraState &camera) {
    std::vector<const Node *> chain{&target};
    auto parentId = target.parentId;
    while (!parentId.empty()) {
        auto parent = std::find_if(project.nodes.begin(), project.nodes.end(),
                                   [&](const auto &node) { return node.id == parentId; });
        if (parent == project.nodes.end())
            return false;
        chain.push_back(&*parent);
        parentId = parent->parentId;
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
    camera.zoom = std::clamp(
        std::min(project.width / (boundsWidth * padding), project.height / (boundsHeight * padding)), 0.1,
        10.0);
    return std::isfinite(camera.x) && std::isfinite(camera.y) && std::isfinite(camera.zoom);
}
} // namespace
CompileResult compileProduction(const ProductionRecipe &recipe, const QMap<QString, PcmAudio> &utterances,
                                const QString &audioOutput, const TimingPlan *timing,
                                const WordTimingMap *wordTimings) {
    CompileResult r;
    auto &p = r.project;
    p = makeEmpty();
    p.schemaVersion = recipe.version >= 3 ? 4 : 3;
    p.artDirection = recipe.artDirection;
    p.id = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
    p.name = recipe.title.toStdString();
    p.audioSource = audioOutput.toStdString();
    constexpr std::int64_t samplesPerFrame = ProductionSampleRate / 30;
    const bool fixed = recipe.timingMode == "fit-to-duration";
    if (fixed && (!timing || !timing->error.isEmpty() || timing->scenes.size() != recipe.scenes.size() ||
                  timing->targetFrames <= 0)) {
        r.error = "Plano temporal ajustado ausente ou incompatível.";
        return r;
    }
    std::vector<std::int64_t> transitions;
    int captionIndex = 0;
    CameraState previousShotCamera{640, 360, 1, 0};
    for (std::size_t sceneIndex = 0; sceneIndex < recipe.scenes.size(); ++sceneIndex) {
        const auto &scene = recipe.scenes[sceneIndex];
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
            const bool themed = recipe.version >= 3 && !recipe.original["artDirection"].isUndefined();
            auto caption = textNode("caption-" + utterance.id.toStdString(), utterance.text.toStdString(),
                                    themed ? recipe.artDirection.safeMargin : 80,
                                    themed && recipe.artDirection.captionPosition == "top"
                                        ? recipe.artDirection.safeMargin
                                        : themed ? 720 - recipe.artDirection.safeMargin - 80 : 620,
                                    themed ? 1280 - recipe.artDirection.safeMargin * 2 : 1120,
                                    80, themed ? recipe.artDirection.captionSize : 27, start, finish - start);
            caption.color = recipe.artDirection.textColor;
            caption.role = "caption";
            caption.sceneId = scene.id.toStdString();
            caption.animated = false;
            caption.motion = "none";
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
        if (!scene.background.isEmpty()) {
            Node bg;
            bg.id = "background-" + scene.id.toStdString();
            bg.kind = NodeKind::Native;
            bg.nativeId = scene.background.toStdString();
            bg.name = "Cenário";
            bg.x = 0;
            bg.y = 0;
            bg.width = 1280;
            bg.height = 720;
            bg.start = sceneStart;
            bg.duration = sceneDuration;
            bg.animated = false;
            bg.role = "background";
            bg.sceneId = scene.id.toStdString();
            // Insert backgrounds beneath previously collected caption overlays.
            p.nodes.insert(p.nodes.begin(), bg);
        }
        if (!scene.kicker.isEmpty()) {
            const bool themed = recipe.version >= 3 && !recipe.original["artDirection"].isUndefined();
            auto kicker = textNode("kicker-" + scene.id.toStdString(), scene.kicker.toStdString(),
                                   themed ? recipe.artDirection.safeMargin : 70, 44,
                                   themed ? 1280 - recipe.artDirection.safeMargin * 2 : 1140, 35,
                                   themed ? recipe.artDirection.bodySize * .55 : 19, sceneStart, sceneDuration);
            kicker.color = themed ? recipe.artDirection.secondaryColor : "#efb873";
            kicker.sceneId = scene.id.toStdString();
            kicker.role = "header";
            p.nodes.push_back(kicker);
        }
        if (!scene.title.isEmpty()) {
            const bool themed = recipe.version >= 3 && !recipe.original["artDirection"].isUndefined();
            auto title = textNode("heading-" + scene.id.toStdString(), scene.title.toStdString(),
                                  themed ? recipe.artDirection.safeMargin : 70, 92,
                                  themed ? 1280 - recipe.artDirection.safeMargin * 2 : 1140, 140,
                                  themed ? recipe.artDirection.titleSize : 58, sceneStart, sceneDuration);
            if (themed)
                title.color = recipe.artDirection.textColor;
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
            for (const auto &shot : scene.shots) {
                const auto start = resolveTime(shot.start, cues, wordStarts);
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
                for (const auto &element : directedShot.elements)
                    if (!compileElement(element, cues, shotStart, shotEnd, scene.id.toStdString(),
                                        directedShot.id.toStdString(), samplesPerFrame, p, r.error))
                        return r;

                CameraState shotCamera{directedShot.cameraX, directedShot.cameraY, directedShot.cameraZoom,
                                       directedShot.cameraRotation};
                if (!directedShot.focusTargetId.isEmpty()) {
                    auto focusTarget = std::find_if(p.nodes.begin(), p.nodes.end(), [&](const auto &node) {
                        return node.id == directedShot.focusTargetId.toStdString() &&
                               node.shotId == directedShot.id.toStdString();
                    });
                    if (focusTarget == p.nodes.end() ||
                        !frameCameraOnTarget(p, *focusTarget, shotStart, directedShot.focusPadding,
                                             shotCamera)) {
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
                p.camera.push_back({shotStart, shotCamera.x, shotCamera.y, shotCamera.zoom,
                                    shotCamera.rotation, "linear", directedShot.id.toStdString()});
                auto beats = directedShot.beats;
                for (const auto &beat : beats)
                    if (resolveTime(beat.at, cues, wordStarts) < 0) {
                        r.error = "wordIndex sem timestamp de alinhamento disponível no Visual Beat: " +
                                  beat.id;
                        return r;
                    }
                std::sort(beats.begin(), beats.end(), [&](const auto &a, const auto &b) {
                    return resolveTime(a.at, cues, wordStarts) < resolveTime(b.at, cues, wordStarts);
                });
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
                    stored.targetId = beat.targetId.toStdString();
                    stored.action = beat.action.toStdString();
                    stored.frame = beatFrame;
                    stored.durationFrames = beat.durationFrames;
                    stored.x = beat.x;
                    stored.y = beat.y;
                    stored.rotation = beat.rotation;
                    stored.scale = beat.scale;
                    stored.opacity = beat.opacity;
                    stored.zoom = beat.zoom;
                    p.visualBeats.push_back(std::move(stored));
                    if (beat.action == "camera") {
                        CameraState target = shotCamera;
                        target.x = beat.x.value_or(target.x);
                        target.y = beat.y.value_or(target.y);
                        target.rotation = beat.rotation.value_or(target.rotation);
                        target.zoom = beat.zoom.value_or(target.zoom);
                        if (beat.durationFrames > 0)
                            p.camera.push_back({beatFrame, shotCamera.x, shotCamera.y, shotCamera.zoom,
                                                shotCamera.rotation, "linear",
                                                directedShot.id.toStdString()});
                        p.camera.push_back({beatFrame + beat.durationFrames, target.x, target.y, target.zoom,
                                            target.rotation, "linear", directedShot.id.toStdString()});
                        shotCamera = target;
                    } else {
                        auto node = std::find_if(p.nodes.begin(), p.nodes.end(), [&](const Node &candidate) {
                            return candidate.id == beat.targetId.toStdString();
                        });
                        if (node == p.nodes.end() || node->shotId != directedShot.id.toStdString()) {
                            r.error =
                                "Visual Beat aponta para um elemento que não foi criado no shot: " + beat.id;
                            return r;
                        }
                        auto current = evaluate(*node, beatFrame);
                        const auto target = beatState(current, beat, node->opacity);
                        std::map<std::int64_t, TransformKeyframe> keysByFrame;
                        for (const auto &key : node->keyframes)
                            keysByFrame[key.frame] = key;
                        if (beat.durationFrames > 0)
                            keysByFrame[beatFrame] = {beatFrame,        current.x,     current.y,
                                                      current.rotation, current.scale, current.opacity};
                        const auto targetFrame = beatFrame + beat.durationFrames;
                        keysByFrame[targetFrame] = {targetFrame,     target.x,     target.y,
                                                    target.rotation, target.scale, target.opacity};
                        node->keyframes.clear();
                        for (const auto &[frame, key] : keysByFrame) {
                            Q_UNUSED(frame);
                            node->keyframes.push_back(key);
                        }
                    }
                }
                previousShotCamera = shotCamera;
                if (directedShot.focusFollow) {
                    auto focusTarget = std::find_if(p.nodes.begin(), p.nodes.end(), [&](const auto &node) {
                        return node.id == directedShot.focusTargetId.toStdString() &&
                               node.shotId == directedShot.id.toStdString();
                    });
                    if (focusTarget == p.nodes.end() ||
                        !frameCameraOnTarget(p, *focusTarget, shotEnd - 1, directedShot.focusPadding,
                                             previousShotCamera)) {
                        r.error =
                            "Não foi possível determinar o enquadramento final do shot: " + directedShot.id;
                        return r;
                    }
                    for (auto frame = shotStart + 1; frame < shotEnd - 1; ++frame) {
                        auto trackedCamera = previousShotCamera;
                        if (!frameCameraOnTarget(p, *focusTarget, frame, directedShot.focusPadding,
                                                 trackedCamera)) {
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
                if (!compileElement(directed, cues, sceneStart, sceneStart + sceneDuration,
                                    scene.id.toStdString(), {}, samplesPerFrame, p, r.error))
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
