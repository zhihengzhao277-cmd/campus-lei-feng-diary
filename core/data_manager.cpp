#include "data_manager.h"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>
using namespace std;

DataManager::DataManager()
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
void DataManager::loadAll()
{
    loadStudents();
    loadAdministrators();
    loadRecords();
    loadDiaries();
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
void DataManager::loadStudents() /// AI大修
{
    ifstream file("data/students.txt");
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
}
void DataManager::loadAdministrators() /// AI大修
{
    ifstream file("data/administrators.txt");
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
}
void DataManager::loadRecords() /// AI大修
{
    ifstream file("data/records.txt");
    string line;
    records.clear();

    while (getline(file, line))
    {
        vector<string> fields = split(line, '|');

        if (fields.size() != 10)
        {
            continue;
        }

        RecordStatus status =
            static_cast<RecordStatus>(stoi(fields[8]));

        records.emplace_back(
            fields[0],
            fields[1],
            fields[2],
            fields[3],
            stod(fields[4]),
            fields[5],
            fields[6],
            fields[7],
            status,
            stod(fields[9]));
    }
}

void DataManager::loadDiaries() /// AI大修
{
    ifstream file("data/diaries.txt");
    string line;

    diaryPosts.getItems().clear();

    while (getline(file, line))
    {
        vector<string> fields = split(line, '|');

        if (fields.size() != 6)
        {
            continue;
        }

        int likeCount = stoi(fields[4]);

        DiaryPost diary(
            fields[0],
            fields[1],
            fields[2],
            fields[3],
            likeCount);

        string likedStudentsText = fields[5];

        if (!likedStudentsText.empty())
        {
            vector<string> likedStudentIds =
                split(likedStudentsText, ',');

            for (const string &studentId :
                 likedStudentIds)
            {
                diary.addLikedStudentId(studentId);
            }
        }

        diaryPosts.add(diary);
    }
}

void DataManager::saveStudents() const /// AI大修
{
    ofstream file("data/students.txt");

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
    ofstream file("data/administrators.txt");

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
    ofstream file("data/records.txt");
    file << fixed << setprecision(2);

    for (const VolunteerRecord &record : records)
    {
        file
            << record.getRecordId() << "|"
            << record.getStudentId() << "|"
            << record.getCategoryId() << "|"
            << record.getDate() << "|"
            << record.getDuration() << "|"
            << record.getPlace() << "|"
            << record.getWitness() << "|"
            << record.getDescription() << "|"
            << static_cast<int>(record.getStatus()) << "|"
            << record.getScore()
            << '\n';
    }
}

void DataManager::saveDiaries() const /// AI大修
{
    ofstream file("data/diaries.txt");

    for (const DiaryPost &diary :
         diaryPosts.getItems())
    {
        file
            << diary.getDiaryId() << "|"
            << diary.getStudentId() << "|"
            << diary.getRecordId() << "|"
            << diary.getMessage() << "|"
            << diary.getLikeCount() << "|";

        const vector<string> &likedStudentIds =
            diary.getLikedStudentIds();

        for (size_t i = 0;
             i < likedStudentIds.size();
             ++i)
        {
            file << likedStudentIds[i];

            if (i + 1 < likedStudentIds.size())
            {
                file << ",";
            }
        }

        file << '\n';
    }
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

    int newNumber = maxNumber + 1;

    stringstream stream;

    stream
        << "R"
        << setw(4)
        << setfill('0')
        << newNumber;

    return stream.str();
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
        if (record.getStudentId() == studentId && record.getStatus() == RecordStatus::Approved)
        {
            total += record.getScore();
        }
    }
    return total;
}

double DataManager::calculateStudentScoreByDateRange(const string &studentId, const string &startDate, const string &endDate) const
{
    double total = 0.0;
    for (const VolunteerRecord &record : records)
    {
        if (record.getStudentId() == studentId && record.getStatus() == RecordStatus::Approved && record.getDate() >= startDate && record.getDate() <= endDate)
        {
            total += record.getScore();
        }
    }
    return total;
}

double DataManager::calculateStudentDurationByCategory(const string &studentId, const string &categoryId) const
{
    double totalDuration = 0.0;
    for (const VolunteerRecord &record : records)
    {
        if (
            record.getStudentId() == studentId &&
            record.getCategoryId() == categoryId &&
            record.getStatus() == RecordStatus::Approved)
        {
            totalDuration += record.getDuration();
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