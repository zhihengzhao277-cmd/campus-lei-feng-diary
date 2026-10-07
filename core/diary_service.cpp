#include "diary_service.h"

#include "data_manager.h"
#include "operation_log_service.h"

#include <algorithm>
#include <cctype>
#include <utility>

namespace
{
std::string trimWhitespace(const std::string &value)
{
    const auto first = std::find_if_not(
        value.begin(), value.end(), [](unsigned char character)
        {
            return std::isspace(character) != 0;
        });
    const auto last = std::find_if_not(
        value.rbegin(), value.rend(), [](unsigned char character)
        {
            return std::isspace(character) != 0;
        }).base();
    if (first >= last)
    {
        return {};
    }
    return std::string(first, last);
}

bool containsUnsupportedCharacter(const std::string &value)
{
    return value.find('|') != std::string::npos ||
           value.find('\r') != std::string::npos ||
           value.find('\n') != std::string::npos;
}

DiaryServiceOutcome outcome(DiaryServiceStatus status)
{
    DiaryServiceOutcome result;
    result.status = status;
    return result;
}

DiaryServiceOutcome restoreAfterPersistenceFailure(
    DataManager &dataManager,
    bool reloadLogs)
{
    bool restored = false;
    try
    {
        restored = dataManager.loadDiaries();
        if (reloadLogs)
        {
            restored = dataManager.loadOperationLogs() && restored;
        }
    }
    catch (...)
    {
        restored = false;
    }
    return outcome(
        restored ? DiaryServiceStatus::PersistenceFailure
                 : DiaryServiceStatus::SeverePersistenceFailure);
}

DiaryServiceOutcome persistSingleDiaryMutation(
    DataManager &dataManager,
    const std::string &diaryId)
{
    try
    {
        const DiaryPersistenceOutcome persistence = dataManager.saveDiaries();
        if (persistence.succeeded())
        {
            DiaryServiceOutcome result;
            result.diaryId = diaryId;
            return result;
        }
        if (persistence.status ==
            DiaryPersistenceStatus::SeverePartialCommit)
        {
            return outcome(DiaryServiceStatus::SeverePersistenceFailure);
        }
    }
    catch (...)
    {
    }
    return restoreAfterPersistenceFailure(dataManager, false);
}

DiaryServiceOutcome persistModerationMutation(DataManager &dataManager)
{
    try
    {
        const DiaryPersistenceOutcome persistence =
            dataManager.saveDiariesAndOperationLogs();
        if (persistence.succeeded())
        {
            return {};
        }
        if (persistence.status ==
            DiaryPersistenceStatus::SeverePartialCommit)
        {
            return outcome(DiaryServiceStatus::SeverePersistenceFailure);
        }
    }
    catch (...)
    {
    }
    return restoreAfterPersistenceFailure(dataManager, true);
}

DiaryServiceOutcome appendModerationLogAndPersist(
    DataManager &dataManager,
    const std::string &adminId,
    const DiaryPost &diary,
    OperationType type,
    const std::string &action)
{
    try
    {
        OperationLogService operationLogService(dataManager);
        operationLogService.append(
            adminId,
            type,
            OperationTargetType::DiaryPost,
            diary.getDiaryId(),
            action + "；日记编号：" + diary.getDiaryId() +
                "；志愿记录：" + diary.getRecordId());
    }
    catch (...)
    {
        return restoreAfterPersistenceFailure(dataManager, true);
    }
    return persistModerationMutation(dataManager);
}

const VolunteerRecord *findApprovedRecord(
    DataManager &dataManager,
    const DiaryPost &diary)
{
    const VolunteerRecord *record =
        dataManager.findRecord(diary.getRecordId());
    if (record == nullptr || record->getStatus() != RecordStatus::Approved ||
        dataManager.findStudent(record->getStudentId()) == nullptr)
    {
        return nullptr;
    }
    return record;
}

DiaryApplicationView makeApplicationView(const DiaryPost &diary)
{
    return {
        diary.getDiaryId(),
        diary.getRecordId(),
        diary.getTitle(),
        diary.getContent(),
        diary.getDisplayStatus(),
        diary.getPublishedAt()};
}
}

DiaryService::DiaryService(DataManager &dataManager)
    : dataManager_(dataManager)
{
}

DiaryServiceOutcome DiaryService::requestDisplay(
    const std::string &studentId,
    const std::string &recordId,
    const std::string &title,
    const std::string &content)
{
    if (dataManager_.findStudent(studentId) == nullptr)
    {
        return outcome(DiaryServiceStatus::StudentNotFound);
    }

    VolunteerRecord *record = dataManager_.findRecord(recordId);
    if (record == nullptr)
    {
        return outcome(DiaryServiceStatus::RecordNotFound);
    }
    if (record->getStudentId() != studentId)
    {
        return outcome(DiaryServiceStatus::RecordNotOwned);
    }
    if (record->getStatus() != RecordStatus::Approved)
    {
        return outcome(DiaryServiceStatus::RecordNotApproved);
    }
    if (dataManager_.findDiaryByRecordId(recordId) != nullptr)
    {
        return outcome(DiaryServiceStatus::DuplicateRecord);
    }

    const std::string normalizedTitle = trimWhitespace(title);
    if (normalizedTitle.empty() ||
        containsUnsupportedCharacter(normalizedTitle))
    {
        return outcome(DiaryServiceStatus::InvalidTitle);
    }
    const std::string normalizedContent = trimWhitespace(content);
    if (normalizedContent.empty() ||
        containsUnsupportedCharacter(normalizedContent))
    {
        return outcome(DiaryServiceStatus::InvalidContent);
    }

    const std::string diaryId = dataManager_.generateDiaryId();
    dataManager_.addDiary(DiaryPost::pending(
        diaryId, recordId, normalizedTitle, normalizedContent));
    return persistSingleDiaryMutation(dataManager_, diaryId);
}

std::vector<DiaryApplicationView> DiaryService::queryMyApplications(
    const std::string &studentId) const
{
    std::vector<DiaryApplicationView> results;
    if (dataManager_.findStudent(studentId) == nullptr)
    {
        return results;
    }
    for (const DiaryPost &diary : dataManager_.getDiaries().getItems())
    {
        const VolunteerRecord *record =
            dataManager_.findRecord(diary.getRecordId());
        if (record != nullptr && record->getStudentId() == studentId)
        {
            results.push_back(makeApplicationView(diary));
        }
    }
    return results;
}

std::vector<DiaryModerationView> DiaryService::queryModeration(
    std::optional<DiaryDisplayStatus> status) const
{
    std::vector<DiaryModerationView> results;
    for (const DiaryPost &diary : dataManager_.getDiaries().getItems())
    {
        if (status.has_value() && diary.getDisplayStatus() != *status)
        {
            continue;
        }
        const VolunteerRecord *record =
            dataManager_.findRecord(diary.getRecordId());
        if (record == nullptr)
        {
            continue;
        }
        const Student *student =
            dataManager_.findStudent(record->getStudentId());
        if (student == nullptr)
        {
            continue;
        }
        const std::string finalCategoryId =
            record->getFinalCategoryId().value_or(
                record->getAppliedCategoryId());
        const double finalDuration = record->getFinalDuration().value_or(
            record->getAppliedDuration());
        const VolunteerCategory *category =
            dataManager_.findCategory(finalCategoryId);
        results.push_back({
            diary.getDiaryId(),
            diary.getRecordId(),
            student->getAccountId(),
            student->getName(),
            diary.getTitle(),
            diary.getContent(),
            category != nullptr ? category->getName() : finalCategoryId,
            record->getDate(),
            finalDuration,
            record->getFinalScore().value_or(record->getScore()),
            record->getPlace(),
            diary.getDisplayStatus(),
            diary.getPublishedAt()});
    }
    return results;
}

DiaryServiceOutcome DiaryService::approveDisplay(
    const std::string &adminId,
    const std::string &diaryId)
{
    if (dataManager_.findAdministrator(adminId) == nullptr)
    {
        return outcome(DiaryServiceStatus::AdministratorNotFound);
    }
    DiaryPost *diary = dataManager_.findDiary(diaryId);
    if (diary == nullptr)
    {
        return outcome(DiaryServiceStatus::DiaryNotFound);
    }
    if (diary->getDisplayStatus() !=
        DiaryDisplayStatus::PendingDisplayReview)
    {
        return outcome(DiaryServiceStatus::InvalidState);
    }
    if (findApprovedRecord(dataManager_, *diary) == nullptr)
    {
        return outcome(DiaryServiceStatus::RecordNotApproved);
    }
    if (!diary->approveDisplay(currentLocalOperationTime()))
    {
        return outcome(DiaryServiceStatus::InvalidState);
    }
    return appendModerationLogAndPersist(
        dataManager_,
        adminId,
        *diary,
        OperationType::DiaryDisplayApproved,
        "日记展示审核通过");
}

DiaryServiceOutcome DiaryService::rejectDisplay(
    const std::string &adminId,
    const std::string &diaryId)
{
    if (dataManager_.findAdministrator(adminId) == nullptr)
    {
        return outcome(DiaryServiceStatus::AdministratorNotFound);
    }
    DiaryPost *diary = dataManager_.findDiary(diaryId);
    if (diary == nullptr)
    {
        return outcome(DiaryServiceStatus::DiaryNotFound);
    }
    if (diary->getDisplayStatus() !=
        DiaryDisplayStatus::PendingDisplayReview)
    {
        return outcome(DiaryServiceStatus::InvalidState);
    }
    if (findApprovedRecord(dataManager_, *diary) == nullptr)
    {
        return outcome(DiaryServiceStatus::RecordNotApproved);
    }
    if (!diary->rejectDisplay())
    {
        return outcome(DiaryServiceStatus::InvalidState);
    }
    return appendModerationLogAndPersist(
        dataManager_,
        adminId,
        *diary,
        OperationType::DiaryDisplayRejected,
        "日记展示审核拒绝");
}

DiaryServiceOutcome DiaryService::takeDown(
    const std::string &adminId,
    const std::string &diaryId)
{
    if (dataManager_.findAdministrator(adminId) == nullptr)
    {
        return outcome(DiaryServiceStatus::AdministratorNotFound);
    }
    DiaryPost *diary = dataManager_.findDiary(diaryId);
    if (diary == nullptr)
    {
        return outcome(DiaryServiceStatus::DiaryNotFound);
    }
    if (!diary->takeDown())
    {
        return outcome(DiaryServiceStatus::InvalidState);
    }
    return appendModerationLogAndPersist(
        dataManager_,
        adminId,
        *diary,
        OperationType::DiaryTakenDown,
        "日记已下架");
}

DiaryServiceOutcome DiaryService::like(
    const std::string &studentId,
    const std::string &diaryId)
{
    if (dataManager_.findStudent(studentId) == nullptr)
    {
        return outcome(DiaryServiceStatus::StudentNotFound);
    }
    DiaryPost *diary = dataManager_.findDiary(diaryId);
    if (diary == nullptr)
    {
        return outcome(DiaryServiceStatus::DiaryNotFound);
    }
    if (diary->getDisplayStatus() != DiaryDisplayStatus::Displayed)
    {
        return outcome(DiaryServiceStatus::InvalidState);
    }
    if (diary->hasLiked(studentId))
    {
        return outcome(DiaryServiceStatus::AlreadyLiked);
    }
    if (!diary->addLike(studentId))
    {
        return outcome(DiaryServiceStatus::InvalidState);
    }
    return persistSingleDiaryMutation(dataManager_, diaryId);
}

DiaryServiceOutcome DiaryService::unlike(
    const std::string &studentId,
    const std::string &diaryId)
{
    if (dataManager_.findStudent(studentId) == nullptr)
    {
        return outcome(DiaryServiceStatus::StudentNotFound);
    }
    DiaryPost *diary = dataManager_.findDiary(diaryId);
    if (diary == nullptr)
    {
        return outcome(DiaryServiceStatus::DiaryNotFound);
    }
    if (diary->getDisplayStatus() != DiaryDisplayStatus::Displayed)
    {
        return outcome(DiaryServiceStatus::InvalidState);
    }
    if (!diary->hasLiked(studentId))
    {
        return outcome(DiaryServiceStatus::LikeNotFound);
    }
    if (!diary->removeLike(studentId))
    {
        return outcome(DiaryServiceStatus::InvalidState);
    }
    return persistSingleDiaryMutation(dataManager_, diaryId);
}

std::vector<DiaryPostPublicView> DiaryService::queryPublicFeed(
    const std::string &currentStudentId) const
{
    std::vector<DiaryPostPublicView> feed;
    for (const DiaryPost &diary : dataManager_.getDiaries().getItems())
    {
        if (diary.getDisplayStatus() != DiaryDisplayStatus::Displayed)
        {
            continue;
        }
        const VolunteerRecord *record =
            dataManager_.findRecord(diary.getRecordId());
        if (record == nullptr ||
            record->getStatus() != RecordStatus::Approved)
        {
            continue;
        }
        const Student *author =
            dataManager_.findStudent(record->getStudentId());
        if (author == nullptr)
        {
            continue;
        }

        const std::string finalCategoryId =
            record->getFinalCategoryId().value_or(
                record->getAppliedCategoryId());
        const double finalDuration = record->getFinalDuration().value_or(
            record->getAppliedDuration());
        const VolunteerCategory *category =
            dataManager_.findCategory(finalCategoryId);

        DiaryPostPublicView view;
        view.diaryId = diary.getDiaryId();
        view.authorAccountId = author->getAccountId();
        view.authorName = author->getName();
        view.title = diary.getTitle();
        view.categoryName = category != nullptr
            ? category->getName()
            : finalCategoryId;
        view.serviceDate = record->getDate();
        view.durationHours = finalDuration;
        view.score = record->getFinalScore().value_or(record->getScore());
        view.place = record->getPlace();
        view.content = diary.getContent();
        view.publishedAt = diary.getPublishedAt();
        view.likeCount = diary.getLikeCount();
        view.likedByCurrentStudent = diary.hasLiked(currentStudentId);
        feed.push_back(std::move(view));
    }

    std::stable_sort(
        feed.begin(), feed.end(),
        [](const DiaryPostPublicView &left,
           const DiaryPostPublicView &right)
        {
            if (left.publishedAt.has_value() !=
                right.publishedAt.has_value())
            {
                return left.publishedAt.has_value();
            }
            if (!left.publishedAt.has_value())
            {
                return false;
            }
            return *left.publishedAt > *right.publishedAt;
        });
    return feed;
}
