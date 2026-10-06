#ifndef OPERATION_LOG_SERVICE_H
#define OPERATION_LOG_SERVICE_H

#include "operation_log.h"

#include <string>
#include <vector>

class DataManager;

class OperationLogService
{
public:
    explicit OperationLogService(DataManager &dataManager);

    OperationLog append(
        const std::string &operatorAccountId,
        OperationType operationType,
        OperationTargetType targetType,
        const std::string &targetId,
        const std::string &description);

    std::vector<OperationLog> query() const;
    std::vector<OperationLog> query(OperationType operationType) const;
    std::vector<OperationLog> queryTarget(
        OperationTargetType targetType,
        const std::string &targetId) const;

private:
    DataManager &dataManager_;
};

#endif
