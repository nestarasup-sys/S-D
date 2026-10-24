#include "workspace_controller.hpp"
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <iostream>
#include <stdexcept>
#include <studio/composition/painter.hpp>
#include <studio/core/library.hpp>

namespace {
int checks = 0;
void check(bool condition, const char *reason) {
    ++checks;
    if (!condition)
        throw std::runtime_error(reason);
}
QVariantMap named(const QVariantList &list, const QString &name) {
    for (auto v : list)
        if (v.toMap()["name"] == name)
            return v.toMap();
    return {};
}
QByteArray read(const QString &path) {
    QFile file(path);
    file.open(QIODevice::ReadOnly);
    return file.readAll();
}
} // namespace
int main(int argc, char **argv) {
    QGuiApplication app(argc, argv);
    QTemporaryDir temp;
    try {
        EditorController editor(nullptr, temp.filePath("recovery"));
        WorkspaceController ui(&editor, temp.filePath("workspace"));
        check(ui.page() == "projects" && !ui.hasDocument(), "startup opens project home");
        check(ui.projectCount() == 0 && ui.assetCount() == 14 && ui.categories().size() == 5,
              "initial complete native library and categories");
        check(!ui.createProject(" "), "empty project names rejected");
        check(ui.createProject("Minha história"), "create managed project");
        check(ui.hasDocument() && ui.page() == "editor" && !editor.dirty(),
              "creation opens an already saved blank document");
        auto first = named(ui.projects(), "Minha história");
        auto firstId = first["id"].toString(), path = first["path"].toString();
        check(QFile::exists(path) && path.startsWith(temp.path()),
              "managed project lives in isolated workspace");
        check(ui.useAsset("native:car"), "use native item from complete library");
        editor.updateSelected("color", "#44aa88");
        check(ui.saveSelection("Meu carro", "objects"), "save configured native object as reusable preset");
        auto preset = named(ui.assets(), "Meu carro");
        auto presetId = preset["id"].toString();
        check(!presetId.isEmpty() && ui.assetCount() == 15, "custom preset catalogued");
        check(ui.useAsset(presetId) && editor.project().nodes.size() == 2, "reuse custom preset");
        check(editor.project().nodes.back().color == "#44aa88" &&
                  editor.project().nodes[0].id != editor.project().nodes[1].id,
              "preset keeps style but receives a new object ID");
        ui.navigate("library");
        check(editor.dirty() && ui.hasDocument(), "navigation preserves in-memory editing");
        check(ui.addCategory("Veículos"), "create category");
        auto category = named(ui.categories(), "Veículos")["id"].toString();
        check(!ui.addCategory("veículos"), "duplicate category names rejected case-insensitively");
        check(ui.updateAsset(presetId, "Meu carro verde", category), "rename item and change category");
        ui.setCategoryFilter(category);
        check(ui.assets().size() == 1, "category filter returns matching item");
        check(ui.renameCategory(category, "Minha garagem"), "rename category");
        check(ui.assets()[0].toMap()["categoryName"] == "Minha garagem", "item follows renamed category");
        check(ui.deleteCategory(category) && ui.assetCount() == 15, "deleting category retains its items");
        check(named(ui.assets(), "Meu carro verde")["categoryId"] == "uncategorized",
              "items reassigned to fallback category");
        check(!ui.deleteCategory("uncategorized"), "fallback category cannot be removed");
        check(ui.deleteAsset("native:car") && ui.assetCount() == 14,
              "native item removed from visible catalogue");
        check(studio::libraryItem("car") && studio::validate(editor.project()).empty(),
              "native removal never breaks existing projects or renderer registry");
        ui.setAssetTrash(true);
        check(ui.assets().size() == 1, "removed native visible in excluded-items view");
        check(ui.restoreAsset("native:car") && ui.assetCount() == 15, "restore native item");
        ui.setAssetTrash(false);
        QImage image(80, 60, QImage::Format_RGB32);
        image.fill(QColor("#adcf8d"));
        auto original = temp.filePath("foto.png");
        check(image.save(original), "create image fixture");
        check(ui.addAsset(QUrl::fromLocalFile(original), "Foto local", "objects"),
              "import image with owned copy");
        auto asset = named(ui.assets(), "Foto local");
        auto assetId = asset["id"].toString(), owned = asset["source"].toString();
        check(owned != original && QFile::exists(owned), "library owns media independently of original");
        check(QFile::remove(original) && ui.useAsset(assetId),
              "library item survives original image removal");
        check(ui.deleteAsset(assetId) && QFile::exists(owned),
              "removing library item retains media referenced by project");
        check(studio::validate(editor.project()).empty(),
              "project remains valid after image removal from library");
        check(ui.restoreAsset(assetId), "restore custom item");
        editor.updateSelected("opacity", .4);
        check(ui.saveSelection("Variação translúcida", "objects"),
              "save image preset with style and owned media");
        auto imagePreset = named(ui.assets(), "Variação translúcida");
        auto decodedPreset = studio::deserialize(
            QJsonDocument(QJsonObject::fromVariantMap(imagePreset["preset"].toMap())).toJson());
        check(decodedPreset.error.isEmpty() && decodedPreset.project.nodes.front().opacity == .4 &&
                  QFile::exists(QString::fromStdString(decodedPreset.project.nodes.front().source)),
              "image preset preserves opacity and an owned file");
        check(editor.save(), "save current edits");
        ui.navigate("projects");
        check(ui.renameProject(firstId, "História renomeada"), "rename project document and catalogue");
        check(ui.page() == "projects", "renaming from home preserves navigation");
        check(editor.title() == "História renomeada" &&
                  studio::loadProject(path).project.name == "História renomeada",
              "rename persists and refreshes current editor");
        check(ui.duplicateProject(firstId) && ui.projectCount() == 2, "duplicate project");
        auto copy = named(ui.projects(), "História renomeada (cópia)");
        auto originalProject = studio::loadProject(path).project,
             copyProject = studio::loadProject(copy["path"].toString()).project;
        check(originalProject.id != copyProject.id &&
                  originalProject.nodes.size() == copyProject.nodes.size(),
              "duplicate gets independent document identity");
        check(ui.deleteProject(firstId) && ui.projectCount() == 1 && !ui.hasDocument(),
              "delete current project closes editing and removes card");
        check(QFile::exists(path) && QFile::exists(owned),
              "project deletion retains actual project and media");
        ui.setProjectTrash(true);
        check(ui.projects().size() == 1, "project is restorable from trash");
        check(!ui.openProject(firstId), "trash projects must be restored before opening");
        check(ui.restoreProject(firstId), "restore project");
        ui.setProjectTrash(false);
        check(ui.openProject(firstId) && editor.project().nodes.size() == 3,
              "restored project reopens with all content");
        ui.setProjectSearch("cópia");
        check(ui.projects().size() == 1, "project search filters by name");
        ui.setProjectSearch("");
        ui.setAssetSearch("Foto local");
        check(ui.assets().size() == 1, "asset search");
        ui.setAssetSearch("");
        // A selected group is stored as a self-contained composition and restored in one undo step.
        editor.selectNode(QString::fromStdString(editor.project().nodes[1].id));
        editor.groupSelected();
        check(ui.canSaveSelection(), "selected root group can be saved");
        editor.setFrame(0);
        editor.recordSelectedKeyframe();
        editor.setFrame(20);
        editor.recordSelectedKeyframe();
        editor.setFrame(0);
        check(ui.saveSelection("Painel com animação", "objects"), "save group and keyframes");
        const auto groupPreset = named(ui.assets(), "Painel com animação");
        const auto groupPresetId = groupPreset["id"].toString();
        auto decodedGroup = studio::deserialize(
            QJsonDocument(QJsonObject::fromVariantMap(groupPreset["preset"].toMap())).toJson());
        check(decodedGroup.error.isEmpty() && decodedGroup.project.nodes.size() == 2 &&
                  decodedGroup.project.nodes.front().start == 0 &&
                  decodedGroup.project.nodes.front().keyframes.size() == 2,
              "group preset preserves child and relative keyframes");
        editor.setFrame(1);
        check(!ui.useAsset(groupPresetId), "reject insert when animated composition cannot fit scene");
        editor.setFrame(0);
        const auto beforeInsert = editor.project().nodes.size();
        check(ui.useAsset(groupPresetId) && editor.project().nodes.size() == beforeInsert + 2,
              "reinsert group with children");
        const auto insertedRoot = editor.project().nodes[beforeInsert];
        const auto insertedChild = editor.project().nodes[beforeInsert + 1];
        check(insertedChild.parentId == insertedRoot.id &&
                  insertedRoot.id != decodedGroup.project.nodes.front().id &&
                  insertedRoot.keyframes.size() == 2 &&
                  studio::validate(editor.project()).empty(),
              "group imported with remapped IDs, parents and valid timeline");
        editor.undo();
        check(editor.project().nodes.size() == beforeInsert, "one undo removes full composition");

        // A child image must survive deletion of its external source in a saved group.
        QImage groupPhoto(32, 32, QImage::Format_RGB32);
        groupPhoto.fill(QColor("#a3be8c"));
        const auto sourcePhoto = temp.filePath("group-photo.png");
        check(groupPhoto.save(sourcePhoto), "create external image for group");
        studio::Node photoRoot, photoChild;
        photoRoot.id = "photo-root";
        photoRoot.kind = studio::NodeKind::Group;
        photoRoot.name = "Grupo de fotos";
        photoRoot.start = 40;
        photoRoot.duration = 120;
        photoChild.id = "photo-child";
        photoChild.kind = studio::NodeKind::Image;
        photoChild.source = sourcePhoto.toStdString();
        photoChild.parentId = photoRoot.id;
        photoChild.start = 50;
        photoChild.duration = 100;
        studio::WorkspaceStore mediaCatalog(temp.filePath("workspace"));
        auto storedGroup = mediaCatalog.addPreset("Grupo de imagem", "objects", {photoRoot, photoChild});
        check(storedGroup.error.isEmpty(), "store grouped media with owned copy");
        const auto storedJson = mediaCatalog.asset(storedGroup.id)["preset"].toObject();
        auto storedProject = studio::deserialize(QJsonDocument(storedJson).toJson());
        check(storedProject.error.isEmpty() && storedProject.project.nodes.size() == 2 &&
                  storedProject.project.nodes[1].start == 10,
              "group timing is normalized to selected root");
        auto mediaFile = QString::fromStdString(storedProject.project.nodes[1].source);
        check(mediaFile != sourcePhoto && QFile::exists(mediaFile), "child file copied to personal library");
        check(QFile::remove(sourcePhoto), "delete external image fixture");
        ui.refresh();
        const auto beforeMedia = editor.project().nodes.size();
        check(ui.useAsset(storedGroup.id) && editor.project().nodes.size() == beforeMedia + 2,
              "group insertion works after original image has been removed");
        check(editor.project().nodes.back().source == mediaFile.toStdString(),
              "inserted group references retained library copy");
        editor.undo();
        check(editor.project().nodes.size() == beforeMedia, "group with media undoes atomically");

        studio::WorkspaceStore firstStore(temp.filePath("workspace")), staleStore(temp.filePath("workspace"));
        check(firstStore.addCategory("Concorrência A").error.isEmpty(), "first catalogue writer commits");
        check(!staleStore.addCategory("Concorrência B").error.isEmpty(), "stale catalogue writer rejected");
        check(staleStore.reload() && staleStore.addCategory("Concorrência B").error.isEmpty(),
              "reload resolves stale catalogue revision");
        ui.refresh();
        check(named(ui.categories(), "Concorrência A").size() > 0, "refresh sees external catalogue update");
        auto index = temp.filePath("workspace/catalogue.json");
        check(QFile::exists(index + ".previous"), "catalogue previous snapshot retained");
        studio::WorkspaceStore reopened(temp.filePath("workspace"));
        check(reopened.data()["projects"].toArray().size() == 2, "catalogue persists across sessions");
        auto brokenRoot = temp.filePath("broken");
        QDir().mkpath(brokenRoot);
        QFile broken(brokenRoot + "/catalogue.json");
        broken.open(QIODevice::WriteOnly);
        broken.write("broken");
        broken.close();
        studio::WorkspaceStore damaged(brokenRoot);
        check(!damaged.error().isEmpty() && !damaged.addCategory("Do not overwrite").error.isEmpty() &&
                  read(broken.fileName()) == "broken",
              "corrupt catalogue is never silently replaced");
        std::cout << "PASS: " << checks
                  << " workspace checks (projects, trash, categories, assets, presets, concurrency)\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "FAIL after " << checks << ": " << e.what() << '\n';
        return 1;
    }
}
