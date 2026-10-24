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
    for (const auto &n : p.nodes)
        nodes.append(QJsonObject{{"id", text(n.id)},
                                 {"name", text(n.name)},
                                 {"kind", n.kind == NodeKind::Text    ? "text"
                                          : n.kind == NodeKind::Shape ? "shape"
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
                                 {"animated", n.animated}});
    return QJsonDocument(QJsonObject{{"format", "studio-dark"},
                                     {"schemaVersion", p.schemaVersion},
                                     {"id", text(p.id)},
                                     {"name", text(p.name)},
                                     {"width", p.width},
                                     {"height", p.height},
                                     {"fps", QJsonObject{{"numerator", p.fps.numerator},
                                                         {"denominator", p.fps.denominator}}},
                                     {"duration", double(p.duration)},
                                     {"nodes", nodes}})
        .toJson();
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
    if (o["format"] != "studio-dark" || o["schemaVersion"] != 1)
        return fail("Formato ou versão não suportada; original preservado.");
    for (auto key : {"width", "height", "duration"})
        if (!integer(o[key]))
            return fail("Dimensão ou duração inválida.");
    auto fps = o["fps"].toObject();
    if (!integer(fps["numerator"]) || !integer(fps["denominator"]) || !o["nodes"].isArray())
        return fail("Sequência inválida.");
    Project p;
    p.id = str(o, "id");
    p.name = str(o, "name");
    p.width = o["width"].toInt();
    p.height = o["height"].toInt();
    p.duration = o["duration"].toInteger();
    p.fps = {fps["numerator"].toInt(), fps["denominator"].toInt()};
    if (o["nodes"].toArray().size() > 200)
        return fail("Máximo de 200 objetos neste protótipo.");
    for (auto value : o["nodes"].toArray()) {
        if (!value.isObject())
            return fail("Objeto inválido.");
        auto v = value.toObject();
        Node n;
        auto kind = v["kind"].toString();
        if (kind != "text" && kind != "shape" && kind != "image")
            return fail("Tipo de objeto não suportado.");
        for (auto key : {"start", "duration"})
            if (!integer(v[key]))
                return fail("Tempo de objeto inválido.");
        for (auto key : {"x", "y", "width", "height", "opacity", "fontSize"})
            if (!v[key].isDouble())
                return fail("Propriedade visual ausente ou inválida.");
        if (!v["animated"].isBool())
            return fail("Animação inválida.");
        n.kind = kind == "text" ? NodeKind::Text : kind == "shape" ? NodeKind::Shape : NodeKind::Image;
        n.id = str(v, "id");
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
        p.nodes.push_back(n);
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
