#pragma once

#include <QAbstractListModel>
#include <QList>

#include "common/Types.h"

class RecordListModel : public QAbstractListModel
{
    Q_OBJECT
public:
    enum Roles { TimeRole = Qt::UserRole + 1, ContentRole };

    explicit RecordListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setRecords(const QList<DailyRecord> &records);

private:
    QList<DailyRecord> mRecords;
};
