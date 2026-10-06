#include "administrator_main_window.h"
#include "style_helper.h"

#include <QAbstractItemView>
#include <QColor>
#include <QComboBox>
#include <QFrame>
#include <QFont>
#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPainter>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSizePolicy>
#include <QStyle>
#include <QStyleOptionViewItem>
#include <QStyledItemDelegate>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include "administrator.h"
#include "data_manager.h"
#include "volunteer_record.h"
#include "volunteer_review_service.h"

namespace
{
    class AdminReviewNoCellFocusDelegate final :
        public QStyledItemDelegate
    {
    public:
        using QStyledItemDelegate::QStyledItemDelegate;

        void paint(
            QPainter *painter,
            const QStyleOptionViewItem &option,
            const QModelIndex &index) const override
        {
            QStyleOptionViewItem itemOption(option);
            itemOption.state &= ~QStyle::State_HasFocus;
            QStyledItemDelegate::paint(
                painter,
                itemOption,
                index);
        }
    };

    QColor reviewStatusColor(RecordStatus status)
    {
        switch (status)
        {
        case RecordStatus::Pending:
            return QColor("#B7791F");
        case RecordStatus::Approved:
            return QColor("#2F855A");
        case RecordStatus::Rejected:
            return QColor("#C2413A");
        }

        return QColor("#68717D");
    }

    QString reviewStatusProperty(RecordStatus status)
    {
        switch (status)
        {
        case RecordStatus::Pending:
            return QStringLiteral("pending");
        case RecordStatus::Approved:
            return QStringLiteral("approved");
        case RecordStatus::Rejected:
            return QStringLiteral("rejected");
        }

        return QStringLiteral("default");
    }

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
      reviewEmptyLabel(nullptr),
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
    homePage->setObjectName("administratorDashboard");
    homePage->setStyleSheet(
        StyleHelper::administratorDashboard());

    QVBoxLayout *mainLayout = new QVBoxLayout(homePage);
    mainLayout->setContentsMargins(22, 20, 22, 22);
    mainLayout->setSpacing(16);

    QHBoxLayout *headerLayout = new QHBoxLayout;
    headerLayout->setSpacing(16);
    QVBoxLayout *headingTextLayout = new QVBoxLayout;
    headingTextLayout->setSpacing(4);

    QLabel *titleLabel = new QLabel("管理员主页");
    titleLabel->setObjectName("adminDashboardTitle");
    QLabel *descriptionLabel = new QLabel(
        "查看校园志愿服务进展，快速进入审核与管理页面。");
    descriptionLabel->setObjectName("adminDashboardSubtitle");

    headingTextLayout->addWidget(titleLabel);
    headingTextLayout->addWidget(descriptionLabel);
    headerLayout->addLayout(headingTextLayout, 1);

    QLabel *identityLabel = new QLabel(
        welcomeLabel == nullptr
            ? QStringLiteral("管理员")
            : welcomeLabel->text());
    identityLabel->setObjectName("adminDashboardIdentity");
    identityLabel->setAlignment(
        Qt::AlignRight | Qt::AlignVCenter);
    headerLayout->addWidget(identityLabel, 0, Qt::AlignRight);
    mainLayout->addLayout(headerLayout);

    QHBoxLayout *cardLayout = new QHBoxLayout;
    cardLayout->setSpacing(12);
    const auto addStatCard = [&cardLayout](
                                 const QString &title,
                                 QLabel *&valueLabel)
    {
        QFrame *card = new QFrame;
        card->setObjectName("adminDashboardStatCard");
        card->setMinimumHeight(116);

        QVBoxLayout *layout = new QVBoxLayout(card);
        layout->setContentsMargins(16, 12, 16, 12);
        layout->setSpacing(6);

        QLabel *caption = new QLabel(title);
        caption->setObjectName("adminDashboardStatCaption");
        valueLabel = new QLabel("0");
        valueLabel->setObjectName("adminDashboardStatValue");

        layout->addStretch(1);
        layout->addWidget(caption);
        layout->addWidget(valueLabel);
        layout->addStretch(1);
        cardLayout->addWidget(card, 1);
    };

    addStatCard("学生数量", studentCountLabel);
    addStatCard("待审核记录", pendingCountLabel);
    addStatCard("已通过记录", approvedCountLabel);
    addStatCard("志愿总记录", recordCountLabel);
    mainLayout->addLayout(cardLayout);

    QFrame *quickSection = new QFrame;
    quickSection->setObjectName("adminDashboardQuickSection");
    QVBoxLayout *quickLayout = new QVBoxLayout(quickSection);
    quickLayout->setContentsMargins(16, 14, 16, 16);
    quickLayout->setSpacing(10);

    QLabel *quickTitle = new QLabel("快捷管理");
    quickTitle->setObjectName("adminDashboardSectionTitle");
    QLabel *quickSubtitle = new QLabel(
        "选择入口后将打开对应的现有管理页面。");
    quickSubtitle->setObjectName("adminDashboardSectionSubtitle");
    quickLayout->addWidget(quickTitle);
    quickLayout->addWidget(quickSubtitle);

    QHBoxLayout *actionsLayout = new QHBoxLayout;
    actionsLayout->setSpacing(8);
    QPushButton *reviewButton = new QPushButton("志愿审核");
    reviewButton->setObjectName("adminDashboardPrimaryAction");
    QPushButton *statisticsButton = new QPushButton("数据统计");
    statisticsButton->setObjectName("adminDashboardAction");
    QPushButton *createStudentButton = new QPushButton("创建学生");
    createStudentButton->setObjectName("adminDashboardAction");
    QPushButton *createAdministratorButton =
        new QPushButton("创建管理员");
    createAdministratorButton->setObjectName("adminDashboardAction");

    for (QPushButton *button :
         {reviewButton,
          statisticsButton,
          createStudentButton,
          createAdministratorButton})
    {
        button->setCursor(Qt::PointingHandCursor);
        button->setMinimumHeight(40);
        actionsLayout->addWidget(button, 1);
    }
    quickLayout->addLayout(actionsLayout);
    mainLayout->addWidget(quickSection);
    mainLayout->addStretch(1);

    connect(reviewButton, &QPushButton::clicked,
            this, [this]() { navigationList->setCurrentRow(1); });
    connect(statisticsButton, &QPushButton::clicked,
            this, [this]() { navigationList->setCurrentRow(2); });
    connect(createStudentButton, &QPushButton::clicked,
            this, [this]() { navigationList->setCurrentRow(3); });
    connect(createAdministratorButton, &QPushButton::clicked,
            this, [this]() { navigationList->setCurrentRow(4); });

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
    reviewPage = new QWidget;
    reviewPage->setObjectName("administratorReviewPage");
    reviewPage->setStyleSheet(
        StyleHelper::administratorReviewPage());

    QVBoxLayout *pageLayout =
        new QVBoxLayout(reviewPage);
    pageLayout->setContentsMargins(0, 0, 0, 0);

    QScrollArea *scrollArea =
        new QScrollArea(reviewPage);
    scrollArea->setObjectName("adminReviewScroll");
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(
        Qt::ScrollBarAlwaysOff);
    scrollArea->setFrameShape(QFrame::NoFrame);

    QWidget *pageContent = new QWidget;
    pageContent->setObjectName("adminReviewContent");

    QVBoxLayout *mainLayout = new QVBoxLayout(pageContent);
    mainLayout->setContentsMargins(22, 20, 22, 22);
    mainLayout->setSpacing(16);

    QVBoxLayout *headingLayout = new QVBoxLayout;
    headingLayout->setSpacing(4);

    QLabel *titleLabel = new QLabel("志愿审核");
    titleLabel->setObjectName("adminReviewTitle");

    QLabel *descriptionLabel = new QLabel(
        "查看学生提交的志愿记录及审核状态。");
    descriptionLabel->setObjectName("adminReviewSubtitle");

    headingLayout->addWidget(titleLabel);
    headingLayout->addWidget(descriptionLabel);
    mainLayout->addLayout(headingLayout);

    QFrame *filterCard = new QFrame;
    filterCard->setObjectName("adminReviewFilterCard");

    QHBoxLayout *filterLayout = new QHBoxLayout(filterCard);
    filterLayout->setContentsMargins(16, 14, 16, 14);
    filterLayout->setSpacing(12);

    QLabel *filterLabel = new QLabel("审核状态");
    filterLabel->setObjectName("adminReviewFieldLabel");

    reviewStatusFilter = new QComboBox;
    reviewStatusFilter->setObjectName(
        "adminReviewStatusFilter");
    reviewStatusFilter->addItem("全部", -1);
    reviewStatusFilter->addItem(
        "待审核",
        static_cast<int>(RecordStatus::Pending));
    reviewStatusFilter->addItem(
        "已通过",
        static_cast<int>(RecordStatus::Approved));
    reviewStatusFilter->addItem(
        "已驳回",
        static_cast<int>(RecordStatus::Rejected));
    reviewStatusFilter->setMinimumSize(170, 40);

    QPushButton *refreshButton = new QPushButton("刷新");
    refreshButton->setObjectName("adminReviewRefreshButton");
    refreshButton->setCursor(Qt::PointingHandCursor);
    refreshButton->setMinimumSize(92, 40);

    filterLayout->addWidget(filterLabel);
    filterLayout->addWidget(reviewStatusFilter);
    filterLayout->addStretch(1);
    filterLayout->addWidget(refreshButton);
    mainLayout->addWidget(filterCard);

    QFrame *tableCard = new QFrame;
    tableCard->setObjectName("adminReviewTableCard");

    QVBoxLayout *tableCardLayout = new QVBoxLayout(tableCard);
    tableCardLayout->setContentsMargins(16, 14, 16, 16);
    tableCardLayout->setSpacing(10);

    QLabel *tableTitle = new QLabel("审核记录");
    tableTitle->setObjectName("adminReviewSectionTitle");
    tableCardLayout->addWidget(tableTitle);

    reviewTable = new QTableWidget;
    reviewTable->setObjectName("adminReviewTable");
    reviewTable->setItemDelegate(
        new AdminReviewNoCellFocusDelegate(reviewTable));
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
    reviewTable->setAlternatingRowColors(true);
    reviewTable->setShowGrid(false);
    reviewTable->verticalHeader()->setVisible(false);
    reviewTable->verticalHeader()->setDefaultSectionSize(44);
    reviewTable->horizontalHeader()->setSectionResizeMode(
        QHeaderView::Stretch);
    reviewTable->horizontalHeader()->setDefaultAlignment(
        Qt::AlignLeft | Qt::AlignVCenter);
    reviewTable->setMinimumHeight(250);

    reviewEmptyLabel = new QLabel(
        "当前筛选条件下暂无志愿记录");
    reviewEmptyLabel->setObjectName("adminReviewEmptyState");
    reviewEmptyLabel->setAlignment(Qt::AlignCenter);
    reviewEmptyLabel->setMinimumHeight(150);
    reviewEmptyLabel->hide();

    tableCardLayout->addWidget(reviewTable, 1);
    tableCardLayout->addWidget(reviewEmptyLabel);
    mainLayout->addWidget(tableCard, 1);

    reviewDetailFrame = new QFrame;
    reviewDetailFrame->setObjectName("adminReviewDetailCard");

    QVBoxLayout *detailLayout =
        new QVBoxLayout(reviewDetailFrame);
    detailLayout->setContentsMargins(18, 16, 18, 16);
    detailLayout->setSpacing(12);

    QLabel *detailTitle = new QLabel("记录详情");
    detailTitle->setObjectName("adminReviewSectionTitle");
    detailLayout->addWidget(detailTitle);

    detailStudentLabel = new QLabel;
    detailCategoryLabel = new QLabel;
    detailDateLabel = new QLabel;
    detailDurationLabel = new QLabel;
    detailPlaceLabel = new QLabel;
    detailWitnessLabel = new QLabel;
    detailDescriptionLabel = new QLabel;
    detailStatusLabel = new QLabel;
    detailScoreLabel = new QLabel;

    for (QLabel *fieldLabel :
         {detailStudentLabel,
          detailCategoryLabel,
          detailDateLabel,
          detailDurationLabel,
          detailPlaceLabel,
          detailWitnessLabel,
          detailDescriptionLabel,
          detailStatusLabel,
          detailScoreLabel})
    {
        fieldLabel->setObjectName("adminReviewField");
    }

    detailStatusLabel->setObjectName(
        "adminReviewDetailStatus");
    detailDescriptionLabel->setWordWrap(true);
    detailDescriptionLabel->setTextInteractionFlags(
        Qt::TextSelectableByMouse);

    QGridLayout *detailGrid = new QGridLayout;
    detailGrid->setHorizontalSpacing(20);
    detailGrid->setVerticalSpacing(10);
    detailGrid->setColumnStretch(0, 1);
    detailGrid->setColumnStretch(1, 1);
    detailGrid->addWidget(detailStudentLabel, 0, 0);
    detailGrid->addWidget(detailCategoryLabel, 0, 1);
    detailGrid->addWidget(detailDateLabel, 1, 0);
    detailGrid->addWidget(detailDurationLabel, 1, 1);
    detailGrid->addWidget(detailPlaceLabel, 2, 0);
    detailGrid->addWidget(detailWitnessLabel, 2, 1);
    detailGrid->addWidget(
        detailDescriptionLabel,
        3,
        0,
        1,
        2);
    detailGrid->addWidget(detailStatusLabel, 4, 0);
    detailGrid->addWidget(detailScoreLabel, 4, 1);
    detailLayout->addLayout(detailGrid);

    QHBoxLayout *actionLayout = new QHBoxLayout;
    actionLayout->addStretch(1);

    approveButton = new QPushButton("通过");
    approveButton->setObjectName("adminReviewApproveButton");
    approveButton->setCursor(Qt::PointingHandCursor);
    approveButton->setMinimumSize(100, 40);

    rejectButton = new QPushButton("驳回");
    rejectButton->setObjectName("adminReviewRejectButton");
    rejectButton->setCursor(Qt::PointingHandCursor);
    rejectButton->setMinimumSize(100, 40);

    actionLayout->addWidget(approveButton);
    actionLayout->addWidget(rejectButton);
    detailLayout->addLayout(actionLayout);
    mainLayout->addWidget(reviewDetailFrame);

    reviewDetailFrame->hide();

    scrollArea->setWidget(pageContent);
    pageLayout->addWidget(scrollArea);

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

        QString recordId = QString::fromStdString(
            record.getRecordId());
        QTableWidgetItem *recordIdItem =
            new QTableWidgetItem(recordId);
        recordIdItem->setData(
            Qt::UserRole,
            recordId);
        recordIdItem->setTextAlignment(
            Qt::AlignCenter);
        reviewTable->setItem(row, 0, recordIdItem);

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

        reviewTable->item(row, 3)->setTextAlignment(
            Qt::AlignCenter);

        reviewTable->setItem(
            row,
            4,
            new QTableWidgetItem(
                QString::number(
                    record.getDuration(),
                    'f',
                    1) +
                " 小时"));
        reviewTable->item(row, 4)->setTextAlignment(
            Qt::AlignCenter);

        QTableWidgetItem *statusItem =
            new QTableWidgetItem(
                statusText(record.getStatus()));
        statusItem->setTextAlignment(Qt::AlignCenter);
        statusItem->setForeground(
            reviewStatusColor(record.getStatus()));
        QFont statusFont = statusItem->font();
        statusFont.setBold(true);
        statusItem->setFont(statusFont);
        reviewTable->setItem(row, 5, statusItem);
    }

    bool hasResults = reviewTable->rowCount() > 0;
    reviewTable->setVisible(hasResults);
    reviewEmptyLabel->setVisible(!hasResults);
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
        idItem->data(Qt::UserRole).toString().toStdString();

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
    detailStatusLabel->setProperty(
        "reviewStatus",
        reviewStatusProperty(record->getStatus()));
    detailStatusLabel->style()->unpolish(
        detailStatusLabel);
    detailStatusLabel->style()->polish(
        detailStatusLabel);

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

    VolunteerReviewService reviewService(*dataManager);
    const auto showReviewFailure = [this](
                                       VolunteerReviewStatus status)
    {
        QString message;
        switch (status)
        {
        case VolunteerReviewStatus::RecordNotFound:
            message = "所选志愿记录已不存在，请刷新后重试。";
            break;
        case VolunteerReviewStatus::RecordNotPending:
            message = "该记录已不处于待审核状态，请刷新后重试。";
            break;
        case VolunteerReviewStatus::CategoryNotFound:
            message = "该记录所属志愿类别不存在，无法计算通过积分。";
            break;
        case VolunteerReviewStatus::Success:
            message = "审核操作未能完成，请刷新后重试。";
            break;
        }
        QMessageBox::warning(this, "审核失败", message);
    };

    const VolunteerReviewOutcome preview =
        reviewService.previewApprovalScore(selectedRecordId);
    if (!preview.succeeded())
    {
        showReviewFailure(preview.status);
        return;
    }
    if (!preview.approvalScore.has_value())
    {
        QMessageBox::warning(
            this,
            "审核失败",
            "暂时无法计算通过积分，请刷新后重试。");
        return;
    }

    QMessageBox::StandardButton result =
        QMessageBox::question(
            this,
            "确认通过",
            "确定通过记录 " +
                QString::fromStdString(selectedRecordId) +
                " 吗？\n\n"
                "审核通过后获得积分：" +
                QString::number(
                    *preview.approvalScore,
                    'f',
                    2),
            QMessageBox::Yes |
                QMessageBox::No,
            QMessageBox::No);

    if (result != QMessageBox::Yes)
    {
        return;
    }

    const VolunteerReviewOutcome approval =
        reviewService.approve(selectedRecordId);
    if (!approval.succeeded())
    {
        showReviewFailure(approval.status);
        return;
    }

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

    QMessageBox::StandardButton result =
        QMessageBox::question(
            this,
            "确认驳回",
            "确定驳回记录 " +
                QString::fromStdString(selectedRecordId) +
                " 吗？",
            QMessageBox::Yes |
                QMessageBox::No,
            QMessageBox::No);

    if (result != QMessageBox::Yes)
    {
        return;
    }

    VolunteerReviewService reviewService(*dataManager);
    const VolunteerReviewOutcome rejection =
        reviewService.reject(selectedRecordId);
    if (!rejection.succeeded())
    {
        QString message;
        switch (rejection.status)
        {
        case VolunteerReviewStatus::RecordNotFound:
            message = "所选志愿记录已不存在，请刷新后重试。";
            break;
        case VolunteerReviewStatus::RecordNotPending:
            message = "该记录已不处于待审核状态，请刷新后重试。";
            break;
        case VolunteerReviewStatus::CategoryNotFound:
            message = "该记录所属志愿类别不存在。";
            break;
        case VolunteerReviewStatus::Success:
            message = "审核操作未能完成，请刷新后重试。";
            break;
        }
        QMessageBox::warning(this, "审核失败", message);
        return;
    }

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
    statisticsPage = new QWidget;
    statisticsPage->setObjectName(
        "administratorStatisticsPage");
    statisticsPage->setStyleSheet(
        StyleHelper::administratorRemainingPages());

    QVBoxLayout *mainLayout =
        new QVBoxLayout(statisticsPage);
    mainLayout->setContentsMargins(28, 24, 28, 24);
    mainLayout->setSpacing(16);

    QHBoxLayout *headerLayout = new QHBoxLayout;
    QVBoxLayout *titleLayout = new QVBoxLayout;
    titleLayout->setSpacing(4);

    QLabel *titleLabel = new QLabel("数据统计");
    titleLabel->setObjectName("adminRemainingPageTitle");
    QLabel *descriptionLabel = new QLabel(
        "汇总当前校园志愿服务数据与积分排名");
    descriptionLabel->setObjectName(
        "adminRemainingPageSubtitle");
    titleLayout->addWidget(titleLabel);
    titleLayout->addWidget(descriptionLabel);

    QPushButton *refreshButton = new QPushButton("刷新统计");
    refreshButton->setObjectName(
        "adminRemainingSecondaryButton");
    refreshButton->setMinimumHeight(40);
    refreshButton->setCursor(Qt::PointingHandCursor);

    headerLayout->addLayout(titleLayout);
    headerLayout->addStretch();
    headerLayout->addWidget(refreshButton);
    mainLayout->addLayout(headerLayout);

    QHBoxLayout *statisticsCardLayout = new QHBoxLayout;
    statisticsCardLayout->setSpacing(16);

    const auto addStatisticCard =
        [&statisticsCardLayout](
            const QString &caption,
            QLabel *&valueLabel)
        {
            QFrame *card = new QFrame;
            card->setObjectName(
                "adminRemainingStatCard");

            QVBoxLayout *cardLayout =
                new QVBoxLayout(card);
            cardLayout->setContentsMargins(
                20,
                18,
                20,
                18);
            cardLayout->setSpacing(8);

            QLabel *captionLabel = new QLabel(caption);
            captionLabel->setObjectName(
                "adminRemainingStatCaption");
            valueLabel = new QLabel("0");
            valueLabel->setObjectName(
                "adminRemainingStatValue");

            cardLayout->addWidget(captionLabel);
            cardLayout->addWidget(valueLabel);
            statisticsCardLayout->addWidget(card);
        };

    addStatisticCard(
        "学生数量",
        statisticsStudentCountLabel);
    addStatisticCard(
        "志愿记录总数",
        statisticsRecordCountLabel);
    addStatisticCard(
        "已通过记录数",
        statisticsApprovedCountLabel);
    addStatisticCard(
        "已通过志愿总时长",
        statisticsDurationLabel);
    mainLayout->addLayout(statisticsCardLayout);

    QFrame *categoryCard = new QFrame;
    categoryCard->setObjectName(
        "adminRemainingSectionCard");
    QVBoxLayout *categoryLayout =
        new QVBoxLayout(categoryCard);
    categoryLayout->setContentsMargins(20, 18, 20, 20);
    categoryLayout->setSpacing(12);

    QLabel *categoryTitle = new QLabel("分类统计");
    categoryTitle->setObjectName(
        "adminRemainingSectionTitle");
    categoryLayout->addWidget(categoryTitle);

    categoryStatisticsTable = new QTableWidget;
    categoryStatisticsTable->setObjectName(
        "adminRemainingCategoryTable");
    categoryStatisticsTable->setColumnCount(4);
    categoryStatisticsTable->setRowCount(3);
    categoryStatisticsTable->setHorizontalHeaderLabels(
        {"志愿类别", "已通过记录数", "累计时长", "累计积分"});
    categoryStatisticsTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers);
    categoryStatisticsTable->setSelectionMode(
        QAbstractItemView::NoSelection);
    categoryStatisticsTable->setAlternatingRowColors(true);
    categoryStatisticsTable->verticalHeader()
        ->setVisible(false);
    categoryStatisticsTable->horizontalHeader()
        ->setSectionResizeMode(QHeaderView::Stretch);
    categoryStatisticsTable->setMinimumHeight(150);
    categoryLayout->addWidget(categoryStatisticsTable);
    mainLayout->addWidget(categoryCard);

    QFrame *rankingCard = new QFrame;
    rankingCard->setObjectName(
        "adminRemainingSectionCard");
    QVBoxLayout *rankingLayout =
        new QVBoxLayout(rankingCard);
    rankingLayout->setContentsMargins(20, 18, 20, 20);
    rankingLayout->setSpacing(12);

    QLabel *rankingTitle = new QLabel("积分排行榜");
    rankingTitle->setObjectName(
        "adminRemainingSectionTitle");
    rankingLayout->addWidget(rankingTitle);

    statisticsRankingTable = new QTableWidget;
    statisticsRankingTable->setObjectName(
        "adminRemainingRankingTable");
    statisticsRankingTable->setColumnCount(4);
    statisticsRankingTable->setHorizontalHeaderLabels(
        {"排名", "学生账号", "学生姓名", "总积分"});
    statisticsRankingTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers);
    statisticsRankingTable->setSelectionBehavior(
        QAbstractItemView::SelectRows);
    statisticsRankingTable->setSelectionMode(
        QAbstractItemView::SingleSelection);
    statisticsRankingTable->setAlternatingRowColors(true);
    statisticsRankingTable->verticalHeader()
        ->setVisible(false);
    statisticsRankingTable->horizontalHeader()
        ->setSectionResizeMode(QHeaderView::Stretch);
    statisticsRankingTable->setMinimumHeight(220);
    rankingLayout->addWidget(statisticsRankingTable);
    mainLayout->addWidget(rankingCard, 1);

    connect(
        refreshButton,
        &QPushButton::clicked,
        this,
        &AdministratorMainWindow::refreshStatisticsPage);

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
    createStudentPage = new QWidget;
    createStudentPage->setObjectName(
        "administratorCreateStudentPage");
    createStudentPage->setStyleSheet(
        StyleHelper::administratorRemainingPages());

    QVBoxLayout *mainLayout =
        new QVBoxLayout(createStudentPage);
    mainLayout->setContentsMargins(28, 24, 28, 24);
    mainLayout->setSpacing(16);

    QLabel *titleLabel = new QLabel("创建学生");
    titleLabel->setObjectName("adminRemainingPageTitle");
    QLabel *descriptionLabel = new QLabel(
        "填写账号与基本资料，提交前可核对确认信息");
    descriptionLabel->setObjectName(
        "adminRemainingPageSubtitle");
    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(descriptionLabel);

    QFrame *formCard = new QFrame;
    formCard->setObjectName("adminRemainingFormCard");
    QVBoxLayout *cardLayout = new QVBoxLayout(formCard);
    cardLayout->setContentsMargins(24, 22, 24, 24);
    cardLayout->setSpacing(16);

    QLabel *formDescription = new QLabel(
        "确认窗口会显示账号、姓名、班级和专业");
    formDescription->setObjectName(
        "adminRemainingFormDescription");
    cardLayout->addWidget(formDescription);

    QFormLayout *formLayout = new QFormLayout;
    formLayout->setLabelAlignment(
        Qt::AlignLeft | Qt::AlignVCenter);
    formLayout->setHorizontalSpacing(20);
    formLayout->setVerticalSpacing(14);
    formLayout->setFieldGrowthPolicy(
        QFormLayout::AllNonFixedFieldsGrow);

    studentAccountEdit = new QLineEdit;
    studentAccountEdit->setObjectName(
        "adminCreateStudentAccount");
    studentAccountEdit->setPlaceholderText(
        "例如：20250002");
    studentAccountEdit->setMinimumHeight(44);

    studentNameEdit = new QLineEdit;
    studentNameEdit->setObjectName(
        "adminCreateStudentName");
    studentNameEdit->setPlaceholderText(
        "请输入学生姓名");
    studentNameEdit->setMinimumHeight(44);

    studentPasswordEdit = new QLineEdit;
    studentPasswordEdit->setObjectName(
        "adminCreateStudentPassword");
    studentPasswordEdit->setPlaceholderText(
        "请输入初始密码");
    studentPasswordEdit->setEchoMode(
        QLineEdit::Password);
    studentPasswordEdit->setMinimumHeight(44);

    studentClassEdit = new QLineEdit;
    studentClassEdit->setObjectName(
        "adminCreateStudentClass");
    studentClassEdit->setPlaceholderText(
        "例如：人工智能1班");
    studentClassEdit->setMinimumHeight(44);

    studentMajorEdit = new QLineEdit;
    studentMajorEdit->setObjectName(
        "adminCreateStudentMajor");
    studentMajorEdit->setPlaceholderText(
        "例如：人工智能");
    studentMajorEdit->setMinimumHeight(44);

    const auto createFieldLabel =
        [](const QString &text)
        {
            QLabel *label = new QLabel(text);
            label->setObjectName(
                "adminRemainingFieldLabel");
            return label;
        };

    formLayout->addRow(
        createFieldLabel("学生账号"),
        studentAccountEdit);
    formLayout->addRow(
        createFieldLabel("学生姓名"),
        studentNameEdit);
    formLayout->addRow(
        createFieldLabel("初始密码"),
        studentPasswordEdit);
    formLayout->addRow(
        createFieldLabel("班级"),
        studentClassEdit);
    formLayout->addRow(
        createFieldLabel("专业"),
        studentMajorEdit);
    cardLayout->addLayout(formLayout);

    QPushButton *createButton = new QPushButton("创建学生");
    createButton->setObjectName(
        "adminRemainingPrimaryButton");
    createButton->setMinimumSize(140, 44);
    createButton->setCursor(Qt::PointingHandCursor);
    cardLayout->addWidget(
        createButton,
        0,
        Qt::AlignRight);

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
    createAdministratorPage = new QWidget;
    createAdministratorPage->setObjectName(
        "administratorCreateAdministratorPage");
    createAdministratorPage->setStyleSheet(
        StyleHelper::administratorRemainingPages());

    QVBoxLayout *mainLayout =
        new QVBoxLayout(createAdministratorPage);
    mainLayout->setContentsMargins(28, 24, 28, 24);
    mainLayout->setSpacing(16);

    QLabel *titleLabel = new QLabel("创建管理员");
    titleLabel->setObjectName("adminRemainingPageTitle");
    QLabel *descriptionLabel = new QLabel(
        "填写新管理员的账号信息，提交前可核对确认内容");
    descriptionLabel->setObjectName(
        "adminRemainingPageSubtitle");
    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(descriptionLabel);

    QFrame *formCard = new QFrame;
    formCard->setObjectName("adminRemainingFormCard");
    QVBoxLayout *cardLayout = new QVBoxLayout(formCard);
    cardLayout->setContentsMargins(24, 22, 24, 24);
    cardLayout->setSpacing(16);

    QLabel *formDescription = new QLabel(
        "确认窗口会显示管理员账号和姓名");
    formDescription->setObjectName(
        "adminRemainingFormDescription");
    cardLayout->addWidget(formDescription);

    QFormLayout *formLayout = new QFormLayout;
    formLayout->setLabelAlignment(
        Qt::AlignLeft | Qt::AlignVCenter);
    formLayout->setHorizontalSpacing(20);
    formLayout->setVerticalSpacing(14);
    formLayout->setFieldGrowthPolicy(
        QFormLayout::AllNonFixedFieldsGrow);

    administratorAccountEdit = new QLineEdit;
    administratorAccountEdit->setObjectName(
        "adminCreateAdministratorAccount");
    administratorAccountEdit->setPlaceholderText(
        "例如：admin002");
    administratorAccountEdit->setMinimumHeight(44);

    administratorNameEdit = new QLineEdit;
    administratorNameEdit->setObjectName(
        "adminCreateAdministratorName");
    administratorNameEdit->setPlaceholderText(
        "请输入管理员姓名");
    administratorNameEdit->setMinimumHeight(44);

    administratorPasswordEdit = new QLineEdit;
    administratorPasswordEdit->setObjectName(
        "adminCreateAdministratorPassword");
    administratorPasswordEdit->setPlaceholderText(
        "请输入初始密码");
    administratorPasswordEdit->setEchoMode(
        QLineEdit::Password);
    administratorPasswordEdit->setMinimumHeight(44);

    const auto createFieldLabel =
        [](const QString &text)
        {
            QLabel *label = new QLabel(text);
            label->setObjectName(
                "adminRemainingFieldLabel");
            return label;
        };

    formLayout->addRow(
        createFieldLabel("管理员账号"),
        administratorAccountEdit);
    formLayout->addRow(
        createFieldLabel("管理员姓名"),
        administratorNameEdit);
    formLayout->addRow(
        createFieldLabel("初始密码"),
        administratorPasswordEdit);
    cardLayout->addLayout(formLayout);

    QPushButton *createButton = new QPushButton("创建管理员");
    createButton->setObjectName(
        "adminRemainingPrimaryButton");
    createButton->setMinimumSize(140, 44);
    createButton->setCursor(Qt::PointingHandCursor);
    cardLayout->addWidget(
        createButton,
        0,
        Qt::AlignRight);

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
    profilePage->setObjectName(
        "administratorProfilePage");
    profilePage->setStyleSheet(
        StyleHelper::profilePages());

    QVBoxLayout *mainLayout =
        new QVBoxLayout(profilePage);
    mainLayout->setContentsMargins(
        28,
        24,
        28,
        24);
    mainLayout->setSpacing(16);

    QLabel *titleLabel =
        new QLabel("个人信息");
    titleLabel->setObjectName(
        "profilePageTitle");

    QLabel *description =
        new QLabel(
            "查看管理员身份资料与账号安全设置");
    description->setObjectName(
        "profilePageSubtitle");

    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(description);

    QFrame *identityCard = new QFrame;
    identityCard->setObjectName(
        "profileIdentityCard");

    QHBoxLayout *identityLayout =
        new QHBoxLayout(identityCard);
    identityLayout->setContentsMargins(
        24,
        20,
        24,
        20);
    identityLayout->setSpacing(14);

    QLabel *avatar = new QLabel("管");
    avatar->setObjectName(
        "profileIdentityAvatar");
    avatar->setFixedSize(64, 64);
    avatar->setAlignment(Qt::AlignCenter);

    QVBoxLayout *nameLayout =
        new QVBoxLayout;
    nameLayout->setSpacing(5);
    profileNameLabel = new QLabel;
    profileNameLabel->setObjectName(
        "profileIdentityName");

    profileAccountLabel = new QLabel;
    profileAccountLabel->setObjectName(
        "profileIdentityAccount");

    nameLayout->addWidget(profileNameLabel);
    nameLayout->addWidget(profileAccountLabel);

    identityLayout->addWidget(avatar);
    identityLayout->addLayout(nameLayout);
    identityLayout->addStretch();
    mainLayout->addWidget(identityCard);

    QFrame *infoCard = new QFrame;
    infoCard->setObjectName(
        "profileInfoCard");
    QVBoxLayout *infoCardLayout =
        new QVBoxLayout(infoCard);
    infoCardLayout->setContentsMargins(
        24,
        20,
        24,
        22);
    infoCardLayout->setSpacing(16);

    QLabel *infoTitle =
        new QLabel("基本信息");
    infoTitle->setObjectName(
        "profileSectionTitle");
    infoCardLayout->addWidget(infoTitle);

    QFormLayout *infoLayout =
        new QFormLayout;
    infoLayout->setLabelAlignment(
        Qt::AlignLeft | Qt::AlignVCenter);
    infoLayout->setHorizontalSpacing(30);
    infoLayout->setVerticalSpacing(14);

    QLabel *accountTitle =
        new QLabel("管理员账号");
    accountTitle->setObjectName(
        "profileFieldLabel");
    QLabel *nameTitle =
        new QLabel("姓名");
    nameTitle->setObjectName(
        "profileFieldLabel");

    QLabel *accountValue = new QLabel;
    accountValue->setObjectName(
        "adminAccountDetail");
    QLabel *nameValue = new QLabel;
    nameValue->setObjectName(
        "adminNameDetail");

    infoLayout->addRow(accountTitle, accountValue);
    infoLayout->addRow(nameTitle, nameValue);
    infoCardLayout->addLayout(infoLayout);
    mainLayout->addWidget(infoCard);

    QFrame *securityCard = new QFrame;
    securityCard->setObjectName(
        "profileSecurityCard");
    QHBoxLayout *securityLayout =
        new QHBoxLayout(securityCard);
    securityLayout->setContentsMargins(
        24,
        18,
        24,
        18);
    securityLayout->setSpacing(16);

    QVBoxLayout *securityTextLayout =
        new QVBoxLayout;
    securityTextLayout->setSpacing(5);
    QLabel *securityTitle =
        new QLabel("账号安全");
    securityTitle->setObjectName(
        "profileSectionTitle");
    QLabel *securityDescription =
        new QLabel(
            "建议定期修改登录密码，保护账号安全。");
    securityDescription->setObjectName(
        "profileSecurityDescription");
    securityTextLayout->addWidget(securityTitle);
    securityTextLayout->addWidget(
        securityDescription);

    QPushButton *passwordButton =
        new QPushButton("修改密码");
    passwordButton->setObjectName(
        "profilePasswordButton");
    passwordButton->setMinimumSize(110, 40);
    passwordButton->setCursor(
        Qt::PointingHandCursor);
    securityLayout->addLayout(
        securityTextLayout);
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
        "管理员账号：" +
        QString::fromStdString(
            administrator->getAccountId()));

    QLabel *accountValue =
        profilePage->findChild<QLabel *>(
            "adminAccountDetail");
    if (accountValue != nullptr)
    {
        accountValue->setText(
            QString::fromStdString(
                administrator->getAccountId()));
    }

    QLabel *nameValue =
        profilePage->findChild<QLabel *>(
            "adminNameDetail");
    if (nameValue != nullptr)
    {
        nameValue->setText(
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
