#include "data_manager.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <system_error>
#include <unordered_set>
#include <utility>
using namespace std;

namespace
{
enum class FileReplaceState
{
    Success,
    FailedUnchanged,
    PartialFailure
};

bool writePreparedFile(
    const filesystem::path &temporaryPath,
    const string &contents,
    string &errorMessage)
{
    error_code error;
    if (filesystem::exists(temporaryPath, error) || error)
    {
        errorMessage = "temporary path already exists or cannot be checked";
        return false;
    }

    ofstream file(temporaryPath, ios::binary | ios::out | ios::trunc);
    if (!file.is_open())
    {
        errorMessage = "could not open temporary file for Prepare";
        return false;
    }

    file.write(contents.data(), static_cast<streamsize>(contents.size()));
    file.flush();
    bool written = file.good();
    file.close();
    written = written && !file.fail();
    if (!written)
    {
        error.clear();
        filesystem::remove(temporaryPath, error);
        errorMessage = "could not finish writing temporary file";
        return false;
    }
    return true;
}

FileReplaceState replacePreparedFile(
    const filesystem::path &temporaryPath,
    const filesystem::path &formalPath,
    const filesystem::path &backupPath)
{
    error_code error;
    if (filesystem::exists(backupPath, error) || error)
    {
        return FileReplaceState::FailedUnchanged;
    }

    const bool hadFormalFile = filesystem::exists(formalPath, error);
    if (error)
    {
        return FileReplaceState::FailedUnchanged;
    }

    if (hadFormalFile)
    {
        filesystem::rename(formalPath, backupPath, error);
        if (error)
        {
            return FileReplaceState::FailedUnchanged;
        }
    }

    error.clear();
    filesystem::rename(temporaryPath, formalPath, error);
    if (!error)
    {
        return FileReplaceState::Success;
    }

    if (hadFormalFile)
    {
        error_code restoreError;
        filesystem::rename(backupPath, formalPath, restoreError);
        if (restoreError)
        {
            return FileReplaceState::PartialFailure;
        }
    }
    return FileReplaceState::FailedUnchanged;
}

void removeTemporaryFile(const filesystem::path &path)
{
    error_code error;
    filesystem::remove(path, error);
}

string serializeRecords(const vector<VolunteerRecord> &records)
{
    ostringstream contents;
    for (const VolunteerRecord &record : records)
    {
        ostringstream row;
        if (record.isLegacyCompatibilityRecord())
        {
            row << fixed << setprecision(2)
                << record.getRecordId() << "|"
                << record.getStudentId() << "|"
                << record.getAppliedCategoryId() << "|"
                << record.getDate() << "|"
                << record.getAppliedDuration() << "|"
                << record.getPlace() << "|"
                << record.getWitness() << "|"
                << record.getDescription() << "|"
                << static_cast<int>(record.getStatus()) << "|"
                << record.legacyScoreForSerialization();
            contents << row.str() << '\n';
            continue;
        }

        row << setprecision(numeric_limits<double>::max_digits10)
            << defaultfloat
            << record.getRecordId() << "|"
            << record.getStudentId() << "|"
            << record.getDate() << "|"
            << record.getAppliedCategoryId() << "|"
            << record.getAppliedDuration() << "|"
            << record.getPlace() << "|"
            << record.getWitness() << "|"
            << record.getDescription() << "|"
            << static_cast<int>(record.getStatus()) << "|"
            << record.getFinalCategoryId().value_or("") << "|";

        if (record.getFinalDuration().has_value())
        {
            row << *record.getFinalDuration();
        }
        row << "|"
            << record.getReviewerAccountId().value_or("") << "|"
            << record.getReviewNote().value_or("") << "|";

        if (record.getSettledCoefficient().has_value())
        {
            row << *record.getSettledCoefficient();
        }
        row << "|";
        if (record.getFinalScore().has_value())
        {
            row << fixed << setprecision(1) << *record.getFinalScore();
        }
        contents << row.str() << '\n';
    }
    return contents.str();
}

string serializeDiaries(const DataList<DiaryPost> &diaries)
{
    ostringstream contents;
    for (const DiaryPost &diary : diaries.getItems())
    {
        if (diary.usesLegacySixFieldFormat())
        {
            contents << diary.getDiaryId() << "|"
                     << diary.getStudentId() << "|"
                     << diary.getRecordId() << "|"
                     << diary.getContent() << "|"
                     << diary.getLikeCount() << "|";
        }
        else
        {
            contents << diary.getDiaryId() << "|"
                     << diary.getRecordId() << "|"
                     << diary.getTitle() << "|"
                     << diary.getContent() << "|"
                     << diaryDisplayStatusToken(diary.getDisplayStatus())
                     << "|"
                     << diary.getPublishedAt().value_or("") << "|"
                     << diary.getLikeCount() << "|";
        }

        const vector<string> &likedStudentIds =
            diary.getLikedStudentIds();
        for (size_t index = 0; index < likedStudentIds.size(); ++index)
        {
            if (index > 0)
            {
                contents << ",";
            }
            contents << likedStudentIds[index];
        }
        contents << '\n';
    }
    return contents.str();
}

bool parseNonnegativeInt(const string &text, int &value)
{
    if (text.empty())
    {
        return false;
    }
    try
    {
        size_t parsedCharacters = 0;
        const int parsed = stoi(text, &parsedCharacters);
        if (parsedCharacters != text.size() || parsed < 0)
        {
            return false;
        }
        value = parsed;
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool parseFiniteDouble(const string &text, double &value)
{
    if (text.empty())
    {
        return false;
    }

    try
    {
        size_t parsedCharacters = 0;
        value = stod(text, &parsedCharacters);
        return parsedCharacters == text.size() && isfinite(value);
    }
    catch (...)
    {
        return false;
    }
}

bool parseRecordStatus(const string &text, RecordStatus &status)
{
    try
    {
        size_t parsedCharacters = 0;
        const int value = stoi(text, &parsedCharacters);
        if (parsedCharacters != text.size())
        {
            return false;
        }
        if (value == static_cast<int>(RecordStatus::Pending))
        {
            status = RecordStatus::Pending;
            return true;
        }
        if (value == static_cast<int>(RecordStatus::Approved))
        {
            status = RecordStatus::Approved;
            return true;
        }
        if (value == static_cast<int>(RecordStatus::Rejected))
        {
            status = RecordStatus::Rejected;
            return true;
        }
    }
    catch (...)
    {
    }
    return false;
}

optional<string> optionalField(const string &text)
{
    if (text.empty())
    {
        return nullopt;
    }
    return text;
}

bool parseOptionalDouble(
    const string &text,
    optional<double> &value)
{
    if (text.empty())
    {
        value.reset();
        return true;
    }

    double parsed = 0.0;
    if (!parseFiniteDouble(text, parsed))
    {
        return false;
    }
    value = parsed;
    return true;
}

double normalizeScoreToOneDecimal(double score)
{
    return round(score * 10.0) / 10.0;
}
}

DataManager::DataManager()
    : DataManager(std::filesystem::path("data"))
{
}

DataManager::DataManager(std::filesystem::path dataRoot)
    : dataRoot_(std::move(dataRoot))
{
    initializeCategories();
}
void DataManager::initializeCategories()
{
    categories.clear();
    categories.emplace_back("C01", "劳动服务", 2.0); /// 原本push_back,AI改成emplace_back
    categories.emplace_back("C02", "环保服务", 1.5);
    categories.emplace_back("C03", "互助服务", 1.0);
}
vector<string> DataManager::split(const string &text, char delimiter) /// AI写的
{
    vector<string> result;
    string item;
    stringstream stream(text);

    while (getline(stream, item, delimiter))
    {
        result.push_back(item);
    }

    if (!text.empty() &&
        text.back() == delimiter)
    {
        result.push_back("");
    }

    return result;
}
bool DataManager::loadAll()
{
    return loadStudents() &&
           loadAdministrators() &&
           loadRecords() &&
           loadDiaries() &&
           loadOperationLogs();
}
/**
 * 保存所有数据的函数
 * 该函数依次调用各个数据保存方法，将学生、管理员、记录和日记数据全部保存
 */
void DataManager::saveAll() const
{
    saveStudents();       // 保存学生数据
    saveAdministrators(); // 保存管理员数据
    saveRecords();        // 保存记录数据
    saveDiaries();        // 保存日记数据
}
bool DataManager::loadStudents() /// AI大修
{
    ifstream file(dataRoot_ / "students.txt");
    if (!file.is_open())
    {
        return false;
    }

    string line;
    students.clear();
    while (getline(file, line))
    {
        vector<string> fields = split(line, '|');

        if (fields.size() != 5)
        {
            continue;
        }

        students.emplace_back(
            fields[0],
            fields[1],
            fields[2],
            fields[3],
            fields[4]);
    }

    return !file.bad();
}
bool DataManager::loadAdministrators() /// AI大修
{
    ifstream file(dataRoot_ / "administrators.txt");
    if (!file.is_open())
    {
        return false;
    }

    string line;
    administrators.clear();

    while (getline(file, line))
    {
        vector<string> fields = split(line, '|');

        if (fields.size() != 3)
        {
            continue;
        }

        administrators.emplace_back(fields[0], fields[1], fields[2]);
    }

    return !file.bad();
}
bool DataManager::loadRecords() /// AI大修
{
    ifstream file(dataRoot_ / "records.txt");
    if (!file.is_open())
    {
        return false;
    }

    vector<VolunteerRecord> loadedRecords;
    string line;
    while (getline(file, line))
    {
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }
        if (line.empty())
        {
            continue;
        }

        vector<string> fields = split(line, '|');
        if (fields.size() != 10 && fields.size() != 15)
        {
            return false;
        }

        RecordStatus status;
        double appliedDuration = 0.0;
        if (!parseRecordStatus(fields[8], status) ||
            !parseFiniteDouble(fields[4], appliedDuration))
        {
            return false;
        }

        if (fields.size() == 10)
        {
            double oldScore = 0.0;
            if (!parseFiniteDouble(fields[9], oldScore))
            {
                return false;
            }

            auto record = VolunteerRecord::fromLegacyFields(
                fields[0], fields[1], fields[2], fields[3], appliedDuration,
                fields[5], fields[6], fields[7], status, oldScore);
            if (!record.has_value())
            {
                return false;
            }
            loadedRecords.push_back(std::move(*record));
            continue;
        }

        optional<double> finalDuration;
        optional<double> settledCoefficient;
        optional<double> finalScore;
        if (!parseOptionalDouble(fields[10], finalDuration) ||
            !parseOptionalDouble(fields[13], settledCoefficient) ||
            !parseOptionalDouble(fields[14], finalScore))
        {
            return false;
        }

        auto record = VolunteerRecord::fromModernFields(
            fields[0], fields[1], fields[2], fields[3], appliedDuration,
            fields[5], fields[6], fields[7], status,
            optionalField(fields[9]), finalDuration,
            optionalField(fields[11]), optionalField(fields[12]),
            settledCoefficient, finalScore);
        if (!record.has_value())
        {
            return false;
        }
        loadedRecords.push_back(std::move(*record));
    }

    if (file.bad())
    {
        return false;
    }
    records = std::move(loadedRecords);
    return true;
}

bool DataManager::loadDiaries() /// AI大修
{
    ifstream file(dataRoot_ / "diaries.txt");
    if (!file.is_open())
    {
        return false;
    }

    vector<DiaryPost> loadedDiaries;
    unordered_set<string> diaryIds;
    unordered_set<string> recordIds;
    string line;
    while (getline(file, line))
    {
        vector<string> fields = split(line, '|');
        if (fields.size() != 6 && fields.size() != 8)
        {
            return false;
        }

        const bool legacyRow = fields.size() == 6;
        const string diaryId = fields[0];
        const string recordId = legacyRow ? fields[2] : fields[1];
        const string likedStudentsText =
            legacyRow ? fields[5] : fields[7];
        int likeCount = 0;
        if (diaryId.empty() || recordId.empty() ||
            !parseNonnegativeInt(
                legacyRow ? fields[4] : fields[6], likeCount) ||
            !diaryIds.insert(diaryId).second ||
            !recordIds.insert(recordId).second)
        {
            return false;
        }

        vector<string> likedStudentIds;
        if (!likedStudentsText.empty())
        {
            likedStudentIds = split(likedStudentsText, ',');
            for (const string &studentId : likedStudentIds)
            {
                if (studentId.empty())
                {
                    return false;
                }
            }
        }
        if (likedStudentIds.size() != static_cast<size_t>(likeCount))
        {
            return false;
        }

        if (legacyRow)
        {
            DiaryPost diary(
                diaryId,
                fields[1],
                recordId,
                fields[3],
                likeCount);
            for (const string &studentId : likedStudentIds)
            {
                if (!diary.addLikedStudentId(studentId))
                {
                    return false;
                }
            }
            loadedDiaries.push_back(std::move(diary));
            continue;
        }

        DiaryDisplayStatus status;
        if (!parseDiaryDisplayStatusToken(fields[4], status))
        {
            return false;
        }
        const optional<string> publishedAt = optionalField(fields[5]);
        optional<DiaryPost> diary = DiaryPost::fromModernFields(
            diaryId,
            recordId,
            fields[2],
            fields[3],
            status,
            publishedAt,
            likeCount,
            likedStudentIds);
        if (!diary.has_value())
        {
            return false;
        }
        loadedDiaries.push_back(std::move(*diary));
    }

    if (file.bad())
    {
        return false;
    }
    diaryPosts.getItems().swap(loadedDiaries);
    return true;
}

bool DataManager::loadOperationLogs()
{
    ifstream file(dataRoot_ / "operation_logs.csv", ios::binary);
    if (!file.is_open())
    {
        return false;
    }

    const string csv{
        istreambuf_iterator<char>(file),
        istreambuf_iterator<char>()};
    if (file.bad())
    {
        return false;
    }

    vector<OperationLog> loadedLogs;
    if (!OperationLogCsvCodec::parse(csv, loadedLogs))
    {
        return false;
    }

    operationLogs.clear();
    for (const OperationLog &log : loadedLogs)
    {
        operationLogs.push_back(log);
    }
    return true;
}

void DataManager::saveStudents() const /// AI大修
{
    ofstream file(dataRoot_ / "students.txt");

    for (const Student &student : students)
    {
        file
            << student.getAccountId() << "|"
            << student.getName() << "|"
            << student.getPassword() << "|"
            << student.getClassName() << "|"
            << student.getMajor()
            << '\n';
    }
}

void DataManager::saveAdministrators() const /// AI大修
{
    ofstream file(dataRoot_ / "administrators.txt");

    for (const Administrator &admin : administrators)
    {
        file
            << admin.getAccountId() << "|"
            << admin.getName() << "|"
            << admin.getPassword()
            << '\n';
    }
}

void DataManager::saveRecords() const /// AI大修
{
    ofstream file(dataRoot_ / "records.txt");
    file << serializeRecords(records);
}

RecordLogPersistenceOutcome
DataManager::saveRecordsAndOperationLogs() const
{
    const filesystem::path recordsPath = dataRoot_ / "records.txt";
    const filesystem::path logsPath = dataRoot_ / "operation_logs.csv";
    const filesystem::path recordsTemporaryPath =
        dataRoot_ / "records.txt.tmp";
    const filesystem::path logsTemporaryPath =
        dataRoot_ / "operation_logs.csv.tmp";
    const filesystem::path recordsBackupPath =
        dataRoot_ / "records.txt.bak";
    const filesystem::path logsBackupPath =
        dataRoot_ / "operation_logs.csv.bak";

    string errorMessage;
    if (!writePreparedFile(
            recordsTemporaryPath, serializeRecords(records), errorMessage))
    {
        return {
            RecordLogPersistenceStatus::PrepareFailure,
            "records.txt Prepare failed: " + errorMessage};
    }

    if (!writePreparedFile(
            logsTemporaryPath,
            OperationLogCsvCodec::serialize(operationLogs),
            errorMessage))
    {
        removeTemporaryFile(recordsTemporaryPath);
        return {
            RecordLogPersistenceStatus::PrepareFailure,
            "operation_logs.csv Prepare failed: " + errorMessage};
    }

    const FileReplaceState recordsCommit = replacePreparedFile(
        recordsTemporaryPath, recordsPath, recordsBackupPath);
    if (recordsCommit == FileReplaceState::FailedUnchanged)
    {
        removeTemporaryFile(recordsTemporaryPath);
        removeTemporaryFile(logsTemporaryPath);
        return {
            RecordLogPersistenceStatus::CommitFailure,
            "records.txt Commit failed before a formal file changed"};
    }
    if (recordsCommit == FileReplaceState::PartialFailure)
    {
        removeTemporaryFile(recordsTemporaryPath);
        removeTemporaryFile(logsTemporaryPath);
        return {
            RecordLogPersistenceStatus::SeverePartialCommit,
            "records.txt replacement failed and its backup could not be restored"};
    }

    const FileReplaceState logsCommit = replacePreparedFile(
        logsTemporaryPath, logsPath, logsBackupPath);
    if (logsCommit != FileReplaceState::Success)
    {
        removeTemporaryFile(logsTemporaryPath);
        return {
            RecordLogPersistenceStatus::SeverePartialCommit,
            "records.txt committed but operation_logs.csv Commit failed"};
    }

    removeTemporaryFile(recordsBackupPath);
    removeTemporaryFile(logsBackupPath);
    return {RecordLogPersistenceStatus::Success, ""};
}

DiaryPersistenceOutcome DataManager::saveDiaries() const
{
    const filesystem::path formalPath = dataRoot_ / "diaries.txt";
    const filesystem::path temporaryPath = dataRoot_ / "diaries.txt.tmp";
    const filesystem::path backupPath = dataRoot_ / "diaries.txt.bak";
    string errorMessage;
    if (!writePreparedFile(
            temporaryPath, serializeDiaries(diaryPosts), errorMessage))
    {
        return {
            DiaryPersistenceStatus::PrepareFailure,
            "diaries.txt Prepare failed: " + errorMessage};
    }

    const FileReplaceState commit =
        replacePreparedFile(temporaryPath, formalPath, backupPath);
    if (commit == FileReplaceState::FailedUnchanged)
    {
        removeTemporaryFile(temporaryPath);
        return {
            DiaryPersistenceStatus::CommitFailure,
            "diaries.txt Commit failed before the formal file changed"};
    }
    if (commit == FileReplaceState::PartialFailure)
    {
        removeTemporaryFile(temporaryPath);
        return {
            DiaryPersistenceStatus::SeverePartialCommit,
            "diaries.txt replacement failed and its backup could not be restored"};
    }

    removeTemporaryFile(backupPath);
    return {DiaryPersistenceStatus::Success, ""};
}

DiaryPersistenceOutcome DataManager::saveDiariesAndOperationLogs() const
{
    const filesystem::path diariesPath = dataRoot_ / "diaries.txt";
    const filesystem::path logsPath = dataRoot_ / "operation_logs.csv";
    const filesystem::path diariesTemporaryPath =
        dataRoot_ / "diaries.txt.tmp";
    const filesystem::path logsTemporaryPath =
        dataRoot_ / "operation_logs.csv.tmp";
    const filesystem::path diariesBackupPath =
        dataRoot_ / "diaries.txt.bak";
    const filesystem::path logsBackupPath =
        dataRoot_ / "operation_logs.csv.bak";

    string errorMessage;
    if (!writePreparedFile(
            diariesTemporaryPath,
            serializeDiaries(diaryPosts),
            errorMessage))
    {
        return {
            DiaryPersistenceStatus::PrepareFailure,
            "diaries.txt Prepare failed: " + errorMessage};
    }
    if (!writePreparedFile(
            logsTemporaryPath,
            OperationLogCsvCodec::serialize(operationLogs),
            errorMessage))
    {
        removeTemporaryFile(diariesTemporaryPath);
        return {
            DiaryPersistenceStatus::PrepareFailure,
            "operation_logs.csv Prepare failed: " + errorMessage};
    }

    const FileReplaceState diariesCommit = replacePreparedFile(
        diariesTemporaryPath, diariesPath, diariesBackupPath);
    if (diariesCommit == FileReplaceState::FailedUnchanged)
    {
        removeTemporaryFile(diariesTemporaryPath);
        removeTemporaryFile(logsTemporaryPath);
        return {
            DiaryPersistenceStatus::CommitFailure,
            "diaries.txt Commit failed before a formal file changed"};
    }
    if (diariesCommit == FileReplaceState::PartialFailure)
    {
        removeTemporaryFile(diariesTemporaryPath);
        removeTemporaryFile(logsTemporaryPath);
        return {
            DiaryPersistenceStatus::SeverePartialCommit,
            "diaries.txt replacement failed and its backup could not be restored"};
    }

    const FileReplaceState logsCommit = replacePreparedFile(
        logsTemporaryPath, logsPath, logsBackupPath);
    if (logsCommit != FileReplaceState::Success)
    {
        removeTemporaryFile(logsTemporaryPath);
        return {
            DiaryPersistenceStatus::SeverePartialCommit,
            "diaries.txt committed but operation_logs.csv Commit failed"};
    }

    removeTemporaryFile(diariesBackupPath);
    removeTemporaryFile(logsBackupPath);
    return {DiaryPersistenceStatus::Success, ""};
}
Student *DataManager::findStudent(const string &accountId)
{
    for (Student &student : students)
    {
        if (student.getAccountId() == accountId)
        {
            return &student;
        }
    }
    return nullptr;
}
Administrator *DataManager::findAdministrator(const string &accountId)
{
    for (Administrator &admin : administrators)
    {
        if (admin.getAccountId() == accountId)
        {
            return &admin;
        }
    }
    return nullptr;
}
VolunteerRecord *DataManager::findRecord(const string &recordId)
{
    for (VolunteerRecord &record : records)
    {
        if (record.getRecordId() == recordId)
        {
            return &record;
        }
    }
    return nullptr;
}
bool DataManager::deleteRecord(const string &recordId)
{
    for (auto it = records.begin(); it != records.end(); ++it)
    {
        if (it->getRecordId() == recordId)
        {
            records.erase(it);
            return true;
        }
    }
    return false;
}
DiaryPost *DataManager::findDiary(const string &diaryId)
{
    for (DiaryPost &diary : diaryPosts.getItems())
    {
        if (diary.getDiaryId() == diaryId)
        {
            return &diary;
        }
    }
    return nullptr;
}
const VolunteerCategory *DataManager::findCategory(const string &categoryId) const
{
    for (const VolunteerCategory &category : categories)
    {
        if (category.getCategoryId() == categoryId)
        {
            return &category;
        }
    }
    return nullptr;
}
void DataManager::addStudent(const Student &student)
{
    students.push_back(student);
}
void DataManager::addAdministrator(
    const Administrator &administrator)
{
    administrators.push_back(administrator);
}
void DataManager::addRecord(const VolunteerRecord &record)
{
    records.push_back(record);
}

void DataManager::addDiary(const DiaryPost &diary)
{
    diaryPosts.add(diary);
}

DiaryPost *DataManager::findDiaryByRecordId(const string &recordId)
{
    for (DiaryPost &diary : diaryPosts.getItems())
    {
        if (diary.getRecordId() == recordId)
        {
            return &diary;
        }
    }
    return nullptr;
}

string DataManager::generateRecordId() const /// 我的思路，AI代写（stringstream）
{
    int maxNumber = 0;

    for (const VolunteerRecord &record : records)
    {
        const string &recordId = record.getRecordId();

        if (recordId.size() > 1 &&
            recordId[0] == 'R')
        {
            try
            {
                int number =
                    stoi(recordId.substr(1));

                if (number > maxNumber)
                {
                    maxNumber = number;
                }
            }
            catch (...)
            {
                // 忽略格式异常的旧数据编号
            }
        }
    }

    for (const OperationLog &log : operationLogs)
    {
        if (log.getTargetType() != OperationTargetType::VolunteerRecord)
        {
            continue;
        }

        const string &recordId = log.getTargetId();
        if (recordId.size() > 1 && recordId[0] == 'R')
        {
            try
            {
                const int number = stoi(recordId.substr(1));
                if (number > maxNumber)
                {
                    maxNumber = number;
                }
            }
            catch (...)
            {
                // Ignore historical IDs that do not use the current sequence format.
            }
        }
    }

    int newNumber = maxNumber + 1;

    stringstream stream;

    stream
        << "R"
        << setw(4)
        << setfill('0')
        << newNumber;

    return stream.str();
}

string DataManager::generateOperationLogId() const
{
    unsigned long long maxSequence = 0;
    for (const OperationLog &log : operationLogs)
    {
        unsigned long long sequence = 0;
        if (parseOperationLogSequence(log.getLogId(), sequence) &&
            sequence > maxSequence)
        {
            maxSequence = sequence;
        }
    }

    if (maxSequence == numeric_limits<unsigned long long>::max())
    {
        throw overflow_error("OperationLog ID sequence is exhausted");
    }

    ostringstream stream;
    stream << "LOG" << setw(6) << setfill('0') << maxSequence + 1;
    return stream.str();
}

void DataManager::addOperationLog(const OperationLog &log)
{
    operationLogs.push_back(log);
}
string DataManager::generateDiaryId() const /// 我的思路，AI代写（stringstream）
{
    int maxNumber = 0;

    for (const DiaryPost &diary :
         diaryPosts.getItems())
    {
        const string &diaryId =
            diary.getDiaryId();

        if (diaryId.size() > 1 &&
            diaryId[0] == 'D')
        {
            try
            {
                int number =
                    stoi(diaryId.substr(1));

                if (number > maxNumber)
                {
                    maxNumber = number;
                }
            }
            catch (...)
            {
                // 忽略格式异常的旧数据编号
            }
        }
    }

    int newNumber = maxNumber + 1;

    stringstream stream;

    stream
        << "D"
        << setw(4)
        << setfill('0')
        << newNumber;

    return stream.str();
}
double DataManager::calculateStudentScore(const string &studentId) const
{
    double total = 0.0;
    for (const VolunteerRecord &record : records)
    {
        if (record.getStudentId() == studentId &&
            record.getStatus() == RecordStatus::Approved &&
            record.getFinalScore().has_value())
        {
            total += *record.getFinalScore();
        }
    }
    return normalizeScoreToOneDecimal(total);
}

double DataManager::calculateStudentScoreByDateRange(const string &studentId, const string &startDate, const string &endDate) const
{
    double total = 0.0;
    for (const VolunteerRecord &record : records)
    {
        if (record.getStudentId() == studentId &&
            record.getStatus() == RecordStatus::Approved &&
            record.getFinalScore().has_value() &&
            record.getDate() >= startDate && record.getDate() <= endDate)
        {
            total += *record.getFinalScore();
        }
    }
    return normalizeScoreToOneDecimal(total);
}

double DataManager::calculateStudentDurationByCategory(const string &studentId, const string &categoryId) const
{
    double totalDuration = 0.0;
    for (const VolunteerRecord &record : records)
    {
        if (
            record.getStudentId() == studentId &&
            record.getStatus() == RecordStatus::Approved &&
            record.getFinalCategoryId().has_value() &&
            *record.getFinalCategoryId() == categoryId &&
            record.getFinalDuration().has_value())
        {
            totalDuration += *record.getFinalDuration();
        }
    }
    return totalDuration;
}

vector<RankingItem> DataManager::generateRanking() const
{
    vector<RankingItem> ranking;
    for (const Student &student : students)
    {
        ranking.push_back({student.getAccountId(), student.getName(), calculateStudentScore(student.getAccountId())});
    }
    sort(ranking.begin(), ranking.end(), [](const RankingItem &left, const RankingItem &right)
         { return left.score > right.score; });
    return ranking;
}
const DataList<DiaryPost> &DataManager::getDiaries() const
{
    return diaryPosts;
}
const vector<Student> &DataManager::getStudents() const
{
    return students;
}
const vector<Administrator> &DataManager::getAdministrators() const
{
    return administrators;
}
const vector<VolunteerCategory> &DataManager::getCategories() const
{
    return categories;
}
const vector<VolunteerRecord> &DataManager::getRecords() const
{
    return records;
}
const vector<OperationLog> &DataManager::getOperationLogs() const
{
    return operationLogs;
}
