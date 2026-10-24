#pragma once
#include <QByteArray>
#include <QString>
#include <studio/core/project.hpp>

namespace studio {
struct LoadResult {
    Project project;
    QByteArray revision;
    QString error;
};
struct SaveResult {
    QByteArray revision;
    QString error;
};
QByteArray serialize(const Project &);
LoadResult deserialize(const QByteArray &);
LoadResult loadProject(const QString &path);
// An empty expected revision permits creating a new file, never replacing one.
SaveResult saveProject(const QString &path, const Project &, const QByteArray &expectedRevision);
} // namespace studio
