#include <QUuid>
#include <algorithm>
#include <cmath>
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
} // namespace
CompileResult compileProduction(const ProductionRecipe &recipe, const QMap<QString, PcmAudio> &utterances,
                                const QString &audioOutput) {
    CompileResult r;
    auto &p = r.project;
    p = makeEmpty();
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
        auto kicker = textNode("kicker-" + scene.id.toStdString(), scene.kicker.toStdString(), 70, 44, 1140,
                               35, 19, sceneStart, sceneDuration);
        kicker.color = "#efb873";
        kicker.sceneId = scene.id.toStdString();
        kicker.role = "header";
        p.nodes.push_back(kicker);
        auto title = textNode("heading-" + scene.id.toStdString(), scene.title.toStdString(), 70, 92, 1140,
                              140, 58, sceneStart, sceneDuration);
        title.sceneId = scene.id.toStdString();
        title.role = "header";
        p.nodes.push_back(title);
        for (const auto &directed : scene.elements) {
            auto n = directed.node;
            auto cue = cues[directed.cue];
            n.start = cue.first / samplesPerFrame + std::llround(directed.offsetSeconds * 30);
            auto end = directed.utteranceOnly ? (cue.second + samplesPerFrame - 1) / samplesPerFrame
                                              : sceneStart + sceneDuration;
            if (n.start >= end) {
                r.error = "Elemento começa depois do intervalo disponível: " + QString::fromStdString(n.id);
                return r;
            }
            n.duration = end - n.start;
            n.sceneId = scene.id.toStdString();
            p.nodes.push_back(n);
        }
    }
    p.duration = r.voice.samples.size() / samplesPerFrame;
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
