#include "volunteer_review_service.h"

#include "data_manager.h"

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
    outcome.approvalScore = score;
    return outcome;
}

VolunteerReviewOutcome VolunteerReviewService::reject(
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
    return outcome;
}
