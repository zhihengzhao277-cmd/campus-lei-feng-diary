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
class QListWidget;
class QStackedWidget;

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

    QFrame *reviewDetailFrame;

    QLabel *detailStudentLabel;
    QLabel *detailCategoryLabel;
    QLabel *detailDateLabel;
    QLabel *detailDurationLabel;
    QLabel *detailPlaceLabel;
    QLabel *detailWitnessLabel;
    QLabel *detailDescriptionLabel;
    QLabel *detailStatusLabel;
    QLabel *detailScoreLabel;

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

    void buildInterface();
    void buildHomePage();
    void buildReviewPage();
    void buildStatisticsPage();
    void buildCreateStudentPage();
    void buildCreateAdministratorPage();
    void buildProfilePage();

    QString categoryName(
        const std::string &categoryId) const;

    QString statusText(
        RecordStatus status) const;
};

#endif