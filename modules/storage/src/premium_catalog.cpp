#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QMap>
#include <QResource>
#include <QRegularExpression>
#include <QSet>
#include <algorithm>
#include <cmath>
#include <mutex>
#include <studio/core/library.hpp>
#include <studio/storage/workspace_store.hpp>

static void initializePremiumManifestResources() { Q_INIT_RESOURCE(premium_manifests); }

namespace studio {
namespace {
bool finiteNumber(const QJsonObject &object, const QString &key, double &out) {
    if (!object.value(key).isDouble())
        return false;
    out = object.value(key).toDouble();
    return std::isfinite(out);
}

bool parseLayout(const QJsonObject &definition, const QString &name, PremiumDeclarativeLayout &layout,
                 QString &error) {
    for (auto it = definition.begin(); it != definition.end(); ++it)
        if (it.key() != "definitionVersion" && it.key() != "landscape" && it.key() != "portrait") {
            error = "campo desconhecido na definição premium: " + it.key() + ".";
            return false;
        }
    const auto variant = definition.value(name).toObject();
    for (auto it = variant.begin(); it != variant.end(); ++it)
        if (it.key() != "canvas" && it.key() != "layers") {
            error = "campo desconhecido na variante " + name + ".";
            return false;
        }
    const auto canvas = variant.value("canvas").toObject();
    for (auto it = canvas.begin(); it != canvas.end(); ++it)
        if (it.key() != "width" && it.key() != "height") {
            error = "campo desconhecido no canvas da variante " + name + ".";
            return false;
        }
    if (!variant.value("canvas").isObject() || !variant.value("layers").isArray() ||
        variant.value("layers").toArray().isEmpty() || variant.value("layers").toArray().size() > 64 ||
        !canvas.value("width").isDouble() || !canvas.value("height").isDouble()) {
        error = "variante " + name + " precisa de canvas e 1–64 camadas.";
        return false;
    }
    layout.referenceWidth = canvas.value("width").toInt();
    layout.referenceHeight = canvas.value("height").toInt();
    if ((name == "landscape" && (layout.referenceWidth != 1280 || layout.referenceHeight != 720)) ||
        (name == "portrait" && (layout.referenceWidth != 1080 || layout.referenceHeight != 1920))) {
        error = "canvas de referência incompatível na variante " + name + ".";
        return false;
    }
    QSet<QString> ids;
    int depth = 0;
    for (const auto &value : variant.value("layers").toArray()) {
        if (!value.isObject()) {
            error = "camada não é um objeto na variante " + name + ".";
            return false;
        }
        const auto object = value.toObject();
        static const QStringList fields{"id", "parentId", "kind", "component", "role", "name", "layout",
                                        "color", "text", "fontSize", "opacity", "rotation", "scale",
                                        "paletteToken", "animation"};
        for (auto it = object.begin(); it != object.end(); ++it)
            if (!fields.contains(it.key())) {
                error = "campo desconhecido '" + it.key() + "' na camada declarativa.";
                return false;
            }
        PremiumDeclarativeLayer layer;
        if (!object.value("id").isString() || !object.value("kind").isString() ||
            !object.value("component").isString() || !object.value("name").isString() ||
            (object.contains("parentId") && !object.value("parentId").isString()) ||
            (object.contains("role") && !object.value("role").isString()) ||
            (object.contains("color") && !object.value("color").isString()) ||
            (object.contains("text") && !object.value("text").isString()) ||
            (object.contains("paletteToken") && !object.value("paletteToken").isString()) ||
            (object.contains("animation") && !object.value("animation").isObject())) {
            error = "tipo de campo inválido na camada declarativa.";
            return false;
        }
        layer.id = object.value("id").toString().toStdString();
        layer.parentId = object.value("parentId").toString().toStdString();
        layer.name = object.value("name").toString().toStdString();
        layer.component = object.value("component").toString().toStdString();
        layer.role = object.value("role").toString().toStdString();
        layer.text = object.value("text").toString().toStdString();
        layer.color = object.value("color").toString("#e4f5e9").toStdString();
        layer.paletteToken = object.value("paletteToken").toString().toStdString();
        if (layer.id.empty() || layer.id.size() > 80 || ids.contains(QString::fromStdString(layer.id)) ||
            layer.name.empty() || layer.name.size() > 120 ||
            !QStringList{"group", "panel", "rule", "badge", "text"}.contains(
                QString::fromStdString(layer.component))) {
            error = "ID, nome ou componente declarativo inválido.";
            return false;
        }
        const auto expectedKind = layer.component == "group" ? "group"
                                 : layer.component == "text" ? "text" : "shape";
        if (object.value("kind").toString() != expectedKind) {
            error = "kind não corresponde ao componente na camada " + QString::fromStdString(layer.id) + ".";
            return false;
        }
        const auto box = object.value("layout").toObject();
        if (!object.value("layout").isObject() || !finiteNumber(box, "x", layer.x) ||
            !finiteNumber(box, "y", layer.y) || !finiteNumber(box, "width", layer.width) ||
            !finiteNumber(box, "height", layer.height) || layer.x < 0 || layer.y < 0 ||
            layer.width <= 0 || layer.height <= 0 || layer.x + layer.width > layout.referenceWidth + 1 ||
            layer.y + layer.height > layout.referenceHeight + 1) {
            error = "layout fora do canvas na camada " + QString::fromStdString(layer.id) + ".";
            return false;
        }
        if (object.contains("fontSize") && !finiteNumber(object, "fontSize", layer.fontSize)) {
            error = "fontSize inválido na camada " + QString::fromStdString(layer.id) + ".";
            return false;
        }
        if (object.contains("opacity") && !finiteNumber(object, "opacity", layer.opacity)) {
            error = "opacity inválida na camada " + QString::fromStdString(layer.id) + ".";
            return false;
        }
        if (object.contains("rotation") && !finiteNumber(object, "rotation", layer.rotation)) {
            error = "rotation inválida na camada " + QString::fromStdString(layer.id) + ".";
            return false;
        }
        if (object.contains("scale") && !finiteNumber(object, "scale", layer.scale)) {
            error = "scale inválida na camada " + QString::fromStdString(layer.id) + ".";
            return false;
        }
        if ((layer.component == "text" && (layer.fontSize < 8 || layer.fontSize > 180 || layer.text.size() > 2000)) ||
            layer.opacity < 0 || layer.opacity > 1 || layer.rotation < -360 || layer.rotation > 360 ||
            layer.scale < .1 || layer.scale > 10 || layer.color.size() != 7 || layer.color.front() != '#') {
            error = "estilo da camada declarativa fora dos limites: " + QString::fromStdString(layer.id) + ".";
            return false;
        }
        static const QRegularExpression colorPattern("^#[0-9A-Fa-f]{6}$");
        if (!colorPattern.match(QString::fromStdString(layer.color)).hasMatch()) {
            error = "cor inválida na camada " + QString::fromStdString(layer.id) + ".";
            return false;
        }
        if (object.contains("animation")) {
            const auto animation = object.value("animation").toObject();
            static const QStringList animationFields{"preset", "delayFrames", "durationFrames", "staggerFrames"};
            for (auto it = animation.begin(); it != animation.end(); ++it)
                if (!animationFields.contains(it.key())) {
                    error = "campo de animação desconhecido na camada " + QString::fromStdString(layer.id) + ".";
                    return false;
                }
            layer.animation = animation.value("preset").toString().toStdString();
            if (!QStringList{"fade-rise", "slide-left", "slide-right", "slide-up", "slide-down",
                             "pop-scale", "soft-float", "line-draw", "stagger-reveal"}
                     .contains(QString::fromStdString(layer.animation))) {
                error = "preset de animação não suportado na camada " + QString::fromStdString(layer.id) + ".";
                return false;
            }
            const auto integer = [&](const char *key, int fallback, int minimum, int maximum, int &out) {
                const auto field = QLatin1String(key);
                if (!animation.contains(field)) {
                    out = fallback;
                    return true;
                }
                const auto number = animation.value(field).toDouble(-1);
                if (!animation.value(field).isDouble() || std::floor(number) != number || number < minimum ||
                    number > maximum)
                    return false;
                out = int(number);
                return true;
            };
            if (!integer("delayFrames", 0, 0, 300, layer.animationDelayFrames) ||
                !integer("durationFrames", 24, 4, 120, layer.animationDurationFrames) ||
                !integer("staggerFrames", 0, 0, 30, layer.staggerFrames)) {
                error = "duração ou atraso de animação fora dos limites na camada " +
                        QString::fromStdString(layer.id) + ".";
                return false;
            }
        }
        ids.insert(QString::fromStdString(layer.id));
        layout.layers.push_back(std::move(layer));
        depth = std::max(depth, int(layout.layers.size()));
    }
    if (layout.layers.front().component != "group" || !layout.layers.front().parentId.empty()) {
        error = "a primeira camada deve ser o grupo raiz sem parentId.";
        return false;
    }
    QMap<QString, QString> parents;
    for (const auto &layer : layout.layers)
        parents.insert(QString::fromStdString(layer.id), QString::fromStdString(layer.parentId));
    for (const auto &layer : layout.layers) {
        auto parent = QString::fromStdString(layer.parentId);
        QSet<QString> seen;
        int levels = 1;
        while (!parent.isEmpty()) {
            if (!parents.contains(parent) || seen.contains(parent) || parent == QString::fromStdString(layer.id) ||
                ++levels > 8) {
                error = "hierarquia inválida ou cíclica na camada " + QString::fromStdString(layer.id) + ".";
                return false;
            }
            seen.insert(parent);
            parent = parents.value(parent);
        }
    }
    Q_UNUSED(depth);
    return true;
}

bool loadDeclarativeDefinitions(QString &error) {
    initializePremiumManifestResources();
    QDir directory(QStringLiteral(":/studio/premium"));
    const auto files = directory.entryList({QStringLiteral("*.manifest.json")}, QDir::Files, QDir::Name);
    for (const auto &fileName : files) {
        QFile file(directory.filePath(fileName));
        if (!file.open(QIODevice::ReadOnly) || file.size() <= 0 || file.size() > 512 * 1024) {
            error = "Manifesto premium embutido ausente ou acima de 512 KiB: " + fileName;
            return false;
        }
        QJsonParseError parse;
        const auto document = QJsonDocument::fromJson(file.readAll(), &parse);
        if (parse.error != QJsonParseError::NoError || !document.isObject()) {
            error = "Manifesto premium embutido inválido: " + fileName;
            return false;
        }
        const auto manifest = document.object();
        if (manifest.value("manifestVersion").toInt() != 2)
            continue;
        if (const auto invalid = validatePremiumManifest(manifest, {}); !invalid.isEmpty()) {
            error = "Manifesto declarativo inválido '" + manifest.value("id").toString() + "': " + invalid;
            return false;
        }
        const auto definition = manifest.value("definition").toObject();
        if (!manifest.value("definition").isObject() || definition.value("definitionVersion").toInt() != 1) {
            error = "Versão da definição declarativa não suportada: " + manifest.value("id").toString();
            return false;
        }
        auto declarative = std::make_shared<PremiumDeclarativeDefinition>();
        if (!parseLayout(definition, QStringLiteral("landscape"), declarative->landscape, error) ||
            !parseLayout(definition, QStringLiteral("portrait"), declarative->portrait, error))
            return false;
        if (manifest.value("rootNodeId").toString() !=
                QString::fromStdString(declarative->landscape.layers.front().id) ||
            declarative->landscape.layers.size() != declarative->portrait.layers.size() ||
            manifest.value("layers").toArray().size() != int(declarative->landscape.layers.size())) {
            error = "raiz ou quantidade de camadas diverge entre variantes: " + manifest.value("id").toString();
            return false;
        }
        for (std::size_t i = 0; i < declarative->landscape.layers.size(); ++i) {
            const auto &landscape = declarative->landscape.layers[i];
            const auto &portrait = declarative->portrait.layers[i];
            const auto metadata = manifest.value("layers").toArray().at(int(i)).toObject();
            const auto expectedKind = landscape.component == "group" ? "group"
                                    : landscape.component == "text" ? "text" : "shape";
            if (landscape.id != portrait.id || landscape.parentId != portrait.parentId ||
                landscape.component != portrait.component || landscape.role != portrait.role ||
                metadata.value("id").toString().toStdString() != landscape.id ||
                metadata.value("parentId").toString().toStdString() != landscape.parentId ||
                metadata.value("kind").toString().toStdString() != expectedKind ||
                metadata.value("role").toString().toStdString() != landscape.role) {
                error = "metadados de camada divergem da definição: " + QString::fromStdString(landscape.id);
                return false;
            }
        }
        PremiumAssetDefinition asset;
        asset.id = manifest.value("id").toString().toStdString();
        asset.name = manifest.value("name").toString().toStdString();
        asset.type = manifest.value("type").toString().toStdString();
        asset.category = manifest.value("category").toString().toStdString();
        asset.manifestResource = fileName.toStdString();
        asset.declarative = declarative;
        asset.manifestVersion = 2;
        for (const auto &token : manifest.value("palette").toObject().value("tokens").toArray())
            asset.paletteTokens.push_back(token.toString().toStdString());
        QMap<QString, std::vector<std::string>> bindings;
        for (const auto *layout : {&declarative->landscape, &declarative->portrait})
            for (const auto &layer : layout->layers) {
                if (!layer.paletteToken.empty())
                    bindings[QString::fromStdString(layer.paletteToken)].push_back(layer.id);
            }
        for (const auto &slot : manifest.value("slots").toArray())
            asset.slotRoles.push_back(slot.toObject().value("role").toString().toStdString());
        for (auto it = bindings.begin(); it != bindings.end(); ++it) {
            auto ids = it.value();
            std::sort(ids.begin(), ids.end());
            ids.erase(std::unique(ids.begin(), ids.end()), ids.end());
            asset.paletteBindings.push_back({it.key().toStdString(), std::move(ids)});
        }
        if (asset.paletteTokens.size() != asset.paletteBindings.size()) {
            error = "Todo token de paleta declarativo precisa estar ligado a uma camada: " +
                    manifest.value("id").toString();
            return false;
        }
        std::string registrationError;
        if (!registerDeclarativePremium(std::move(asset), registrationError)) {
            error = QString::fromStdString(registrationError);
            return false;
        }
    }
    return true;
}
} // namespace

bool initializePremiumCatalog(QString *error) {
    static std::once_flag once;
    static QString loadError;
    std::call_once(once, [] { loadDeclarativeDefinitions(loadError); });
    if (error)
        *error = loadError;
    return loadError.isEmpty();
}
} // namespace studio
