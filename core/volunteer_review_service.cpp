#include "volunteer_review_service.h"

#include "data_manager.h"
#include "operation_log_service.h"

namespace
{
bool reloadRecords(DataManager &dataManager) noexcept
{
    try
    {
        return dataManager.loadRecords();
    }
    catch (...)
    {
        return false;
    }
}

bool reloadLogs(DataManager &dataManager) noexcept
{
    try
    {
        return dataManager.loadOperationLogs();
    }
    catch (...)
    {
        return false;
    }
}

bool restoreReviewState(DataManager &dataManager) noexcept
{
    const bool recordsLoaded = reloadRecords(dataManager);
    const bool logsLoaded = reloadLogs(dataManager);
    return recordsLoaded && logsLoaded;
}

RecordLogPersistenceOutcome appendAuditAndSave(
    DataManager &dataManager,
    const std::string &operatorAccountId,
    const std::string &recordId,
    OperationType operationType,
    const std::string &description)
{
    OperationLogService operationLogService(dataManager);
    operationLogService.append(
        operatorAccountId,
        operationType,
        OperationTargetType::VolunteerRecord,
        recordId,
        description);
    return dataManager.saveRecordsAndOperationLogs();
}

VolunteerReviewOutcome failureAfterRestore(DataManager &dataManager)
{
    VolunteerReviewOutcome outcome;
    outcome.status = restoreReviewState(dataManager)
                         ? VolunteerReviewStatus::PersistenceFailure
                         : VolunteerReviewStatus::SeverePersistenceFailure;
    return outcome;
}

VolunteerReviewOutcome persistAuditedMutation(
    DataManager &dataManager,
    const std::string &operatorAccountId,
    const std::string &recordId,
    OperationType operationType,
    const std::string &description,
    std::optional<double> approvalScore)
{
    VolunteerReviewOutcome outcome;
    RecordLogPersistenceOutcome persistence;
    try
    {
        persistence = appendAuditAndSave(
            dataManager,
            operatorAccountId,
            recordId,
            operationType,
            description);
    }
    catch (...)
    {
        return failureAfterRestore(dataManager);
    }

    if (persistence.status == RecordLogPersistenceStatus::Success)
    {
        outcome.approvalScore = approvalScore;
        return outcome;
    }
    if (persistence.status == RecordLogPersistenceStatus::SeverePartialCommit)
    {
        outcome.status = VolunteerReviewStatus::SeverePersistenceFailure;
        return outcome;
    }
    return failureAfterRestore(dataManager);
}
}

VolunteerReviewService::VolunteerReviewService(DataManager &dataManager)
    : dataManager_(dataManager)
{
}

VolunteerReviewOutcome VolunteerReviewService::previewApprovalScore(
    const std::string &recordId) const
{
    VolunteerReviewOutcome outcome;
    VolunteerRecord *record = dataManager_.findRecord(recordId);
    if (record == nullptr)
    {
        outcome.status = VolunteerReviewStatus::RecordNotFound;
        return outcome;
    }

    if (record->getStatus() != RecordStatus::Pending)
    {
        outcome.status = VolunteerReviewStatus::RecordNotPending;
        return outcome;
    }

    const VolunteerCategory *category =
        dataManager_.findCategory(record->getCategoryId());
    if (category == nullptr)
    {
        outcome.status = VolunteerReviewStatus::CategoryNotFound;
        return outcome;
    }

    outcome.approvalScore =
        category->calculateScore(record->getDuration());
    return outcome;
}

VolunteerReviewOutcome VolunteerReviewService::approve(
    const std::string &operatorAccountId,
    const std::string &recordId)
{
    VolunteerReviewOutcome outcome;
    VolunteerRecord *record = dataManager_.findRecord(recordId);
    if (record == nullptr)
    {
        outcome.status = VolunteerReviewStatus::RecordNotFound;
        return outcome;
    }

    if (record->getStatus() != RecordStatus::Pending)
    {
        outcome.status = VolunteerReviewStatus::RecordNotPending;
        return outcome;
    }

    const VolunteerCategory *category =
        dataManager_.findCategory(record->getCategoryId());
    if (category == nullptr)
    {
        outcome.status = VolunteerReviewStatus::CategoryNotFound;
        return outcome;
    }

    const double score =
        category->calculateScore(record->getDuration());
    record->approve(score);
    return persistAuditedMutation(
        dataManager_,
        operatorAccountId,
        recordId,
        OperationType::VolunteerRecordApproved,
        "志愿记录审核通过",
        score);
}

VolunteerReviewOutcome VolunteerReviewService::reject(
    const std::string &operatorAccountId,
    const std::string &recordId)
{
    VolunteerReviewOutcome outcome;
    VolunteerRecord *record = dataManager_.findRecord(recordId);
    if (record == nullptr)
    {
        outcome.status = VolunteerReviewStatus::RecordNotFound;
        return outcome;
    }

    if (record->getStatus() != RecordStatus::Pending)
    {
        outcome.status = VolunteerReviewStatus::RecordNotPending;
        return outcome;
    }

    record->reject();
    return persistAuditedMutation(
        dataManager_,
        operatorAccountId,
        recordId,
        OperationType::VolunteerRecordRejected,
        "志愿记录审核驳回",
        std::nullopt);
}
