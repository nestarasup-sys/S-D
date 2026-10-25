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
bool readArtDirection(const QJsonObject &style, ArtDirection &art) {
    const QStringList fields{"backgroundColor", "glowColor", "primaryColor", "secondaryColor", "textColor",
                             "cardColor", "captionBackgroundColor", "fontFamily", "captionPosition",
                             "titleSize", "bodySize", "captionSize", "safeMargin"};
    for (auto it = style.begin(); it != style.end(); ++it)
        if (!fields.contains(it.key()))
            return false;
    for (const auto &field : {"backgroundColor", "glowColor", "primaryColor", "secondaryColor", "textColor",
                              "cardColor", "captionBackgroundColor", "fontFamily", "captionPosition"}) {
        if (style.contains(QLatin1String(field)) && !style[QLatin1String(field)].isString())
            return false;
    }
    for (const auto &field : {"titleSize", "bodySize", "captionSize"})
        if (style.contains(QLatin1String(field)) && !style[QLatin1String(field)].isDouble())
            return false;
    if (style.contains("safeMargin") && !integer(style["safeMargin"]))
        return false;
    auto assign = [&](const char *field, std::string &out) {
        if (style.contains(QLatin1String(field)))
            out = style[QLatin1String(field)].toString().toStdString();
    };
    assign("backgroundColor", art.backgroundColor);
    assign("glowColor", art.glowColor);
    assign("primaryColor", art.primaryColor);
    assign("secondaryColor", art.secondaryColor);
    assign("textColor", art.textColor);
    assign("cardColor", art.cardColor);
    assign("captionBackgroundColor", art.captionBackgroundColor);
    assign("fontFamily", art.fontFamily);
    assign("captionPosition", art.captionPosition);
    art.titleSize = style["titleSize"].toDouble(art.titleSize);
    art.bodySize = style["bodySize"].toDouble(art.bodySize);
    art.captionSize = style["captionSize"].toDouble(art.captionSize);
    art.safeMargin = style["safeMargin"].toInt(art.safeMargin);
    return true;
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
        QJsonArray captionWords;
        for (const auto &word : n.captionWords)
            captionWords.append(QJsonObject{{"text", text(word.text)}, {"start", double(word.start)},
                                            {"end", double(word.end)}});
        QJsonObject serializedNode{{"id", text(n.id)},
                                 {"parentId", text(n.parentId)},
                                 {"shotId", text(n.shotId)},
                                 {"name", text(n.name)},
                                 {"kind", n.kind == NodeKind::Group    ? "group"
                                          : n.kind == NodeKind::Text   ? "text"
                                          : n.kind == NodeKind::Shape  ? "shape"
                                          : n.kind == NodeKind::Native ? "native"
                                          : n.kind == NodeKind::Svg    ? "svg"
                                          : n.kind == NodeKind::Video  ? "video"
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
                                 {"keyframes", keyframes},
                                 {"captionWords", captionWords}};
        const bool hasWordAnimation = n.wordAnimationPreset != "highlight" ||
                                      n.wordAnimationDurationFrames != 8 ||
                                      std::abs(n.wordAnimationStartScale - 0.82) > 1e-9 ||
                                      std::abs(n.wordAnimationTravelY - 12) > 1e-9;
        if (p.schemaVersion >= 12 && n.role == "caption" && hasWordAnimation)
            serializedNode["wordAnimation"] = QJsonObject{{"preset", text(n.wordAnimationPreset)},
                                                          {"durationFrames", n.wordAnimationDurationFrames},
                                                          {"startScale", n.wordAnimationStartScale},
                                                          {"travelY", n.wordAnimationTravelY}};
        if (p.schemaVersion >= 10 && n.kind == NodeKind::Video)
            serializedNode["video"] = QJsonObject{{"sourceInSeconds", n.videoSourceInSeconds},
                                                  {"speed", n.videoPlaybackRate},
                                                  {"crop", QJsonObject{{"x", n.videoCropX},
                                                                        {"y", n.videoCropY},
                                                                        {"width", n.videoCropWidth},
                                                                        {"height", n.videoCropHeight}}}};
        nodes.append(serializedNode);
    }
    QJsonArray scenes;
    for (const auto &s : p.scenes) {
        QJsonObject scene{{"id", text(s.id)},
                                  {"name", text(s.name)},
                                  {"narration", text(s.narration)},
                                  {"start", double(s.start)},
                                  {"duration", double(s.duration)}};
        if (p.schemaVersion >= 11 && s.artDirection)
            scene["artDirection"] = artDirectionJson(*s.artDirection);
        scenes.append(scene);
    }
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
    for (const auto &shot : p.shots) {
        QJsonObject value{{"id", text(shot.id)},
                                     {"sceneId", text(shot.sceneId)},
                                     {"name", text(shot.name)},
                                     {"transitionIn", text(shot.transitionIn)},
                                     {"transitionDurationFrames", shot.transitionDurationFrames},
                                     {"transitionDirection", text(shot.transitionDirection)},
                                     {"start", double(shot.start)},
                                     {"duration", double(shot.duration)}};
        if (p.schemaVersion >= 11 && shot.artDirection)
            value["artDirection"] = artDirectionJson(*shot.artDirection);
        shotArray.append(value);
    }
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
    QJsonArray audioTracks;
    for (const auto &track : p.audioTracks) {
        QJsonArray automation;
        for (const auto &point : track.gainAutomation)
            automation.append(QJsonObject{{"frame", double(point.frame)}, {"value", point.value},
                                          {"easing", text(point.easing)}});
        QJsonObject trackJson{{"id", text(track.id)},
                              {"name", text(track.name)},
                              {"source", text(track.source)},
                              {"start", double(track.start)},
                              {"duration", double(track.duration)},
                              {"gain", track.gain},
                              {"muted", track.muted},
                              {"duckUnderVoice", track.duckUnderVoice},
                              {"duckAmount", track.duckAmount},
                              {"fadeInFrames", double(track.fadeInFrames)},
                              {"fadeOutFrames", double(track.fadeOutFrames)},
                              {"gainAutomation", automation}};
        if (p.schemaVersion >= 13) {
            trackJson["sourceInSeconds"] = track.sourceInSeconds;
            trackJson["playbackRate"] = track.playbackRate;
            if (!track.linkedVideoNodeId.empty())
                trackJson["linkedVideoNodeId"] = text(track.linkedVideoNodeId);
        }
        audioTracks.append(trackJson);
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
        {"audioSource", text(p.audioSource)},
        {"voiceDuckingSource", text(p.voiceDuckingSource)},
        {"audioTracks", audioTracks}};
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
                                         o["schemaVersion"] != 3 && o["schemaVersion"] != 4 &&
                                         o["schemaVersion"] != 5 && o["schemaVersion"] != 6 &&
                                         o["schemaVersion"] != 7 && o["schemaVersion"] != 8 &&
                                         o["schemaVersion"] != 9 && o["schemaVersion"] != 10 &&
                                         o["schemaVersion"] != 11 && o["schemaVersion"] != 12 &&
                                         o["schemaVersion"] != 13))
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
        if (!readArtDirection(o["artDirection"].toObject(), p.artDirection))
            return fail("Direção de arte contém campos inválidos ou desconhecidos.");
    }
    if (o.contains("audioSource") && !o["audioSource"].isString())
        return fail("Referência de áudio inválida.");
    p.audioSource = str(o, "audioSource");
    if (o.contains("voiceDuckingSource") && !o["voiceDuckingSource"].isString())
        return fail("Referência de áudio de ducking inválida.");
    p.voiceDuckingSource = str(o, "voiceDuckingSource");
    if (o.contains("audioTracks")) {
        if (!o["audioTracks"].isArray() || o["audioTracks"].toArray().size() > 32)
            return fail("Lista de pistas de áudio inválida.");
        for (const auto &value : o["audioTracks"].toArray()) {
            if (!value.isObject())
                return fail("Pista de áudio inválida.");
            const auto track = value.toObject();
            for (const auto *field : {"start", "duration"})
                if (!integer(track[field]))
                    return fail("Tempo da pista de áudio inválido.");
            for (const auto *field : {"fadeInFrames", "fadeOutFrames"})
                if (track.contains(field) && !integer(track[field]))
                    return fail("Duração de fade da pista de áudio inválida.");
            for (const auto *field : {"gain", "duckAmount"})
                if (!track[field].isDouble())
                    return fail("Volume de pista de áudio inválido.");
            if (!track["id"].isString() || !track["name"].isString() || !track["source"].isString() ||
                (track.contains("muted") && !track["muted"].isBool()) ||
                (track.contains("duckUnderVoice") && !track["duckUnderVoice"].isBool()))
                return fail("Metadados da pista de áudio inválidos.");
            if ((track.contains("sourceInSeconds") || track.contains("playbackRate")) && p.schemaVersion < 13)
                return fail("Trim e velocidade da fonte de áudio exigem schema 13.");
            if (track.contains("linkedVideoNodeId") &&
                (p.schemaVersion < 13 || !track["linkedVideoNodeId"].isString()))
                return fail("Vínculo com camada de vídeo exige texto e schema 13.");
            if ((track.contains("sourceInSeconds") && !track["sourceInSeconds"].isDouble()) ||
                (track.contains("playbackRate") && !track["playbackRate"].isDouble()))
                return fail("Trim e velocidade da fonte de áudio devem ser numéricos.");
            p.audioTracks.push_back({str(track, "id"), str(track, "name"), str(track, "source"),
                                     track["start"].toInteger(), track["duration"].toInteger(),
                                     track["gain"].toDouble(), track["muted"].toBool(),
                                     track["duckUnderVoice"].toBool(), track["duckAmount"].toDouble(.65),
                                     track["fadeInFrames"].toInteger(), track["fadeOutFrames"].toInteger()});
            auto &loadedTrack = p.audioTracks.back();
            loadedTrack.sourceInSeconds = track["sourceInSeconds"].toDouble(0);
            loadedTrack.playbackRate = track["playbackRate"].toDouble(1);
            loadedTrack.linkedVideoNodeId = str(track, "linkedVideoNodeId");
            if (track.contains("gainAutomation") && !track["gainAutomation"].isArray())
                return fail("Lista de automação de ganho inválida.");
            for (const auto pointValue : track["gainAutomation"].toArray()) {
                if (!pointValue.isObject())
                    return fail("Ponto de automação de ganho inválido.");
                const auto point = pointValue.toObject();
                if (!integer(point["frame"]) || !point["value"].isDouble() ||
                    (point.contains("easing") && !point["easing"].isString()))
                    return fail("Ponto de automação de ganho incompleto.");
                loadedTrack.gainAutomation.push_back({point["frame"].toInteger(), point["value"].toDouble(),
                                                       point["easing"].toString("linear").toStdString()});
            }
        }
    }
    if (o.contains("scenes") && !o["scenes"].isArray())
        return fail("Lista de cenas inválida.");
    for (auto value : o["scenes"].toArray()) {
        auto s = value.toObject();
        if (!integer(s["start"]) || !integer(s["duration"]))
            return fail("Tempo de cena inválido.");
        SceneMarker marker{str(s, "id"), str(s, "name"), str(s, "narration"), s["start"].toInteger(),
                           s["duration"].toInteger()};
        if (s.contains("artDirection")) {
            if (p.schemaVersion < 11 || !s["artDirection"].isObject())
                return fail("Direção de arte local exige cena e schema 11.");
            ArtDirection style = p.artDirection;
            if (!readArtDirection(s["artDirection"].toObject(), style))
                return fail("Direção de arte da cena contém campos inválidos ou desconhecidos.");
            marker.artDirection = std::move(style);
        }
        p.scenes.push_back(std::move(marker));
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
            kind != "video" && kind != "group")
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
                 : kind == "video"  ? NodeKind::Video
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
        if (v.contains("video")) {
            if (p.schemaVersion < 10 || n.kind != NodeKind::Video || !v["video"].isObject())
                return fail("Controles de vídeo exigem vídeo e schema 10.");
            const auto video = v["video"].toObject();
            if (!video["sourceInSeconds"].isDouble() || !video["speed"].isDouble() ||
                !video["crop"].isObject())
                return fail("Controles de vídeo incompletos.");
            const auto crop = video["crop"].toObject();
            for (const auto *key : {"x", "y", "width", "height"})
                if (!crop[key].isDouble())
                    return fail("Recorte de vídeo incompleto.");
            n.videoSourceInSeconds = video["sourceInSeconds"].toDouble();
            n.videoPlaybackRate = video["speed"].toDouble();
            n.videoCropX = crop["x"].toDouble();
            n.videoCropY = crop["y"].toDouble();
            n.videoCropWidth = crop["width"].toDouble();
            n.videoCropHeight = crop["height"].toDouble();
        }
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
        if (v.contains("captionWords") && !v["captionWords"].isArray())
            return fail("Lista de tempos de legenda inválida.");
        for (auto wordValue : v["captionWords"].toArray()) {
            if (!wordValue.isObject())
                return fail("Tempo de palavra da legenda inválido.");
            const auto word = wordValue.toObject();
            if (!word["text"].isString() || !integer(word["start"]) || !integer(word["end"]))
                return fail("Tempo de palavra da legenda incompleto.");
            n.captionWords.push_back({word["text"].toString().toStdString(), word["start"].toInteger(),
                                      word["end"].toInteger()});
        }
        if (v.contains("wordAnimation")) {
            if (p.schemaVersion < 12 || n.role != "caption" || !v["wordAnimation"].isObject())
                return fail("Animação por palavra exige legenda e schema 12.");
            const auto animation = v["wordAnimation"].toObject();
            if (animation.size() != 4 || !animation["preset"].isString() ||
                !integer(animation["durationFrames"]) || !animation["startScale"].isDouble() ||
                !animation["travelY"].isDouble())
                return fail("Animação por palavra incompleta ou com campos desconhecidos.");
            n.wordAnimationPreset = animation["preset"].toString().toStdString();
            n.wordAnimationDurationFrames = animation["durationFrames"].toInt();
            n.wordAnimationStartScale = animation["startScale"].toDouble();
            n.wordAnimationTravelY = animation["travelY"].toDouble();
        }
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
        ShotMarker marker{str(shot, "id"), str(shot, "sceneId"), str(shot, "name"),
                         str(shot, "transitionIn"), shot["start"].toInteger(), shot["duration"].toInteger(),
                         shot["transitionDurationFrames"].toInt(0),
                         shot.contains("transitionDirection")
                             ? shot["transitionDirection"].toString().toStdString()
                             : "left"};
        if (shot.contains("artDirection")) {
            if (p.schemaVersion < 11 || !shot["artDirection"].isObject())
                return fail("Direção de arte local exige shot e schema 11.");
            ArtDirection style = p.artDirection;
            const auto scene = std::find_if(p.scenes.begin(), p.scenes.end(), [&](const SceneMarker &candidate) {
                return candidate.id == str(shot, "sceneId");
            });
            if (scene != p.scenes.end() && scene->artDirection)
                style = *scene->artDirection;
            if (!readArtDirection(shot["artDirection"].toObject(), style))
                return fail("Direção de arte do shot contém campos inválidos ou desconhecidos.");
            marker.artDirection = std::move(style);
        }
        p.shots.push_back(std::move(marker));
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
