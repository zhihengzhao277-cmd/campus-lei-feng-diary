#ifndef VOLUNTEER_REVIEW_SERVICE_H
#define VOLUNTEER_REVIEW_SERVICE_H

#include <optional>
#include <string>

class DataManager;

enum class VolunteerReviewStatus
{
    Success,
    RecordNotFound,
    RecordNotPending,
    CategoryNotFound,
    InvalidFinalDuration,
    ReviewNoteRequired,
    InvalidReviewNote,
    PersistenceFailure,
    SeverePersistenceFailure
};

struct VolunteerApprovalInput
{
    std::string finalCategoryId;
    double finalDuration = 0.0;
    std::string reviewNote;
};

struct VolunteerReviewOutcome
{
    VolunteerReviewStatus status = VolunteerReviewStatus::Success;
    std::optional<double> approvalScore;

    bool succeeded() const
    {
        return status == VolunteerReviewStatus::Success;
    }
};

class VolunteerReviewService
{
public:
    explicit VolunteerReviewService(DataManager &dataManager);

    VolunteerReviewOutcome previewApproval(
        const std::string &recordId,
        const VolunteerApprovalInput &input) const;

    VolunteerReviewOutcome approve(
        const std::string &operatorAccountId,
        const std::string &recordId,
        const VolunteerApprovalInput &input);

    VolunteerReviewOutcome reject(
        const std::string &operatorAccountId,
        const std::string &recordId,
        const std::string &reviewNote);

private:
    DataManager &dataManager_;
};

#endif
