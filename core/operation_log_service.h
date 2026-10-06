#ifndef OPERATION_LOG_SERVICE_H
#define OPERATION_LOG_SERVICE_H

#include "operation_log.h"

#include <optional>
#include <string>
#include <vector>

class DataManager;

struct OperationLogView
{
    std::string logId;
    std::string operatorAccountId;
    OperationType operationType;
    OperationTargetType targetType;
    std::string targetId;
    std::string description;
    std::string operationTime;
};

struct OperationLogQuery
{
    std::optional<OperationType> operationType;
    std::optional<std::string> targetId;
};

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

    std::vector<OperationLogView> query(
        const OperationLogQuery &query = {}) const;

private:
    DataManager &dataManager_;
};

#endif
