#include "diary_service.h"

#include "data_manager.h"

#include <utility>

DiaryService::DiaryService(const DataManager &dataManager)
    : dataManager_(dataManager)
{
}

std::vector<DiaryPostPublicView> DiaryService::queryPublicFeed(
    const std::string &currentStudentId) const
{
    std::vector<DiaryPostPublicView> feed;
    const std::vector<VolunteerRecord> &records =
        dataManager_.getRecords();
    const std::vector<Student> &students =
        dataManager_.getStudents();

    for (const DiaryPost &diary :
         dataManager_.getDiaries().getItems())
    {
        const VolunteerRecord *record = nullptr;
        for (const VolunteerRecord &candidate : records)
        {
            if (candidate.getRecordId() == diary.getRecordId())
            {
                record = &candidate;
                break;
            }
        }

        if (record == nullptr ||
            record->getStatus() != RecordStatus::Approved)
        {
            continue;
        }

        const Student *author = nullptr;
        for (const Student &candidate : students)
        {
            if (candidate.getAccountId() == record->getStudentId())
            {
                author = &candidate;
                break;
            }
        }

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
        view.categoryName = category != nullptr
            ? category->getName()
            : finalCategoryId;
        view.serviceDate = record->getDate();
        view.durationHours = finalDuration;
        view.place = record->getPlace();
        view.content = diary.getMessage();
        view.likeCount = diary.getLikeCount();
        view.likedByCurrentStudent =
            diary.hasLiked(currentStudentId);
        feed.push_back(std::move(view));
    }

    return feed;
}
