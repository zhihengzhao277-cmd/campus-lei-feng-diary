#ifndef STUDENT_MAIN_WINDOW_H
#define STUDENT_MAIN_WINDOW_H

#include <QWidget>

#include <string>

class QDateEdit;
class QDoubleSpinBox;
class QLineEdit;
class QTextEdit;
class QComboBox;
class QDateEdit;
class QScrollArea;
class QVBoxLayout;
class QHBoxLayout;
class DataManager;
class QLabel;
class QListWidget;
class QStackedWidget;
class QTableWidget;

class StudentMainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit StudentMainWindow(
        const std::string &accountId,
        DataManager *dataManager,
        QWidget *parent = nullptr);

signals:
    void logoutRequested();

private slots:
    void handleNavigationChanged(int row);
    void refreshMyRecords();
    void refreshDashboard();
    void applyRecordFilter();
    void clearRecordFilter();
    void modifySelectedRecord();
    void deleteSelectedRecord();
    void submitVolunteerRecord();
    void refreshScorePage();
    void calculateMonthlyScore();
    void calculateSemesterScore();
    void refreshRankingPage();
    void refreshBadgePage();
    void refreshDiaryPublishOptions();
    void refreshDiaryWall();
    void publishDiary();
    void refreshProfilePage();
    void changePassword();

private:
    std::string accountId;
    DataManager *dataManager;
    QLabel *welcomeLabel;
    QListWidget *navigationList;
    QStackedWidget *contentStack;
    QWidget *homePage;
    QLabel *dashboardGreetingLabel;
    QLabel *dashboardScoreLabel;
    QLabel *dashboardRankLabel;
    QLabel *dashboardApprovedRecordsLabel;
    QLabel *dashboardEmptyBadgesLabel;
    QWidget *recordsPage;
    QTableWidget *recordsTable;
    QLabel *recordsEmptyLabel;
    void buildInterface();
    void buildHomePage();
    void buildRecordsPage();
    QString statusText(int statusValue) const;
    QComboBox *categoryFilter;
    QDateEdit *startDateEdit;
    QDateEdit *endDateEdit;
    QString categoryName(const std::string &categoryId) const;
    QWidget *submitPage;
    QComboBox *submitCategoryCombo;
    QDateEdit *submitDateEdit;
    QDoubleSpinBox *submitDurationSpin;
    QLineEdit *submitPlaceEdit;
    QLineEdit *submitWitnessEdit;
    QTextEdit *submitDescriptionEdit;
    QWidget *scorePage;
    QLabel *totalScoreLabel;
    QLabel *monthlyScoreLabel;
    QLabel *semesterScoreLabel;
    QDateEdit *monthDateEdit;
    QDateEdit *semesterStartEdit;
    QDateEdit *semesterEndEdit;
    QWidget *rankingPage;
    QTableWidget *rankingTable;
    QWidget *badgePage;
    void buildScorePage();
    void buildSubmitPage();
    void buildRankingPage();
    QWidget *createRankingSpecialtyBadgeStrip(
        const std::string &studentId) const;
    void appendRankingSpecialtyBadge(
        QHBoxLayout *layout,
        const std::string &studentId,
        const std::string &categoryId) const;
    void addRankingTableRow(
        int row,
        int rank,
        const std::string &studentId,
        const std::string &studentName,
        double score);
    void buildBadgePage();
    void refreshAchievementCategory(
        const char *categoryId,
        const double *thresholds,
        const QString *levelNames);
    QString badgeText(
        const std::string &categoryId,
        double duration,
        int *earnedLevel = nullptr) const;
    QWidget *diaryPage;
    QWidget *diaryWallPage;

    QComboBox *diaryRecordCombo;
    QTextEdit *diaryMessageEdit;

    QScrollArea *diaryScrollArea;
    QWidget *diaryContainer;
    QVBoxLayout *diaryFeedLayout;
    void buildDiaryPage();
    void buildDiaryWallPage();
    QWidget *profilePage;
    QLabel *profileAccountLabel;
    QLabel *profileNameLabel;
    QLabel *profileClassLabel;
    QLabel *profileMajorLabel;
    void buildProfilePage();
};

#endif
