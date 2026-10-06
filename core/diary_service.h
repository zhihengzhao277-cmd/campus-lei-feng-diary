#ifndef DIARY_SERVICE_H
#define DIARY_SERVICE_H

#include <string>
#include <vector>

class DataManager;

struct DiaryPostPublicView
{
    std::string diaryId;
    std::string authorAccountId;
    std::string authorName;
    std::string categoryName;
    std::string serviceDate;
    double durationHours = 0.0;
    std::string place;
    std::string content;
    int likeCount = 0;
    bool likedByCurrentStudent = false;
};

class DiaryService
{
public:
    explicit DiaryService(const DataManager &dataManager);

    std::vector<DiaryPostPublicView> queryPublicFeed(
        const std::string &currentStudentId) const;

private:
    const DataManager &dataManager_;
};

#endif
