#pragma once
#include <QAbstractListModel>
#include <QHash>
#include <QString>
#include <QVariantMap>

// Stable row identity keeps active mouse gestures alive while values change.
class NodeListModel : public QAbstractListModel {
  public:
    explicit NodeListModel(QObject *parent = nullptr) : QAbstractListModel(parent) {}
    int rowCount(const QModelIndex &parent = {}) const override {
        return parent.isValid() ? 0 : rows_.size();
    }
    QVariant data(const QModelIndex &index, int role) const override {
        return index.isValid() && index.row() < rows_.size() && role == Qt::UserRole ? rows_[index.row()]
                                                                                     : QVariant{};
    }
    QHash<int, QByteArray> roleNames() const override {
        return {{Qt::UserRole, "modelData"}};
    }
    int rowForId(const QString &id) const { return rowById_.value(id, -1); }
    void sync(QVariantList rows) {
        bool same = rows.size() == rows_.size();
        for (int i = 0; same && i < rows.size(); ++i)
            same = rows[i].toMap()["id"] == rows_[i].toMap()["id"];
        if (!same) {
            beginResetModel();
            rows_ = std::move(rows);
            rebuildIndex();
            endResetModel();
            return;
        }
        for (int i = 0; i < rows.size(); ++i)
            if (rows[i] != rows_[i]) {
                rows_[i] = rows[i];
                emit dataChanged(index(i), index(i), {Qt::UserRole});
            }
    }

  private:
    void rebuildIndex() {
        rowById_.clear();
        rowById_.reserve(rows_.size());
        for (int i = 0; i < rows_.size(); ++i)
            rowById_.insert(rows_[i].toMap().value("id").toString(), i);
    }
    QVariantList rows_;
    QHash<QString, int> rowById_;
};
