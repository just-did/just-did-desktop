#include "RecordListModel.h"

RecordListModel::RecordListModel(QObject *parent) : QAbstractListModel(parent) {}

int RecordListModel::rowCount(const QModelIndex &) const { return mRecords.size(); }

QVariant RecordListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= mRecords.size()) return {};
    const auto &r = mRecords[index.row()];
    switch (role) {
    case TimeRole: return r.time;
    case ContentRole: return r.content;
    default: return {};
    }
}

QHash<int, QByteArray> RecordListModel::roleNames() const
{
    return {{TimeRole, "time"}, {ContentRole, "content"}};
}

void RecordListModel::setRecords(const QList<DailyRecord> &records)
{
    beginResetModel();
    mRecords = records;
    endResetModel();
}
