//
// Created by veli on 3/4/19.
//

#include <src/util/AppUtils.h>
#include <src/util/TransferUtils.h>
#include "TransferObjectModel.h"

TransferObjectModel::TransferObjectModel(groupid groupId, QObject *parent)
        : QAbstractTableModel(parent)
{
    m_groupId = groupId;
    connect(gDatabase, &AccessDatabase::databaseChanged, this, &TransferObjectModel::databaseChanged);
    databaseChanged(SqlSelection(), ChangeType::Any);
}

int TransferObjectModel::columnCount(const QModelIndex &parent) const
{
    return ColumnNames::__itemCount;
}

int TransferObjectModel::rowCount(const QModelIndex &parent) const
{
    return m_list.size();
}

QVariant TransferObjectModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole) {
        if (orientation == Qt::Horizontal) {
            switch (section) {
                case ColumnNames::Status:
                    return tr("Status");
                case ColumnNames::FileName:
                    return tr("File name");
                case ColumnNames::Size:
                    return tr("Size");
                case ColumnNames::Directory:
                    return tr("Directory");
                default:
                    return QString("?");
            }
        } else
            return QString("%1").arg(section);
    }

    return QVariant();
}

QVariant TransferObjectModel::data(const QModelIndex &index, int role) const
{
    if (role == Qt::DisplayRole) {
        const auto &currentObject = m_list.at(index.row());

        switch (index.column()) {
            case ColumnNames::FileName:
                return currentObject.friendlyName;
            case ColumnNames::Status:
                return TransferUtils::getFlagString(currentObject.flag);
            case ColumnNames::Size:
                return TransferUtils::sizeExpression(currentObject.fileSize, false);
            case ColumnNames::Directory:
                return currentObject.directory;
            default:
                return QString("Data id %1x%2")
                        .arg(index.row())
                        .arg(index.column());
        }
    } else if (role == Qt::DecorationRole) {
        switch (index.column()) {
            case ColumnNames::FileName: {
                const auto &currentGroup = m_list.at(index.row());
                return QIcon(currentGroup.type == TransferObject::Type::Incoming
                             ? ":/icon/arrow_down"
                             : ":/icon/arrow_up");
            }
            default: {
                // do nothing
            }
        }
    }

    return QVariant();
}

const QList<TransferObject> &TransferObjectModel::list() const
{
    return m_list;
}

void TransferObjectModel::databaseChanged(const SqlSelection &change, ChangeType type)
{
    emit layoutAboutToBeChanged();

    m_list.clear();

    {
        SqlSelection selection;
        selection.setTableName(DB_TABLE_TRANSFER);
        selection.setOrderBy(DB_FIELD_TRANSFER_NAME, true);
        selection.setOrderBy(QString("%1 ASC, %2 ASC")
                                     .arg(DB_FIELD_TRANSFER_NAME)
                                     .arg(DB_FIELD_TRANSFER_DIRECTORY));
        selection.setWhere(QString("%1 = ?").arg(DB_FIELD_TRANSFER_GROUPID));
        selection.whereArgs << m_groupId;

        m_list = gDatabase->castQuery(selection, TransferObject());
    }

    if (m_list.empty()) {
        SqlSelection selection;
        selection.setTableName(DB_DIVIS_TRANSFER);
        selection.setOrderBy(QString("%1 ASC, %2 ASC")
                                     .arg(DB_FIELD_TRANSFER_NAME)
                                     .arg(DB_FIELD_TRANSFER_DIRECTORY));
        selection.setWhere(QString("%1 = ?").arg(DB_FIELD_TRANSFER_GROUPID));
        selection.whereArgs << m_groupId;

        m_list = gDatabase->castQuery(selection, TransferObject());
    }

    emit layoutChanged();
}