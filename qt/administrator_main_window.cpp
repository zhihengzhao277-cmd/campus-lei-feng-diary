#include "administrator_main_window.h"
#include "style_helper.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QFrame>
#include <QFont>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include "administrator.h"
#include "data_manager.h"
#include "volunteer_record.h"

namespace
{
    bool containsInvalidPersistenceCharacter(
        const QString &text)
    {
        return text.contains('|') ||
               text.contains('\n') ||
               text.contains('\r');
    }
}

AdministratorMainWindow::AdministratorMainWindow(
    const std::string &accountId,
    DataManager *dataManager,
    QWidget *parent)
    : QWidget(parent),
      accountId(accountId),
      dataManager(dataManager),
      welcomeLabel(nullptr),
      navigationList(nullptr),
      contentStack(nullptr),
      homePage(nullptr),
      studentCountLabel(nullptr),
      pendingCountLabel(nullptr),
      approvedCountLabel(nullptr),
      recordCountLabel(nullptr),
      reviewPage(nullptr),
      reviewStatusFilter(nullptr),
      reviewTable(nullptr),
      reviewDetailFrame(nullptr),
      detailStudentLabel(nullptr),
      detailCategoryLabel(nullptr),
      detailDateLabel(nullptr),
      detailDurationLabel(nullptr),
      detailPlaceLabel(nullptr),
      detailWitnessLabel(nullptr),
      detailDescriptionLabel(nullptr),
      detailStatusLabel(nullptr),
      detailScoreLabel(nullptr),
      approveButton(nullptr),
      rejectButton(nullptr),
      selectedRecordId(""),
      statisticsPage(nullptr),
      statisticsStudentCountLabel(nullptr),
      statisticsRecordCountLabel(nullptr),
      statisticsApprovedCountLabel(nullptr),
      statisticsDurationLabel(nullptr),
      categoryStatisticsTable(nullptr),
      statisticsRankingTable(nullptr),
      createStudentPage(nullptr),
      studentAccountEdit(nullptr),
      studentNameEdit(nullptr),
      studentPasswordEdit(nullptr),
      studentClassEdit(nullptr),
      studentMajorEdit(nullptr),
      createAdministratorPage(nullptr),
      administratorAccountEdit(nullptr),
      administratorNameEdit(nullptr),
      administratorPasswordEdit(nullptr),
      profilePage(nullptr),
      profileNameLabel(nullptr),
      profileAccountLabel(nullptr)
{
    setWindowTitle(
        "校园雷锋日记 - 管理员端");

    resize(1000, 650);

    buildInterface();
}

void AdministratorMainWindow::buildInterface()
{
    setStyleSheet(
        StyleHelper::pageBackground());

    QVBoxLayout *mainLayout =
        new QVBoxLayout(this);

    mainLayout->setContentsMargins(
        18,
        14,
        18,
        18);

    mainLayout->setSpacing(14);

    // =========================
    // 顶部
    // =========================

    QHBoxLayout *topLayout =
        new QHBoxLayout;

    QLabel *systemTitle =
        new QLabel(
            "校园雷锋日记 · 管理后台");

    QFont systemFont =
        systemTitle->font();

    systemFont.setPointSize(18);
    systemFont.setBold(true);

    systemTitle->setFont(systemFont);

    systemTitle->setStyleSheet(
        "QLabel {"
        "color: #222222;"
        "background: transparent;"
        "}");

    QString administratorName =
        "未知管理员";

    if (dataManager != nullptr)
    {
        Administrator *administrator =
            dataManager->findAdministrator(
                accountId);

        if (administrator != nullptr)
        {
            administratorName =
                QString::fromStdString(
                    administrator->getName());
        }
    }

    welcomeLabel =
        new QLabel(
            "管理员：" +
            administratorName +
            "（" +
            QString::fromStdString(
                accountId) +
            "）");

    welcomeLabel->setStyleSheet(
        "QLabel {"
        "color: #555555;"
        "background: transparent;"
        "}");

    QPushButton *logoutButton =
        new QPushButton("退出登录");

    logoutButton->setMinimumSize(
        90,
        36);

    logoutButton->setStyleSheet(
        "QPushButton {"
        "background-color: white;"
        "color: #555555;"
        "border: 1px solid #dddddd;"
        "border-radius: 9px;"
        "}"
        "QPushButton:hover {"
        "background-color: #eeeeee;"
        "}");

    topLayout->addWidget(
        systemTitle);

    topLayout->addStretch();

    topLayout->addWidget(
        welcomeLabel);

    topLayout->addWidget(
        logoutButton);

    mainLayout->addLayout(
        topLayout);

    // =========================
    // 主体
    // =========================

    QHBoxLayout *bodyLayout =
        new QHBoxLayout;

    bodyLayout->setSpacing(16);

    navigationList =
        new QListWidget;

    navigationList->setFocusPolicy(Qt::NoFocus);

    navigationList->setFixedWidth(
        180);

    navigationList->addItem(
        "管理员主页");

    navigationList->addItem(
        "志愿审核");

    navigationList->addItem(
        "数据统计");

    navigationList->addItem(
        "创建学生");

    navigationList->addItem(
        "创建管理员");

    navigationList->addItem(
        "个人信息");

    navigationList->setStyleSheet(
        StyleHelper::navigation());

    contentStack =
        new QStackedWidget;

    buildHomePage();
    buildReviewPage();
    buildStatisticsPage();
    buildCreateStudentPage();
    buildCreateAdministratorPage();
    buildProfilePage();

    contentStack->addWidget(
        homePage);

    contentStack->addWidget(
        reviewPage);

    contentStack->addWidget(
        statisticsPage);

    contentStack->addWidget(
        createStudentPage);

    contentStack->addWidget(
        createAdministratorPage);

    contentStack->addWidget(
        profilePage);

    bodyLayout->addWidget(
        navigationList);

    bodyLayout->addWidget(
        contentStack);

    mainLayout->addLayout(
        bodyLayout);

    // =========================
    // 信号
    // =========================

    connect(
        logoutButton,
        &QPushButton::clicked,
        this,
        &AdministratorMainWindow::
            logoutRequested);

    connect(
        navigationList,
        &QListWidget::currentRowChanged,
        this,
        &AdministratorMainWindow::
            handleNavigationChanged);

    navigationList->setCurrentRow(0);
}

void AdministratorMainWindow::buildHomePage()
{
    homePage = new QWidget;

    QVBoxLayout *mainLayout =
        new QVBoxLayout(homePage);

    mainLayout->setContentsMargins(
        8,
        4,
        8,
        8);

    mainLayout->setSpacing(18);

    // =========================
    // 标题
    // =========================

    QLabel *titleLabel =
        new QLabel("管理员主页");

    QFont titleFont =
        titleLabel->font();

    titleFont.setPointSize(20);
    titleFont.setBold(true);

    titleLabel->setFont(
        titleFont);

    titleLabel->setStyleSheet(
        "QLabel {"
        "color: #222222;"
        "background: transparent;"
        "}");

    QLabel *descriptionLabel =
        new QLabel(
            "查看校园志愿服务系统当前运行情况");

    descriptionLabel->setStyleSheet(
        "QLabel {"
        "color: #888888;"
        "font-size: 14px;"
        "background: transparent;"
        "}");

    mainLayout->addWidget(
        titleLabel);

    mainLayout->addWidget(
        descriptionLabel);

    // =========================
    // 统计卡片
    // =========================

    QHBoxLayout *cardLayout =
        new QHBoxLayout;

    cardLayout->setSpacing(16);

    // -------- 学生数 --------

    QFrame *studentCard =
        new QFrame;

    studentCard->setStyleSheet(
        StyleHelper::card());

    QVBoxLayout *studentLayout =
        new QVBoxLayout(
            studentCard);

    studentLayout->setContentsMargins(
        20,
        18,
        20,
        18);

    QLabel *studentTitle =
        new QLabel("学生数量");

    studentTitle->setStyleSheet(
        "color: #777777;"
        "background: transparent;");

    studentCountLabel =
        new QLabel("0");

    QFont numberFont =
        studentCountLabel->font();

    numberFont.setPointSize(25);
    numberFont.setBold(true);

    studentCountLabel->setFont(
        numberFont);

    studentCountLabel->setStyleSheet(
        "color: #222222;"
        "background: transparent;");

    studentLayout->addWidget(
        studentTitle);

    studentLayout->addWidget(
        studentCountLabel);

    // -------- 待审核 --------

    QFrame *pendingCard =
        new QFrame;

    pendingCard->setStyleSheet(
        "QFrame {"
        "background-color: white;"
        "border: 1px solid #eeeeee;"
        "border-radius: 16px;"
        "}");

    QVBoxLayout *pendingLayout =
        new QVBoxLayout(
            pendingCard);

    pendingLayout->setContentsMargins(
        20,
        18,
        20,
        18);

    QLabel *pendingTitle =
        new QLabel("待审核记录");

    pendingTitle->setStyleSheet(
        "color: #777777;"
        "background: transparent;");

    pendingCountLabel =
        new QLabel("0");

    pendingCountLabel->setFont(
        numberFont);

    pendingCountLabel->setStyleSheet(
        "color: #b91f35;"
        "background: transparent;");

    pendingLayout->addWidget(
        pendingTitle);

    pendingLayout->addWidget(
        pendingCountLabel);

    // -------- 已通过 --------

    QFrame *approvedCard =
        new QFrame;

    approvedCard->setStyleSheet(
        "QFrame {"
        "background-color: white;"
        "border: 1px solid #eeeeee;"
        "border-radius: 16px;"
        "}");

    QVBoxLayout *approvedLayout =
        new QVBoxLayout(
            approvedCard);

    approvedLayout->setContentsMargins(
        20,
        18,
        20,
        18);

    QLabel *approvedTitle =
        new QLabel("已通过记录");

    approvedTitle->setStyleSheet(
        "color: #777777;"
        "background: transparent;");

    approvedCountLabel =
        new QLabel("0");

    approvedCountLabel->setFont(
        numberFont);

    approvedCountLabel->setStyleSheet(
        "color: #222222;"
        "background: transparent;");

    approvedLayout->addWidget(
        approvedTitle);

    approvedLayout->addWidget(
        approvedCountLabel);

    // -------- 总记录 --------

    QFrame *recordCard =
        new QFrame;

    recordCard->setStyleSheet(
        "QFrame {"
        "background-color: white;"
        "border: 1px solid #eeeeee;"
        "border-radius: 16px;"
        "}");

    QVBoxLayout *recordLayout =
        new QVBoxLayout(
            recordCard);

    recordLayout->setContentsMargins(
        20,
        18,
        20,
        18);

    QLabel *recordTitle =
        new QLabel("志愿总记录");

    recordTitle->setStyleSheet(
        "color: #777777;"
        "background: transparent;");

    recordCountLabel =
        new QLabel("0");

    recordCountLabel->setFont(
        numberFont);

    recordCountLabel->setStyleSheet(
        "color: #222222;"
        "background: transparent;");

    recordLayout->addWidget(
        recordTitle);

    recordLayout->addWidget(
        recordCountLabel);

    cardLayout->addWidget(
        studentCard);

    cardLayout->addWidget(
        pendingCard);

    cardLayout->addWidget(
        approvedCard);

    cardLayout->addWidget(
        recordCard);

    mainLayout->addLayout(
        cardLayout);

    // =========================
    // 管理提示
    // =========================

    QFrame *tipCard =
        new QFrame;

    tipCard->setStyleSheet(
        "QFrame {"
        "background-color: white;"
        "border: 1px solid #eeeeee;"
        "border-radius: 16px;"
        "}");

    QVBoxLayout *tipLayout =
        new QVBoxLayout(
            tipCard);

    tipLayout->setContentsMargins(
        22,
        20,
        22,
        20);

    QLabel *tipTitle =
        new QLabel("管理工作台");

    QFont tipFont =
        tipTitle->font();

    tipFont.setPointSize(15);
    tipFont.setBold(true);

    tipTitle->setFont(
        tipFont);

    QLabel *tipText =
        new QLabel(
            "你可以通过左侧菜单进行志愿审核、"
            "查看数据统计，以及创建学生和管理员账号。");

    tipText->setWordWrap(true);

    tipText->setStyleSheet(
        "color: #777777;"
        "background: transparent;");

    tipLayout->addWidget(
        tipTitle);

    tipLayout->addWidget(
        tipText);

    mainLayout->addWidget(
        tipCard);

    mainLayout->addStretch();

    refreshHomePage();
}

void AdministratorMainWindow::
    refreshHomePage()
{
    if (dataManager == nullptr)
    {
        return;
    }

    int studentCount =
        static_cast<int>(
            dataManager
                ->getStudents()
                .size());

    int totalCount = 0;
    int pendingCount = 0;
    int approvedCount = 0;

    for (const VolunteerRecord &record :
         dataManager->getRecords())
    {
        ++totalCount;

        if (record.getStatus() ==
            RecordStatus::Pending)
        {
            ++pendingCount;
        }
        else if (record.getStatus() ==
                 RecordStatus::Approved)
        {
            ++approvedCount;
        }
    }

    studentCountLabel->setText(
        QString::number(
            studentCount));

    pendingCountLabel->setText(
        QString::number(
            pendingCount));

    approvedCountLabel->setText(
        QString::number(
            approvedCount));

    recordCountLabel->setText(
        QString::number(
            totalCount));
}

void AdministratorMainWindow::
    handleNavigationChanged(
        int row)
{
    if (row == 0)
    {
        refreshHomePage();

        contentStack->setCurrentWidget(
            homePage);

        return;
    }

    if (row == 1)
    {
        refreshReviewPage();

        contentStack->setCurrentWidget(
            reviewPage);

        return;
    }

    if (row == 2)
    {
        refreshStatisticsPage();

        contentStack->setCurrentWidget(
            statisticsPage);

        return;
    }

    if (row == 3)
    {
        contentStack->setCurrentWidget(
            createStudentPage);

        return;
    }

    if (row == 4)
    {
        contentStack->setCurrentWidget(
            createAdministratorPage);

        return;
    }

    if (row == 5)
    {
        refreshProfilePage();

        contentStack->setCurrentWidget(
            profilePage);

        return;
    }
}

QString AdministratorMainWindow::categoryName(
    const std::string &categoryId) const
{
    if (categoryId == "C01")
    {
        return "劳动服务";
    }

    if (categoryId == "C02")
    {
        return "环保服务";
    }

    if (categoryId == "C03")
    {
        return "互助服务";
    }

    return QString::fromStdString(
        categoryId);
}

QString AdministratorMainWindow::statusText(
    RecordStatus status) const
{
    if (status == RecordStatus::Pending)
    {
        return "待审核";
    }

    if (status == RecordStatus::Approved)
    {
        return "已通过";
    }

    return "已驳回";
}

void AdministratorMainWindow::buildReviewPage()
{
    reviewPage =
        new QWidget;

    QVBoxLayout *mainLayout =
        new QVBoxLayout(reviewPage);

    mainLayout->setContentsMargins(
        8,
        4,
        8,
        8);

    mainLayout->setSpacing(16);

    QLabel *titleLabel =
        new QLabel("志愿审核");

    QFont titleFont =
        titleLabel->font();

    titleFont.setPointSize(20);
    titleFont.setBold(true);

    titleLabel->setFont(
        titleFont);

    titleLabel->setStyleSheet(
        "QLabel {"
        "color: #222222;"
        "background: transparent;"
        "border: none;"
        "}");

    QLabel *descriptionLabel =
        new QLabel(
            "查看和审核学生提交的志愿服务记录");

    descriptionLabel->setStyleSheet(
        "QLabel {"
        "color: #888888;"
        "font-size: 14px;"
        "background: transparent;"
        "border: none;"
        "}");

    mainLayout->addWidget(
        titleLabel);

    mainLayout->addWidget(
        descriptionLabel);

    QHBoxLayout *filterLayout =
        new QHBoxLayout;

    QLabel *filterLabel =
        new QLabel("状态：");

    reviewStatusFilter =
        new QComboBox;

    reviewStatusFilter->addItem(
        "全部",
        -1);

    reviewStatusFilter->addItem(
        "待审核",
        static_cast<int>(
            RecordStatus::Pending));

    reviewStatusFilter->addItem(
        "已通过",
        static_cast<int>(
            RecordStatus::Approved));

    reviewStatusFilter->addItem(
        "已驳回",
        static_cast<int>(
            RecordStatus::Rejected));

    reviewStatusFilter->setMinimumSize(
        140,
        38);

    reviewStatusFilter->setStyleSheet(
        "QComboBox {"
        "background-color: white;"
        "border: 1px solid #dddddd;"
        "border-radius: 9px;"
        "padding: 6px 10px;"
        "}");

    QPushButton *refreshButton =
        new QPushButton("刷新");

    refreshButton->setMinimumSize(
        80,
        38);

    refreshButton->setStyleSheet(
        "QPushButton {"
        "background-color: white;"
        "border: 1px solid #dddddd;"
        "border-radius: 9px;"
        "color: #444444;"
        "}"
        "QPushButton:hover {"
        "background-color: #eeeeee;"
        "}");

    filterLayout->addWidget(
        filterLabel);

    filterLayout->addWidget(
        reviewStatusFilter);

    filterLayout->addStretch();

    filterLayout->addWidget(
        refreshButton);

    mainLayout->addLayout(
        filterLayout);

    reviewTable =
        new QTableWidget;

    reviewTable->setColumnCount(6);

    reviewTable->setHorizontalHeaderLabels(
        {"记录编号",
         "学生",
         "志愿类别",
         "服务日期",
         "服务时长",
         "状态"});

    reviewTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers);

    reviewTable->setSelectionBehavior(
        QAbstractItemView::SelectRows);

    reviewTable->setSelectionMode(
        QAbstractItemView::SingleSelection);

    reviewTable->verticalHeader()
        ->setVisible(false);

    reviewTable->horizontalHeader()
        ->setSectionResizeMode(
            QHeaderView::Stretch);

    reviewTable->setMinimumHeight(220);

    reviewTable->setStyleSheet(
        "QTableWidget {"
        "background-color: white;"
        "border: 1px solid #eeeeee;"
        "border-radius: 12px;"
        "gridline-color: #eeeeee;"
        "}"
        "QTableWidget::item {"
        "padding: 7px;"
        "}"
        "QTableWidget::item:selected {"
        "background-color: #fff0f2;"
        "color: #222222;"
        "}"
        "QHeaderView::section {"
        "background-color: #fafafa;"
        "border: none;"
        "border-bottom: 1px solid #eeeeee;"
        "padding: 8px;"
        "font-weight: bold;"
        "}");

    mainLayout->addWidget(
        reviewTable);

    reviewDetailFrame =
        new QFrame;

    reviewDetailFrame->setStyleSheet(
        "QFrame {"
        "background-color: white;"
        "border: 1px solid #eeeeee;"
        "border-radius: 16px;"
        "}");

    QVBoxLayout *detailLayout =
        new QVBoxLayout(
            reviewDetailFrame);

    detailLayout->setContentsMargins(
        22,
        18,
        22,
        18);

    detailLayout->setSpacing(10);

    QLabel *detailTitle =
        new QLabel("记录详情");

    QFont detailFont =
        detailTitle->font();

    detailFont.setPointSize(15);
    detailFont.setBold(true);

    detailTitle->setFont(
        detailFont);

    detailTitle->setStyleSheet(
        "background: transparent;"
        "border: none;");

    detailLayout->addWidget(
        detailTitle);

    detailStudentLabel = new QLabel;
    detailCategoryLabel = new QLabel;
    detailDateLabel = new QLabel;
    detailDurationLabel = new QLabel;
    detailPlaceLabel = new QLabel;
    detailWitnessLabel = new QLabel;
    detailDescriptionLabel = new QLabel;
    detailStatusLabel = new QLabel;
    detailScoreLabel = new QLabel;

    detailDescriptionLabel->setWordWrap(
        true);

    QString detailStyle =
        "QLabel {"
        "color: #444444;"
        "font-size: 14px;"
        "background: transparent;"
        "border: none;"
        "}";

    detailStudentLabel->setStyleSheet(detailStyle);
    detailCategoryLabel->setStyleSheet(detailStyle);
    detailDateLabel->setStyleSheet(detailStyle);
    detailDurationLabel->setStyleSheet(detailStyle);
    detailPlaceLabel->setStyleSheet(detailStyle);
    detailWitnessLabel->setStyleSheet(detailStyle);
    detailDescriptionLabel->setStyleSheet(detailStyle);
    detailStatusLabel->setStyleSheet(detailStyle);
    detailScoreLabel->setStyleSheet(detailStyle);

    detailLayout->addWidget(detailStudentLabel);
    detailLayout->addWidget(detailCategoryLabel);
    detailLayout->addWidget(detailDateLabel);
    detailLayout->addWidget(detailDurationLabel);
    detailLayout->addWidget(detailPlaceLabel);
    detailLayout->addWidget(detailWitnessLabel);
    detailLayout->addWidget(detailDescriptionLabel);
    detailLayout->addWidget(detailStatusLabel);
    detailLayout->addWidget(detailScoreLabel);

    QHBoxLayout *actionLayout =
        new QHBoxLayout;

    actionLayout->addStretch();

    approveButton =
        new QPushButton("通过");

    rejectButton =
        new QPushButton("驳回");

    approveButton->setMinimumSize(
        100,
        38);

    rejectButton->setMinimumSize(
        100,
        38);

    approveButton->setStyleSheet(
        "QPushButton {"
        "background-color: #b91f35;"
        "color: white;"
        "border: none;"
        "border-radius: 9px;"
        "font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "background-color: #9f192d;"
        "}");

    rejectButton->setStyleSheet(
        "QPushButton {"
        "background-color: white;"
        "color: #b91f35;"
        "border: 1px solid #b91f35;"
        "border-radius: 9px;"
        "font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "background-color: #fff0f2;"
        "}");

    actionLayout->addWidget(approveButton);
    actionLayout->addWidget(rejectButton);
    detailLayout->addLayout(actionLayout);
    mainLayout->addWidget(reviewDetailFrame);

    reviewDetailFrame->hide();

    connect(
        reviewStatusFilter,
        QOverload<int>::of(
            &QComboBox::currentIndexChanged),
        this,
        &AdministratorMainWindow::
            applyReviewFilter);

    connect(
        refreshButton,
        &QPushButton::clicked,
        this,
        &AdministratorMainWindow::
            refreshReviewPage);

    connect(
        reviewTable,
        &QTableWidget::cellClicked,
        this,
        [this](int, int)
        {
            showSelectedRecordDetail();
        });

    connect(
        approveButton,
        &QPushButton::clicked,
        this,
        &AdministratorMainWindow::
            approveSelectedRecord);

    connect(
        rejectButton,
        &QPushButton::clicked,
        this,
        &AdministratorMainWindow::
            rejectSelectedRecord);

    refreshReviewPage();
}

void AdministratorMainWindow::refreshReviewPage()
{
    if (dataManager == nullptr ||
        reviewTable == nullptr)
    {
        return;
    }

    selectedRecordId.clear();

    reviewDetailFrame->hide();
    reviewTable->setRowCount(0);

    int selectedStatus = -1;

    if (reviewStatusFilter != nullptr)
    {
        selectedStatus =
            reviewStatusFilter->currentData().toInt();
    }

    for (const VolunteerRecord &record :
         dataManager->getRecords())
    {
        if (selectedStatus != -1 &&
            static_cast<int>(record.getStatus()) !=
                selectedStatus)
        {
            continue;
        }

        QString studentText =
            QString::fromStdString(
                record.getStudentId());

        Student *student =
            dataManager->findStudent(
                record.getStudentId());

        if (student != nullptr)
        {
            studentText =
                QString::fromStdString(
                    student->getName()) +
                "（" +
                QString::fromStdString(
                    record.getStudentId()) +
                "）";
        }

        int row = reviewTable->rowCount();

        reviewTable->insertRow(row);

        reviewTable->setItem(
            row,
            0,
            new QTableWidgetItem(
                QString::fromStdString(
                    record.getRecordId())));

        reviewTable->setItem(
            row,
            1,
            new QTableWidgetItem(studentText));

        reviewTable->setItem(
            row,
            2,
            new QTableWidgetItem(
                categoryName(
                    record.getCategoryId())));

        reviewTable->setItem(
            row,
            3,
            new QTableWidgetItem(
                QString::fromStdString(
                    record.getDate())));

        reviewTable->setItem(
            row,
            4,
            new QTableWidgetItem(
                QString::number(
                    record.getDuration(),
                    'f',
                    1) +
                " 小时"));

        reviewTable->setItem(
            row,
            5,
            new QTableWidgetItem(
                statusText(
                    record.getStatus())));
    }
}

void AdministratorMainWindow::applyReviewFilter()
{
    refreshReviewPage();
}

void AdministratorMainWindow::showSelectedRecordDetail()
{
    if (dataManager == nullptr ||
        reviewTable == nullptr)
    {
        return;
    }

    int row = reviewTable->currentRow();

    if (row < 0)
    {
        reviewDetailFrame->hide();
        return;
    }

    QTableWidgetItem *idItem =
        reviewTable->item(row, 0);

    if (idItem == nullptr)
    {
        reviewDetailFrame->hide();
        return;
    }

    selectedRecordId =
        idItem->text().toStdString();

    VolunteerRecord *record =
        dataManager->findRecord(
            selectedRecordId);

    if (record == nullptr)
    {
        reviewDetailFrame->hide();
        return;
    }

    Student *student =
        dataManager->findStudent(
            record->getStudentId());

    QString studentText =
        QString::fromStdString(
            record->getStudentId());

    if (student != nullptr)
    {
        studentText =
            QString::fromStdString(
                student->getName()) +
            "（" +
            QString::fromStdString(
                record->getStudentId()) +
            "）";
    }

    detailStudentLabel->setText(
        "学生：" + studentText);

    detailCategoryLabel->setText(
        "志愿类别：" +
        categoryName(record->getCategoryId()));

    detailDateLabel->setText(
        "服务日期：" +
        QString::fromStdString(
            record->getDate()));

    detailDurationLabel->setText(
        "服务时长：" +
        QString::number(
            record->getDuration(),
            'f',
            1) +
        " 小时");

    detailPlaceLabel->setText(
        "服务地点：" +
        QString::fromStdString(
            record->getPlace()));

    detailWitnessLabel->setText(
        "证明人：" +
        QString::fromStdString(
            record->getWitness()));

    detailDescriptionLabel->setText(
        "服务描述：" +
        QString::fromStdString(
            record->getDescription()));

    detailStatusLabel->setText(
        "当前状态：" +
        statusText(record->getStatus()));

    if (record->getStatus() ==
        RecordStatus::Approved)
    {
        detailScoreLabel->setText(
            "获得积分：" +
            QString::number(
                record->getScore(),
                'f',
                2));

        detailScoreLabel->show();
    }
    else
    {
        detailScoreLabel->hide();
    }

    bool canReview =
        record->getStatus() ==
        RecordStatus::Pending;

    approveButton->setVisible(canReview);
    rejectButton->setVisible(canReview);
    reviewDetailFrame->show();
}

void AdministratorMainWindow::approveSelectedRecord()
{
    if (dataManager == nullptr ||
        selectedRecordId.empty())
    {
        return;
    }

    VolunteerRecord *record =
        dataManager->findRecord(
            selectedRecordId);

    if (record == nullptr ||
        record->getStatus() !=
            RecordStatus::Pending)
    {
        return;
    }

    double coefficient = 1.0;

    if (record->getCategoryId() == "C01")
    {
        coefficient = 2.0;
    }
    else if (record->getCategoryId() == "C02")
    {
        coefficient = 1.5;
    }

    double finalScore =
        record->getDuration() * coefficient;

    QMessageBox::StandardButton result =
        QMessageBox::question(
            this,
            "确认通过",
            "确定通过记录 " +
                QString::fromStdString(
                    record->getRecordId()) +
                " 吗？\n\n"
                "审核通过后获得积分：" +
                QString::number(
                    finalScore,
                    'f',
                    2),
            QMessageBox::Yes |
                QMessageBox::No,
            QMessageBox::No);

    if (result != QMessageBox::Yes)
    {
        return;
    }

    record->approve(finalScore);
    dataManager->saveRecords();

    refreshReviewPage();
    refreshHomePage();

    QMessageBox::information(
        this,
        "审核完成",
        "该志愿记录已审核通过。");
}

void AdministratorMainWindow::rejectSelectedRecord()
{
    if (dataManager == nullptr ||
        selectedRecordId.empty())
    {
        return;
    }

    VolunteerRecord *record =
        dataManager->findRecord(
            selectedRecordId);

    if (record == nullptr ||
        record->getStatus() !=
            RecordStatus::Pending)
    {
        return;
    }

    QMessageBox::StandardButton result =
        QMessageBox::question(
            this,
            "确认驳回",
            "确定驳回记录 " +
                QString::fromStdString(
                    record->getRecordId()) +
                " 吗？",
            QMessageBox::Yes |
                QMessageBox::No,
            QMessageBox::No);

    if (result != QMessageBox::Yes)
    {
        return;
    }

    record->reject();
    dataManager->saveRecords();

    refreshReviewPage();
    refreshHomePage();

    QMessageBox::information(
        this,
        "审核完成",
        "该志愿记录已驳回。");
}

void AdministratorMainWindow::buildStatisticsPage()
{
    statisticsPage =
        new QWidget;

    QVBoxLayout *mainLayout =
        new QVBoxLayout(statisticsPage);

    mainLayout->setContentsMargins(
        8,
        4,
        8,
        8);

    mainLayout->setSpacing(16);

    QHBoxLayout *titleLayout =
        new QHBoxLayout;

    QVBoxLayout *titleTextLayout =
        new QVBoxLayout;

    QLabel *titleLabel =
        new QLabel("数据统计");

    QFont titleFont =
        titleLabel->font();

    titleFont.setPointSize(20);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);

    titleLabel->setStyleSheet(
        "QLabel {"
        "color: #222222;"
        "background: transparent;"
        "}");

    QLabel *descriptionLabel =
        new QLabel(
            "查看校园志愿服务的整体统计情况");

    descriptionLabel->setStyleSheet(
        "QLabel {"
        "color: #888888;"
        "font-size: 14px;"
        "background: transparent;"
        "}");

    titleTextLayout->addWidget(titleLabel);
    titleTextLayout->addWidget(descriptionLabel);

    QPushButton *refreshButton =
        new QPushButton("刷新统计");

    refreshButton->setMinimumSize(
        100,
        38);

    refreshButton->setStyleSheet(
        "QPushButton {"
        "background-color: white;"
        "border: 1px solid #dddddd;"
        "border-radius: 9px;"
        "color: #444444;"
        "}"
        "QPushButton:hover {"
        "background-color: #eeeeee;"
        "}");

    titleLayout->addLayout(titleTextLayout);
    titleLayout->addStretch();
    titleLayout->addWidget(refreshButton);
    mainLayout->addLayout(titleLayout);

    QHBoxLayout *cardLayout =
        new QHBoxLayout;

    cardLayout->setSpacing(16);

    QFont numberFont;
    numberFont.setPointSize(23);
    numberFont.setBold(true);

    QFrame *studentCard = new QFrame;
    studentCard->setStyleSheet(
        "QFrame {"
        "background-color: white;"
        "border: 1px solid #eeeeee;"
        "border-radius: 16px;"
        "}");

    QVBoxLayout *studentLayout =
        new QVBoxLayout(studentCard);

    studentLayout->setContentsMargins(
        18,
        16,
        18,
        16);

    QLabel *studentTitle =
        new QLabel("学生总数");

    studentTitle->setStyleSheet(
        "color: #777777;"
        "background: transparent;");

    statisticsStudentCountLabel =
        new QLabel("0");

    statisticsStudentCountLabel->setFont(numberFont);
    studentLayout->addWidget(studentTitle);
    studentLayout->addWidget(
        statisticsStudentCountLabel);

    QFrame *recordCard = new QFrame;
    recordCard->setStyleSheet(
        "QFrame {"
        "background-color: white;"
        "border: 1px solid #eeeeee;"
        "border-radius: 16px;"
        "}");

    QVBoxLayout *recordLayout =
        new QVBoxLayout(recordCard);

    recordLayout->setContentsMargins(
        18,
        16,
        18,
        16);

    QLabel *recordTitle =
        new QLabel("志愿总记录");

    recordTitle->setStyleSheet(
        "color: #777777;"
        "background: transparent;");

    statisticsRecordCountLabel =
        new QLabel("0");

    statisticsRecordCountLabel->setFont(numberFont);
    recordLayout->addWidget(recordTitle);
    recordLayout->addWidget(
        statisticsRecordCountLabel);

    QFrame *approvedCard = new QFrame;
    approvedCard->setStyleSheet(
        "QFrame {"
        "background-color: white;"
        "border: 1px solid #eeeeee;"
        "border-radius: 16px;"
        "}");

    QVBoxLayout *approvedLayout =
        new QVBoxLayout(approvedCard);

    approvedLayout->setContentsMargins(
        18,
        16,
        18,
        16);

    QLabel *approvedTitle =
        new QLabel("已通过记录");

    approvedTitle->setStyleSheet(
        "color: #777777;"
        "background: transparent;");

    statisticsApprovedCountLabel =
        new QLabel("0");

    statisticsApprovedCountLabel->setFont(numberFont);
    approvedLayout->addWidget(approvedTitle);
    approvedLayout->addWidget(
        statisticsApprovedCountLabel);

    QFrame *durationCard = new QFrame;
    durationCard->setStyleSheet(
        "QFrame {"
        "background-color: white;"
        "border: 1px solid #eeeeee;"
        "border-radius: 16px;"
        "}");

    QVBoxLayout *durationLayout =
        new QVBoxLayout(durationCard);

    durationLayout->setContentsMargins(
        18,
        16,
        18,
        16);

    QLabel *durationTitle =
        new QLabel("总志愿时长");

    durationTitle->setStyleSheet(
        "color: #777777;"
        "background: transparent;");

    statisticsDurationLabel =
        new QLabel("0.0 小时");

    statisticsDurationLabel->setFont(numberFont);
    durationLayout->addWidget(durationTitle);
    durationLayout->addWidget(
        statisticsDurationLabel);

    cardLayout->addWidget(studentCard);
    cardLayout->addWidget(recordCard);
    cardLayout->addWidget(approvedCard);
    cardLayout->addWidget(durationCard);
    mainLayout->addLayout(cardLayout);

    QLabel *categoryTitle =
        new QLabel("分类统计");

    QFont sectionFont =
        categoryTitle->font();

    sectionFont.setPointSize(15);
    sectionFont.setBold(true);
    categoryTitle->setFont(sectionFont);
    mainLayout->addWidget(categoryTitle);

    categoryStatisticsTable =
        new QTableWidget;

    categoryStatisticsTable->setColumnCount(4);
    categoryStatisticsTable->setRowCount(3);

    categoryStatisticsTable->setHorizontalHeaderLabels(
        {"志愿类别",
         "已通过记录数",
         "总时长",
         "总积分"});

    categoryStatisticsTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers);

    categoryStatisticsTable->setSelectionMode(
        QAbstractItemView::NoSelection);

    categoryStatisticsTable->verticalHeader()
        ->setVisible(false);

    categoryStatisticsTable->horizontalHeader()
        ->setSectionResizeMode(
            QHeaderView::Stretch);

    categoryStatisticsTable->setMaximumHeight(165);

    categoryStatisticsTable->setStyleSheet(
        "QTableWidget {"
        "background-color: white;"
        "border: 1px solid #eeeeee;"
        "border-radius: 12px;"
        "gridline-color: #eeeeee;"
        "}"
        "QHeaderView::section {"
        "background-color: #fafafa;"
        "border: none;"
        "border-bottom: 1px solid #eeeeee;"
        "padding: 7px;"
        "font-weight: bold;"
        "}");

    mainLayout->addWidget(categoryStatisticsTable);

    QLabel *rankingTitle =
        new QLabel("积分排行榜");

    rankingTitle->setFont(sectionFont);
    mainLayout->addWidget(rankingTitle);

    statisticsRankingTable =
        new QTableWidget;

    statisticsRankingTable->setColumnCount(4);

    statisticsRankingTable->setHorizontalHeaderLabels(
        {"排名",
         "学生账号",
         "学生姓名",
         "总积分"});

    statisticsRankingTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers);

    statisticsRankingTable->setSelectionBehavior(
        QAbstractItemView::SelectRows);

    statisticsRankingTable->setSelectionMode(
        QAbstractItemView::SingleSelection);

    statisticsRankingTable->verticalHeader()
        ->setVisible(false);

    statisticsRankingTable->horizontalHeader()
        ->setSectionResizeMode(
            QHeaderView::Stretch);

    statisticsRankingTable->setStyleSheet(
        "QTableWidget {"
        "background-color: white;"
        "border: 1px solid #eeeeee;"
        "border-radius: 12px;"
        "gridline-color: #eeeeee;"
        "}"
        "QTableWidget::item:selected {"
        "background-color: #fff0f2;"
        "color: #222222;"
        "}"
        "QHeaderView::section {"
        "background-color: #fafafa;"
        "border: none;"
        "border-bottom: 1px solid #eeeeee;"
        "padding: 7px;"
        "font-weight: bold;"
        "}");

    mainLayout->addWidget(statisticsRankingTable);

    connect(
        refreshButton,
        &QPushButton::clicked,
        this,
        &AdministratorMainWindow::
            refreshStatisticsPage);

    refreshStatisticsPage();
}

void AdministratorMainWindow::refreshStatisticsPage()
{
    if (dataManager == nullptr)
    {
        return;
    }

    int studentCount =
        static_cast<int>(
            dataManager->getStudents().size());

    int recordCount =
        static_cast<int>(
            dataManager->getRecords().size());

    int approvedCount = 0;
    double totalDuration = 0.0;

    int categoryCount[3] =
        {0, 0, 0};

    double categoryDuration[3] =
        {0.0, 0.0, 0.0};

    double categoryScore[3] =
        {0.0, 0.0, 0.0};

    for (const VolunteerRecord &record :
         dataManager->getRecords())
    {
        if (record.getStatus() !=
            RecordStatus::Approved)
        {
            continue;
        }

        ++approvedCount;
        totalDuration += record.getDuration();

        int index = -1;

        if (record.getCategoryId() == "C01")
        {
            index = 0;
        }
        else if (record.getCategoryId() == "C02")
        {
            index = 1;
        }
        else if (record.getCategoryId() == "C03")
        {
            index = 2;
        }

        if (index >= 0)
        {
            ++categoryCount[index];
            categoryDuration[index] +=
                record.getDuration();
            categoryScore[index] +=
                record.getScore();
        }
    }

    statisticsStudentCountLabel->setText(
        QString::number(studentCount));

    statisticsRecordCountLabel->setText(
        QString::number(recordCount));

    statisticsApprovedCountLabel->setText(
        QString::number(approvedCount));

    statisticsDurationLabel->setText(
        QString::number(
            totalDuration,
            'f',
            1) +
        " 小时");

    const QString categoryNames[3] =
        {
            "劳动服务",
            "环保服务",
            "互助服务"};

    for (int i = 0; i < 3; ++i)
    {
        categoryStatisticsTable->setItem(
            i,
            0,
            new QTableWidgetItem(
                categoryNames[i]));

        categoryStatisticsTable->setItem(
            i,
            1,
            new QTableWidgetItem(
                QString::number(
                    categoryCount[i])));

        categoryStatisticsTable->setItem(
            i,
            2,
            new QTableWidgetItem(
                QString::number(
                    categoryDuration[i],
                    'f',
                    1) +
                " 小时"));

        categoryStatisticsTable->setItem(
            i,
            3,
            new QTableWidgetItem(
                QString::number(
                    categoryScore[i],
                    'f',
                    2)));
    }

    statisticsRankingTable->setRowCount(0);

    std::vector<RankingItem> ranking =
        dataManager->generateRanking();

    for (size_t i = 0; i < ranking.size(); ++i)
    {
        const RankingItem &item = ranking[i];

        int row =
            statisticsRankingTable->rowCount();

        statisticsRankingTable->insertRow(row);

        statisticsRankingTable->setItem(
            row,
            0,
            new QTableWidgetItem(
                QString::number(
                    static_cast<int>(i + 1))));

        statisticsRankingTable->setItem(
            row,
            1,
            new QTableWidgetItem(
                QString::fromStdString(
                    item.studentId)));

        statisticsRankingTable->setItem(
            row,
            2,
            new QTableWidgetItem(
                QString::fromStdString(
                    item.studentName)));

        statisticsRankingTable->setItem(
            row,
            3,
            new QTableWidgetItem(
                QString::number(
                    item.score,
                    'f',
                    2)));
    }
}

void AdministratorMainWindow::buildCreateStudentPage()
{
    createStudentPage =
        new QWidget;

    QVBoxLayout *mainLayout =
        new QVBoxLayout(createStudentPage);

    mainLayout->setContentsMargins(
        28,
        22,
        28,
        22);

    mainLayout->setSpacing(18);

    QLabel *titleLabel =
        new QLabel("创建学生");

    QFont titleFont =
        titleLabel->font();

    titleFont.setPointSize(20);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);

    QLabel *descriptionLabel =
        new QLabel(
            "创建新的学生登录账号和基本信息");

    descriptionLabel->setStyleSheet(
        "color: #888888;"
        "font-size: 14px;"
        "background: transparent;");

    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(descriptionLabel);

    QFrame *formCard = new QFrame;

    formCard->setStyleSheet(
        "QFrame {"
        "background-color: white;"
        "border: 1px solid #eeeeee;"
        "border-radius: 18px;"
        "}");

    QVBoxLayout *cardLayout =
        new QVBoxLayout(formCard);

    cardLayout->setContentsMargins(
        28,
        26,
        28,
        26);

    cardLayout->setSpacing(18);

    QFormLayout *formLayout =
        new QFormLayout;

    formLayout->setHorizontalSpacing(30);
    formLayout->setVerticalSpacing(16);

    studentAccountEdit = new QLineEdit;
    studentNameEdit = new QLineEdit;
    studentPasswordEdit = new QLineEdit;
    studentClassEdit = new QLineEdit;
    studentMajorEdit = new QLineEdit;

    studentAccountEdit->setPlaceholderText(
        "例如：20250002");

    studentNameEdit->setPlaceholderText(
        "请输入学生姓名");

    studentPasswordEdit->setPlaceholderText(
        "请输入初始密码");

    studentClassEdit->setPlaceholderText(
        "例如：人工智能1班");

    studentMajorEdit->setPlaceholderText(
        "例如：人工智能");

    studentPasswordEdit->setEchoMode(
        QLineEdit::Password);

    QString editStyle =
        "QLineEdit {"
        "background-color: white;"
        "border: 1px solid #dddddd;"
        "border-radius: 10px;"
        "padding: 8px 12px;"
        "font-size: 14px;"
        "}"
        "QLineEdit:focus {"
        "border: 1px solid #b91f35;"
        "}";

    studentAccountEdit->setStyleSheet(editStyle);
    studentNameEdit->setStyleSheet(editStyle);
    studentPasswordEdit->setStyleSheet(editStyle);
    studentClassEdit->setStyleSheet(editStyle);
    studentMajorEdit->setStyleSheet(editStyle);

    studentAccountEdit->setMinimumHeight(40);
    studentNameEdit->setMinimumHeight(40);
    studentPasswordEdit->setMinimumHeight(40);
    studentClassEdit->setMinimumHeight(40);
    studentMajorEdit->setMinimumHeight(40);

    formLayout->addRow(
        "学生账号：",
        studentAccountEdit);

    formLayout->addRow(
        "学生姓名：",
        studentNameEdit);

    formLayout->addRow(
        "初始密码：",
        studentPasswordEdit);

    formLayout->addRow(
        "班级：",
        studentClassEdit);

    formLayout->addRow(
        "专业：",
        studentMajorEdit);

    cardLayout->addLayout(formLayout);

    QHBoxLayout *buttonLayout =
        new QHBoxLayout;

    buttonLayout->addStretch();

    QPushButton *createButton =
        new QPushButton("创建学生");

    createButton->setMinimumSize(
        120,
        42);

    createButton->setCursor(
        Qt::PointingHandCursor);

    createButton->setStyleSheet(
        "QPushButton {"
        "background-color: #b91f35;"
        "color: white;"
        "border: none;"
        "border-radius: 10px;"
        "font-size: 14px;"
        "font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "background-color: #9f192d;"
        "}");

    buttonLayout->addWidget(createButton);
    cardLayout->addLayout(buttonLayout);
    mainLayout->addWidget(formCard);
    mainLayout->addStretch();

    connect(
        createButton,
        &QPushButton::clicked,
        this,
        &AdministratorMainWindow::createStudent);
}

void AdministratorMainWindow::createStudent()
{
    if (dataManager == nullptr)
    {
        return;
    }

    std::string newAccountId =
        studentAccountEdit->text()
            .trimmed()
            .toStdString();

    std::string name =
        studentNameEdit->text()
            .trimmed()
            .toStdString();

    std::string password =
        studentPasswordEdit->text()
            .toStdString();

    std::string className =
        studentClassEdit->text()
            .trimmed()
            .toStdString();

    std::string major =
        studentMajorEdit->text()
            .trimmed()
            .toStdString();

    if (newAccountId.empty() ||
        name.empty() ||
        password.empty() ||
        className.empty() ||
        major.empty())
    {
        QMessageBox::information(
            this,
            "提示",
            "请完整填写学生信息。");

        return;
    }

    if (containsInvalidPersistenceCharacter(
            studentAccountEdit->text()) ||
        containsInvalidPersistenceCharacter(
            studentNameEdit->text()) ||
        containsInvalidPersistenceCharacter(
            studentPasswordEdit->text()) ||
        containsInvalidPersistenceCharacter(
            studentClassEdit->text()) ||
        containsInvalidPersistenceCharacter(
            studentMajorEdit->text()))
    {
        QMessageBox::information(
            this,
            "提示",
            "账号、姓名、密码、班级和专业中不能包含字符 |。");

        return;
    }

    if (dataManager->findStudent(
            newAccountId) != nullptr)
    {
        QMessageBox::warning(
            this,
            "创建失败",
            "该账号已经被学生使用。");

        return;
    }

    if (dataManager->findAdministrator(
            newAccountId) != nullptr)
    {
        QMessageBox::warning(
            this,
            "创建失败",
            "该账号已经被管理员使用。");

        return;
    }

    QMessageBox::StandardButton result =
        QMessageBox::question(
            this,
            "确认创建",
            "确定创建学生账号吗？\n\n"
            "账号：" +
                QString::fromStdString(
                    newAccountId) +
                "\n姓名：" +
                QString::fromStdString(name) +
                "\n班级：" +
                QString::fromStdString(
                    className) +
                "\n专业：" +
                QString::fromStdString(major),
            QMessageBox::Yes |
                QMessageBox::No,
            QMessageBox::No);

    if (result != QMessageBox::Yes)
    {
        return;
    }

    Student student(
        newAccountId,
        name,
        password,
        className,
        major);

    dataManager->addStudent(student);
    dataManager->saveStudents();

    refreshHomePage();
    refreshStatisticsPage();

    QMessageBox::information(
        this,
        "创建成功",
        "学生账号创建成功。\n账号：" +
            QString::fromStdString(newAccountId));

    studentAccountEdit->clear();
    studentNameEdit->clear();
    studentPasswordEdit->clear();
    studentClassEdit->clear();
    studentMajorEdit->clear();
    studentAccountEdit->setFocus();
}

void AdministratorMainWindow::buildCreateAdministratorPage()
{
    createAdministratorPage =
        new QWidget;

    QVBoxLayout *mainLayout =
        new QVBoxLayout(
            createAdministratorPage);

    mainLayout->setContentsMargins(
        28,
        22,
        28,
        22);

    mainLayout->setSpacing(18);

    QLabel *titleLabel =
        new QLabel("创建管理员");

    QFont titleFont =
        titleLabel->font();

    titleFont.setPointSize(20);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);

    QLabel *descriptionLabel =
        new QLabel(
            "创建新的管理员登录账号");

    descriptionLabel->setStyleSheet(
        "color: #888888;"
        "font-size: 14px;"
        "background: transparent;");

    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(descriptionLabel);

    QFrame *formCard = new QFrame;

    formCard->setStyleSheet(
        "QFrame {"
        "background-color: white;"
        "border: 1px solid #eeeeee;"
        "border-radius: 18px;"
        "}");

    QVBoxLayout *cardLayout =
        new QVBoxLayout(formCard);

    cardLayout->setContentsMargins(
        28,
        26,
        28,
        26);

    cardLayout->setSpacing(18);

    QFormLayout *formLayout =
        new QFormLayout;

    formLayout->setHorizontalSpacing(30);
    formLayout->setVerticalSpacing(16);

    administratorAccountEdit = new QLineEdit;
    administratorNameEdit = new QLineEdit;
    administratorPasswordEdit = new QLineEdit;

    administratorAccountEdit->setPlaceholderText(
        "例如：admin002");

    administratorNameEdit->setPlaceholderText(
        "请输入管理员姓名");

    administratorPasswordEdit->setPlaceholderText(
        "请输入初始密码");

    administratorPasswordEdit->setEchoMode(
        QLineEdit::Password);

    QString editStyle =
        "QLineEdit {"
        "background-color: white;"
        "border: 1px solid #dddddd;"
        "border-radius: 10px;"
        "padding: 8px 12px;"
        "font-size: 14px;"
        "}"
        "QLineEdit:focus {"
        "border: 1px solid #b91f35;"
        "}";

    administratorAccountEdit->setStyleSheet(editStyle);
    administratorNameEdit->setStyleSheet(editStyle);
    administratorPasswordEdit->setStyleSheet(editStyle);

    administratorAccountEdit->setMinimumHeight(40);
    administratorNameEdit->setMinimumHeight(40);
    administratorPasswordEdit->setMinimumHeight(40);

    formLayout->addRow(
        "管理员账号：",
        administratorAccountEdit);

    formLayout->addRow(
        "管理员姓名：",
        administratorNameEdit);

    formLayout->addRow(
        "初始密码：",
        administratorPasswordEdit);

    cardLayout->addLayout(formLayout);

    QHBoxLayout *buttonLayout =
        new QHBoxLayout;

    buttonLayout->addStretch();

    QPushButton *createButton =
        new QPushButton("创建管理员");

    createButton->setMinimumSize(
        120,
        42);

    createButton->setCursor(
        Qt::PointingHandCursor);

    createButton->setStyleSheet(
        "QPushButton {"
        "background-color: #b91f35;"
        "color: white;"
        "border: none;"
        "border-radius: 10px;"
        "font-size: 14px;"
        "font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "background-color: #9f192d;"
        "}");

    buttonLayout->addWidget(createButton);
    cardLayout->addLayout(buttonLayout);
    mainLayout->addWidget(formCard);
    mainLayout->addStretch();

    connect(
        createButton,
        &QPushButton::clicked,
        this,
        &AdministratorMainWindow::createAdministrator);
}

void AdministratorMainWindow::createAdministrator()
{
    if (dataManager == nullptr)
    {
        return;
    }

    std::string newAccountId =
        administratorAccountEdit->text()
            .trimmed()
            .toStdString();

    std::string name =
        administratorNameEdit->text()
            .trimmed()
            .toStdString();

    std::string password =
        administratorPasswordEdit->text()
            .toStdString();

    if (newAccountId.empty() ||
        name.empty() ||
        password.empty())
    {
        QMessageBox::information(
            this,
            "提示",
            "请完整填写管理员信息。");

        return;
    }

    if (containsInvalidPersistenceCharacter(
            administratorAccountEdit->text()) ||
        containsInvalidPersistenceCharacter(
            administratorNameEdit->text()) ||
        containsInvalidPersistenceCharacter(
            administratorPasswordEdit->text()))
    {
        QMessageBox::information(
            this,
            "提示",
            "账号、姓名和密码中不能包含字符 |。");

        return;
    }

    if (dataManager->findStudent(
            newAccountId) != nullptr)
    {
        QMessageBox::warning(
            this,
            "创建失败",
            "该账号已经被学生使用。");

        return;
    }

    if (dataManager->findAdministrator(
            newAccountId) != nullptr)
    {
        QMessageBox::warning(
            this,
            "创建失败",
            "该管理员账号已经存在。");

        return;
    }

    QMessageBox::StandardButton result =
        QMessageBox::question(
            this,
            "确认创建",
            "确定创建管理员账号吗？\n\n"
            "账号：" +
                QString::fromStdString(
                    newAccountId) +
                "\n姓名：" +
                QString::fromStdString(name),
            QMessageBox::Yes |
                QMessageBox::No,
            QMessageBox::No);

    if (result != QMessageBox::Yes)
    {
        return;
    }

    Administrator administrator(
        newAccountId,
        name,
        password);

    dataManager->addAdministrator(administrator);
    dataManager->saveAdministrators();

    QMessageBox::information(
        this,
        "创建成功",
        "管理员账号创建成功。\n账号：" +
            QString::fromStdString(newAccountId));

    administratorAccountEdit->clear();
    administratorNameEdit->clear();
    administratorPasswordEdit->clear();
    administratorAccountEdit->setFocus();
}

void AdministratorMainWindow::buildProfilePage()
{
    profilePage =
        new QWidget;

    QVBoxLayout *mainLayout =
        new QVBoxLayout(profilePage);

    mainLayout->setContentsMargins(
        28,
        22,
        28,
        22);

    mainLayout->setSpacing(18);

    QLabel *titleLabel =
        new QLabel("个人信息");

    QFont titleFont =
        titleLabel->font();

    titleFont.setPointSize(20);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);

    QLabel *description =
        new QLabel(
            "查看管理员账号信息和账号安全设置");

    description->setStyleSheet(
        "color:#888888;"
        "background:transparent;");

    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(description);

    QFrame *infoCard = new QFrame;

    infoCard->setStyleSheet(
        "QFrame{"
        "background:white;"
        "border:1px solid #eeeeee;"
        "border-radius:18px;"
        "}");

    QVBoxLayout *cardLayout =
        new QVBoxLayout(infoCard);

    cardLayout->setContentsMargins(
        25,
        25,
        25,
        25);

    QHBoxLayout *header =
        new QHBoxLayout;

    QLabel *avatar = new QLabel("管");

    avatar->setFixedSize(70, 70);
    avatar->setAlignment(Qt::AlignCenter);
    avatar->setStyleSheet(
        "QLabel{"
        "background:#fff0f2;"
        "color:#b91f35;"
        "border-radius:35px;"
        "font-size:28px;"
        "font-weight:bold;"
        "}");

    QVBoxLayout *nameLayout =
        new QVBoxLayout;

    profileNameLabel = new QLabel;

    QFont nameFont =
        profileNameLabel->font();

    nameFont.setPointSize(18);
    nameFont.setBold(true);
    profileNameLabel->setFont(nameFont);

    profileAccountLabel = new QLabel;

    profileAccountLabel->setStyleSheet(
        "color:#888888;");

    nameLayout->addWidget(profileNameLabel);
    nameLayout->addWidget(profileAccountLabel);

    header->addWidget(avatar);
    header->addSpacing(15);
    header->addLayout(nameLayout);
    header->addStretch();
    cardLayout->addLayout(header);

    QFrame *line = new QFrame;
    line->setFrameShape(QFrame::HLine);
    cardLayout->addWidget(line);

    QLabel *info = new QLabel;
    info->setObjectName("adminInfoLabel");
    info->setStyleSheet(
        "color:#444444;"
        "font-size:15px;");
    cardLayout->addWidget(info);
    mainLayout->addWidget(infoCard);

    QFrame *securityCard = new QFrame;

    securityCard->setStyleSheet(
        "QFrame{"
        "background:white;"
        "border:1px solid #eeeeee;"
        "border-radius:18px;"
        "}");

    QHBoxLayout *securityLayout =
        new QHBoxLayout(securityCard);

    QLabel *securityText =
        new QLabel(
            "账号安全\n建议定期修改登录密码");

    securityText->setStyleSheet(
        "color:#555555;");

    QPushButton *passwordButton =
        new QPushButton("修改密码");

    passwordButton->setMinimumSize(110, 40);
    passwordButton->setStyleSheet(
        "QPushButton{"
        "background:#b91f35;"
        "color:white;"
        "border:none;"
        "border-radius:10px;"
        "font-weight:bold;"
        "}");

    securityLayout->addWidget(securityText);
    securityLayout->addStretch();
    securityLayout->addWidget(passwordButton);
    mainLayout->addWidget(securityCard);
    mainLayout->addStretch();

    connect(
        passwordButton,
        &QPushButton::clicked,
        this,
        &AdministratorMainWindow::changePassword);

    refreshProfilePage();
}

void AdministratorMainWindow::refreshProfilePage()
{
    if (dataManager == nullptr)
    {
        return;
    }

    Administrator *administrator =
        dataManager->findAdministrator(accountId);

    if (administrator == nullptr)
    {
        return;
    }

    profileNameLabel->setText(
        QString::fromStdString(
            administrator->getName()));

    profileAccountLabel->setText(
        "账号：" +
        QString::fromStdString(
            administrator->getAccountId()));

    QLabel *info =
        profilePage->findChild<QLabel *>(
            "adminInfoLabel");

    if (info != nullptr)
    {
        info->setText(
            "管理员账号：" +
            QString::fromStdString(
                administrator->getAccountId()) +
            "\n\n姓名：" +
            QString::fromStdString(
                administrator->getName()));
    }
}

void AdministratorMainWindow::changePassword()
{
    if (dataManager == nullptr)
    {
        return;
    }

    Administrator *administrator =
        dataManager->findAdministrator(accountId);

    if (administrator == nullptr)
    {
        return;
    }

    bool ok = false;

    QString oldPassword =
        QInputDialog::getText(
            this,
            "修改密码",
            "当前密码：",
            QLineEdit::Password,
            "",
            &ok);

    if (!ok)
    {
        return;
    }

    if (!administrator->checkPassword(
            oldPassword.toStdString()))
    {
        QMessageBox::warning(
            this,
            "错误",
            "当前密码错误。");

        return;
    }

    QString newPassword =
        QInputDialog::getText(
            this,
            "修改密码",
            "新密码：",
            QLineEdit::Password,
            "",
            &ok);

    if (!ok)
    {
        return;
    }

    if (newPassword.isEmpty())
    {
        QMessageBox::information(
            this,
            "提示",
            "密码不能为空。");

        return;
    }

    administrator->setPassword(
        newPassword.toStdString());

    dataManager->saveAdministrators();

    QMessageBox::information(
        this,
        "成功",
        "密码修改成功。");
}