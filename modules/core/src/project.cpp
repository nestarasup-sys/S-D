#include <algorithm>
#include <cctype>
#include <cmath>
#include <studio/core/project.hpp>
#include <unordered_set>

namespace studio {
double FrameRate::value() const {
    return double(numerator) / denominator;
}
std::string validate(const Project &p) {
    if (p.schemaVersion != 1)
        return "Versão de projeto não suportada.";
    if (p.id.empty() || p.name.empty())
        return "Projeto sem identificação ou nome.";
    if (p.width < 64 || p.height < 64 || p.width > 3840 || p.height > 2160 || p.width % 2 || p.height % 2)
        return "Resolução inválida: use dimensões pares entre 64 e 3840 × 2160.";
    if (p.fps.numerator <= 0 || p.fps.numerator > 120000 || p.fps.denominator <= 0 ||
        p.fps.denominator > 10000 || p.fps.value() < 1 || p.fps.value() > 120)
        return "Taxa de quadros inválida.";
    if (p.duration < 1 || p.duration > 18000 || p.nodes.size() > 200)
        return "Projeto excede os limites deste protótipo.";
    std::unordered_set<std::string> ids;
    for (const auto &n : p.nodes) {
        if (n.id.empty() || !ids.insert(n.id).second)
            return "Identificação de objeto duplicada ou vazia.";
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
    }
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
    p.nodes = {accent, title, subtitle};
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
    const auto t = n.animated ? std::clamp(double(frame - n.start) / 24., 0., 1.) : 1.;
    const double ease = 1 - std::pow(1 - t, 3);
    return {n.x, n.y + 28 * (1 - ease), n.opacity * ease};
}
} // namespace studio
