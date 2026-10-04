#include "student_main_window.h"
#include "style_helper.h"

#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QFont>
#include <QTextEdit>
#include <QComboBox>
#include <QDateEdit>
#include <QDate>
#include <QAbstractItemView>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QInputDialog>
#include <QMessageBox>
#include <QScrollArea>
#include <QFrame>

#include "data_manager.h"
#include "diary_post.h"
#include "student.h"
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

StudentMainWindow::StudentMainWindow(
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
      recordsPage(nullptr),
      recordsTable(nullptr), categoryFilter(nullptr),
      startDateEdit(nullptr),
      endDateEdit(nullptr),
      submitPage(nullptr),
      submitCategoryCombo(nullptr),
      submitDateEdit(nullptr),
      submitDurationSpin(nullptr),
      submitPlaceEdit(nullptr),
      submitWitnessEdit(nullptr),
      submitDescriptionEdit(nullptr),
      scorePage(nullptr),
      totalScoreLabel(nullptr),
      monthlyScoreLabel(nullptr),
      semesterScoreLabel(nullptr),
      monthDateEdit(nullptr),
      semesterStartEdit(nullptr),
      semesterEndEdit(nullptr),
      rankingPage(nullptr),
      rankingTable(nullptr),
      badgePage(nullptr),
      laborBadgeLabel(nullptr),
      environmentBadgeLabel(nullptr),
      mutualAidBadgeLabel(nullptr),
      laborProgressLabel(nullptr),
      environmentProgressLabel(nullptr),
      mutualAidProgressLabel(nullptr),
      diaryPage(nullptr),
      diaryWallPage(nullptr),
      diaryRecordCombo(nullptr),
      diaryMessageEdit(nullptr),
      diaryScrollArea(nullptr),
      diaryContainer(nullptr),
      diaryFeedLayout(nullptr),
      profilePage(nullptr),
      profileAccountLabel(nullptr),
      profileNameLabel(nullptr),
      profileClassLabel(nullptr),
      profileMajorLabel(nullptr)
{
    setWindowTitle("校园雷锋日记 - 学生端");

    resize(1000, 650);

    buildInterface();
}

void StudentMainWindow::buildInterface()
{
    setStyleSheet(
        StyleHelper::pageBackground());

    QVBoxLayout *mainLayout =
        new QVBoxLayout(this);

    // ===== 顶部区域 =====

    QHBoxLayout *topLayout =
        new QHBoxLayout;

    QLabel *systemTitle =
        new QLabel("校园雷锋日记");

    QString studentName = "未知学生";

    if (dataManager != nullptr)
    {
        Student *student =
            dataManager->findStudent(accountId);

        if (student != nullptr)
        {
            studentName =
                QString::fromStdString(
                    student->getName());
        }
    }

    welcomeLabel =
        new QLabel(
            "当前用户：" +
            studentName +
            "（" +
            QString::fromStdString(accountId) +
            "）");

    QPushButton *logoutButton =
        new QPushButton("退出登录");

    topLayout->addWidget(systemTitle);

    topLayout->addStretch();

    topLayout->addWidget(welcomeLabel);
    topLayout->addWidget(logoutButton);

    mainLayout->addLayout(topLayout);

    // ===== 主体区域 =====

    QHBoxLayout *bodyLayout =
        new QHBoxLayout;

    navigationList =
        new QListWidget;

    navigationList->setFixedWidth(180);

    navigationList->setStyleSheet(
        StyleHelper::navigation());

    navigationList->addItem("个人主页");
    navigationList->addItem("我的志愿记录");
    navigationList->addItem("提交志愿");
    navigationList->addItem("我的积分");
    navigationList->addItem("排行榜");
    navigationList->addItem("我的徽章");
    navigationList->addItem("日记墙");
    navigationList->addItem("个人信息");

    contentStack =
        new QStackedWidget;

    buildHomePage();
    buildRecordsPage();
    buildSubmitPage();
    buildScorePage();
    buildRankingPage();
    buildBadgePage();
    buildDiaryPage();
    buildDiaryWallPage();
    buildProfilePage();

    contentStack->addWidget(homePage);
    contentStack->addWidget(recordsPage);
    contentStack->addWidget(submitPage);
    contentStack->addWidget(scorePage);
    contentStack->addWidget(rankingPage);
    contentStack->addWidget(badgePage);
    contentStack->addWidget(diaryPage);
    contentStack->addWidget(diaryWallPage);
    contentStack->addWidget(profilePage);

    bodyLayout->addWidget(navigationList);
    bodyLayout->addWidget(contentStack);

    mainLayout->addLayout(bodyLayout);

    // ===== 信号连接 =====

    connect(
        logoutButton,
        &QPushButton::clicked,
        this,
        &StudentMainWindow::logoutRequested);

    connect(
        navigationList,
        &QListWidget::currentRowChanged,
        this,
        &StudentMainWindow::handleNavigationChanged);

    navigationList->setCurrentRow(0);
}

void StudentMainWindow::buildHomePage()
{
    homePage =
        new QWidget;

    QVBoxLayout *layout =
        new QVBoxLayout(homePage);

    QLabel *titleLabel =
        new QLabel("学生主页");

    titleLabel->setStyleSheet(
        StyleHelper::title());

    QLabel *descriptionLabel =
        new QLabel(
            "欢迎使用校园雷锋日记系统。\n\n"
            "请通过左侧菜单选择功能。");

    descriptionLabel->setStyleSheet(
        StyleHelper::subtitle());

    layout->addWidget(titleLabel);
    layout->addWidget(descriptionLabel);

    layout->addStretch();
}

void StudentMainWindow::buildRecordsPage()
{
    recordsPage = new QWidget;

    QVBoxLayout *layout =
        new QVBoxLayout(recordsPage);

    QHBoxLayout *titleLayout =
        new QHBoxLayout;

    QLabel *titleLabel =
        new QLabel("我的志愿记录");

    titleLabel->setStyleSheet(
        StyleHelper::title());

    QPushButton *refreshButton =
        new QPushButton("刷新");

    refreshButton->setStyleSheet(
        StyleHelper::secondaryButton());

    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();
    titleLayout->addWidget(refreshButton);

    layout->addLayout(titleLayout);

    // ===== 查询条件 =====

    QHBoxLayout *filterLayout =
        new QHBoxLayout;

    categoryFilter =
        new QComboBox;

    categoryFilter->addItem("全部类别", "");

    categoryFilter->addItem(
        "劳动服务",
        "C01");

    categoryFilter->addItem(
        "环保服务",
        "C02");

    categoryFilter->addItem(
        "互助服务",
        "C03");

    categoryFilter->setStyleSheet(
        StyleHelper::input());

    startDateEdit =
        new QDateEdit;

    endDateEdit =
        new QDateEdit;

    startDateEdit->setCalendarPopup(true);
    endDateEdit->setCalendarPopup(true);

    startDateEdit->setDisplayFormat(
        "yyyy/MM/dd");

    endDateEdit->setDisplayFormat(
        "yyyy/MM/dd");

    startDateEdit->setDate(
        QDate(2000, 1, 1));

    endDateEdit->setDate(
        QDate(2100, 12, 31));

    QPushButton *searchButton =
        new QPushButton("查询");

    searchButton->setStyleSheet(
        StyleHelper::secondaryButton());

    QPushButton *clearButton =
        new QPushButton("重置");

    clearButton->setStyleSheet(
        StyleHelper::secondaryButton());

    filterLayout->addWidget(
        new QLabel("类别："));

    filterLayout->addWidget(
        categoryFilter);

    filterLayout->addWidget(
        new QLabel("开始日期："));

    filterLayout->addWidget(
        startDateEdit);

    filterLayout->addWidget(
        new QLabel("结束日期："));

    filterLayout->addWidget(
        endDateEdit);

    filterLayout->addWidget(
        searchButton);

    filterLayout->addWidget(
        clearButton);

    layout->addLayout(filterLayout);

    QHBoxLayout *actionLayout =
        new QHBoxLayout;

    QPushButton *modifyButton =
        new QPushButton("修改选中记录");

    modifyButton->setStyleSheet(
        StyleHelper::secondaryButton());

    QPushButton *deleteButton =
        new QPushButton("删除选中记录");

    deleteButton->setStyleSheet(
        StyleHelper::secondaryButton());

    actionLayout->addWidget(modifyButton);
    actionLayout->addWidget(deleteButton);
    actionLayout->addStretch();

    layout->addLayout(actionLayout);

    // ===== 表格 =====

    recordsTable =
        new QTableWidget;

    recordsTable->setColumnCount(6);

    recordsTable->setHorizontalHeaderLabels(
        {"记录编号",
         "志愿类别",
         "服务日期",
         "服务时长",
         "状态",
         "积分"});

    recordsTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers);

    recordsTable->setSelectionBehavior(
        QAbstractItemView::SelectRows);

    recordsTable->setSelectionMode(
        QAbstractItemView::SingleSelection);

    recordsTable->verticalHeader()
        ->setVisible(false);

    recordsTable->horizontalHeader()
        ->setSectionResizeMode(
            QHeaderView::Stretch);

    recordsTable->setStyleSheet(
        StyleHelper::table());

    layout->addWidget(recordsTable);

    connect(
        refreshButton,
        &QPushButton::clicked,
        this,
        &StudentMainWindow::refreshMyRecords);

    connect(
        searchButton,
        &QPushButton::clicked,
        this,
        &StudentMainWindow::applyRecordFilter);

    connect(
        clearButton,
        &QPushButton::clicked,
        this,
        &StudentMainWindow::clearRecordFilter);

    connect(
        modifyButton,
        &QPushButton::clicked,
        this,
        &StudentMainWindow::modifySelectedRecord);

    connect(
        deleteButton,
        &QPushButton::clicked,
        this,
        &StudentMainWindow::deleteSelectedRecord);

    refreshMyRecords();
}

void StudentMainWindow::handleNavigationChanged(
    int row)
{
    if (row == 0)
    {
        contentStack->setCurrentWidget(homePage);

        return;
    }

    if (row == 1)
    {
        refreshMyRecords();

        contentStack->setCurrentWidget(recordsPage);

        return;
    }

    if (row == 2)
    {
        contentStack->setCurrentWidget(submitPage);

        return;
    }

    if (row == 3)
    {
        refreshScorePage();
        contentStack->setCurrentWidget(scorePage);

        return;
    }

    if (row == 4)
    {
        refreshRankingPage();
        contentStack->setCurrentWidget(rankingPage);

        return;
    }

    if (row == 5)
    {
        refreshBadgePage();
        contentStack->setCurrentWidget(badgePage);

        return;
    }

    if (row == 6)
    {
        refreshDiaryPublishOptions();
        contentStack->setCurrentWidget(
            diaryPage);

        return;
    }

    if (row == 7)
    {
        refreshProfilePage();
        contentStack->setCurrentWidget(
            profilePage);

        return;
    }
}

void StudentMainWindow::buildSubmitPage()
{
    submitPage = new QWidget;

    QVBoxLayout *mainLayout =
        new QVBoxLayout(submitPage);

    QLabel *titleLabel =
        new QLabel("提交志愿记录");

    titleLabel->setStyleSheet(
        StyleHelper::title());

    mainLayout->addWidget(titleLabel);

    QFormLayout *formLayout =
        new QFormLayout;

    submitCategoryCombo =
        new QComboBox;

    submitCategoryCombo->addItem(
        "劳动服务",
        "C01");

    submitCategoryCombo->addItem(
        "环保服务",
        "C02");

    submitCategoryCombo->addItem(
        "互助服务",
        "C03");

    submitCategoryCombo->setStyleSheet(
        StyleHelper::input());

    submitDateEdit =
        new QDateEdit;

    submitDateEdit->setCalendarPopup(true);

    submitDateEdit->setDisplayFormat(
        "yyyy/MM/dd");

    submitDateEdit->setDate(
        QDate::currentDate());

    submitDurationSpin =
        new QDoubleSpinBox;

    submitDurationSpin->setRange(
        0.1,
        10000.0);

    submitDurationSpin->setDecimals(1);
    submitDurationSpin->setSingleStep(0.5);
    submitDurationSpin->setSuffix(" 小时");

    submitPlaceEdit =
        new QLineEdit;

    submitPlaceEdit->setPlaceholderText(
        "请输入服务地点");

    submitPlaceEdit->setStyleSheet(
        StyleHelper::input());

    submitWitnessEdit =
        new QLineEdit;

    submitWitnessEdit->setPlaceholderText(
        "请输入证明人");

    submitWitnessEdit->setStyleSheet(
        StyleHelper::input());

    submitDescriptionEdit =
        new QTextEdit;

    submitDescriptionEdit->setPlaceholderText(
        "请输入志愿服务内容");

    submitDescriptionEdit->setFixedHeight(120);

    submitDescriptionEdit->setStyleSheet(
        StyleHelper::input());

    formLayout->addRow(
        "志愿类别：",
        submitCategoryCombo);

    formLayout->addRow(
        "服务日期：",
        submitDateEdit);

    formLayout->addRow(
        "服务时长：",
        submitDurationSpin);

    formLayout->addRow(
        "服务地点：",
        submitPlaceEdit);

    formLayout->addRow(
        "证明人：",
        submitWitnessEdit);

    formLayout->addRow(
        "服务描述：",
        submitDescriptionEdit);

    mainLayout->addLayout(formLayout);

    QPushButton *submitButton =
        new QPushButton("提交志愿记录");

    submitButton->setStyleSheet(
        StyleHelper::primaryButton());

    mainLayout->addWidget(submitButton);
    mainLayout->addStretch();

    connect(
        submitButton,
        &QPushButton::clicked,
        this,
        &StudentMainWindow::submitVolunteerRecord);
}

void StudentMainWindow::buildScorePage()
{
    scorePage = new QWidget;

    QVBoxLayout *mainLayout =
        new QVBoxLayout(scorePage);

    QLabel *titleLabel =
        new QLabel("我的积分");

    titleLabel->setStyleSheet(
        StyleHelper::title());

    mainLayout->addWidget(titleLabel);

    QLabel *totalTitle =
        new QLabel("总积分");

    totalScoreLabel =
        new QLabel("0.00");

    QPushButton *refreshButton =
        new QPushButton("刷新积分");

    refreshButton->setStyleSheet(
        StyleHelper::secondaryButton());

    mainLayout->addWidget(totalTitle);
    mainLayout->addWidget(totalScoreLabel);
    mainLayout->addWidget(refreshButton);

    QLabel *monthTitle =
        new QLabel("月度积分");

    monthDateEdit =
        new QDateEdit;

    monthDateEdit->setCalendarPopup(true);

    monthDateEdit->setDisplayFormat(
        "yyyy/MM");

    monthDateEdit->setDate(
        QDate::currentDate());

    QPushButton *monthButton =
        new QPushButton("查询月度积分");

    monthButton->setStyleSheet(
        StyleHelper::secondaryButton());

    monthlyScoreLabel =
        new QLabel("请选择月份");

    QFormLayout *monthLayout =
        new QFormLayout;

    monthLayout->addRow(
        "查询月份：",
        monthDateEdit);

    monthLayout->addRow(
        monthButton,
        monthlyScoreLabel);

    mainLayout->addWidget(monthTitle);
    mainLayout->addLayout(monthLayout);

    QLabel *semesterTitle =
        new QLabel("学期积分");

    semesterStartEdit =
        new QDateEdit;

    semesterEndEdit =
        new QDateEdit;

    semesterStartEdit->setCalendarPopup(true);
    semesterEndEdit->setCalendarPopup(true);

    semesterStartEdit->setDisplayFormat(
        "yyyy/MM/dd");

    semesterEndEdit->setDisplayFormat(
        "yyyy/MM/dd");

    semesterStartEdit->setDate(
        QDate(
            QDate::currentDate().year(),
            9,
            1));

    semesterEndEdit->setDate(
        QDate(
            QDate::currentDate().year() + 1,
            1,
            31));

    QPushButton *semesterButton =
        new QPushButton("查询学期积分");

    semesterButton->setStyleSheet(
        StyleHelper::secondaryButton());

    semesterScoreLabel =
        new QLabel("请选择学期时间范围");

    QFormLayout *semesterLayout =
        new QFormLayout;

    semesterLayout->addRow(
        "开始日期：",
        semesterStartEdit);

    semesterLayout->addRow(
        "结束日期：",
        semesterEndEdit);

    semesterLayout->addRow(
        semesterButton,
        semesterScoreLabel);

    mainLayout->addWidget(semesterTitle);
    mainLayout->addLayout(semesterLayout);

    mainLayout->addStretch();

    connect(
        refreshButton,
        &QPushButton::clicked,
        this,
        &StudentMainWindow::refreshScorePage);

    connect(
        monthButton,
        &QPushButton::clicked,
        this,
        &StudentMainWindow::calculateMonthlyScore);

    connect(
        semesterButton,
        &QPushButton::clicked,
        this,
        &StudentMainWindow::calculateSemesterScore);

    refreshScorePage();
}

void StudentMainWindow::buildRankingPage()
{
    rankingPage = new QWidget;

    QVBoxLayout *layout =
        new QVBoxLayout(rankingPage);

    QHBoxLayout *titleLayout =
        new QHBoxLayout;

    QLabel *titleLabel =
        new QLabel("积分排行榜");

    titleLabel->setStyleSheet(
        StyleHelper::title());

    QPushButton *refreshButton =
        new QPushButton("刷新排行榜");

    refreshButton->setStyleSheet(
        StyleHelper::secondaryButton());

    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();
    titleLayout->addWidget(refreshButton);

    layout->addLayout(titleLayout);

    rankingTable =
        new QTableWidget;

    rankingTable->setColumnCount(5);

    rankingTable->setHorizontalHeaderLabels(
        {"排名",
         "学生账号",
         "学生姓名",
         "总积分",
         "荣誉"});

    rankingTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers);

    rankingTable->setSelectionBehavior(
        QAbstractItemView::SelectRows);

    rankingTable->setSelectionMode(
        QAbstractItemView::SingleSelection);

    rankingTable->verticalHeader()
        ->setVisible(false);

    rankingTable->horizontalHeader()
        ->setSectionResizeMode(
            QHeaderView::Stretch);

    rankingTable->setStyleSheet(
        StyleHelper::table());

    layout->addWidget(rankingTable);

    connect(
        refreshButton,
        &QPushButton::clicked,
        this,
        &StudentMainWindow::refreshRankingPage);

    refreshRankingPage();
}

void StudentMainWindow::buildBadgePage()
{
    badgePage = new QWidget;

    QVBoxLayout *mainLayout =
        new QVBoxLayout(badgePage);

    QHBoxLayout *titleLayout =
        new QHBoxLayout;

    QLabel *titleLabel =
        new QLabel("我的专项徽章");

    titleLabel->setStyleSheet(
        StyleHelper::title());

    QPushButton *refreshButton =
        new QPushButton("刷新徽章");

    titleLayout->addWidget(titleLabel);
    titleLayout->addStretch();
    titleLayout->addWidget(refreshButton);

    mainLayout->addLayout(titleLayout);

    QLabel *ruleLabel =
        new QLabel(
            "徽章根据已审核通过的志愿服务时长动态计算：\n"
            "10 小时：铜级  30 小时：银级  60 小时：金级");

    mainLayout->addWidget(ruleLabel);

    QLabel *laborTitle =
        new QLabel("劳动服务");

    laborBadgeLabel =
        new QLabel("暂无徽章");

    laborProgressLabel =
        new QLabel;

    mainLayout->addWidget(laborTitle);
    mainLayout->addWidget(laborBadgeLabel);
    mainLayout->addWidget(laborProgressLabel);

    QLabel *environmentTitle =
        new QLabel("环保服务");

    environmentBadgeLabel =
        new QLabel("暂无徽章");

    environmentProgressLabel =
        new QLabel;

    mainLayout->addWidget(environmentTitle);
    mainLayout->addWidget(environmentBadgeLabel);
    mainLayout->addWidget(environmentProgressLabel);

    QLabel *mutualAidTitle =
        new QLabel("互助服务");

    mutualAidBadgeLabel =
        new QLabel("暂无徽章");

    mutualAidProgressLabel =
        new QLabel;

    mainLayout->addWidget(mutualAidTitle);
    mainLayout->addWidget(mutualAidBadgeLabel);
    mainLayout->addWidget(mutualAidProgressLabel);

    mainLayout->addStretch();

    connect(
        refreshButton,
        &QPushButton::clicked,
        this,
        &StudentMainWindow::refreshBadgePage);

    refreshBadgePage();
}

void StudentMainWindow::buildDiaryPage()
{
    diaryPage = new QWidget;

    diaryPage->setStyleSheet(
        "QWidget {"
        "background-color: #f7f7f7;"
        "}");

    QVBoxLayout *mainLayout =
        new QVBoxLayout(diaryPage);

    mainLayout->setContentsMargins(
        18,
        15,
        18,
        15);

    mainLayout->setSpacing(16);

    QLabel *titleLabel =
        new QLabel("发布志愿日记");

    QFont titleFont =
        titleLabel->font();

    titleFont.setPointSize(20);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);

    titleLabel->setStyleSheet(
        "QLabel {"
        "color: #222222;"
        "background: transparent;"
        "border: none;"
        "}");

    mainLayout->addWidget(titleLabel);

    QLabel *tipLabel =
        new QLabel(
            "分享你的志愿服务经历，"
            "记录每一次有意义的行动。");

    tipLabel->setStyleSheet(
        "QLabel {"
        "color: #888888;"
        "font-size: 14px;"
        "background: transparent;"
        "border: none;"
        "}");

    mainLayout->addWidget(tipLabel);

    QFrame *publishFrame =
        new QFrame;

    publishFrame->setStyleSheet(
        "QFrame {"
        "background-color: white;"
        "border: 1px solid #eeeeee;"
        "border-radius: 18px;"
        "}");

    QVBoxLayout *publishLayout =
        new QVBoxLayout(publishFrame);

    publishLayout->setContentsMargins(24, 22, 24, 22);
    publishLayout->setSpacing(14);

    QLabel *publishTitle =
        new QLabel("分享我的志愿日记");

    QFont publishFont =
        publishTitle->font();

    publishFont.setPointSize(14);
    publishFont.setBold(true);
    publishTitle->setFont(publishFont);

    publishTitle->setStyleSheet(
        "QLabel {"
        "color: #222222;"
        "background: transparent;"
        "border: none;"
        "}");

    QLabel *recordTip =
        new QLabel("选择已审核通过的志愿记录");

    recordTip->setStyleSheet(
        "QLabel {"
        "color: #666666;"
        "background: transparent;"
        "border: none;"
        "}");

    diaryRecordCombo =
        new QComboBox;

    diaryRecordCombo->setMinimumHeight(42);

    diaryRecordCombo->setStyleSheet(
        "QComboBox {"
        "background-color: white;"
        "border: 1px solid #dddddd;"
        "border-radius: 10px;"
        "padding: 6px 12px;"
        "color: #333333;"
        "font-size: 14px;"
        "}"
        "QComboBox:hover {"
        "border: 1px solid #bbbbbb;"
        "}"
        "QComboBox::drop-down {"
        "border: none;"
        "width: 30px;"
        "}");

    diaryMessageEdit =
        new QTextEdit;

    diaryMessageEdit->setPlaceholderText(
        "记录这次志愿服务中的故事和感受……");

    diaryMessageEdit->setMinimumHeight(180);

    diaryMessageEdit->setStyleSheet(
        "QTextEdit {"
        "background-color: white;"
        "border: 1px solid #dddddd;"
        "border-radius: 14px;"
        "padding: 10px;"
        "color: #222222;"
        "font-size: 14px;"
        "}"
        "QTextEdit:focus {"
        "border: 1px solid #ff8a9d;"
        "}");

    QPushButton *publishButton =
        new QPushButton("发布日记");

    publishButton->setMinimumSize(120, 42);

    publishButton->setCursor(
        Qt::PointingHandCursor);

    publishButton->setStyleSheet(
        "QPushButton {"
        "background-color: #ff2442;"
        "color: white;"
        "border: none;"
        "border-radius: 10px;"
        "padding: 8px 22px;"
        "font-size: 14px;"
        "font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "background-color: #ed1f3b;"
        "}"
        "QPushButton:pressed {"
        "background-color: #d91c35;"
        "}");

    QLabel *messageLabel =
        new QLabel("日记内容");

    messageLabel->setStyleSheet(
        "QLabel {"
        "color: #444444;"
        "font-weight: bold;"
        "background: transparent;"
        "border: none;"
        "}");

    publishLayout->addWidget(publishTitle);
    publishLayout->addWidget(recordTip);
    publishLayout->addWidget(diaryRecordCombo);
    publishLayout->addWidget(messageLabel);
    publishLayout->addWidget(diaryMessageEdit);

    QHBoxLayout *publishButtonLayout =
        new QHBoxLayout;

    publishButtonLayout->addStretch();
    publishButtonLayout->addWidget(publishButton);
    publishLayout->addLayout(publishButtonLayout);

    mainLayout->addWidget(publishFrame);
    mainLayout->addStretch();

    QPushButton *enterWallButton =
        new QPushButton("进入日记墙  →");

    enterWallButton->setMinimumHeight(46);
    enterWallButton->setCursor(Qt::PointingHandCursor);
    enterWallButton->setStyleSheet(
        "QPushButton {"
        "background-color: white;"
        "color: #ff2442;"
        "border: 1px solid #ff9aaa;"
        "border-radius: 12px;"
        "font-size: 15px;"
        "font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "background-color: #fff0f3;"
        "}");

    mainLayout->addWidget(enterWallButton);

    connect(
        publishButton,
        &QPushButton::clicked,
        this,
        &StudentMainWindow::publishDiary);

    connect(
        enterWallButton,
        &QPushButton::clicked,
        this,
        [this]()
        {
            refreshDiaryWall();
            contentStack->setCurrentWidget(
                diaryWallPage);
        });

    refreshDiaryPublishOptions();
}

void StudentMainWindow::buildDiaryWallPage()
{
    diaryWallPage = new QWidget;

    diaryWallPage->setStyleSheet(
        "QWidget {"
        "background-color: #f7f7f7;"
        "}");

    QVBoxLayout *mainLayout =
        new QVBoxLayout(diaryWallPage);

    mainLayout->setContentsMargins(20, 16, 20, 16);
    mainLayout->setSpacing(14);

    QHBoxLayout *topLayout =
        new QHBoxLayout;

    QPushButton *backButton =
        new QPushButton("← 返回发布");

    backButton->setMinimumSize(110, 36);
    backButton->setCursor(Qt::PointingHandCursor);
    backButton->setStyleSheet(
        "QPushButton {"
        "background-color: white;"
        "border: 1px solid #dddddd;"
        "border-radius: 9px;"
        "color: #444444;"
        "font-size: 14px;"
        "}"
        "QPushButton:hover {"
        "background-color: #eeeeee;"
        "}");

    QLabel *titleLabel =
        new QLabel("校园日记墙");

    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(20);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);

    QPushButton *refreshButton =
        new QPushButton("刷新");

    refreshButton->setMinimumSize(80, 36);
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

    topLayout->addWidget(backButton);
    topLayout->addStretch();
    topLayout->addWidget(titleLabel);
    topLayout->addStretch();
    topLayout->addWidget(refreshButton);
    mainLayout->addLayout(topLayout);

    diaryScrollArea = new QScrollArea;
    diaryScrollArea->setWidgetResizable(true);
    diaryScrollArea->setFrameShape(QFrame::NoFrame);
    diaryScrollArea->setStyleSheet(
        "QScrollArea {"
        "background: transparent;"
        "border: none;"
        "}");

    diaryContainer = new QWidget;
    diaryContainer->setStyleSheet(
        "background: transparent;");

    diaryFeedLayout =
        new QVBoxLayout(diaryContainer);
    diaryFeedLayout->setContentsMargins(0, 5, 0, 5);
    diaryFeedLayout->setSpacing(16);
    diaryFeedLayout->setAlignment(Qt::AlignTop);

    diaryScrollArea->setWidget(diaryContainer);
    mainLayout->addWidget(diaryScrollArea);

    connect(
        backButton,
        &QPushButton::clicked,
        this,
        [this]()
        {
            refreshDiaryPublishOptions();
            contentStack->setCurrentWidget(diaryPage);
        });

    connect(
        refreshButton,
        &QPushButton::clicked,
        this,
        &StudentMainWindow::refreshDiaryWall);
}

void StudentMainWindow::buildProfilePage()
{
    profilePage = new QWidget;

    profilePage->setStyleSheet(
        StyleHelper::pageBackground());

    QVBoxLayout *mainLayout =
        new QVBoxLayout(profilePage);

    mainLayout->setContentsMargins(28, 24, 28, 24);
    mainLayout->setSpacing(18);

    QLabel *titleLabel =
        new QLabel("个人信息");

    titleLabel->setStyleSheet(
        StyleHelper::title());

    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(20);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);

    QLabel *tipLabel =
        new QLabel("查看个人基本资料和账号信息");

    tipLabel->setStyleSheet(
        StyleHelper::subtitle());

    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(tipLabel);

    QFrame *profileCard = new QFrame;
    profileCard->setStyleSheet(
        StyleHelper::card());

    QVBoxLayout *cardLayout =
        new QVBoxLayout(profileCard);

    cardLayout->setContentsMargins(26, 24, 26, 24);
    cardLayout->setSpacing(18);

    QHBoxLayout *userLayout = new QHBoxLayout;

    QLabel *avatarLabel = new QLabel("志");
    avatarLabel->setFixedSize(68, 68);
    avatarLabel->setAlignment(Qt::AlignCenter);
    avatarLabel->setStyleSheet(
        "QLabel {"
        "background-color: #ffedf0;"
        "color: #ff2442;"
        "border: none;"
        "border-radius: 34px;"
        "font-size: 26px;"
        "font-weight: bold;"
        "}");

    QVBoxLayout *nameLayout = new QVBoxLayout;

    profileNameLabel = new QLabel;
    QFont nameFont = profileNameLabel->font();
    nameFont.setPointSize(17);
    nameFont.setBold(true);
    profileNameLabel->setFont(nameFont);

    profileAccountLabel = new QLabel;
    profileAccountLabel->setStyleSheet(
        "QLabel {"
        "color: #888888;"
        "font-size: 13px;"
        "background: transparent;"
        "border: none;"
        "}");

    nameLayout->addWidget(profileNameLabel);
    nameLayout->addWidget(profileAccountLabel);
    userLayout->addWidget(avatarLabel);
    userLayout->addSpacing(14);
    userLayout->addLayout(nameLayout);
    userLayout->addStretch();
    cardLayout->addLayout(userLayout);

    QFrame *line = new QFrame;
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("color: #eeeeee;");
    cardLayout->addWidget(line);

    QFormLayout *infoLayout = new QFormLayout;
    infoLayout->setHorizontalSpacing(30);
    infoLayout->setVerticalSpacing(18);

    QLabel *accountTitle = new QLabel("学生账号");
    QLabel *nameTitle = new QLabel("姓名");
    QLabel *classTitle = new QLabel("班级");
    QLabel *majorTitle = new QLabel("专业");

    QString titleStyle =
        "QLabel {"
        "color: #888888;"
        "font-size: 14px;"
        "background: transparent;"
        "border: none;"
        "}";

    accountTitle->setStyleSheet(titleStyle);
    nameTitle->setStyleSheet(titleStyle);
    classTitle->setStyleSheet(titleStyle);
    majorTitle->setStyleSheet(titleStyle);

    QLabel *accountDetailLabel = new QLabel;
    QLabel *nameDetailLabel = new QLabel;
    profileClassLabel = new QLabel;
    profileMajorLabel = new QLabel;

    QString valueStyle =
        "QLabel {"
        "color: #222222;"
        "font-size: 15px;"
        "font-weight: bold;"
        "background: transparent;"
        "border: none;"
        "}";

    accountDetailLabel->setStyleSheet(valueStyle);
    nameDetailLabel->setStyleSheet(valueStyle);
    profileClassLabel->setStyleSheet(valueStyle);
    profileMajorLabel->setStyleSheet(valueStyle);

    accountDetailLabel->setObjectName(
        "profileAccountDetail");
    nameDetailLabel->setObjectName(
        "profileNameDetail");

    infoLayout->addRow(accountTitle, accountDetailLabel);
    infoLayout->addRow(nameTitle, nameDetailLabel);
    infoLayout->addRow(classTitle, profileClassLabel);
    infoLayout->addRow(majorTitle, profileMajorLabel);
    cardLayout->addLayout(infoLayout);
    mainLayout->addWidget(profileCard);

    QFrame *securityCard = new QFrame;
    securityCard->setStyleSheet(
        StyleHelper::card());

    QHBoxLayout *securityLayout =
        new QHBoxLayout(securityCard);
    securityLayout->setContentsMargins(24, 20, 24, 20);

    QVBoxLayout *securityTextLayout = new QVBoxLayout;
    QLabel *securityTitle = new QLabel("账号安全");
    QFont securityFont = securityTitle->font();
    securityFont.setBold(true);
    securityFont.setPointSize(14);
    securityTitle->setFont(securityFont);

    QLabel *securityDescription =
        new QLabel("建议定期修改登录密码，保护账号安全。");
    securityDescription->setStyleSheet(
        "QLabel {"
        "color: #888888;"
        "font-size: 13px;"
        "background: transparent;"
        "border: none;"
        "}");

    securityTextLayout->addWidget(securityTitle);
    securityTextLayout->addWidget(securityDescription);

    QPushButton *passwordButton =
        new QPushButton("修改密码");
    passwordButton->setMinimumSize(110, 40);
    passwordButton->setCursor(Qt::PointingHandCursor);
    passwordButton->setStyleSheet(
        StyleHelper::primaryButton());

    securityLayout->addLayout(securityTextLayout);
    securityLayout->addStretch();
    securityLayout->addWidget(passwordButton);
    mainLayout->addWidget(securityCard);
    mainLayout->addStretch();

    connect(
        passwordButton,
        &QPushButton::clicked,
        this,
        &StudentMainWindow::changePassword);

    refreshProfilePage();
}

void StudentMainWindow::refreshMyRecords()
{
    if (recordsTable == nullptr ||
        dataManager == nullptr)
    {
        return;
    }

    recordsTable->setRowCount(0);

    std::string selectedCategory;

    if (categoryFilter != nullptr)
    {
        selectedCategory =
            categoryFilter
                ->currentData()
                .toString()
                .toStdString();
    }

    QString startDate = "0000/00/00";
    QString endDate = "9999/99/99";

    if (startDateEdit != nullptr)
    {
        startDate =
            startDateEdit
                ->date()
                .toString("yyyy/MM/dd");
    }

    if (endDateEdit != nullptr)
    {
        endDate =
            endDateEdit
                ->date()
                .toString("yyyy/MM/dd");
    }

    for (const VolunteerRecord &recordValue :
         dataManager->getRecords())
    {
        const VolunteerRecord *record = &recordValue;

        if (record == nullptr)
        {
            continue;
        }

        if (record->getStudentId() != accountId)
        {
            continue;
        }

        if (!selectedCategory.empty() &&
            record->getCategoryId() != selectedCategory)
        {
            continue;
        }

        QString recordDate =
            QString::fromStdString(
                record->getDate());

        if (recordDate < startDate ||
            recordDate > endDate)
        {
            continue;
        }

        int row =
            recordsTable->rowCount();

        recordsTable->insertRow(row);

        recordsTable->setItem(
            row,
            0,
            new QTableWidgetItem(
                QString::fromStdString(
                    record->getRecordId())));

        recordsTable->setItem(
            row,
            1,
            new QTableWidgetItem(
                categoryName(
                    record->getCategoryId())));

        recordsTable->setItem(
            row,
            2,
            new QTableWidgetItem(
                recordDate));

        recordsTable->setItem(
            row,
            3,
            new QTableWidgetItem(
                QString::number(
                    record->getDuration(),
                    'f',
                    1) +
                " 小时"));

        QString status;

        if (record->getStatus() ==
            RecordStatus::Pending)
        {
            status = "待审核";
        }
        else if (record->getStatus() ==
                 RecordStatus::Approved)
        {
            status = "已通过";
        }
        else
        {
            status = "已驳回";
        }

        recordsTable->setItem(
            row,
            4,
            new QTableWidgetItem(status));

        recordsTable->setItem(
            row,
            5,
            new QTableWidgetItem(
                QString::number(
                    record->getScore(),
                    'f',
                    2)));
    }
}

QString StudentMainWindow::statusText(
    int statusValue) const
{
    if (
        statusValue ==
        static_cast<int>(
            RecordStatus::Pending))
    {
        return "待审核";
    }

    if (
        statusValue ==
        static_cast<int>(
            RecordStatus::Approved))
    {
        return "已通过";
    }

    return "已驳回";
}
QString StudentMainWindow::categoryName(
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

void StudentMainWindow::applyRecordFilter()
{
    if (startDateEdit->date() >
        endDateEdit->date())
    {
        return;
    }

    refreshMyRecords();
}

void StudentMainWindow::clearRecordFilter()
{
    categoryFilter->setCurrentIndex(0);

    startDateEdit->setDate(
        QDate(2000, 1, 1));

    endDateEdit->setDate(
        QDate(2100, 12, 31));

    refreshMyRecords();
}
void StudentMainWindow::modifySelectedRecord()
{
    if (recordsTable == nullptr ||
        dataManager == nullptr)
    {
        return;
    }

    int row =
        recordsTable->currentRow();

    if (row < 0)
    {
        QMessageBox::information(
            this,
            "提示",
            "请先选择一条志愿记录。");

        return;
    }

    QTableWidgetItem *idItem =
        recordsTable->item(row, 0);

    if (idItem == nullptr)
    {
        return;
    }

    std::string recordId =
        idItem->text().toStdString();

    VolunteerRecord *record =
        dataManager->findRecord(recordId);

    if (record == nullptr)
    {
        QMessageBox::warning(
            this,
            "错误",
            "未找到该志愿记录。");

        return;
    }

    if (record->getStudentId() != accountId)
    {
        QMessageBox::warning(
            this,
            "错误",
            "不能修改其他学生的记录。");

        return;
    }

    if (record->getStatus() ==
        RecordStatus::Approved)
    {
        QMessageBox::information(
            this,
            "提示",
            "已审核通过的记录不能修改。");

        return;
    }

    // =========================
    // 志愿类别
    // =========================

    QStringList categoryNames;

    categoryNames
        << "劳动服务"
        << "环保服务"
        << "互助服务";

    int currentCategoryIndex = 0;

    if (record->getCategoryId() == "C02")
    {
        currentCategoryIndex = 1;
    }
    else if (record->getCategoryId() == "C03")
    {
        currentCategoryIndex = 2;
    }

    bool ok = false;

    QString selectedCategory =
        QInputDialog::getItem(
            this,
            "修改志愿记录",
            "志愿类别：",
            categoryNames,
            currentCategoryIndex,
            false,
            &ok);

    if (!ok)
    {
        return;
    }

    std::string newCategoryId;

    if (selectedCategory == "劳动服务")
    {
        newCategoryId = "C01";
    }
    else if (selectedCategory == "环保服务")
    {
        newCategoryId = "C02";
    }
    else
    {
        newCategoryId = "C03";
    }

    // =========================
    // 日期
    // =========================

    QString newDate =
        QInputDialog::getText(
            this,
            "修改志愿记录",
            "服务日期（YYYY/MM/DD）：",
            QLineEdit::Normal,
            QString::fromStdString(
                record->getDate()),
            &ok);

    if (!ok)
    {
        return;
    }

    // =========================
    // 时长
    // =========================

    double newDuration =
        QInputDialog::getDouble(
            this,
            "修改志愿记录",
            "服务时长（小时）：",
            record->getDuration(),
            0.1,
            10000.0,
            1,
            &ok);

    if (!ok)
    {
        return;
    }

    // =========================
    // 地点
    // =========================

    QString newPlace =
        QInputDialog::getText(
            this,
            "修改志愿记录",
            "服务地点：",
            QLineEdit::Normal,
            QString::fromStdString(
                record->getPlace()),
            &ok);

    if (!ok)
    {
        return;
    }

    // =========================
    // 证明人
    // =========================

    QString newWitness =
        QInputDialog::getText(
            this,
            "修改志愿记录",
            "证明人：",
            QLineEdit::Normal,
            QString::fromStdString(
                record->getWitness()),
            &ok);

    if (!ok)
    {
        return;
    }

    // =========================
    // 描述
    // =========================

    QString newDescription =
        QInputDialog::getText(
            this,
            "修改志愿记录",
            "志愿描述：",
            QLineEdit::Normal,
            QString::fromStdString(
                record->getDescription()),
            &ok);

    if (!ok)
    {
        return;
    }

    if (containsInvalidPersistenceCharacter(newPlace) ||
        containsInvalidPersistenceCharacter(newWitness) ||
        containsInvalidPersistenceCharacter(newDescription))
    {
        QMessageBox::information(
            this,
            "提示",
            "服务地点、证明人和志愿描述中不能包含字符 | 或换行。");

        return;
    }

    // 所有输入都成功以后再真正修改，
    // 避免中途取消导致只修改了一半。

    record->setCategoryId(
        newCategoryId);

    record->setDate(
        newDate.toStdString());

    record->setDuration(
        newDuration);

    record->setPlace(
        newPlace.toStdString());

    record->setWitness(
        newWitness.toStdString());

    record->setDescription(
        newDescription.toStdString());

    // 驳回记录修改以后重新进入待审核状态。
    if (record->getStatus() ==
        RecordStatus::Rejected)
    {
        record->resubmit();
    }

    dataManager->saveRecords();

    refreshMyRecords();

    QMessageBox::information(
        this,
        "修改成功",
        "志愿记录已保存。");
}
void StudentMainWindow::deleteSelectedRecord()
{
    if (recordsTable == nullptr ||
        dataManager == nullptr)
    {
        return;
    }

    int row =
        recordsTable->currentRow();

    if (row < 0)
    {
        QMessageBox::information(
            this,
            "提示",
            "请先选择一条志愿记录。");

        return;
    }

    QTableWidgetItem *idItem =
        recordsTable->item(row, 0);

    if (idItem == nullptr)
    {
        return;
    }

    std::string recordId =
        idItem->text().toStdString();

    VolunteerRecord *record =
        dataManager->findRecord(recordId);

    if (record == nullptr)
    {
        QMessageBox::warning(
            this,
            "错误",
            "未找到该志愿记录。");

        return;
    }

    if (record->getStudentId() != accountId)
    {
        QMessageBox::warning(
            this,
            "错误",
            "不能删除其他学生的记录。");

        return;
    }

    if (record->getStatus() ==
        RecordStatus::Approved)
    {
        QMessageBox::information(
            this,
            "提示",
            "已审核通过的记录不能删除。");

        return;
    }

    QMessageBox::StandardButton result =
        QMessageBox::question(
            this,
            "确认删除",
            "确定要删除记录 " +
                QString::fromStdString(recordId) +
                " 吗？",
            QMessageBox::Yes |
                QMessageBox::No,
            QMessageBox::No);

    if (result != QMessageBox::Yes)
    {
        return;
    }

    bool deleted =
        dataManager->deleteRecord(
            recordId);

    if (!deleted)
    {
        QMessageBox::warning(
            this,
            "删除失败",
            "删除志愿记录失败。");

        return;
    }

    dataManager->saveRecords();

    refreshMyRecords();

    QMessageBox::information(
        this,
        "删除成功",
        "志愿记录已删除。");
}

void StudentMainWindow::submitVolunteerRecord()
{
    if (dataManager == nullptr)
    {
        return;
    }

    std::string categoryId =
        submitCategoryCombo
            ->currentData()
            .toString()
            .toStdString();

    std::string date =
        submitDateEdit
            ->date()
            .toString("yyyy/MM/dd")
            .toStdString();

    double duration =
        submitDurationSpin->value();

    std::string place =
        submitPlaceEdit
            ->text()
            .trimmed()
            .toStdString();

    std::string witness =
        submitWitnessEdit
            ->text()
            .trimmed()
            .toStdString();

    std::string description =
        submitDescriptionEdit
            ->toPlainText()
            .trimmed()
            .toStdString();

    if (place.empty())
    {
        QMessageBox::information(
            this,
            "提示",
            "请输入服务地点。");

        return;
    }

    if (witness.empty())
    {
        QMessageBox::information(
            this,
            "提示",
            "请输入证明人。");

        return;
    }

    if (description.empty())
    {
        QMessageBox::information(
            this,
            "提示",
            "请输入志愿服务描述。");

        return;
    }

    if (containsInvalidPersistenceCharacter(
            QString::fromStdString(place)) ||
        containsInvalidPersistenceCharacter(
            QString::fromStdString(witness)) ||
        containsInvalidPersistenceCharacter(
            QString::fromStdString(description)))
    {
        QMessageBox::information(
            this,
            "提示",
            "服务地点、证明人和服务描述中不能包含字符 | 或换行。");

        return;
    }

    std::string recordId =
        dataManager->generateRecordId();

    VolunteerRecord record(
        recordId,
        accountId,
        categoryId,
        date,
        duration,
        place,
        witness,
        description,
        RecordStatus::Pending,
        0.0);

    dataManager->addRecord(record);

    dataManager->saveRecords();

    QMessageBox::information(
        this,
        "提交成功",
        "志愿记录提交成功。\n"
        "记录编号：" +
            QString::fromStdString(recordId) +
            "\n当前状态：待审核");

    submitCategoryCombo->setCurrentIndex(0);

    submitDateEdit->setDate(
        QDate::currentDate());

    submitDurationSpin->setValue(0.1);

    submitPlaceEdit->clear();
    submitWitnessEdit->clear();
    submitDescriptionEdit->clear();

    navigationList->setCurrentRow(1);

    refreshMyRecords();
}

void StudentMainWindow::refreshScorePage()
{
    if (dataManager == nullptr ||
        totalScoreLabel == nullptr)
    {
        return;
    }

    double score =
        dataManager->calculateStudentScore(
            accountId);

    totalScoreLabel->setText(
        QString::number(
            score,
            'f',
            2));
}

void StudentMainWindow::calculateMonthlyScore()
{
    if (dataManager == nullptr ||
        monthDateEdit == nullptr)
    {
        return;
    }

    QDate selectedDate =
        monthDateEdit->date();

    QDate firstDay(
        selectedDate.year(),
        selectedDate.month(),
        1);

    QDate lastDay(
        selectedDate.year(),
        selectedDate.month(),
        selectedDate.daysInMonth());

    std::string startDate =
        firstDay
            .toString("yyyy/MM/dd")
            .toStdString();

    std::string endDate =
        lastDay
            .toString("yyyy/MM/dd")
            .toStdString();

    double score =
        dataManager
            ->calculateStudentScoreByDateRange(
                accountId,
                startDate,
                endDate);

    monthlyScoreLabel->setText(
        QString::number(score, 'f', 2));
}

void StudentMainWindow::calculateSemesterScore()
{
    if (dataManager == nullptr)
    {
        return;
    }

    QDate start =
        semesterStartEdit->date();

    QDate end =
        semesterEndEdit->date();

    if (start > end)
    {
        QMessageBox::information(
            this,
            "提示",
            "开始日期不能晚于结束日期。");

        return;
    }

    std::string startDate =
        start
            .toString("yyyy/MM/dd")
            .toStdString();

    std::string endDate =
        end
            .toString("yyyy/MM/dd")
            .toStdString();

    double score =
        dataManager
            ->calculateStudentScoreByDateRange(
                accountId,
                startDate,
                endDate);

    semesterScoreLabel->setText(
        QString::number(score, 'f', 2));
}

void StudentMainWindow::refreshRankingPage()
{
    if (dataManager == nullptr ||
        rankingTable == nullptr)
    {
        return;
    }

    rankingTable->setRowCount(0);

    std::vector<RankingItem> ranking =
        dataManager->generateRanking();

    for (size_t i = 0;
         i < ranking.size();
         ++i)
    {
        const RankingItem &item =
            ranking[i];

        int row =
            rankingTable->rowCount();

        rankingTable->insertRow(row);

        rankingTable->setItem(
            row,
            0,
            new QTableWidgetItem(
                QString::number(
                    static_cast<int>(i + 1))));

        rankingTable->setItem(
            row,
            1,
            new QTableWidgetItem(
                QString::fromStdString(
                    item.studentId)));

        rankingTable->setItem(
            row,
            2,
            new QTableWidgetItem(
                QString::fromStdString(
                    item.studentName)));

        rankingTable->setItem(
            row,
            3,
            new QTableWidgetItem(
                QString::number(
                    item.score,
                    'f',
                    2)));

        QString honor = "-";

        if (i < 3)
        {
            honor = "雷锋之星";
        }

        rankingTable->setItem(
            row,
            4,
            new QTableWidgetItem(honor));

        if (item.studentId == accountId)
        {
            for (int column = 0;
                 column < rankingTable->columnCount();
                 ++column)
            {
                QTableWidgetItem *tableItem =
                    rankingTable->item(
                        row,
                        column);

                if (tableItem != nullptr)
                {
                    QFont font =
                        tableItem->font();

                    font.setBold(true);

                    tableItem->setFont(font);
                }
            }
        }
    }
}

void StudentMainWindow::refreshDiaryPublishOptions()
{
    if (dataManager == nullptr ||
        diaryRecordCombo == nullptr)
    {
        return;
    }

    diaryRecordCombo->clear();

    for (const VolunteerRecord &record :
         dataManager->getRecords())
    {
        if (record.getStudentId() != accountId ||
            record.getStatus() != RecordStatus::Approved)
        {
            continue;
        }

        if (dataManager->findDiaryByRecordId(
                record.getRecordId()) != nullptr)
        {
            continue;
        }

        QString text =
            QString::fromStdString(
                record.getRecordId()) +
            " · " +
            categoryName(record.getCategoryId()) +
            " · " +
            QString::fromStdString(record.getDate());

        diaryRecordCombo->addItem(
            text,
            QString::fromStdString(
                record.getRecordId()));
    }

    if (diaryRecordCombo->count() == 0)
    {
        diaryRecordCombo->addItem(
            "暂无可用于发布日记的志愿记录",
            "");
    }
}

void StudentMainWindow::refreshDiaryWall()
{
    if (dataManager == nullptr ||
        diaryFeedLayout == nullptr)
    {
        return;
    }

    while (QLayoutItem *item =
               diaryFeedLayout->takeAt(0))
    {
        if (item->widget() != nullptr)
        {
            delete item->widget();
        }

        delete item;
    }

    const std::vector<DiaryPost> &diaries =
        dataManager->getDiaries().getItems();

    if (diaries.empty())
    {
        QLabel *emptyLabel =
            new QLabel(
                "暂时还没有志愿日记，来发布第一篇吧～");

        emptyLabel->setAlignment(Qt::AlignCenter);
        diaryFeedLayout->addWidget(emptyLabel);

        return;
    }

    for (const DiaryPost &diary : diaries)
    {
        Student *author =
            dataManager->findStudent(diary.getStudentId());

        VolunteerRecord *record =
            dataManager->findRecord(diary.getRecordId());

        QFrame *card =
            new QFrame;

        card->setFrameShape(QFrame::StyledPanel);
        card->setStyleSheet(
            "QFrame {"
            "background-color: white;"
            "border: 1px solid #eeeeee;"
            "border-radius: 18px;"
            "}");

        QVBoxLayout *cardLayout =
            new QVBoxLayout(card);

        cardLayout->setContentsMargins(
            22,
            20,
            22,
            18);

        cardLayout->setSpacing(10);

        QHBoxLayout *authorLayout =
            new QHBoxLayout;

        QLabel *avatarLabel =
            new QLabel("志");

        avatarLabel->setFixedSize(
            38,
            38);

        avatarLabel->setAlignment(
            Qt::AlignCenter);

        avatarLabel->setStyleSheet(
            "QLabel {"
            "background-color: #ffedf0;"
            "color: #ff2442;"
            "border-radius: 19px;"
            "font-weight: bold;"
            "}");

        QString authorName =
            "未知学生";

        if (author != nullptr)
        {
            authorName =
                QString::fromStdString(
                    author->getName());
        }

        QLabel *authorLabel =
            new QLabel(
                authorName +
                "\n" +
                QString::fromStdString(
                    diary.getStudentId()));

        QFont authorFont =
            authorLabel->font();

        authorFont.setBold(true);

        authorLabel->setFont(
            authorFont);

        authorLabel->setMinimumWidth(130);

        authorLayout->addWidget(
            avatarLabel);

        authorLayout->addWidget(
            authorLabel);

        authorLayout->addStretch();

        cardLayout->addLayout(
            authorLayout);

        if (record != nullptr)
        {
            QLabel *recordLabel =
                new QLabel(
                    categoryName(record->getCategoryId()) +
                    " · " +
                    QString::fromStdString(record->getDate()) +
                    " · " +
                    QString::number(record->getDuration(), 'f', 1) +
                    " 小时");

            recordLabel->setStyleSheet(
                "QLabel {"
                "color: #888888;"
                "font-size: 13px;"
                "background: transparent;"
                "border: none;"
                "}");
            cardLayout->addWidget(recordLabel);

            QLabel *placeLabel =
                new QLabel(
                    "地点：" +
                    QString::fromStdString(
                        record->getPlace()));

            placeLabel->setStyleSheet(
                "QLabel {"
                "color: #999999;"
                "font-size: 13px;"
                "background: transparent;"
                "border: none;"
                "}");
            cardLayout->addWidget(placeLabel);
        }

        QLabel *messageLabel =
            new QLabel(
                QString::fromStdString(
                    diary.getMessage()));

        messageLabel->setWordWrap(true);
        messageLabel->setTextInteractionFlags(
            Qt::TextSelectableByMouse);
        messageLabel->setStyleSheet(
            "QLabel {"
            "font-size: 16px;"
            "color: #222222;"
            "background: transparent;"
            "border: none;"
            "padding-top: 10px;"
            "padding-bottom: 10px;"
            "}");
        cardLayout->addWidget(messageLabel);

        QHBoxLayout *bottomLayout =
            new QHBoxLayout;

        bottomLayout->addStretch();

        bool alreadyLiked =
            diary.hasLiked(accountId);

        QPushButton *likeButton =
            new QPushButton;

        likeButton->setFixedSize(34, 34);
        likeButton->setCursor(Qt::PointingHandCursor);
        likeButton->setFlat(true);

        QLabel *likeCountLabel =
            new QLabel(
                QString::number(
                    diary.getLikeCount()));

        QFont likeFont =
            likeButton->font();

        likeFont.setPointSize(18);
        likeFont.setBold(true);

        likeButton->setFont(likeFont);

        if (alreadyLiked)
        {
            likeButton->setText("♥");
            likeButton->setStyleSheet(
                "QPushButton {"
                "color: #ff2442;"
                "border: none;"
                "background: transparent;"
                "}");

            likeCountLabel->setStyleSheet(
                "QLabel {"
                "color: #ff2442;"
                "font-size: 15px;"
                "font-weight: bold;"
                "}");
        }
        else
        {
            likeButton->setText("♡");
            likeButton->setStyleSheet(
                "QPushButton {"
                "color: #666666;"
                "border: none;"
                "background: transparent;"
                "}"
                "QPushButton:hover {"
                "color: #ff2442;"
                "}");

            likeCountLabel->setStyleSheet(
                "QLabel {"
                "color: #666666;"
                "font-size: 15px;"
                "}");
        }

        std::string diaryId =
            diary.getDiaryId();

        connect(
            likeButton,
            &QPushButton::clicked,
            this,
            [this, diaryId]()
            {
                if (dataManager == nullptr)
                {
                    return;
                }

                DiaryPost *selectedDiary =
                    dataManager->findDiary(
                        diaryId);

                if (selectedDiary == nullptr)
                {
                    QMessageBox::warning(
                        this,
                        "错误",
                        "没有找到该日记。");

                    return;
                }

                if (selectedDiary->hasLiked(
                        accountId))
                {
                    QMessageBox::information(
                        this,
                        "提示",
                        "你已经点赞过这篇日记了。");

                    return;
                }

                if (!selectedDiary->addLike(
                        accountId))
                {
                    QMessageBox::information(
                        this,
                        "提示",
                        "点赞失败或已经点赞。");

                    return;
                }

                dataManager->saveDiaries();
                refreshDiaryWall();
            });

        bottomLayout->addWidget(likeButton);
        bottomLayout->addSpacing(4);
        bottomLayout->addWidget(likeCountLabel);
        cardLayout->addLayout(bottomLayout);
        diaryFeedLayout->addWidget(card);
    }

    diaryFeedLayout->addStretch();
}

void StudentMainWindow::publishDiary()
{
    if (dataManager == nullptr ||
        diaryRecordCombo == nullptr ||
        diaryMessageEdit == nullptr)
    {
        return;
    }

    if (diaryRecordCombo->count() == 0 ||
        diaryRecordCombo
            ->currentData()
            .toString()
            .isEmpty())
    {
        QMessageBox::information(
            this,
            "提示",
            "目前没有可以发布日记的志愿记录。\n"
            "只有审核通过且尚未发布日记的记录可以使用。");

        return;
    }

    QString message =
        diaryMessageEdit->toPlainText().trimmed();

    if (message.isEmpty())
    {
        QMessageBox::information(
            this,
            "提示",
            "请输入日记内容。");

        return;
    }

    if (containsInvalidPersistenceCharacter(message))
    {
        QMessageBox::information(
            this,
            "提示",
            "日记内容中不能包含字符 | 或换行。");

        return;
    }

    std::string recordId =
        diaryRecordCombo->currentData()
            .toString()
            .toStdString();

    VolunteerRecord *record =
        dataManager->findRecord(recordId);

    if (record == nullptr)
    {
        QMessageBox::warning(
            this,
            "错误",
            "没有找到对应的志愿记录。");

        return;
    }

    if (record->getStudentId() != accountId ||
        record->getStatus() != RecordStatus::Approved)
    {
        QMessageBox::information(
            this,
            "提示",
            "只有本人审核通过的志愿记录才能发布日记。");

        return;
    }

    if (dataManager->findDiaryByRecordId(recordId) != nullptr)
    {
        QMessageBox::information(
            this,
            "提示",
            "这条志愿记录已经发布过日记。");

        refreshDiaryWall();
        return;
    }

    std::string diaryId =
        dataManager->generateDiaryId();

    DiaryPost diary(
        diaryId,
        accountId,
        recordId,
        message.toStdString(),
        0);

    dataManager->addDiary(diary);
    dataManager->saveDiaries();

    diaryMessageEdit->clear();

    QMessageBox::information(
        this,
        "发布成功",
        "志愿日记发布成功。");

    refreshDiaryPublishOptions();
}

void StudentMainWindow::refreshProfilePage()
{
    if (dataManager == nullptr ||
        profilePage == nullptr)
    {
        return;
    }

    Student *student =
        dataManager->findStudent(accountId);

    if (student == nullptr)
    {
        return;
    }

    QString studentName =
        QString::fromStdString(student->getName());

    QString studentAccount =
        QString::fromStdString(student->getAccountId());

    profileNameLabel->setText(studentName);
    profileAccountLabel->setText(
        "账号：" + studentAccount);
    profileClassLabel->setText(
        QString::fromStdString(
            student->getClassName()));
    profileMajorLabel->setText(
        QString::fromStdString(
            student->getMajor()));

    QLabel *accountDetailLabel =
        profilePage->findChild<QLabel *>(
            "profileAccountDetail");

    if (accountDetailLabel != nullptr)
    {
        accountDetailLabel->setText(studentAccount);
    }

    QLabel *nameDetailLabel =
        profilePage->findChild<QLabel *>(
            "profileNameDetail");

    if (nameDetailLabel != nullptr)
    {
        nameDetailLabel->setText(studentName);
    }
}

void StudentMainWindow::changePassword()
{
    if (dataManager == nullptr)
    {
        return;
    }

    Student *student =
        dataManager->findStudent(accountId);

    if (student == nullptr)
    {
        QMessageBox::warning(
            this,
            "错误",
            "没有找到当前学生账号。");
        return;
    }

    bool ok = false;

    QString oldPassword =
        QInputDialog::getText(
            this,
            "修改密码",
            "请输入当前密码：",
            QLineEdit::Password,
            "",
            &ok);

    if (!ok)
    {
        return;
    }

    if (!student->checkPassword(
            oldPassword.toStdString()))
    {
        QMessageBox::warning(
            this,
            "修改失败",
            "当前密码输入错误。");
        return;
    }

    QString newPassword =
        QInputDialog::getText(
            this,
            "修改密码",
            "请输入新密码：",
            QLineEdit::Password,
            "",
            &ok);

    if (!ok)
    {
        return;
    }

    if (newPassword.trimmed().isEmpty())
    {
        QMessageBox::information(
            this,
            "提示",
            "新密码不能为空。");
        return;
    }

    if (containsInvalidPersistenceCharacter(newPassword))
    {
        QMessageBox::information(
            this,
            "提示",
            "密码中不能包含字符 | 或换行。");
        return;
    }

    QString confirmPassword =
        QInputDialog::getText(
            this,
            "修改密码",
            "请再次输入新密码：",
            QLineEdit::Password,
            "",
            &ok);

    if (!ok)
    {
        return;
    }

    if (newPassword != confirmPassword)
    {
        QMessageBox::warning(
            this,
            "修改失败",
            "两次输入的新密码不一致。");
        return;
    }

    if (student->checkPassword(
            newPassword.toStdString()))
    {
        QMessageBox::information(
            this,
            "提示",
            "新密码不能与当前密码相同。");
        return;
    }

    student->setPassword(
        newPassword.toStdString());
    dataManager->saveStudents();

    QMessageBox::information(
        this,
        "修改成功",
        "登录密码已修改。");
}

QString StudentMainWindow::badgeText(
    const std::string &categoryId,
    double duration) const
{
    QString badgeName;

    if (categoryId == "C01")
    {
        badgeName = "劳动先锋";
    }
    else if (categoryId == "C02")
    {
        badgeName = "环保卫士";
    }
    else if (categoryId == "C03")
    {
        badgeName = "互助之星";
    }
    else
    {
        return "未知类别";
    }

    if (duration >= 60.0)
    {
        return "金级 · " + badgeName;
    }

    if (duration >= 30.0)
    {
        return "银级 · " + badgeName;
    }

    if (duration >= 10.0)
    {
        return "铜级 · " + badgeName;
    }

    return "暂无徽章";
}

void StudentMainWindow::refreshBadgePage()
{
    if (dataManager == nullptr)
    {
        return;
    }

    double laborDuration =
        dataManager
            ->calculateStudentDurationByCategory(
                accountId,
                "C01");

    double environmentDuration =
        dataManager
            ->calculateStudentDurationByCategory(
                accountId,
                "C02");

    double mutualAidDuration =
        dataManager
            ->calculateStudentDurationByCategory(
                accountId,
                "C03");

    laborBadgeLabel->setText(
        badgeText(
            "C01",
            laborDuration));

    environmentBadgeLabel->setText(
        badgeText(
            "C02",
            environmentDuration));

    mutualAidBadgeLabel->setText(
        badgeText(
            "C03",
            mutualAidDuration));

    laborProgressLabel->setText(
        "累计已通过服务时长：" +
        QString::number(
            laborDuration,
            'f',
            1) +
        " 小时");

    environmentProgressLabel->setText(
        "累计已通过服务时长：" +
        QString::number(
            environmentDuration,
            'f',
            1) +
        " 小时");

    mutualAidProgressLabel->setText(
        "累计已通过服务时长：" +
        QString::number(
            mutualAidDuration,
            'f',
            1) +
        " 小时");
}