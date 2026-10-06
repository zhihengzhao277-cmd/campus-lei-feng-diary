#include "operation_log_service.h"

#include "data_manager.h"

#include <algorithm>

using namespace std;

namespace
{
vector<const OperationLog *> newestFirst(
    const vector<const OperationLog *> &matchingLogs)
{
    vector<const OperationLog *> ordered = matchingLogs;
    stable_sort(
        ordered.begin(),
        ordered.end(),
        [](const OperationLog *left, const OperationLog *right)
        {
            return left->getOperationTime() > right->getOperationTime();
        });
    return ordered;
}

vector<OperationLog> copyLogs(
    const vector<const OperationLog *> &ordered)
{
    vector<OperationLog> result;
    result.reserve(ordered.size());
    for (const OperationLog *log : ordered)
    {
        result.push_back(*log);
    }
    return result;
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

vector<OperationLog> OperationLogService::query() const
{
    vector<const OperationLog *> matchingLogs;
    for (const OperationLog &log : dataManager_.getOperationLogs())
    {
        matchingLogs.push_back(&log);
    }
    return copyLogs(newestFirst(matchingLogs));
}

vector<OperationLog> OperationLogService::query(
    OperationType operationType) const
{
    vector<const OperationLog *> matchingLogs;
    for (const OperationLog &log : dataManager_.getOperationLogs())
    {
        if (log.getOperationType() == operationType)
        {
            matchingLogs.push_back(&log);
        }
    }
    return copyLogs(newestFirst(matchingLogs));
}

vector<OperationLog> OperationLogService::queryTarget(
    OperationTargetType targetType,
    const string &targetId) const
{
    vector<const OperationLog *> matchingLogs;
    for (const OperationLog &log : dataManager_.getOperationLogs())
    {
        if (log.getTargetType() == targetType &&
            log.getTargetId() == targetId)
        {
            matchingLogs.push_back(&log);
        }
    }
    return copyLogs(newestFirst(matchingLogs));
}
