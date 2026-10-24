#include <algorithm>
#include <studio/core/library.hpp>

namespace studio {
const std::vector<LibraryItem> &nativeLibrary() {
    static const std::vector<LibraryItem> items{{"car", "Carro", "objetos", 460, 210},
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
                                                {"studio", "Estúdio", "cenários", 1280, 720}};
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
