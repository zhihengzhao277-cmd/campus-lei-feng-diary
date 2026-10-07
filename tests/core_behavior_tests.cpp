#include "data_manager.h"
#include "diary_service.h"
#include "diary_post.h"
#include "operation_log_service.h"
#include "student_volunteer_service.h"
#include "volunteer_category.h"
#include "volunteer_record.h"
#include "volunteer_review_service.h"

#include <algorithm>
#include <cmath>
#include <chrono>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <string>
#include <system_error>
#include <type_traits>
#include <utility>
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

string readFile(const filesystem::path &path)
{
    ifstream file(path, ios::binary);
    require(file.is_open(), "synthetic runtime file should be readable");
    return string(
        istreambuf_iterator<char>(file),
        istreambuf_iterator<char>());
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
    writeRuntimeFile(
        root,
        "operation_logs.csv",
        "logId,operatorAccountId,operationType,targetType,targetId,description,operationTime\n");
}

void createReviewRuntimeData(
    const filesystem::path &root,
    const string &recordLines)
{
    createSyntheticRuntimeData(root);
    writeRuntimeFile(root, "records.txt", recordLines);
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
    require(data.getOperationLogs().empty(),
            "header-only OperationLog file should load as an empty collection");
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

VolunteerApprovalInput makeApprovalInput(
    const string &categoryId,
    double duration,
    const string &reviewNote = "")
{
    return {categoryId, duration, reviewNote};
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
    if (status == RecordStatus::Pending)
    {
        return VolunteerRecord(
            recordId,
            studentId,
            categoryId,
            date,
            duration,
            "Campus",
            "Witness",
            "Synthetic test record");
    }

    std::optional<VolunteerRecord> legacy =
        VolunteerRecord::fromLegacyFields(
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
    if (!legacy.has_value())
    {
        throw runtime_error("legacy synthetic record should be constructible");
    }
    return std::move(*legacy);
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
    require(approved.approve("A0001", "C02", 1.5, 1.5, 2.3, "调整时长"),
            "valid approval should transition Pending to Approved");
    require(approved.getStatus() == RecordStatus::Approved, "approve should set Approved state");
    require(approved.getAppliedCategoryId() == "C01" &&
                approved.getFinalCategoryId() == "C02",
            "approval should preserve application category and store final category");
    requireNear(*approved.getFinalScore(), 2.3,
                "approve should store the normalized final score");

    VolunteerRecord rejected("R0003", "S0001", "C01", "2026-03-01", 1.0, "Campus", "Witness", "Test");
    require(rejected.reject("A0001", "需要补充证明"),
            "valid rejection should transition Pending to Rejected");
    require(rejected.getStatus() == RecordStatus::Rejected, "reject should set Rejected state");
    require(rejected.resubmit(), "Rejected record should resubmit to Pending");
    require(rejected.getStatus() == RecordStatus::Pending, "resubmit should return Rejected record to Pending");
    require(!rejected.getReviewerAccountId().has_value() &&
                !rejected.getReviewNote().has_value() &&
                !rejected.getFinalScore().has_value(),
            "resubmit should clear current review and settlement fields");
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

    requireNear(data.calculateStudentScore("S0001"), 7.3, "total score should include only approved records for the student and expose one decimal");
    requireNear(data.calculateStudentScoreByDateRange("S0001", "2026-03-01", "2026-03-31"), 6.3, "date-range score should include only matching approved records and expose one decimal");
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

void testDiaryServicePublishesOnlyApprovedPostsWithRecordOwnerFacts()
{
    DataManager data;
    data.addStudent(Student("S_OWNER", "Record Owner", "unused", "Class A", "Major A"));
    data.addStudent(Student("S_OTHER", "Other Owner", "unused", "Class B", "Major B"));
    data.addStudent(Student("S_LEGACY", "Legacy Publisher", "unused", "Class C", "Major C"));

    auto correctedOwnerRecord = VolunteerRecord::fromModernFields(
        "R_OWNER", "S_OWNER", "2026/05/01", "C01", 1.5,
        "Campus", "Witness", "Synthetic test record",
        RecordStatus::Approved, string("C02"), 2.0,
        string("A9001"), string("审核修正"), 1.5, 3.0);
    require(correctedOwnerRecord.has_value(),
            "corrected approved fixture should satisfy modern invariants");
    data.addRecord(*correctedOwnerRecord);
    data.addRecord(makeRecord(
        "R_PENDING", "S_OTHER", "C02", "2026/05/02", 2.0,
        RecordStatus::Pending, 0.0));
    data.addRecord(makeRecord(
        "R_OTHER", "S_OTHER", "C02", "2026/05/03", 0.5,
        RecordStatus::Approved, 1.0));
    data.addRecord(makeRecord(
        "R_MISSING_OWNER", "S_NOT_PRESENT", "C03", "2026/05/04", 1.0,
        RecordStatus::Approved, 1.0));

    DiaryPost first("D_FIRST", "S_LEGACY", "R_OWNER", "First approved post", 2);
    first.addLikedStudentId("S_VIEWER");
    data.addDiary(first);
    data.addDiary(DiaryPost("D_MISSING_RECORD", "S_OWNER", "R_NOT_PRESENT", "No record"));
    data.addDiary(DiaryPost("D_PENDING", "S_OWNER", "R_PENDING", "Not approved"));
    data.addDiary(DiaryPost("D_MISSING_OWNER", "S_LEGACY", "R_MISSING_OWNER", "No owner"));
    data.addDiary(DiaryPost("D_SECOND", "S_LEGACY", "R_OTHER", "Second approved post", 1));

    DiaryService service(data);
    const vector<DiaryPostPublicView> feed = service.queryPublicFeed("S_VIEWER");

    require(feed.size() == 2,
            "only Approved diary posts with an existing record owner should be returned");
    require(feed[0].diaryId == "D_FIRST" && feed[1].diaryId == "D_SECOND",
            "public feed should retain stored diary order among eligible posts");
    require(feed[0].authorAccountId == "S_OWNER",
            "public author account should come from the linked record owner, not legacy diary student ID");
    require(feed[0].authorName == "Record Owner",
            "public author name should come from the linked record owner");
    require(feed[0].categoryName == "环保服务",
            "public category should use the final reviewed category");
    requireNear(feed[0].score, 3.0,
                "public score should use the final reviewed score");
    require(feed[0].serviceDate == "2026/05/01",
            "public service date should come from the linked record");
    requireNear(feed[0].durationHours, 2.0,
                "public duration should use the final reviewed duration");
    require(feed[0].place == "Campus",
            "public place should come from the linked record");
    require(feed[0].content == "First approved post",
            "public content should come from the diary post");
    require(feed[0].likeCount == 2 && feed[0].likedByCurrentStudent,
            "public view should report the stored like count and current student's like state");
    require(feed[1].authorAccountId == "S_OTHER" &&
                feed[1].authorName == "Other Owner" &&
                feed[1].categoryName == "环保服务" &&
                !feed[1].likedByCurrentStudent,
            "later eligible post should expose its own owner, category and unliked state");
    require(data.getDiaries().getItems()[0].getLikeCount() == 2 &&
                data.getDiaries().getItems()[0].getLikedStudentIds().size() == 1,
            "querying the public feed should not mutate diary likes");
}

void testDiaryServiceReturnsEmptyForIneligiblePosts()
{
    DataManager data;
    data.addRecord(makeRecord(
        "R_PENDING_ONLY", "S_NOT_PRESENT", "C01", "2026/05/05", 1.0,
        RecordStatus::Pending, 0.0));
    data.addDiary(DiaryPost(
        "D_PENDING_ONLY", "S_LEGACY", "R_PENDING_ONLY", "No public post"));

    DiaryService service(data);
    require(service.queryPublicFeed("S_VIEWER").empty(),
            "feed should be empty when no post has both an Approved record and owner student");
}

void testDiaryLoaderSupportsTheApprovedModernRowShape()
{
    ScopedTemporaryDirectory temporaryDirectory(
        "diary_modern_row_shape");
    createSyntheticRuntimeData(temporaryDirectory.path());
    writeRuntimeFile(
        temporaryDirectory.path(),
        "diaries.txt",
        "D9001|S9001|R9001|Legacy diary|1|S9001\n"
        "D9002|R9002|Modern title|Modern content|PendingDisplayReview||0|\n"
        "D9003|R9003|Modern displayed|Public content|Displayed|2026-10-07T12:34:56|0|\n");

    DataManager data(temporaryDirectory.path());
    require(data.loadDiaries(),
            "valid legacy and modern diary rows should load together");
    const vector<DiaryPost> &loaded = data.getDiaries().getItems();
    require(loaded.size() == 3,
            "the loader should retain legacy, modern pending and modern displayed rows");
    require(loaded[0].getDiaryId() == "D9001" &&
                loaded[0].getMessage() == "Legacy diary" &&
                loaded[1].getDiaryId() == "D9002" &&
                loaded[1].getMessage() == "Modern content" &&
                loaded[2].getDiaryId() == "D9003" &&
                loaded[2].getDisplayStatus() == DiaryDisplayStatus::Displayed &&
                loaded[2].getPublishedAt() ==
                    optional<string>("2026-10-07T12:34:56"),
            "loading should preserve legacy content and validate modern displayed publication time");

    data.addDiary(DiaryPost(
        "D_KEEP", "S9001", "R_KEEP", "Keep on failed load"));
    writeRuntimeFile(
        temporaryDirectory.path(),
        "diaries.txt",
        "D9001|S9001|R9001|Valid row|0|\n"
        "D9002|R9002||Modern content|Displayed||0|\n");
    require(!data.loadDiaries(),
            "an eight-field Displayed row without a modern title and time must fail the full load");
    require(data.getDiaries().size() == 4 &&
                data.findDiary("D_KEEP") != nullptr,
            "a failed diary load must not publish a partial collection");
}

void testDiaryLoaderEnforcesLikePersistenceInvariants()
{
    const auto loadSingleDiaryRow = [](const string &label, const string &row)
    {
        ScopedTemporaryDirectory temporaryDirectory(label);
        createSyntheticRuntimeData(temporaryDirectory.path());
        writeRuntimeFile(
            temporaryDirectory.path(), "diaries.txt", row);
        DataManager data(temporaryDirectory.path());
        return data.loadDiaries();
    };

    require(loadSingleDiaryRow(
                "diary_like_legacy_valid",
                "D_LEGACY|S9001|R_LEGACY|Legacy body|1|S9001\n"),
            "a six-field legacy Displayed row with one matching like should load");
    require(!loadSingleDiaryRow(
                "diary_like_legacy_mismatch",
                "D_LEGACY_BAD|S9001|R_LEGACY_BAD|Legacy body|2|S9001\n"),
            "legacy likeCount must equal the number of liked student IDs");
    require(loadSingleDiaryRow(
                "diary_like_modern_displayed",
                "D_MODERN|R_MODERN|Title|Body|Displayed|2026-10-07T12:34:56|1|S9001\n"),
            "a modern Displayed row with a title, valid time and matching like should load");
    require(!loadSingleDiaryRow(
                "diary_like_pending_state",
                "D_PENDING_LIKE|R_PENDING_LIKE|Title|Body|PendingDisplayReview||1|S9001\n"),
            "PendingDisplayReview rows must not contain persisted likes");
    require(!loadSingleDiaryRow(
                "diary_like_rejected_state",
                "D_REJECTED_LIKE|R_REJECTED_LIKE|Title|Body|Rejected||1|S9001\n"),
            "Rejected rows must not contain persisted likes");
    require(!loadSingleDiaryRow(
                "diary_like_modern_mismatch",
                "D_MODERN_BAD|R_MODERN_BAD|Title|Body|Displayed|2026-10-07T12:34:56|2|S9001\n"),
            "modern likeCount must equal the number of liked student IDs");
    require(!loadSingleDiaryRow(
                "diary_like_modern_duplicate",
                "D_MODERN_DUP|R_MODERN_DUP|Title|Body|Displayed|2026-10-07T12:34:56|2|S9001,S9001\n"),
            "duplicate liked student IDs must remain invalid");
    require(!loadSingleDiaryRow(
                "diary_like_legacy_empty_id",
                "D_LEGACY_EMPTY|S9001|R_LEGACY_EMPTY|Legacy body|2|S9001,\n"),
            "empty liked student IDs must remain invalid in legacy rows");

    require(!DiaryPost::fromModernFields(
                "D_DIRECT_BAD",
                "R_DIRECT_BAD",
                "Title",
                "Body",
                DiaryDisplayStatus::Displayed,
                optional<string>("2026-10-07T12:34:56"),
                2,
                {"S9001"})
                 .has_value(),
            "modern construction must reject a likeCount/list mismatch");
}

void testDiaryServiceRequestModerationAndLikes()
{
    ScopedTemporaryDirectory temporaryDirectory("diary_service_lifecycle");
    createSyntheticRuntimeData(temporaryDirectory.path());
    DataManager data(temporaryDirectory.path());
    require(data.loadAll(), "synthetic diary service data should load");
    data.addStudent(Student("S9002", "Second Student", "unused", "Class B", "Major B"));
    data.addRecord(makeRecord(
        "R9002", "S9001", "C01", "2026/04/02", 1.0,
        RecordStatus::Approved, 2.0));
    data.addRecord(makeRecord(
        "R9003", "S9002", "C02", "2026/04/03", 2.0,
        RecordStatus::Approved, 3.0));

    DiaryService service(data);
    require(service.requestDisplay(
                "S_MISSING", "R9002", "Title", "Content").status ==
                DiaryServiceStatus::StudentNotFound,
            "only an existing student may request display");
    require(service.requestDisplay(
                "S9002", "R9002", "Title", "Content").status ==
                DiaryServiceStatus::RecordNotOwned,
            "a student may not request display for another student's record");
    require(service.requestDisplay(
                "S9001", "R9001", "Title", "Content").status ==
                DiaryServiceStatus::RecordNotApproved,
            "a Pending source record cannot be used for a display request");
    require(service.requestDisplay(
                "S9001", "R9002", "  |bad", "Content").status ==
                DiaryServiceStatus::InvalidTitle,
            "title validation should reject the legacy delimiter after trimming");
    require(service.requestDisplay(
                "S9001", "R9002", "Title", "  \n ").status ==
                DiaryServiceStatus::InvalidContent,
            "content validation should reject blank text after trimming");

    const DiaryServiceOutcome requested = service.requestDisplay(
        "S9001", "R9002", "  Campus work  ", "  Helped at the campus event.  ");
    require(requested.succeeded() && !requested.diaryId.empty(),
            "an owned Approved record should create a pending display request");
    DiaryPost *pending = data.findDiary(requested.diaryId);
    require(pending != nullptr &&
                pending->getDisplayStatus() ==
                    DiaryDisplayStatus::PendingDisplayReview &&
                pending->getTitle() == "Campus work" &&
                pending->getContent() == "Helped at the campus event." &&
                !pending->getPublishedAt().has_value(),
            "new requests should trim title/content and have no publication time");
    require(service.queryPublicFeed("S9002").empty(),
            "a pending request must not appear in the public feed");
    require(service.like("S9002", requested.diaryId).status ==
                DiaryServiceStatus::InvalidState &&
                service.unlike("S9002", requested.diaryId).status ==
                    DiaryServiceStatus::InvalidState,
            "pending requests must reject like and unlike");
    require(service.requestDisplay(
                "S9001", "R9002", "Second title", "Second content").status ==
                DiaryServiceStatus::DuplicateRecord,
            "a record may have at most one DiaryPost");
    const vector<DiaryApplicationView> applications =
        service.queryMyApplications("S9001");
    const bool requestedApplicationFound = std::any_of(
        applications.begin(), applications.end(),
        [&requested](const DiaryApplicationView &application)
        {
            return application.diaryId == requested.diaryId;
        });
    require(requestedApplicationFound &&
                service.queryModeration(
                    DiaryDisplayStatus::PendingDisplayReview).size() == 1,
            "student and moderation queries should expose the new pending request");
    require(data.getOperationLogs().empty(),
            "student display requests should not create OperationLog entries");

    require(service.approveDisplay("A_MISSING", requested.diaryId).status ==
                DiaryServiceStatus::AdministratorNotFound,
            "approval requires an existing administrator");
    require(service.approveDisplay("A9001", requested.diaryId).succeeded(),
            "an administrator should approve a pending request");
    const DiaryPost *displayed = data.findDiary(requested.diaryId);
    require(displayed != nullptr &&
                displayed->getDisplayStatus() == DiaryDisplayStatus::Displayed &&
                displayed->getPublishedAt().has_value() &&
                displayed->getPublishedAt()->size() == 19,
            "approval should set Displayed and a real DateTime");
    const vector<DiaryPostPublicView> feed = service.queryPublicFeed("S9002");
    require(feed.size() == 1 && feed.front().diaryId == requested.diaryId &&
                feed.front().title == "Campus work" &&
                feed.front().content == "Helped at the campus event." &&
                feed.front().authorAccountId == "S9001" &&
                std::abs(feed.front().score - 2.0) < 1e-9,
            "approved requests should be public using record ownership and final text");
    require(data.getOperationLogs().size() == 1 &&
                data.getOperationLogs().front().getOperationType() ==
                    OperationType::DiaryDisplayApproved &&
                data.getOperationLogs().front().getTargetType() ==
                    OperationTargetType::DiaryPost &&
                data.getOperationLogs().front().getTargetId() == requested.diaryId,
            "successful approval should create exactly one DiaryPost audit log");
    OperationLogService logService(data);
    OperationLogQuery diaryTargetQuery;
    diaryTargetQuery.targetType = OperationTargetType::DiaryPost;
    diaryTargetQuery.targetId = requested.diaryId;
    require(logService.query(diaryTargetQuery).size() == 1,
            "OperationLog queries should filter DiaryPost targets explicitly");

    require(service.like("S9002", requested.diaryId).succeeded(),
            "a student should like a Displayed post");
    require(service.like("S9002", requested.diaryId).status ==
                DiaryServiceStatus::AlreadyLiked,
            "duplicate likes should be rejected");
    require(service.unlike("S9002", requested.diaryId).succeeded(),
            "a student should remove an existing like");
    require(service.unlike("S9002", requested.diaryId).status ==
                DiaryServiceStatus::LikeNotFound,
            "unlike should require an existing like");
    require(data.getOperationLogs().size() == 1,
            "like and unlike should not create admin audit logs");

    require(service.like("S9002", requested.diaryId).succeeded(),
            "the student should be able to like again after unlike");
    const double scoreBeforeTakedown =
        data.calculateStudentScore("S9001");
    require(service.takeDown("A9001", requested.diaryId).succeeded(),
            "an administrator should take down a Displayed post");
    const DiaryPost *takenDown = data.findDiary(requested.diaryId);
    require(takenDown != nullptr &&
                takenDown->getDisplayStatus() == DiaryDisplayStatus::TakenDown &&
                takenDown->getLikeCount() == 1 &&
                takenDown->hasLiked("S9002") &&
                takenDown->getPublishedAt().has_value(),
            "takedown should preserve the published time and existing likes");
    require(service.queryPublicFeed("S9002").empty() &&
                service.like("S9002", requested.diaryId).status ==
                    DiaryServiceStatus::InvalidState &&
                service.unlike("S9002", requested.diaryId).status ==
                    DiaryServiceStatus::InvalidState,
            "TakenDown posts leave the feed and reject new like interactions");
    requireNear(
        data.calculateStudentScore("S9001"),
        scoreBeforeTakedown,
        "diary takedown must not change the source record score");

    const DiaryServiceOutcome rejectedRequest = service.requestDisplay(
        "S9002", "R9003", "Student effort", "Helped a classmate.");
    require(rejectedRequest.succeeded(),
            "another owned Approved record should allow a separate request");
    require(service.rejectDisplay("A9001", rejectedRequest.diaryId).succeeded(),
            "an administrator should reject a pending request");
    require(data.findDiary(rejectedRequest.diaryId)->getDisplayStatus() ==
                DiaryDisplayStatus::Rejected &&
                service.queryPublicFeed("S9001").empty() &&
                service.like("S9001", rejectedRequest.diaryId).status ==
                    DiaryServiceStatus::InvalidState &&
                service.unlike("S9001", rejectedRequest.diaryId).status ==
                    DiaryServiceStatus::InvalidState,
            "rejected requests should stay outside the public feed");
    require(data.getOperationLogs().size() == 3,
            "successful approve, takedown and reject should each create one log");
    const vector<OperationLog> &logs = data.getOperationLogs();
    require(std::count_if(
                logs.begin(), logs.end(), [](const OperationLog &log)
                {
                    return log.getOperationType() ==
                           OperationType::DiaryDisplayApproved;
                }) == 1 &&
                std::count_if(
                    logs.begin(), logs.end(), [](const OperationLog &log)
                    {
                        return log.getOperationType() ==
                               OperationType::DiaryDisplayRejected;
                    }) == 1 &&
                std::count_if(
                    logs.begin(), logs.end(), [](const OperationLog &log)
                    {
                        return log.getOperationType() ==
                               OperationType::DiaryTakenDown;
                    }) == 1,
            "approve, reject and takedown should each append one distinct event type");

    DataManager reloaded(temporaryDirectory.path());
    require(reloaded.loadDiaries() && reloaded.loadOperationLogs(),
            "modern diary rows and new moderation logs should reload");
    require(reloaded.findDiary(requested.diaryId)->getDisplayStatus() ==
                DiaryDisplayStatus::TakenDown &&
                reloaded.findDiary(rejectedRequest.diaryId)->getDisplayStatus() ==
                    DiaryDisplayStatus::Rejected &&
                reloaded.getOperationLogs().size() == 3,
            "modern state transitions and moderation logs should round-trip");
}

void testDiaryServicePersistenceFailureSemantics()
{
    {
        ScopedTemporaryDirectory temporaryDirectory(
            "diary_single_file_prepare_failure");
        createSyntheticRuntimeData(temporaryDirectory.path());
        DataManager data(temporaryDirectory.path());
        require(data.loadAll(), "single-file failure fixture should load");
        data.addRecord(makeRecord(
            "R9002", "S9001", "C01", "2026/04/02", 1.0,
            RecordStatus::Approved, 2.0));
        writeRuntimeFile(
            temporaryDirectory.path(), "diaries.txt.tmp", "block");
        DiaryService service(data);
        const DiaryServiceOutcome result = service.requestDisplay(
            "S9001", "R9002", "Title", "Content");
        require(result.status == DiaryServiceStatus::PersistenceFailure &&
                    data.findDiaryByRecordId("R9002") == nullptr,
                "a failed single-diary save must not report success or retain the request");
    }

    {
        ScopedTemporaryDirectory temporaryDirectory(
            "diary_audit_prepare_failure");
        createSyntheticRuntimeData(temporaryDirectory.path());
        DataManager data(temporaryDirectory.path());
        require(data.loadAll(), "audit prepare-failure fixture should load");
        data.addRecord(makeRecord(
            "R9002", "S9001", "C01", "2026/04/02", 1.0,
            RecordStatus::Approved, 2.0));
        DiaryService service(data);
        const DiaryServiceOutcome requested = service.requestDisplay(
            "S9001", "R9002", "Title", "Content");
        require(requested.succeeded(), "audit fixture request should persist");
        const string diaryFileBefore =
            readFile(temporaryDirectory.path() / "diaries.txt");
        writeRuntimeFile(
            temporaryDirectory.path(), "operation_logs.csv.tmp", "block");
        const DiaryServiceOutcome result =
            service.approveDisplay("A9001", requested.diaryId);
        require(result.status == DiaryServiceStatus::PersistenceFailure &&
                    data.findDiary(requested.diaryId)->getDisplayStatus() ==
                        DiaryDisplayStatus::PendingDisplayReview &&
                    data.getOperationLogs().empty() &&
                    readFile(temporaryDirectory.path() / "diaries.txt") ==
                        diaryFileBefore,
                "ordinary audit Prepare failure should restore in-memory and disk state");
    }

    {
        ScopedTemporaryDirectory temporaryDirectory(
            "diary_audit_partial_commit");
        createSyntheticRuntimeData(temporaryDirectory.path());
        DataManager data(temporaryDirectory.path());
        require(data.loadAll(), "audit partial-commit fixture should load");
        data.addRecord(makeRecord(
            "R9002", "S9001", "C01", "2026/04/02", 1.0,
            RecordStatus::Approved, 2.0));
        DiaryService service(data);
        const DiaryServiceOutcome requested = service.requestDisplay(
            "S9001", "R9002", "Title", "Content");
        require(requested.succeeded(), "partial-commit fixture request should persist");
        writeRuntimeFile(
            temporaryDirectory.path(), "operation_logs.csv.bak", "block");
        const DiaryServiceOutcome result =
            service.approveDisplay("A9001", requested.diaryId);
        require(result.status ==
                    DiaryServiceStatus::SeverePersistenceFailure,
                "a log Commit failure after diary Commit must be severe and not success");
        DataManager reloaded(temporaryDirectory.path());
        require(reloaded.loadDiaries() && reloaded.loadOperationLogs() &&
                    reloaded.findDiary(requested.diaryId)->getDisplayStatus() ==
                        DiaryDisplayStatus::Displayed &&
                    reloaded.getOperationLogs().empty(),
                "partial commit should truthfully leave diary committed without claiming its log");
    }
}

void testLegacyDiaryTakedownDoesNotInventHistory()
{
    ScopedTemporaryDirectory temporaryDirectory("legacy_diary_takedown");
    createSyntheticRuntimeData(temporaryDirectory.path());
    writeRuntimeFile(
        temporaryDirectory.path(),
        "records.txt",
        "R9001|S9001|C01|2026/04/01|0.50|Campus|Witness|Source record|1|1.0\n");
    writeRuntimeFile(
        temporaryDirectory.path(),
        "diaries.txt",
        "D9001|S9001|R9001|Legacy body|1|S9001\n");

    DataManager data(temporaryDirectory.path());
    require(data.loadAll(), "legacy diary data should load");
    const DiaryPost *legacy = data.findDiary("D9001");
    require(legacy != nullptr && legacy->isLegacyCompatibilityRecord() &&
                legacy->getDisplayStatus() == DiaryDisplayStatus::Displayed &&
                legacy->getTitle().empty() &&
                !legacy->getPublishedAt().has_value(),
            "six-field legacy data should be Displayed without fabricated title/time");
    DiaryService service(data);
    const vector<DiaryPostPublicView> feed = service.queryPublicFeed("S9001");
    require(feed.size() == 1 && feed.front().diaryId == "D9001" &&
                feed.front().title.empty() &&
                !feed.front().publishedAt.has_value(),
            "legacy Displayed posts should remain publicly queryable with unknown time");
    require(service.takeDown("A9001", "D9001").succeeded(),
            "a legacy Displayed post should be administratively taken down");
    const DiaryPost *takenDown = data.findDiary("D9001");
    require(takenDown != nullptr &&
                takenDown->getDisplayStatus() == DiaryDisplayStatus::TakenDown &&
                !takenDown->getPublishedAt().has_value() &&
                takenDown->getLikeCount() == 1 &&
                takenDown->hasLiked("S9001"),
            "legacy takedown should preserve likes and keep the historical time absent");

    DataManager reloaded(temporaryDirectory.path());
    require(reloaded.loadDiaries(),
            "legacy takedown should persist in the modern eight-field row");
    require(readFile(temporaryDirectory.path() / "diaries.txt") ==
                "D9001|R9001||Legacy body|TakenDown||1|S9001\n",
            "legacy-derived takedown should persist as eight fields without fabricated title or time");
    const DiaryPost *reloadedPost = reloaded.findDiary("D9001");
    require(reloadedPost != nullptr &&
                reloadedPost->getDisplayStatus() == DiaryDisplayStatus::TakenDown &&
                !reloadedPost->getPublishedAt().has_value() &&
                reloadedPost->getLikeCount() == 1 &&
                reloadedPost->hasLiked("S9001"),
            "legacy compatibility should survive a restart without inventing history");
}

void testDiaryPublicFeedOrdersModernBeforeStableLegacyPosts()
{
    ScopedTemporaryDirectory temporaryDirectory("diary_public_order");
    createSyntheticRuntimeData(temporaryDirectory.path());
    writeRuntimeFile(
        temporaryDirectory.path(),
        "records.txt",
        "R9001|S9001|C01|2026/04/01|0.50|Campus|Witness|Legacy source|1|1.0\n"
        "R9002|S9001|C01|2026/04/02|0.50|Campus|Witness|New source|1|1.0\n"
        "R9003|S9001|C01|2026/04/03|0.50|Campus|Witness|Old source|1|1.0\n");
    writeRuntimeFile(
        temporaryDirectory.path(),
        "diaries.txt",
        "D9001|S_WRONG|R9001|Legacy content|1|S9001\n"
        "D9002|R9002|Newest title|Newest content|Displayed|2026-10-01T12:00:00|0|\n"
        "D9003|R9003|Older title|Older content|Displayed|2026-09-01T12:00:00|0|\n");

    DataManager data(temporaryDirectory.path());
    require(data.loadAll(), "public ordering fixtures should load");
    DiaryService service(data);
    const vector<DiaryPostPublicView> feed = service.queryPublicFeed("S9001");
    require(feed.size() == 3 &&
                feed[0].diaryId == "D9002" &&
                feed[1].diaryId == "D9003" &&
                feed[2].diaryId == "D9001",
            "modern posts should sort newest first and legacy posts should retain stable trailing order");
    require(feed[2].authorAccountId == "S9001" &&
                !feed[2].publishedAt.has_value(),
            "legacy publisher and missing time must not override record ownership or invent chronology");
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

void testVolunteerReviewPreviewAndApprovalUseCurrentCategoryScore()
{
    ScopedTemporaryDirectory temporaryDirectory(
        "review_approval_audit");
    createReviewRuntimeData(
        temporaryDirectory.path(),
        "R_REVIEW_C01|S9001|C01|2026/06/01|1.50|Campus|Witness|Synthetic|0|0.00\n"
        "R_REVIEW_C02|S9001|C02|2026/06/01|1.50|Campus|Witness|Synthetic|0|0.00\n"
        "R_REVIEW_C03|S9001|C03|2026/06/01|1.50|Campus|Witness|Synthetic|0|0.00\n");
    DataManager data(temporaryDirectory.path());
    require(data.loadAll(),
            "synthetic review files should load before approval");
    VolunteerReviewService service(data);

    const vector<string> categoryIds = {"C01", "C02", "C03"};
    size_t expectedLogCount = 0;
    for (const string &categoryId : categoryIds)
    {
        const string recordId = "R_REVIEW_" + categoryId;
        const VolunteerRecord *beforePreview = data.findRecord(recordId);
        require(beforePreview != nullptr,
                "synthetic review record should exist before preview");
        const VolunteerApprovalInput input =
            makeApprovalInput(categoryId, 1.5);
        const VolunteerReviewOutcome preview =
            service.previewApproval(recordId, input);
        const VolunteerCategory *category = data.findCategory(categoryId);
        require(category != nullptr,
                "built-in review category should exist");
        const double expectedScore =
            std::round(category->calculateScore(
                           beforePreview->getAppliedDuration()) * 10.0) /
            10.0;
        const size_t logsBeforePreview = data.getOperationLogs().size();

        require(preview.succeeded(),
                "Pending record approval preview should succeed");
        require(preview.approvalScore.has_value(),
                "successful approval preview should return a score");
        requireNear(*preview.approvalScore, expectedScore,
                    "preview should normalize current category calculation to one decimal");
        require(data.getOperationLogs().size() == logsBeforePreview,
                "approval preview must not append an audit log");

        const VolunteerRecord *afterPreview = data.findRecord(recordId);
        require(afterPreview != nullptr &&
                    afterPreview->getStatus() == RecordStatus::Pending,
                "approval preview should not change record status");
        requireNear(afterPreview->getScore(), 0.0,
                    "approval preview should not change stored score");

        const VolunteerReviewOutcome approval = service.approve(
            "A9001", recordId, input);
        require(approval.succeeded(),
                "Pending record approval should succeed");
        require(approval.approvalScore.has_value(),
                "successful approval should return its authoritative score");
        requireNear(*approval.approvalScore, expectedScore,
                    "approval should recalculate with current category data");

        const VolunteerRecord *approved = data.findRecord(recordId);
        require(approved != nullptr &&
                    approved->getStatus() == RecordStatus::Approved,
                "successful review should transition record to Approved");
        require(approved->getFinalCategoryId() == categoryId &&
                    approved->getFinalDuration() == 1.5 &&
                    approved->getReviewerAccountId() == "A9001" &&
                    !approved->getReviewNote().has_value(),
                "unchanged approval should store final facts and reviewer without inventing a note");
        requireNear(*approved->getSettledCoefficient(),
                    category->getCoefficient(),
                    "approval should freeze the current category coefficient");
        requireNear(*approved->getFinalScore(), expectedScore,
                    "Approved record should store one-decimal final score");
        ++expectedLogCount;
        require(data.getOperationLogs().size() == expectedLogCount,
                "each successful approval should append exactly one log");
        const OperationLog &log = data.getOperationLogs().back();
        require(log.getOperatorAccountId() == "A9001" &&
                    log.getOperationType() ==
                        OperationType::VolunteerRecordApproved &&
                    log.getTargetType() ==
                        OperationTargetType::VolunteerRecord &&
                    log.getTargetId() == recordId &&
                    log.getDescription().find("最终类别：" + categoryId) != string::npos &&
                    log.getDescription().find("最终积分：") != string::npos,
                "approval audit should preserve operator and describe final settlement");
    }

    DataManager reloaded(temporaryDirectory.path());
    require(reloaded.loadAll(),
            "approved records and audit logs should reload together");
    require(reloaded.getOperationLogs().size() == 3,
            "all successful approvals should be durably recorded once");
    for (const string &categoryId : categoryIds)
    {
        const string recordId = "R_REVIEW_" + categoryId;
        const VolunteerRecord *record = reloaded.findRecord(recordId);
        require(record != nullptr &&
                record->getStatus() == RecordStatus::Approved,
                "approved state should persist with its audit history");
        bool matchingAuditFound = false;
        for (const OperationLog &log : reloaded.getOperationLogs())
        {
            if (log.getTargetId() == recordId &&
                log.getOperationType() ==
                    OperationType::VolunteerRecordApproved &&
                log.getOperatorAccountId() == "A9001")
            {
                matchingAuditFound = true;
                break;
            }
        }
        require(matchingAuditFound,
                "reloaded approved record should have its matching operator audit");
    }
}

void testVolunteerReviewRejectAndNonPendingGuards()
{
    ScopedTemporaryDirectory temporaryDirectory(
        "review_rejection_audit");
    createReviewRuntimeData(
        temporaryDirectory.path(),
        "R_REVIEW_REJECT|S9001|missing-category|2026/06/02|1.00|Campus|Witness|Synthetic|0|0.00\n"
        "R_REVIEW_APPROVED|S9001|C01|2026/06/03|1.00|Campus|Witness|Synthetic|1|2.00\n"
        "R_REVIEW_REJECTED|S9001|C02|2026/06/04|1.00|Campus|Witness|Synthetic|2|0.00\n");
    DataManager data(temporaryDirectory.path());
    require(data.loadAll(),
            "synthetic review files should load before rejection");
    VolunteerReviewService service(data);

    require(service.reject("A9001", "R_REVIEW_REJECT", "  \t  ").status ==
                VolunteerReviewStatus::ReviewNoteRequired,
            "blank rejection reason should be rejected before mutation");
    const VolunteerReviewOutcome rejection =
        service.reject("A9001", "R_REVIEW_REJECT", "补充材料不足");
    require(rejection.succeeded(),
            "Pending record should be rejectable without category lookup");
    const VolunteerRecord *rejected = data.findRecord("R_REVIEW_REJECT");
    require(rejected != nullptr &&
                rejected->getStatus() == RecordStatus::Rejected &&
                rejected->getReviewerAccountId() == "A9001" &&
                rejected->getReviewNote() == "补充材料不足" &&
                !rejected->getFinalScore().has_value(),
            "successful rejection should transition record to Rejected");
    requireNear(rejected->getScore(), 0.0,
                "rejection should preserve Domain score-reset behavior");
    require(data.getOperationLogs().size() == 1,
            "successful rejection should append exactly one log");
    const OperationLog &rejectionLog = data.getOperationLogs().front();
    require(rejectionLog.getOperatorAccountId() == "A9001" &&
                rejectionLog.getOperationType() ==
                    OperationType::VolunteerRecordRejected &&
                rejectionLog.getTargetId() == "R_REVIEW_REJECT" &&
                rejectionLog.getDescription().find("补充材料不足") != string::npos,
            "rejection audit should identify its operator and target");

    const vector<pair<string, RecordStatus>> nonPendingRecords = {
        {"R_REVIEW_APPROVED", RecordStatus::Approved},
        {"R_REVIEW_REJECTED", RecordStatus::Rejected}};
    for (const auto &[recordId, expectedStatus] : nonPendingRecords)
    {
        const VolunteerRecord *before = data.findRecord(recordId);
        require(before != nullptr,
                "non-Pending review fixture should exist");
        const double expectedScore = before->getScore();

        const VolunteerApprovalInput input = makeApprovalInput("C01", 1.0);
        require(service.previewApproval(recordId, input).status ==
                    VolunteerReviewStatus::RecordNotPending,
                "preview should reject non-Pending records");
        require(service.approve("A9001", recordId, input).status ==
                    VolunteerReviewStatus::RecordNotPending,
                "approval should reject non-Pending records");
        require(service.reject("A9001", recordId, "reason").status ==
                    VolunteerReviewStatus::RecordNotPending,
                "rejection should reject non-Pending records");

        const VolunteerRecord *after = data.findRecord(recordId);
        require(after != nullptr && after->getStatus() == expectedStatus,
                "non-Pending review attempts should preserve record status");
        requireNear(after->getScore(), expectedScore,
                    "non-Pending review attempts should preserve score");
    }
    require(data.getOperationLogs().size() == 1,
            "non-Pending actions must not add audit logs");

    DataManager reloaded(temporaryDirectory.path());
    require(reloaded.loadAll() && reloaded.getOperationLogs().size() == 1,
            "rejection state and its single audit fact should reload");
    const VolunteerRecord *reloadedRejected =
        reloaded.findRecord("R_REVIEW_REJECT");
    require(reloadedRejected != nullptr &&
                reloadedRejected->getStatus() == RecordStatus::Rejected,
            "rejected state should reload with the persisted audit fact");
}

void testVolunteerReviewReportsMissingRecordsAndCategories()
{
    ScopedTemporaryDirectory temporaryDirectory(
        "review_invalid_actions");
    createReviewRuntimeData(
        temporaryDirectory.path(),
        "R_REVIEW_NO_CATEGORY|S9001|missing-category|2026/06/05|1.50|Campus|Witness|Synthetic|0|7.00\n");
    DataManager data(temporaryDirectory.path());
    require(data.loadAll(),
            "synthetic review files should load before invalid actions");
    VolunteerReviewService service(data);

    const VolunteerApprovalInput validInput = makeApprovalInput("C01", 1.5);

    require(service.previewApproval("missing-record", validInput).status ==
                VolunteerReviewStatus::RecordNotFound,
            "preview should report a missing record");
    require(service.approve("A9001", "missing-record", validInput).status ==
                VolunteerReviewStatus::RecordNotFound,
            "approval should report a missing record");
    require(service.reject("A9001", "missing-record", "reason").status ==
                VolunteerReviewStatus::RecordNotFound,
            "rejection should report a missing record");

    const VolunteerApprovalInput missingCategoryInput =
        makeApprovalInput("missing-category", 1.5);
    require(service.previewApproval(
                "R_REVIEW_NO_CATEGORY", missingCategoryInput).status ==
                VolunteerReviewStatus::CategoryNotFound,
            "preview should report a missing category");
    require(service.approve(
                "A9001", "R_REVIEW_NO_CATEGORY", missingCategoryInput).status ==
                VolunteerReviewStatus::CategoryNotFound,
            "approval should report a missing category");
    const VolunteerRecord *unchanged =
        data.findRecord("R_REVIEW_NO_CATEGORY");
    require(unchanged != nullptr &&
                unchanged->getStatus() == RecordStatus::Pending,
            "missing-category preview and approval should preserve status");
    requireNear(unchanged->getScore(), 0.0,
                "legacy Pending row must not fabricate an unsettled score");
    require(data.getOperationLogs().empty(),
            "preview, missing record and missing category must not log");
}

void testVolunteerReviewCorrectionRequiresNoteAndFreezesSettlement()
{
    ScopedTemporaryDirectory temporaryDirectory("review_corrected_settlement");
    createReviewRuntimeData(temporaryDirectory.path(), "");
    DataManager data(temporaryDirectory.path());
    require(data.loadAll(), "empty synthetic review root should load");
    data.addRecord(makeRecord(
        "R_CORRECT", "S9001", "C01", "2026/10/07", 1.0,
        RecordStatus::Pending, 0.0));
    VolunteerReviewService service(data);

    const VolunteerApprovalInput categoryCorrectionWithoutNote =
        makeApprovalInput("C02", 1.0);
    const VolunteerReviewOutcome categoryPreview =
        service.previewApproval(
            "R_CORRECT", categoryCorrectionWithoutNote);
    require(categoryPreview.succeeded() &&
                categoryPreview.approvalScore == 1.5,
            "corrected category preview should return the authoritative score without a note");
    require(service.approve(
                "A9001", "R_CORRECT", categoryCorrectionWithoutNote).status ==
                VolunteerReviewStatus::ReviewNoteRequired,
            "corrected category approval should require a note before mutation");

    const VolunteerApprovalInput durationCorrectionWithoutNote =
        makeApprovalInput("C01", 1.5);
    const VolunteerReviewOutcome durationPreview =
        service.previewApproval(
            "R_CORRECT", durationCorrectionWithoutNote);
    require(durationPreview.succeeded(),
            "corrected duration preview should succeed without a note");
    require(durationPreview.approvalScore.has_value(),
            "corrected duration preview should include a score");
    requireNear(*durationPreview.approvalScore, 3.0,
                "corrected duration preview should return the authoritative score");
    require(service.approve(
                "A9001", "R_CORRECT", durationCorrectionWithoutNote).status ==
                VolunteerReviewStatus::ReviewNoteRequired,
            "corrected duration approval should require a note before mutation");

    VolunteerApprovalInput corrected = makeApprovalInput("C02", 1.5);
    const VolunteerReviewOutcome combinedPreview =
        service.previewApproval("R_CORRECT", corrected);
    require(combinedPreview.succeeded() &&
                combinedPreview.approvalScore == 2.3,
            "combined correction preview should return the authoritative score without a note");
    require(service.approve("A9001", "R_CORRECT", corrected).status ==
                VolunteerReviewStatus::ReviewNoteRequired,
            "combined correction approval should require a note before mutation");
    require(data.findRecord("R_CORRECT")->getStatus() == RecordStatus::Pending &&
                data.getOperationLogs().empty(),
            "note-required approvals must preserve the Pending record and create no log");

    const vector<double> invalidDurations = {
        0.0,
        -0.5,
        0.25,
        numeric_limits<double>::infinity(),
        numeric_limits<double>::quiet_NaN()};
    for (double duration : invalidDurations)
    {
        const VolunteerReviewOutcome invalid = service.previewApproval(
            "R_CORRECT", makeApprovalInput("C01", duration));
        require(invalid.status == VolunteerReviewStatus::InvalidFinalDuration,
                "zero, negative, non-half-hour or non-finite final duration should fail");
    }

    require(service.previewApproval(
                "R_CORRECT", makeApprovalInput("missing-category", 1.0)).status ==
                VolunteerReviewStatus::CategoryNotFound,
            "unknown final category should fail without mutation");
    require(service.previewApproval(
                "R_CORRECT", makeApprovalInput("C01", 1.0, "bad|note")).status ==
                VolunteerReviewStatus::InvalidReviewNote,
            "review note delimiter should be rejected");
    require(service.previewApproval(
                "R_CORRECT", makeApprovalInput("C01", 1.0, "bad\nnote")).status ==
                VolunteerReviewStatus::InvalidReviewNote,
            "review note newline should be rejected");

    const VolunteerRecord *pending = data.findRecord("R_CORRECT");
    require(pending != nullptr && pending->getStatus() == RecordStatus::Pending &&
                pending->getAppliedCategoryId() == "C01" &&
                !pending->getFinalScore().has_value() &&
                data.getOperationLogs().empty(),
            "all invalid previews and approvals must leave facts and logs untouched");

    corrected.reviewNote = "  类别及时长修正  ";
    const VolunteerReviewOutcome preview =
        service.previewApproval("R_CORRECT", corrected);
    require(preview.succeeded() && preview.approvalScore == 2.3,
            "preview should show the one-decimal corrected score");
    require(data.getOperationLogs().empty() &&
                data.findRecord("R_CORRECT")->getStatus() == RecordStatus::Pending,
            "corrected preview must remain read-only");

    const VolunteerReviewOutcome approved =
        service.approve("A9001", "R_CORRECT", corrected);
    require(approved.succeeded() && approved.approvalScore == 2.3,
            "corrected review should approve at the authoritative rounded score");
    const VolunteerRecord *settled = data.findRecord("R_CORRECT");
    require(settled != nullptr &&
                settled->getAppliedCategoryId() == "C01" &&
                settled->getAppliedDuration() == 1.0 &&
                settled->getFinalCategoryId() == "C02" &&
                settled->getFinalDuration() == 1.5 &&
                settled->getReviewerAccountId() == "A9001" &&
                settled->getReviewNote() == "类别及时长修正",
            "approval should preserve application facts and store normalized final review facts");
    requireNear(*settled->getSettledCoefficient(), 1.5,
                "approval should snapshot the final category coefficient");
    requireNear(*settled->getFinalScore(), 2.3,
                "1.5 hours times 1.5 coefficient should freeze as 2.3");
    require(data.getOperationLogs().size() == 1,
            "one corrected approval should append exactly one log");
    const string description = data.getOperationLogs().front().getDescription();
    require(description.find("类别调整：C01→C02") != string::npos &&
                description.find("时长调整：1.0→1.5") != string::npos &&
                description.find("审核意见：类别及时长修正") != string::npos,
            "single approval log should describe final facts, changes and note");

    const double frozenCoefficient = *settled->getSettledCoefficient();
    const double frozenScore = *settled->getFinalScore();
    VolunteerCategory currentCategory("C02", "当前类别", frozenCoefficient);
    currentCategory = VolunteerCategory("C02", "后续类别定义", 9.0);
    require(currentCategory.getCoefficient() != frozenCoefficient &&
                *settled->getSettledCoefficient() == frozenCoefficient &&
                *settled->getFinalScore() == frozenScore,
            "stored settlement snapshot should remain independent of later category definitions");
}

void testApprovedQueriesUseFinalReviewedSnapshots()
{
    DataManager data;
    data.addStudent(Student("S_FINAL", "Final Facts", "unused", "Class", "Major"));
    auto approved = VolunteerRecord::fromModernFields(
        "R_FINAL", "S_FINAL", "2026/10/07", "C01", 1.0,
        "Campus", "Witness", "Corrected record", RecordStatus::Approved,
        string("C02"), 1.5, string("A9001"), string("修正说明"), 1.5, 2.3);
    require(approved.has_value(), "modern corrected Approved record should construct");
    data.addRecord(*approved);

    requireNear(data.calculateStudentScore("S_FINAL"), 2.3,
                "total score should use finalScore");
    requireNear(
        data.calculateStudentScoreByDateRange(
            "S_FINAL", "2026/10/01", "2026/10/31"),
        2.3,
        "date-range score should use finalScore");
    requireNear(data.calculateStudentDurationByCategory("S_FINAL", "C01"), 0.0,
                "approved duration should not remain assigned to applied category");
    requireNear(data.calculateStudentDurationByCategory("S_FINAL", "C02"), 1.5,
                "approved duration should use final category and final duration");
    const vector<RankingItem> ranking = data.generateRanking();
    require(ranking.size() == 1 && ranking.front().studentId == "S_FINAL",
            "ranking should include the synthetic student");
    requireNear(ranking.front().score, 2.3,
                "ranking should consume finalScore-based totals");
}

void testCategoryAndDurationCorrectionsEachRequireANote()
{
    ScopedTemporaryDirectory temporaryDirectory("review_separate_corrections");
    createReviewRuntimeData(temporaryDirectory.path(), "");
    DataManager data(temporaryDirectory.path());
    require(data.loadAll(), "synthetic correction root should load");
    data.addRecord(makeRecord(
        "R_CATEGORY_ONLY", "S9001", "C01", "2026/10/07", 1.0,
        RecordStatus::Pending, 0.0));
    data.addRecord(makeRecord(
        "R_DURATION_ONLY", "S9001", "C01", "2026/10/07", 1.0,
        RecordStatus::Pending, 0.0));
    VolunteerReviewService service(data);

    const VolunteerApprovalInput categoryWithoutNote =
        makeApprovalInput("C02", 1.0);
    const VolunteerApprovalInput durationWithoutNote =
        makeApprovalInput("C01", 1.5);
    require(service.approve(
                "A9001", "R_CATEGORY_ONLY", categoryWithoutNote).status ==
                VolunteerReviewStatus::ReviewNoteRequired,
            "category-only correction should require a review note");
    require(service.approve(
                "A9001", "R_DURATION_ONLY", durationWithoutNote).status ==
                VolunteerReviewStatus::ReviewNoteRequired,
            "duration-only correction should require a review note");
    require(data.getOperationLogs().empty(),
            "blocked corrections must not create audit rows");

    require(service.approve(
                "A9001", "R_CATEGORY_ONLY",
                makeApprovalInput("C02", 1.0, "类别调整原因")).succeeded(),
            "category correction with a note should succeed");
    const VolunteerRecord *categoryCorrected =
        data.findRecord("R_CATEGORY_ONLY");
    require(categoryCorrected != nullptr &&
                categoryCorrected->getAppliedCategoryId() == "C01" &&
                categoryCorrected->getFinalCategoryId() == "C02" &&
                categoryCorrected->getFinalDuration() == 1.0,
            "category correction should retain application facts separately");

    require(service.approve(
                "A9001", "R_DURATION_ONLY",
                makeApprovalInput("C01", 1.5, "时长调整原因")).succeeded(),
            "duration correction with a note should succeed");
    const VolunteerRecord *durationCorrected =
        data.findRecord("R_DURATION_ONLY");
    require(durationCorrected != nullptr &&
                durationCorrected->getFinalCategoryId() == "C01" &&
                durationCorrected->getFinalDuration() == 1.5,
            "duration correction should store only the changed final fact");
    requireNear(*durationCorrected->getFinalScore(), 3.0,
                "corrected duration should determine finalScore");
    require(data.getOperationLogs().size() == 2,
            "each successful corrected approval should create one audit row");
}

void testReviewDoesNotImposeTwelveHourCap()
{
    ScopedTemporaryDirectory temporaryDirectory("review_no_twelve_hour_cap");
    createReviewRuntimeData(temporaryDirectory.path(), "");
    DataManager data(temporaryDirectory.path());
    require(data.loadAll(), "synthetic long-duration root should load");
    data.addRecord(makeRecord(
        "R_LONG", "S9001", "C01", "2026/10/07", 1.0,
        RecordStatus::Pending, 0.0));
    VolunteerReviewService service(data);

    const VolunteerApprovalInput input =
        makeApprovalInput("C01", 12.5, "核实服务时长");
    const VolunteerReviewOutcome approved =
        service.approve("A9001", "R_LONG", input);
    require(approved.succeeded(),
            "valid duration above 12 hours should not hit a new hard cap");
    requireNear(*data.findRecord("R_LONG")->getFinalDuration(), 12.5,
                "approved final duration should preserve values above 12 hours");
    requireNear(*data.findRecord("R_LONG")->getFinalScore(), 25.0,
                "score should derive from the uncapped final duration");
}

void testMixedLegacyAndModernRecordPersistence()
{
    ScopedTemporaryDirectory temporaryDirectory("mixed_record_formats");
    createSyntheticRuntimeData(temporaryDirectory.path());
    writeRuntimeFile(
        temporaryDirectory.path(),
        "records.txt",
        "R_LEGACY_APPROVED|S9001|C01|2026/10/01|1.50|Campus|Witness|Old approved|1|2.25\n"
        "R_LEGACY_PENDING|S9001|C01|2026/10/02|1.00|Campus|Witness|Old pending|0|0.00\n"
        "R_LEGACY_REJECTED|S9001|C02|2026/10/03|1.00|Campus|Witness|Old rejected|2|0.00\n"
        "R_MODERN_PENDING|S9001|2026/10/04|C01|1.0|Campus|Witness|Modern pending|0||||||\n"
        "R_MODERN_REJECTED|S9001|2026/10/05|C01|1.0|Campus|Witness|Modern rejected|2|||A9001|补充证明||\n"
        "R_MODERN_APPROVED|S9001|2026/10/06|C01|1.0|Campus|Witness|Modern approved|1|C02|1.5|A9001|类别修正|1.5|2.3\n");

    DataManager data(temporaryDirectory.path());
    require(data.loadRecords() && data.getRecords().size() == 6,
            "mixed legacy and modern records should load together");
    const VolunteerRecord *legacyApproved =
        data.findRecord("R_LEGACY_APPROVED");
    require(legacyApproved != nullptr &&
                legacyApproved->isLegacyCompatibilityRecord() &&
                legacyApproved->getFinalCategoryId() == "C01" &&
                legacyApproved->getFinalDuration() == 1.5 &&
                legacyApproved->getFinalScore() == 2.25 &&
                !legacyApproved->getReviewerAccountId().has_value() &&
                !legacyApproved->getReviewNote().has_value() &&
                !legacyApproved->getSettledCoefficient().has_value(),
            "legacy Approved should map known facts without fabricating review history");
    const VolunteerRecord *legacyRejected =
        data.findRecord("R_LEGACY_REJECTED");
    require(legacyRejected != nullptr &&
                legacyRejected->getStatus() == RecordStatus::Rejected &&
                legacyRejected->isLegacyCompatibilityRecord() &&
                !legacyRejected->getReviewerAccountId().has_value() &&
                !legacyRejected->getReviewNote().has_value() &&
                !legacyRejected->getFinalScore().has_value(),
            "legacy Rejected should preserve state without invented review facts");
    const VolunteerRecord *modernRejected =
        data.findRecord("R_MODERN_REJECTED");
    require(modernRejected != nullptr &&
                !modernRejected->isLegacyCompatibilityRecord() &&
                modernRejected->getReviewerAccountId() == "A9001" &&
                modernRejected->getReviewNote() == "补充证明" &&
                !modernRejected->getFinalCategoryId().has_value(),
            "modern Rejected invariants should round-trip");

    data.saveRecords();
    const string serialized = readFile(
        temporaryDirectory.path() / "records.txt");
    const auto countFields = [](const string &row)
    {
        return 1 + static_cast<int>(count(row.begin(), row.end(), '|'));
    };
    istringstream rows(serialized);
    string row;
    bool sawLegacyApproved = false;
    bool sawModernPending = false;
    bool sawModernRejected = false;
    bool sawModernApproved = false;
    while (getline(rows, row))
    {
        if (row.rfind("R_LEGACY_APPROVED|", 0) == 0)
        {
            sawLegacyApproved = countFields(row) == 10 &&
                                row.find("|2.25") != string::npos;
        }
        if (row.rfind("R_MODERN_APPROVED|", 0) == 0)
        {
            sawModernApproved = countFields(row) == 15 &&
                                row.find("|C02|1.5|A9001|类别修正|1.5|2.3") != string::npos;
        }
        if (row.rfind("R_MODERN_PENDING|", 0) == 0)
        {
            sawModernPending = countFields(row) == 15;
        }
        if (row.rfind("R_MODERN_REJECTED|", 0) == 0)
        {
            sawModernRejected = countFields(row) == 15;
        }
    }
    require(sawLegacyApproved && sawModernPending && sawModernRejected &&
                sawModernApproved,
            "one serializer should preserve legacy 10-field and all modern 15-field states");

    DataManager roundTripped(temporaryDirectory.path());
    require(roundTripped.loadRecords() &&
                roundTripped.getRecords().size() == 6 &&
                roundTripped.findRecord("R_MODERN_PENDING")->getStatus() ==
                    RecordStatus::Pending &&
                roundTripped.findRecord("R_MODERN_REJECTED")->getReviewNote() ==
                    "补充证明" &&
                roundTripped.findRecord("R_MODERN_APPROVED")->getFinalScore() ==
                    2.3,
            "Pending, Rejected and Approved modern records should survive round-trip");

    writeRuntimeFile(
        temporaryDirectory.path(),
        "records.txt",
        "R_KEEP|S9001|C01|2026/10/01|1.00|Campus|Witness|Keep|0|0.00\n"
        "R_BAD_MODERN|S9001|2026/10/02|C01|1.0|Campus|Witness|Bad|1|C01|1.0|| |2.0|2.0\n");
    require(!data.loadRecords(),
            "malformed modern Approved invariants should fail the full load");
    require(data.getRecords().size() == 6 &&
                data.findRecord("R_LEGACY_APPROVED") != nullptr &&
                data.findRecord("R_KEEP") == nullptr,
            "failed modern load should preserve the previous collection atomically");
}

OperationLog makeOperationLog(
    const string &logId,
    OperationType operationType,
    const string &targetId,
    const string &description,
    const string &operationTime)
{
    return OperationLog(
        logId,
        "A9001",
        operationType,
        OperationTargetType::VolunteerRecord,
        targetId,
        description,
        operationTime);
}

void testRejectedModificationClearsCurrentFactsAndKeepsAuditHistory()
{
    ScopedTemporaryDirectory temporaryDirectory("rejected_resubmit_history");
    createReviewRuntimeData(
        temporaryDirectory.path(),
        "R_REJECTED|S9001|2026/10/07|C01|1.0|Campus|Witness|Rejected|2|||A9001|请补充证明||\n");
    DataManager data(temporaryDirectory.path());
    require(data.loadAll(), "modern Rejected runtime data should load");
    data.addOperationLog(makeOperationLog(
        "LOG000001", OperationType::VolunteerRecordRejected,
        "R_REJECTED", "请补充证明", "2026-10-07T12:00:00"));
    StudentVolunteerService service(data);

    const StudentVolunteerOutcome modified = service.modify(
        "S9001", "R_REJECTED", validStudentVolunteerInput());
    require(modified.succeeded() && modified.recordId == "R_REJECTED",
            "owner modification should preserve stable record identity");
    const VolunteerRecord *resubmitted = data.findRecord("R_REJECTED");
    require(resubmitted != nullptr &&
                resubmitted->getStatus() == RecordStatus::Pending &&
                !resubmitted->getReviewerAccountId().has_value() &&
                !resubmitted->getReviewNote().has_value() &&
                !resubmitted->getFinalCategoryId().has_value() &&
                !resubmitted->getFinalDuration().has_value() &&
                !resubmitted->getSettledCoefficient().has_value() &&
                !resubmitted->getFinalScore().has_value(),
            "Rejected modification should clear current review and settlement facts");
    require(data.getOperationLogs().size() == 1 &&
                data.getOperationLogs().front().getTargetId() == "R_REJECTED",
            "student resubmission should leave historical OperationLog unchanged");
}

void testDataManagerLoadsModernReviewRecord()
{
    ScopedTemporaryDirectory temporaryDirectory("modern_review_record");
    createSyntheticRuntimeData(temporaryDirectory.path());
    writeRuntimeFile(
        temporaryDirectory.path(),
        "records.txt",
        "R_MODERN|S9001|2026/10/01|C01|1.0|Campus|Witness|Modern pending|0||||||\n");

    DataManager data(temporaryDirectory.path());
    require(data.loadRecords(),
            "a valid 15-field modern record should load");
    require(data.getRecords().size() == 1 &&
                data.findRecord("R_MODERN") != nullptr,
            "the modern record should be published after a successful load");
}

void testDataManagerDoesNotPublishPartialRecordLoad()
{
    ScopedTemporaryDirectory temporaryDirectory("atomic_record_load");
    createSyntheticRuntimeData(temporaryDirectory.path());
    writeRuntimeFile(
        temporaryDirectory.path(),
        "records.txt",
        "R_KEEP|S9001|C01|2026/10/01|1.00|Campus|Witness|Existing|0|0.00\n");

    DataManager data(temporaryDirectory.path());
    require(data.loadRecords() && data.getRecords().size() == 1,
            "the initial legacy collection should load");

    writeRuntimeFile(
        temporaryDirectory.path(),
        "records.txt",
        "R_NEW|S9001|2026/10/02|C02|1.0|Campus|Witness|Modern pending|0||||||\n"
        "malformed|row\n");

    require(!data.loadRecords(),
            "an unsupported or malformed row should fail the whole record load");
    require(data.getRecords().size() == 1 &&
                data.findRecord("R_KEEP") != nullptr &&
                data.findRecord("R_NEW") == nullptr,
            "a failed load must preserve the prior authoritative collection");
}

void testDataManagerScoreTotalsUseOneDecimalPrecision()
{
    ScopedTemporaryDirectory temporaryDirectory("one_decimal_score_total");
    createSyntheticRuntimeData(temporaryDirectory.path());
    writeRuntimeFile(
        temporaryDirectory.path(),
        "records.txt",
        "R_SCORE|S9001|C02|2026/10/03|1.50|Campus|Witness|Historical|1|2.25\n");

    DataManager data(temporaryDirectory.path());
    require(data.loadRecords(),
            "the historical score fixture should load");
    requireNear(data.calculateStudentScore("S9001"), 2.3,
                "score totals should expose one-decimal business precision");
    requireNear(
        data.calculateStudentScoreByDateRange(
            "S9001", "2026/10/01", "2026/10/31"),
        2.3,
        "date-range score totals should expose one-decimal precision");
}

void testOperationLogFieldsEnumsAndCsvRoundTrip()
{
    static_assert(
        !is_assignable<OperationLog &, OperationLog>::value,
        "OperationLog should not be assignable after construction");

    const string description = "Approved, note: \"checked\"\r\nsecond line";
    vector<OperationLog> original;
    original.push_back(makeOperationLog(
        "LOG000007",
        OperationType::VolunteerRecordApproved,
        "R_DELETED_TARGET",
        description,
        "2026-10-06T12:34:56"));
    original.push_back(makeOperationLog(
        "LOG000008",
        OperationType::VolunteerRecordRejected,
        "R0008",
        "",
        "2026-10-06T12:35:01"));

    require(operationTypeToken(OperationType::VolunteerRecordApproved) ==
                "VolunteerRecordApproved",
            "approved operation type should use its stable CSV token");
    require(operationTypeToken(OperationType::VolunteerRecordRejected) ==
                "VolunteerRecordRejected",
            "rejected operation type should use its stable CSV token");
    OperationType parsedType = OperationType::VolunteerRecordRejected;
    require(parseOperationTypeToken("VolunteerRecordApproved", parsedType) &&
                parsedType == OperationType::VolunteerRecordApproved,
            "approved operation type token should parse");
    require(!parseOperationTypeToken("UnknownOperation", parsedType),
            "unknown operation type token should fail parsing");
    unsigned long long parsedSequence = 0;
    require(!parseOperationLogSequence("LOG000000", parsedSequence),
            "zero should not be accepted as a generated log sequence");
    require(operationTargetTypeToken(OperationTargetType::VolunteerRecord) ==
                "VolunteerRecord",
            "target type should use its stable CSV token");

    const string csv = OperationLogCsvCodec::serialize(original);
    require(csv.rfind(
                "logId,operatorAccountId,operationType,targetType,targetId,description,operationTime\n",
                0) == 0,
            "OperationLog CSV should begin with its fixed header");
    vector<OperationLog> parsed;
    require(OperationLogCsvCodec::parse(csv, parsed),
            "valid OperationLog CSV should parse");
    require(parsed.size() == 2,
            "CSV round trip should preserve both audit facts");
    require(parsed[0].getLogId() == "LOG000007" &&
                parsed[0].getOperatorAccountId() == "A9001" &&
                parsed[0].getOperationType() ==
                    OperationType::VolunteerRecordApproved &&
                parsed[0].getTargetType() ==
                    OperationTargetType::VolunteerRecord &&
                parsed[0].getTargetId() == "R_DELETED_TARGET" &&
                parsed[0].getDescription() == description &&
                parsed[0].getOperationTime() == "2026-10-06T12:34:56",
            "CSV round trip should preserve immutable log field values");
    require(parsed[1].getDescription().empty(),
            "CSV round trip should preserve an allowed empty description");
}

void testOperationLogCsvRejectsDuplicateIdsWithoutPublishingPartialResults()
{
    const string csv =
        "logId,operatorAccountId,operationType,targetType,targetId,description,operationTime\n"
        "LOG000001,A9001,VolunteerRecordApproved,VolunteerRecord,R0001,first,2026-10-06T12:00:00\n"
        "LOG000001,A9001,VolunteerRecordRejected,VolunteerRecord,R0002,second,2026-10-06T12:01:00\n";

    vector<OperationLog> parsed;
    parsed.push_back(makeOperationLog(
        "LOG000099",
        OperationType::VolunteerRecordApproved,
        "R_EXISTING",
        "existing output",
        "2026-10-06T11:00:00"));

    require(!OperationLogCsvCodec::parse(csv, parsed),
            "valid CSV with duplicate log IDs should fail parsing");
    require(parsed.size() == 1 &&
                parsed[0].getLogId() == "LOG000099" &&
                parsed[0].getTargetId() == "R_EXISTING",
            "failed duplicate-ID parsing should leave output unchanged");
}

void testOperationLogServiceQueriesAndGeneratesNextId()
{
    DataManager data;
    data.addOperationLog(makeOperationLog(
        "LOG000002",
        OperationType::VolunteerRecordApproved,
        "R_MISSING_TARGET",
        "first",
        "2026-10-06T12:00:00"));
    data.addOperationLog(makeOperationLog(
        "LOG000009",
        OperationType::VolunteerRecordRejected,
        "R0009",
        "second",
        "2026-10-06T13:00:00"));
    data.addOperationLog(makeOperationLog(
        "LOG000004",
        OperationType::VolunteerRecordApproved,
        "R0004",
        "third",
        "2026-10-06T12:30:00"));

    OperationLogService service(data);
    const vector<OperationLogView> newestFirst = service.query();
    require(newestFirst.size() == 3 &&
                newestFirst[0].logId == "LOG000009" &&
                newestFirst[1].logId == "LOG000004" &&
                newestFirst[2].logId == "LOG000002" &&
                newestFirst[0].operatorAccountId == "A9001" &&
                newestFirst[0].targetId == "R0009" &&
                newestFirst[0].operationTime == "2026-10-06T13:00:00",
            "OperationLog query should return newest timestamps first");

    OperationLogQuery approvedQuery;
    approvedQuery.operationType =
        OperationType::VolunteerRecordApproved;
    const vector<OperationLogView> approved =
        service.query(approvedQuery);
    require(approved.size() == 2 &&
                approved[0].logId == "LOG000004" &&
                approved[1].logId == "LOG000002",
            "operation type query should filter and retain newest-first order");

    OperationLogQuery targetQuery;
    targetQuery.targetId = "R0004";
    const vector<OperationLogView> target =
        service.query(targetQuery);
    require(target.size() == 1 &&
                target[0].logId == "LOG000004" &&
                target[0].targetType ==
                    OperationTargetType::VolunteerRecord,
            "target query should filter by target type and target ID");

    targetQuery.operationType = OperationType::VolunteerRecordRejected;
    require(service.query(targetQuery).empty(),
            "combined type and target filters should be applied together");
    require(data.getRecords().empty(),
            "historical target queries should not require the target to resolve");

    const OperationLog appended = service.append(
        "A9002",
        OperationType::VolunteerRecordRejected,
        OperationTargetType::VolunteerRecord,
        "R_MISSING_TARGET",
        "later action");
    require(appended.getLogId() == "LOG000010",
            "new log ID should be one greater than the maximum existing ID");
    require(appended.getOperationTime().size() == 19 &&
                appended.getOperationTime()[4] == '-' &&
                appended.getOperationTime()[10] == 'T',
            "appended log should use local YYYY-MM-DDTHH:MM:SS time format");
}

void testHistoricalOperationLogTargetProtectsRecordId()
{
    DataManager data;
    data.addRecord(VolunteerRecord(
        "R0004", "S9001", "C01", "2026/10/01", 1.0,
        "Campus", "Witness", "Reviewed"));
    require(data.deleteRecord("R0004"),
            "synthetic record should be removable before testing history guard");
    data.addOperationLog(makeOperationLog(
        "LOG000001",
        OperationType::VolunteerRecordApproved,
        "R0004",
        "reviewed then removed",
        "2026-10-06T12:00:00"));

    require(data.generateRecordId() == "R0005",
            "historical VolunteerRecord target ID should not be reused");
}

void testOperationLogIsRequiredForLoadAll()
{
    ScopedTemporaryDirectory temporaryDirectory("missing_operation_logs");
    createSyntheticRuntimeData(temporaryDirectory.path());
    filesystem::remove(temporaryDirectory.path() / "operation_logs.csv");

    DataManager data(temporaryDirectory.path());
    require(!data.loadAll(),
            "missing required operation_logs.csv should fail loadAll");
}

void verifyReviewPersistenceFailureRestoresState(bool failPrepare)
{
    const string label = failPrepare
                             ? "review_service_prepare_failure"
                             : "review_service_commit_failure";
    ScopedTemporaryDirectory temporaryDirectory(label);
    createReviewRuntimeData(
        temporaryDirectory.path(),
        "R0001|S9001|C01|2026/10/01|1.00|Campus|Witness|Original|0|0.00\n");

    DataManager data(temporaryDirectory.path());
    require(data.loadAll(),
            "synthetic files should load before injected save failure");
    const string originalRecords =
        readFile(temporaryDirectory.path() / "records.txt");
    const string originalLogs =
        readFile(temporaryDirectory.path() / "operation_logs.csv");

    const filesystem::path blocker = temporaryDirectory.path() /
        (failPrepare ? "operation_logs.csv.tmp" : "records.txt.bak");
    filesystem::create_directory(blocker);

    VolunteerReviewService service(data);
    const VolunteerReviewOutcome outcome =
        service.approve("A9001", "R0001", makeApprovalInput("C01", 1.0));
    require(outcome.status == VolunteerReviewStatus::PersistenceFailure,
            "unchanged-file save failure should report PersistenceFailure");
    const VolunteerRecord *restored = data.findRecord("R0001");
    require(restored != nullptr &&
                restored->getStatus() == RecordStatus::Pending &&
                restored->getScore() == 0.0,
            "ordinary save failure should restore the record from disk");
    require(data.getOperationLogs().empty(),
            "ordinary save failure should discard the uncommitted log in memory");
    require(readFile(temporaryDirectory.path() / "records.txt") ==
                originalRecords &&
                readFile(temporaryDirectory.path() / "operation_logs.csv") ==
                    originalLogs,
            "ordinary save failure should leave both formal files unchanged");
}

void testReviewServiceRestoresAfterPrepareFailure()
{
    verifyReviewPersistenceFailureRestoresState(true);
}

void testReviewServiceRestoresAfterUnchangedCommitFailure()
{
    verifyReviewPersistenceFailureRestoresState(false);
}

void testReviewServiceReportsSeverePartialCommit()
{
    ScopedTemporaryDirectory temporaryDirectory(
        "review_service_severe_partial");
    createReviewRuntimeData(
        temporaryDirectory.path(),
        "R0001|S9001|C01|2026/10/01|1.00|Campus|Witness|Original|0|0.00\n");
    DataManager data(temporaryDirectory.path());
    require(data.loadAll(),
            "synthetic files should load before partial-commit test");
    const string originalRecords =
        readFile(temporaryDirectory.path() / "records.txt");
    const string originalLogs =
        readFile(temporaryDirectory.path() / "operation_logs.csv");
    filesystem::create_directory(
        temporaryDirectory.path() / "operation_logs.csv.bak");

    VolunteerReviewService service(data);
    const VolunteerReviewOutcome outcome =
        service.approve("A9001", "R0001", makeApprovalInput("C01", 1.0));
    require(outcome.status ==
                VolunteerReviewStatus::SeverePersistenceFailure,
            "partial formal commit must report severe persistence failure");
    require(!outcome.succeeded(),
            "partial formal commit must never report review success");
    require(readFile(temporaryDirectory.path() / "records.txt") !=
                originalRecords &&
                readFile(temporaryDirectory.path() / "operation_logs.csv") ==
                    originalLogs,
            "severe partial commit test should preserve the primitive's ordered outcome");
}

void testReviewPersistencePrepareFailureLeavesBothFilesUnchanged()
{
    ScopedTemporaryDirectory temporaryDirectory("review_prepare_failure");
    const string originalRecords =
        "R0001|S9001|C01|2026/10/01|1.00|Campus|Witness|Original|0|0.00\n";
    const string originalLogs =
        "logId,operatorAccountId,operationType,targetType,targetId,description,operationTime\n";
    writeRuntimeFile(temporaryDirectory.path(), "records.txt", originalRecords);
    writeRuntimeFile(
        temporaryDirectory.path(), "operation_logs.csv", originalLogs);
    filesystem::create_directory(
        temporaryDirectory.path() / "operation_logs.csv.tmp");

    DataManager data(temporaryDirectory.path());
    data.addRecord(makeRecord(
        "R0001", "S9001", "C01", "2026/10/01", 1.0,
        RecordStatus::Approved, 2.0));
    data.addOperationLog(makeOperationLog(
        "LOG000001",
        OperationType::VolunteerRecordApproved,
        "R0001",
        "reviewed",
        "2026-10-06T12:00:00"));

    const RecordLogPersistenceOutcome result =
        data.saveRecordsAndOperationLogs();
    require(result.status == RecordLogPersistenceStatus::PrepareFailure,
            "a second-file prepare failure should be reported before commit");
    require(readFile(temporaryDirectory.path() / "records.txt") ==
                originalRecords &&
                readFile(temporaryDirectory.path() / "operation_logs.csv") ==
                    originalLogs,
            "prepare failure should leave both formal files byte-for-byte unchanged");
    require(!filesystem::exists(
                temporaryDirectory.path() / "records.txt.tmp"),
            "prepare failure should clean the temporary file it created");
}

void testReviewPersistenceReportsOrderedSeverePartialCommit()
{
    ScopedTemporaryDirectory temporaryDirectory("review_partial_commit");
    const string originalRecords =
        "R0001|S9001|C01|2026/10/01|1.00|Campus|Witness|Original|0|0.00\n";
    const string originalLogs =
        "logId,operatorAccountId,operationType,targetType,targetId,description,operationTime\n";
    writeRuntimeFile(temporaryDirectory.path(), "records.txt", originalRecords);
    writeRuntimeFile(
        temporaryDirectory.path(), "operation_logs.csv", originalLogs);
    writeRuntimeFile(
        temporaryDirectory.path(),
        "operation_logs.csv.bak",
        "reserved backup path\n");

    DataManager data(temporaryDirectory.path());
    data.addRecord(makeRecord(
        "R0001", "S9001", "C01", "2026/10/01", 1.0,
        RecordStatus::Approved, 2.0));
    data.addOperationLog(makeOperationLog(
        "LOG000001",
        OperationType::VolunteerRecordApproved,
        "R0001",
        "reviewed",
        "2026-10-06T12:00:00"));

    const RecordLogPersistenceOutcome result =
        data.saveRecordsAndOperationLogs();
    require(result.status == RecordLogPersistenceStatus::SeverePartialCommit,
            "failure after the first formal replacement should be severe partial commit");
    require(readFile(temporaryDirectory.path() / "records.txt") !=
                originalRecords &&
                readFile(temporaryDirectory.path() / "operation_logs.csv") ==
                    originalLogs,
            "records must commit before logs, which are the final participant");
    require(filesystem::exists(
                temporaryDirectory.path() / "records.txt.bak"),
            "severe partial commit should preserve the prior records backup");
}

void testReviewPersistenceWritesAndReloadsBothFiles()
{
    ScopedTemporaryDirectory temporaryDirectory("review_commit_success");
    writeRuntimeFile(
        temporaryDirectory.path(),
        "records.txt",
        "R0001|S9001|C01|2026/10/01|1.00|Campus|Witness|Original|0|0.00\n");
    writeRuntimeFile(
        temporaryDirectory.path(),
        "operation_logs.csv",
        "logId,operatorAccountId,operationType,targetType,targetId,description,operationTime\n");

    DataManager data(temporaryDirectory.path());
    require(data.loadRecords() && data.loadOperationLogs(),
            "synthetic review files should load before commit");
    VolunteerRecord *record = data.findRecord("R0001");
    require(record != nullptr, "synthetic review record should load");
    require(record->approve("A9001", "C01", 1.0, 2.0, 2.0, ""),
            "synthetic pending record should approve before persistence");
    data.addOperationLog(makeOperationLog(
        "LOG000001",
        OperationType::VolunteerRecordApproved,
        "R0001",
        "approved",
        "2026-10-06T12:00:00"));

    const RecordLogPersistenceOutcome result =
        data.saveRecordsAndOperationLogs();
    require(result.status == RecordLogPersistenceStatus::Success,
            "both prepared files should commit successfully");

    DataManager reloaded(temporaryDirectory.path());
    require(reloaded.loadRecords() && reloaded.loadOperationLogs(),
            "both committed files should reload");
    require(reloaded.findRecord("R0001") != nullptr &&
                reloaded.findRecord("R0001")->getStatus() ==
                    RecordStatus::Approved &&
                reloaded.findRecord("R0001")->getFinalScore() == 2.0,
            "records.txt should contain the committed review state");
    require(reloaded.getOperationLogs().size() == 1 &&
                reloaded.getOperationLogs()[0].getLogId() == "LOG000001" &&
                reloaded.getOperationLogs()[0].getTargetId() == "R0001",
            "operation_logs.csv should contain the matching audit fact");
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
        testDiaryServicePublishesOnlyApprovedPostsWithRecordOwnerFacts();
        testDiaryServiceReturnsEmptyForIneligiblePosts();
        testDiaryLoaderSupportsTheApprovedModernRowShape();
        testDiaryLoaderEnforcesLikePersistenceInvariants();
        testDiaryServiceRequestModerationAndLikes();
        testDiaryServicePersistenceFailureSemantics();
        testLegacyDiaryTakedownDoesNotInventHistory();
        testDiaryPublicFeedOrdersModernBeforeStableLegacyPosts();
        testStudentVolunteerSubmitDurationAndCategoryRules();
        testStudentVolunteerModifyRejectsForeignApprovedAndInvalidChanges();
        testStudentVolunteerModifyResubmitsRejectedRecord();
        testStudentVolunteerDeleteRejectsForeignAndApprovedRecords();
        testStudentVolunteerRejectsMissingTargets();
        testVolunteerReviewPreviewAndApprovalUseCurrentCategoryScore();
        testVolunteerReviewRejectAndNonPendingGuards();
        testVolunteerReviewReportsMissingRecordsAndCategories();
        testVolunteerReviewCorrectionRequiresNoteAndFreezesSettlement();
        testApprovedQueriesUseFinalReviewedSnapshots();
        testCategoryAndDurationCorrectionsEachRequireANote();
        testReviewDoesNotImposeTwelveHourCap();
        testMixedLegacyAndModernRecordPersistence();
        testDataManagerLoadsFromExplicitSyntheticRoot();
        testDataManagerReportsMissingRootOrRequiredFile();
        testDataManagerScoreTotalsUseOneDecimalPrecision();
        testDataManagerDoesNotPublishPartialRecordLoad();
        testDataManagerLoadsModernReviewRecord();
        testExplicitDataRootIgnoresProcessWorkingDirectory();
        testOperationLogFieldsEnumsAndCsvRoundTrip();
        testRejectedModificationClearsCurrentFactsAndKeepsAuditHistory();
        testOperationLogCsvRejectsDuplicateIdsWithoutPublishingPartialResults();
        testOperationLogServiceQueriesAndGeneratesNextId();
        testHistoricalOperationLogTargetProtectsRecordId();
        testOperationLogIsRequiredForLoadAll();
        testReviewServiceRestoresAfterPrepareFailure();
        testReviewServiceRestoresAfterUnchangedCommitFailure();
        testReviewServiceReportsSeverePartialCommit();
        testReviewPersistencePrepareFailureLeavesBothFilesUnchanged();
        testReviewPersistenceReportsOrderedSeverePartialCommit();
        testReviewPersistenceWritesAndReloadsBothFiles();
        cout << "All core characterization checks passed." << endl;
        return 0;
    }
    catch (const exception &error)
    {
        cerr << "Core characterization failure: " << error.what() << endl;
        return 1;
    }
}
