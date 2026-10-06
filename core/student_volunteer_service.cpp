#include "student_volunteer_service.h"

#include "data_manager.h"

#include <cmath>

namespace
{
bool hasValidDuration(double durationHours)
{
    if (!std::isfinite(durationHours) || durationHours <= 0.0)
    {
        return false;
    }

    return std::abs(std::remainder(durationHours, 0.5)) <= 1e-9;
}

StudentVolunteerOutcome failure(StudentVolunteerStatus status)
{
    return {status, {}};
}
}

StudentVolunteerService::StudentVolunteerService(
    DataManager &dataManager)
    : dataManager_(dataManager)
{
}

StudentVolunteerOutcome StudentVolunteerService::submit(
    const std::string &studentId,
    const StudentVolunteerInput &input)
{
    if (dataManager_.findCategory(input.categoryId) == nullptr)
    {
        return failure(StudentVolunteerStatus::CategoryNotFound);
    }

    if (!hasValidDuration(input.durationHours))
    {
        return failure(StudentVolunteerStatus::InvalidDuration);
    }

    const std::string recordId = dataManager_.generateRecordId();
    dataManager_.addRecord(VolunteerRecord(
        recordId,
        studentId,
        input.categoryId,
        input.date,
        input.durationHours,
        input.place,
        input.witness,
        input.description,
        RecordStatus::Pending,
        0.0));

    return {StudentVolunteerStatus::Success, recordId};
}

StudentVolunteerOutcome StudentVolunteerService::modify(
    const std::string &studentId,
    const std::string &recordId,
    const StudentVolunteerInput &input)
{
    VolunteerRecord *record = dataManager_.findRecord(recordId);
    if (record == nullptr)
    {
        return failure(StudentVolunteerStatus::RecordNotFound);
    }

    if (record->getStudentId() != studentId)
    {
        return failure(StudentVolunteerStatus::NotOwner);
    }

    if (record->getStatus() == RecordStatus::Approved)
    {
        return failure(StudentVolunteerStatus::ApprovedRecordLocked);
    }

    if (dataManager_.findCategory(input.categoryId) == nullptr)
    {
        return failure(StudentVolunteerStatus::CategoryNotFound);
    }

    if (!hasValidDuration(input.durationHours))
    {
        return failure(StudentVolunteerStatus::InvalidDuration);
    }

    const bool wasRejected =
        record->getStatus() == RecordStatus::Rejected;

    record->setCategoryId(input.categoryId);
    record->setDate(input.date);
    record->setDuration(input.durationHours);
    record->setPlace(input.place);
    record->setWitness(input.witness);
    record->setDescription(input.description);

    if (wasRejected)
    {
        record->resubmit();
    }

    return {StudentVolunteerStatus::Success, recordId};
}

StudentVolunteerOutcome StudentVolunteerService::deleteRecord(
    const std::string &studentId,
    const std::string &recordId)
{
    VolunteerRecord *record = dataManager_.findRecord(recordId);
    if (record == nullptr)
    {
        return failure(StudentVolunteerStatus::RecordNotFound);
    }

    if (record->getStudentId() != studentId)
    {
        return failure(StudentVolunteerStatus::NotOwner);
    }

    if (record->getStatus() == RecordStatus::Approved)
    {
        return failure(StudentVolunteerStatus::ApprovedRecordLocked);
    }

    if (!dataManager_.deleteRecord(recordId))
    {
        return failure(StudentVolunteerStatus::RecordNotFound);
    }

    return {StudentVolunteerStatus::Success, recordId};
}
