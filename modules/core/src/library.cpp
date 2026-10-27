#include <algorithm>
#include <cmath>
#include <unordered_set>
#include <studio/core/library.hpp>

namespace studio {
const std::vector<LibraryItem> &nativeLibrary() {
    static const std::vector<LibraryItem> items{
        {"car", "Carro", "objetos", 460, 210},
        {"person", "Personagem", "personagens", 180, 350},
        {"house", "Casa", "objetos", 240, 210},
        {"building", "Prédio", "objetos", 160, 300},
        {"document", "Documento", "objetos", 200, 250},
        {"computer", "Computador", "objetos", 280, 210},
        {"phone", "Celular", "objetos", 120, 220},
        {"coins", "Moedas", "objetos", 250, 200},
        {"price-tag", "Etiqueta", "objetos", 250, 150},
        {"bars", "Barras ilustrativas", "dados", 320, 240},
        {"street", "Rua", "cenários", 1280, 720},
        {"office", "Escritório", "cenários", 1280, 720},
        {"showroom", "Concessionária", "cenários", 1280, 720},
        {"studio", "Estúdio", "cenários", 1280, 720},
        {"laptop", "Notebook", "tecnologia", 320, 240},
        {"tablet", "Tablet", "tecnologia", 320, 240},
        {"camera", "Câmera", "tecnologia", 320, 240},
        {"microphone", "Microfone", "tecnologia", 320, 240},
        {"headphones", "Fones de ouvido", "tecnologia", 320, 240},
        {"chip", "Chip de IA", "tecnologia", 320, 240},
        {"server", "Servidor", "tecnologia", 320, 240},
        {"cloud", "Nuvem digital", "tecnologia", 320, 240},
        {"wifi", "Sinal Wi-Fi", "tecnologia", 320, 240},
        {"lock", "Cadeado digital", "tecnologia", 320, 240},
        {"notification", "Notificação", "tecnologia", 320, 240},
        {"robot", "Robô", "tecnologia", 320, 240},
        {"chat-bubble", "Balão de conversa", "comunicação", 320, 240},
        {"email", "E-mail", "comunicação", 320, 240},
        {"newspaper", "Jornal", "comunicação", 320, 240},
        {"book", "Livro", "comunicação", 320, 240},
        {"calendar", "Calendário", "comunicação", 320, 240},
        {"clock", "Relógio", "comunicação", 320, 240},
        {"hourglass", "Ampulheta", "comunicação", 320, 240},
        {"eye", "Olho", "comunicação", 320, 240},
        {"magnifier", "Lupa", "comunicação", 320, 240},
        {"target", "Alvo", "comunicação", 320, 240},
        {"chart-line", "Gráfico de linhas", "gráficos", 320, 240},
        {"chart-pie", "Gráfico de pizza", "gráficos", 320, 240},
        {"chart-donut", "Gráfico de rosca", "gráficos", 320, 240},
        {"chart-area", "Gráfico de área", "gráficos", 320, 240},
        {"timeline", "Linha do tempo", "gráficos", 320, 240},
        {"flowchart", "Fluxograma", "gráficos", 320, 240},
        {"network", "Rede de conexões", "gráficos", 320, 240},
        {"dashboard", "Painel de indicadores", "gráficos", 320, 240},
        {"progress-ring", "Anel de progresso ilustrativo", "gráficos", 320, 240},
        {"data-table", "Tabela de dados ilustrativa", "gráficos", 320, 240},
        {"bank", "Banco", "finanças", 320, 240},
        {"wallet", "Carteira", "finanças", 320, 240},
        {"credit-card", "Cartão de crédito", "finanças", 320, 240},
        {"cash", "Dinheiro", "finanças", 320, 240},
        {"receipt", "Recibo", "finanças", 320, 240},
        {"shopping-cart", "Carrinho de compras", "finanças", 320, 240},
        {"factory", "Fábrica", "finanças", 320, 240},
        {"briefcase", "Maleta", "finanças", 320, 240},
        {"atom", "Átomo", "ciência", 320, 240},
        {"dna", "DNA", "ciência", 320, 240},
        {"flask", "Frasco de laboratório", "ciência", 320, 240},
        {"brain", "Cérebro", "ciência", 320, 240},
        {"globe", "Globo terrestre", "ciência", 320, 240},
        {"map-pin", "Marcador de mapa", "ciência", 320, 240},
        {"compass", "Bússola", "ciência", 320, 240},
        {"rocket", "Foguete", "ciência", 320, 240},
        {"satellite", "Satélite", "ciência", 320, 240},
        {"lightbulb", "Lâmpada", "ciência", 320, 240},
        {"night-city", "Cidade à noite", "cenários", 1280, 720},
        {"server-room", "Sala de servidores", "cenários", 1280, 720},
        {"newsroom", "Redação de notícias", "cenários", 1280, 720},
        {"laboratory-scene", "Laboratório", "cenários", 1280, 720},
        {"map-room", "Mapa mundial estilizado", "cenários", 1280, 720},
        {"financial-district", "Distrito financeiro", "cenários", 1280, 720},
        {"cyber-grid", "Grade digital", "cenários", 1280, 720},
        {"archive-room", "Arquivo documental", "cenários", 1280, 720}
    };
    return items;
}
std::vector<Node> makePremiumSmartphone() { return makePremiumSmartphone(1280, 720); }
std::vector<Node> makePremiumSmartphone(int canvasWidth, int canvasHeight) {
    std::vector<Node> nodes;
    nodes.reserve(24);
    auto add = [&](std::string id, std::string name, NodeKind kind, double x, double y,
                   double w, double h, std::string color, std::string parent = {}) -> Node & {
        Node n;
        n.id = std::move(id); n.name = std::move(name); n.kind = kind;
        n.x = x; n.y = y; n.width = w; n.height = h; n.color = std::move(color);
        n.parentId = std::move(parent); n.start = 0; n.duration = 360;
        n.animated = false; n.motion = "none";
        nodes.push_back(std::move(n));
        return nodes.back();
    };
    const bool portrait = canvasHeight > canvasWidth;
    const double factor = std::clamp((portrait ? canvasHeight * .34 : canvasHeight * .72) / 500.0, .85, 1.25);
    const double phoneX = portrait ? (canvasWidth - 260 * factor) / 2.0 : canvasWidth * .64 - 130 * factor;
    const double phoneY = (canvasHeight - 500 * factor) / 2.0;
    auto &root = add("phone-premium", "Smartphone premium", NodeKind::Group, phoneX, phoneY, 260, 500, "#0b1118");
    root.scale = factor;
    root.keyframes = {{0, phoneX, phoneY + 25, 0, factor * .96, 0, "ease-out"},
                      {24, phoneX, phoneY, 0, factor, 1, "ease-out"},
                      {60, phoneX, phoneY - 6, 0, factor * 1.015, 1, "ease-in-out"},
                      {120, phoneX, phoneY, 0, factor, 1, "ease-in-out"}};
    auto child = [&](std::string id, std::string name, NodeKind kind, double x, double y,
                     double w, double h, std::string color, std::string role = {}) -> Node & {
        auto &n = add(std::move(id), std::move(name), kind, x, y, w, h, std::move(color), "phone-premium");
        n.role = std::move(role);
        return n;
    };
    child("shadow", "Sombra simulada", NodeKind::Shape, 8, 12, 244, 480, "#030609").opacity = .75;
    child("rail", "Aro externo", NodeKind::Shape, 0, 0, 244, 480, "#71808c", "palette:frame");
    child("body", "Corpo em grafite", NodeKind::Shape, 4, 4, 236, 472, "#171e27", "palette:surface");
    child("inner-rail", "Moldura metálica", NodeKind::Shape, 10, 10, 224, 460, "#323e49", "palette:frame");
    child("glass", "Vidro frontal", NodeKind::Shape, 14, 14, 216, 452, "#05090e");
    child("screen", "Slot de imagem da tela", NodeKind::Shape, 24, 42, 196, 394, "#102539", "premium:image:screen");
    child("reflection", "Reflexo de vidro", NodeKind::Shape, 24, 42, 196, 394, "#496579", "palette:highlight").opacity = .13;
    auto &camera = child("camera-island", "Ilha de sensores", NodeKind::Shape, 82, 18, 80, 20, "#020407");
    camera.opacity = .98;
    child("earpiece", "Alto-falante", NodeKind::Shape, 104, 24, 36, 4, "#53616c");
    auto &clock = child("clock", "Hora da tela", NodeKind::Text, 39, 64, 125, 38, "#f2f6f8", "premium:text:screen-title");
    clock.text = "09:41"; clock.fontSize = 27;
    auto &status = child("status", "Indicadores", NodeKind::Text, 40, 92, 165, 18, "#d8e1e8", "palette:muted");
    status.text = "●  5G                             98%"; status.fontSize = 9;
    child("divider", "Filete de luz", NodeKind::Shape, 30, 119, 184, 2, "#9bc9e3", "palette:accent").opacity = .65;
    auto &notification = child("notification", "Grupo de notificação", NodeKind::Group, 34, 142, 176, 92, "#dbe6eb");
    notification.opacity = 0;
    notification.keyframes = {{0,34,142,0,1,0,"linear"},{30,34,142,0,1,0,"ease-out"},
                              {42,34,142,0,1,1,"ease-out"},{92,34,130,0,1,1,"ease-in-out"}};
    auto &notificationCard = add("notification-card", "Cartão da notificação", NodeKind::Shape,
                                 0, 0, 176, 92, "#dbe6eb", "notification");
    notificationCard.role = "palette:notification";
    notificationCard.animated = false; notificationCard.motion = "none";
    auto &app = add("app-name", "Aplicativo da notificação", NodeKind::Text, 12, 10, 150, 20,
                    "#233545", "notification");
    app.role = ""; app.text = "STUDIO DARK"; app.fontSize = 11;
    auto &message = add("message", "Mensagem substituível", NodeKind::Text, 12, 33, 150, 49,
                        "#1b2a35", "notification");
    message.role = "premium:text:notification";
    message.text = "Seu projeto está pronto\npara a próxima cena."; message.fontSize = 12;
    for (auto *part : {&app, &message}) {
        part->opacity = 0;
        part->keyframes = {{0, part->x, part->y, 0, 1, 0, "linear"},
                           {30, part->x, part->y, 0, 1, 0, "ease-out"},
                           {42, part->x, part->y, 0, 1, 1, "ease-out"},
                           {92, part->x, part->y, 0, 1, 1, "ease-in-out"}};
    }
    child("home", "Indicador inferior", NodeKind::Shape, 93, 451, 58, 4, "#d6dfe4");
    child("edge-highlight", "Brilho de borda", NodeKind::Shape, 6, 8, 3, 456, "#b4d7e9", "palette:highlight").opacity = .5;
    return nodes;
}
namespace {
std::vector<Node> makePremiumTitleRevealImpl(int width, int height) {
    std::vector<Node> nodes;
    nodes.reserve(8);
    const bool portrait = height > width;
    const double margin = width * .085;
    const double s = std::clamp(std::min(width / 1280.0, height / 720.0), .72, 1.6);
    auto add = [&](std::string id, std::string name, NodeKind kind, double x, double y,
                   double w, double h, std::string color, std::string parent = {}) -> Node & {
        Node n;
        n.id = std::move(id); n.name = std::move(name); n.kind = kind;
        n.x = x; n.y = y; n.width = w; n.height = h; n.color = std::move(color);
        n.parentId = std::move(parent); n.start = 0; n.duration = 360;
        n.animated = false; n.motion = "none";
        nodes.push_back(std::move(n));
        return nodes.back();
    };
    auto &root = add("title-reveal-root", "Title Reveal · composição", NodeKind::Group,
                     0, 0, width, height, "#000000");
    root.keyframes = {{0, 0, 0, 0, 1, 0, "ease-out"},
                      {24, 0, 0, 0, 1, 1, "ease-out"},
                      {336, 0, 0, 0, 1, 1, "linear"}};
    root.exitFrames = 24;
    const double top = portrait ? height * .31 : height * .27;
    const double markerH = portrait ? height * .20 : height * .25;
    auto &rule = add("title-editorial-rule", "Filete editorial", NodeKind::Shape,
                     margin, top, 5 * s, markerH, "#b4f078", "title-reveal-root");
    rule.role = "palette:accent";
    rule.opacity = .95;
    rule.keyframes = {{0, margin, top, 0, .12, 0, "ease-out"},
                      {22, margin, top, 0, 1, 1, "ease-out"}};
    auto &eyebrow = add("title-editorial-id", "Identificação editorial", NodeKind::Text,
                        margin + 28 * s, top, width - margin * 2 - 28 * s, 30 * s,
                        "#b4f078", "title-reveal-root");
    eyebrow.role = "premium:title:editorial";
    eyebrow.text = "STUDIO DARK  /  ESPECIAL"; eyebrow.fontSize = 16 * s;
    eyebrow.animated = true; eyebrow.motion = "rise"; eyebrow.motionFrames = 28;
    const double titleY = top + markerH * .20;
    const double titleH = portrait ? height * .24 : height * .32;
    auto &title = add("title-main", "Título principal", NodeKind::Text,
                      margin + 28 * s, titleY, width - margin * 2 - 36 * s, titleH,
                      "#eef4ef", "title-reveal-root");
    title.role = "premium:title:main";
    title.text = "UMA HISTÓRIA\nEM MOVIMENTO";
    title.fontSize = (portrait ? 78 : 88) * s;
    title.animated = false; title.motion = "none"; title.motionFrames = 42;
    const double subtitleY = titleY + titleH + (portrait ? 20 : 10) * s;
    auto &subtitle = add("title-subtitle", "Subtítulo", NodeKind::Text,
                         margin + 28 * s, subtitleY, width - margin * 2 - 36 * s, 64 * s,
                         "#a8bbc0", "title-reveal-root");
    subtitle.role = "premium:title:subtitle";
    subtitle.text = "Ideias que merecem ser vistas de perto.";
    subtitle.fontSize = 26 * s; subtitle.animated = true; subtitle.motion = "rise";
    subtitle.motionFrames = 34;
    subtitle.keyframes = {{0, subtitle.x, subtitle.y + 18 * s, 0, 1, 0, "ease-out"},
                          {34, subtitle.x, subtitle.y, 0, 1, 1, "ease-out"}};
    auto &tick = add("title-editorial-tick", "Detalhe editorial", NodeKind::Shape,
                     width - margin - 44 * s, titleY + titleH + 5 * s,
                     44 * s, 2 * s, "#efb873", "title-reveal-root");
    tick.role = "palette:secondary";
    tick.keyframes = {{0, tick.x, tick.y, 0, .1, 0, "ease-out"},
                      {30, tick.x, tick.y, 0, 1, 1, "ease-out"}};
    return nodes;
}
} // namespace
std::vector<Node> makePremiumTitleReveal(int width, int height) {
    return makePremiumTitleRevealImpl(std::clamp(width, 1, 30000), std::clamp(height, 1, 30000));
}
std::vector<Node> makePremiumComparisonPack(int width, int height) {
    width = std::clamp(width, 1, 30000);
    height = std::clamp(height, 1, 30000);
    std::vector<Node> nodes;
    nodes.reserve(24);
    const bool portrait = height > width;
    const double scale = std::clamp(std::min(width / 1280.0, height / 720.0), .72, 1.6);
    const double margin = width * .075;
    auto add = [&](std::string id, std::string name, NodeKind kind, double x, double y,
                   double w, double h, std::string color, std::string parent = {}) -> Node & {
        Node n;
        n.id = std::move(id); n.name = std::move(name); n.kind = kind;
        n.x = x; n.y = y; n.width = w; n.height = h; n.color = std::move(color);
        n.parentId = std::move(parent); n.start = 0; n.duration = 360;
        n.animated = false; n.motion = "none";
        nodes.push_back(std::move(n));
        return nodes.back();
    };
    auto &root = add("comparison-root", "Comparison Pack · composição", NodeKind::Group,
                     0, 0, width, height, "#000000");
    root.keyframes = {{0, 0, 0, 0, 1, 0, "ease-out"},
                      {28, 0, 0, 0, 1, 1, "ease-out"},
                      {336, 0, 0, 0, 1, 1, "linear"}};
    root.exitFrames = 22;
    auto &eyebrow = add("comparison-eyebrow", "Identificação editorial", NodeKind::Text,
                        margin, height * .15, width - 2 * margin, 24 * scale,
                        "#b4f078", "comparison-root");
    eyebrow.role = "palette:highlight";
    eyebrow.text = "COMPARAÇÃO  /  TEMPLATE DEMONSTRATIVO";
    eyebrow.fontSize = 15 * scale;
    auto &heading = add("comparison-heading", "Título da comparação", NodeKind::Text,
                        margin, height * .19, width - 2 * margin, height * .1,
                        "#eef4ef", "comparison-root");
    heading.text = "DOIS CENÁRIOS. UMA ESCOLHA.";
    heading.fontSize = (portrait ? 45 : 52) * scale;
    const double gap = portrait ? 24 * scale : 28 * scale;
    const double panelX1 = margin;
    const double panelWidth = portrait ? width - 2 * margin : (width - 2 * margin - gap) / 2;
    const double panelX2 = portrait ? margin : panelX1 + panelWidth + gap;
    const double panelY1 = portrait ? height * .32 : height * .35;
    const double panelHeight = portrait ? height * .235 : height * .43;
    const double panelY2 = portrait ? height * .585 : panelY1;
    const std::string panelIds[]{"comparison-panel-left", "comparison-panel-right"};
    const double panelXs[]{panelX1, panelX2};
    const double panelYs[]{panelY1, panelY2};
    const std::string panelColors[]{"#10242d", "#1c2a28"};
    const std::string accents[]{"#83bdc7", "#b4f078"};
    const std::string titles[]{"CENÁRIO A", "CENÁRIO B"};
    const std::string values[]{"PERSONALIZE\nESTE VALOR", "PERSONALIZE\nESTE VALOR"};
    const std::string descriptions[]{"Substitua por informação verificada.",
                                     "Substitua por informação verificada."};
    for (int i = 0; i < 2; ++i) {
        auto &panel = add(panelIds[i], i == 0 ? "Painel esquerdo" : "Painel direito",
                          NodeKind::Group, panelXs[i], panelYs[i], panelWidth, panelHeight,
                          panelColors[i], "comparison-root");
        panel.opacity = 0;
        panel.keyframes = {{0, panel.x + (i == 0 ? -26 : 26), panel.y, 0, .96, 0, "ease-out"},
                           {34 + i * 8, panel.x, panel.y, 0, 1, 1, "ease-out"}};
        auto &panelSurface = add("comparison-panel-bg-" + std::to_string(i), "Superfície do painel", NodeKind::Shape,
                                 0, 0, panelWidth, panelHeight, panelColors[i], panel.id);
        panelSurface.role = i == 0 ? "palette:left" : "palette:right";
        auto &panelAccent = add("comparison-panel-accent-" + std::to_string(i), "Acento do painel", NodeKind::Shape,
                                0, 0, panelWidth * .24, 5 * scale, accents[i], panel.id);
        panelAccent.role = i == 0 ? "palette:primary" : "palette:secondary";
        const double inset = panelWidth * .075;
        auto &label = add(i == 0 ? "comparison-left-title" : "comparison-right-title",
                          i == 0 ? "Título esquerdo" : "Título direito", NodeKind::Text,
                          inset, panelHeight * .16, panelWidth - 2 * inset, 32 * scale,
                          accents[i], panel.id);
        label.role = i == 0 ? "premium:comparison:left-title" : "premium:comparison:right-title";
        label.text = titles[i]; label.fontSize = 18 * scale;
        auto &value = add(i == 0 ? "comparison-left-value" : "comparison-right-value",
                          i == 0 ? "Valor esquerdo" : "Valor direito", NodeKind::Text,
                          inset, panelHeight * .34, panelWidth - 2 * inset, panelHeight * .32,
                          "#eef4ef", panel.id);
        value.role = i == 0 ? "premium:comparison:left-value" : "premium:comparison:right-value";
        value.text = values[i]; value.fontSize = (portrait ? 38 : 34) * scale;
        auto &description = add(i == 0 ? "comparison-left-description" : "comparison-right-description",
                                i == 0 ? "Descrição esquerda" : "Descrição direita", NodeKind::Text,
                                inset, panelHeight * .74, panelWidth - 2 * inset, panelHeight * .19,
                                "#9fb2b9", panel.id);
        description.role = i == 0 ? "premium:comparison:left-description"
                                  : "premium:comparison:right-description";
        description.text = descriptions[i]; description.fontSize = 15 * scale;
    }
    {
        const double splitX = (panelX1 + panelWidth + panelX2) / 2;
        const double dividerX = portrait ? margin + panelWidth * .1 : splitX - 1.5 * scale;
        const double dividerY = portrait ? (panelY1 + panelHeight + panelY2) / 2 : panelY1 + 28 * scale;
        const double dividerW = portrait ? panelWidth * .8 : 3 * scale;
        const double dividerH = portrait ? 3 * scale : panelHeight - 56 * scale;
        auto &divider = add("comparison-divider", "Separador central", NodeKind::Shape,
                            dividerX, dividerY, dividerW, dividerH, "#71898b", "comparison-root");
        divider.role = "palette:accent";
        divider.opacity = .65;
    }
    auto &footer = add("comparison-footer", "Nota do template", NodeKind::Text,
                       margin, portrait ? height * .86 : height * .82,
                       width - 2 * margin, 40 * scale, "#efb873", "comparison-root");
    footer.text = "EXEMPLO VISUAL — substitua por dados com fonte.";
    footer.fontSize = 17 * scale;
    return nodes;
}
namespace {
std::vector<PremiumAssetDefinition> &premiumRegistry() {
    static std::vector<PremiumAssetDefinition> assets{
        {"premium:smartphone-premium-01", "Smartphone premium", "premium-object", "technology",
         "smartphone-premium-01.manifest.json", makePremiumSmartphone,
         {"premium:text:screen-title", "premium:text:notification", "premium:image:screen"},
         {"surface", "frame", "highlight", "accent", "screen", "notification", "muted", "text"}, {}, {}, 1, {}},
        {"premium:title-reveal-01", "Title Reveal · título cinematográfico", "premium-typography-template",
         "typography", "title-reveal-01.manifest.json", makePremiumTitleReveal,
         {"premium:title:main", "premium:title:subtitle", "premium:title:editorial"},
         {"accent", "secondary", "highlight", "text", "muted"}, {}, {}, 1, {}},
        {"premium:comparison-pack-01", "Comparison Pack · comparação editorial", "premium-motion-template",
         "data", "comparison-pack-01.manifest.json", makePremiumComparisonPack,
         {"premium:comparison:left-title", "premium:comparison:right-title",
          "premium:comparison:left-value", "premium:comparison:right-value",
          "premium:comparison:left-description", "premium:comparison:right-description"},
         {"left", "right", "primary", "secondary", "accent", "highlight", "text", "muted"}, {}, {}, 1, {}},
        {"premium:dramatic-quote-01", "Dramatic Quote · citação editorial", "premium-typography-template",
         "typography", "dramatic-quote-01.manifest.json", makePremiumDramaticQuote,
         {"premium:quote:text", "premium:quote:attribution", "premium:quote:source"},
         {"accent", "text", "muted"}, {}, {}, 1, {}},
        {"premium:lower-third-editorial-01", "Lower Third Editorial · identificação", "premium-overlay",
         "typography", "lower-third-editorial-01.manifest.json", makePremiumLowerThirdEditorial,
         {"premium:lower-third:title", "premium:lower-third:subtitle", "premium:lower-third:source"},
         {"surface", "accent", "text", "muted"}, {}, {}, 1, {}},
        {"premium:source-label-01", "Source Label · referência editorial", "premium-overlay",
         "communication", "source-label-01.manifest.json", makePremiumSourceLabel,
         {"premium:source-label:title", "premium:source-label:detail"},
         {"surface", "highlight", "text", "muted"}, {}, {}, 1, {}},
        {"premium:number-emphasis-01", "Number Emphasis · dado em destaque", "premium-motion-template",
         "data", "number-emphasis-01.manifest.json", makePremiumNumberEmphasis,
         {"premium:number:value", "premium:number:unit", "premium:number:description", "premium:number:source"},
         {"surface", "accent", "text", "muted", "secondary"}, {}, {}, 1, {}}};
    return assets;
}
} // namespace
const std::vector<PremiumAssetDefinition> &premiumLibrary() {
    return premiumRegistry();
}
bool registerDeclarativePremium(PremiumAssetDefinition definition, std::string &error) {
    if (!definition.declarative || definition.id.empty() || definition.build ||
        definition.manifestVersion != 2 || definition.declarative->landscape.layers.empty() ||
        definition.declarative->portrait.layers.empty()) {
        error = "Definição premium declarativa incompleta.";
        return false;
    }
    auto &assets = premiumRegistry();
    if (std::any_of(assets.begin(), assets.end(), [&](const auto &asset) { return asset.id == definition.id; })) {
        error = "ID premium duplicado: " + definition.id;
        return false;
    }
    assets.push_back(std::move(definition));
    return true;
}
bool registerPremiumTextLayoutPolicies(const std::string &assetId,
                                       std::vector<PremiumTextLayoutPolicy> policies,
                                       std::string &error) {
    auto &assets = premiumRegistry();
    auto asset = std::find_if(assets.begin(), assets.end(),
                              [&](const auto &candidate) { return candidate.id == assetId; });
    if (asset == assets.end()) {
        error = "Recurso premium ausente ao registrar política textual: " + assetId;
        return false;
    }
    std::unordered_set<std::string> roles;
    for (const auto &policy : policies) {
        if (policy.role.empty() || !roles.insert(policy.role).second ||
            std::find(asset->slotRoles.begin(), asset->slotRoles.end(), policy.role) == asset->slotRoles.end() ||
            !std::isfinite(policy.minimumFontSize) || policy.minimumFontSize < 8 ||
            policy.minimumFontSize > 72 || !std::isfinite(policy.padding) || policy.padding < 0 ||
            policy.padding > 64) {
            error = "Política textual inválida para o premium " + assetId + ".";
            return false;
        }
    }
    asset->textLayoutPolicies = std::move(policies);
    return true;
}
const PremiumAssetDefinition *premiumAsset(const std::string &id) {
    const auto &assets = premiumLibrary();
    const auto it = std::find_if(assets.begin(), assets.end(),
                                 [&](const auto &asset) { return asset.id == id; });
    return it == assets.end() ? nullptr : &*it;
}
std::vector<Node> makePremiumAsset(const std::string &id, int width, int height) {
    const auto *asset = premiumAsset(id);
    if (!asset)
        return {};
    const auto canvasWidth = std::clamp(width, 1, 30000);
    const auto canvasHeight = std::clamp(height, 1, 30000);
    auto nodes = asset->build ? asset->build(canvasWidth, canvasHeight)
                              : buildDeclarativePremium(*asset, canvasWidth, canvasHeight);
    const int referenceWidth = canvasHeight > canvasWidth ? 1080 : 1280;
    const int referenceHeight = canvasHeight > canvasWidth ? 1920 : 720;
    const double scale = std::min(double(canvasWidth) / referenceWidth,
                                  double(canvasHeight) / referenceHeight);
    for (const auto &policy : asset->textLayoutPolicies)
        for (auto &node : nodes)
            if (node.role == policy.role) {
                node.textAutoFit = true;
                node.textMinFontSize = std::min(node.fontSize, policy.minimumFontSize * scale);
                node.textPadding = policy.padding * scale;
            }
    return nodes;
}
bool premiumSlotRole(const std::string &role) {
    if (role.empty()) return false;
    static const auto roles = [] {
        std::unordered_set<std::string> result;
        for (const auto &asset : premiumLibrary())
            for (const auto &node : makePremiumAsset(asset.id, 1280, 720))
                if (!node.role.empty()) result.insert(node.role);
        return result;
    }();
    if (roles.contains(role))
        return true;
    // The core role cache may have been built before Storage loads declarative manifests.
    // Check the live registry so nodes created after that point are valid too.
    for (const auto &asset : premiumLibrary())
        if (asset.declarative)
            for (const auto *layout : {&asset.declarative->landscape, &asset.declarative->portrait})
                for (const auto &layer : layout->layers)
                    if (layer.role == role)
                        return true;
    return false;
}
const LibraryItem *libraryItem(const std::string &id) {
    const auto &items = nativeLibrary();
    auto it = std::find_if(items.begin(), items.end(), [&](const auto &a) { return a.id == id; });
    return it == items.end() ? nullptr : &*it;
}
const std::vector<std::string> &motionNames() {
    static const std::vector<std::string> v{"none", "fade", "rise", "slide-left", "slide-right", "pop"};
    return v;
}
const std::vector<std::string> &poseNames() {
    static const std::vector<std::string> v{"neutral", "point", "wave", "worried"};
    return v;
}
bool supportedMotion(const std::string &name) {
    const auto &v = motionNames();
    return std::find(v.begin(), v.end(), name) != v.end();
}
bool supportedPose(const std::string &name) {
    const auto &v = poseNames();
    return std::find(v.begin(), v.end(), name) != v.end();
}
} // namespace studio
