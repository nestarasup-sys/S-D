#include <algorithm>
#include <cctype>
#include <cmath>
#include <studio/core/library.hpp>
#include <studio/core/project.hpp>
#include <unordered_map>
#include <unordered_set>

namespace studio {
double FrameRate::value() const {
    return double(numerator) / denominator;
}
bool supportedEasing(const std::string &name) {
    return name == "linear" || name == "ease-in" || name == "ease-out" ||
           name == "ease-in-out";
}
namespace {
double eased(double t, const std::string &curve) {
    if (curve == "ease-in")
        return t * t * t;
    if (curve == "ease-out")
        return 1 - std::pow(1 - t, 3);
    if (curve == "ease-in-out")
        return t * t * (3 - 2 * t);
    return t; // Preserve original interpolation when easing was absent.
}
}
std::string validate(const Project &p) {
    if (p.schemaVersion != 2 && p.schemaVersion != 3 && p.schemaVersion != 4)
        return "Versão de projeto não suportada.";
    if (p.id.empty() || p.name.empty())
        return "Projeto sem identificação ou nome.";
    if (p.width < 64 || p.height < 64 || p.width > 3840 || p.height > 2160 || p.width % 2 || p.height % 2)
        return "Resolução inválida: use dimensões pares entre 64 e 3840 × 2160.";
    if (p.fps.numerator <= 0 || p.fps.numerator > 120000 || p.fps.denominator <= 0 ||
        p.fps.denominator > 10000 || p.fps.value() < 1 || p.fps.value() > 120)
        return "Taxa de quadros inválida.";
    if (p.duration < 1 || p.duration > 18000 || p.nodes.size() > 200 || p.shots.size() > 256 ||
        p.visualBeats.size() > 4096)
        return "Projeto excede os limites deste protótipo.";
    std::unordered_set<std::string> ids;
    std::unordered_set<std::string> parentIds;
    std::unordered_set<std::string> sceneIds;
    std::unordered_set<std::string> shotIds;
    std::unordered_set<std::string> beatIds;
    std::int64_t sceneEnd = 0;
    for (const auto &scene : p.scenes) {
        if (scene.id.empty() || !sceneIds.insert(scene.id).second || scene.start < sceneEnd ||
            scene.duration < 1 || scene.start < 0 || scene.duration > p.duration - scene.start)
            return "Marcadores de cena inválidos ou sobrepostos.";
        sceneEnd = scene.start + scene.duration;
    }
    for (const auto &n : p.nodes) {
        if (n.id.empty() || !ids.insert(n.id).second)
            return "Identificação de objeto duplicada ou vazia.";
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
        if (n.kind == NodeKind::Image && n.source.empty())
            return "Imagem sem arquivo de origem.";
        if (n.kind == NodeKind::Svg && n.source.empty())
            return "Vetor SVG sem arquivo de origem.";
        if (n.kind == NodeKind::Native && !libraryItem(n.nativeId))
            return "Objeto nativo desconhecido.";
        if (!n.sceneId.empty() && !sceneIds.contains(n.sceneId))
            return "Objeto aponta para cena inexistente.";
        if (!n.sceneId.empty())
            for (const auto &scene : p.scenes)
                if (scene.id == n.sceneId &&
                    (n.start < scene.start || n.start + n.duration > scene.start + scene.duration))
                    return "Objeto ultrapassa o intervalo da sua cena.";
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
        if (n.role != "" && n.role != "caption" && n.role != "background" && n.role != "header")
            return "Papel visual desconhecido.";
    }
    std::unordered_map<std::string, const ShotMarker *> shotsById;
    std::unordered_map<std::string, std::vector<const ShotMarker *>> shotsByScene;
    for (const auto &shot : p.shots) {
        if (shot.id.empty() || !shotIds.insert(shot.id).second || !sceneIds.contains(shot.sceneId) ||
            shot.start < 0 || shot.duration < 1 || shot.start >= p.duration ||
            shot.duration > p.duration - shot.start || shot.name.size() > 120 ||
            shot.transitionIn != "cut")
            return "Plano visual inválido; esta versão exige cortes secos.";
        const auto scene = std::find_if(p.scenes.begin(), p.scenes.end(), [&](const SceneMarker &candidate) {
            return candidate.id == shot.sceneId;
        });
        if (scene == p.scenes.end() || shot.start < scene->start ||
            shot.start + shot.duration > scene->start + scene->duration)
            return "Plano visual fica fora da cena narrativa.";
        shotsById.emplace(shot.id, &shot);
        shotsByScene[shot.sceneId].push_back(&shot);
    }
    for (auto &[sceneId, sceneShots] : shotsByScene) {
        std::sort(sceneShots.begin(), sceneShots.end(), [](const auto *a, const auto *b) {
            return a->start < b->start;
        });
        const auto scene = std::find_if(p.scenes.begin(), p.scenes.end(), [&](const SceneMarker &candidate) {
            return candidate.id == sceneId;
        });
        if (sceneShots.front()->start != scene->start ||
            sceneShots.back()->start + sceneShots.back()->duration != scene->start + scene->duration)
            return "Planos visuais precisam cobrir toda a cena narrativa.";
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
                return "Elemento aponta para um shot inexistente ou excede seu intervalo.";
        }
    for (const auto &beat : p.visualBeats) {
        const auto shot = shotsById.find(beat.shotId);
        if (beat.id.empty() || !beatIds.insert(beat.id).second || shot == shotsById.end() || beat.frame < 0 ||
            beat.frame >= p.duration || beat.durationFrames < 0 ||
            beat.frame < shot->second->start ||
            beat.durationFrames > shot->second->start + shot->second->duration - beat.frame - 1 ||
            (beat.action != "show" && beat.action != "hide" && beat.action != "transform" &&
             beat.action != "camera"))
            return "Visual Beat inválido ou fora do shot.";
        const bool cameraBeat = beat.action == "camera";
        if (cameraBeat != beat.targetId.empty())
            return "Visual Beat precisa de alvo visual, exceto quando controla a câmera.";
        if (!cameraBeat) {
            const auto node = std::find_if(p.nodes.begin(), p.nodes.end(), [&](const Node &candidate) {
                return candidate.id == beat.targetId;
            });
            if (node == p.nodes.end() || node->shotId != beat.shotId ||
                beat.action == "camera")
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
    for (const auto &n : p.nodes) {
        if (!n.parentId.empty() && (n.parentId == n.id || !parentIds.contains(n.parentId)))
            return "Grupo pai inexistente ou inválido.";
        std::unordered_set<std::string> seen{n.id};
        auto parent = n.parentId;
        while (!parent.empty()) {
            if (!seen.insert(parent).second)
                return "Ciclo encontrado na hierarquia de grupos.";
            auto it = std::find_if(p.nodes.begin(), p.nodes.end(),
                                   [&](const Node &candidate) { return candidate.id == parent; });
            if (it == p.nodes.end())
                return "Grupo pai inexistente.";
            parent = it->parentId;
        }
        if (!n.parentId.empty()) {
            const auto parentNode = std::find_if(p.nodes.begin(), p.nodes.end(), [&](const Node &candidate) {
                return candidate.id == n.parentId;
            });
            if (n.start < parentNode->start ||
                n.start + n.duration > parentNode->start + parentNode->duration)
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
    if (p.schemaVersion < 3 && !p.camera.empty())
        return "Câmera animada exige a versão 3 do projeto.";
    if (p.camera.size() > 256)
        return "A câmera pode ter no máximo 256 keyframes.";
    std::int64_t previousCamera = -1;
    for (const auto &key : p.camera) {
        if (key.frame < 0 || key.frame >= p.duration || key.frame <= previousCamera ||
            !std::isfinite(key.x) || std::abs(key.x) > 10000 || !std::isfinite(key.y) ||
            std::abs(key.y) > 10000 || !std::isfinite(key.zoom) || key.zoom < 0.1 || key.zoom > 10 ||
            !std::isfinite(key.rotation) || std::abs(key.rotation) > 360 ||
            !supportedEasing(key.easing))
            return "Keyframes da câmera inválidos ou fora de ordem.";
        previousCamera = key.frame;
    }
    if (p.audioSource.size() > 4096)
        return "Caminho de áudio inválido.";
    return {};
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
    p.camera = {
        {0, 640, 360, 1.10, 0}, {72, 640, 360, 1, 0}, {220, 760, 360, 1.08, 0}, {359, 640, 360, 1, 0}};
    return p;
}
const Node *findNode(const Project &p, const std::string &id) {
    auto it = std::find_if(p.nodes.begin(), p.nodes.end(), [&](const Node &n) { return n.id == id; });
    return it == p.nodes.end() ? nullptr : &*it;
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
    if (upper == n.keyframes.begin()) {
        const auto &key = *upper;
        return {key.x, key.y, key.opacity, key.rotation, key.scale};
    }
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
