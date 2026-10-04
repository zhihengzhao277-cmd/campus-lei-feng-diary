#include "volunteer_record.h"
using namespace std;

VolunteerRecord::VolunteerRecord(const string &recordId, const string &studentId, const string &categoryId, const string &date, double duration, const string &place, const string &witness, const string &description, RecordStatus status, double score) : recordId(recordId), studentId(studentId), categoryId(categoryId), date(date), duration(duration), place(place), witness(witness), description(description), status(status), score(score) {}
string VolunteerRecord::getRecordId() const
{
    return recordId;
}
string VolunteerRecord::getStudentId() const
{
    return studentId;
}
string VolunteerRecord::getCategoryId() const
{
    return categoryId;
}
string VolunteerRecord::getDate() const
{
    return date;
}
double VolunteerRecord::getDuration() const
{
    return duration;
}
string VolunteerRecord::getPlace() const
{
    return place;
}
string VolunteerRecord::getWitness() const
{
    return witness;
}
string VolunteerRecord::getDescription() const
{
    return description;
}
RecordStatus VolunteerRecord::getStatus() const
{
    return status;
}
string VolunteerRecord::getStatusText() const
{
    if (status == RecordStatus::Approved)
    {
        return "Approved";
    }
    if (status == RecordStatus::Rejected)
    {
        return "Rejected";
    }
    return "Pending";
}
double VolunteerRecord::getScore() const
{
    return score;
}
void VolunteerRecord::setCategoryId(const string &value)
{
    categoryId = value;
}
void VolunteerRecord::setDate(const string &value)
{
    date = value;
}
void VolunteerRecord::setDuration(double value)
{
    duration = value;
}
void VolunteerRecord::setPlace(const string &value)
{
    place = value;
}
void VolunteerRecord::setWitness(const string &value)
{
    witness = value;
}
void VolunteerRecord::setDescription(const string &value)
{
    description = value;
}
void VolunteerRecord::approve(double finalScore)
{
    status = RecordStatus::Approved;
    score = finalScore;
}
void VolunteerRecord::reject()
{
    status = RecordStatus::Rejected;
    score = 0.0;
}
void VolunteerRecord::resubmit()
{
    status = RecordStatus::Pending;
    score = 0.0;
}