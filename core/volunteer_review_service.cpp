#include "volunteer_review_service.h"

#include "data_manager.h"
#include "operation_log_service.h"

#include <cmath>
#include <iomanip>
#include <sstream>

namespace
{
bool reloadRecords(DataManager &dataManager)
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

bool reloadLogs(DataManager &dataManager)
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

bool restoreReviewState(DataManager &dataManager)
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

double normalizeScoreToOneDecimal(double score)
{
    return std::round(score * 10.0) / 10.0;
}

std::string formatDuration(double duration)
{
    std::ostringstream formatted;
    formatted << std::fixed << std::setprecision(1) << duration;
    return formatted.str();
}

bool validDuration(double duration)
{
    return std::isfinite(duration) && duration > 0.0 &&
           std::abs(std::remainder(duration, 0.5)) <= 1e-9;
}

struct ApprovalValidation
{
    VolunteerReviewStatus status = VolunteerReviewStatus::Success;
    const VolunteerCategory *category = nullptr;
    std::string normalizedNote;
    double finalScore = 0.0;
};

ApprovalValidation validateApproval(
    const DataManager &dataManager,
    const VolunteerRecord &record,
    const VolunteerApprovalInput &input,
    bool requireCorrectionNote)
{
    ApprovalValidation validation;
    validation.category = dataManager.findCategory(input.finalCategoryId);
    if (validation.category == nullptr)
    {
        validation.status = VolunteerReviewStatus::CategoryNotFound;
        return validation;
    }

    if (!validDuration(input.finalDuration))
    {
        validation.status = VolunteerReviewStatus::InvalidFinalDuration;
        return validation;
    }

    if (!normalizeReviewNoteText(input.reviewNote, validation.normalizedNote))
    {
        validation.status = VolunteerReviewStatus::InvalidReviewNote;
        return validation;
    }

    const bool categoryChanged =
        input.finalCategoryId != record.getAppliedCategoryId();
    const bool durationChanged =
        std::abs(input.finalDuration - record.getAppliedDuration()) > 1e-9;
    if (requireCorrectionNote &&
        (categoryChanged || durationChanged) &&
        validation.normalizedNote.empty())
    {
        validation.status = VolunteerReviewStatus::ReviewNoteRequired;
        return validation;
    }

    const double coefficient = validation.category->getCoefficient();
    if (!std::isfinite(coefficient) || coefficient <= 0.0)
    {
        validation.status = VolunteerReviewStatus::CategoryNotFound;
        validation.category = nullptr;
        return validation;
    }

    validation.finalScore = normalizeScoreToOneDecimal(
        validation.category->calculateScore(input.finalDuration));
    return validation;
}

std::string makeApprovalDescription(
    const VolunteerRecord &record,
    const VolunteerApprovalInput &input,
    const std::string &normalizedNote,
    double finalScore)
{
    std::ostringstream description;
    description << "志愿记录审核通过；最终类别："
                << input.finalCategoryId
                << "；最终时长：" << formatDuration(input.finalDuration)
                << " 小时；最终积分：" << std::fixed
                << std::setprecision(1) << finalScore;

    if (input.finalCategoryId != record.getAppliedCategoryId())
    {
        description << "；类别调整："
                    << record.getAppliedCategoryId() << "→"
                    << input.finalCategoryId;
    }
    if (std::abs(input.finalDuration - record.getAppliedDuration()) > 1e-9)
    {
        description << "；时长调整："
                    << formatDuration(record.getAppliedDuration()) << "→"
                    << formatDuration(input.finalDuration) << " 小时";
    }
    if (!normalizedNote.empty())
    {
        description << "；审核意见：" << normalizedNote;
    }
    return description.str();
}
}

VolunteerReviewService::VolunteerReviewService(DataManager &dataManager)
    : dataManager_(dataManager)
{
}

VolunteerReviewOutcome VolunteerReviewService::previewApproval(
    const std::string &recordId,
    const VolunteerApprovalInput &input) const
{
    VolunteerReviewOutcome outcome;
    const VolunteerRecord *record = dataManager_.findRecord(recordId);
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

    const ApprovalValidation validation =
        validateApproval(dataManager_, *record, input, false);
    outcome.status = validation.status;
    if (outcome.succeeded())
    {
        outcome.approvalScore = validation.finalScore;
    }
    return outcome;
}

VolunteerReviewOutcome VolunteerReviewService::approve(
    const std::string &operatorAccountId,
    const std::string &recordId,
    const VolunteerApprovalInput &input)
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

    const ApprovalValidation validation =
        validateApproval(dataManager_, *record, input, true);
    if (validation.status != VolunteerReviewStatus::Success)
    {
        outcome.status = validation.status;
        return outcome;
    }

    const std::string description = makeApprovalDescription(
        *record, input, validation.normalizedNote, validation.finalScore);
    if (!record->approve(
            operatorAccountId,
            input.finalCategoryId,
            input.finalDuration,
            validation.category->getCoefficient(),
            validation.finalScore,
            validation.normalizedNote))
    {
        outcome.status = VolunteerReviewStatus::InvalidReviewNote;
        return outcome;
    }

    return persistAuditedMutation(
        dataManager_,
        operatorAccountId,
        recordId,
        OperationType::VolunteerRecordApproved,
        description,
        validation.finalScore);
}

VolunteerReviewOutcome VolunteerReviewService::reject(
    const std::string &operatorAccountId,
    const std::string &recordId,
    const std::string &reviewNote)
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

    std::string normalizedNote;
    if (!normalizeReviewNoteText(reviewNote, normalizedNote))
    {
        outcome.status = VolunteerReviewStatus::InvalidReviewNote;
        return outcome;
    }
    if (normalizedNote.empty())
    {
        outcome.status = VolunteerReviewStatus::ReviewNoteRequired;
        return outcome;
    }

    if (!record->reject(operatorAccountId, normalizedNote))
    {
        outcome.status = VolunteerReviewStatus::InvalidReviewNote;
        return outcome;
    }

    return persistAuditedMutation(
        dataManager_,
        operatorAccountId,
        recordId,
        OperationType::VolunteerRecordRejected,
        "志愿记录审核驳回；驳回原因：" + normalizedNote,
        std::nullopt);
}
