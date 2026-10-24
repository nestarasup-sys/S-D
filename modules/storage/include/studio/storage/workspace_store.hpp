#pragma once
#include <QJsonObject>
#include <QString>
#include <functional>
#include <studio/core/project.hpp>

namespace studio {
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
    WorkspaceResult addAsset(const QString &name, const QString &category, const QString &source);
    WorkspaceResult addPreset(const QString &name, const QString &category, Node node);
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
