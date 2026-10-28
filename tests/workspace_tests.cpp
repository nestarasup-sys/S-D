#include "workspace_controller.hpp"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QImage>
#include <QImageReader>
#include <QPainter>
#include <QProcess>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <iostream>
#include <stdexcept>
#include <studio/composition/painter.hpp>
#include <studio/core/library.hpp>
#include <studio/production/recipe.hpp>

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
        check(ui.projectCount() == 0 &&
                  ui.assetCount() == int(studio::nativeLibrary().size()) +
                                         int(studio::premiumLibrary().size()) &&
                  ui.categories().size() == 11 &&
                  !named(ui.categories(), "Tecnologia").isEmpty() &&
                  !named(ui.categories(), "Comunicação").isEmpty() &&
                  !named(ui.categories(), "Finanças").isEmpty() &&
                  !named(ui.categories(), "Ciência").isEmpty(),
              ("initial complete native library and categories: " + ui.message().toStdString() +
               ", assets=" + std::to_string(ui.assetCount())).c_str());
        const auto pngCount = std::count_if(studio::nativeLibrary().begin(), studio::nativeLibrary().end(),
                                            [](const auto &asset) { return !asset.resourcePath.empty(); });
        const auto previewCount = std::count_if(studio::nativeLibrary().begin(), studio::nativeLibrary().end(),
                                                [](const auto &asset) { return asset.previewOnly; });
        check(studio::nativeLibrary().size() == 72 && pngCount == 60 && previewCount == 12 &&
                  studio::libraryItem("illustrated:car") &&
                  studio::libraryItem("illustrated:car") == studio::libraryItem("car") &&
                  studio::libraryItem("car")->fallbackId == "car",
              "60 original native IDs carry the default PNG art, with old illustrated IDs accepted only as aliases");
        const auto nativeCar = named(ui.assets(), "Carro");
        const auto nativeScene = named(ui.assets(), "Rua");
        check(nativeCar["id"] == "native:car" && nativeCar["nativeId"] == "car" &&
                  nativeCar["typeLabel"] == "ARTE IA" &&
                  nativeScene["id"] == "native:street" && nativeScene["nativeId"] == "street" &&
                  nativeScene["previewOnly"].toBool() && nativeScene["sourceWidth"].toInt() == 483 &&
                  nativeScene["sourceHeight"].toInt() == 272 && nativeScene["typeLabel"] == "PRÉVIA" &&
                  ui.assetCount() == 81,
              "the library shows each original ID once and labels low-resolution native scenes as previews");
        const auto premium = named(ui.assets(), "Smartphone premium");
        check(premium["id"] == "premium:smartphone-premium-01" &&
                  premium["kind"] == "premium" && !premium["thumbnail"].toString().isEmpty(),
              "versioned smartphone premium is visible with generated preview");
        ui.setAssetTypeFilter("premium");
        const auto premiumAssets = ui.assets();
        check(premiumAssets.size() == 9, "all nine registered premium assets appear in the library");
        for (const auto &entryValue : premiumAssets) {
            const auto entry = entryValue.toMap();
            const QImage thumbnail(QUrl(entry["thumbnail"].toString()).toLocalFile());
            check(!thumbnail.isNull() && thumbnail.width() <= 480 && thumbnail.height() <= 270,
                  "every registered premium asset has a real generated library thumbnail");
            const auto assetId = entry["id"].toString().toStdString();
            auto project = studio::makeEmpty();
            project.schemaVersion = 15;
            project.nodes = studio::makePremiumAsset(assetId, 1280, 720);
            check(!studio::validate(project).size(), "registered premium factory produces a valid project");
            const auto manifest = QJsonObject::fromVariantMap(entry["manifest"].toMap());
            const auto nodes = project.nodes;
            check(manifest["layers"].toArray().size() == int(nodes.size()),
                  "manifest layer count agrees with registered template");
            for (const auto &slotValue : manifest["slots"].toArray()) {
                const auto slot = slotValue.toObject();
                check(std::any_of(nodes.begin(), nodes.end(), [&](const auto &node) {
                          return node.id == slot["nodeId"].toString().toStdString() &&
                                 node.role == slot["role"].toString().toStdString();
                      }),
                      "every manifest slot resolves to a real editable layer role");
            }
        }
        ui.setAssetTypeFilter("basic");
        check(ui.assets().size() == int(studio::nativeLibrary().size()),
              "basic filter contains exactly the 72 original native IDs without duplicate artwork entries");
        ui.setAssetTypeFilter("media");
        check(ui.assets().isEmpty(), "real-media category is separate from native and premium assets");
        ui.setAssetTypeFilter("");
        check(!ui.createProject(" "), "empty project names rejected");
        check(ui.createProject("Minha história"), "create managed project");
        check(ui.hasDocument() && ui.page() == "editor" && !editor.dirty(),
              "creation opens an already saved blank document");
        const auto primaryProject = named(ui.projects(), "Minha história");
        const auto firstId = primaryProject["id"].toString();
        const auto path = primaryProject["path"].toString();
        check(ui.useAsset("native:car"), "the original native ID inserts the default PNG artwork");
        const auto insertedArtwork = std::find_if(
            editor.project().nodes.begin(), editor.project().nodes.end(),
            [](const auto &node) { return node.nativeId == "car"; });
        check(insertedArtwork != editor.project().nodes.end() && insertedArtwork->kind == studio::NodeKind::Native &&
                  insertedArtwork->width == studio::libraryItem("car")->width,
              "new artwork is an ordinary transformable node using only the original native ID");
        const auto persistedArtwork = studio::deserialize(studio::serialize(editor.project()));
        check(persistedArtwork.error.isEmpty() &&
                  std::any_of(persistedArtwork.project.nodes.begin(), persistedArtwork.project.nodes.end(),
                              [](const auto &node) { return node.nativeId == "car"; }),
              "original native ID survives project serialization and reopening");
        studio::CompositionPainter illustrationPainter;
        auto legacyCar = studio::makeEmpty();
        studio::Node legacyCarNode;
        legacyCarNode.id = "car"; legacyCarNode.kind = studio::NodeKind::Native; legacyCarNode.nativeId = "car";
        legacyCarNode.name = "Carro"; legacyCarNode.x = 410; legacyCarNode.y = 220;
        legacyCarNode.width = 460; legacyCarNode.height = 210; legacyCarNode.animated = false;
        legacyCar.nodes.push_back(legacyCarNode);
        auto aiCar = legacyCar;
        aiCar.nodes.front().nativeId = "illustrated:car";
        const auto nativeCarFrame = illustrationPainter.frame(legacyCar, 30);
        const auto legacyAliasFrame = illustrationPainter.frame(aiCar, 30);
        check(!nativeCarFrame.isNull() && nativeCarFrame == legacyAliasFrame,
              "old illustrated IDs resolve invisibly to the same new default art as the original ID");
        check(ui.useAsset("premium:smartphone-premium-01") && editor.project().nodes.size() >= 15,
              "premium smartphone inserts its editable vector hierarchy");
        const auto slotText = std::find_if(editor.project().nodes.begin(), editor.project().nodes.end(),
                                           [](const auto &n) { return n.role == "premium:text:notification"; });
        check(slotText != editor.project().nodes.end(), "premium text slot is a real project node");
        const auto slotId = QString::fromStdString(slotText->id);
        editor.selectNode(slotId);
        editor.updateSelected("text", "Oferta atualizada");
        check(studio::findNode(editor.project(), slotId.toStdString())->text == "Oferta atualizada",
              "premium text slot uses the standard editable text command");
        const auto imageSlot = std::find_if(editor.project().nodes.begin(), editor.project().nodes.end(),
                                            [](const auto &n) { return n.role == "premium:image:screen"; });
        check(imageSlot != editor.project().nodes.end() && imageSlot->kind == studio::NodeKind::Shape,
              "premium image slot starts as editable screen surface");
        auto slotImage = QImage(32, 32, QImage::Format_RGBA8888);
        slotImage.fill(QColor(220, 80, 60, 180));
        const auto imagePath = temp.filePath("slot.png");
        check(slotImage.save(imagePath), "create a temporary image for premium slot test");
        const auto imageSlotId = QString::fromStdString(imageSlot->id);
        editor.selectNode(imageSlotId);
        editor.updateSelected("source", imagePath);
        check(studio::findNode(editor.project(), imageSlotId.toStdString())->kind == studio::NodeKind::Shape &&
                  editor.message().contains("gerenciada"),
              "direct external paths are rejected for premium image slots");
        const auto managedImage = ui.importSlotMedia(QUrl::fromLocalFile(imagePath));
        check(!managedImage.isEmpty() && managedImage != imagePath &&
                  managedImage.startsWith(temp.filePath("workspace/library/media")),
              "slot image is copied under the managed workspace media root");
        editor.updateSelected("source", managedImage);
        check(studio::findNode(editor.project(), imageSlotId.toStdString())->kind == studio::NodeKind::Image &&
                  studio::findNode(editor.project(), imageSlotId.toStdString())->source == managedImage.toStdString(),
              "image slot substitutes managed media while keeping its transform and hierarchy");
        QFile::remove(imagePath);
        check(QFile::exists(managedImage), "moving or deleting the source file does not break the workspace copy");
        const auto sameManagedImage = ui.importSlotMedia(QUrl::fromLocalFile(managedImage));
        check(sameManagedImage == managedImage, "hash-addressed slot media is reused without another copy");
        const auto premiumRoundTrip = studio::deserialize(studio::serialize(editor.project()));
        check(premiumRoundTrip.error.isEmpty() && premiumRoundTrip.project.nodes.size() == editor.project().nodes.size() &&
                  std::any_of(premiumRoundTrip.project.nodes.begin(), premiumRoundTrip.project.nodes.end(),
                              [](const auto &n) { return n.role == "premium:image:screen" && n.kind == studio::NodeKind::Image; }),
              "premium slot and media persist through project save and reopen");
        editor.undo();
        check(studio::findNode(editor.project(), imageSlotId.toStdString())->kind == studio::NodeKind::Shape,
              "premium image replacement can be undone");
        editor.redo();
        check(studio::findNode(editor.project(), imageSlotId.toStdString())->source == managedImage.toStdString(),
              "premium image replacement can be redone");
        editor.undo();
        editor.undo();
        check(studio::findNode(editor.project(), slotId.toStdString())->text != "Oferta atualizada",
              "premium text edit can be undone");
        editor.undo();
        check(std::none_of(editor.project().nodes.begin(), editor.project().nodes.end(),
                           [](const auto &n) { return n.role.starts_with("premium:"); }),
              "premium insertion is one history action");
        for (const auto &premiumId : {QStringLiteral("premium:title-reveal-01"),
                                      QStringLiteral("premium:comparison-pack-01")}) {
            check(ui.useAsset(premiumId), "registered cinematic premium can be inserted");
            const auto expectedSlots = premiumId.endsWith("title-reveal-01") ? 3 : 6;
            const auto slotCount = std::count_if(editor.project().nodes.begin(), editor.project().nodes.end(),
                [&](const auto &n) {
                    return n.role.starts_with("premium:") &&
                           (n.role.rfind("premium:title:", 0) == 0 ||
                            n.role.rfind("premium:comparison:", 0) == 0);
                });
            check(slotCount == expectedSlots, "inserted cinematic template exposes its declared editable slots");
            if (premiumId.endsWith("comparison-pack-01")) {
                auto rightValue = std::find_if(editor.project().nodes.begin(), editor.project().nodes.end(),
                    [](const auto &n) { return n.role == "premium:comparison:right-value"; });
                check(rightValue != editor.project().nodes.end(), "Comparison Pack has a labeled right-value slot");
                const auto id = QString::fromStdString(rightValue->id);
                editor.selectNode(id);
                editor.updateSelected("text", "Valor verificado");
                check(studio::findNode(editor.project(), id.toStdString())->text == "Valor verificado",
                      "comparison slots use normal editable and undoable project text");
                editor.undo();
            }
            editor.undo();
        }
        check(ui.useAsset("premium:smartphone-premium-01"), "smartphone premium can be reinserted as a fresh instance");
        const auto reinsertedSlot = std::find_if(editor.project().nodes.begin(), editor.project().nodes.end(),
            [](const auto &n) { return n.role == "premium:image:screen"; });
        check(reinsertedSlot != editor.project().nodes.end(), "reinserted smartphone recreates its image slot");
        editor.selectNode(QString::fromStdString(reinsertedSlot->id));
        editor.updateSelected("source", managedImage);
        const auto savedPremiumProject = temp.filePath("premium-workspace-media.studio.json");
        const auto savedPremium = studio::saveProject(savedPremiumProject, editor.project(), {});
        const auto reopenedPremium = studio::loadProject(savedPremiumProject);
        check(savedPremium.error.isEmpty() && reopenedPremium.error.isEmpty() &&
                  std::any_of(reopenedPremium.project.nodes.begin(), reopenedPremium.project.nodes.end(),
                      [&](const auto &n) {
                          return n.role == "premium:image:screen" && n.kind == studio::NodeKind::Image &&
                                 n.source == managedImage.toStdString() && QFile::exists(QString::fromStdString(n.source));
                      }),
              "saved and reopened projects retain the managed premium image reference");
        auto missingProject = reopenedPremium.project;
        const auto missingPath = temp.filePath("workspace/library/media/deleted-image.png");
        for (auto &n : missingProject.nodes)
            if (n.role == "premium:image:screen")
                n.source = missingPath.toStdString();
        const auto missingProjectPath = temp.filePath("premium-missing-media.studio.json");
        EditorController missingMediaEditor(nullptr, temp.filePath("missing-media-recovery"));
        WorkspaceController missingMediaWorkspace(&missingMediaEditor, temp.filePath("missing-media-workspace"));
        check(studio::saveProject(missingProjectPath, missingProject, {}).error.isEmpty() &&
                  missingMediaEditor.open(QUrl::fromLocalFile(missingProjectPath)),
              "a legacy project with missing slot media still opens without overwriting it");
        const auto missingSlot = std::find_if(missingMediaEditor.project().nodes.begin(), missingMediaEditor.project().nodes.end(),
            [](const auto &n) { return n.role == "premium:image:screen"; });
        missingMediaEditor.selectNode(QString::fromStdString(missingSlot->id));
        check(missingMediaEditor.selected()["mediaMissing"].toBool(),
              "inspector selection reports missing workspace slot media for replacement");
        check(editor.open(QUrl::fromLocalFile(path)) && editor.project().nodes.empty(),
              "premium slot fixtures leave the main legacy project test isolated");
        const auto videoPath = temp.filePath("library-video.mp4");
        QProcess videoEncoder;
        videoEncoder.start(qEnvironmentVariable("STUDIO_FFMPEG", "ffmpeg"),
                           {"-hide_banner", "-loglevel", "error", "-f", "lavfi", "-i",
                            "color=c=steelblue:s=160x90:d=1:r=30", "-c:v", "libx264", "-pix_fmt",
                            "yuv420p", "-an", "-y", videoPath});
        const bool encodedVideo = videoEncoder.waitForStarted(10000) && videoEncoder.waitForFinished(30000) &&
                                  videoEncoder.exitCode() == 0;
        check(encodedVideo, "generate a short real video file for the media-library test");
        check(ui.addAsset(QUrl::fromLocalFile(videoPath), "Clipe de mídia real", "technology"),
              "import and probe a local video into the real-media library");
        ui.setAssetTypeFilter("media");
        const auto importedVideo = named(ui.assets(), "Clipe de mídia real");
        check(importedVideo["kind"] == "video" && importedVideo["width"].toInt() == 160 &&
                  importedVideo["height"].toInt() == 90,
              "video library item preserves detected dimensions and is in the media category");
        const QJsonObject videoRecipe{{"format", "studio-dark-production"}, {"version", 3},
                                      {"catalogVersion", 1}, {"title", "Vídeo da biblioteca"},
                                      {"voice", "auto"}, {"captions", false},
                                      {"editing", QJsonObject{{"intensity", 50}, {"rhythm", "narration"}}},
                                      {"musicGain", 0}, {"effectsGain", 0},
                                      {"scenes", QJsonArray{QJsonObject{
                                          {"id", "video-scene"}, {"narration", QJsonArray{"Um clipe demonstrativo."}},
                                          {"shots", QJsonArray{QJsonObject{
                                              {"id", "video-shot"},
                                              {"camera", QJsonObject{{"x", 640}, {"y", 360}, {"zoom", 1}, {"rotation", 0}}},
                                              {"elements", QJsonArray{QJsonObject{{"id", "video-library"},
                                                                                  {"kind", "library"},
                                                                                  {"assetId", importedVideo["id"].toString()},
                                                                                  {"cue", 0}}}}}}}}}}};
        const auto parsedVideoRecipe = studio::parseRecipe(
            QJsonDocument(videoRecipe).toJson(), temp.path(),
            QJsonObject{{"assets", QJsonArray{QJsonObject::fromVariantMap(importedVideo)}}});
        check(parsedVideoRecipe.error.isEmpty() &&
                  parsedVideoRecipe.recipe.scenes.front().shots.front().elements.front().node.kind ==
                      studio::NodeKind::Video,
              "v3 AI recipe resolves a real catalogued video through kind library");
        ui.setAssetTypeFilter("");
        const auto hadDocumentBeforeVideo = ui.hasDocument();
        const auto insertedVideo = ui.useAsset(importedVideo["id"].toString());
        const auto hasInsertedVideo = std::any_of(editor.project().nodes.begin(), editor.project().nodes.end(), [&](const auto &node) {
            return node.kind == studio::NodeKind::Video && node.source == importedVideo["source"].toString().toStdString();
        });
        check(insertedVideo && editor.project().schemaVersion >= 10 && hasInsertedVideo,
              ("real video library item inserts as an editable production video layer: inserted=" +
               std::to_string(insertedVideo) + ", schema=" + std::to_string(editor.project().schemaVersion) +
               ", found=" + std::to_string(hasInsertedVideo) + ", hadDocument=" + std::to_string(hadDocumentBeforeVideo) +
               ", path=" + editor.projectPath().toStdString() + ", message=" + ui.message().toStdString()).c_str());
        editor.undo();
        const auto fontPath = QStringLiteral("C:/Windows/Fonts/arial.ttf");
        if (QFile::exists(fontPath)) {
            check(ui.addFont(QUrl::fromLocalFile(fontPath)), "import a system font into the private library");
            const auto importedFont = named(ui.assets(), "Arial");
            check(importedFont["kind"].toString() == "font" && importedFont["source"].toString() != fontPath &&
                      QFile::exists(importedFont["source"].toString()),
                  "external font is copied and catalogued independently of its original file");
            check(ui.useAsset(importedFont["id"].toString()) &&
                      editor.project().artDirection.fontFamily == "Arial",
                  "a library font can be applied to the project direction of art");
        }
        editor.updateArtDirection("primaryColor", "#d85e3f");
        check(ui.saveVisualStylePreset("Noite editorial"), "save named visual style from current direction of art");
        const auto stylePreset = named(ui.assets(), "Noite editorial");
        check(stylePreset["kind"].toString() == "visual-style" &&
                  stylePreset["style"].toMap()["primaryColor"].toString() == "#d85e3f" &&
                  ui.useAsset(stylePreset["id"].toString()) &&
                  editor.project().artDirection.primaryColor == "#d85e3f",
              "named visual style can be reapplied to the selected scope");
        const auto baseAssetCount = ui.assetCount();
        check(QFile::exists(path) && path.startsWith(temp.path()),
              "managed project lives in isolated workspace");
        check(ui.useAsset("native:car"), "use native item from complete library");
        editor.updateSelected("color", "#44aa88");
        const auto configuredCarId = editor.project().nodes.back().id;
        check(ui.saveSelection("Meu carro", "objects"), "save configured native object as reusable preset");
        auto preset = named(ui.assets(), "Meu carro");
        auto presetId = preset["id"].toString();
        check(!presetId.isEmpty() && ui.assetCount() == baseAssetCount + 1, "custom preset catalogued");
        const auto nodesBeforePresetReuse = editor.project().nodes.size();
        const auto reusedPreset = ui.useAsset(presetId);
        check(reusedPreset && editor.project().nodes.size() == nodesBeforePresetReuse + 1,
              ("reuse custom preset: " + ui.message().toStdString() + ", nodes=" +
               std::to_string(editor.project().nodes.size())).c_str());
        check(editor.project().nodes.back().color == "#44aa88" &&
                  editor.project().nodes.back().id != configuredCarId,
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
        check(ui.deleteCategory(category) && ui.assetCount() == baseAssetCount + 1, "deleting category retains its items");
        check(named(ui.assets(), "Meu carro verde")["categoryId"] == "uncategorized",
              "items reassigned to fallback category");
        check(!ui.deleteCategory("uncategorized"), "fallback category cannot be removed");
        check(ui.deleteAsset("native:car") && ui.assetCount() == baseAssetCount,
              "native item removed from visible catalogue");
        check(studio::libraryItem("car") && studio::validate(editor.project()).empty(),
              "native removal never breaks existing projects or renderer registry");
        ui.setAssetTrash(true);
        check(ui.assets().size() == 1, "removed native visible in excluded-items view");
        check(ui.restoreAsset("native:car") && ui.assetCount() == baseAssetCount + 1, "restore native item");
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
        QImage webpImage(96, 64, QImage::Format_ARGB32_Premultiplied);
        webpImage.fill(Qt::transparent);
        {
            QPainter painter(&webpImage);
            painter.fillRect(QRect(48, 0, 48, 64), QColor("#b4f078"));
            painter.fillRect(QRect(72, 16, 24, 32), QColor(239, 184, 115, 210));
        }
        const auto webpOriginal = temp.filePath("translucent.webp");
        check(webpImage.save(webpOriginal, "WEBP", 100), "Qt Image Formats can encode WebP");
        QImageReader webpReader(webpOriginal);
        const auto decodedWebp = webpReader.read();
        check(webpReader.format().toLower() == QByteArrayLiteral("webp") && !decodedWebp.isNull(),
              "Qt imageformats plugin decodes a real WebP file");
        check(decodedWebp.pixelColor(2, 2).alpha() == 0 && decodedWebp.pixelColor(60, 32).alpha() >= 240,
              "WebP decoding preserves transparent and opaque alpha regions");
        studio::CompositionPainter webpPainter;
        const auto frameBeforeWebp = webpPainter.frame(editor.project(), 120);
        check(ui.addAsset(QUrl::fromLocalFile(webpOriginal), "Imagem WebP", "objects"),
              "library validates and imports a decoded WebP image");
        const auto webpAsset = named(ui.assets(), "Imagem WebP");
        const auto webpAssetId = webpAsset["id"].toString();
        const auto webpOwned = webpAsset["source"].toString();
        check(webpAsset["kind"].toString() == "image" && QFile::exists(webpOwned) &&
                  QFileInfo(webpOwned).suffix().compare("webp", Qt::CaseInsensitive) == 0,
              "library retains its independent WebP copy");
        check(ui.useAsset(webpAssetId), "imported WebP can be inserted into an editable project");
        const auto frameWithWebp = webpPainter.frame(editor.project(), 120);
        const bool webpTransparentMatches =
            frameWithWebp.pixelColor(150, 250) == frameBeforeWebp.pixelColor(150, 250);
        const bool webpOpaqueDiffers = frameWithWebp.pixelColor(500, 250) != frameBeforeWebp.pixelColor(500, 250);
        if (!webpTransparentMatches || !webpOpaqueDiffers)
            std::cerr << "WebP samples transparent=" << frameBeforeWebp.pixelColor(150, 250).name().toStdString()
                      << "/" << frameWithWebp.pixelColor(150, 250).name().toStdString() << " opaque="
                      << frameBeforeWebp.pixelColor(500, 250).name().toStdString() << "/"
                      << frameWithWebp.pixelColor(500, 250).name().toStdString() << '\n';
        check(webpTransparentMatches && webpOpaqueDiffers,
              "CompositionPainter composites WebP transparency and visible color");
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
        const auto savedProjectNodeCount = editor.project().nodes.size();
        check(editor.save(), "save current edits");
        ui.navigate("projects");
        check(ui.renameProject(firstId, "História renomeada"), "rename project document and catalogue");
        check(ui.page() == "projects", "renaming from home preserves navigation");
        check(editor.title() == "História renomeada" &&
                  studio::loadProject(path).project.name == "História renomeada",
              "rename persists and refreshes current editor");
        const auto duplicatedProject = ui.duplicateProject(firstId);
        check(duplicatedProject && ui.projectCount() == 2, "duplicate project");
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
        check(ui.openProject(firstId) && editor.project().nodes.size() == savedProjectNodeCount,
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
        check(ui.createProject("Projeto vertical", 1080, 1920) && editor.project().width == 1080 &&
                  editor.project().height == 1920,
              "create a managed portrait project with its chosen canvas dimensions");
        check(!ui.createProject("Formato inválido", 1000, 1000), "reject unsupported new project dimensions");
        editor.addNode("text");
        editor.updateSelected("text", "PREÇO EM ALTA");
        editor.updateSelected("fontSize", 84);
        editor.updateSelected("color", "#f2b84b");
        check(ui.saveSelection("Título de impacto", "objects"),
              "save styled text as a reusable title preset through the composition library");
        const auto titlePreset = named(ui.assets(), "Título de impacto");
        const auto titlePresetId = titlePreset["id"].toString();
        check(titlePreset["kind"].toString() == "preset" && ui.useAsset(titlePresetId) &&
                  editor.project().nodes.back().text == "PREÇO EM ALTA" &&
                  editor.project().nodes.back().fontSize == 84 && editor.project().nodes.back().color == "#f2b84b",
              "reusable title preset restores its text, size and color in the editor");
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
