#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLockFile>
#include <QSaveFile>
#include <cmath>
#include <studio/storage/project_store.hpp>

namespace studio {
namespace {
QByteArray hash(const QByteArray &bytes) {
    return QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex();
}
QString text(const std::string &s) {
    return QString::fromStdString(s);
}
std::string str(const QJsonObject &o, const char *key) {
    return o[key].toString().toStdString();
}
bool integer(const QJsonValue &v) {
    auto d = v.toDouble(-1);
    return v.isDouble() && std::isfinite(d) && std::floor(d) == d && std::abs(d) < 1000000000;
}
QJsonObject artDirectionJson(const ArtDirection &art) {
    return {{"backgroundColor", text(art.backgroundColor)},
            {"glowColor", text(art.glowColor)},
            {"primaryColor", text(art.primaryColor)},
            {"secondaryColor", text(art.secondaryColor)},
            {"textColor", text(art.textColor)},
            {"cardColor", text(art.cardColor)},
            {"captionBackgroundColor", text(art.captionBackgroundColor)},
            {"fontFamily", text(art.fontFamily)},
            {"captionPosition", text(art.captionPosition)},
            {"titleSize", art.titleSize},
            {"bodySize", art.bodySize},
            {"captionSize", art.captionSize},
            {"safeMargin", art.safeMargin}};
}
bool writeAtomic(const QString &path, const QByteArray &bytes, QString &error) {
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit()) {
        error = "Não foi possível gravar: " + path + ". " + file.errorString();
        return false;
    }
    return true;
}
} // namespace
QByteArray serialize(const Project &p) {
    QJsonArray nodes;
    for (const auto &n : p.nodes) {
        QJsonArray keyframes;
        for (const auto &key : n.keyframes)
            keyframes.append(QJsonObject{{"frame", double(key.frame)},
                                         {"x", key.x},
                                         {"y", key.y},
                                         {"rotation", key.rotation},
                                         {"scale", key.scale},
                                         {"opacity", key.opacity},
                                         {"easing", text(key.easing)}});
        nodes.append(QJsonObject{{"id", text(n.id)},
                                 {"parentId", text(n.parentId)},
                                 {"shotId", text(n.shotId)},
                                 {"name", text(n.name)},
                                 {"kind", n.kind == NodeKind::Group    ? "group"
                                          : n.kind == NodeKind::Text   ? "text"
                                          : n.kind == NodeKind::Shape  ? "shape"
                                          : n.kind == NodeKind::Native ? "native"
                                          : n.kind == NodeKind::Svg    ? "svg"
                                                                       : "image"},
                                 {"text", text(n.text)},
                                 {"color", text(n.color)},
                                 {"source", text(n.source)},
                                 {"start", double(n.start)},
                                 {"duration", double(n.duration)},
                                 {"x", n.x},
                                 {"y", n.y},
                                 {"width", n.width},
                                 {"height", n.height},
                                 {"opacity", n.opacity},
                                 {"fontSize", n.fontSize},
                                 {"animated", n.animated},
                                 {"nativeId", text(n.nativeId)},
                                 {"sceneId", text(n.sceneId)},
                                 {"role", text(n.role)},
                                 {"pose", text(n.pose)},
                                 {"motion", text(n.motion)},
                                 {"secondaryColor", text(n.secondaryColor)},
                                 {"rotation", n.rotation},
                                 {"scale", n.scale},
                                 {"travelX", n.travelX},
                                 {"travelY", n.travelY},
                                 {"spinDegrees", n.spinDegrees},
                                 {"motionFrames", n.motionFrames},
                                 {"exitFrames", n.exitFrames},
                                 {"keyframes", keyframes}});
    }
    QJsonArray scenes;
    for (const auto &s : p.scenes)
        scenes.append(QJsonObject{{"id", text(s.id)},
                                  {"name", text(s.name)},
                                  {"narration", text(s.narration)},
                                  {"start", double(s.start)},
                                  {"duration", double(s.duration)}});
    QJsonArray camera;
    for (const auto &key : p.camera)
        camera.append(QJsonObject{{"frame", double(key.frame)},
                                  {"x", key.x},
                                  {"y", key.y},
                                  {"zoom", key.zoom},
                                  {"rotation", key.rotation},
                                  {"easing", text(key.easing)},
                                  {"shotId", text(key.shotId)}});
    QJsonObject shots;
    QJsonArray shotArray;
    for (const auto &shot : p.shots)
        shotArray.append(QJsonObject{{"id", text(shot.id)},
                                     {"sceneId", text(shot.sceneId)},
                                     {"name", text(shot.name)},
                                     {"transitionIn", text(shot.transitionIn)},
                                     {"transitionDurationFrames", shot.transitionDurationFrames},
                                     {"transitionDirection", text(shot.transitionDirection)},
                                     {"start", double(shot.start)},
                                     {"duration", double(shot.duration)}});
    QJsonArray beats;
    for (const auto &beat : p.visualBeats) {
        QJsonObject value{{"id", text(beat.id)},
                          {"shotId", text(beat.shotId)},
                          {"targetId", text(beat.targetId)},
                          {"action", text(beat.action)},
                          {"frame", double(beat.frame)},
                          {"durationFrames", double(beat.durationFrames)}};
        if (beat.x)
            value["x"] = *beat.x;
        if (beat.y)
            value["y"] = *beat.y;
        if (beat.rotation)
            value["rotation"] = *beat.rotation;
        if (beat.scale)
            value["scale"] = *beat.scale;
        if (beat.opacity)
            value["opacity"] = *beat.opacity;
        if (beat.zoom)
            value["zoom"] = *beat.zoom;
        beats.append(value);
    }
    QJsonObject document{
        {"format", "studio-dark"},
        {"schemaVersion", p.schemaVersion},
        {"id", text(p.id)},
        {"name", text(p.name)},
        {"width", p.width},
        {"height", p.height},
        {"fps", QJsonObject{{"numerator", p.fps.numerator}, {"denominator", p.fps.denominator}}},
        {"artDirection", artDirectionJson(p.artDirection)},
        {"duration", double(p.duration)},
        {"nodes", nodes},
        {"camera", camera},
        {"scenes", scenes},
        {"audioSource", text(p.audioSource)}};
    if (p.schemaVersion >= 4) {
        document["shots"] = shotArray;
        document["visualBeats"] = beats;
    }
    return QJsonDocument(document).toJson();
}
LoadResult deserialize(const QByteArray &bytes) {
    LoadResult result;
    QJsonParseError parse;
    auto document = QJsonDocument::fromJson(bytes, &parse);
    auto fail = [&](QString message) {
        result.error = std::move(message);
        return result;
    };
    if (parse.error != QJsonParseError::NoError || !document.isObject())
        return fail("Arquivo de projeto inválido.");
    auto o = document.object();
    if (o["format"] != "studio-dark" || (o["schemaVersion"] != 1 && o["schemaVersion"] != 2 &&
                                         o["schemaVersion"] != 3 && o["schemaVersion"] != 4))
        return fail("Formato ou versão não suportada; original preservado.");
    for (auto key : {"width", "height", "duration"})
        if (!integer(o[key]))
            return fail("Dimensão ou duração inválida.");
    auto fps = o["fps"].toObject();
    if (!integer(fps["numerator"]) || !integer(fps["denominator"]) || !o["nodes"].isArray())
        return fail("Sequência inválida.");
    Project p;
    p.schemaVersion = o["schemaVersion"] == 1 ? 2 : o["schemaVersion"].toInt();
    p.id = str(o, "id");
    p.name = str(o, "name");
    p.width = o["width"].toInt();
    p.height = o["height"].toInt();
    p.duration = o["duration"].toInteger();
    p.fps = {fps["numerator"].toInt(), fps["denominator"].toInt()};
    if (o.contains("artDirection")) {
        if (!o["artDirection"].isObject())
            return fail("Direção de arte deve ser um objeto.");
        const auto style = o["artDirection"].toObject();
        const QStringList stringFields{"backgroundColor", "glowColor", "primaryColor", "secondaryColor",
                                       "textColor", "cardColor", "captionBackgroundColor", "fontFamily",
                                       "captionPosition"};
        for (const auto &field : stringFields)
            if (style.contains(field) && !style[field].isString())
                return fail("Campo textual da direção de arte inválido.");
        for (auto field : {"titleSize", "bodySize", "captionSize"})
            if (style.contains(field) && !style[field].isDouble())
                return fail("Tamanho tipográfico da direção de arte inválido.");
        if (style.contains("safeMargin") && !integer(style["safeMargin"]))
            return fail("Margem de segurança da direção de arte inválida.");
        auto assignText = [&](const char *field, std::string &destination) {
            if (style.contains(field))
                destination = str(style, field);
        };
        assignText("backgroundColor", p.artDirection.backgroundColor);
        assignText("glowColor", p.artDirection.glowColor);
        assignText("primaryColor", p.artDirection.primaryColor);
        assignText("secondaryColor", p.artDirection.secondaryColor);
        assignText("textColor", p.artDirection.textColor);
        assignText("cardColor", p.artDirection.cardColor);
        assignText("captionBackgroundColor", p.artDirection.captionBackgroundColor);
        assignText("fontFamily", p.artDirection.fontFamily);
        assignText("captionPosition", p.artDirection.captionPosition);
        p.artDirection.titleSize = style["titleSize"].toDouble(p.artDirection.titleSize);
        p.artDirection.bodySize = style["bodySize"].toDouble(p.artDirection.bodySize);
        p.artDirection.captionSize = style["captionSize"].toDouble(p.artDirection.captionSize);
        p.artDirection.safeMargin = style["safeMargin"].toInt(p.artDirection.safeMargin);
    }
    if (o.contains("audioSource") && !o["audioSource"].isString())
        return fail("Referência de áudio inválida.");
    p.audioSource = str(o, "audioSource");
    if (o.contains("scenes") && !o["scenes"].isArray())
        return fail("Lista de cenas inválida.");
    for (auto value : o["scenes"].toArray()) {
        auto s = value.toObject();
        if (!integer(s["start"]) || !integer(s["duration"]))
            return fail("Tempo de cena inválido.");
        p.scenes.push_back({str(s, "id"), str(s, "name"), str(s, "narration"), s["start"].toInteger(),
                            s["duration"].toInteger()});
    }
    if (o["nodes"].toArray().size() > 200)
        return fail("Máximo de 200 objetos neste protótipo.");
    for (auto value : o["nodes"].toArray()) {
        if (!value.isObject())
            return fail("Objeto inválido.");
        auto v = value.toObject();
        Node n;
        auto kind = v["kind"].toString();
        if (kind != "text" && kind != "shape" && kind != "image" && kind != "native" && kind != "svg" &&
            kind != "group")
            return fail("Tipo de objeto não suportado.");
        for (auto key : {"start", "duration"})
            if (!integer(v[key]))
                return fail("Tempo de objeto inválido.");
        for (auto key : {"x", "y", "width", "height", "opacity", "fontSize"})
            if (!v[key].isDouble())
                return fail("Propriedade visual ausente ou inválida.");
        if (!v["animated"].isBool())
            return fail("Animação inválida.");
        n.kind = kind == "group"    ? NodeKind::Group
                 : kind == "text"   ? NodeKind::Text
                 : kind == "shape"  ? NodeKind::Shape
                 : kind == "native" ? NodeKind::Native
                 : kind == "svg"    ? NodeKind::Svg
                                    : NodeKind::Image;
        n.id = str(v, "id");
        if (v.contains("parentId") && !v["parentId"].isString())
            return fail("Grupo pai inválido.");
        n.parentId = str(v, "parentId");
        if (v.contains("shotId") && !v["shotId"].isString())
            return fail("Referência de shot inválida.");
        n.shotId = str(v, "shotId");
        n.name = str(v, "name");
        n.text = str(v, "text");
        n.color = str(v, "color");
        n.source = str(v, "source");
        n.start = v["start"].toInteger();
        n.duration = v["duration"].toInteger();
        n.x = v["x"].toDouble();
        n.y = v["y"].toDouble();
        n.width = v["width"].toDouble();
        n.height = v["height"].toDouble();
        n.opacity = v["opacity"].toDouble();
        n.fontSize = v["fontSize"].toDouble();
        n.animated = v["animated"].toBool();
        for (auto key : {"nativeId", "sceneId", "role", "pose", "motion", "secondaryColor"})
            if (v.contains(key) && !v[key].isString())
                return fail("Propriedade de objeto inválida.");
        n.nativeId = str(v, "nativeId");
        n.sceneId = str(v, "sceneId");
        n.role = str(v, "role");
        if (v.contains("pose"))
            n.pose = str(v, "pose");
        if (v.contains("motion"))
            n.motion = str(v, "motion");
        if (v.contains("secondaryColor"))
            n.secondaryColor = str(v, "secondaryColor");
        for (auto key : {"rotation", "scale", "travelX", "travelY", "spinDegrees"})
            if (v.contains(key) && !v[key].isDouble())
                return fail("Animação inválida.");
        n.rotation = v["rotation"].toDouble(0);
        n.scale = v["scale"].toDouble(1);
        n.travelX = v["travelX"].toDouble(0);
        n.travelY = v["travelY"].toDouble(0);
        n.spinDegrees = v["spinDegrees"].toDouble(0);
        for (auto key : {"motionFrames", "exitFrames"})
            if (v.contains(key) && !integer(v[key]))
                return fail("Duração da animação inválida.");
        n.motionFrames = v["motionFrames"].toInt(24);
        n.exitFrames = v["exitFrames"].toInt(0);
        if (v.contains("keyframes") && !v["keyframes"].isArray())
            return fail("Lista de keyframes inválida.");
        for (auto keyValue : v["keyframes"].toArray()) {
            if (!keyValue.isObject())
                return fail("Keyframe inválido.");
            auto key = keyValue.toObject();
            if (!integer(key["frame"]))
                return fail("Tempo de keyframe inválido.");
            for (auto field : {"x", "y", "rotation", "scale", "opacity"})
                if (!key[field].isDouble())
                    return fail("Transformação de keyframe incompleta.");
            if (key.contains("easing") && !key["easing"].isString())
                return fail("Curva de keyframe inválida.");
            n.keyframes.push_back({key["frame"].toInteger(), key["x"].toDouble(), key["y"].toDouble(),
                                   key["rotation"].toDouble(), key["scale"].toDouble(),
                                   key["opacity"].toDouble(),
                                   key["easing"].toString("linear").toStdString()});
        }
        p.nodes.push_back(n);
    }
    if (o.contains("camera") && !o["camera"].isArray())
        return fail("Lista de keyframes da câmera inválida.");
    for (auto keyValue : o["camera"].toArray()) {
        if (!keyValue.isObject())
            return fail("Keyframe de câmera inválido.");
        auto key = keyValue.toObject();
        if (!integer(key["frame"]))
            return fail("Tempo de câmera inválido.");
        for (auto field : {"x", "y", "zoom", "rotation"})
            if (!key[field].isDouble())
                return fail("Transformação de câmera incompleta.");
        if (key.contains("easing") && !key["easing"].isString())
            return fail("Curva de câmera inválida.");
        if (key.contains("shotId") && !key["shotId"].isString())
            return fail("Vínculo de câmera com shot inválido.");
        p.camera.push_back({key["frame"].toInteger(), key["x"].toDouble(), key["y"].toDouble(),
                            key["zoom"].toDouble(), key["rotation"].toDouble(),
                            key["easing"].toString("linear").toStdString(), str(key, "shotId")});
    }
    if (o.contains("shots") && !o["shots"].isArray())
        return fail("Lista de shots inválida.");
    for (const auto value : o["shots"].toArray()) {
        if (!value.isObject())
            return fail("Shot inválido.");
        const auto shot = value.toObject();
        if (!integer(shot["start"]) || !integer(shot["duration"]))
            return fail("Tempo de shot inválido.");
        if (shot.contains("transitionDurationFrames") && !integer(shot["transitionDurationFrames"]))
            return fail("Duração da transição inválida.");
        if (shot.contains("transitionDirection") && !shot["transitionDirection"].isString())
            return fail("Direção da transição inválida.");
        p.shots.push_back({str(shot, "id"), str(shot, "sceneId"), str(shot, "name"),
                           str(shot, "transitionIn"), shot["start"].toInteger(), shot["duration"].toInteger(),
                           shot["transitionDurationFrames"].toInt(0),
                           shot.contains("transitionDirection")
                               ? shot["transitionDirection"].toString().toStdString()
                               : "left"});
    }
    if (o.contains("visualBeats") && !o["visualBeats"].isArray())
        return fail("Lista de Visual Beats inválida.");
    for (const auto value : o["visualBeats"].toArray()) {
        if (!value.isObject())
            return fail("Visual Beat inválido.");
        const auto beatObject = value.toObject();
        if (!integer(beatObject["frame"]) || !integer(beatObject["durationFrames"]))
            return fail("Tempo de Visual Beat inválido.");
        for (const auto *key : {"id", "shotId", "targetId", "action"})
            if (!beatObject[key].isString())
                return fail("Identificação de Visual Beat inválida.");
        VisualBeat beat;
        beat.id = str(beatObject, "id");
        beat.shotId = str(beatObject, "shotId");
        beat.targetId = str(beatObject, "targetId");
        beat.action = str(beatObject, "action");
        beat.frame = beatObject["frame"].toInteger();
        beat.durationFrames = beatObject["durationFrames"].toInteger();
        for (auto [key, target] : {std::pair{"x", &beat.x},
                                   {"y", &beat.y},
                                   {"rotation", &beat.rotation},
                                   {"scale", &beat.scale},
                                   {"opacity", &beat.opacity},
                                   {"zoom", &beat.zoom}}) {
            if (beatObject.contains(key) && !beatObject[key].isDouble())
                return fail("Propriedade de Visual Beat inválida.");
            if (beatObject.contains(key))
                *target = beatObject[key].toDouble();
        }
        p.visualBeats.push_back(std::move(beat));
    }
    if (auto error = validate(p); !error.empty())
        return fail(text(error));
    result.project = std::move(p);
    result.revision = hash(bytes);
    return result;
}
LoadResult loadProject(const QString &path) {
    QFile file(path);
    LoadResult r;
    if (!file.open(QIODevice::ReadOnly)) {
        r.error = "Não foi possível abrir: " + file.errorString();
        return r;
    }
    if (file.size() > 4 * 1024 * 1024) {
        r.error = "Arquivo excede 4 MiB.";
        return r;
    }
    return deserialize(file.readAll());
}
SaveResult saveProject(const QString &path, const Project &p, const QByteArray &expected) {
    SaveResult r;
    if (auto error = validate(p); !error.empty()) {
        r.error = text(error);
        return r;
    }
    QLockFile lock(path + ".lock");
    if (!lock.tryLock(0)) {
        r.error = "Projeto está sendo salvo por outro processo.";
        return r;
    }
    if (QFile::exists(path)) {
        QFile existing(path);
        if (!existing.open(QIODevice::ReadOnly)) {
            r.error = existing.errorString();
            return r;
        }
        if (existing.size() > 4 * 1024 * 1024) {
            r.error = "Arquivo existente excede o limite; preservado.";
            return r;
        }
        auto old = existing.readAll();
        existing.close();
        if (expected.isEmpty() || hash(old) != expected) {
            r.error = "O arquivo mudou no disco. Abra a versão atual ou salve uma nova cópia.";
            return r;
        }
        if (!writeAtomic(path + ".previous", old, r.error))
            return r;
    } else if (!expected.isEmpty()) {
        r.error = "O arquivo original foi removido. Salve uma nova cópia.";
        return r;
    }
    auto bytes = serialize(p);
    if (writeAtomic(path, bytes, r.error))
        r.revision = hash(bytes);
    return r;
}
} // namespace studio
