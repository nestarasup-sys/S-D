#pragma once
#include "editor_controller.hpp"
#include <QObject>
#include <QVariantList>
#include <studio/storage/workspace_store.hpp>

class WorkspaceController : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString page READ page NOTIFY pageChanged)
    Q_PROPERTY(bool hasDocument READ hasDocument NOTIFY pageChanged)
    Q_PROPERTY(QString message READ message NOTIFY messageChanged)
    Q_PROPERTY(QVariantList projects READ projects NOTIFY projectsChanged)
    Q_PROPERTY(QVariantList assets READ assets NOTIFY assetsChanged)
    Q_PROPERTY(QVariantList categories READ categories NOTIFY assetsChanged)
    Q_PROPERTY(QVariantList assetChoices READ assetChoices NOTIFY assetsChanged)
    Q_PROPERTY(int projectCount READ projectCount NOTIFY projectsChanged)
    Q_PROPERTY(int assetCount READ assetCount NOTIFY assetsChanged)
    Q_PROPERTY(QString projectSearch READ projectSearch WRITE setProjectSearch NOTIFY projectsChanged)
    Q_PROPERTY(QString assetSearch READ assetSearch WRITE setAssetSearch NOTIFY assetsChanged)
    Q_PROPERTY(QString categoryFilter READ categoryFilter WRITE setCategoryFilter NOTIFY assetsChanged)
    Q_PROPERTY(bool projectTrash READ projectTrash WRITE setProjectTrash NOTIFY projectsChanged)
    Q_PROPERTY(bool assetTrash READ assetTrash WRITE setAssetTrash NOTIFY assetsChanged)
    Q_PROPERTY(bool canSaveSelection READ canSaveSelection NOTIFY selectionChanged)
  public:
    explicit WorkspaceController(EditorController *editor, QString directory = {}, QObject *parent = nullptr);
    QString page() const {
        return page_;
    }
    bool hasDocument() const {
        return hasDocument_;
    }
    QString message() const {
        return message_;
    }
    QVariantList projects() const;
    QVariantList assets() const;
    QVariantList categories() const;
    QVariantList assetChoices() const;
    int projectCount() const;
    int assetCount() const;
    QString projectSearch() const {
        return projectSearch_;
    }
    QString assetSearch() const {
        return assetSearch_;
    }
    QString categoryFilter() const {
        return categoryFilter_;
    }
    bool projectTrash() const {
        return projectTrash_;
    }
    bool assetTrash() const {
        return assetTrash_;
    }
    bool canSaveSelection() const;
    void setProjectSearch(QString value);
    void setAssetSearch(QString value);
    void setCategoryFilter(QString value);
    void setProjectTrash(bool value);
    void setAssetTrash(bool value);
    Q_INVOKABLE void navigate(QString page);
    Q_INVOKABLE void refresh();
    Q_INVOKABLE bool createProject(QString name, int width = 1280, int height = 720);
    Q_INVOKABLE bool openProject(QString id);
    Q_INVOKABLE bool importProject(QUrl path);
    Q_INVOKABLE bool renameProject(QString id, QString name);
    Q_INVOKABLE bool duplicateProject(QString id);
    Q_INVOKABLE bool deleteProject(QString id);
    Q_INVOKABLE bool restoreProject(QString id);
    Q_INVOKABLE bool addCategory(QString name);
    Q_INVOKABLE bool renameCategory(QString id, QString name);
    Q_INVOKABLE bool deleteCategory(QString id);
    Q_INVOKABLE bool addAsset(QUrl source, QString name, QString category);
    Q_INVOKABLE bool saveSelection(QString name, QString category);
    Q_INVOKABLE void saveAiGuide(QUrl path);
    Q_INVOKABLE bool updateAsset(QString id, QString name, QString category);
    Q_INVOKABLE bool deleteAsset(QString id);
    Q_INVOKABLE bool restoreAsset(QString id);
    Q_INVOKABLE bool useAsset(QString id);
    Q_INVOKABLE QString suggestedAssetName(QUrl source) const;
  signals:
    void pageChanged();
    void projectsChanged();
    void assetsChanged();
    void selectionChanged();
    void messageChanged();

  private:
    bool complete(studio::WorkspaceResult result, QString message, bool projects = false);
    bool busy();
    void notify(QString message);
    void rememberProject(QString path);
    void makeProjectCover(QString id);
    QString thumbnailPath(QString id) const;
    EditorController *editor_;
    studio::WorkspaceStore store_;
    QString page_{"projects"},
        message_{"Seu espaço de criação. Escolha um projeto ou comece uma nova ideia."};
    QString projectSearch_, assetSearch_, categoryFilter_;
    bool projectTrash_{false}, assetTrash_{false}, hasDocument_{false};
};
