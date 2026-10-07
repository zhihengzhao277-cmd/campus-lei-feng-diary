#ifndef DIARY_POST_H
#define DIARY_POST_H

#include <optional>
#include <string>
#include <vector>

enum class DiaryDisplayStatus
{
    PendingDisplayReview,
    Displayed,
    Rejected,
    TakenDown
};

std::string diaryDisplayStatusToken(DiaryDisplayStatus status);
bool parseDiaryDisplayStatusToken(
    const std::string &token,
    DiaryDisplayStatus &status);

class DiaryPost
{
private:
    std::string diaryId_;
    std::string legacyStudentId_;
    std::string recordId_;
    std::string title_;
    std::string content_;
    DiaryDisplayStatus displayStatus_ = DiaryDisplayStatus::PendingDisplayReview;
    std::optional<std::string> publishedAt_;
    int likeCount_ = 0;
    std::vector<std::string> likedStudentIds_;
    bool legacyCompatibility_ = false;
    bool legacySixFieldFormat_ = false;

    DiaryPost(
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
        bool legacySixFieldFormat);

public:
    // Compatibility constructor for existing callers that create legacy public posts.
    DiaryPost(
        const std::string &diaryId,
        const std::string &studentId,
        const std::string &recordId,
        const std::string &message,
        int likeCount = 0);

    static DiaryPost pending(
        const std::string &diaryId,
        const std::string &recordId,
        const std::string &title,
        const std::string &content);

    static std::optional<DiaryPost> fromModernFields(
        const std::string &diaryId,
        const std::string &recordId,
        const std::string &title,
        const std::string &content,
        DiaryDisplayStatus displayStatus,
        const std::optional<std::string> &publishedAt,
        int likeCount,
        const std::vector<std::string> &likedStudentIds);

    const std::string &getDiaryId() const;
    const std::string &getStudentId() const;
    const std::string &getRecordId() const;
    const std::string &getTitle() const;
    const std::string &getContent() const;
    const std::string &getMessage() const;
    DiaryDisplayStatus getDisplayStatus() const;
    const std::optional<std::string> &getPublishedAt() const;
    int getLikeCount() const;
    const std::vector<std::string> &getLikedStudentIds() const;
    bool isLegacyCompatibilityRecord() const;
    bool usesLegacySixFieldFormat() const;

    bool approveDisplay(const std::string &publishedAt);
    bool rejectDisplay();
    bool takeDown();
    bool hasLiked(const std::string &studentId) const;
    bool addLike(const std::string &studentId);
    bool removeLike(const std::string &studentId);
    bool addLikedStudentId(const std::string &studentId);
};

#endif
