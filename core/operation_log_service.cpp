#include "operation_log_service.h"

#include "data_manager.h"

#include <algorithm>

using namespace std;

namespace
{
OperationLogView makeView(const OperationLog &log)
{
    return {
        log.getLogId(),
        log.getOperatorAccountId(),
        log.getOperationType(),
        log.getTargetType(),
        log.getTargetId(),
        log.getDescription(),
        log.getOperationTime()};
}

bool matchesQuery(
    const OperationLog &log,
    const OperationLogQuery &query)
{
    if (query.operationType.has_value() &&
        log.getOperationType() != *query.operationType)
    {
        return false;
    }
    const std::optional<OperationTargetType> targetType =
        query.targetId.has_value() && !query.targetType.has_value()
            ? std::optional<OperationTargetType>(
                  OperationTargetType::VolunteerRecord)
            : query.targetType;
    if (targetType.has_value() && log.getTargetType() != *targetType)
    {
        return false;
    }
    return !query.targetId.has_value() ||
           log.getTargetId() == *query.targetId;
}

void sortNewestFirst(vector<OperationLogView> &results)
{
    stable_sort(
        results.begin(),
        results.end(),
        [](const OperationLogView &left, const OperationLogView &right)
        {
            return left.operationTime > right.operationTime;
        });
}
}

OperationLogService::OperationLogService(DataManager &dataManager)
    : dataManager_(dataManager)
{
}

OperationLog OperationLogService::append(
    const string &operatorAccountId,
    OperationType operationType,
    OperationTargetType targetType,
    const string &targetId,
    const string &description)
{
    OperationLog log(
        dataManager_.generateOperationLogId(),
        operatorAccountId,
        operationType,
        targetType,
        targetId,
        description,
        currentLocalOperationTime());
    dataManager_.addOperationLog(log);
    return log;
}

vector<OperationLogView> OperationLogService::query(
    const OperationLogQuery &query) const
{
    vector<OperationLogView> results;
    for (const OperationLog &log : dataManager_.getOperationLogs())
    {
        if (!matchesQuery(log, query))
        {
            continue;
        }
        results.push_back(makeView(log));
    }
    sortNewestFirst(results);
    return results;
}
