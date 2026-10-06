#ifndef STUDENT_VOLUNTEER_SERVICE_H
#define STUDENT_VOLUNTEER_SERVICE_H

#include <string>

class DataManager;

struct StudentVolunteerInput
{
    std::string categoryId;
    std::string date;
    double durationHours = 0.0;
    std::string place;
    std::string witness;
    std::string description;
};

enum class StudentVolunteerStatus
{
    Success,
    CategoryNotFound,
    InvalidDuration,
    RecordNotFound,
    NotOwner,
    ApprovedRecordLocked
};

struct StudentVolunteerOutcome
{
    StudentVolunteerStatus status = StudentVolunteerStatus::Success;
    std::string recordId;

    bool succeeded() const
    {
        return status == StudentVolunteerStatus::Success;
    }
};

class StudentVolunteerService
{
public:
    explicit StudentVolunteerService(DataManager &dataManager);

    StudentVolunteerOutcome submit(
        const std::string &studentId,
        const StudentVolunteerInput &input);

    StudentVolunteerOutcome modify(
        const std::string &studentId,
        const std::string &recordId,
        const StudentVolunteerInput &input);

    StudentVolunteerOutcome deleteRecord(
        const std::string &studentId,
        const std::string &recordId);

private:
    DataManager &dataManager_;
};

#endif
