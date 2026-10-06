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
    PersistenceFailure,
    SeverePersistenceFailure
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

    VolunteerReviewOutcome previewApprovalScore(
        const std::string &recordId) const;

    VolunteerReviewOutcome approve(
        const std::string &operatorAccountId,
        const std::string &recordId);

    VolunteerReviewOutcome reject(
        const std::string &operatorAccountId,
        const std::string &recordId);

private:
    DataManager &dataManager_;
};

#endif
