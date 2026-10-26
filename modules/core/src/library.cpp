#include <algorithm>
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
std::vector<Node> makePremiumSmartphone() {
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
    auto &root = add("phone-premium", "Smartphone premium", NodeKind::Group, 790, 140, 260, 500, "#0b1118");
    root.keyframes = {{0, 790, 165, 0, .96, 0, "ease-out"},
                      {24, 790, 140, 0, 1, 1, "ease-out"},
                      {60, 790, 134, 0, 1.015, 1, "ease-in-out"},
                      {120, 790, 140, 0, 1, 1, "ease-in-out"}};
    auto child = [&](std::string id, std::string name, NodeKind kind, double x, double y,
                     double w, double h, std::string color, std::string role = {}) -> Node & {
        auto &n = add(std::move(id), std::move(name), kind, x, y, w, h, std::move(color), "phone-premium");
        n.role = std::move(role);
        return n;
    };
    child("shadow", "Sombra simulada", NodeKind::Shape, 8, 12, 244, 480, "#030609").opacity = .75;
    child("rail", "Aro externo", NodeKind::Shape, 0, 0, 244, 480, "#71808c");
    child("body", "Corpo em grafite", NodeKind::Shape, 4, 4, 236, 472, "#171e27");
    child("inner-rail", "Moldura metálica", NodeKind::Shape, 10, 10, 224, 460, "#323e49");
    child("glass", "Vidro frontal", NodeKind::Shape, 14, 14, 216, 452, "#05090e");
    child("screen", "Slot de imagem da tela", NodeKind::Shape, 24, 42, 196, 394, "#102539", "premium:image:screen");
    child("reflection", "Reflexo de vidro", NodeKind::Shape, 24, 42, 196, 394, "#496579").opacity = .13;
    auto &camera = child("camera-island", "Ilha de sensores", NodeKind::Shape, 82, 18, 80, 20, "#020407");
    camera.opacity = .98;
    child("earpiece", "Alto-falante", NodeKind::Shape, 104, 24, 36, 4, "#53616c");
    auto &clock = child("clock", "Hora da tela", NodeKind::Text, 39, 64, 125, 38, "#f2f6f8", "premium:text:screen-title");
    clock.text = "09:41"; clock.fontSize = 27;
    auto &status = child("status", "Indicadores", NodeKind::Text, 40, 92, 165, 18, "#d8e1e8");
    status.text = "●  5G                             98%"; status.fontSize = 9;
    child("divider", "Filete de luz", NodeKind::Shape, 30, 119, 184, 2, "#9bc9e3").opacity = .65;
    auto &notification = child("notification", "Grupo de notificação", NodeKind::Group, 34, 142, 176, 92, "#dbe6eb");
    notification.opacity = 0;
    notification.keyframes = {{0,34,142,0,1,0,"linear"},{30,34,142,0,1,0,"ease-out"},
                              {42,34,142,0,1,1,"ease-out"},{92,34,130,0,1,1,"ease-in-out"}};
    auto &notificationCard = add("notification-card", "Cartão da notificação", NodeKind::Shape,
                                 0, 0, 176, 92, "#dbe6eb", "notification");
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
    child("edge-highlight", "Brilho de borda", NodeKind::Shape, 6, 8, 3, 456, "#b4d7e9").opacity = .5;
    return nodes;
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
