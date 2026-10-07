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

struct RankingItem
{
    std::string studentId;
    std::string studentName;
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

    std::vector<Student> students;
    std::vector<Administrator> administrators;
    std::vector<VolunteerCategory> categories;
    std::vector<VolunteerRecord> records;
    std::vector<OperationLog> operationLogs;

    DataList<DiaryPost> diaryPosts;

    static std::vector<std::string> split(
        const std::string &text,
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
        const std::string &accountId);

    Administrator *findAdministrator(
        const std::string &accountId);

    VolunteerRecord *findRecord(
        const std::string &recordId);

    const VolunteerCategory *findCategory(
        const std::string &categoryId) const;

    void addRecord(
        const VolunteerRecord &record);

    bool deleteRecord(
        const std::string &recordId);

    void addStudent(
        const Student &student);

    void addAdministrator(
        const Administrator &administrator);

    std::string generateRecordId() const;
    std::string generateOperationLogId() const;

    void addOperationLog(const OperationLog &log);

    double calculateStudentScore(
        const std::string &studentId) const;

    double calculateStudentScoreByDateRange(
        const std::string &studentId,
        const std::string &startDate,
        const std::string &endDate) const;

    double calculateStudentDurationByCategory(
        const std::string &studentId,
        const std::string &categoryId) const;

    std::vector<RankingItem> generateRanking() const;

    const std::vector<Student> &
    getStudents() const;

    const std::vector<Administrator> &
    getAdministrators() const;

    const std::vector<VolunteerCategory> &
    getCategories() const;

    const std::vector<VolunteerRecord> &
    getRecords() const;

    const std::vector<OperationLog> &
    getOperationLogs() const;

    void addDiary(
        const DiaryPost &diary);

    DiaryPost *findDiaryByRecordId(
        const std::string &recordId);

    DiaryPost *findDiary(
        const std::string &diaryId);

    std::string generateDiaryId() const;

    const DataList<DiaryPost> &
    getDiaries() const;
};

#endif
