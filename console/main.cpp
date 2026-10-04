#include "data_manager.h"
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
using namespace std;

string readText(const string &prompt)
{
    cout << prompt;

    string value;
    getline(cin, value);

    return value;
}

double readDouble(const string &prompt)
{
    while (true)
    {
        cout << prompt;

        double value;

        if (cin >> value)
        {
            cin.ignore(
                numeric_limits<streamsize>::max(),
                '\n');

            return value;
        }

        cin.clear();

        cin.ignore(
            numeric_limits<streamsize>::max(),
            '\n');

        cout << "输入无效，请重新输入。\n";
    }
}

int readInt(const string &prompt)
{
    while (true)
    {
        cout << prompt;

        int value;

        if (cin >> value)
        {
            cin.ignore(
                numeric_limits<streamsize>::max(),
                '\n');

            return value;
        }

        cin.clear();

        cin.ignore(
            numeric_limits<streamsize>::max(),
            '\n');

        cout << "输入无效，请重新输入。\n";
    }
}

void printRecord(
    const VolunteerRecord &record,
    const DataManager &data)
{
    const VolunteerCategory *category =
        data.findCategory(record.getCategoryId());

    cout << "\n记录编号：" << record.getRecordId();
    cout << "\n学生账号：" << record.getStudentId();

    cout << "\n志愿类别：";

    if (category != nullptr)
    {
        cout << category->getName();
    }
    else
    {
        cout << record.getCategoryId();
    }

    cout << "\n服务日期：" << record.getDate();
    cout << "\n服务时长：" << record.getDuration();
    cout << "\n地点：" << record.getPlace();
    cout << "\n证明人：" << record.getWitness();
    cout << "\n简述：" << record.getDescription();
    cout << "\n状态：" << record.getStatusText();
    cout << "\n积分：" << record.getScore();
    cout << "\n-----------------------------\n";
}

void showCategories(const DataManager &data)
{
    cout << "\n===== 志愿类别 =====\n";

    for (const VolunteerCategory &category :
         data.getCategories())
    {
        cout
            << category.getCategoryId()
            << "  "
            << category.getName()
            << "  积分系数："
            << category.getCoefficient()
            << '\n';
    }
}

void showRanking(const DataManager &data)
{
    vector<RankingItem> ranking =
        data.generateRanking();

    cout << "\n===== 积分排行榜 =====\n";

    for (size_t i = 0; i < ranking.size(); ++i)
    {
        cout
            << i + 1
            << ". "
            << ranking[i].studentName
            << "  "
            << ranking[i].studentId
            << "  "
            << fixed
            << setprecision(2)
            << ranking[i].score
            << " 分\n";
    }
}

string getBadgeLevel(double duration)
{
    if (duration >= 60.0)
    {
        return "金";
    }

    if (duration >= 30.0)
    {
        return "银";
    }

    if (duration >= 10.0)
    {
        return "铜";
    }

    return "";
}

bool showCategoryBadge(
    const Student &student,
    const DataManager &data,
    const string &categoryId,
    const string &badgeName)
{
    double duration =
        data.calculateStudentDurationByCategory(
            student.getAccountId(),
            categoryId);

    string level =
        getBadgeLevel(duration);

    if (level.empty())
    {
        return false;
    }

    cout << badgeName
         << "·"
         << level
         << "（累计服务 "
         << fixed
         << setprecision(2)
         << duration
         << " 小时）\n";

    return true;
}

void showBadge(DataManager &data, const Student *student)
{
    vector<RankingItem> ranking = data.generateRanking();

    string badge = "无";

    for (size_t i = 0; i < ranking.size(); ++i)
    {
        if (ranking[i].studentId == student->getAccountId())
        {
            if (i < 3)
            {
                badge = "雷锋之星";
            }

            break;
        }
    }

    cout << "\n===== 我的徽章 =====\n";
    cout << "当前徽章：" << badge << "\n";

    cout << "\n专项徽章：\n";

    bool hasBadge = false;

    if (showCategoryBadge(
            *student,
            data,
            "C01",
            "劳动先锋"))
    {
        hasBadge = true;
    }

    if (showCategoryBadge(
            *student,
            data,
            "C02",
            "环保卫士"))
    {
        hasBadge = true;
    }

    if (showCategoryBadge(
            *student,
            data,
            "C03",
            "互助之星"))
    {
        hasBadge = true;
    }

    if (!hasBadge)
    {
        cout << "无\n";
    }
}

void showStudentInfo(DataManager &data, const Student *student)
{
    cout << "\n===== 个人信息 =====\n";
    cout << "账号：" << student->getAccountId() << "\n";
    cout << "姓名：" << student->getName() << "\n";
    cout << "班级：" << student->getClassName() << "\n";
    cout << "专业：" << student->getMajor() << "\n";

    vector<RankingItem> ranking = data.generateRanking();

    string badge = "无";

    for (size_t i = 0; i < ranking.size(); ++i)
    {
        if (ranking[i].studentId == student->getAccountId())
        {
            if (i < 3)
            {
                badge = "雷锋之星";
            }

            break;
        }
    }

    cout << "徽章：" << badge << "\n";
}

void changePassword(DataManager &data, Student *student)
{
    string oldPassword = readText("请输入原密码：");

    if (!student->checkPassword(oldPassword))
    {
        cout << "原密码错误。\n";
        return;
    }

    string newPassword = readText("请输入新密码：");

    if (newPassword.empty())
    {
        cout << "新密码不能为空。\n";
        return;
    }

    string confirmPassword = readText("请再次输入新密码：");

    if (newPassword != confirmPassword)
    {
        cout << "两次输入的密码不一致。\n";
        return;
    }

    student->setPassword(newPassword);
    data.saveStudents();

    cout << "密码修改成功。\n";
}

void submitRecord(
    Student &student,
    DataManager &data)
{
    showCategories(data);

    string categoryId =
        readText("请输入志愿类别编号：");

    if (data.findCategory(categoryId) == nullptr)
    {
        cout << "志愿类别不存在。\n";
        return;
    }

    string date =
        readText("请输入服务日期（例如：2025/10/01）：");

    double duration =
        readDouble("请输入服务时长：");

    if (duration <= 0)
    {
        cout << "服务时长必须大于 0。\n";
        return;
    }

    string place =
        readText("请输入服务地点：");

    string witness =
        readText("请输入证明人：");

    string description =
        readText("请输入志愿事迹简述：");

    VolunteerRecord record(
        data.generateRecordId(),
        student.getAccountId(),
        categoryId,
        date,
        duration,
        place,
        witness,
        description);

    data.addRecord(record);
    data.saveRecords();

    cout << "志愿记录提交成功，等待管理员审核。\n";
}

void showMyRecords(
    const Student &student,
    const DataManager &data)
{
    bool found = false;

    for (const VolunteerRecord &record :
         data.getRecords())
    {
        if (
            record.getStudentId() == student.getAccountId())
        {
            printRecord(record, data);
            found = true;
        }
    }

    if (!found)
    {
        cout << "当前没有志愿记录。\n";
    }
}

void queryRecordsByCategory(
    Student &student,
    DataManager &data)
{
    cout << "\n===== 志愿类别 =====\n";

    for (const VolunteerCategory &category :
         data.getCategories())
    {
        cout << category.getCategoryId()
             << "  "
             << category.getName()
             << "\n";
    }

    string categoryId =
        readText("请输入要查询的类别编号：");

    const VolunteerCategory *category =
        data.findCategory(categoryId);

    if (category == nullptr)
    {
        cout << "志愿类别不存在。\n";
        return;
    }

    bool found = false;

    cout << "\n===== 查询结果 =====\n";

    for (const VolunteerRecord &record :
         data.getRecords())
    {
        if (
            record.getStudentId() ==
                student.getAccountId() &&
            record.getCategoryId() ==
                categoryId)
        {
            cout << "\n记录编号："
                 << record.getRecordId()
                 << "\n类别："
                 << category->getName()
                 << "\n日期："
                 << record.getDate()
                 << "\n时长："
                 << record.getDuration()
                 << "\n地点："
                 << record.getPlace()
                 << "\n证明人："
                 << record.getWitness()
                 << "\n描述："
                 << record.getDescription()
                 << "\n状态："
                 << record.getStatusText()
                 << "\n积分："
                 << record.getScore()
                 << "\n";

            found = true;
        }
    }

    if (!found)
    {
        cout << "当前没有该类别的志愿记录。\n";
    }
}

void queryRecordsByDateRange(
    Student &student,
    DataManager &data)
{
    string startDate =
        readText("请输入开始日期（YYYY/MM/DD）：");

    string endDate =
        readText("请输入结束日期（YYYY/MM/DD）：");

    if (startDate > endDate)
    {
        cout << "开始日期不能晚于结束日期。\n";
        return;
    }

    bool found = false;

    cout << "\n===== 查询结果 =====\n";

    for (const VolunteerRecord &record :
         data.getRecords())
    {
        if (
            record.getStudentId() ==
                student.getAccountId() &&
            record.getDate() >= startDate &&
            record.getDate() <= endDate)
        {
            const VolunteerCategory *category =
                data.findCategory(
                    record.getCategoryId());

            cout << "\n记录编号："
                 << record.getRecordId();

            if (category != nullptr)
            {
                cout << "\n类别："
                     << category->getName();
            }

            cout << "\n日期："
                 << record.getDate()
                 << "\n时长："
                 << record.getDuration()
                 << "\n地点："
                 << record.getPlace()
                 << "\n证明人："
                 << record.getWitness()
                 << "\n描述："
                 << record.getDescription()
                 << "\n状态："
                 << record.getStatusText()
                 << "\n积分："
                 << record.getScore()
                 << "\n";

            found = true;
        }
    }

    if (!found)
    {
        cout << "该时间段内没有志愿记录。\n";
    }
}

void showMyRecordsMenu(
    Student &student,
    DataManager &data)
{
    while (true)
    {
        cout << "\n===== 我的志愿记录 =====\n";
        cout << "1. 查看全部\n";
        cout << "2. 按类型查询\n";
        cout << "3. 按时间段查询\n";
        cout << "0. 返回\n";

        int choice =
            readInt("请选择：");

        if (choice == 1)
        {
            showMyRecords(student, data);
        }
        else if (choice == 2)
        {
            queryRecordsByCategory(
                student,
                data);
        }
        else if (choice == 3)
        {
            queryRecordsByDateRange(
                student,
                data);
        }
        else if (choice == 0)
        {
            return;
        }
        else
        {
            cout << "无效选项。\n";
        }
    }
}

void modifyPendingRecord(
    Student &student,
    DataManager &data)
{
    cout << "\n===== 可修改的志愿记录 =====\n";

    bool found = false;

    for (const VolunteerRecord &item :
         data.getRecords())
    {
        if (
            item.getStudentId() == student.getAccountId() &&
            item.getStatus() != RecordStatus::Approved)
        {
            const VolunteerCategory *category =
                data.findCategory(item.getCategoryId());

            cout << item.getRecordId() << "  ";

            if (category != nullptr)
            {
                cout << category->getName();
            }
            else
            {
                cout << item.getCategoryId();
            }

            cout << "  "
                 << item.getDate()
                 << "  "
                 << item.getStatusText()
                 << "\n";

            found = true;
        }
    }

    if (!found)
    {
        cout << "当前没有可修改的志愿记录。\n";
        return;
    }

    string recordId =
        readText("请输入要修改的记录编号：");

    VolunteerRecord *record =
        data.findRecord(recordId);

    if (record == nullptr)
    {
        cout << "记录不存在。\n";
        return;
    }

    if (record->getStudentId() != student.getAccountId())
    {
        cout << "不能修改其他学生的记录。\n";
        return;
    }

    if (record->getStatus() == RecordStatus::Approved)
    {
        cout << "审核通过的记录不能修改。\n";
        return;
    }

    showCategories(data);

    string categoryId =
        readText("新的志愿类别编号：");

    if (data.findCategory(categoryId) == nullptr)
    {
        cout << "志愿类别不存在。\n";
        return;
    }

    record->setCategoryId(categoryId);
    record->setDate(
        readText("新的服务日期（例如：2025/10/01）："));

    double duration =
        readDouble("新的服务时长：");

    if (duration <= 0)
    {
        cout << "服务时长必须大于 0。\n";
        return;
    }

    record->setDuration(duration);
    record->setPlace(readText("新的服务地点："));
    record->setWitness(readText("新的证明人："));

    record->setDescription(
        readText("新的志愿事迹简述："));

    if (record->getStatus() == RecordStatus::Rejected)
    {
        record->resubmit();
    }

    data.saveRecords();

    cout << "记录修改成功。\n";
}

void deleteVolunteerRecord(
    Student &student,
    DataManager &data)
{
    cout << "\n===== 可删除的志愿记录 =====\n";

    bool found = false;

    for (const VolunteerRecord &record :
         data.getRecords())
    {
        if (
            record.getStudentId() ==
                student.getAccountId() &&
            record.getStatus() !=
                RecordStatus::Approved)
        {
            const VolunteerCategory *category =
                data.findCategory(
                    record.getCategoryId());

            cout << record.getRecordId() << "  ";

            if (category != nullptr)
            {
                cout << category->getName();
            }
            else
            {
                cout << record.getCategoryId();
            }

            cout << "  "
                 << record.getDate()
                 << "  "
                 << record.getStatusText()
                 << "\n";

            found = true;
        }
    }

    if (!found)
    {
        cout << "当前没有可删除的志愿记录。\n";
        return;
    }

    string recordId =
        readText("请输入要删除的记录编号：");

    VolunteerRecord *record =
        data.findRecord(recordId);

    if (record == nullptr)
    {
        cout << "志愿记录不存在。\n";
        return;
    }

    if (record->getStudentId() !=
        student.getAccountId())
    {
        cout << "不能删除其他学生的志愿记录。\n";
        return;
    }

    if (record->getStatus() ==
        RecordStatus::Approved)
    {
        cout << "审核通过的记录不能由学生删除。\n";
        return;
    }

    if (data.deleteRecord(recordId))
    {
        data.saveRecords();
        cout << "志愿记录删除成功。\n";
    }
    else
    {
        cout << "删除失败。\n";
    }
}

void publishDiary(Student &student, DataManager &data)
{
    cout << "\n===== 可发布到日记墙的志愿记录 =====\n";
    bool found = false;
    for (const VolunteerRecord &item : data.getRecords())
    {
        if (item.getStudentId() == student.getAccountId() && item.getStatus() == RecordStatus::Approved && data.findDiaryByRecordId(item.getRecordId()) == nullptr)
        {
            const VolunteerCategory *category = data.findCategory(item.getCategoryId());
            cout << item.getRecordId() << "  ";
            if (category != nullptr)
            {
                cout << category->getName();
            }
            else
            {
                cout << item.getCategoryId();
            }
            cout << "  "
                 << item.getDate()
                 << "  Approved\n";
            found = true;
        }
    }
    if (!found)
    {
        cout << "当前没有可发布到日记墙的志愿记录。\n";
        return;
    }
    string recordId = readText("请输入要发布到日记墙的志愿记录编号：");

    VolunteerRecord *record = data.findRecord(recordId);
    if (record == nullptr)
    {
        cout << "志愿记录不存在。\n";
        return;
    }
    if (record->getStudentId() != student.getAccountId())
    {
        cout << "不能发布其他学生的志愿记录。\n";
        return;
    }
    if (record->getStatus() != RecordStatus::Approved)
    {
        cout << "只有审核通过的志愿记录才能发布日记。\n";
        return;
    }
    if (data.findDiaryByRecordId(recordId) != nullptr)
    {
        cout << "该志愿记录已经发布过日记。\n";
        return;
    }
    string message = readText("请输入你的日记留言：");
    if (message.empty())
    {
        cout << "日记留言不能为空。\n";
        return;
    }

    DiaryPost diary(
        data.generateDiaryId(),
        student.getAccountId(),
        recordId,
        message);

    data.addDiary(diary);
    data.saveDiaries();

    cout << "日记发布成功。\n";
}

void showDiaryWall(const DataManager &data)
{
    const DataList<DiaryPost> &diaries = data.getDiaries();
    if (diaries.empty())
    {
        cout << "当前日记墙还没有内容。\n";
        return;
    }
    cout << "\n===== 校园雷锋日记墙 =====\n";
    for (const DiaryPost &diary :
         diaries.getItems())
    {
        const Student *student = nullptr;
        for (const Student &item :
             data.getStudents())
        {
            if (item.getAccountId() == diary.getStudentId())
            {
                student = &item;
                break;
            }
        }
        const VolunteerRecord *record = nullptr;
        for (const VolunteerRecord &item :
             data.getRecords())
        {
            if (
                item.getRecordId() ==
                diary.getRecordId())
            {
                record = &item;
                break;
            }
        }

        cout << "\n日记编号："
             << diary.getDiaryId();

        cout << "\n发布者：";

        if (student != nullptr)
        {
            cout << student->getName();
        }
        else
        {
            cout << diary.getStudentId();
        }

        cout << "\n志愿记录编号："
             << diary.getRecordId();

        if (record != nullptr)
        {
            const VolunteerCategory *category =
                data.findCategory(
                    record->getCategoryId());

            cout << "\n志愿类别：";

            if (category != nullptr)
            {
                cout << category->getName();
            }
            else
            {
                cout << record->getCategoryId();
            }

            cout << "\n服务日期："
                 << record->getDate();
            cout << "\n服务时长："
                 << record->getDuration()
                 << " 小时";
            cout << "\n服务地点："
                 << record->getPlace();
        }

        cout << "\n留言："
             << diary.getMessage();
        cout << "\n点赞数："
             << diary.getLikeCount();
        cout
            << "\n-----------------------------\n";
    }
}

void likeDiary(
    Student &student,
    DataManager &data)
{
    string diaryId =
        readText("请输入要点赞的日记编号：");

    DiaryPost *diary =
        data.findDiary(diaryId);

    if (diary == nullptr)
    {
        cout << "日记不存在。\n";
        return;
    }
    if (!diary->addLike(student.getAccountId()))
    {
        cout << "你已经点过赞了。\n";
        return;
    }
    data.saveDiaries();

    cout << "点赞成功。\n";
}

void showPersonalScore(
    const Student &student,
    const DataManager &data)
{
    double score =
        data.calculateStudentScore(
            student.getAccountId());

    cout
        << "当前总积分："
        << fixed
        << setprecision(2)
        << score
        << '\n';
}

void showMonthlyScore(
    const Student &student,
    const DataManager &data)
{
    string month =
        readText("请输入月份（YYYY/MM，例如 2026/09）：");

    if (
        month.size() != 7 ||
        month[4] != '/')
    {
        cout << "月份格式错误，请使用 YYYY/MM。\n";
        return;
    }

    string startDate =
        month + "/01";

    string endDate =
        month + "/31";

    double score =
        data.calculateStudentScoreByDateRange(
            student.getAccountId(),
            startDate,
            endDate);

    cout
        << month
        << " 月度积分："
        << fixed
        << setprecision(2)
        << score
        << " 分\n";
}

void showSemesterScore(
    const Student &student,
    const DataManager &data)
{
    cout << "\n===== 学期积分统计 =====\n";

    string startDate =
        readText("请输入学期开始日期（YYYY/MM/DD）：");

    string endDate =
        readText("请输入学期结束日期（YYYY/MM/DD）：");

    if (startDate > endDate)
    {
        cout << "开始日期不能晚于结束日期。\n";
        return;
    }

    double score =
        data.calculateStudentScoreByDateRange(
            student.getAccountId(),
            startDate,
            endDate);

    cout
        << "该学期积分："
        << fixed
        << setprecision(2)
        << score
        << " 分\n";
}

void showPersonalScoreMenu(
    const Student &student,
    const DataManager &data)
{
    while (true)
    {
        cout << "\n===== 我的积分 =====\n";
        cout << "1. 查看总积分\n";
        cout << "2. 查看月度积分\n";
        cout << "3. 查看学期积分\n";
        cout << "0. 返回\n";

        int choice =
            readInt("请选择：");

        if (choice == 1)
        {
            showPersonalScore(
                student,
                data);
        }
        else if (choice == 2)
        {
            showMonthlyScore(
                student,
                data);
        }
        else if (choice == 3)
        {
            showSemesterScore(
                student,
                data);
        }
        else if (choice == 0)
        {
            return;
        }
        else
        {
            cout << "菜单选项无效。\n";
        }
    }
}

void studentSession(
    Student &student,
    DataManager &data)
{
    while (true)
    {
        User *currentUser = &student;
        currentUser->showMenu();

        int choice =
            readInt("请选择：");

        if (choice == 0)
        {
            break;
        }

        if (choice == 1)
        {
            showStudentInfo(data, &student);
        }
        else if (choice == 2)
        {
            submitRecord(student, data);
        }
        else if (choice == 3)
        {
            showMyRecordsMenu(student, data);
        }
        else if (choice == 4)
        {
            modifyPendingRecord(student, data);
        }
        else if (choice == 5)
        {
            publishDiary(student, data);
        }
        else if (choice == 6)
        {
            showDiaryWall(data);
        }
        else if (choice == 7)
        {
            likeDiary(student, data);
        }
        else if (choice == 8)
        {
            changePassword(data, &student);
        }
        else if (choice == 9)
        {
            showPersonalScoreMenu(
                student,
                data);
        }
        else if (choice == 10)
        {
            showBadge(data, &student);
        }
        else if (choice == 11)
        {
            showRanking(data);
        }
        else if (choice == 12)
        {
            deleteVolunteerRecord(student, data);
        }
        else
        {
            cout << "菜单选项无效。\n";
        }
    }
}

void showPendingRecords(const DataManager &data)
{
    bool found = false;

    for (const VolunteerRecord &record :
         data.getRecords())
    {
        if (record.getStatus() == RecordStatus::Pending)
        {
            printRecord(record, data);
            found = true;
        }
    }

    if (!found)
    {
        cout << "当前没有待审核记录。\n";
    }
}

void reviewRecord(DataManager &data)
{
    cout << "\n===== 待审核志愿记录 =====\n";

    bool found = false;

    for (const VolunteerRecord &item :
         data.getRecords())
    {
        if (item.getStatus() == RecordStatus::Pending)
        {
            const VolunteerCategory *category =
                data.findCategory(item.getCategoryId());
            Student *student =
                data.findStudent(item.getStudentId());

            cout << item.getRecordId() << "  ";
            if (student != nullptr)
            {
                cout << "学生：" << student->getName() << "  ";
            }
            else
            {
                cout << "学生：" << item.getStudentId() << "  ";
            }
            if (category != nullptr)
            {
                cout << category->getName();
            }
            else
            {
                cout << item.getCategoryId();
            }
            cout << "  "
                 << item.getDate()
                 << "\n";

            found = true;
        }
    }

    if (!found)
    {
        cout << "当前没有待审核记录。\n";
        return;
    }

    string recordId =
        readText("请输入记录编号：");

    VolunteerRecord *record =
        data.findRecord(recordId);

    if (record == nullptr)
    {
        cout << "记录不存在。\n";
        return;
    }

    if (record->getStatus() != RecordStatus::Pending)
    {
        cout << "该记录已经完成审核。\n";
        return;
    }

    printRecord(*record, data);

    cout << "1. 审核通过\n";
    cout << "2. 审核驳回\n";

    int choice =
        readInt("请选择审核结果：");

    if (choice == 1)
    {
        const VolunteerCategory *category =
            data.findCategory(
                record->getCategoryId());

        if (category == nullptr)
        {
            cout << "志愿类别不存在。\n";
            return;
        }

        double score =
            category->calculateScore(
                record->getDuration());

        record->approve(score);

        cout << "审核通过，积分已计算。\n";
    }
    else if (choice == 2)
    {
        record->reject();

        cout << "记录已驳回。\n";
    }
    else
    {
        cout << "审核选项无效。\n";
        return;
    }

    data.saveRecords();
}

void showAllRecords(const DataManager &data)
{
    if (data.getRecords().empty())
    {
        cout << "当前没有志愿记录。\n";
        return;
    }

    for (const VolunteerRecord &record :
         data.getRecords())
    {
        printRecord(record, data);
    }
}

void queryRecordsByStudent(
    DataManager &data)
{
    string studentId =
        readText("请输入要查询的学生账号：");

    Student *student =
        data.findStudent(studentId);

    if (student == nullptr)
    {
        cout << "学生账号不存在。\n";
        return;
    }

    bool found = false;

    cout << "\n===== 学生志愿记录查询 =====\n";
    cout << "学生姓名：" << student->getName() << "\n";
    cout << "学生账号：" << student->getAccountId() << "\n";

    for (const VolunteerRecord &record :
         data.getRecords())
    {
        if (record.getStudentId() == studentId)
        {
            printRecord(record, data);
            found = true;
        }
    }

    if (!found)
    {
        cout << "该学生当前没有志愿记录。\n";
    }
}

void createStudentAccount(DataManager &data)
{
    string accountId =
        readText("请输入学生账号：");

    if (
        data.findStudent(accountId) != nullptr ||
        data.findAdministrator(accountId) != nullptr)
    {
        cout << "该账号已经存在。\n";
        return;
    }

    string name =
        readText("请输入学生姓名：");

    string className =
        readText("请输入班级：");

    string major =
        readText("请输入专业：");

    Student student(
        accountId,
        name,
        "123456",
        className,
        major);

    data.addStudent(student);
    data.saveStudents();

    cout << "学生账号创建成功。\n";
    cout << "默认密码：123456\n";
}

void showAllStudents(const DataManager &data)
{
    const vector<Student> &students =
        data.getStudents();

    if (students.empty())
    {
        cout << "当前没有学生账号。\n";
        return;
    }

    cout << "\n===== 全部学生账号 =====\n";

    for (const Student &student : students)
    {
        cout << "账号：" << student.getAccountId() << "\n";
        cout << "姓名：" << student.getName() << "\n";
        cout << "班级：" << student.getClassName() << "\n";
        cout << "专业：" << student.getMajor() << "\n";
        cout << "-----------------------------\n";
    }
}

void showAllAdministrators(const DataManager &data)
{
    const vector<Administrator> &administrators =
        data.getAdministrators();

    if (administrators.empty())
    {
        cout << "当前没有管理员账号。\n";
        return;
    }

    cout << "\n===== 全部管理员账号 =====\n";

    for (const Administrator &administrator : administrators)
    {
        cout << "账号：" << administrator.getAccountId() << "\n";
        cout << "姓名：" << administrator.getName() << "\n";
        cout << "-----------------------------\n";
    }
}

void createAdministratorAccount(DataManager &data)
{
    string accountId =
        readText("请输入管理员账号：");

    if (
        data.findStudent(accountId) != nullptr ||
        data.findAdministrator(accountId) != nullptr)
    {
        cout << "该账号已经存在。\n";
        return;
    }

    string name =
        readText("请输入管理员姓名：");

    string password =
        readText("请输入管理员密码：");

    if (password.empty())
    {
        cout << "密码不能为空。\n";
        return;
    }

    Administrator administrator(
        accountId,
        name,
        password);

    data.addAdministrator(administrator);
    data.saveAdministrators();

    cout << "管理员账号创建成功。\n";
}

void administratorSession(
    const string &administratorId,
    DataManager &data)
{
    while (true)
    {
        Administrator *administrator =
            data.findAdministrator(administratorId);

        if (administrator == nullptr)
        {
            cout << "当前管理员账号不存在。\n";
            return;
        }

        User *currentUser = administrator;
        currentUser->showMenu();

        int choice =
            readInt("请选择：");

        if (choice == 0)
        {
            break;
        }

        if (choice == 1)
        {
            showPendingRecords(data);
        }
        else if (choice == 2)
        {
            reviewRecord(data);
        }
        else if (choice == 3)
        {
            showAllRecords(data);
        }
        else if (choice == 4)
        {
            showRanking(data);
        }
        else if (choice == 5)
        {
            createStudentAccount(data);
        }
        else if (choice == 6)
        {
            createAdministratorAccount(data);
        }
        else if (choice == 7)
        {
            showAllStudents(data);
        }
        else if (choice == 8)
        {
            showAllAdministrators(data);
        }
        else if (choice == 9)
        {
            queryRecordsByStudent(data);
        }
        else
        {
            cout << "菜单选项无效。\n";
        }
    }
}

bool login(DataManager &data)
{
    cout << "\n===== 校园雷锋日记 =====\n";

    string accountId =
        readText("账号：");

    string password =
        readText("密码：");

    Student *student =
        data.findStudent(accountId);

    if (
        student != nullptr && student->checkPassword(password))
    {
        cout
            << "登录成功，欢迎 "
            << student->getName()
            << "！\n";

        studentSession(*student, data);
        return true;
    }

    Administrator *administrator =
        data.findAdministrator(accountId);

    if (
        administrator != nullptr && administrator->checkPassword(password))
    {
        cout
            << "管理员登录成功，欢迎 "
            << administrator->getName()
            << "！\n";

        administratorSession(
            accountId,
            data);

        return true;
    }

    cout << "账号或密码错误。\n";

    return false;
}

int main()
{
    system("chcp 65001 > nul");
    DataManager data;

    data.loadAll();

    cout << "校园雷锋日记系统启动成功。\n";

    while (true)
    {
        cout << "\n1. 登录\n";
        cout << "0. 退出系统\n";

        int choice =
            readInt("请选择：");

        if (choice == 0)
        {
            break;
        }

        if (choice == 1)
        {
            login(data);
        }
        else
        {
            cout << "菜单选项无效。\n";
        }
    }

    data.saveAll();

    cout << "系统已退出。\n";

    return 0;
}