#ifndef VOLUNTEER_RECORD_H
#define VOLUNTEER_RECORD_H

#include <optional>
#include <string>

enum class RecordStatus
{
    Pending,
    Approved,
    Rejected
};

bool normalizeReviewNoteText(
    const std::string &input,
    std::string &normalized);

class VolunteerRecord
{
public:
    VolunteerRecord(
        const std::string &recordId,
        const std::string &studentId,
        const std::string &appliedCategoryId,
        const std::string &date,
        double appliedDuration,
        const std::string &place,
        const std::string &witness,
        const std::string &description);

    static std::optional<VolunteerRecord> fromLegacyFields(
        const std::string &recordId,
        const std::string &studentId,
        const std::string &categoryId,
        const std::string &date,
        double duration,
        const std::string &place,
        const std::string &witness,
        const std::string &description,
        RecordStatus status,
        double oldScore);

    static std::optional<VolunteerRecord> fromModernFields(
        const std::string &recordId,
        const std::string &studentId,
        const std::string &date,
        const std::string &appliedCategoryId,
        double appliedDuration,
        const std::string &place,
        const std::string &witness,
        const std::string &description,
        RecordStatus status,
        const std::optional<std::string> &finalCategoryId,
        const std::optional<double> &finalDuration,
        const std::optional<std::string> &reviewerAccountId,
        const std::optional<std::string> &reviewNote,
        const std::optional<double> &settledCoefficient,
        const std::optional<double> &finalScore);

    std::string getRecordId() const;
    std::string getStudentId() const;

    // Compatibility aliases: application facts remain the legacy view.
    std::string getCategoryId() const;
    double getDuration() const;
    double getScore() const;

    const std::string &getAppliedCategoryId() const;
    double getAppliedDuration() const;
    const std::optional<std::string> &getFinalCategoryId() const;
    const std::optional<double> &getFinalDuration() const;
    const std::optional<std::string> &getReviewerAccountId() const;
    const std::optional<std::string> &getReviewNote() const;
    const std::optional<double> &getSettledCoefficient() const;
    const std::optional<double> &getFinalScore() const;

    bool isLegacyCompatibilityRecord() const;
    double legacyScoreForSerialization() const;

    std::string getDate() const;
    std::string getPlace() const;
    std::string getWitness() const;
    std::string getDescription() const;
    RecordStatus getStatus() const;
    std::string getStatusText() const;

    void setAppliedCategoryId(const std::string &value);
    void setCategoryId(const std::string &value);
    void setDate(const std::string &value);
    void setAppliedDuration(double value);
    void setDuration(double value);
    void setPlace(const std::string &value);
    void setWitness(const std::string &value);
    void setDescription(const std::string &value);

    bool approve(
        const std::string &reviewerAccountId,
        const std::string &finalCategoryId,
        double finalDuration,
        double settledCoefficient,
        double finalScore,
        const std::string &reviewNote);

    bool reject(
        const std::string &reviewerAccountId,
        const std::string &reviewNote);

    bool resubmit();

private:
    VolunteerRecord(
        const std::string &recordId,
        const std::string &studentId,
        const std::string &appliedCategoryId,
        const std::string &date,
        double appliedDuration,
        const std::string &place,
        const std::string &witness,
        const std::string &description,
        RecordStatus status,
        const std::optional<std::string> &finalCategoryId,
        const std::optional<double> &finalDuration,
        const std::optional<std::string> &reviewerAccountId,
        const std::optional<std::string> &reviewNote,
        const std::optional<double> &settledCoefficient,
        const std::optional<double> &finalScore,
        bool legacyCompatibilityRecord,
        double legacyScore);

    void clearCurrentReviewAndSettlement();

    std::string recordId_;
    std::string studentId_;
    std::string appliedCategoryId_;
    std::string date_;
    double appliedDuration_;
    std::string place_;
    std::string witness_;
    std::string description_;
    RecordStatus status_;
    std::optional<std::string> finalCategoryId_;
    std::optional<double> finalDuration_;
    std::optional<std::string> reviewerAccountId_;
    std::optional<std::string> reviewNote_;
    std::optional<double> settledCoefficient_;
    std::optional<double> finalScore_;
    bool legacyCompatibilityRecord_;
    double legacyScore_;
};

#endif
