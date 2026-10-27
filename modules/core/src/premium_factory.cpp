#include <studio/core/library.hpp>

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>

namespace studio {
namespace {
enum class Motion { FadeRise, SlideLeft, SlideRight, PopScale, SoftFloat, LineDraw };

class PremiumFactory {
  public:
    PremiumFactory(int width, int height, std::string rootId, std::string rootName)
        : width_(std::clamp(width, 1, 30000)), height_(std::clamp(height, 1, 30000)) {
        nodes_.reserve(24);
        add(std::move(rootId), std::move(rootName), NodeKind::Group, 0, 0, width_, height_, "#000000", {});
    }

    Node &panel(std::string id, std::string name, std::string parent, double x, double y, double w, double h,
                std::string paletteRole) {
        return add(std::move(id), std::move(name), NodeKind::Shape, x, y, w, h, "#14212a",
                   std::move(parent), "palette:" + paletteRole);
    }
    Node &rule(std::string id, std::string parent, double x, double y, double w, double h,
               std::string paletteRole = "accent") {
        return add(std::move(id), "Filete editorial", NodeKind::Shape, x, y, w, h, "#9bd0cc",
                   std::move(parent), "palette:" + paletteRole);
    }
    Node &text(std::string id, std::string name, std::string parent, double x, double y, double w, double h,
               std::string value, double size, std::string slotRole, std::string color = "#eff5f2") {
        return add(std::move(id), std::move(name), NodeKind::Text, x, y, w, h, std::move(color),
                   std::move(parent), std::move(slotRole), std::move(value), size);
    }
    void animate(Node &node, Motion motion, int delay = 0, int frames = 24) {
        frames = std::clamp(frames, 4, 120);
        delay = std::clamp(delay, 0, 240);
        const double x = node.x, y = node.y, scale = node.scale;
        double fromX = x, fromY = y, fromScale = scale;
        if (motion == Motion::FadeRise) fromY += std::max(8.0, height_ * .018);
        if (motion == Motion::SlideLeft) fromX -= std::max(16.0, width_ * .035);
        if (motion == Motion::SlideRight) fromX += std::max(16.0, width_ * .035);
        if (motion == Motion::PopScale) fromScale *= .88;
        if (motion == Motion::LineDraw) fromScale = .08;
        node.animated = false;
        node.motion = "none";
        node.opacity = 0;
        node.keyframes.clear();
        if (delay > 0)
            node.keyframes.push_back({0, fromX, fromY, node.rotation, fromScale, 0, "linear"});
        node.keyframes.push_back({delay, fromX, fromY, node.rotation, fromScale, 0, "linear"});
        node.keyframes.push_back({delay + frames, x, y, node.rotation, scale, 1, "ease-out"});
        if (motion == Motion::SoftFloat) {
            node.keyframes.push_back({delay + frames + 30, x, y - 5, node.rotation, scale, 1, "ease-in-out"});
            node.keyframes.push_back({delay + frames + 60, x, y, node.rotation, scale, 1, "ease-in-out"});
        }
    }
    std::vector<Node> finish() { return std::move(nodes_); }
    int width() const { return width_; }
    int height() const { return height_; }

  private:
    Node &add(std::string id, std::string name, NodeKind kind, double x, double y, double w, double h,
              std::string color, std::string parent, std::string role = {}, std::string value = {},
              double fontSize = 44) {
        Node node;
        node.id = std::move(id);
        node.name = std::move(name);
        node.kind = kind;
        node.x = x;
        node.y = y;
        node.width = w;
        node.height = h;
        node.color = std::move(color);
        node.parentId = std::move(parent);
        node.role = std::move(role);
        node.text = std::move(value);
        node.fontSize = fontSize;
        node.start = 0;
        node.duration = 360;
        node.animated = false;
        node.motion = "none";
        nodes_.push_back(std::move(node));
        return nodes_.back();
    }
    int width_, height_;
    std::vector<Node> nodes_;
};

double scaleFor(int width, int height) {
    return std::clamp(std::min(width / 1280.0, height / 720.0), .72, 1.6);
}

std::vector<Node> dramaticQuote(int width, int height) {
    PremiumFactory f(width, height, "dramatic-quote-root", "Dramatic Quote · composição");
    const bool portrait = height > width;
    const double s = scaleFor(width, height), margin = width * .105;
    const double quoteX = margin + 44 * s;
    const double quoteY = portrait ? height * .29 : height * .28;
    const double quoteW = width - quoteX - margin;
    const double quoteH = portrait ? height * .28 : height * .31;
    auto &rule = f.rule("quote-rule", "dramatic-quote-root", margin, quoteY + 8 * s, 4 * s,
                        portrait ? height * .33 : height * .39);
    f.animate(rule, Motion::LineDraw, 0, 26);
    auto &mark = f.text("quote-mark", "Aspas editoriais", "dramatic-quote-root", quoteX, quoteY - 62 * s,
                        100 * s, 78 * s, "“", 106 * s, {}, "#84bfc0");
    f.animate(mark, Motion::FadeRise, 0, 22);
    auto &quote = f.text("quote-copy", "Frase principal", "dramatic-quote-root", quoteX, quoteY + 18 * s,
                         quoteW, quoteH, "UMA FRASE\nQUE FICA.", (portrait ? 56 : 68) * s,
                         "premium:quote:text");
    f.animate(quote, Motion::FadeRise, 6, 32);
    auto &attribution = f.text("quote-attribution", "Autoria ou contexto", "dramatic-quote-root", quoteX,
                               quoteY + quoteH + 30 * s, quoteW * .82, 36 * s,
                               "NOME OU CONTEXTO EDITORIAL", 19 * s,
                               "premium:quote:attribution", "#aebfc4");
    f.animate(attribution, Motion::SlideLeft, 24, 24);
    auto &source = f.text("quote-source", "Fonte opcional", "dramatic-quote-root", quoteX,
                          quoteY + quoteH + 68 * s, quoteW * .9, 30 * s,
                          "FONTE VERIFICADA · ANO", 14 * s,
                          "premium:quote:source", "#91a6ac");
    f.animate(source, Motion::FadeRise, 32, 20);
    return f.finish();
}

std::vector<Node> lowerThird(int width, int height) {
    PremiumFactory f(width, height, "lower-third-root", "Lower Third Editorial · composição");
    const bool portrait = height > width;
    const double s = scaleFor(width, height), margin = width * .065;
    const double boxW = portrait ? width * .88 : width * .60;
    const double boxH = portrait ? 190 * s : 168 * s;
    // Keep the identifier above the default caption safe area in both aspect ratios.
    const double x = margin, y = height - boxH - height * .20;
    auto &surface = f.panel("lower-third-surface", "Superfície editorial", "lower-third-root", x, y,
                            boxW, boxH, "surface");
    surface.opacity = .96;
    auto &accent = f.rule("lower-third-accent", "lower-third-root", x, y, 5 * s, boxH, "accent");
    auto &eyebrow = f.text("lower-third-kicker", "Identificação editorial", "lower-third-root",
                           x + 24 * s, y + 17 * s, boxW - 45 * s, 23 * s,
                           "REPORTAGEM  /  CONTEXTO", 13 * s, {}, "#8fbfc0");
    auto &name = f.text("lower-third-name", "Nome ou título", "lower-third-root", x + 24 * s,
                        y + 47 * s, boxW - 46 * s, 48 * s, "TÍTULO EDITORIAL",
                        (portrait ? 30 : 34) * s, "premium:lower-third:title");
    auto &subtitle = f.text("lower-third-subtitle", "Descrição secundária", "lower-third-root",
                            x + 24 * s, y + 100 * s, boxW - 46 * s, 28 * s,
                            "Identificação ou contexto em uma linha.", 17 * s,
                            "premium:lower-third:subtitle", "#b0c0c5");
    auto &source = f.text("lower-third-source", "Fonte ou localização", "lower-third-root",
                          x + 24 * s, y + 133 * s, boxW - 46 * s, 21 * s,
                          "FONTE · LOCAL · DATA", 12 * s,
                          "premium:lower-third:source", "#92a7ad");
    f.animate(surface, Motion::FadeRise, 0, 22);
    f.animate(accent, Motion::LineDraw, 0, 26);
    f.animate(eyebrow, Motion::FadeRise, 4, 18);
    f.animate(name, Motion::SlideLeft, 8, 24);
    f.animate(subtitle, Motion::FadeRise, 16, 22);
    f.animate(source, Motion::FadeRise, 22, 18);
    return f.finish();
}

std::vector<Node> sourceLabel(int width, int height) {
    PremiumFactory f(width, height, "source-label-root", "Source Label · composição");
    const bool portrait = height > width;
    const double s = scaleFor(width, height), margin = width * .06;
    const double boxW = portrait ? width * .78 : width * .43;
    const double boxH = 92 * s;
    // Captions occupy the bottom edge; keep source metadata in the lower-middle safe area.
    const double x = margin, y = height * (portrait ? .76 : .68);
    auto &surface = f.panel("source-label-surface", "Base discreta", "source-label-root", x, y,
                            boxW, boxH, "surface");
    surface.opacity = .94;
    auto &accent = f.rule("source-label-accent", "source-label-root", x, y, 4 * s, boxH, "highlight");
    auto &label = f.text("source-label-title", "Publicação ou fonte", "source-label-root", x + 18 * s,
                         y + 12 * s, boxW - 34 * s, 30 * s,
                         "PUBLICAÇÃO / FONTE", 16 * s,
                         "premium:source-label:title");
    auto &detail = f.text("source-label-detail", "Nota de referência", "source-label-root", x + 18 * s,
                          y + 47 * s, boxW - 34 * s, 28 * s,
                          "Título, seção ou data verificável", 13 * s,
                          "premium:source-label:detail", "#aebfc4");
    f.animate(surface, Motion::SlideLeft, 0, 20);
    f.animate(accent, Motion::LineDraw, 0, 20);
    f.animate(label, Motion::FadeRise, 4, 18);
    f.animate(detail, Motion::FadeRise, 12, 18);
    return f.finish();
}

std::vector<Node> numberEmphasis(int width, int height) {
    PremiumFactory f(width, height, "number-emphasis-root", "Number Emphasis · composição");
    const bool portrait = height > width;
    const double s = scaleFor(width, height), margin = width * .09;
    const double boxW = width - 2 * margin;
    const double boxH = portrait ? height * .27 : height * .46;
    const double x = margin, y = portrait ? height * .29 : height * .23;
    auto &surface = f.panel("number-surface", "Área de dado", "number-emphasis-root", x, y,
                            boxW, boxH, "surface");
    surface.opacity = .92;
    auto &accent = f.rule("number-accent", "number-emphasis-root", x, y, boxW * .17, 5 * s, "accent");
    auto &kicker = f.text("number-kicker", "Identificação", "number-emphasis-root", x + 26 * s,
                          y + 24 * s, boxW - 52 * s, 26 * s,
                          "DADO EM DESTAQUE · DEMONSTRATIVO", 14 * s, {}, "#93bfc0");
    auto &value = f.text("number-value", "Valor principal", "number-emphasis-root", x + 26 * s,
                         y + 62 * s, boxW * .74, boxH * .46, "00,0", (portrait ? 104 : 120) * s,
                         "premium:number:value");
    auto &unit = f.text("number-unit", "Unidade", "number-emphasis-root", x + 28 * s,
                        y + (portrait ? boxH * .39 : boxH * .68), boxW * .72, 36 * s,
                        "%  /  UNIDADE", 22 * s, "premium:number:unit", "#9db7bc");
    auto &description = f.text("number-description", "Descrição e contexto", "number-emphasis-root",
                               x + 28 * s, y + (portrait ? boxH * .59 : boxH + 20 * s), boxW * .84, 58 * s,
                               "Substitua por um número com contexto e fonte verificada.",
                               17 * s, "premium:number:description", "#b0c0c5");
    auto &source = f.text("number-source", "Fonte verificável", "number-emphasis-root",
                          x + 28 * s, y + (portrait ? boxH * .72 : boxH + 65 * s), boxW * .84, 26 * s,
                          "FONTE · PERÍODO · METODOLOGIA", 12 * s,
                          "premium:number:source", "#91a6ac");
    f.animate(surface, Motion::FadeRise, 0, 20);
    f.animate(accent, Motion::LineDraw, 0, 24);
    f.animate(kicker, Motion::FadeRise, 4, 16);
    f.animate(value, Motion::PopScale, 8, 28);
    f.animate(unit, Motion::FadeRise, 18, 20);
    f.animate(description, Motion::FadeRise, 25, 20);
    f.animate(source, Motion::FadeRise, 30, 18);
    return f.finish();
}
} // namespace

std::vector<Node> makePremiumDramaticQuote(int width, int height) {
    return dramaticQuote(std::clamp(width, 1, 30000), std::clamp(height, 1, 30000));
}
std::vector<Node> makePremiumLowerThirdEditorial(int width, int height) {
    return lowerThird(std::clamp(width, 1, 30000), std::clamp(height, 1, 30000));
}
std::vector<Node> makePremiumSourceLabel(int width, int height) {
    return sourceLabel(std::clamp(width, 1, 30000), std::clamp(height, 1, 30000));
}
std::vector<Node> makePremiumNumberEmphasis(int width, int height) {
    return numberEmphasis(std::clamp(width, 1, 30000), std::clamp(height, 1, 30000));
}
} // namespace studio
