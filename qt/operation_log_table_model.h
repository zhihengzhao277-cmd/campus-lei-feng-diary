#ifndef OPERATION_LOG_TABLE_MODEL_H
#define OPERATION_LOG_TABLE_MODEL_H

#include "operation_log_service.h"

#include <QAbstractTableModel>

#include <vector>

class OperationLogTableModel final : public QAbstractTableModel
{
public:
    explicit OperationLogTableModel(QObject *parent = nullptr);

    int rowCount(
        const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(
        const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(
        const QModelIndex &index,
        int role = Qt::DisplayRole) const override;
    QVariant headerData(
        int section,
        Qt::Orientation orientation,
        int role = Qt::DisplayRole) const override;

    void setResults(std::vector<OperationLogView> results);

private:
    std::vector<OperationLogView> results_;
};

#endif
