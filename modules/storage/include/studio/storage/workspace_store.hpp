#pragma once
#include <QJsonObject>
#include <QJsonArray>
#include <QString>
#include <QStringList>
#include <functional>
#include <studio/core/project.hpp>

namespace studio {
QStringList loadWorkspaceFonts(const QString &workspaceDirectory);
QString validatePremiumManifest(const QJsonObject &manifest, const QString &packageRoot);
struct WorkspaceResult {
    QString id, error;
};
// Catalogue deletion is reversible. Referenced media and project files are retained.
class WorkspaceStore {
  public:
    explicit WorkspaceStore(QString directory);
    QString root() const {
        return root_;
    }
    QString error() const {
        return error_;
    }
    const QJsonObject &data() const {
        return data_;
    }
    bool reload();
    WorkspaceResult registerProject(const QString &path);
    WorkspaceResult createProject(Project project);
    WorkspaceResult renameProject(const QString &id, const QString &name);
    WorkspaceResult setProjectDeleted(const QString &id, bool deleted);
    WorkspaceResult addCategory(const QString &name);
    WorkspaceResult renameCategory(const QString &id, const QString &name);
    WorkspaceResult deleteCategory(const QString &id);
    WorkspaceResult addAsset(const QString &name, const QString &category, const QString &source,
                             int width = 0, int height = 0);
    WorkspaceResult addFont(const QString &family, const QString &source);
    WorkspaceResult addVisualStylePreset(const QString &name, const QJsonObject &style);
    WorkspaceResult addPreset(const QString &name, const QString &category, std::vector<Node> nodes);
    WorkspaceResult updateAsset(const QString &id, const QString &name, const QString &category);
    WorkspaceResult setAssetDeleted(const QString &id, bool deleted);
    QJsonObject project(const QString &id) const;
    QJsonObject asset(const QString &id) const;

  private:
    WorkspaceResult mutate(const std::function<WorkspaceResult(QJsonObject &)> &action);
    QString root_, error_;
    QByteArray revision_;
    QJsonObject data_;
};
} // namespace studio
