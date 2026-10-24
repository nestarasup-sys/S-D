#include "workspace_controller.hpp"
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QImageReader>
#include <QJsonArray>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QSvgRenderer>
#include <algorithm>
#include <studio/composition/painter.hpp>
#include <studio/core/library.hpp>

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
bool WorkspaceController::createProject(QString name) {
    if (busy())
        return false;
    auto p = studio::makeEmpty();
    p.name = name.trimmed().toStdString();
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
    if (QFileInfo(path).suffix().compare("svg", Qt::CaseInsensitive) == 0) {
        QSvgRenderer svg(path);
        if (QFileInfo(path).size() > 4 * 1024 * 1024 || !svg.isValid()) {
            notify("SVG inválido ou maior que 4 MiB.");
            return false;
        }
    } else {
        QImageReader reader(path);
        auto size = reader.size();
        if (!reader.canRead() || size.isEmpty() || qint64(size.width()) * size.height() > 40000000) {
            notify("Imagem inválida ou maior que 40 megapixels.");
            return false;
        }
        size.scale(640, 640, Qt::KeepAspectRatio);
        reader.setScaledSize(size);
        if (reader.read().isNull()) {
            notify("A imagem não pôde ser decodificada.");
            return false;
        }
    }
    return complete(store_.addAsset(name, category, path),
                    "Item copiado para a biblioteca. O arquivo original não precisa ser movido.");
}
bool WorkspaceController::canSaveSelection() const {
    const auto *node = studio::findNode(editor_->project(), editor_->selectedId().toStdString());
    return hasDocument_ && node && node->kind != studio::NodeKind::Group;
}
bool WorkspaceController::saveSelection(QString name, QString category) {
    auto n = studio::findNode(editor_->project(), editor_->selectedId().toStdString());
    if (!hasDocument_ || !n) {
        notify("Selecione um objeto no editor primeiro.");
        return false;
    }
    if (n->kind == studio::NodeKind::Group) {
        notify("Salvar composições inteiras na biblioteca ainda será adicionado.");
        return false;
    }
    auto r = store_.addPreset(name, category, *n);
    if (!r.error.isEmpty())
        return complete(r, {});
    auto p = studio::makeEmpty();
    auto preview = *n;
    preview.id = "preview";
    preview.sceneId.clear();
    preview.start = 0;
    preview.duration = 360;
    preview.animated = false;
    preview.rotation = 0;
    preview.scale = 1;
    preview.travelX = preview.travelY = preview.spinDegrees = 0;
    double factor = std::min(1000. / preview.width, 550. / preview.height);
    preview.width *= factor;
    preview.height *= factor;
    preview.x = (1280 - preview.width) / 2;
    preview.y = (720 - preview.height) / 2;
    p.nodes = {preview};
    QDir().mkpath(store_.root() + "/cache");
    studio::CompositionPainter painter;
    painter.frame(p, 60).scaled(480, 270).save(thumbnailPath(r.id));
    return complete(r, "Objeto personalizado salvo na biblioteca.");
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
        if (!loaded.error.isEmpty() || loaded.project.nodes.size() != 1) {
            notify("Objeto personalizado inválido.");
            return false;
        }
        n = loaded.project.nodes.front();
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
    if (!editor_->insertLibraryNode(n)) {
        notify(editor_->message());
        return false;
    }
    navigate("editor");
    notify("Item adicionado ao projeto.");
    return true;
}
