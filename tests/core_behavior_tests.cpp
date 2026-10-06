#include "data_manager.h"
#include "diary_post.h"
#include "volunteer_category.h"
#include "volunteer_record.h"

#include <cmath>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

namespace
{
void require(bool condition, const string &message)
{
    if (!condition)
    {
        throw runtime_error(message);
    }
}

void requireNear(double actual, double expected, const string &message)
{
    if (fabs(actual - expected) > 1e-9)
    {
        throw runtime_error(message);
    }
}

VolunteerRecord makeRecord(
    const string &recordId,
    const string &studentId,
    const string &categoryId,
    const string &date,
    double duration,
    RecordStatus status,
    double score)
{
    return VolunteerRecord(
        recordId,
        studentId,
        categoryId,
        date,
        duration,
        "Campus",
        "Witness",
        "Synthetic test record",
        status,
        score);
}

void testVolunteerCategoryScores()
{
    DataManager data;
    const VolunteerCategory *labor = data.findCategory("C01");
    const VolunteerCategory *environment = data.findCategory("C02");
    const VolunteerCategory *mutualAid = data.findCategory("C03");

    require(labor != nullptr, "C01 category should exist");
    require(environment != nullptr, "C02 category should exist");
    require(mutualAid != nullptr, "C03 category should exist");
    requireNear(labor->calculateScore(2.0), 4.0, "C01 score should use coefficient 2.0");
    requireNear(environment->calculateScore(2.0), 3.0, "C02 score should use coefficient 1.5");
    requireNear(mutualAid->calculateScore(2.0), 2.0, "C03 score should use coefficient 1.0");
}

void testVolunteerRecordTransitions()
{
    VolunteerRecord pending("R0001", "S0001", "C01", "2026-03-01", 1.0, "Campus", "Witness", "Test");
    require(pending.getStatus() == RecordStatus::Pending, "new record should start Pending");

    VolunteerRecord approved("R0002", "S0001", "C01", "2026-03-01", 1.0, "Campus", "Witness", "Test");
    approved.approve(4.25);
    require(approved.getStatus() == RecordStatus::Approved, "approve should set Approved state");
    requireNear(approved.getScore(), 4.25, "approve should store the supplied score");

    VolunteerRecord rejected("R0003", "S0001", "C01", "2026-03-01", 1.0, "Campus", "Witness", "Test");
    rejected.reject();
    require(rejected.getStatus() == RecordStatus::Rejected, "reject should set Rejected state");
    rejected.resubmit();
    require(rejected.getStatus() == RecordStatus::Pending, "resubmit should return Rejected record to Pending");
}

void testDataManagerDerivedQueries()
{
    DataManager data;
    data.addStudent(Student("S0001", "Student One", "unused", "Class A", "Major A"));
    data.addStudent(Student("S0002", "Student Two", "unused", "Class B", "Major B"));
    data.addStudent(Student("S0003", "Student Three", "unused", "Class C", "Major C"));

    data.addRecord(makeRecord("R0001", "S0001", "C01", "2026-03-05", 2.0, RecordStatus::Approved, 4.0));
    data.addRecord(makeRecord("R0002", "S0001", "C02", "2026-03-15", 1.5, RecordStatus::Approved, 2.25));
    data.addRecord(makeRecord("R0003", "S0001", "C01", "2026-03-20", 3.0, RecordStatus::Pending, 99.0));
    data.addRecord(makeRecord("R0004", "S0001", "C01", "2026-03-25", 4.0, RecordStatus::Rejected, 99.0));
    data.addRecord(makeRecord("R0005", "S0001", "C01", "2026-04-01", 0.5, RecordStatus::Approved, 1.0));
    data.addRecord(makeRecord("R0006", "S0002", "C01", "2026-03-10", 5.0, RecordStatus::Approved, 10.0));

    requireNear(data.calculateStudentScore("S0001"), 7.25, "total score should include only approved records for the student");
    requireNear(data.calculateStudentScoreByDateRange("S0001", "2026-03-01", "2026-03-31"), 6.25, "date-range score should include only matching approved records");
    requireNear(data.calculateStudentDurationByCategory("S0001", "C01"), 2.5, "category duration should include only approved records for the student");

    const vector<RankingItem> ranking = data.generateRanking();
    require(ranking.size() == 3, "ranking should include all synthetic students");
    require(ranking[0].studentId == "S0002", "higher-score student should rank first");
    require(ranking[1].studentId == "S0001", "next-highest student should rank second");
    require(ranking[2].studentId == "S0003", "zero-score student should rank after positive scores");
}

void testDiaryPostLikes()
{
    DiaryPost diary("D0001", "S0001", "R0001", "Synthetic diary post");

    require(diary.addLike("S0002"), "first like from a student should succeed");
    require(diary.hasLiked("S0002"), "successful like should record the student ID");
    require(diary.getLikeCount() == 1, "successful like should increment the count");
    require(!diary.addLike("S0002"), "duplicate like from the same student should be rejected");
    require(diary.getLikeCount() == 1, "duplicate like should not increment the count");
}
}

int main()
{
    try
    {
        testVolunteerCategoryScores();
        testVolunteerRecordTransitions();
        testDataManagerDerivedQueries();
        testDiaryPostLikes();
        cout << "All core characterization checks passed." << endl;
        return 0;
    }
    catch (const exception &error)
    {
        cerr << "Core characterization failure: " << error.what() << endl;
        return 1;
    }
}
