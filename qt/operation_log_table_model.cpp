#include "operation_log_table_model.h"

#include <QString>
#include <QStringList>
#include <utility>

using namespace std;

namespace
{
const QStringList &tableHeaders()
{
    static const QStringList headers = {
        QStringLiteral("日志编号"),
        QStringLiteral("操作时间"),
        QStringLiteral("操作管理员"),
        QStringLiteral("操作类型"),
        QStringLiteral("目标类型"),
        QStringLiteral("目标编号"),
        QStringLiteral("描述")};
    return headers;
}

QString operationTypeText(OperationType type)
{
    switch (type)
    {
    case OperationType::VolunteerRecordApproved:
        return QStringLiteral("审核通过");
    case OperationType::VolunteerRecordRejected:
        return QStringLiteral("审核驳回");
    case OperationType::DiaryDisplayApproved:
        return QStringLiteral("日记展示审核通过");
    case OperationType::DiaryDisplayRejected:
        return QStringLiteral("日记展示审核未通过");
    case OperationType::DiaryTakenDown:
        return QStringLiteral("日记下架");
    }
    return QString();
}

QString targetTypeText(OperationTargetType type)
{
    switch (type)
    {
    case OperationTargetType::VolunteerRecord:
        return QStringLiteral("志愿记录");
    case OperationTargetType::DiaryPost:
        return QStringLiteral("志愿日记");
    }
    return QString();
}

QVariant displayValue(const OperationLogView &log, int column)
{
    switch (column)
    {
    case 0:
        return QString::fromStdString(log.logId);
    case 1:
        return QString::fromStdString(log.operationTime);
    case 2:
        return QString::fromStdString(log.operatorAccountId);
    case 3:
        return operationTypeText(log.operationType);
    case 4:
        return targetTypeText(log.targetType);
    case 5:
        return QString::fromStdString(log.targetId);
    case 6:
        return QString::fromStdString(log.description);
    default:
        return {};
    }
}
}

OperationLogTableModel::OperationLogTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

int OperationLogTableModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(results_.size());
}

int OperationLogTableModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 7;
}

QVariant OperationLogTableModel::data(
    const QModelIndex &index,
    int role) const
{
    if (!index.isValid() ||
        index.row() < 0 || index.row() >= static_cast<int>(results_.size()))
    {
        return {};
    }

    if (role == Qt::ToolTipRole && index.column() == 6)
    {
        return QString::fromStdString(
            results_[static_cast<size_t>(index.row())].description);
    }

    if (role != Qt::DisplayRole)
    {
        return {};
    }

    return displayValue(
        results_[static_cast<size_t>(index.row())],
        index.column());
}

QVariant OperationLogTableModel::headerData(
    int section,
    Qt::Orientation orientation,
    int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
    {
        return QAbstractTableModel::headerData(
            section, orientation, role);
    }

    const QStringList &headers = tableHeaders();
    if (section < 0 || section >= headers.size())
    {
        return {};
    }
    return headers[section];
}

void OperationLogTableModel::setResults(
    std::vector<OperationLogView> results)
{
    beginResetModel();
    results_ = std::move(results);
    endResetModel();
}
