#ifndef DIARY_SERVICE_H
#define DIARY_SERVICE_H

#include "diary_post.h"

#include <optional>
#include <string>
#include <vector>

class DataManager;

enum class DiaryServiceStatus
{
    Success,
    StudentNotFound,
    AdministratorNotFound,
    RecordNotFound,
    RecordNotOwned,
    RecordNotApproved,
    DuplicateRecord,
    DiaryNotFound,
    InvalidTitle,
    InvalidContent,
    InvalidState,
    AlreadyLiked,
    LikeNotFound,
    PersistenceFailure,
    SeverePersistenceFailure
};

struct DiaryServiceOutcome
{
    DiaryServiceStatus status = DiaryServiceStatus::Success;
    std::string diaryId;

    bool succeeded() const
    {
        return status == DiaryServiceStatus::Success;
    }
};

struct DiaryPostPublicView
{
    std::string diaryId;
    std::string authorAccountId;
    std::string authorName;
    std::string title;
    std::string categoryName;
    std::string serviceDate;
    double durationHours = 0.0;
    double score = 0.0;
    std::string place;
    std::string content;
    std::optional<std::string> publishedAt;
    int likeCount = 0;
    bool likedByCurrentStudent = false;
};

struct DiaryApplicationView
{
    std::string diaryId;
    std::string recordId;
    std::string title;
    std::string content;
    DiaryDisplayStatus displayStatus = DiaryDisplayStatus::PendingDisplayReview;
    std::optional<std::string> publishedAt;
};

struct DiaryModerationView
{
    std::string diaryId;
    std::string recordId;
    std::string studentAccountId;
    std::string studentName;
    std::string title;
    std::string content;
    std::string categoryName;
    std::string serviceDate;
    double durationHours = 0.0;
    double score = 0.0;
    std::string place;
    DiaryDisplayStatus displayStatus = DiaryDisplayStatus::PendingDisplayReview;
    std::optional<std::string> publishedAt;
};

class DiaryService
{
public:
    explicit DiaryService(DataManager &dataManager);

    DiaryServiceOutcome requestDisplay(
        const std::string &studentId,
        const std::string &recordId,
        const std::string &title,
        const std::string &content);

    std::vector<DiaryApplicationView> queryMyApplications(
        const std::string &studentId) const;

    std::vector<DiaryModerationView> queryModeration(
        std::optional<DiaryDisplayStatus> status = std::nullopt) const;

    DiaryServiceOutcome approveDisplay(
        const std::string &adminId,
        const std::string &diaryId);

    DiaryServiceOutcome rejectDisplay(
        const std::string &adminId,
        const std::string &diaryId);

    DiaryServiceOutcome takeDown(
        const std::string &adminId,
        const std::string &diaryId);

    DiaryServiceOutcome like(
        const std::string &studentId,
        const std::string &diaryId);

    DiaryServiceOutcome unlike(
        const std::string &studentId,
        const std::string &diaryId);

    std::vector<DiaryPostPublicView> queryPublicFeed(
        const std::string &currentStudentId) const;

private:
    DataManager &dataManager_;
};

#endif
