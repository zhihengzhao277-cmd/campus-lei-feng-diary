#include "diary_post.h"

#include <algorithm>
#include <cctype>
#include <unordered_set>
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

bool containsLegacyDelimiter(const std::string &value)
{
    return value.find('|') != std::string::npos ||
           value.find('\r') != std::string::npos ||
           value.find('\n') != std::string::npos;
}

bool validDateTimeShape(const std::string &value)
{
    if (value.size() != 19 || value[4] != '-' || value[7] != '-' ||
        value[10] != 'T' || value[13] != ':' || value[16] != ':')
    {
        return false;
    }
    for (std::size_t index = 0; index < value.size(); ++index)
    {
        if (index == 4 || index == 7 || index == 10 ||
            index == 13 || index == 16)
        {
            continue;
        }
        if (value[index] < '0' || value[index] > '9')
        {
            return false;
        }
    }
    const int year = std::stoi(value.substr(0, 4));
    const int month = std::stoi(value.substr(5, 2));
    const int day = std::stoi(value.substr(8, 2));
    const int hour = std::stoi(value.substr(11, 2));
    const int minute = std::stoi(value.substr(14, 2));
    const int second = std::stoi(value.substr(17, 2));
    if (year < 1 || month < 1 || month > 12 ||
        hour > 23 || minute > 59 || second > 59)
    {
        return false;
    }
    const bool leapYear =
        (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
    const int daysByMonth[] = {
        31, leapYear ? 29 : 28, 31, 30, 31, 30,
        31, 31, 30, 31, 30, 31};
    return day >= 1 && day <= daysByMonth[month - 1];
}
}

std::string diaryDisplayStatusToken(DiaryDisplayStatus status)
{
    switch (status)
    {
    case DiaryDisplayStatus::PendingDisplayReview:
        return "PendingDisplayReview";
    case DiaryDisplayStatus::Displayed:
        return "Displayed";
    case DiaryDisplayStatus::Rejected:
        return "Rejected";
    case DiaryDisplayStatus::TakenDown:
        return "TakenDown";
    }
    return {};
}

bool parseDiaryDisplayStatusToken(
    const std::string &token,
    DiaryDisplayStatus &status)
{
    if (token == "PendingDisplayReview")
    {
        status = DiaryDisplayStatus::PendingDisplayReview;
        return true;
    }
    if (token == "Displayed")
    {
        status = DiaryDisplayStatus::Displayed;
        return true;
    }
    if (token == "Rejected")
    {
        status = DiaryDisplayStatus::Rejected;
        return true;
    }
    if (token == "TakenDown")
    {
        status = DiaryDisplayStatus::TakenDown;
        return true;
    }
    return false;
}

DiaryPost::DiaryPost(
    std::string diaryId,
    std::string legacyStudentId,
    std::string recordId,
    std::string title,
    std::string content,
    DiaryDisplayStatus displayStatus,
    std::optional<std::string> publishedAt,
    int likeCount,
    std::vector<std::string> likedStudentIds,
    bool legacyCompatibility,
    bool legacySixFieldFormat)
    : diaryId_(std::move(diaryId)),
      legacyStudentId_(std::move(legacyStudentId)),
      recordId_(std::move(recordId)),
      title_(std::move(title)),
      content_(std::move(content)),
      displayStatus_(displayStatus),
      publishedAt_(std::move(publishedAt)),
      likeCount_(likeCount),
      likedStudentIds_(std::move(likedStudentIds)),
      legacyCompatibility_(legacyCompatibility),
      legacySixFieldFormat_(legacySixFieldFormat)
{
}

DiaryPost::DiaryPost(
    const std::string &diaryId,
    const std::string &studentId,
    const std::string &recordId,
    const std::string &message,
    int likeCount)
    : DiaryPost(
          diaryId,
          studentId,
          recordId,
          "",
          message,
          DiaryDisplayStatus::Displayed,
          std::nullopt,
          likeCount,
          {},
          true,
          true)
{
}

DiaryPost DiaryPost::pending(
    const std::string &diaryId,
    const std::string &recordId,
    const std::string &title,
    const std::string &content)
{
    return DiaryPost(
        diaryId,
        "",
        recordId,
        trimWhitespace(title),
        trimWhitespace(content),
        DiaryDisplayStatus::PendingDisplayReview,
        std::nullopt,
        0,
        {},
        false,
        false);
}

std::optional<DiaryPost> DiaryPost::fromModernFields(
    const std::string &diaryId,
    const std::string &recordId,
    const std::string &title,
    const std::string &content,
    DiaryDisplayStatus displayStatus,
    const std::optional<std::string> &publishedAt,
    int likeCount,
    const std::vector<std::string> &likedStudentIds)
{
    if (diaryId.empty() || recordId.empty() || likeCount < 0 ||
        diaryDisplayStatusToken(displayStatus).empty())
    {
        return std::nullopt;
    }

    const std::string trimmedTitle = trimWhitespace(title);
    const std::string trimmedContent = trimWhitespace(content);
    const bool legacyDerivedTakedown =
        trimmedTitle.empty() && !publishedAt.has_value() &&
        displayStatus == DiaryDisplayStatus::TakenDown;

    if (legacyDerivedTakedown)
    {
        if (trimmedContent.empty() || containsLegacyDelimiter(trimmedContent))
        {
            return std::nullopt;
        }
    }
    else
    {
        if (trimmedTitle.empty() || trimmedContent.empty() ||
            containsLegacyDelimiter(trimmedTitle) ||
            containsLegacyDelimiter(trimmedContent))
        {
            return std::nullopt;
        }
        if ((displayStatus == DiaryDisplayStatus::Displayed ||
             displayStatus == DiaryDisplayStatus::TakenDown) !=
                publishedAt.has_value() ||
            ((displayStatus == DiaryDisplayStatus::PendingDisplayReview ||
              displayStatus == DiaryDisplayStatus::Rejected) &&
             publishedAt.has_value()))
        {
            return std::nullopt;
        }
        if (publishedAt.has_value() &&
            !validDateTimeShape(*publishedAt))
        {
            return std::nullopt;
        }
    }

    std::unordered_set<std::string> uniqueLikes;
    for (const std::string &studentId : likedStudentIds)
    {
        if (studentId.empty() || !uniqueLikes.insert(studentId).second)
        {
            return std::nullopt;
        }
    }
    if (likedStudentIds.size() != static_cast<std::size_t>(likeCount) ||
        ((displayStatus == DiaryDisplayStatus::PendingDisplayReview ||
          displayStatus == DiaryDisplayStatus::Rejected) &&
         !likedStudentIds.empty()))
    {
        return std::nullopt;
    }

    return DiaryPost(
        diaryId,
        "",
        recordId,
        trimmedTitle,
        trimmedContent,
        displayStatus,
        publishedAt,
        likeCount,
        likedStudentIds,
        legacyDerivedTakedown,
        false);
}

const std::string &DiaryPost::getDiaryId() const
{
    return diaryId_;
}

const std::string &DiaryPost::getStudentId() const
{
    return legacyStudentId_;
}

const std::string &DiaryPost::getRecordId() const
{
    return recordId_;
}

const std::string &DiaryPost::getTitle() const
{
    return title_;
}

const std::string &DiaryPost::getContent() const
{
    return content_;
}

const std::string &DiaryPost::getMessage() const
{
    return content_;
}

DiaryDisplayStatus DiaryPost::getDisplayStatus() const
{
    return displayStatus_;
}

const std::optional<std::string> &DiaryPost::getPublishedAt() const
{
    return publishedAt_;
}

int DiaryPost::getLikeCount() const
{
    return likeCount_;
}

const std::vector<std::string> &DiaryPost::getLikedStudentIds() const
{
    return likedStudentIds_;
}

bool DiaryPost::isLegacyCompatibilityRecord() const
{
    return legacyCompatibility_;
}

bool DiaryPost::usesLegacySixFieldFormat() const
{
    return legacySixFieldFormat_ &&
           displayStatus_ == DiaryDisplayStatus::Displayed &&
           !publishedAt_.has_value();
}

bool DiaryPost::approveDisplay(const std::string &publishedAt)
{
    if (displayStatus_ != DiaryDisplayStatus::PendingDisplayReview ||
        !validDateTimeShape(publishedAt))
    {
        return false;
    }
    displayStatus_ = DiaryDisplayStatus::Displayed;
    publishedAt_ = publishedAt;
    legacyCompatibility_ = false;
    return true;
}

bool DiaryPost::rejectDisplay()
{
    if (displayStatus_ != DiaryDisplayStatus::PendingDisplayReview)
    {
        return false;
    }
    displayStatus_ = DiaryDisplayStatus::Rejected;
    publishedAt_.reset();
    return true;
}

bool DiaryPost::takeDown()
{
    if (displayStatus_ != DiaryDisplayStatus::Displayed)
    {
        return false;
    }
    displayStatus_ = DiaryDisplayStatus::TakenDown;
    return true;
}

bool DiaryPost::hasLiked(const std::string &studentId) const
{
    return std::find(
               likedStudentIds_.begin(),
               likedStudentIds_.end(),
               studentId) != likedStudentIds_.end();
}

bool DiaryPost::addLike(const std::string &studentId)
{
    if (displayStatus_ != DiaryDisplayStatus::Displayed ||
        studentId.empty() || hasLiked(studentId))
    {
        return false;
    }
    likedStudentIds_.push_back(studentId);
    ++likeCount_;
    return true;
}

bool DiaryPost::removeLike(const std::string &studentId)
{
    if (displayStatus_ != DiaryDisplayStatus::Displayed)
    {
        return false;
    }
    const auto found = std::find(
        likedStudentIds_.begin(), likedStudentIds_.end(), studentId);
    if (found == likedStudentIds_.end())
    {
        return false;
    }
    likedStudentIds_.erase(found);
    if (likeCount_ > 0)
    {
        --likeCount_;
    }
    return true;
}

bool DiaryPost::addLikedStudentId(const std::string &studentId)
{
    if (studentId.empty() || hasLiked(studentId))
    {
        return false;
    }
    likedStudentIds_.push_back(studentId);
    return true;
}
