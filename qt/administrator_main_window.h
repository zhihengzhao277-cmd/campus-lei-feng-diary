#ifndef ADMINISTRATOR_MAIN_WINDOW_H
#define ADMINISTRATOR_MAIN_WINDOW_H

#include <QWidget>
#include <string>

#include "volunteer_record.h"

class DataManager;
class QComboBox;
class QTableWidget;
class QLineEdit;
class QFrame;
class QPushButton;
class QLabel;
class QDoubleSpinBox;
class QListWidget;
class QStackedWidget;
class QTableView;
class QHBoxLayout;
class OperationLogTableModel;
enum class VolunteerReviewStatus;

class AdministratorMainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit AdministratorMainWindow(
        const std::string &accountId,
        DataManager *dataManager,
        QWidget *parent = nullptr);

signals:
    void logoutRequested();

private slots:
    void handleNavigationChanged(int row);
    void refreshHomePage();
    void refreshReviewPage();
    void applyReviewFilter();
    void showSelectedRecordDetail();
    void approveSelectedRecord();
    void rejectSelectedRecord();
    void updateReviewPreview();
    void refreshOperationLogPage();
    void applyOperationLogFilter();
    void refreshStatisticsPage();
    void createStudent();
    void createAdministrator();
    void refreshProfilePage();
    void changePassword();

private:
    std::string accountId;
    DataManager *dataManager;

    QLabel *welcomeLabel;

    QListWidget *navigationList;
    QStackedWidget *contentStack;

    QWidget *homePage;

    QLabel *studentCountLabel;
    QLabel *pendingCountLabel;
    QLabel *approvedCountLabel;
    QLabel *recordCountLabel;

    QWidget *reviewPage;

    QComboBox *reviewStatusFilter;
    QTableWidget *reviewTable;
    QLabel *reviewEmptyLabel;

    QFrame *reviewDetailFrame;

    QLabel *detailStudentLabel;
    QLabel *detailCategoryLabel;
    QLabel *detailDateLabel;
    QLabel *detailDurationLabel;
    QLabel *detailPlaceLabel;
    QLabel *detailWitnessLabel;
    QLabel *detailDescriptionLabel;
    QLabel *detailStatusLabel;
    QLabel *detailReviewerLabel;
    QLabel *detailScoreLabel;
    QLabel *detailFinalFactsLabel;
    QLabel *detailReviewNoteLabel;

    QComboBox *reviewFinalCategoryCombo;
    QDoubleSpinBox *reviewFinalDurationSpin;
    QLineEdit *reviewNoteEdit;
    QLabel *reviewPreviewScoreLabel;
    QWidget *reviewInputsContainer;

    QPushButton *approveButton;
    QPushButton *rejectButton;

    std::string selectedRecordId;

    QWidget *statisticsPage;

    QLabel *statisticsStudentCountLabel;
    QLabel *statisticsRecordCountLabel;
    QLabel *statisticsApprovedCountLabel;
    QLabel *statisticsDurationLabel;

    QTableWidget *categoryStatisticsTable;
    QTableWidget *statisticsRankingTable;

    QWidget *createStudentPage;

    QLineEdit *studentAccountEdit;
    QLineEdit *studentNameEdit;
    QLineEdit *studentPasswordEdit;
    QLineEdit *studentClassEdit;
    QLineEdit *studentMajorEdit;

    QWidget *createAdministratorPage;

    QLineEdit *administratorAccountEdit;
    QLineEdit *administratorNameEdit;
    QLineEdit *administratorPasswordEdit;

    QWidget *profilePage;

    QLabel *profileNameLabel;
    QLabel *profileAccountLabel;

    QWidget *operationLogPage;
    QComboBox *operationLogTypeFilter;
    QLineEdit *operationLogTargetIdEdit;
    QTableView *operationLogTable;
    QLabel *operationLogEmptyLabel;
    OperationLogTableModel *operationLogModel;

    void buildInterface();
    void buildHomePage();
    void buildReviewPage();
    void buildStatisticsPage();
    void buildCreateStudentPage();
    void buildCreateAdministratorPage();
    void buildProfilePage();
    void buildOperationLogPage();
    QFrame *buildOperationLogFilterCard();
    QFrame *buildOperationLogTableCard();
    void addOperationLogTypeFilter(QHBoxLayout *layout);
    void addOperationLogTargetFilter(QHBoxLayout *layout);
    void configureOperationLogTable();
    void connectOperationLogFilters(
        QPushButton *clearButton,
        QPushButton *refreshButton);
    void showReviewFailure(VolunteerReviewStatus status);

    QString categoryName(
        const std::string &categoryId) const;

    QString statusText(
        RecordStatus status) const;
};

#endif
