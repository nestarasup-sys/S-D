#include "workspace_controller.hpp"
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSaveFile>
#include <QStandardPaths>
#include <QSvgRenderer>
#include <algorithm>
#include <unordered_set>
#include <studio/composition/painter.hpp>
#include <studio/core/library.hpp>
#include <studio/production/recipe.hpp>

namespace {
QString defaultRoot() {
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/workspace";
}
QString displayDate(QString iso) {
    auto date = QDateTime::fromString(iso, Qt::ISODateWithMs);
    return date.toLocalTime().toString("dd/MM/yyyy · HH:mm");
}
} // namespace
WorkspaceController::WorkspaceController(EditorController *editor, QString directory, QObject *parent)
    : QObject(parent), editor_(editor), store_(directory.isEmpty() ? defaultRoot() : directory) {
    connect(editor_, &EditorController::projectAvailable, this, [this](QString path) {
        rememberProject(path);
        hasDocument_ = true;
        navigate("editor");
    });
    connect(editor_, &EditorController::documentChanged, this, &WorkspaceController::selectionChanged);
    if (!store_.error().isEmpty())
        notify(store_.error());
    if (directory.isEmpty() && store_.data()["projects"].toArray().isEmpty()) {
        auto old = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/productions";
        for (const auto &child : QDir(old).entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            auto path = old + "/" + child + "/project.studio.json";
            if (QFileInfo::exists(path)) {
                auto result = store_.registerProject(path);
                if (result.error.isEmpty())
                    makeProjectCover(result.id);
            }
        }
    }
}
void WorkspaceController::notify(QString text) {
    message_ = std::move(text);
    emit messageChanged();
}
bool WorkspaceController::busy() {
    if (editor_->exporting() || editor_->producing()) {
        notify("Aguarde ou cancele a tarefa em andamento antes de trocar ou alterar projetos.");
        return true;
    }
    return false;
}
bool WorkspaceController::complete(studio::WorkspaceResult r, QString text, bool projects) {
    if (!r.error.isEmpty()) {
        notify(r.error);
        return false;
    }
    notify(std::move(text));
    if (projects)
        emit projectsChanged();
    else
        emit assetsChanged();
    return true;
}
void WorkspaceController::navigate(QString page) {
    if (page != "projects" && page != "library" && page != "editor")
        return;
    if (page != "editor" && editor_->playing())
        editor_->togglePlayback();
    page_ = std::move(page);
    emit pageChanged();
}
void WorkspaceController::refresh() {
    if (!store_.reload())
        notify(store_.error());
    else
        notify("Projetos e biblioteca atualizados.");
    emit projectsChanged();
    emit assetsChanged();
}
void WorkspaceController::setProjectSearch(QString v) {
    projectSearch_ = std::move(v);
    emit projectsChanged();
}
void WorkspaceController::setAssetSearch(QString v) {
    assetSearch_ = std::move(v);
    emit assetsChanged();
}
void WorkspaceController::setCategoryFilter(QString v) {
    categoryFilter_ = std::move(v);
    emit assetsChanged();
}
void WorkspaceController::setProjectTrash(bool v) {
    projectTrash_ = v;
    emit projectsChanged();
}
void WorkspaceController::setAssetTrash(bool v) {
    assetTrash_ = v;
    emit assetsChanged();
}
int WorkspaceController::projectCount() const {
    int total = 0;
    for (auto v : store_.data()["projects"].toArray())
        if (!v.toObject()["deleted"].toBool())
            ++total;
    return total;
}
int WorkspaceController::assetCount() const {
    int total = 0;
    for (auto v : store_.data()["assets"].toArray())
        if (!v.toObject()["deleted"].toBool())
            ++total;
    return total;
}
QString WorkspaceController::thumbnailPath(QString id) const {
    return store_.root() + "/cache/" +
           QString::fromLatin1(QCryptographicHash::hash(id.toUtf8(), QCryptographicHash::Sha256).toHex()) +
           ".png";
}
void WorkspaceController::makeProjectCover(QString id) {
    auto item = store_.project(id);
    auto loaded = studio::loadProject(item["path"].toString());
    if (!loaded.error.isEmpty())
        return;
    QDir().mkpath(store_.root() + "/cache");
    studio::CompositionPainter painter;
    painter.frame(loaded.project, std::min<std::int64_t>(60, loaded.project.duration - 1))
        .scaled(480, 270, Qt::KeepAspectRatio, Qt::SmoothTransformation)
        .save(thumbnailPath(id));
}
void WorkspaceController::rememberProject(QString path) {
    auto r = store_.registerProject(path);
    if (r.error.isEmpty()) {
        if (store_.project(r.id)["deleted"].toBool())
            store_.setProjectDeleted(r.id, false);
        makeProjectCover(r.id);
        emit projectsChanged();
    } else
        notify(r.error);
}
QVariantList WorkspaceController::projects() const {
    QVector<QJsonObject> list;
    for (auto v : store_.data()["projects"].toArray()) {
        auto o = v.toObject();
        if (o["deleted"].toBool() != projectTrash_ ||
            !o["name"].toString().contains(projectSearch_, Qt::CaseInsensitive))
            continue;
        o["dateLabel"] = displayDate(o["updatedAt"].toString());
        o["missing"] = !QFileInfo::exists(o["path"].toString());
        auto thumb = thumbnailPath(o["id"].toString());
        o["thumbnail"] = QFileInfo::exists(thumb) ? QUrl::fromLocalFile(thumb).toString() : QString{};
        list.append(o);
    }
    std::sort(list.begin(), list.end(), [](const auto &a, const auto &b) {
        return a["updatedAt"].toString() > b["updatedAt"].toString();
    });
    QVariantList result;
    for (const auto &o : list)
        result.append(o.toVariantMap());
    return result;
}
QVariantList WorkspaceController::categories() const {
    QVariantList result;
    for (auto value : store_.data()["categories"].toArray()) {
        auto o = value.toObject();
        int count = 0;
        for (auto a : store_.data()["assets"].toArray())
            if (a.toObject()["categoryId"] == o["id"] && !a.toObject()["deleted"].toBool())
                ++count;
        o["count"] = count;
        result.append(o.toVariantMap());
    }
    return result;
}
QVariantList WorkspaceController::assetChoices() const {
    QVariantList result;
    for (auto value : store_.data()["assets"].toArray()) {
        auto o = value.toObject();
        if (!o["deleted"].toBool())
            result.append(o.toVariantMap());
    }
    return result;
}
QVariantList WorkspaceController::assets() const {
    QVariantList result;
    for (auto value : store_.data()["assets"].toArray()) {
        auto o = value.toObject();
        if (o["deleted"].toBool() != assetTrash_ ||
            (!categoryFilter_.isEmpty() && o["categoryId"] != categoryFilter_) ||
            !o["name"].toString().contains(assetSearch_, Qt::CaseInsensitive))
            continue;
        for (auto c : store_.data()["categories"].toArray())
            if (c.toObject()["id"] == o["categoryId"])
                o["categoryName"] = c.toObject()["name"];
        auto kind = o["kind"].toString();
        if (kind == "native")
            o["thumbnail"] = "image://studio/native/" + o["nativeId"].toString();
        else if (kind == "preset") {
            auto path = thumbnailPath(o["id"].toString());
            o["thumbnail"] = QFileInfo::exists(path) ? QUrl::fromLocalFile(path).toString() : QString{};
        } else {
            o["thumbnail"] = QUrl::fromLocalFile(o["source"].toString()).toString();
            o["missing"] = !QFileInfo::exists(o["source"].toString());
        }
        o["typeLabel"] = kind == "native"   ? "NATIVO"
                         : kind == "preset" ? "PERSONALIZADO"
                                            : QFileInfo(o["source"].toString()).suffix().toUpper();
        result.append(o.toVariantMap());
    }
    return result;
}
bool WorkspaceController::createProject(QString name, int width, int height) {
    if (busy())
        return false;
    if (!((width == 1280 && height == 720) || (width == 1920 && height == 1080) ||
          (width == 1080 && height == 1920))) {
        notify("Escolha um formato compatível: 1280×720, 1920×1080 ou 1080×1920.");
        return false;
    }
    auto p = studio::makeEmpty();
    p.name = name.trimmed().toStdString();
    p.width = width;
    p.height = height;
    auto r = store_.createProject(p);
    if (!complete(r, "Projeto criado e salvo.", true))
        return false;
    return openProject(r.id);
}
bool WorkspaceController::openProject(QString id) {
    if (busy())
        return false;
    auto item = store_.project(id);
    if (item.isEmpty() || item["deleted"].toBool()) {
        notify("Projeto indisponível. Restaure pela lixeira, se necessário.");
        return false;
    }
    if (!editor_->open(QUrl::fromLocalFile(item["path"].toString()))) {
        notify(editor_->message());
        return false;
    }
    return true;
}
bool WorkspaceController::importProject(QUrl path) {
    if (busy())
        return false;
    QFile source(path.toLocalFile());
    if (source.open(QIODevice::ReadOnly) && source.size() <= 1024 * 1024) {
        const auto document = QJsonDocument::fromJson(source.readAll());
        if (document.isObject() && document.object()["format"] == "studio-dark-production") {
            editor_->importProduction(path);
            navigate("editor");
            return true;
        }
    }
    if (!editor_->open(path)) {
        notify(editor_->message());
        return false;
    }
    return true;
}
bool WorkspaceController::renameProject(QString id, QString name) {
    if (busy())
        return false;
    auto item = store_.project(id);
    auto previousPage = page_;
    auto current = item["path"].toString().compare(editor_->projectPath(), Qt::CaseInsensitive) == 0;
    if (current && editor_->dirty() && !editor_->save()) {
        notify(editor_->message());
        return false;
    }
    auto result = store_.renameProject(id, name);
    if (!complete(result, "Projeto renomeado.", true))
        return false;
    makeProjectCover(id);
    if (current)
        editor_->open(QUrl::fromLocalFile(item["path"].toString()));
    navigate(previousPage);
    emit projectsChanged();
    return true;
}
bool WorkspaceController::duplicateProject(QString id) {
    if (busy())
        return false;
    auto item = store_.project(id);
    if (item.isEmpty())
        return false;
    auto loaded = studio::loadProject(item["path"].toString());
    if (item["path"].toString().compare(editor_->projectPath(), Qt::CaseInsensitive) == 0)
        loaded.project = editor_->project();
    else if (!loaded.error.isEmpty()) {
        notify(loaded.error);
        return false;
    }
    loaded.project.name = QString(item["name"].toString().left(110) + " (cópia)").toStdString();
    auto r = store_.createProject(loaded.project);
    if (!complete(r, "Cópia criada. O projeto original foi preservado.", true))
        return false;
    makeProjectCover(r.id);
    emit projectsChanged();
    return true;
}
bool WorkspaceController::deleteProject(QString id) {
    if (busy())
        return false;
    auto item = store_.project(id);
    bool current = item["path"].toString().compare(editor_->projectPath(), Qt::CaseInsensitive) == 0;
    if (current && editor_->dirty() && !editor_->save()) {
        notify(editor_->message());
        return false;
    }
    if (!complete(store_.setProjectDeleted(id, true), "Projeto movido para a lixeira. É possível restaurar.",
                  true))
        return false;
    if (current) {
        editor_->newProject(false);
        hasDocument_ = false;
        navigate("projects");
    }
    return true;
}
bool WorkspaceController::restoreProject(QString id) {
    return complete(store_.setProjectDeleted(id, false), "Projeto restaurado.", true);
}
bool WorkspaceController::addCategory(QString name) {
    return complete(store_.addCategory(name), "Categoria criada.");
}
bool WorkspaceController::renameCategory(QString id, QString name) {
    return complete(store_.renameCategory(id, name), "Categoria renomeada.");
}
bool WorkspaceController::deleteCategory(QString id) {
    auto r = store_.deleteCategory(id);
    if (!r.error.isEmpty())
        return complete(r, {});
    categoryFilter_.clear();
    return complete(r, "Categoria excluída. Itens movidos para Sem categoria.");
}
QString WorkspaceController::suggestedAssetName(QUrl source) const {
    return QFileInfo(source.toLocalFile()).completeBaseName().left(120);
}
bool WorkspaceController::addAsset(QUrl source, QString name, QString category) {
    auto path = source.toLocalFile();
    QSize mediaSize;
    if (QFileInfo(path).suffix().compare("svg", Qt::CaseInsensitive) == 0) {
        QSvgRenderer svg(path);
        if (QFileInfo(path).size() > 4 * 1024 * 1024 || !svg.isValid()) {
            notify("SVG inválido ou maior que 4 MiB.");
            return false;
        }
        mediaSize = svg.defaultSize();
    } else {
        QImageReader reader(path);
        auto size = reader.size();
        if (!reader.canRead() || size.isEmpty() || qint64(size.width()) * size.height() > 40000000) {
            notify("Imagem inválida ou maior que 40 megapixels.");
            return false;
        }
        mediaSize = size;
        size.scale(640, 640, Qt::KeepAspectRatio);
        reader.setScaledSize(size);
        if (reader.read().isNull()) {
            notify("A imagem não pôde ser decodificada.");
            return false;
        }
    }
    return complete(store_.addAsset(name, category, path, mediaSize.width(), mediaSize.height()),
                    "Item copiado para a biblioteca. O arquivo original não precisa ser movido.");
}
bool WorkspaceController::canSaveSelection() const {
    const auto *node = studio::findNode(editor_->project(), editor_->selectedId().toStdString());
    return hasDocument_ && node && node->role != "background" && node->parentId.empty();
}
bool WorkspaceController::saveSelection(QString name, QString category) {
    const auto *root = studio::findNode(editor_->project(), editor_->selectedId().toStdString());
    if (!root || !canSaveSelection()) {
        notify("Selecione um objeto ou grupo raiz (não um cenário de fundo).");
        return false;
    }
    std::vector<studio::Node> nodes{*root};
    std::unordered_set<std::string> collected{root->id};
    // Project storage can list children before their parent. Collect by parent link,
    // not by row adjacency, preserving the visual order of unrelated siblings.
    bool found = true;
    while (found) {
        found = false;
        for (const auto &node : editor_->project().nodes)
            if (!node.parentId.empty() && collected.contains(node.parentId) &&
                collected.insert(node.id).second) {
                nodes.push_back(node);
                found = true;
            }
    }
    auto r = store_.addPreset(name, category, std::move(nodes));
    if (!r.error.isEmpty())
        return complete(r, {});
    const auto asset = store_.asset(r.id);
    auto loaded = studio::deserialize(QJsonDocument(asset["preset"].toObject()).toJson());
    if (loaded.error.isEmpty()) {
        QDir().mkpath(store_.root() + "/cache");
        studio::CompositionPainter painter;
        const auto frame = std::min<std::int64_t>(loaded.project.duration - 1, 30);
        painter.frame(loaded.project, frame).scaled(480, 270).save(thumbnailPath(r.id));
    }
    return complete(r, "Composição salva na biblioteca com hierarquia, animações e mídias.");
}
void WorkspaceController::saveAiGuide(QUrl path) {
    if (!store_.error().isEmpty()) {
        notify(store_.error());
        return;
    }
    QSaveFile file(path.toLocalFile());
    const auto bytes = studio::aiGuide(store_.data()).toUtf8();
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit())
        notify("Não foi possível salvar o guia com a biblioteca atual.");
    else
        notify("Guia para IA salvo com o catálogo nativo e a biblioteca pessoal ativa.");
}
bool WorkspaceController::updateAsset(QString id, QString name, QString category) {
    return complete(store_.updateAsset(id, name, category), "Item organizado.");
}
bool WorkspaceController::deleteAsset(QString id) {
    return complete(store_.setAssetDeleted(id, true),
                    "Item removido da biblioteca. Projetos existentes continuam funcionando.");
}
bool WorkspaceController::restoreAsset(QString id) {
    return complete(store_.setAssetDeleted(id, false), "Item restaurado na biblioteca.");
}
bool WorkspaceController::useAsset(QString id) {
    if (!hasDocument_) {
        notify("Crie ou abra um projeto antes de adicionar um item.");
        return false;
    }
    auto item = store_.asset(id);
    if (item.isEmpty() || item["deleted"].toBool()) {
        notify("Item indisponível.");
        return false;
    }
    studio::Node n;
    std::vector<studio::Node> presetNodes;
    auto kind = item["kind"].toString();
    if (kind == "native") {
        const auto asset = studio::libraryItem(item["nativeId"].toString().toStdString());
        if (!asset) {
            notify("Objeto nativo não encontrado.");
            return false;
        }
        n.kind = studio::NodeKind::Native;
        n.nativeId = asset->id;
        n.width = asset->width;
        n.height = asset->height;
        n.color = "#efb873";
    } else if (kind == "preset") {
        auto loaded = studio::deserialize(QJsonDocument(item["preset"].toObject()).toJson());
        if (!loaded.error.isEmpty() || loaded.project.nodes.empty() ||
            !loaded.project.nodes.front().parentId.empty()) {
            notify("Composição personalizada inválida.");
            return false;
        }
        presetNodes = std::move(loaded.project.nodes);
        for (const auto &part : presetNodes)
            if ((part.kind == studio::NodeKind::Image || part.kind == studio::NodeKind::Svg) &&
                !QFileInfo::exists(QString::fromStdString(part.source))) {
                notify("Uma mídia da composição não está disponível.");
                return false;
            }
        n = presetNodes.front();
    } else {
        n.kind = kind == "svg" ? studio::NodeKind::Svg : studio::NodeKind::Image;
        n.source = item["source"].toString().toStdString();
        n.width = 480;
        n.height = 300;
        if (!QFileInfo::exists(QString::fromStdString(n.source))) {
            notify("Arquivo da biblioteca ausente.");
            return false;
        }
    }
    n.name = item["name"].toString().toStdString();
    const bool compound = kind == "preset" &&
                          (presetNodes.size() > 1 || !presetNodes.front().keyframes.empty() ||
                           presetNodes.front().kind == studio::NodeKind::Group);
    if (compound)
        presetNodes.front().name = n.name;
    if (!(compound ? editor_->insertLibraryNodes(std::move(presetNodes))
                   : editor_->insertLibraryNode(n))) {
        notify(editor_->message());
        return false;
    }
    navigate("editor");
    notify("Item adicionado ao projeto.");
    return true;
}
