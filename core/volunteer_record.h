#ifndef VOLUNTEER_RECORD_H
#define VOLUNTEER_RECORD_H
#include <string>
using namespace std;

enum class RecordStatus
{
    Pending,
    Approved,
    Rejected
};
class VolunteerRecord
{
private:
    string recordId;
    string studentId;
    string categoryId;
    string date;
    double duration;
    string place;
    string witness;
    string description;
    RecordStatus status;
    double score;

public:
    VolunteerRecord(const string &recordId, const string &studentId, const string &categoryId, const string &date, double duration, const string &place, const string &witness, const string &description, RecordStatus status = RecordStatus::Pending, double score = 0.0);
    string getRecordId() const;
    string getStudentId() const;
    string getCategoryId() const;
    string getDate() const;
    double getDuration() const;
    string getPlace() const;
    string getWitness() const;
    string getDescription() const;
    RecordStatus getStatus() const;
    string getStatusText() const;
    double getScore() const;
    void setCategoryId(const string &value);
    void setDate(const string &value);
    void setDuration(double value);
    void setPlace(const string &value);
    void setWitness(const string &value);
    void setDescription(const string &value);
    void approve(double finalScore);
    void reject();
    void resubmit();
};
#endif