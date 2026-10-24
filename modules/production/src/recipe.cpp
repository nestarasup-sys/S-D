#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSet>
#include <QUrl>
#include <cmath>
#include <studio/core/library.hpp>
#include <studio/production/recipe.hpp>

namespace studio {
namespace {
bool allowed(const QJsonObject &o, QStringList keys) {
    for (auto it = o.begin(); it != o.end(); ++it)
        if (!keys.contains(it.key()))
            return false;
    return true;
}
bool number(const QJsonObject &o, const QString &key, double &target) {
    if (!o.contains(key))
        return true;
    if (!o[key].isDouble() || !std::isfinite(o[key].toDouble()))
        return false;
    target = o[key].toDouble();
    return true;
}
} // namespace
RecipeResult parseRecipe(const QByteArray &bytes, const QString &baseDirectory) {
    RecipeResult result;
    auto fail = [&](QString message) {
        result.error = std::move(message);
        return result;
    };
    if (bytes.size() > 1024 * 1024)
        return fail("Receita excede 1 MiB.");
    QJsonParseError error;
    auto doc = QJsonDocument::fromJson(bytes, &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject())
        return fail("JSON de produção inválido.");
    const auto o = doc.object();
    if (o["format"] != "studio-dark-production" || o["version"] != 1 || o["catalogVersion"] != 1)
        return fail("Formato, versão ou catálogo da receita não suportado.");
    if (!allowed(o, {"format", "version", "catalogVersion", "title", "voice", "speechRate", "musicGain",
                     "effectsGain", "scenes", "sources"}))
        return fail("Receita contém campo desconhecido. Consulte o guia desta versão.");
    ProductionRecipe r;
    r.original = o;
    r.title = o["title"].toString().trimmed();
    if (r.title.isEmpty() || r.title.size() > 120)
        return fail("Título inválido.");
    if (o.contains("voice")) {
        if (!o["voice"].isString())
            return fail("Voz inválida.");
        r.voice = o["voice"].toString();
    }
    if (o.contains("speechRate")) {
        double rate = o["speechRate"].toDouble(100);
        if (std::floor(rate) != rate || rate < -3 || rate > 3)
            return fail("Velocidade de voz deve ser inteira entre -3 e 3.");
        r.speechRate = int(rate);
    }
    if (!number(o, "musicGain", r.musicGain) || !number(o, "effectsGain", r.effectsGain) || r.musicGain < 0 ||
        r.musicGain > .08 || r.effectsGain < 0 || r.effectsGain > .15)
        return fail("Volume de música ou efeitos fora dos limites.");
    if (!o["scenes"].isArray() || o["scenes"].toArray().isEmpty() || o["scenes"].toArray().size() > 20)
        return fail("Use de 1 a 20 cenas.");
    QSet<QString> ids;
    int totalText = 0, totalNodes = 0;
    for (const auto sceneValue : o["scenes"].toArray()) {
        if (!sceneValue.isObject())
            return fail("Cena inválida.");
        const auto s = sceneValue.toObject();
        if (!allowed(s, {"id", "title", "kicker", "background", "narration", "elements"}))
            return fail("Campo de cena desconhecido.");
        DirectedScene scene;
        scene.id = s["id"].toString();
        scene.title = s["title"].toString();
        scene.kicker = s["kicker"].toString();
        scene.background = s["background"].toString("studio");
        if (scene.id.isEmpty() || scene.id.size() > 80 || ids.contains(scene.id) || scene.title.size() > 70 ||
            scene.kicker.size() > 70)
            return fail("Identificação ou texto de cena inválido.");
        ids.insert(scene.id);
        const auto bg = libraryItem(scene.background.toStdString());
        if (!bg || bg->category != "cenários")
            return fail("Cenário desconhecido: " + scene.background);
        if (!s["narration"].isArray() || s["narration"].toArray().isEmpty() ||
            s["narration"].toArray().size() > 10)
            return fail("Cada cena precisa de 1 a 10 trechos de narração.");
        int cue = 0;
        for (auto line : s["narration"].toArray()) {
            if (!line.isString() || line.toString().trimmed().isEmpty() || line.toString().size() > 150)
                return fail("Narração: use trechos com até 150 caracteres.");
            totalText += line.toString().size();
            scene.narration.push_back({scene.id + "-u" + QString::number(++cue), line.toString()});
        }
        if (!s["elements"].isArray())
            return fail("Lista de elementos ausente.");
        for (const auto elementValue : s["elements"].toArray()) {
            if (!elementValue.isObject())
                return fail("Elemento inválido.");
            const auto e = elementValue.toObject();
            if (!allowed(e, {"id",
                             "kind",
                             "asset",
                             "source",
                             "text",
                             "x",
                             "y",
                             "width",
                             "height",
                             "color",
                             "secondaryColor",
                             "fontSize",
                             "opacity",
                             "rotation",
                             "scale",
                             "travelX",
                             "travelY",
                             "spinDegrees",
                             "motion",
                             "motionFrames",
                             "exitFrames",
                             "pose",
                             "cue",
                             "offsetSeconds",
                             "span"}))
                return fail("Elemento contém campo desconhecido.");
            DirectedElement element;
            auto &n = element.node;
            auto id = e["id"].toString();
            if (id.isEmpty() || id.size() > 80 || ids.contains(id))
                return fail("ID de elemento vazio ou duplicado.");
            ids.insert(id);
            n.id = id.toStdString();
            n.name = n.id;
            auto kind = e["kind"].toString();
            if (kind == "native") {
                n.kind = NodeKind::Native;
                n.nativeId = e["asset"].toString().toStdString();
                auto item = libraryItem(n.nativeId);
                if (!item || item->category == "cenários")
                    return fail("Objeto nativo desconhecido: " + QString::fromStdString(n.nativeId));
                n.name = item->name;
                n.width = item->width;
                n.height = item->height;
            } else if (kind == "text") {
                n.kind = NodeKind::Text;
                n.text = e["text"].toString().toStdString();
                n.name = "Texto";
            } else if (kind == "shape") {
                n.kind = NodeKind::Shape;
                n.name = "Forma";
            } else if (kind == "image" || kind == "svg") {
                n.kind = kind == "svg" ? NodeKind::Svg : NodeKind::Image;
                auto source = e["source"].toString();
                if (source.isEmpty() || source.contains("://") || source.startsWith("//") ||
                    source.startsWith("\\\\"))
                    return fail("Mídia deve ser um arquivo local.");
                n.source = QDir(baseDirectory).absoluteFilePath(source).toStdString();
                n.name = source.toStdString();
            } else
                return fail("Tipo de elemento desconhecido.");
            for (auto pair : {std::pair{"x", &n.x},
                              {"y", &n.y},
                              {"width", &n.width},
                              {"height", &n.height},
                              {"fontSize", &n.fontSize},
                              {"opacity", &n.opacity},
                              {"rotation", &n.rotation},
                              {"scale", &n.scale},
                              {"travelX", &n.travelX},
                              {"travelY", &n.travelY},
                              {"spinDegrees", &n.spinDegrees}})
                if (!number(e, pair.first, *pair.second))
                    return fail("Propriedade numérica inválida.");
            for (auto key : {"text", "color", "secondaryColor", "motion", "pose"})
                if (e.contains(key) && !e[key].isString())
                    return fail("Texto, cor, movimento ou pose inválido.");
            if (e.contains("color"))
                n.color = e["color"].toString().toStdString();
            if (e.contains("secondaryColor"))
                n.secondaryColor = e["secondaryColor"].toString().toStdString();
            if (e.contains("motion"))
                n.motion = e["motion"].toString().toStdString();
            if (e.contains("pose"))
                n.pose = e["pose"].toString().toStdString();
            for (auto key : {"cue", "motionFrames", "exitFrames"})
                if (e.contains(key) &&
                    (!e[key].isDouble() || std::floor(e[key].toDouble()) != e[key].toDouble()))
                    return fail("Cue e durações de movimento devem ser inteiros.");
            element.cue = e["cue"].toInt(0);
            n.motionFrames = e["motionFrames"].toInt(24);
            n.exitFrames = e["exitFrames"].toInt(8);
            element.utteranceOnly = e["span"].toString("scene") == "utterance";
            if (e.contains("span") && e["span"] != "scene" && e["span"] != "utterance")
                return fail("Span deve ser scene ou utterance.");
            if (!number(e, "offsetSeconds", element.offsetSeconds) || element.offsetSeconds < 0 ||
                element.offsetSeconds > 10 || element.cue < 0 || element.cue >= int(scene.narration.size()))
                return fail("Cue ou deslocamento de elemento inválido.");
            auto test = makeEmpty();
            test.nodes = {n};
            if (auto error = validate(test); !error.empty())
                return fail(QString::fromStdString(error));
            scene.elements.push_back(std::move(element));
        }
        totalNodes += int(scene.elements.size() + scene.narration.size() + 3);
        r.scenes.push_back(std::move(scene));
    }
    if (totalText > 12000 || totalNodes > 200)
        return fail("Produção excede os limites de texto ou 200 objetos.");
    if (o.contains("sources")) {
        if (!o["sources"].isArray())
            return fail("Fontes devem ser uma lista.");
        for (auto value : o["sources"].toArray()) {
            auto source = value.toObject();
            QUrl url(source["url"].toString());
            if (!value.isObject() || source["title"].toString().isEmpty() || url.scheme() != "https" ||
                !url.isValid())
                return fail("Fonte precisa de título e URL HTTPS.");
        }
    }
    result.recipe = std::move(r);
    return result;
}
} // namespace studio
