#include "data_manager.h"
#include "diary_post.h"
#include "student_volunteer_service.h"
#include "volunteer_category.h"
#include "volunteer_record.h"

#include <cmath>
#include <chrono>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

using namespace std;

namespace
{
class ScopedTemporaryDirectory
{
public:
    explicit ScopedTemporaryDirectory(const string &label)
    {
        static unsigned long long nextId = 0;
        const auto tick = chrono::steady_clock::now()
                              .time_since_epoch()
                              .count();
        path_ = filesystem::temp_directory_path() /
                ("leifeng_iu_arch_03b_" + label + "_" +
                 to_string(tick) + "_" + to_string(++nextId));
        filesystem::create_directories(path_);
    }

    ~ScopedTemporaryDirectory()
    {
        error_code error;
        filesystem::remove_all(path_, error);
    }

    const filesystem::path &path() const
    {
        return path_;
    }

private:
    filesystem::path path_;
};

class ScopedCurrentPath
{
public:
    ScopedCurrentPath()
        : originalPath_(filesystem::current_path())
    {
    }

    ~ScopedCurrentPath()
    {
        error_code error;
        filesystem::current_path(originalPath_, error);
    }

private:
    filesystem::path originalPath_;
};

void require(bool condition, const string &message)
{
    if (!condition)
    {
        throw runtime_error(message);
    }
}

void writeRuntimeFile(
    const filesystem::path &root,
    const string &filename,
    const string &contents)
{
    ofstream file(root / filename, ios::binary);
    require(file.is_open(), "synthetic runtime file should be writable");
    file << contents;
    require(file.good(), "synthetic runtime file should be written");
}

void createSyntheticRuntimeData(const filesystem::path &root)
{
    filesystem::create_directories(root);
    writeRuntimeFile(
        root,
        "students.txt",
        "S9001|Synthetic Student|unused|Class A|Major A\n");
    writeRuntimeFile(
        root,
        "administrators.txt",
        "A9001|Synthetic Admin|unused\n");
    writeRuntimeFile(
        root,
        "records.txt",
        "R9001|S9001|C01|2026/04/01|0.50|Campus|Witness|Synthetic record|0|0.00\n");
    writeRuntimeFile(
        root,
        "diaries.txt",
        "D9001|S9001|R9001|Synthetic diary|0|\n");
}

void testDataManagerLoadsFromExplicitSyntheticRoot()
{
    ScopedTemporaryDirectory temporaryDirectory("valid_root");
    createSyntheticRuntimeData(temporaryDirectory.path());

    DataManager data(temporaryDirectory.path());
    require(data.loadAll(),
            "all required files in an explicit data root should load");
    require(data.getStudents().size() == 1,
            "explicit data root should load its student file");
    require(data.getAdministrators().size() == 1,
            "explicit data root should load its administrator file");
    require(data.getRecords().size() == 1,
            "explicit data root should load its record file");
    require(data.findDiaryByRecordId("R9001") != nullptr,
            "explicit data root should load its diary file");
}

void testDataManagerReportsMissingRootOrRequiredFile()
{
    ScopedTemporaryDirectory temporaryDirectory("missing_files");

    DataManager missingRoot(temporaryDirectory.path() / "absent");
    require(!missingRoot.loadAll(),
            "a missing data root should be reported as a load failure");

    const filesystem::path partialRoot =
        temporaryDirectory.path() / "partial";
    filesystem::create_directories(partialRoot);
    writeRuntimeFile(
        partialRoot,
        "students.txt",
        "S9001|Synthetic Student|unused|Class A|Major A\n");
    writeRuntimeFile(
        partialRoot,
        "administrators.txt",
        "A9001|Synthetic Admin|unused\n");
    writeRuntimeFile(
        partialRoot,
        "records.txt",
        "R9001|S9001|C01|2026/04/01|0.50|Campus|Witness|Synthetic record|0|0.00\n");

    DataManager missingDiary(partialRoot);
    require(!missingDiary.loadAll(),
            "a missing required data file should be reported as a load failure");
}

void testExplicitDataRootIgnoresProcessWorkingDirectory()
{
    ScopedTemporaryDirectory dataDirectory("cwd_data");
    ScopedTemporaryDirectory unrelatedWorkingDirectory("unrelated_cwd");
    createSyntheticRuntimeData(dataDirectory.path());

    ScopedCurrentPath restoreCurrentPath;
    filesystem::current_path(unrelatedWorkingDirectory.path());

    DataManager data(dataDirectory.path());
    require(data.loadAll(),
            "an explicit absolute data root should load outside the project cwd");
    require(data.findStudent("S9001") != nullptr,
            "working directory changes should not redirect student loading");
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

StudentVolunteerInput validStudentVolunteerInput()
{
    return {
        "C01",
        "2026/04/01",
        0.5,
        "Campus",
        "Witness",
        "Synthetic service record"};
}

void testStudentVolunteerSubmitDurationAndCategoryRules()
{
    DataManager data;
    StudentVolunteerService service(data);
    StudentVolunteerInput input = validStudentVolunteerInput();

    input.durationHours = 0.0;
    StudentVolunteerOutcome zero = service.submit("S0001", input);
    require(zero.status == StudentVolunteerStatus::InvalidDuration,
            "zero-hour submission should be rejected");
    require(data.getRecords().empty(),
            "rejected zero-hour submission should not add a record");

    input.durationHours = 0.25;
    StudentVolunteerOutcome quarter = service.submit("S0001", input);
    require(quarter.status == StudentVolunteerStatus::InvalidDuration,
            "non-half-hour submission should be rejected");
    require(data.getRecords().empty(),
            "rejected non-half-hour submission should not add a record");

    input.durationHours = std::numeric_limits<double>::infinity();
    StudentVolunteerOutcome infinite = service.submit("S0001", input);
    require(infinite.status == StudentVolunteerStatus::InvalidDuration,
            "infinite duration should be rejected");
    require(data.getRecords().empty(),
            "rejected infinite-duration submission should not add a record");

    input.durationHours = std::numeric_limits<double>::quiet_NaN();
    StudentVolunteerOutcome notANumber = service.submit("S0001", input);
    require(notANumber.status == StudentVolunteerStatus::InvalidDuration,
            "NaN duration should be rejected");
    require(data.getRecords().empty(),
            "rejected NaN-duration submission should not add a record");

    input = validStudentVolunteerInput();
    input.categoryId = "missing-category";
    StudentVolunteerOutcome missingCategory = service.submit("S0001", input);
    require(missingCategory.status == StudentVolunteerStatus::CategoryNotFound,
            "unknown category should be rejected");
    require(data.getRecords().empty(),
            "unknown-category submission should not add a record");

    input = validStudentVolunteerInput();
    StudentVolunteerOutcome accepted = service.submit("S0001", input);
    require(accepted.status == StudentVolunteerStatus::Success,
            "half-hour submission should succeed");
    require(!accepted.recordId.empty(),
            "successful submission should return the generated record ID");
    const VolunteerRecord *created = data.findRecord(accepted.recordId);
    require(created != nullptr,
            "successful submission should add the returned record");
    require(created->getStudentId() == "S0001",
            "submitted record should belong to the requesting student");
    require(created->getStatus() == RecordStatus::Pending,
            "submitted record should start Pending");
    requireNear(created->getDuration(), 0.5,
                "submitted record should retain the half-hour duration");
    requireNear(created->getScore(), 0.0,
                "submitted record should start with zero score");
}

void testStudentVolunteerModifyRejectsForeignApprovedAndInvalidChanges()
{
    DataManager data;
    data.addRecord(makeRecord(
        "R0101", "S0001", "C01", "2026/04/01", 1.0,
        RecordStatus::Pending, 0.0));
    VolunteerRecord approved = makeRecord(
        "R0102", "S0001", "C01", "2026/04/02", 1.0,
        RecordStatus::Approved, 3.0);
    data.addRecord(approved);
    StudentVolunteerService service(data);
    StudentVolunteerInput input = validStudentVolunteerInput();

    StudentVolunteerOutcome foreign = service.modify("S0002", "R0101", input);
    require(foreign.status == StudentVolunteerStatus::NotOwner,
            "student should not modify another student's record");

    StudentVolunteerOutcome locked = service.modify("S0001", "R0102", input);
    require(locked.status == StudentVolunteerStatus::ApprovedRecordLocked,
            "Approved record should not be modifiable by a student");

    input.categoryId = "C02";
    input.date = "changed-date";
    input.durationHours = 0.25;
    input.place = "Changed place";
    input.witness = "Changed witness";
    input.description = "Changed description";
    StudentVolunteerOutcome invalid = service.modify("S0001", "R0101", input);
    require(invalid.status == StudentVolunteerStatus::InvalidDuration,
            "invalid duration should reject the whole modification");

    const VolunteerRecord *unchanged = data.findRecord("R0101");
    require(unchanged != nullptr, "pending record should remain present");
    require(unchanged->getCategoryId() == "C01",
            "invalid modification should preserve category");
    require(unchanged->getDate() == "2026/04/01",
            "invalid modification should preserve date");
    requireNear(unchanged->getDuration(), 1.0,
                "invalid modification should preserve duration");
    require(unchanged->getPlace() == "Campus",
            "invalid modification should preserve place");
    require(unchanged->getWitness() == "Witness",
            "invalid modification should preserve witness");
    require(unchanged->getDescription() == "Synthetic test record",
            "invalid modification should preserve description");
}

void testStudentVolunteerModifyResubmitsRejectedRecord()
{
    DataManager data;
    VolunteerRecord rejected = makeRecord(
        "R0201", "S0001", "C01", "2026/04/03", 1.0,
        RecordStatus::Rejected, 0.0);
    data.addRecord(rejected);
    StudentVolunteerService service(data);

    StudentVolunteerOutcome outcome = service.modify(
        "S0001", "R0201", validStudentVolunteerInput());
    require(outcome.status == StudentVolunteerStatus::Success,
            "owner should be able to modify a Rejected record");
    const VolunteerRecord *updated = data.findRecord("R0201");
    require(updated != nullptr, "modified record should remain present");
    require(updated->getStatus() == RecordStatus::Pending,
            "modified Rejected record should return to Pending");
    requireNear(updated->getScore(), 0.0,
                "resubmitted record score should be reset");
    requireNear(updated->getDuration(), 0.5,
                "valid modification should update the record duration");
}

void testStudentVolunteerDeleteRejectsForeignAndApprovedRecords()
{
    DataManager data;
    data.addRecord(makeRecord(
        "R0301", "S0001", "C01", "2026/04/04", 1.0,
        RecordStatus::Pending, 0.0));
    data.addRecord(makeRecord(
        "R0302", "S0001", "C01", "2026/04/05", 1.0,
        RecordStatus::Approved, 2.0));
    StudentVolunteerService service(data);

    StudentVolunteerOutcome foreign = service.deleteRecord("S0002", "R0301");
    require(foreign.status == StudentVolunteerStatus::NotOwner,
            "student should not delete another student's record");
    require(data.findRecord("R0301") != nullptr,
            "foreign delete attempt should preserve the record");

    StudentVolunteerOutcome locked = service.deleteRecord("S0001", "R0302");
    require(locked.status == StudentVolunteerStatus::ApprovedRecordLocked,
            "Approved record should not be deletable by a student");
    require(data.findRecord("R0302") != nullptr,
            "Approved delete attempt should preserve the record");

    StudentVolunteerOutcome deleted = service.deleteRecord("S0001", "R0301");
    require(deleted.status == StudentVolunteerStatus::Success,
            "owner should be able to delete a Pending record");
    require(data.findRecord("R0301") == nullptr,
            "successful delete should remove the target record");
}

void testStudentVolunteerRejectsMissingTargets()
{
    DataManager data;
    StudentVolunteerService service(data);

    StudentVolunteerOutcome modify = service.modify(
        "S0001", "missing-record", validStudentVolunteerInput());
    require(modify.status == StudentVolunteerStatus::RecordNotFound,
            "modification should reject a missing record");

    StudentVolunteerOutcome remove = service.deleteRecord(
        "S0001", "missing-record");
    require(remove.status == StudentVolunteerStatus::RecordNotFound,
            "deletion should reject a missing record");
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
        testStudentVolunteerSubmitDurationAndCategoryRules();
        testStudentVolunteerModifyRejectsForeignApprovedAndInvalidChanges();
        testStudentVolunteerModifyResubmitsRejectedRecord();
        testStudentVolunteerDeleteRejectsForeignAndApprovedRecords();
        testStudentVolunteerRejectsMissingTargets();
        testDataManagerLoadsFromExplicitSyntheticRoot();
        testDataManagerReportsMissingRootOrRequiredFile();
        testExplicitDataRootIgnoresProcessWorkingDirectory();
        cout << "All core characterization checks passed." << endl;
        return 0;
    }
    catch (const exception &error)
    {
        cerr << "Core characterization failure: " << error.what() << endl;
        return 1;
    }
}
