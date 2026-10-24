#include <QDir>
#include <QFileInfo>
#include <QImageReader>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSet>
#include <QSvgRenderer>
#include <QUrl>
#include <algorithm>
#include <cmath>
#include <functional>
#include <map>
#include <studio/core/library.hpp>
#include <studio/production/recipe.hpp>
#include <studio/storage/project_store.hpp>

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
bool parseTimeRef(const QJsonObject &o, int cueCount, DirectedTime &time, QString &error,
                  const QString &label) {
    if (!allowed(o, {"cue", "progress", "cueProgress", "offsetSeconds"}) || !o["cue"].isDouble() ||
        std::floor(o["cue"].toDouble()) != o["cue"].toDouble()) {
        error = label + " precisa de cue inteiro e campos temporais conhecidos.";
        return false;
    }
    time.cue = o["cue"].toInt();
    if (o.contains("progress") && o.contains("cueProgress")) {
        error = label + " use progress ou cueProgress, não ambos.";
        return false;
    }
    const auto progressField =
        o.contains("cueProgress") ? QStringLiteral("cueProgress") : QStringLiteral("progress");
    if (o.contains(progressField) && !o[progressField].isDouble()) {
        error = label + " progress precisa ser numérico.";
        return false;
    }
    if (o.contains("offsetSeconds") && !o["offsetSeconds"].isDouble()) {
        error = label + " offsetSeconds precisa ser numérico.";
        return false;
    }
    const bool hasProgress = o.contains("progress") || o.contains("cueProgress");
    if (hasProgress && o.contains("offsetSeconds")) {
        error = label + " use progress/cueProgress ou offsetSeconds, não ambos.";
        return false;
    }
    time.progress = hasProgress ? o[progressField].toDouble() : -1;
    time.offsetSeconds = o["offsetSeconds"].toDouble(0);
    if (time.cue < 0 || time.cue >= cueCount || !std::isfinite(time.progress) || time.progress < 0 ||
        time.progress > 1 || !std::isfinite(time.offsetSeconds) || time.offsetSeconds < 0 ||
        time.offsetSeconds > 10) {
        error = label + " está fora dos limites de cue, progress ou offsetSeconds.";
        return false;
    }
    return true;
}
QJsonObject libraryAsset(const QJsonObject &library, const QString &id) {
    for (const auto &value : library["assets"].toArray()) {
        const auto asset = value.toObject();
        if (asset["id"].toString() == id)
            return asset;
    }
    return {};
}
bool parseElement(const QJsonObject &e, const QString &baseDirectory, const QJsonObject &library, int version,
                  int cueCount, QSet<QString> &ids, int &elementCount, DirectedElement &element,
                  QString &error, int depth = 0, DirectedTime defaultStart = {}) {
    auto fail = [&](QString message) {
        error = std::move(message);
        return false;
    };
    if (depth > 8)
        return fail("Grupos podem ter no máximo 8 níveis.");
    QStringList keys{"id",
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
                     "span"};
    if (version >= 2)
        keys << "assetId" << "children" << "keyframes" << "cueProgress";
    if (version >= 3)
        keys << "progress";
    if (!allowed(e, keys))
        return fail("Elemento contém campo desconhecido.");
    if (!e["id"].isString() || !e["kind"].isString())
        return fail("Elemento precisa de id e tipo em texto.");
    const auto id = e["id"].toString();
    if (id.isEmpty() || id.size() > 80 || ids.contains(id))
        return fail("ID de elemento vazio ou duplicado.");
    ids.insert(id);
    if (++elementCount > 200)
        return fail("Produção excede 200 elementos, incluindo grupos.");
    auto &n = element.node;
    n.id = id.toStdString();
    n.name = n.id;
    const auto kind = e["kind"].toString();
    std::vector<Node> presetNodes;
    if (kind == "group") {
        if (version < 2 || !e["children"].isArray() || e["children"].toArray().isEmpty())
            return fail("Grupo exige uma lista não vazia de filhos na receita v2.");
        n.kind = NodeKind::Group;
        n.name = "Grupo";
        n.width = 600;
        n.height = 400;
    } else if (kind == "native") {
        if (!e["asset"].isString())
            return fail("Elemento nativo precisa do id asset.");
        n.kind = NodeKind::Native;
        n.nativeId = e["asset"].toString().toStdString();
        const auto item = libraryItem(n.nativeId);
        if (!item)
            return fail("Objeto nativo desconhecido: " + QString::fromStdString(n.nativeId));
        n.name = item->name;
        n.width = item->width;
        n.height = item->height;
    } else if (kind == "library") {
        if (version < 2 || !e["assetId"].isString())
            return fail("Referência de biblioteca exige assetId e receita v2.");
        const auto assetId = e["assetId"].toString();
        const auto asset = libraryAsset(library, assetId);
        if (asset.isEmpty() || asset["deleted"].toBool())
            return fail("Item pessoal não está disponível na biblioteca: " + assetId);
        const auto assetKind = asset["kind"].toString();
        if (assetKind == "native") {
            n.kind = NodeKind::Native;
            n.nativeId = asset["nativeId"].toString().toStdString();
            const auto item = libraryItem(n.nativeId);
            if (!item)
                return fail("Objeto nativo removido do catálogo: " + assetId);
            n.name = item->name;
            n.width = item->width;
            n.height = item->height;
        } else if (assetKind == "image" || assetKind == "svg") {
            n.kind = assetKind == "svg" ? NodeKind::Svg : NodeKind::Image;
            const auto source = asset["source"].toString();
            if (source.isEmpty() || !QFileInfo::exists(source))
                return fail("Arquivo do item pessoal ausente: " + assetId);
            n.source = source.toStdString();
            n.name = asset["name"].toString().toStdString();
            if (asset["width"].isDouble() && asset["height"].isDouble()) {
                n.width = asset["width"].toDouble();
                n.height = asset["height"].toDouble();
            } else if (assetKind == "svg") {
                QSvgRenderer svg(source);
                if (!svg.isValid() || svg.defaultSize().isEmpty())
                    return fail("SVG pessoal inválido ou sem dimensões: " + assetId);
                n.width = svg.defaultSize().width();
                n.height = svg.defaultSize().height();
            } else {
                QImageReader reader(source);
                const auto size = reader.size();
                if (!reader.canRead() || size.isEmpty())
                    return fail("Imagem pessoal inválida ou sem dimensões: " + assetId);
                n.width = size.width();
                n.height = size.height();
            }
            const double dimensionScale = std::min({1.0, 10000. / n.width, 10000. / n.height});
            n.width *= dimensionScale;
            n.height *= dimensionScale;
        } else if (assetKind == "preset") {
            const auto loaded = deserialize(QJsonDocument(asset["preset"].toObject()).toJson());
            if (!loaded.error.isEmpty() || loaded.project.nodes.empty() ||
                !loaded.project.nodes.front().parentId.empty())
                return fail("Preset da biblioteca inválido: " + assetId);
            presetNodes = loaded.project.nodes;
            for (const auto &part : presetNodes)
                if ((part.kind == NodeKind::Image || part.kind == NodeKind::Svg) &&
                    !QFileInfo::exists(QString::fromStdString(part.source)))
                    return fail("Mídia de preset ausente: " + assetId);
            n = presetNodes.front();
            n.parentId.clear();
            n.name = asset["name"].toString().toStdString();
            // A single legacy preset remains stretched to the scene as before.
            element.presetTiming = presetNodes.size() > 1 || !n.keyframes.empty();
        } else
            return fail("Tipo de item pessoal não suportado na receita: " + assetId);
        n.id = id.toStdString();
    } else if (kind == "text") {
        n.kind = NodeKind::Text;
        if (e.contains("text") && !e["text"].isString())
            return fail("Texto do elemento precisa ser uma string.");
        n.text = e["text"].toString().toStdString();
        n.name = "Texto";
    } else if (kind == "shape") {
        n.kind = NodeKind::Shape;
        n.name = "Forma";
    } else if (kind == "image" || kind == "svg") {
        n.kind = kind == "svg" ? NodeKind::Svg : NodeKind::Image;
        const auto source = e["source"].toString();
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
    for (auto key : {"color", "secondaryColor", "motion", "pose"})
        if (e.contains(key) && !e[key].isString())
            return fail("Cor, movimento ou pose inválidos.");
    if (e.contains("color"))
        n.color = e["color"].toString().toStdString();
    if (e.contains("secondaryColor"))
        n.secondaryColor = e["secondaryColor"].toString().toStdString();
    if (e.contains("motion"))
        n.motion = e["motion"].toString().toStdString();
    if (e.contains("pose"))
        n.pose = e["pose"].toString().toStdString();
    for (auto key : {"cue", "motionFrames", "exitFrames"})
        if (e.contains(key) && (!e[key].isDouble() || std::floor(e[key].toDouble()) != e[key].toDouble()))
            return fail("Cue e durações de movimento devem ser inteiros.");
    if (e.contains("progress") && !e["progress"].isDouble())
        return fail("progress de elemento precisa ser numérico.");
    if (e.contains("progress") && e.contains("cueProgress"))
        return fail("Use progress ou cueProgress no elemento, não ambos.");
    element.cue = e["cue"].toInt(defaultStart.cue);
    element.progress = e["progress"].toDouble(e.contains("cue") ? 0 : std::max(0.0, defaultStart.progress));
    element.offsetSeconds = e["offsetSeconds"].toDouble(e.contains("cue") ? 0 : defaultStart.offsetSeconds);
    element.cueProgress = e["cueProgress"].toDouble(
        e.contains("progress") ? element.progress : (e.contains("cue") ? -1 : defaultStart.progress));
    if ((e.contains("cueProgress") || e.contains("progress")) && e.contains("offsetSeconds"))
        return fail("Use cueProgress/progress ou offsetSeconds no elemento, não ambos.");
    if (e.contains("cueProgress")) {
        if (!number(e, "cueProgress", element.cueProgress) || element.cueProgress < 0 ||
            element.cueProgress > 1 || e.contains("offsetSeconds"))
            return fail("cueProgress deve ser 0..1 e não pode ser combinado com offsetSeconds.");
    }
    n.motionFrames = e["motionFrames"].toInt(24);
    n.exitFrames = e["exitFrames"].toInt(8);
    element.utteranceOnly = e["span"].toString("scene") == "utterance";
    if (!number(e, "offsetSeconds", element.offsetSeconds) || element.offsetSeconds < 0 ||
        element.offsetSeconds > 10 || element.cue < 0 || element.cue >= cueCount ||
        !std::isfinite(element.progress) || element.progress < 0 || element.progress > 1)
        return fail("Cue ou deslocamento de elemento inválido.");
    if (e.contains("span") && e["span"] != "scene" && e["span"] != "utterance")
        return fail("Span deve ser scene ou utterance.");
    if (e.contains("asset") && kind != "native")
        return fail("Campo asset só vale para elementos native.");
    if (e.contains("assetId") && kind != "library")
        return fail("Campo assetId só vale para elementos library.");
    if (e.contains("source") && kind != "image" && kind != "svg")
        return fail("Campo source só vale para imagem ou SVG.");
    if (e.contains("text") && kind != "text")
        return fail("Campo text só vale para elementos de texto.");
    if (e.contains("children") && kind != "group")
        return fail("Somente grupos podem conter filhos.");
    if (e.contains("keyframes")) {
        if (!e["keyframes"].isArray() || e["keyframes"].toArray().size() > 256)
            return fail("Keyframes devem ser uma lista de até 256 itens.");
        for (const auto &keyValue : e["keyframes"].toArray()) {
            if (!keyValue.isObject())
                return fail("Keyframe inválido.");
            const auto key = keyValue.toObject();
            QStringList keyFields{"cue", "offsetSeconds", "cueProgress", "progress", "x",
                                  "y",   "rotation",      "scale",       "opacity",  "easing"};
            if (version >= 3)
                keyFields << "progress";
            if (!allowed(key, keyFields))
                return fail("Keyframe contém campo desconhecido.");
            if (key.contains("cue") &&
                (!key["cue"].isDouble() || std::floor(key["cue"].toDouble()) != key["cue"].toDouble()))
                return fail("Cue de keyframe deve ser inteiro.");
            for (auto field : {"cue", "offsetSeconds", "progress"})
                if (key.contains(field) && !key[field].isDouble())
                    return fail("Referência temporal de keyframe inválida.");
            if (key.contains("progress") && key.contains("cueProgress"))
                return fail("Use progress ou cueProgress no keyframe, não ambos.");
            const auto keyProgress =
                key.contains("cueProgress")
                    ? key["cueProgress"].toDouble(-1)
                    : key["progress"].toDouble(key.contains("cue") ? -1 : element.cueProgress);
            DirectedKeyframe parsed{key["cue"].toInt(element.cue),
                                    key["progress"].toDouble(key.contains("cue") ? 0 : element.progress),
                                    key["offsetSeconds"].toDouble(0),
                                    keyProgress,
                                    key["x"].toDouble(n.x),
                                    key["y"].toDouble(n.y),
                                    key["rotation"].toDouble(n.rotation),
                                    key["scale"].toDouble(n.scale),
                                    key["opacity"].toDouble(n.opacity),
                                    key["easing"].toString("linear").toStdString()};
            if ((key.contains("easing") && !key["easing"].isString()) || !supportedEasing(parsed.easing))
                return fail("Easing de elemento inválido.");
            if ((key.contains("cueProgress") || key.contains("progress")) && key.contains("offsetSeconds"))
                return fail("Use progress/cueProgress ou offsetSeconds no keyframe, não ambos.");
            if (key.contains("cueProgress") &&
                (!number(key, "cueProgress", parsed.cueProgress) || parsed.cueProgress < 0 ||
                 parsed.cueProgress > 1 || key.contains("offsetSeconds")))
                return fail("cueProgress de keyframe inválido ou combinado com offsetSeconds.");
            if (key.contains("progress") && (!number(key, "progress", parsed.cueProgress) ||
                                             parsed.cueProgress < 0 || parsed.cueProgress > 1))
                return fail("progress de keyframe deve estar entre 0 e 1.");
            if (parsed.cue < 0 || parsed.cue >= cueCount || !std::isfinite(parsed.progress) ||
                parsed.progress < 0 || parsed.progress > 1 || !std::isfinite(parsed.offsetSeconds) ||
                parsed.offsetSeconds < 0 || parsed.offsetSeconds > 10)
                return fail("Cue de keyframe inválido.");
            for (auto field : {"x", "y", "rotation", "scale", "opacity"})
                if (key.contains(field) && !key[field].isDouble())
                    return fail("Transformação de keyframe inválida.");
            element.keyframes.push_back(parsed);
        }
    }
    if (kind == "group")
        for (const auto &child : e["children"].toArray()) {
            if (!child.isObject())
                return fail("Filho de grupo inválido.");
            DirectedElement parsedChild;
            if (!parseElement(child.toObject(), baseDirectory, library, version, cueCount, ids, elementCount,
                              parsedChild, error, depth + 1,
                              {element.cue, element.cueProgress, element.offsetSeconds}))
                return false;
            parsedChild.node.parentId = n.id;
            element.children.push_back(std::move(parsedChild));
        }
    if (!presetNodes.empty() && element.presetTiming) {
        if (e.contains("keyframes"))
            return fail("Presets animados usam suas próprias chaves; não combine com keyframes no JSON.");
        std::size_t expanded = 1;
        const auto recipeId = id;
        std::function<bool(DirectedElement &, const std::string &, int)> expand =
            [&](DirectedElement &parent, const std::string &parentId, int level) {
                for (std::size_t i = 1; i < presetNodes.size(); ++i) {
                    const auto &part = presetNodes[i];
                    if (part.parentId != parentId)
                        continue;
                    if (level > 8)
                        return fail("Composição da biblioteca excede 8 níveis de grupos.");
                    DirectedElement child;
                    child.node = part;
                    const auto childId = recipeId + "-preset-" + QString::number(expanded++);
                    if (ids.contains(childId) || ++elementCount > 200)
                        return fail("Composição da biblioteca ultrapassa IDs ou limite de 200 elementos.");
                    ids.insert(childId);
                    child.node.id = childId.toStdString();
                    child.node.parentId = parent.node.id;
                    child.cue = element.cue;
                    child.progress = element.progress;
                    child.offsetSeconds = element.offsetSeconds;
                    child.cueProgress = element.cueProgress;
                    child.utteranceOnly = element.utteranceOnly;
                    child.presetTiming = true;
                    if (!expand(child, part.id, level + 1))
                        return false;
                    parent.children.push_back(std::move(child));
                }
                return true;
            };
        if (!expand(element, presetNodes.front().id, depth + 1) || expanded != presetNodes.size())
            return fail(error.isEmpty() ? "Preset com hierarquia incompleta." : error);
    }
    auto candidate = makeEmpty();
    candidate.duration = std::max<std::int64_t>(candidate.duration, n.start + n.duration);
    candidate.nodes = {n};
    candidate.nodes.front().parentId.clear();
    if (auto validation = validate(candidate); !validation.empty())
        return fail(QString::fromStdString(validation));
    return true;
}
} // namespace
RecipeResult parseRecipe(const QByteArray &bytes, const QString &baseDirectory, const QJsonObject &library) {
    RecipeResult result;
    auto fail = [&](QString message) {
        result.error = std::move(message);
        return result;
    };
    if (bytes.size() > 1024 * 1024)
        return fail("Receita excede 1 MiB.");
    QJsonParseError error;
    auto doc = QJsonDocument::fromJson(bytes, &error);
    if (error.error != QJsonParseError::NoError) {
        const auto prefix = bytes.left(std::clamp<qsizetype>(error.offset, 0, bytes.size()));
        const auto line = prefix.count('\n') + 1;
        const auto lastBreak = prefix.lastIndexOf('\n');
        const auto column = QString::fromUtf8(prefix.mid(lastBreak + 1)).size() + 1;
        return fail(QString("JSON inválido: %1 (linha %2, coluna %3).")
                        .arg(error.errorString())
                        .arg(line)
                        .arg(column));
    }
    if (!doc.isObject())
        return fail("A raiz do JSON deve ser um objeto.");
    const auto o = doc.object();
    if (!o["version"].isDouble() || std::floor(o["version"].toDouble()) != o["version"].toDouble())
        return fail("Versão da receita deve ser inteira.");
    const int version = o["version"].toInt();
    if (o["format"] != "studio-dark-production" || (version < 1 || version > 3) || o["catalogVersion"] != 1)
        return fail("Formato, versão ou catálogo da receita não suportado.");
    QStringList rootKeys{"format",     "version",   "catalogVersion", "title",  "voice",
                         "speechRate", "musicGain", "effectsGain",    "scenes", "sources"};
    if (version >= 2)
        rootKeys << "captions" << "targetDurationSeconds" << "timingMode" << "preserveVoicePitch";
    if (!allowed(o, rootKeys))
        return fail("Receita contém campo desconhecido. Consulte o guia desta versão.");
    ProductionRecipe r;
    r.version = version;
    r.original = o;
    if (o.contains("timingMode") && !o["timingMode"].isString())
        return fail("timingMode deve ser natural ou fit-to-duration.");
    r.timingMode = o["timingMode"].toString("natural");
    if (r.timingMode != "natural" && r.timingMode != "fit-to-duration")
        return fail("timingMode não suportado.");
    if (o.contains("targetDurationSeconds") &&
        (!number(o, "targetDurationSeconds", r.targetDurationSeconds) || r.targetDurationSeconds < 1 ||
         r.targetDurationSeconds > 600))
        return fail("targetDurationSeconds precisa estar entre 1 e 600 segundos.");
    if (o.contains("preserveVoicePitch")) {
        if (!o["preserveVoicePitch"].isBool())
            return fail("preserveVoicePitch deve ser true ou false.");
        r.preserveVoicePitch = o["preserveVoicePitch"].toBool();
    }
    if (!r.preserveVoicePitch)
        return fail("Somente preserveVoicePitch: true é suportado nesta versão.");
    if (r.timingMode == "fit-to-duration" && r.targetDurationSeconds == 0)
        return fail("fit-to-duration exige targetDurationSeconds.");
    if (r.timingMode == "natural" && r.targetDurationSeconds > 0)
        return fail("targetDurationSeconds exige timingMode: fit-to-duration.");
    r.title = o["title"].toString().trimmed();
    if (r.title.isEmpty() || r.title.size() > 120)
        return fail("Título inválido.");
    if (o.contains("voice")) {
        if (!o["voice"].isString())
            return fail("Voz inválida.");
        r.voice = o["voice"].toString();
    }
    if (o.contains("captions")) {
        if (!o["captions"].isBool())
            return fail("captions deve ser true ou false.");
        r.renderCaptions = o["captions"].toBool();
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
    int totalText = 0, totalNodes = 0, totalCameraKeys = 0, totalShots = 0, totalBeats = 0;
    QString elementError;
    for (const auto sceneValue : o["scenes"].toArray()) {
        if (!sceneValue.isObject())
            return fail("Cena inválida.");
        const auto s = sceneValue.toObject();
        QStringList sceneKeys{"id", "narration"};
        if (version < 3)
            sceneKeys << "title" << "kicker" << "background" << "elements";
        else
            sceneKeys << "shots";
        if (version == 2)
            sceneKeys << "camera";
        if (version >= 2)
            sceneKeys << "targetDurationSeconds";
        if (!allowed(s, sceneKeys))
            return fail("Campo de cena desconhecido.");
        if (!s["id"].isString() ||
            (version < 3 && ((s.contains("title") && !s["title"].isString()) ||
                             (s.contains("kicker") && !s["kicker"].isString()) ||
                             (s.contains("background") && !s["background"].isString()))))
            return fail("Identificação, título, kicker ou cenário da cena inválidos.");
        DirectedScene scene;
        scene.id = s["id"].toString();
        scene.title = s["title"].toString();
        scene.kicker = s["kicker"].toString();
        scene.background = s["background"].toString(version == 1 ? "studio" : "");
        if (s.contains("targetDurationSeconds") &&
            (!number(s, "targetDurationSeconds", scene.targetDurationSeconds) ||
             scene.targetDurationSeconds < 0.1 || scene.targetDurationSeconds > 600))
            return fail("Duração-alvo de cena inválida.");
        if (scene.targetDurationSeconds > 0 && r.timingMode != "fit-to-duration")
            return fail("Tempo por cena requer fit-to-duration.");
        if (scene.id.isEmpty() || scene.id.size() > 80 || ids.contains(scene.id) || scene.title.size() > 70 ||
            scene.kicker.size() > 70)
            return fail("Identificação ou texto de cena inválido.");
        ids.insert(scene.id);
        if (!scene.background.isEmpty()) {
            const auto bg = libraryItem(scene.background.toStdString());
            if (!bg || bg->category != "cenários")
                return fail("Cenário desconhecido: " + scene.background);
        }
        if (!s["narration"].isArray() || s["narration"].toArray().isEmpty() ||
            s["narration"].toArray().size() > 10)
            return fail("Cada cena precisa de 1 a 10 trechos de narração.");
        int cue = 0;
        for (auto line : s["narration"].toArray()) {
            QString text;
            double target = 0;
            if (line.isString())
                text = line.toString();
            else if (version >= 2 && line.isObject()) {
                const auto u = line.toObject();
                if (!allowed(u, {"text", "targetDurationSeconds"}) || !u["text"].isString())
                    return fail("Trecho de narração deve conter somente text e targetDurationSeconds.");
                text = u["text"].toString();
                if (u.contains("targetDurationSeconds") &&
                    (!number(u, "targetDurationSeconds", target) || target < .1 || target > 60))
                    return fail("Duração-alvo de fala precisa estar entre 0,1 e 60 segundos.");
                if (target > 0 && r.timingMode != "fit-to-duration")
                    return fail("Duração por fala exige fit-to-duration.");
            } else
                return fail("Narração deve ser texto ou objeto com text.");
            if (text.trimmed().isEmpty() || text.size() > 150)
                return fail("Narração: use trechos com até 150 caracteres.");
            totalText += text.size();
            scene.narration.push_back({scene.id + "-u" + QString::number(++cue), text, target});
        }
        int elementCount = 0;
        if (version == 3) {
            if (!s["shots"].isArray() || s["shots"].toArray().isEmpty() || s["shots"].toArray().size() > 64)
                return fail("Cada cena v3 precisa de 1 a 64 shots visuais.");
            for (const auto shotValue : s["shots"].toArray()) {
                if (!shotValue.isObject())
                    return fail("Shot visual inválido.");
                const auto shotObject = shotValue.toObject();
                if (!allowed(shotObject,
                             {"id", "name", "start", "transitionIn", "camera", "elements", "beats"}))
                    return fail("Shot contém campo desconhecido.");
                if (!shotObject["id"].isString() || !shotObject["start"].isObject() ||
                    !shotObject["elements"].isArray())
                    return fail("Shot precisa de id, start e elements válidos.");
                DirectedShot shot;
                shot.id = shotObject["id"].toString();
                shot.name = shotObject["name"].toString(shot.id);
                shot.transitionIn = shotObject["transitionIn"].toString("cut");
                if (shot.id.isEmpty() || shot.id.size() > 80 || ids.contains(shot.id) ||
                    shot.name.size() > 120)
                    return fail("ID ou nome de shot inválido ou duplicado.");
                ids.insert(shot.id);
                if (shot.transitionIn != "cut")
                    return fail(
                        "Receita v3 suporta cortes secos; transições animadas virão em uma etapa posterior.");
                QString timeError;
                if (!parseTimeRef(shotObject["start"].toObject(), int(scene.narration.size()), shot.start,
                                  timeError, "start do shot"))
                    return fail(timeError);
                if (shotObject["camera"].isObject()) {
                    const auto camera = shotObject["camera"].toObject();
                    if (!allowed(camera, {"x", "y", "zoom", "rotation"}) ||
                        !number(camera, "x", shot.cameraX) || !number(camera, "y", shot.cameraY) ||
                        !number(camera, "zoom", shot.cameraZoom) ||
                        !number(camera, "rotation", shot.cameraRotation) || shot.cameraZoom < .1 ||
                        shot.cameraZoom > 10 || std::abs(shot.cameraX) > 10000 ||
                        std::abs(shot.cameraY) > 10000 || std::abs(shot.cameraRotation) > 360)
                        return fail("Câmera inicial do shot inválida.");
                } else
                    return fail("Cada shot precisa declarar sua câmera inicial.");
                for (const auto elementValue : shotObject["elements"].toArray()) {
                    if (!elementValue.isObject())
                        return fail("Elemento de shot inválido.");
                    DirectedElement element;
                    if (!parseElement(elementValue.toObject(), baseDirectory, library, version,
                                      int(scene.narration.size()), ids, elementCount, element, elementError,
                                      0, shot.start))
                        return fail(elementError);
                    shot.elements.push_back(std::move(element));
                }
                if (shotObject.contains("beats")) {
                    if (!shotObject["beats"].isArray() || shotObject["beats"].toArray().size() > 512)
                        return fail("Visual Beats devem ser uma lista de até 512 eventos por shot.");
                    for (const auto beatValue : shotObject["beats"].toArray()) {
                        if (!beatValue.isObject())
                            return fail("Visual Beat inválido.");
                        const auto beatObject = beatValue.toObject();
                        if (!allowed(beatObject, {"id", "at", "action", "target", "durationFrames", "x", "y",
                                                  "rotation", "scale", "opacity", "zoom"}))
                            return fail("Visual Beat contém campo desconhecido.");
                        DirectedBeat beat;
                        beat.id = beatObject["id"].toString();
                        beat.action = beatObject["action"].toString();
                        beat.targetId = beatObject["target"].toString();
                        if (beat.id.isEmpty() || beat.id.size() > 80 || ids.contains(beat.id) ||
                            !beatObject["at"].isObject() ||
                            !parseTimeRef(beatObject["at"].toObject(), int(scene.narration.size()), beat.at,
                                          timeError, "at do Visual Beat"))
                            return fail(timeError.isEmpty() ? "Visual Beat sem id único ou referência at."
                                                            : timeError);
                        ids.insert(beat.id);
                        if (beat.action != "show" && beat.action != "hide" && beat.action != "transform" &&
                            beat.action != "camera")
                            return fail("Visual Beat aceita action show, hide, transform ou camera.");
                        if ((beat.action == "camera") != beat.targetId.isEmpty())
                            return fail("Visual Beat precisa de target, exceto quando action é camera.");
                        if (beatObject.contains("durationFrames") &&
                            (!beatObject["durationFrames"].isDouble() ||
                             std::floor(beatObject["durationFrames"].toDouble()) !=
                                 beatObject["durationFrames"].toDouble()))
                            return fail("durationFrames do Visual Beat deve ser inteiro.");
                        beat.durationFrames = beatObject["durationFrames"].toInt(0);
                        if (beat.durationFrames < 0 || beat.durationFrames > 300 ||
                            ((beat.action == "show" || beat.action == "hide") && beat.durationFrames != 0))
                            return fail("Duração do Visual Beat fora dos limites.");
                        for (auto [key, target] : {std::pair{"x", &beat.x},
                                                   {"y", &beat.y},
                                                   {"rotation", &beat.rotation},
                                                   {"scale", &beat.scale},
                                                   {"opacity", &beat.opacity},
                                                   {"zoom", &beat.zoom}}) {
                            if (beatObject.contains(key)) {
                                if (!beatObject[key].isDouble() || !std::isfinite(beatObject[key].toDouble()))
                                    return fail("Propriedade numérica do Visual Beat inválida.");
                                *target = beatObject[key].toDouble();
                            }
                        }
                        if ((beat.action == "show" || beat.action == "hide") &&
                            (beat.x || beat.y || beat.rotation || beat.scale || beat.opacity || beat.zoom))
                            return fail("show/hide não aceita propriedades; use action transform.");
                        if (beat.action == "transform" &&
                            !(beat.x || beat.y || beat.rotation || beat.scale || beat.opacity))
                            return fail("transform precisa de ao menos uma propriedade visual.");
                        if (beat.action == "camera" &&
                            (!(beat.x || beat.y || beat.rotation || beat.zoom) || beat.scale || beat.opacity))
                            return fail("camera precisa de x/y/rotation/zoom e não aceita scale/opacity.");
                        if (beat.action != "camera" && beat.zoom)
                            return fail("zoom só pode ser usado em Visual Beat de câmera.");
                        ++totalBeats;
                        shot.beats.push_back(std::move(beat));
                    }
                }
                scene.shots.push_back(std::move(shot));
                if (++totalShots > 256 || totalBeats > 4096)
                    return fail("Produção excede 256 shots ou 4096 Visual Beats.");
            }
            totalNodes += elementCount + (r.renderCaptions ? int(scene.narration.size()) : 0);
            r.scenes.push_back(std::move(scene));
            continue;
        }
        if (!s["elements"].isArray())
            return fail("Lista de elementos ausente.");
        for (const auto elementValue : s["elements"].toArray()) {
            if (!elementValue.isObject())
                return fail("Elemento inválido.");
            DirectedElement element;
            if (!parseElement(elementValue.toObject(), baseDirectory, library, version,
                              int(scene.narration.size()), ids, elementCount, element, elementError))
                return fail(elementError);
            scene.elements.push_back(std::move(element));
        }
        if (s.contains("camera")) {
            if (!s["camera"].isArray() || s["camera"].toArray().size() > 256)
                return fail("Câmera precisa de uma lista de até 256 keyframes por cena.");
            for (const auto &cameraValue : s["camera"].toArray()) {
                if (!cameraValue.isObject())
                    return fail("Keyframe de câmera inválido.");
                const auto c = cameraValue.toObject();
                if (!allowed(c,
                             {"cue", "offsetSeconds", "cueProgress", "x", "y", "zoom", "rotation", "easing"}))
                    return fail("Keyframe de câmera contém campo desconhecido.");
                DirectedCameraKeyframe key;
                if (c.contains("easing") && !c["easing"].isString())
                    return fail("Easing de câmera inválido.");
                key.easing = c["easing"].toString("linear").toStdString();
                if (!supportedEasing(key.easing))
                    return fail("Curva de câmera não suportada.");
                for (auto pair : {std::pair{"x", &key.x},
                                  {"y", &key.y},
                                  {"zoom", &key.zoom},
                                  {"rotation", &key.rotation}})
                    if (!c[pair.first].isDouble() || !std::isfinite(*pair.second = c[pair.first].toDouble()))
                        return fail("Keyframe de câmera incompleto.");
                if (c.contains("cue") &&
                    (!c["cue"].isDouble() || std::floor(c["cue"].toDouble()) != c["cue"].toDouble()))
                    return fail("Cue de câmera deve ser um inteiro.");
                key.cue = c["cue"].toInt(0);
                key.offsetSeconds = c["offsetSeconds"].toDouble(0);
                if (c.contains("cueProgress") &&
                    (!number(c, "cueProgress", key.cueProgress) || key.cueProgress < 0 ||
                     key.cueProgress > 1 || c.contains("offsetSeconds")))
                    return fail("cueProgress de câmera inválido ou combinado com offsetSeconds.");
                if (!number(c, "offsetSeconds", key.offsetSeconds) || key.cue < 0 ||
                    key.cue >= int(scene.narration.size()) || key.offsetSeconds < 0 || key.offsetSeconds > 10)
                    return fail("Cue ou deslocamento de câmera inválido.");
                if (++totalCameraKeys > 256)
                    return fail("Use no máximo 256 keyframes de câmera na produção.");
                scene.camera.push_back(key);
            }
        }
        const int headers = version == 1 ? 2 : int(!scene.title.isEmpty()) + int(!scene.kicker.isEmpty());
        const int background = int(!scene.background.isEmpty());
        totalNodes +=
            elementCount + (r.renderCaptions ? int(scene.narration.size()) : 0) + headers + background;
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
