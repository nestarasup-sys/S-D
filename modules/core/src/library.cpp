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
