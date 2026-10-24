#include <QUuid>
#include <algorithm>
#include <cmath>
#include <map>
#include <studio/production/compiler.hpp>

namespace studio {
namespace {
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
                    std::int64_t sceneEnd, const std::string &sceneId, std::int64_t samplesPerFrame,
                    Project &project, QString &error) {
    auto node = directed.node;
    const auto cue = cues[directed.cue];
    const auto anchor = cue.first / samplesPerFrame + std::llround(directed.offsetSeconds * 30);
    node.start = anchor + (directed.presetTiming ? node.start : 0);
    const auto availableEnd =
        directed.utteranceOnly ? (cue.second + samplesPerFrame - 1) / samplesPerFrame : sceneEnd;
    const auto end = directed.presetTiming ? std::min(availableEnd, node.start + node.duration)
                                           : availableEnd;
    if (node.start >= end) {
        error = "Elemento começa depois do intervalo disponível: " + QString::fromStdString(node.id);
        return false;
    }
    node.duration = end - node.start;
    node.sceneId = sceneId;
    if (directed.presetTiming) {
        for (auto &key : node.keyframes)
            key.frame += anchor;
    } else {
        node.keyframes.clear();
        for (const auto &directedKey : directed.keyframes) {
            const auto frame =
                cues[directedKey.cue].first / samplesPerFrame + std::llround(directedKey.offsetSeconds * 30);
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
        if (!compileElement(child, cues, sceneStart, sceneEnd, sceneId, samplesPerFrame, project, error))
            return false;
    Q_UNUSED(sceneStart);
    return true;
}
} // namespace
CompileResult compileProduction(const ProductionRecipe &recipe, const QMap<QString, PcmAudio> &utterances,
                                const QString &audioOutput) {
    CompileResult r;
    auto &p = r.project;
    p = makeEmpty();
    p.schemaVersion = 3;
    p.id = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
    p.name = recipe.title.toStdString();
    p.audioSource = audioOutput.toStdString();
    constexpr std::int64_t samplesPerFrame = ProductionSampleRate / 30;
    std::vector<std::int64_t> transitions;
    int captionIndex = 0;
    for (const auto &scene : recipe.scenes) {
        const auto sceneSample = std::int64_t(r.voice.samples.size());
        const auto sceneStart = sceneSample / samplesPerFrame;
        transitions.push_back(sceneSample);
        std::vector<std::pair<std::int64_t, std::int64_t>> cues;
        for (const auto &utterance : scene.narration) {
            if (!utterances.contains(utterance.id) || utterances[utterance.id].samples.empty()) {
                r.error = "Narração ausente: " + utterance.id;
                return r;
            }
            const auto &audio = utterances[utterance.id].samples;
            if (audio.size() > ProductionSampleRate * 30) {
                r.error = "Trecho de narração excede 30 segundos.";
                return r;
            }
            std::int64_t begin = r.voice.samples.size();
            r.voice.samples.insert(r.voice.samples.end(), audio.begin(), audio.end());
            std::int64_t end = r.voice.samples.size();
            cues.push_back({begin, end});
            r.subtitles += QString::number(++captionIndex) + "\n" + srtTime(begin) + " --> " + srtTime(end) +
                           "\n" + utterance.text + "\n\n";
            auto start = begin / samplesPerFrame, finish = (end + samplesPerFrame - 1) / samplesPerFrame;
            auto caption = textNode("caption-" + utterance.id.toStdString(), utterance.text.toStdString(), 80,
                                    620, 1120, 80, 27, start, finish - start);
            caption.role = "caption";
            caption.sceneId = scene.id.toStdString();
            caption.animated = false;
            caption.motion = "none";
            if (recipe.renderCaptions)
                p.nodes.push_back(caption);
            r.voice.samples.resize(r.voice.samples.size() + ProductionSampleRate / 7, 0);
        }
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
            auto kicker = textNode("kicker-" + scene.id.toStdString(), scene.kicker.toStdString(), 70, 44,
                                   1140, 35, 19, sceneStart, sceneDuration);
            kicker.color = "#efb873";
            kicker.sceneId = scene.id.toStdString();
            kicker.role = "header";
            p.nodes.push_back(kicker);
        }
        if (!scene.title.isEmpty()) {
            auto title = textNode("heading-" + scene.id.toStdString(), scene.title.toStdString(), 70, 92,
                                  1140, 140, 58, sceneStart, sceneDuration);
            title.sceneId = scene.id.toStdString();
            title.role = "header";
            p.nodes.push_back(title);
        }
        for (const auto &directed : scene.elements) {
            if (!compileElement(directed, cues, sceneStart, sceneStart + sceneDuration,
                                scene.id.toStdString(), samplesPerFrame, p, r.error))
                return r;
        }
        for (const auto &directedCamera : scene.camera) {
            const auto frame = cues[directedCamera.cue].first / samplesPerFrame +
                               std::llround(directedCamera.offsetSeconds * 30);
            p.camera.push_back(
                {frame, directedCamera.x, directedCamera.y, directedCamera.zoom, directedCamera.rotation,
                 directedCamera.easing});
        }
    }
    p.duration = r.voice.samples.size() / samplesPerFrame;
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
