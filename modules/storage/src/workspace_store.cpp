#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFontDatabase>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLockFile>
#include <QMap>
#include <QSaveFile>
#include <QRegularExpression>
#include <QSet>
#include <QUuid>
#include <studio/core/library.hpp>
#include <studio/storage/project_store.hpp>
#include <studio/storage/workspace_store.hpp>

namespace studio {
namespace {
QString uuid() {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}
QString now() {
    return QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
}
QByteArray hash(const QByteArray &b) {
    return QCryptographicHash::hash(b, QCryptographicHash::Sha256).toHex();
}
bool write(const QString &path, const QByteArray &b) {
    QSaveFile f(path);
    return f.open(QIODevice::WriteOnly) && f.write(b) == b.size() && f.commit();
}
QString nativeCategory(const std::string &s) {
    if (s == "personagens") return "characters";
    if (s == "cenários") return "scenes";
    if (s == "dados" || s == "gráficos") return "data";
    if (s == "tecnologia") return "technology";
    if (s == "comunicação") return "communication";
    if (s == "finanças") return "finance";
    if (s == "ciência") return "science";
    return "objects";
}
bool categoryExists(const QJsonObject &data, const QString &id);
void ensureCategory(QJsonObject &data, const QString &id, const QString &name) {
    if (categoryExists(data, id))
        return;
    auto categories = data["categories"].toArray();
    categories.append(QJsonObject{{"id", id}, {"name", name}});
    data["categories"] = categories;
}
bool validName(const QString &s) {
    return !s.trimmed().isEmpty() && s.trimmed().size() <= 120 && !s.contains('\n') && !s.contains('\r');
}
bool categoryExists(const QJsonObject &data, const QString &id) {
    for (auto v : data["categories"].toArray())
        if (v.toObject()["id"] == id)
            return true;
    return false;
}
QJsonObject find(const QJsonObject &data, const char *key, const QString &id) {
    for (auto v : data[key].toArray())
        if (v.toObject()["id"] == id)
            return v.toObject();
    return {};
}
QString validateCatalogue(const QJsonObject &data) {
    if (data["format"] != "studio-dark-workspace" || data["version"] != 1)
        return "Catálogo de outra versão ou inválido; arquivo preservado.";
    for (auto key : {"projects", "assets", "categories"})
        if (!data[key].isArray() || data[key].toArray().size() > 10000)
            return "Lista do catálogo inválida ou grande demais.";
    QSet<QString> categories;
    for (auto v : data["categories"].toArray()) {
        auto o = v.toObject();
        auto id = o["id"].toString();
        if (id.isEmpty() || categories.contains(id) || !validName(o["name"].toString()))
            return "Categoria inválida no catálogo.";
        categories.insert(id);
    }
    if (!categories.contains("uncategorized"))
        return "Categoria de destino padrão ausente.";
    for (auto key : {"projects", "assets"}) {
        QSet<QString> ids;
        for (auto v : data[key].toArray()) {
            auto o = v.toObject();
            auto id = o["id"].toString();
            if (id.isEmpty() || ids.contains(id) || !validName(o["name"].toString()) ||
                !o["deleted"].isBool())
                return "Registro inválido no catálogo.";
            ids.insert(id);
            if (QString(key) == "assets" &&
                (!categories.contains(o["categoryId"].toString()) || !o["kind"].isString()))
                return "Categoria ou tipo de item inválido.";
            if (QString(key) == "assets" && (o.contains("width") || o.contains("height")) &&
                (!o["width"].isDouble() || !o["height"].isDouble() || o["width"].toInt() < 1 ||
                 o["height"].toInt() < 1 || o["width"].toInt() > 30000 || o["height"].toInt() > 30000))
                return "Dimensões de mídia inválidas no catálogo.";
            if (QString(key) == "assets" && o["kind"] == "font" &&
                (!o["family"].isString() || o["family"].toString().isEmpty() || !o["source"].isString() ||
                 !QFileInfo(o["source"].toString()).isAbsolute()))
                return "Fonte externa inválida no catálogo.";
            if (QString(key) == "assets" && o["kind"] == "visual-style" && !o["style"].isObject())
                return "Preset visual inválido no catálogo.";
            if (QString(key) == "projects" &&
                (!o["path"].isString() || !QFileInfo(o["path"].toString()).isAbsolute()))
                return "Caminho de projeto inválido.";
        }
    }
    return {};
}
QJsonObject defaults() {
    QJsonArray categories;
    for (auto pair : {std::pair{"objects", "Objetos"},
                      {"characters", "Personagens"},
                      {"technology", "Tecnologia"},
                      {"communication", "Comunicação"},
                      {"finance", "Finanças"},
                      {"science", "Ciência"},
                      {"scenes", "Cenários"},
                      {"data", "Dados"},
                      {"fonts", "Fontes"},
                      {"visual-styles", "Estilos visuais"},
                      {"uncategorized", "Sem categoria"}})
        categories.append(QJsonObject{{"id", pair.first}, {"name", pair.second}});
    QJsonArray assets;
    for (const auto &a : nativeLibrary())
        assets.append(QJsonObject{{"id", "native:" + QString::fromStdString(a.id)},
                                  {"nativeId", QString::fromStdString(a.id)},
                                  {"name", QString::fromStdString(a.name)},
                                  {"kind", "native"},
                                  {"categoryId", nativeCategory(a.category)},
                                  {"deleted", false},
                                  {"builtin", true},
                                  {"createdAt", now()}});
    auto premium = makeEmpty();
    premium.name = "Smartphone premium";
    premium.duration = 360;
    premium.nodes = makePremiumSmartphone();
    assets.append(QJsonObject{{"id", "premium:smartphone-premium-01"},
                              {"name", "Smartphone premium"},
                              {"kind", "premium"}, {"premiumType", "object"},
                              {"manifestVersion", 1}, {"preset", QJsonDocument::fromJson(serialize(premium)).object()},
                              {"categoryId", "technology"}, {"deleted", false}, {"builtin", true},
                              {"slots", QJsonArray{"text:screen-title", "text:notification", "image:screen"}},
                              {"createdAt", "2026-10-26T00:00:00.000Z"}});
    return {{"format", "studio-dark-workspace"},
            {"version", 1},
            {"projects", QJsonArray{}},
            {"categories", categories},
            {"assets", assets}};
}
} // namespace
QStringList loadWorkspaceFonts(const QString &workspaceDirectory) {
    static QSet<QString> registeredPaths;
    QStringList loadedFamilies;
    QFile catalogue(QDir(workspaceDirectory).filePath("catalogue.json"));
    if (!catalogue.open(QIODevice::ReadOnly) || catalogue.size() > 8 * 1024 * 1024)
        return loadedFamilies;
    const auto data = QJsonDocument::fromJson(catalogue.readAll()).object();
    for (const auto &value : data["assets"].toArray()) {
        const auto asset = value.toObject();
        if (asset["kind"] != "font" || asset["deleted"].toBool())
            continue;
        const auto path = QFileInfo(asset["source"].toString()).absoluteFilePath();
        if (!QFileInfo(path).isFile())
            continue;
        if (!registeredPaths.contains(path)) {
            if (QFontDatabase::addApplicationFont(path) < 0)
                continue;
            registeredPaths.insert(path);
        }
        const auto family = asset["family"].toString();
        if (!family.isEmpty() && !loadedFamilies.contains(family))
            loadedFamilies.append(family);
    }
    return loadedFamilies;
}
QString validatePremiumManifest(const QJsonObject &manifest, const QString &packageRoot) {
    if (manifest["format"] != "studio-dark-premium" || manifest["manifestVersion"] != 1)
        return "Formato ou versão de manifesto premium não suportado.";
    const auto id = manifest["id"].toString();
    static const QRegularExpression validId("^premium:[a-z0-9][a-z0-9-]{2,78}$");
    static const QRegularExpression semanticVersion("^\\d+\\.\\d+\\.\\d+$");
    const auto packageType = manifest["type"].toString();
    const auto canvas = manifest["canvas"].toObject();
    if (!validId.match(id).hasMatch() || !validName(manifest["name"].toString()) ||
        !semanticVersion.match(manifest["version"].toString()).hasMatch() ||
        !QStringList{"premium-object", "premium-scene-kit", "premium-motion-template",
                     "premium-typography-template", "premium-overlay"}.contains(packageType) ||
        manifest["category"].toString().isEmpty() || !canvas["width"].isDouble() ||
        !canvas["height"].isDouble() || canvas["width"].toInt() < 1 || canvas["height"].toInt() < 1 ||
        canvas["width"].toInt() > 30000 || canvas["height"].toInt() > 30000 ||
        !manifest["license"].isObject() || manifest["license"]["spdx"].toString().isEmpty() ||
        !manifest["origin"].isObject() || manifest["origin"]["kind"].toString().isEmpty() ||
        !manifest["tags"].isArray() || manifest["tags"].toArray().size() > 32 ||
        !manifest["preview"].isObject() || !manifest["hierarchy"].isObject() ||
        !manifest["layers"].isArray() ||
        manifest["layers"].toArray().isEmpty() || manifest["layers"].toArray().size() > 200 ||
        !manifest["slots"].isArray() || manifest["slots"].toArray().size() > 64 ||
        !manifest["dependencies"].isArray() || manifest["dependencies"].toArray().size() > 64)
        return "Metadados, limites, ID ou tipo de pacote premium inválidos.";
    QSet<QString> layerIds, slotIds, dependencies;
    QMap<QString, QString> layerParents;
    const auto insidePackage = [&](const QString &relative) {
        const auto root = QFileInfo(packageRoot).canonicalFilePath();
        const auto resolved = QFileInfo(QDir(packageRoot).filePath(relative)).canonicalFilePath();
        if (root.isEmpty() || resolved.isEmpty())
            return false;
        const auto child = QDir(root).relativeFilePath(resolved);
        return !QFileInfo(child).isAbsolute() && child != ".." &&
               !child.startsWith("../") && !child.startsWith("..\\");
    };
    for (const auto &value : manifest["layers"].toArray()) {
        const auto layer = value.toObject();
        const auto layerId = layer["id"].toString();
        const auto kind = layer["kind"].toString();
        if (layerId.isEmpty() || layerId.size() > 80 || layerIds.contains(layerId) ||
            !QStringList{"group", "shape", "text", "native", "image", "svg", "video", "shape-or-image"}.contains(kind))
            return "Camada premium inválida ou com ID duplicado.";
        layerIds.insert(layerId);
        if (layer.contains("parentId") && !layer["parentId"].isString())
            return "Relação pai de camada premium inválida.";
        layerParents.insert(layerId, layer["parentId"].toString());
        if (layer.contains("source")) {
            const auto source = layer["source"].toString();
            const auto relative = QDir::cleanPath(source);
            if (source.isEmpty() || QFileInfo(source).isAbsolute() || source.contains("://") ||
                relative == ".." || relative.startsWith("../") || relative.startsWith("..\\"))
                return "Caminho de camada fora do pacote premium.";
            if (!insidePackage(relative))
                return "Arquivo de pacote ausente ou fora da raiz permitida.";
        }
    }
    const auto rootId = manifest["rootNodeId"].toString();
    if (!layerIds.contains(rootId) || !layerParents[rootId].isEmpty())
        return "Nó raiz premium não existe na lista de camadas.";
    for (const auto &layerId : layerIds) {
        QSet<QString> ancestry;
        auto parent = layerParents.value(layerId);
        int depth = 1;
        while (!parent.isEmpty()) {
            if (!layerIds.contains(parent) || ancestry.contains(parent) || parent == layerId)
                return "Hierarquia premium contém pai ausente ou ciclo.";
            ancestry.insert(parent);
            parent = layerParents.value(parent);
            if (++depth > 8)
                return "Hierarquia premium excede oito níveis.";
        }
    }
    for (const auto &value : manifest["slots"].toArray()) {
        const auto slot = value.toObject();
        const auto slotId = slot["id"].toString();
        const auto kind = slot["kind"].toString();
        if (slotId.isEmpty() || slotId.size() > 80 || slotIds.contains(slotId) ||
            !QStringList{"text", "image", "video", "icon"}.contains(kind) ||
            !layerIds.contains(slot["nodeId"].toString()))
            return "Slot premium inválido, duplicado ou aponta para camada ausente.";
        slotIds.insert(slotId);
    }
    for (const auto &value : manifest["dependencies"].toArray()) {
        if (!value.isString() || value.toString().isEmpty() || value.toString() == id ||
            dependencies.contains(value.toString()))
            return "Dependências premium inválidas ou duplicadas.";
        dependencies.insert(value.toString());
    }
    const auto preview = manifest["preview"].toObject();
    if (preview.contains("source")) {
        const auto source = preview["source"].toString();
        const auto relative = QDir::cleanPath(source);
        if (source.isEmpty() || QFileInfo(source).isAbsolute() || source.contains("://") ||
            relative == ".." || relative.startsWith("../") || relative.startsWith("..\\"))
            return "Caminho de preview fora do pacote premium.";
        if (!insidePackage(relative))
            return "Preview premium ausente ou fora da raiz permitida.";
    }
    return {};
}
WorkspaceStore::WorkspaceStore(QString directory) : root_(QDir(directory).absolutePath()) {
    QDir().mkpath(root_);
    reload();
}
bool WorkspaceStore::reload() {
    auto path = root_ + "/catalogue.json";
    if (!QFile::exists(path)) {
        data_ = defaults();
        revision_.clear();
        error_.clear();
        return true;
    }
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly) || f.size() > 8 * 1024 * 1024) {
        error_ = "Não foi possível ler o catálogo local; original preservado.";
        return false;
    }
    auto bytes = f.readAll();
    QJsonParseError parse;
    auto doc = QJsonDocument::fromJson(bytes, &parse);
    if (parse.error != QJsonParseError::NoError || !doc.isObject()) {
        error_ = "Catálogo local corrompido. A cópia .previous foi preservada.";
        return false;
    }
    if (auto error = validateCatalogue(doc.object()); !error.isEmpty()) {
        error_ = error;
        return false;
    }
    data_ = doc.object();
    // Keep personal items and deletions intact, but register new built-in resources
    // in existing workspace catalogues when the application is upgraded.
    ensureCategory(data_, "technology", "Tecnologia");
    ensureCategory(data_, "communication", "Comunicação");
    ensureCategory(data_, "finance", "Finanças");
    ensureCategory(data_, "science", "Ciência");
    auto assets = data_["assets"].toArray();
    QSet<QString> known;
    for (const auto &entry : assets)
        known.insert(entry.toObject()["id"].toString());
    for (const auto &item : nativeLibrary()) {
        const auto id = "native:" + QString::fromStdString(item.id);
        if (known.contains(id))
            continue;
        assets.append(QJsonObject{{"id", id},
                                  {"nativeId", QString::fromStdString(item.id)},
                                  {"name", QString::fromStdString(item.name)},
                                  {"kind", "native"},
                                  {"categoryId", nativeCategory(item.category)},
                                  {"deleted", false},
                                  {"builtin", true},
                                  {"createdAt", now()}});
    }
    if (!known.contains("premium:smartphone-premium-01")) {
        auto premium = makeEmpty();
        premium.name = "Smartphone premium";
        premium.duration = 360;
        premium.nodes = makePremiumSmartphone();
        assets.append(QJsonObject{{"id", "premium:smartphone-premium-01"},
                                  {"name", "Smartphone premium"}, {"kind", "premium"},
                                  {"premiumType", "object"}, {"manifestVersion", 1},
                                  {"preset", QJsonDocument::fromJson(serialize(premium)).object()},
                                  {"categoryId", "technology"}, {"deleted", false}, {"builtin", true},
                                  {"slots", QJsonArray{"text:screen-title", "text:notification", "image:screen"}},
                                  {"createdAt", "2026-10-26T00:00:00.000Z"}});
    }
    data_["assets"] = assets;
    revision_ = hash(bytes);
    error_.clear();
    return true;
}
WorkspaceResult WorkspaceStore::mutate(const std::function<WorkspaceResult(QJsonObject &)> &action) {
    if (!error_.isEmpty())
        return {{}, error_};
    QLockFile lock(root_ + "/catalogue.lock");
    if (!lock.tryLock(0))
        return {{}, "O catálogo está sendo alterado em outra janela."};
    auto path = root_ + "/catalogue.json";
    QByteArray old;
    if (QFile::exists(path)) {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly) || f.size() > 8 * 1024 * 1024)
            return {{}, "Falha ao ler o catálogo atual."};
        old = f.readAll();
        if (revision_.isEmpty() || hash(old) != revision_)
            return {{}, "O catálogo mudou em outra janela. Clique em Atualizar e tente novamente."};
    } else if (!revision_.isEmpty())
        return {{}, "O catálogo foi removido no disco. Clique em Atualizar."};
    auto candidate = data_;
    auto result = action(candidate);
    if (!result.error.isEmpty())
        return result;
    if (auto error = validateCatalogue(candidate); !error.isEmpty())
        return {{}, error};
    auto bytes = QJsonDocument(candidate).toJson();
    if (!old.isEmpty() && !write(path + ".previous", old))
        return {{}, "Não foi possível preservar a versão anterior do catálogo."};
    if (!write(path, bytes))
        return {{}, "Não foi possível salvar o catálogo local."};
    data_ = std::move(candidate);
    revision_ = hash(bytes);
    return result;
}
QJsonObject WorkspaceStore::project(const QString &id) const {
    return find(data_, "projects", id);
}
QJsonObject WorkspaceStore::asset(const QString &id) const {
    return find(data_, "assets", id);
}
WorkspaceResult WorkspaceStore::registerProject(const QString &input) {
    auto path = QFileInfo(input).absoluteFilePath();
    auto loaded = loadProject(path);
    if (!loaded.error.isEmpty())
        return {{}, loaded.error};
    auto displayName =
        QString::fromStdString(loaded.project.name).replace('\n', ' ').replace('\r', ' ').trimmed().left(120);
    if (displayName.isEmpty())
        displayName = "Projeto importado";
    return mutate([&](QJsonObject &d) {
        auto list = d["projects"].toArray();
        QString id;
        bool exists = false;
        for (int i = 0; i < list.size(); ++i) {
            auto o = list[i].toObject();
            if (o["path"].toString().compare(path, Qt::CaseInsensitive) == 0) {
                id = o["id"].toString();
                o["name"] = displayName;
                o["updatedAt"] = now();
                list[i] = o;
                exists = true;
                break;
            }
        }
        if (!exists) {
            id = uuid();
            list.append(QJsonObject{{"id", id},
                                    {"name", displayName},
                                    {"path", path},
                                    {"createdAt", now()},
                                    {"updatedAt", now()},
                                    {"deleted", false}});
        }
        d["projects"] = list;
        return WorkspaceResult{id, {}};
    });
}
WorkspaceResult WorkspaceStore::createProject(Project p) {
    if (!validName(QString::fromStdString(p.name)))
        return {{}, "Informe um nome de até 120 caracteres."};
    p.id = uuid().toStdString();
    QString id = uuid();
    auto directory = root_ + "/projects/" + id;
    auto path = directory + "/project.studio.json";
    return mutate([&](QJsonObject &d) {
        if (!QDir().mkpath(directory))
            return WorkspaceResult{{}, "Não foi possível criar a pasta do projeto."};
        auto saved = saveProject(path, p, {});
        if (!saved.error.isEmpty())
            return WorkspaceResult{{}, saved.error};
        auto list = d["projects"].toArray();
        list.append(QJsonObject{{"id", id},
                                {"name", QString::fromStdString(p.name)},
                                {"path", path},
                                {"createdAt", now()},
                                {"updatedAt", now()},
                                {"deleted", false}});
        d["projects"] = list;
        return WorkspaceResult{id, {}};
    });
}
WorkspaceResult WorkspaceStore::renameProject(const QString &id, const QString &input) {
    auto name = input.trimmed();
    if (!validName(name))
        return {{}, "Informe um nome de até 120 caracteres."};
    return mutate([&](QJsonObject &d) {
        auto list = d["projects"].toArray();
        for (int i = 0; i < list.size(); ++i) {
            auto o = list[i].toObject();
            if (o["id"] != id)
                continue;
            auto path = o["path"].toString();
            auto loaded = loadProject(path);
            if (!loaded.error.isEmpty())
                return WorkspaceResult{{}, loaded.error};
            loaded.project.name = name.toStdString();
            auto saved = saveProject(path, loaded.project, loaded.revision);
            if (!saved.error.isEmpty())
                return WorkspaceResult{{}, saved.error};
            o["name"] = name;
            o["updatedAt"] = now();
            list[i] = o;
            d["projects"] = list;
            return WorkspaceResult{id, {}};
        }
        return WorkspaceResult{{}, "Projeto não encontrado."};
    });
}
WorkspaceResult WorkspaceStore::setProjectDeleted(const QString &id, bool deleted) {
    return mutate([&](QJsonObject &d) {
        auto list = d["projects"].toArray();
        for (int i = 0; i < list.size(); ++i) {
            auto o = list[i].toObject();
            if (o["id"] != id)
                continue;
            o["deleted"] = deleted;
            o["updatedAt"] = now();
            list[i] = o;
            d["projects"] = list;
            return WorkspaceResult{id, {}};
        }
        return WorkspaceResult{{}, "Projeto não encontrado."};
    });
}
WorkspaceResult WorkspaceStore::addCategory(const QString &input) {
    auto name = input.trimmed();
    if (!validName(name))
        return {{}, "Informe um nome de até 120 caracteres."};
    return mutate([&](QJsonObject &d) {
        auto list = d["categories"].toArray();
        for (auto v : list)
            if (v.toObject()["name"].toString().compare(name, Qt::CaseInsensitive) == 0)
                return WorkspaceResult{{}, "Já existe uma categoria com esse nome."};
        auto id = uuid();
        list.append(QJsonObject{{"id", id}, {"name", name}});
        d["categories"] = list;
        return WorkspaceResult{id, {}};
    });
}
WorkspaceResult WorkspaceStore::renameCategory(const QString &id, const QString &input) {
    auto name = input.trimmed();
    if (!validName(name))
        return {{}, "Informe um nome de até 120 caracteres."};
    return mutate([&](QJsonObject &d) {
        auto list = d["categories"].toArray();
        for (auto v : list)
            if (v.toObject()["id"] != id &&
                v.toObject()["name"].toString().compare(name, Qt::CaseInsensitive) == 0)
                return WorkspaceResult{{}, "Já existe uma categoria com esse nome."};
        for (int i = 0; i < list.size(); ++i) {
            auto o = list[i].toObject();
            if (o["id"] == id) {
                o["name"] = name;
                list[i] = o;
                d["categories"] = list;
                return WorkspaceResult{id, {}};
            }
        }
        return WorkspaceResult{{}, "Categoria não encontrada."};
    });
}
WorkspaceResult WorkspaceStore::deleteCategory(const QString &id) {
    if (id == "uncategorized")
        return {{}, "A categoria padrão é necessária para receber itens sem categoria."};
    return mutate([&](QJsonObject &d) {
        if (!categoryExists(d, id))
            return WorkspaceResult{{}, "Categoria não encontrada."};
        QJsonArray categories;
        for (auto v : d["categories"].toArray())
            if (v.toObject()["id"] != id)
                categories.append(v);
        d["categories"] = categories;
        auto assets = d["assets"].toArray();
        for (int i = 0; i < assets.size(); ++i) {
            auto o = assets[i].toObject();
            if (o["categoryId"] == id) {
                o["categoryId"] = "uncategorized";
                assets[i] = o;
            }
        }
        d["assets"] = assets;
        return WorkspaceResult{id, {}};
    });
}
WorkspaceResult WorkspaceStore::addAsset(const QString &input, const QString &category, const QString &source,
                                         int width, int height) {
    auto name = input.trimmed();
    if (!validName(name))
        return {{}, "Informe um nome de até 120 caracteres."};
    QFile f(source);
    auto extension = QFileInfo(source).suffix().toLower();
    const QStringList videoExtensions{"mp4", "mov", "mkv", "webm", "avi", "m4v"};
    if (!QStringList{"png", "jpg", "jpeg", "webp", "bmp", "svg", "mp4", "mov", "mkv", "webm", "avi", "m4v"}.contains(extension) ||
        !f.open(QIODevice::ReadOnly) || f.size() < 1 || f.size() > 32 * 1024 * 1024)
        return {{}, "Use imagem PNG/JPG/WebP/BMP/SVG ou vídeo MP4/MOV/MKV/WebM/AVI/M4V, até 32 MiB."};
    auto bytes = f.readAll();
    auto owned = root_ + "/library/media/" + QString::fromLatin1(hash(bytes)) + "." + extension;
    auto id = uuid();
    return mutate([&](QJsonObject &d) {
        if (!categoryExists(d, category))
            return WorkspaceResult{{}, "Categoria não encontrada."};
        if (!QDir().mkpath(root_ + "/library/media"))
            return WorkspaceResult{{}, "Falha ao criar a biblioteca."};
        if (!QFile::exists(owned) && !write(owned, bytes))
            return WorkspaceResult{{}, "Não foi possível copiar a mídia para a biblioteca."};
        auto list = d["assets"].toArray();
        QJsonObject asset{
            {"id", id},         {"name", name},           {"kind", extension == "svg" ? "svg" : videoExtensions.contains(extension) ? "video" : "image"},
            {"source", owned},  {"categoryId", category}, {"deleted", false},
            {"builtin", false}, {"createdAt", now()}};
        if (width > 0 && height > 0) {
            asset["width"] = width;
            asset["height"] = height;
        }
        list.append(asset);
        d["assets"] = list;
        return WorkspaceResult{id, {}};
    });
}
WorkspaceResult WorkspaceStore::addPreset(const QString &input, const QString &category,
                                         std::vector<Node> nodes) {
    const auto name = input.trimmed();
    if (!validName(name))
        return {{}, "Informe um nome de até 120 caracteres."};
    if (nodes.empty() || nodes.size() > 200)
        return {{}, "Seleção vazia ou maior que 200 objetos."};
    const auto origin = nodes.front().start;
    const auto clipDuration = nodes.front().duration;
    if (origin < 0 || clipDuration < 1 || clipDuration > 18000)
        return {{}, "Intervalo do preset inválido."};
    // Store a self-contained, frame-zero composition. Children retain local transforms,
    // relative timing, keys and IDs; IDs are remapped when a preset is instantiated.
    nodes.front().parentId.clear();
    QMap<QString, QByteArray> media;
    for (auto &node : nodes) {
        node.start -= origin;
        node.sceneId.clear();
        node.role.clear();
        for (auto &key : node.keyframes)
            key.frame -= origin;
        if (node.kind != NodeKind::Image && node.kind != NodeKind::Svg)
            continue;
        QFile file(QString::fromStdString(node.source));
        const auto extension = QFileInfo(file).suffix().toLower();
        if (!QStringList{"png", "jpg", "jpeg", "webp", "bmp", "svg"}.contains(extension) ||
            !file.open(QIODevice::ReadOnly) || file.size() < 1 || file.size() > 32 * 1024 * 1024)
            return {{}, "Mídia de composição inválida ou maior que 32 MiB."};
        const auto bytes = file.readAll();
        const auto owned = root_ + "/library/media/" + QString::fromLatin1(hash(bytes)) + "." + extension;
        media.insert(owned, bytes);
        node.source = owned.toStdString();
    }
    auto p = makeEmpty();
    p.duration = clipDuration;
    p.nodes = std::move(nodes);
    if (auto error = validate(p); !error.empty())
        return {{}, QString::fromStdString(error)};
    if (p.nodes.size() > 1)
        for (std::size_t i = 1; i < p.nodes.size(); ++i)
            if (p.nodes[i].parentId.empty())
                return {{}, "Preset deve conter apenas uma raiz e seus descendentes."};
    const auto id = uuid();
    return mutate([&](QJsonObject &d) {
        if (!categoryExists(d, category))
            return WorkspaceResult{{}, "Categoria não encontrada."};
        if (!media.isEmpty() && !QDir().mkpath(root_ + "/library/media"))
            return WorkspaceResult{{}, "Falha ao criar a pasta de mídia."};
        for (auto it = media.cbegin(); it != media.cend(); ++it)
            if (!QFile::exists(it.key()) && !write(it.key(), it.value()))
                return WorkspaceResult{{}, "Não foi possível guardar a mídia da composição."};
        auto list = d["assets"].toArray();
        list.append(QJsonObject{{"id", id},
                                {"name", name},
                                {"kind", "preset"},
                                {"preset", QJsonDocument::fromJson(serialize(p)).object()},
                                {"categoryId", category},
                                {"deleted", false},
                                {"builtin", false},
                                {"createdAt", now()}});
        d["assets"] = list;
        return WorkspaceResult{id, {}};
    });
}
WorkspaceResult WorkspaceStore::addFont(const QString &family, const QString &source) {
    const auto name = family.trimmed();
    const auto extension = QFileInfo(source).suffix().toLower();
    QFile file(source);
    if (!validName(name) || !QStringList{"ttf", "otf"}.contains(extension) ||
        !file.open(QIODevice::ReadOnly) || file.size() < 1 || file.size() > 64 * 1024 * 1024)
        return {{}, "Use uma fonte TTF ou OTF válida, de até 64 MiB."};
    const auto bytes = file.readAll();
    const auto owned = root_ + "/library/fonts/" + QString::fromLatin1(hash(bytes)) + "." + extension;
    const auto id = uuid();
    return mutate([&](QJsonObject &data) {
        ensureCategory(data, "fonts", "Fontes");
        if (!QDir().mkpath(root_ + "/library/fonts"))
            return WorkspaceResult{{}, "Falha ao criar a biblioteca de fontes."};
        if (!QFile::exists(owned) && !write(owned, bytes))
            return WorkspaceResult{{}, "Não foi possível copiar a fonte para a biblioteca."};
        auto assets = data["assets"].toArray();
        assets.append(QJsonObject{{"id", id},
                                  {"name", name},
                                  {"kind", "font"},
                                  {"family", name},
                                  {"source", owned},
                                  {"categoryId", "fonts"},
                                  {"deleted", false},
                                  {"builtin", false},
                                  {"createdAt", now()}});
        data["assets"] = assets;
        return WorkspaceResult{id, {}};
    });
}
WorkspaceResult WorkspaceStore::addVisualStylePreset(const QString &input, const QJsonObject &style) {
    const auto name = input.trimmed();
    if (!validName(name) || style.isEmpty())
        return {{}, "Informe um nome e uma direção de arte válida."};
    const auto id = uuid();
    return mutate([&](QJsonObject &data) {
        ensureCategory(data, "visual-styles", "Estilos visuais");
        auto assets = data["assets"].toArray();
        assets.append(QJsonObject{{"id", id},
                                  {"name", name},
                                  {"kind", "visual-style"},
                                  {"style", style},
                                  {"categoryId", "visual-styles"},
                                  {"deleted", false},
                                  {"builtin", false},
                                  {"createdAt", now()}});
        data["assets"] = assets;
        return WorkspaceResult{id, {}};
    });
}
WorkspaceResult WorkspaceStore::updateAsset(const QString &id, const QString &input,
                                            const QString &category) {
    auto name = input.trimmed();
    if (!validName(name))
        return {{}, "Informe um nome de até 120 caracteres."};
    return mutate([&](QJsonObject &d) {
        if (!categoryExists(d, category))
            return WorkspaceResult{{}, "Categoria não encontrada."};
        auto list = d["assets"].toArray();
        for (int i = 0; i < list.size(); ++i) {
            auto o = list[i].toObject();
            if (o["id"] == id) {
                o["name"] = name;
                o["categoryId"] = category;
                list[i] = o;
                d["assets"] = list;
                return WorkspaceResult{id, {}};
            }
        }
        return WorkspaceResult{{}, "Item não encontrado."};
    });
}
WorkspaceResult WorkspaceStore::setAssetDeleted(const QString &id, bool deleted) {
    return mutate([&](QJsonObject &d) {
        auto list = d["assets"].toArray();
        for (int i = 0; i < list.size(); ++i) {
            auto o = list[i].toObject();
            if (o["id"] == id) {
                o["deleted"] = deleted;
                list[i] = o;
                d["assets"] = list;
                return WorkspaceResult{id, {}};
            }
        }
        return WorkspaceResult{{}, "Item não encontrado."};
    });
}
} // namespace studio
