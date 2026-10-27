#include <algorithm>
#include <cctype>
#include <cmath>
#include <new>
#include <studio/core/library.hpp>
#include <studio/core/project.hpp>
#include <unordered_map>
#include <unordered_set>

namespace studio {
double FrameRate::value() const {
    return double(numerator) / denominator;
}
bool supportedEasing(const std::string &name) {
    return name == "linear" || name == "ease-in" || name == "ease-out" || name == "ease-in-out";
}
namespace {
bool validColor(const std::string &value) {
    return value.size() == 7 && value[0] == '#' &&
           std::all_of(value.begin() + 1, value.end(),
                       [](unsigned char c) { return std::isxdigit(c); });
}
bool validArtDirection(const ArtDirection &art) {
    return validColor(art.backgroundColor) && validColor(art.glowColor) && validColor(art.primaryColor) &&
           validColor(art.secondaryColor) && validColor(art.textColor) && validColor(art.cardColor) &&
           validColor(art.captionBackgroundColor) && validColor(art.captionHighlightColor) &&
           !art.fontFamily.empty() && art.fontFamily.size() <= 120 &&
           (art.captionPosition == "top" || art.captionPosition == "bottom") &&
           std::isfinite(art.titleSize) && art.titleSize >= 8 && art.titleSize <= 160 &&
           std::isfinite(art.bodySize) && art.bodySize >= 8 && art.bodySize <= 160 &&
           std::isfinite(art.captionSize) && art.captionSize >= 8 && art.captionSize <= 120 &&
           art.safeMargin >= 0 && art.safeMargin <= 300;
}
double eased(double t, const std::string &curve) {
    if (curve == "ease-in")
        return t * t * t;
    if (curve == "ease-out")
        return 1 - std::pow(1 - t, 3);
    if (curve == "ease-in-out")
        return t * t * (3 - 2 * t);
    return t; // Preserve original interpolation when easing was absent.
}
} // namespace
std::string validate(const Project &p) try {
    if (p.schemaVersion < 2 || p.schemaVersion > 14)
        return "Versão de projeto não suportada.";
    if (p.id.empty() || p.name.empty())
        return "Projeto sem identificação ou nome.";
    if (p.width < 64 || p.height < 64 || p.width > 3840 || p.height > 2160 || p.width % 2 || p.height % 2)
        return "Resolução inválida: use dimensões pares entre 64 e 3840 × 2160.";
    if (!validArtDirection(p.artDirection))
        return "Direção de arte global inválida.";
    if (p.fps.numerator <= 0 || p.fps.numerator > 120000 || p.fps.denominator <= 0 ||
        p.fps.denominator > 10000 || p.fps.value() < 1 || p.fps.value() > 120)
        return "Taxa de quadros inválida.";
    if (p.duration < 1 || p.duration > 18000 || p.shots.size() > 256 || p.visualBeats.size() > 4096 ||
        p.audioTracks.size() > 32)
        return "Duração, quantidade de shots, Visual Beats ou pistas de áudio fora dos limites suportados.";
    if (p.schemaVersion < 4 && !p.audioTracks.empty())
        return "Pistas de áudio exigem a versão 4 do projeto.";
    if (p.schemaVersion < 5 && std::any_of(p.nodes.begin(), p.nodes.end(),
                                           [](const auto &node) { return node.kind == NodeKind::Video; }))
        return "Camadas de vídeo exigem a versão 5 do projeto.";
    if (p.schemaVersion < 6 && std::any_of(p.audioTracks.begin(), p.audioTracks.end(), [](const auto &track) {
            return track.fadeInFrames != 0 || track.fadeOutFrames != 0;
        }))
        return "Fades de pista de áudio exigem a versão 6 do projeto.";
    if (p.schemaVersion < 8 && std::any_of(p.audioTracks.begin(), p.audioTracks.end(), [](const auto &track) {
            return !track.gainAutomation.empty();
        }))
        return "Automação de ganho exige a versão 8 do projeto.";
    if (p.schemaVersion < 13 && std::any_of(p.audioTracks.begin(), p.audioTracks.end(), [](const auto &track) {
            return track.sourceInSeconds != 0 || track.playbackRate != 1 || !track.linkedVideoNodeId.empty();
        }))
        return "Trim, velocidade ou vínculo de áudio com vídeo exige a versão 13 do projeto.";
    std::unordered_set<std::string> ids;
    std::unordered_set<std::string> parentIds;
    std::unordered_set<std::string> sceneIds;
    std::unordered_set<std::string> shotIds;
    std::unordered_set<std::string> beatIds;
    std::unordered_map<std::string, const Node *> nodesById;
    nodesById.reserve(p.nodes.size());
    std::unordered_map<std::string, const SceneMarker *> scenesById;
    scenesById.reserve(p.scenes.size());
    std::int64_t sceneEnd = 0;
    for (const auto &scene : p.scenes) {
        if (scene.id.empty() || !sceneIds.insert(scene.id).second || scene.start < sceneEnd ||
            scene.duration < 1 || scene.start < 0 || scene.duration > p.duration - scene.start)
            return "Marcadores de cena inválidos ou sobrepostos.";
        if (scene.artDirection && (p.schemaVersion < 11 || !validArtDirection(*scene.artDirection)))
            return "Direção de arte da cena inválida; exige schema 11.";
        scenesById.emplace(scene.id, &scene);
        sceneEnd = scene.start + scene.duration;
    }
    for (const auto &n : p.nodes) {
        if (n.id.empty() || !ids.insert(n.id).second)
            return "Identificação de objeto duplicada ou vazia.";
        nodesById.emplace(n.id, &n);
        if (n.kind == NodeKind::Group)
            parentIds.insert(n.id);
        if (p.schemaVersion < 3 && (n.kind == NodeKind::Group || !n.parentId.empty() || !n.keyframes.empty()))
            return "Grupos e keyframes exigem a versão 3 do projeto.";
        if (p.schemaVersion < 4 && !n.shotId.empty())
            return "Vínculos de shot exigem a versão 4 do projeto.";
        if (n.start < 0 || n.duration < 1 || n.start >= p.duration || n.duration > p.duration - n.start)
            return "Objeto fora da duração da sequência.";
        if (!std::isfinite(n.x) || !std::isfinite(n.y) || std::abs(n.x) > 10000 || std::abs(n.y) > 10000 ||
            !std::isfinite(n.width) || !std::isfinite(n.height) || n.width < 1 || n.height < 1 ||
            n.width > 10000 || n.height > 10000 || !std::isfinite(n.opacity) || n.opacity < 0 ||
            n.opacity > 1 || !std::isfinite(n.fontSize) || n.fontSize < 8 || n.fontSize > 300)
            return "Propriedade visual inválida.";
        if (n.color.size() != 7 || n.color[0] != '#' ||
            !std::all_of(n.color.begin() + 1, n.color.end(),
                         [](unsigned char c) { return std::isxdigit(c); }))
            return "Cor inválida; use #RRGGBB.";
        if (n.text.size() > 4000 || n.name.size() > 240 || n.source.size() > 4096)
            return "Texto ou caminho excede o limite.";
        if (n.captionWords.size() > 300)
            return "Legenda excede 300 intervalos de palavra.";
        if (!n.captionWords.empty() && p.schemaVersion < 7)
            return "Alinhamento de palavras da legenda exige schema 7.";
        if (!n.captionWords.empty() && n.role != "caption")
            return "Alinhamento de palavras só pode pertencer a uma legenda.";
        std::int64_t priorWordEnd = n.start;
        for (const auto &word : n.captionWords) {
            if (word.text.empty() || word.text.size() > 128 || word.start < priorWordEnd ||
                word.end <= word.start || word.end > n.start + n.duration)
                return "Tempos das palavras da legenda inválidos.";
            priorWordEnd = word.end;
        }
        const bool wordAnimationCustomized = n.wordAnimationPreset != "highlight" ||
                                             n.wordAnimationDurationFrames != 8 ||
                                             std::abs(n.wordAnimationStartScale - 0.82) > 1e-9 ||
                                             std::abs(n.wordAnimationTravelY - 12) > 1e-9;
        if (n.wordAnimationPreset != "highlight" && n.wordAnimationPreset != "none" &&
            n.wordAnimationPreset != "pop" && n.wordAnimationPreset != "rise" &&
            n.wordAnimationPreset != "fade" && n.wordAnimationPreset != "pop-rise")
            return "Preset de animação por palavra desconhecido.";
        if (n.wordAnimationDurationFrames < 1 || n.wordAnimationDurationFrames > 30 ||
            !std::isfinite(n.wordAnimationStartScale) || n.wordAnimationStartScale < 0.5 ||
            n.wordAnimationStartScale > 1.5 || !std::isfinite(n.wordAnimationTravelY) ||
            std::abs(n.wordAnimationTravelY) > 80)
            return "Parâmetros de animação por palavra fora dos limites.";
        if (wordAnimationCustomized &&
            (p.schemaVersion < 12 || n.role != "caption" ||
             (n.wordAnimationPreset != "none" && n.captionWords.empty())))
            return "Animação por palavra exige uma legenda alinhada e schema 12.";
        if (n.kind == NodeKind::Image && n.source.empty())
            return "Imagem sem arquivo de origem.";
        if (n.kind == NodeKind::Svg && n.source.empty())
            return "Vetor SVG sem arquivo de origem.";
        if (n.kind == NodeKind::Video && n.source.empty())
            return "Vídeo sem arquivo de origem.";
        const bool hasVideoSettings = n.videoSourceInSeconds != 0 || n.videoPlaybackRate != 1 ||
                                      n.videoCropX != 0 || n.videoCropY != 0 || n.videoCropWidth != 1 ||
                                      n.videoCropHeight != 1;
        if (n.kind == NodeKind::Video &&
            (!std::isfinite(n.videoSourceInSeconds) || n.videoSourceInSeconds < 0 ||
             n.videoSourceInSeconds > 3600 || !std::isfinite(n.videoPlaybackRate) ||
             n.videoPlaybackRate < .25 || n.videoPlaybackRate > 4 || !std::isfinite(n.videoCropX) ||
             !std::isfinite(n.videoCropY) || !std::isfinite(n.videoCropWidth) ||
             !std::isfinite(n.videoCropHeight) || n.videoCropX < 0 || n.videoCropY < 0 ||
             n.videoCropWidth <= 0 || n.videoCropHeight <= 0 || n.videoCropX + n.videoCropWidth > 1 ||
             n.videoCropY + n.videoCropHeight > 1))
            return "Trim, velocidade ou recorte de vídeo fora dos limites.";
        if ((n.kind != NodeKind::Video && hasVideoSettings) ||
            (p.schemaVersion < 10 && n.kind == NodeKind::Video && hasVideoSettings))
            return "Controles avançados de vídeo exigem um nó de vídeo e schema 10.";
        if (n.kind == NodeKind::Native && !libraryItem(n.nativeId))
            return "Objeto nativo desconhecido.";
        if (!n.sceneId.empty() && !sceneIds.contains(n.sceneId))
            return "Objeto aponta para cena inexistente.";
        if (!n.sceneId.empty()) {
            const auto scene = scenesById.find(n.sceneId);
            if (scene != scenesById.end() &&
                (n.start < scene->second->start ||
                 n.start + n.duration > scene->second->start + scene->second->duration))
                return "Objeto ultrapassa o intervalo da sua cena.";
        }
        if (!supportedMotion(n.motion) || !supportedPose(n.pose))
            return "Movimento ou pose não suportado.";
        if (!std::isfinite(n.rotation) || std::abs(n.rotation) > 3600 || !std::isfinite(n.scale) ||
            n.scale < 0.01 || n.scale > 10 || !std::isfinite(n.travelX) || std::abs(n.travelX) > 10000 ||
            !std::isfinite(n.travelY) || std::abs(n.travelY) > 10000 || !std::isfinite(n.spinDegrees) ||
            std::abs(n.spinDegrees) > 3600 || n.motionFrames < 1 || n.motionFrames > 300 ||
            n.exitFrames < 0 || n.exitFrames > 300)
            return "Parâmetros de animação inválidos.";
        if (n.secondaryColor.size() != 7 || n.secondaryColor[0] != '#' ||
            !std::all_of(n.secondaryColor.begin() + 1, n.secondaryColor.end(),
                         [](unsigned char c) { return std::isxdigit(c); }))
            return "Cor secundária inválida.";
        if (n.role != "" && n.role != "caption" && n.role != "background" && n.role != "header" &&
            !premiumSlotRole(n.role))
            return "Papel visual desconhecido.";
    }
    std::unordered_map<std::string, const ShotMarker *> shotsById;
    std::unordered_map<std::string, std::vector<const ShotMarker *>> shotsByScene;
    for (const auto &shot : p.shots) {
        if (shot.id.empty() || !shotIds.insert(shot.id).second || !sceneIds.contains(shot.sceneId) ||
            shot.start < 0 || shot.duration < 1 || shot.start >= p.duration ||
            shot.duration > p.duration - shot.start || shot.name.size() > 120 ||
            (shot.transitionIn != "cut" && shot.transitionIn != "fade" && shot.transitionIn != "dissolve" &&
             shot.transitionIn != "slide" && shot.transitionIn != "zoom"))
            return "Plano visual inválido ou tipo de transição não suportado.";
        if ((shot.transitionIn == "cut" && shot.transitionDurationFrames != 0) ||
            (shot.transitionIn != "cut" &&
             (shot.transitionDurationFrames < 2 || shot.transitionDurationFrames > 300 ||
              shot.transitionDurationFrames > shot.duration)) ||
            (shot.transitionIn == "slide" && shot.transitionDirection != "left" &&
             shot.transitionDirection != "right" && shot.transitionDirection != "up" &&
             shot.transitionDirection != "down") ||
            (shot.transitionIn != "slide" && shot.transitionDirection != "left"))
            return "Duração ou direção da transição inválida.";
        if (shot.artDirection && (p.schemaVersion < 11 || !validArtDirection(*shot.artDirection)))
            return "Direção de arte do shot inválida; exige schema 11.";
        const auto scene = scenesById.find(shot.sceneId);
        if (scene == scenesById.end() || shot.start < scene->second->start ||
            shot.start + shot.duration > scene->second->start + scene->second->duration)
            return "Plano visual fica fora da cena narrativa.";
        shotsById.emplace(shot.id, &shot);
        shotsByScene[shot.sceneId].push_back(&shot);
    }
    for (auto &[sceneId, sceneShots] : shotsByScene) {
        std::sort(sceneShots.begin(), sceneShots.end(),
                  [](const auto *a, const auto *b) { return a->start < b->start; });
        const auto scene = scenesById.find(sceneId);
        if (scene == scenesById.end() || sceneShots.front()->start != scene->second->start ||
            sceneShots.back()->start + sceneShots.back()->duration !=
                scene->second->start + scene->second->duration)
            return "Planos visuais precisam cobrir toda a cena narrativa.";
        if (sceneShots.front()->transitionIn != "cut")
            return "O primeiro plano da cena não pode ter transição de entrada.";
        for (std::size_t i = 1; i < sceneShots.size(); ++i)
            if (sceneShots[i - 1]->start + sceneShots[i - 1]->duration != sceneShots[i]->start)
                return "Planos visuais da mesma cena têm lacuna ou sobreposição.";
    }
    for (const auto &node : p.nodes)
        if (!node.shotId.empty()) {
            const auto shot = shotsById.find(node.shotId);
            if (shot == shotsById.end() || shot->second->sceneId != node.sceneId ||
                node.start < shot->second->start ||
                node.start + node.duration > shot->second->start + shot->second->duration)
                return "Elemento '" + node.id + "' aponta para o shot '" + node.shotId +
                       "' ou excede seu intervalo (elemento " + std::to_string(node.start) + "–" +
                       std::to_string(node.start + node.duration) + ", shot " +
                       (shot == shotsById.end() ? std::string("inexistente")
                                                : std::to_string(shot->second->start) + "–" +
                                                      std::to_string(shot->second->start + shot->second->duration)) +
                       ").";
        }
    std::unordered_set<std::string> soundTracksByBeat;
    for (const auto &beat : p.visualBeats) {
        const auto shot = shotsById.find(beat.shotId);
        if (beat.id.empty() || !beatIds.insert(beat.id).second || shot == shotsById.end() || beat.frame < 0 ||
            beat.frame >= p.duration || beat.durationFrames < 0 || beat.frame < shot->second->start ||
            beat.durationFrames > shot->second->start + shot->second->duration - beat.frame - 1 ||
            (beat.action != "show" && beat.action != "hide" && beat.action != "transform" &&
             beat.action != "camera" && beat.action != "sound"))
            return "Visual Beat inválido ou fora do shot.";
        const bool cameraBeat = beat.action == "camera";
        const bool soundBeat = beat.action == "sound";
        if (soundBeat) {
            if (p.schemaVersion < 9 || beat.targetId.empty() || beat.durationFrames != 0 || beat.x || beat.y ||
                beat.rotation || beat.scale || beat.opacity || beat.zoom ||
                !soundTracksByBeat.insert(beat.targetId).second)
                return "Visual Beat sonoro inválido ou exige projeto schema 9.";
            const auto track = std::find_if(p.audioTracks.begin(), p.audioTracks.end(), [&](const auto &candidate) {
                return candidate.id == beat.targetId;
            });
            if (track == p.audioTracks.end() || track->start != beat.frame)
                return "Visual Beat sonoro precisa de uma pista no mesmo quadro.";
            continue;
        }
        if (cameraBeat != beat.targetId.empty())
            return "Visual Beat precisa de alvo visual, exceto quando controla a câmera.";
        if (!cameraBeat) {
            const auto node = nodesById.find(beat.targetId);
            if (node == nodesById.end() || node->second->shotId != beat.shotId || beat.action == "camera")
                return "Visual Beat aponta para um elemento inexistente ou de outro shot.";
        }
        auto validNumber = [](const std::optional<double> &value, double minimum, double maximum) {
            return !value || (std::isfinite(*value) && *value >= minimum && *value <= maximum);
        };
        if (!validNumber(beat.x, -10000, 10000) || !validNumber(beat.y, -10000, 10000) ||
            !validNumber(beat.rotation, -3600, 3600) || !validNumber(beat.scale, .01, 10) ||
            !validNumber(beat.opacity, 0, 1) || !validNumber(beat.zoom, .1, 10) ||
            (cameraBeat && (beat.scale || beat.opacity)) || (!cameraBeat && beat.zoom) ||
            ((beat.action == "show" || beat.action == "hide") &&
             (beat.x || beat.y || beat.rotation || beat.scale || beat.opacity || beat.zoom)) ||
            (beat.action == "transform" &&
             !(beat.x || beat.y || beat.rotation || beat.scale || beat.opacity)) ||
            (cameraBeat && !(beat.x || beat.y || beat.rotation || beat.zoom)))
            return "Propriedades do Visual Beat inválidas.";
    }
    std::unordered_map<std::string, unsigned char> hierarchyState;
    hierarchyState.reserve(p.nodes.size());
    for (const auto &n : p.nodes) {
        if (!n.parentId.empty() && (n.parentId == n.id || !parentIds.contains(n.parentId)))
            return "Grupo pai inexistente ou inválido.";
        if (!n.parentId.empty()) {
            const auto parentNode = nodesById.find(n.parentId);
            if (parentNode == nodesById.end() || n.start < parentNode->second->start ||
                n.start + n.duration > parentNode->second->start + parentNode->second->duration)
                return "Elemento ultrapassa o intervalo do seu grupo.";
        }
        if (n.keyframes.size() > 256)
            return "Um elemento pode ter no máximo 256 keyframes.";
        std::int64_t previous = -1;
        for (const auto &key : n.keyframes) {
            if (key.frame < 0 || key.frame >= p.duration || key.frame <= previous || !std::isfinite(key.x) ||
                std::abs(key.x) > 10000 || !std::isfinite(key.y) || std::abs(key.y) > 10000 ||
                !std::isfinite(key.rotation) || std::abs(key.rotation) > 3600 || !std::isfinite(key.scale) ||
                key.scale < 0.01 || key.scale > 10 || !std::isfinite(key.opacity) || key.opacity < 0 ||
                key.opacity > 1 || !supportedEasing(key.easing))
                return "Keyframes do elemento inválidos ou fora de ordem.";
            previous = key.frame;
        }
    }
    // Resolve every parent chain once. This detects cycles without recursive calls or
    // quadratic repeated walks in deeply nested documents.
    for (const auto &node : p.nodes) {
        auto found = hierarchyState.find(node.id);
        if (found != hierarchyState.end() && found->second == 2)
            continue;
        std::vector<const Node *> path;
        const Node *current = &node;
        while (current) {
            auto state = hierarchyState.find(current->id);
            if (state != hierarchyState.end()) {
                if (state->second == 1)
                    return "Ciclo encontrado na hierarquia de grupos.";
                break;
            }
            hierarchyState.emplace(current->id, 1);
            path.push_back(current);
            if (current->parentId.empty())
                break;
            auto parent = nodesById.find(current->parentId);
            if (parent == nodesById.end())
                return "Grupo pai inexistente.";
            current = parent->second;
        }
        for (const auto *part : path)
            hierarchyState[part->id] = 2;
    }
    if (p.schemaVersion < 3 && !p.camera.empty())
        return "Câmera animada exige a versão 3 do projeto.";
    if (p.camera.size() > 20000)
        return "A câmera pode ter no máximo 20000 keyframes.";
    std::int64_t previousCamera = -1;
    for (const auto &key : p.camera) {
        if (key.frame < 0 || key.frame >= p.duration || key.frame <= previousCamera ||
            !std::isfinite(key.x) || std::abs(key.x) > 10000 || !std::isfinite(key.y) ||
            std::abs(key.y) > 10000 || !std::isfinite(key.zoom) || key.zoom < 0.1 || key.zoom > 10 ||
            !std::isfinite(key.rotation) || std::abs(key.rotation) > 360 || !supportedEasing(key.easing))
            return "Keyframes da câmera inválidos ou fora de ordem.";
        if (!key.shotId.empty()) {
            const auto shot = shotsById.find(key.shotId);
            if (shot == shotsById.end() || key.frame < std::max<std::int64_t>(0, shot->second->start - 1) ||
                key.frame >= shot->second->start + shot->second->duration)
                return "Keyframe da câmera aponta para um shot inexistente ou fora do intervalo.";
        }
        previousCamera = key.frame;
    }
    std::unordered_set<std::string> audioTrackIds;
    for (const auto &track : p.audioTracks) {
        if (track.id.empty() || !audioTrackIds.insert(track.id).second || track.name.empty() ||
            track.name.size() > 120 || track.source.empty() || track.source.size() > 4096 || track.start < 0 ||
            track.duration < 1 || track.start >= p.duration || track.duration > p.duration - track.start ||
            !std::isfinite(track.sourceInSeconds) || track.sourceInSeconds < 0 || track.sourceInSeconds > 3600 ||
            !std::isfinite(track.playbackRate) || track.playbackRate < .25 || track.playbackRate > 4 ||
            track.fadeInFrames < 0 || track.fadeOutFrames < 0 ||
            track.fadeInFrames + track.fadeOutFrames > track.duration || track.fadeInFrames > 1800 ||
            track.fadeOutFrames > 1800 ||
            !std::isfinite(track.gain) || track.gain < 0 || track.gain > 4 ||
            !std::isfinite(track.duckAmount) || track.duckAmount < 0 || track.duckAmount > 1)
            return "Pista de áudio inválida ou fora dos limites.";
        if (!track.linkedVideoNodeId.empty()) {
            const auto *linkedNode = findNode(p, track.linkedVideoNodeId);
            if (!linkedNode || linkedNode->kind != NodeKind::Video || linkedNode->source != track.source ||
                linkedNode->start != track.start || linkedNode->duration != track.duration ||
                linkedNode->videoSourceInSeconds != track.sourceInSeconds ||
                linkedNode->videoPlaybackRate != track.playbackRate)
                return "Pista de áudio vinculada deve acompanhar a camada de vídeo.";
        }
        if (track.gainAutomation.size() > 128)
            return "Automação de ganho excede 128 pontos por pista.";
        std::int64_t previousPointFrame = -1;
        for (const auto &point : track.gainAutomation) {
            if (point.frame <= previousPointFrame || point.frame < 0 || point.frame >= track.duration ||
                !std::isfinite(point.value) || point.value < 0 || point.value > 4 ||
                !supportedEasing(point.easing))
                return "Ponto de automação de ganho inválido.";
            previousPointFrame = point.frame;
        }
    }
    if (std::any_of(p.audioTracks.begin(), p.audioTracks.end(), [](const auto &track) {
            return !track.muted && track.duckUnderVoice;
        }) && p.audioSource.empty() && p.voiceDuckingSource.empty())
        return "Ducking de áudio exige uma faixa de voz para referência.";
    if (p.audioSource.size() > 4096)
        return "Caminho de áudio inválido.";
    if (p.voiceDuckingSource.size() > 4096)
        return "Caminho da voz para ducking inválido.";
    return {};
} catch (const std::bad_alloc &) {
    return "Memória insuficiente para validar o projeto.";
}
Project makeEmpty() {
    Project p;
    p.id = "prototype-project";
    p.name = "Nova composição";
    return p;
}
Project makeDemo() {
    auto p = makeEmpty();
    p.name = "Além do visível";
    Node accent;
    accent.id = "accent";
    accent.name = "01 · Linha de luz";
    accent.kind = NodeKind::Shape;
    accent.x = 110;
    accent.y = 214;
    accent.width = 84;
    accent.height = 6;
    accent.color = "#b4f078";
    Node title;
    title.id = "title";
    title.name = "02 · Título principal";
    title.text = "Ideias ganham\nmovimento.";
    title.x = 105;
    title.y = 250;
    title.width = 1000;
    title.height = 250;
    title.fontSize = 84;
    title.start = 15;
    title.duration = 345;
    Node subtitle;
    subtitle.id = "subtitle";
    subtitle.name = "03 · Assinatura";
    subtitle.text = "UM NOVO ESPAÇO PARA CRIAR";
    subtitle.x = 110;
    subtitle.y = 548;
    subtitle.width = 1000;
    subtitle.height = 50;
    subtitle.fontSize = 22;
    subtitle.color = "#a1aba9";
    subtitle.start = 45;
    subtitle.duration = 315;
    Node board;
    board.id = "composition-board";
    board.name = "Grupo · Composição";
    board.kind = NodeKind::Group;
    board.width = p.width;
    board.height = p.height;
    board.duration = p.duration;
    accent.parentId = board.id;
    title.parentId = board.id;
    subtitle.parentId = board.id;
    p.nodes = {accent, title, subtitle, board};
    p.camera = {{0, 640, 360, 1.10, 0, "linear", {}},
                {72, 640, 360, 1, 0, "linear", {}},
                {220, 760, 360, 1.08, 0, "linear", {}},
                {359, 640, 360, 1, 0, "linear", {}}};
    return p;
}
const Node *findNode(const Project &p, const std::string &id) {
    auto it = std::find_if(p.nodes.begin(), p.nodes.end(), [&](const Node &n) { return n.id == id; });
    return it == p.nodes.end() ? nullptr : &*it;
}
double evaluateGainAutomation(const AudioTrack &track, std::int64_t localFrame) {
    if (track.gainAutomation.empty())
        return 1;
    if (localFrame <= track.gainAutomation.front().frame)
        return track.gainAutomation.front().value;
    for (std::size_t i = 1; i < track.gainAutomation.size(); ++i) {
        const auto &previous = track.gainAutomation[i - 1];
        const auto &current = track.gainAutomation[i];
        if (localFrame > current.frame)
            continue;
        double progress = double(localFrame - previous.frame) / double(current.frame - previous.frame);
        if (current.easing == "ease-in")
            progress = progress * progress * progress;
        else if (current.easing == "ease-out")
            progress = 1 - std::pow(1 - progress, 3);
        else if (current.easing == "ease-in-out")
            progress = progress * progress * (3 - 2 * progress);
        return previous.value + (current.value - previous.value) * progress;
    }
    return track.gainAutomation.back().value;
}
bool active(const Node &n, std::int64_t frame) {
    return frame >= n.start && frame < n.start + n.duration;
}
VisualState evaluate(const Node &n, std::int64_t frame) {
    if (!active(n, frame))
        return {n.x, n.y, 0};
    const auto t = n.animated ? std::clamp(double(frame - n.start) / n.motionFrames, 0., 1.) : 1.;
    const double ease = 1 - std::pow(1 - t, 3);
    const double life =
        std::clamp(double(frame - n.start) / std::max<std::int64_t>(1, n.duration - 1), 0., 1.);
    double x = n.x + n.travelX * life, y = n.y + n.travelY * life;
    if (n.animated && n.motion == "rise")
        y += 28 * (1 - ease);
    if (n.animated && n.motion == "slide-left")
        x += 70 * (1 - ease);
    if (n.animated && n.motion == "slide-right")
        x -= 70 * (1 - ease);
    double opacity = n.opacity * ((!n.animated || n.motion == "none") ? 1 : ease);
    if (n.exitFrames > 0)
        opacity *= std::clamp(double(n.start + n.duration - 1 - frame) / n.exitFrames, 0., 1.);
    const auto scale = n.scale * ((n.animated && n.motion == "pop") ? 0.82 + 0.18 * ease : 1.);
    VisualState state{x, y, opacity, n.rotation + n.spinDegrees * life, scale};
    if (n.keyframes.empty())
        return state;
    const auto upper =
        std::upper_bound(n.keyframes.begin(), n.keyframes.end(), frame,
                         [](std::int64_t f, const TransformKeyframe &key) { return f < key.frame; });
    // A future key must not leak backward in time. Before the first key, keep
    // the node's own animated/base state; the first key takes effect on its frame.
    if (upper == n.keyframes.begin())
        return state;
    if (upper == n.keyframes.end()) {
        const auto &key = n.keyframes.back();
        return {key.x, key.y, key.opacity, key.rotation, key.scale};
    }
    const auto &a = *(upper - 1);
    const auto &b = *upper;
    const double mixAmount = eased(double(frame - a.frame) / double(b.frame - a.frame), b.easing);
    const auto mix = [mixAmount](double from, double to) { return from + (to - from) * mixAmount; };
    return {mix(a.x, b.x), mix(a.y, b.y), mix(a.opacity, b.opacity), mix(a.rotation, b.rotation),
            mix(a.scale, b.scale)};
}
CameraState evaluateCamera(const Project &p, std::int64_t frame) {
    CameraState state{p.width / 2.0, p.height / 2.0, 1, 0};
    if (p.camera.empty())
        return state;
    const auto upper =
        std::upper_bound(p.camera.begin(), p.camera.end(), frame,
                         [](std::int64_t f, const CameraKeyframe &key) { return f < key.frame; });
    if (upper == p.camera.begin()) {
        const auto &k = *upper;
        return {k.x, k.y, k.zoom, k.rotation};
    }
    if (upper == p.camera.end()) {
        const auto &k = p.camera.back();
        return {k.x, k.y, k.zoom, k.rotation};
    }
    const auto &a = *(upper - 1);
    const auto &b = *upper;
    const double t = eased(double(frame - a.frame) / double(b.frame - a.frame), b.easing);
    const auto mix = [t](double from, double to) { return from + (to - from) * t; };
    return {mix(a.x, b.x), mix(a.y, b.y), mix(a.zoom, b.zoom), mix(a.rotation, b.rotation)};
}
} // namespace studio
