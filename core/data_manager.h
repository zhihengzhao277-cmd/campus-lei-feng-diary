#ifndef DATA_MANAGER_H
#define DATA_MANAGER_H

#include "administrator.h"
#include "student.h"
#include "volunteer_category.h"
#include "volunteer_record.h"
#include "data_list.h"
#include "diary_post.h"
#include "operation_log.h"

#include <filesystem>
#include <string>
#include <vector>

using namespace std;

struct RankingItem
{
    string studentId;
    string studentName;
    double score;
};

enum class RecordLogPersistenceStatus
{
    Success,
    PrepareFailure,
    CommitFailure,
    SeverePartialCommit
};

struct RecordLogPersistenceOutcome
{
    RecordLogPersistenceStatus status =
        RecordLogPersistenceStatus::Success;
    std::string message;

    bool succeeded() const
    {
        return status == RecordLogPersistenceStatus::Success;
    }
};

enum class DiaryPersistenceStatus
{
    Success,
    PrepareFailure,
    CommitFailure,
    SeverePartialCommit
};

struct DiaryPersistenceOutcome
{
    DiaryPersistenceStatus status = DiaryPersistenceStatus::Success;
    std::string message;

    bool succeeded() const
    {
        return status == DiaryPersistenceStatus::Success;
    }
};

class DataManager
{
private:
    std::filesystem::path dataRoot_;

    vector<Student> students;
    vector<Administrator> administrators;
    vector<VolunteerCategory> categories;
    vector<VolunteerRecord> records;
    vector<OperationLog> operationLogs;

    DataList<DiaryPost> diaryPosts;

    static vector<string> split(
        const string &text,
        char delimiter);

    void initializeCategories();

public:
    DataManager();
    explicit DataManager(std::filesystem::path dataRoot);

    bool loadAll();
    void saveAll() const;

    bool loadStudents();
    bool loadAdministrators();
    bool loadRecords();
    bool loadDiaries();
    bool loadOperationLogs();

    void saveStudents() const;
    void saveAdministrators() const;
    void saveRecords() const;
    DiaryPersistenceOutcome saveDiaries() const;

    RecordLogPersistenceOutcome saveRecordsAndOperationLogs() const;
    DiaryPersistenceOutcome saveDiariesAndOperationLogs() const;

    Student *findStudent(
        const string &accountId);

    Administrator *findAdministrator(
        const string &accountId);

    VolunteerRecord *findRecord(
        const string &recordId);

    const VolunteerCategory *findCategory(
        const string &categoryId) const;

    void addRecord(
        const VolunteerRecord &record);

    bool deleteRecord(
        const string &recordId);

    void addStudent(
        const Student &student);

    void addAdministrator(
        const Administrator &administrator);

    string generateRecordId() const;
    string generateOperationLogId() const;

    void addOperationLog(const OperationLog &log);

    double calculateStudentScore(
        const string &studentId) const;

    double calculateStudentScoreByDateRange(
        const string &studentId,
        const string &startDate,
        const string &endDate) const;

    double calculateStudentDurationByCategory(
        const string &studentId,
        const string &categoryId) const;

    vector<RankingItem> generateRanking() const;

    const vector<Student> &
    getStudents() const;

    const vector<Administrator> &
    getAdministrators() const;

    const vector<VolunteerCategory> &
    getCategories() const;

    const vector<VolunteerRecord> &
    getRecords() const;

    const vector<OperationLog> &
    getOperationLogs() const;

    void addDiary(
        const DiaryPost &diary);

    DiaryPost *findDiaryByRecordId(
        const string &recordId);

    DiaryPost *findDiary(
        const string &diaryId);

    string generateDiaryId() const;

    const DataList<DiaryPost> &
    getDiaries() const;
};

#endif
