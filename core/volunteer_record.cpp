#include "volunteer_record.h"

#include <cmath>
#include <cctype>

namespace
{
bool hasValidDuration(double duration)
{
    return std::isfinite(duration) &&
           duration > 0.0 &&
           std::abs(std::remainder(duration, 0.5)) <= 1e-9;
}

double normalizeScore(double score)
{
    return std::round(score * 10.0) / 10.0;
}

bool hasNoSettlement(
    const std::optional<std::string> &finalCategoryId,
    const std::optional<double> &finalDuration,
    const std::optional<double> &settledCoefficient,
    const std::optional<double> &finalScore)
{
    return !finalCategoryId.has_value() &&
           !finalDuration.has_value() &&
           !settledCoefficient.has_value() &&
           !finalScore.has_value();
}
}

bool normalizeReviewNoteText(
    const std::string &input,
    std::string &normalized)
{
    if (input.find('|') != std::string::npos ||
        input.find('\r') != std::string::npos ||
        input.find('\n') != std::string::npos)
    {
        normalized.clear();
        return false;
    }

    std::size_t first = 0;
    while (first < input.size() &&
           std::isspace(static_cast<unsigned char>(input[first])))
    {
        ++first;
    }

    std::size_t last = input.size();
    while (last > first &&
           std::isspace(static_cast<unsigned char>(input[last - 1])))
    {
        --last;
    }

    normalized.assign(input, first, last - first);
    return true;
}

VolunteerRecord::VolunteerRecord(
    const std::string &recordId,
    const std::string &studentId,
    const std::string &appliedCategoryId,
    const std::string &date,
    double appliedDuration,
    const std::string &place,
    const std::string &witness,
    const std::string &description)
    : VolunteerRecord(
          recordId,
          studentId,
          appliedCategoryId,
          date,
          appliedDuration,
          place,
          witness,
          description,
          RecordStatus::Pending,
          std::nullopt,
          std::nullopt,
          std::nullopt,
          std::nullopt,
          std::nullopt,
          std::nullopt,
          false,
          0.0)
{
}

VolunteerRecord::VolunteerRecord(
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
    double legacyScore)
    : recordId_(recordId),
      studentId_(studentId),
      appliedCategoryId_(appliedCategoryId),
      date_(date),
      appliedDuration_(appliedDuration),
      place_(place),
      witness_(witness),
      description_(description),
      status_(status),
      finalCategoryId_(finalCategoryId),
      finalDuration_(finalDuration),
      reviewerAccountId_(reviewerAccountId),
      reviewNote_(reviewNote),
      settledCoefficient_(settledCoefficient),
      finalScore_(finalScore),
      legacyCompatibilityRecord_(legacyCompatibilityRecord),
      legacyScore_(legacyScore)
{
}

std::optional<VolunteerRecord> VolunteerRecord::fromLegacyFields(
    const std::string &recordId,
    const std::string &studentId,
    const std::string &categoryId,
    const std::string &date,
    double duration,
    const std::string &place,
    const std::string &witness,
    const std::string &description,
    RecordStatus status,
    double oldScore)
{
    if (!std::isfinite(oldScore) ||
        (status != RecordStatus::Pending &&
         status != RecordStatus::Approved &&
         status != RecordStatus::Rejected))
    {
        return std::nullopt;
    }

    std::optional<std::string> finalCategoryId;
    std::optional<double> finalDuration;
    std::optional<double> finalScore;
    if (status == RecordStatus::Approved)
    {
        finalCategoryId = categoryId;
        finalDuration = duration;
        finalScore = oldScore;
    }

    return VolunteerRecord(
        recordId,
        studentId,
        categoryId,
        date,
        duration,
        place,
        witness,
        description,
        status,
        finalCategoryId,
        finalDuration,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        finalScore,
        true,
        oldScore);
}

std::optional<VolunteerRecord> VolunteerRecord::fromModernFields(
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
    const std::optional<double> &finalScore)
{
    if (recordId.empty() || studentId.empty() || date.empty() ||
        appliedCategoryId.empty() || !hasValidDuration(appliedDuration))
    {
        return std::nullopt;
    }

    std::string normalizedNote;
    if (reviewNote.has_value() &&
        (!normalizeReviewNoteText(*reviewNote, normalizedNote) ||
         normalizedNote != *reviewNote))
    {
        return std::nullopt;
    }

    if (status == RecordStatus::Pending)
    {
        if (reviewerAccountId.has_value() ||
            reviewNote.has_value() ||
            !hasNoSettlement(
                finalCategoryId,
                finalDuration,
                settledCoefficient,
                finalScore))
        {
            return std::nullopt;
        }
    }
    else if (status == RecordStatus::Rejected)
    {
        if (!reviewerAccountId.has_value() ||
            reviewerAccountId->empty() ||
            !reviewNote.has_value() ||
            reviewNote->empty() ||
            !hasNoSettlement(
                finalCategoryId,
                finalDuration,
                settledCoefficient,
                finalScore))
        {
            return std::nullopt;
        }
    }
    else if (status == RecordStatus::Approved)
    {
        if (!reviewerAccountId.has_value() ||
            reviewerAccountId->empty() ||
            !finalCategoryId.has_value() ||
            finalCategoryId->empty() ||
            !finalDuration.has_value() ||
            !hasValidDuration(*finalDuration) ||
            !settledCoefficient.has_value() ||
            !std::isfinite(*settledCoefficient) ||
            *settledCoefficient <= 0.0 ||
            !finalScore.has_value() ||
            !std::isfinite(*finalScore) ||
            *finalScore < 0.0 ||
            std::abs(
                *finalScore -
                normalizeScore(*finalDuration * *settledCoefficient)) >
                1e-9)
        {
            return std::nullopt;
        }

        const bool correctedFacts =
            *finalCategoryId != appliedCategoryId ||
            std::abs(*finalDuration - appliedDuration) > 1e-9;
        if ((correctedFacts &&
             (!reviewNote.has_value() || reviewNote->empty())) ||
            (reviewNote.has_value() && reviewNote->empty()))
        {
            return std::nullopt;
        }
    }
    else
    {
        return std::nullopt;
    }

    return VolunteerRecord(
        recordId,
        studentId,
        appliedCategoryId,
        date,
        appliedDuration,
        place,
        witness,
        description,
        status,
        finalCategoryId,
        finalDuration,
        reviewerAccountId,
        reviewNote,
        settledCoefficient,
        finalScore,
        false,
        0.0);
}

std::string VolunteerRecord::getRecordId() const
{
    return recordId_;
}

std::string VolunteerRecord::getStudentId() const
{
    return studentId_;
}

std::string VolunteerRecord::getCategoryId() const
{
    return getAppliedCategoryId();
}

double VolunteerRecord::getDuration() const
{
    return getAppliedDuration();
}

double VolunteerRecord::getScore() const
{
    return getFinalScore().value_or(0.0);
}

const std::string &VolunteerRecord::getAppliedCategoryId() const
{
    return appliedCategoryId_;
}

double VolunteerRecord::getAppliedDuration() const
{
    return appliedDuration_;
}

const std::optional<std::string> &
VolunteerRecord::getFinalCategoryId() const
{
    return finalCategoryId_;
}

const std::optional<double> &VolunteerRecord::getFinalDuration() const
{
    return finalDuration_;
}

const std::optional<std::string> &
VolunteerRecord::getReviewerAccountId() const
{
    return reviewerAccountId_;
}

const std::optional<std::string> &VolunteerRecord::getReviewNote() const
{
    return reviewNote_;
}

const std::optional<double> &
VolunteerRecord::getSettledCoefficient() const
{
    return settledCoefficient_;
}

const std::optional<double> &VolunteerRecord::getFinalScore() const
{
    return finalScore_;
}

bool VolunteerRecord::isLegacyCompatibilityRecord() const
{
    return legacyCompatibilityRecord_;
}

double VolunteerRecord::legacyScoreForSerialization() const
{
    return legacyScore_;
}

std::string VolunteerRecord::getDate() const
{
    return date_;
}

std::string VolunteerRecord::getPlace() const
{
    return place_;
}

std::string VolunteerRecord::getWitness() const
{
    return witness_;
}

std::string VolunteerRecord::getDescription() const
{
    return description_;
}

RecordStatus VolunteerRecord::getStatus() const
{
    return status_;
}

std::string VolunteerRecord::getStatusText() const
{
    if (status_ == RecordStatus::Approved)
    {
        return "Approved";
    }
    if (status_ == RecordStatus::Rejected)
    {
        return "Rejected";
    }
    return "Pending";
}

void VolunteerRecord::setAppliedCategoryId(const std::string &value)
{
    appliedCategoryId_ = value;
    legacyCompatibilityRecord_ = false;
}

void VolunteerRecord::setCategoryId(const std::string &value)
{
    setAppliedCategoryId(value);
}

void VolunteerRecord::setDate(const std::string &value)
{
    date_ = value;
    legacyCompatibilityRecord_ = false;
}

void VolunteerRecord::setAppliedDuration(double value)
{
    appliedDuration_ = value;
    legacyCompatibilityRecord_ = false;
}

void VolunteerRecord::setDuration(double value)
{
    setAppliedDuration(value);
}

void VolunteerRecord::setPlace(const std::string &value)
{
    place_ = value;
    legacyCompatibilityRecord_ = false;
}

void VolunteerRecord::setWitness(const std::string &value)
{
    witness_ = value;
    legacyCompatibilityRecord_ = false;
}

void VolunteerRecord::setDescription(const std::string &value)
{
    description_ = value;
    legacyCompatibilityRecord_ = false;
}

bool VolunteerRecord::approve(
    const std::string &reviewerAccountId,
    const std::string &finalCategoryId,
    double finalDuration,
    double settledCoefficient,
    double finalScore,
    const std::string &reviewNote)
{
    if (status_ != RecordStatus::Pending ||
        reviewerAccountId.empty() ||
        finalCategoryId.empty() ||
        !hasValidDuration(finalDuration) ||
        !std::isfinite(settledCoefficient) ||
        settledCoefficient <= 0.0 ||
        !std::isfinite(finalScore) ||
        finalScore < 0.0 ||
        std::abs(finalScore - normalizeScore(finalDuration * settledCoefficient)) > 1e-9)
    {
        return false;
    }

    std::string normalizedNote;
    if (!normalizeReviewNoteText(reviewNote, normalizedNote) ||
        normalizedNote != reviewNote ||
        ((finalCategoryId != appliedCategoryId_ ||
          std::abs(finalDuration - appliedDuration_) > 1e-9) &&
         normalizedNote.empty()))
    {
        return false;
    }

    status_ = RecordStatus::Approved;
    finalCategoryId_ = finalCategoryId;
    finalDuration_ = finalDuration;
    reviewerAccountId_ = reviewerAccountId;
    reviewNote_ = normalizedNote.empty()
                      ? std::nullopt
                      : std::optional<std::string>(normalizedNote);
    settledCoefficient_ = settledCoefficient;
    finalScore_ = finalScore;
    legacyCompatibilityRecord_ = false;
    legacyScore_ = 0.0;
    return true;
}

bool VolunteerRecord::reject(
    const std::string &reviewerAccountId,
    const std::string &reviewNote)
{
    if (status_ != RecordStatus::Pending || reviewerAccountId.empty())
    {
        return false;
    }

    std::string normalizedNote;
    if (!normalizeReviewNoteText(reviewNote, normalizedNote) ||
        normalizedNote.empty())
    {
        return false;
    }

    status_ = RecordStatus::Rejected;
    clearCurrentReviewAndSettlement();
    reviewerAccountId_ = reviewerAccountId;
    reviewNote_ = normalizedNote;
    legacyCompatibilityRecord_ = false;
    legacyScore_ = 0.0;
    return true;
}

bool VolunteerRecord::resubmit()
{
    if (status_ != RecordStatus::Rejected)
    {
        return false;
    }

    clearCurrentReviewAndSettlement();
    status_ = RecordStatus::Pending;
    legacyCompatibilityRecord_ = false;
    legacyScore_ = 0.0;
    return true;
}

void VolunteerRecord::clearCurrentReviewAndSettlement()
{
    finalCategoryId_.reset();
    finalDuration_.reset();
    reviewerAccountId_.reset();
    reviewNote_.reset();
    settledCoefficient_.reset();
    finalScore_.reset();
}
