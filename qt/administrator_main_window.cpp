#include "administrator_main_window.h"
#include "style_helper.h"
#include "operation_log_table_model.h"

#include <QAbstractItemView>
#include <QColor>
#include <QComboBox>
#include <QCoreApplication>
#include <QDoubleSpinBox>
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
#include <QSignalBlocker>
#include <QStyle>
#include <QStyleOptionViewItem>
#include <QStyledItemDelegate>
#include <QStackedWidget>
#include <QTableView>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextEdit>
#include <QVBoxLayout>

#include <algorithm>
#include <utility>

#include "administrator.h"
#include "data_manager.h"
#include "diary_service.h"
#include "operation_log_service.h"
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

    QString reviewFailureMessage(VolunteerReviewStatus status)
    {
        switch (status)
        {
        case VolunteerReviewStatus::RecordNotFound:
            return "所选志愿记录已不存在，请刷新后重试。";
        case VolunteerReviewStatus::RecordNotPending:
            return "该记录已不处于待审核状态，请刷新后重试。";
        case VolunteerReviewStatus::CategoryNotFound:
            return "该记录所属志愿类别不存在，无法完成审核。";
        case VolunteerReviewStatus::InvalidFinalDuration:
            return "最终服务时长必须大于 0，并以 0.5 小时为单位。";
        case VolunteerReviewStatus::ReviewNoteRequired:
            return "类别或时长发生修正时，请填写审核意见；驳回时请填写驳回原因。";
        case VolunteerReviewStatus::InvalidReviewNote:
            return "审核意见不能包含竖线或换行符。";
        case VolunteerReviewStatus::Success:
        case VolunteerReviewStatus::PersistenceFailure:
        case VolunteerReviewStatus::SeverePersistenceFailure:
            return "审核操作未能完成，请刷新后重试。";
        }
        return "审核操作未能完成，请刷新后重试。";
    }

    OperationLogQuery operationLogQueryFrom(
        const QComboBox *typeFilter,
        const QComboBox *targetTypeFilter,
        const QLineEdit *targetIdEdit)
    {
        OperationLogQuery query;
        const int selectedType = typeFilter->currentData().toInt();
        if (selectedType >= 0)
        {
            query.operationType = static_cast<OperationType>(selectedType);
        }
        const int selectedTargetType =
            targetTypeFilter->currentData().toInt();
        if (selectedTargetType >= 0)
        {
            query.targetType =
                static_cast<OperationTargetType>(selectedTargetType);
        }
        const QString targetId = targetIdEdit->text().trimmed();
        if (!targetId.isEmpty())
        {
            query.targetId = targetId.toStdString();
        }
        return query;
    }

    QString diaryDisplayStatusText(DiaryDisplayStatus status)
    {
        switch (status)
        {
        case DiaryDisplayStatus::PendingDisplayReview:
            return "待展示审核";
        case DiaryDisplayStatus::Displayed:
            return "已展示";
        case DiaryDisplayStatus::Rejected:
            return "审核未通过";
        case DiaryDisplayStatus::TakenDown:
            return "已下架";
        }
        return "未知状态";
    }

    QString diaryServiceFailureMessage(DiaryServiceStatus status)
    {
        switch (status)
        {
        case DiaryServiceStatus::StudentNotFound:
            return "学生账号不存在，请刷新后重试。";
        case DiaryServiceStatus::AdministratorNotFound:
            return "当前管理员账号不存在，请重新登录。";
        case DiaryServiceStatus::RecordNotFound:
            return "关联志愿记录不存在。";
        case DiaryServiceStatus::RecordNotOwned:
            return "关联志愿记录与作者不匹配。";
        case DiaryServiceStatus::RecordNotApproved:
            return "关联志愿记录未通过审核，无法执行此操作。";
        case DiaryServiceStatus::DuplicateRecord:
            return "该志愿记录已有日记申请。";
        case DiaryServiceStatus::DiaryNotFound:
            return "所选日记不存在，请刷新后重试。";
        case DiaryServiceStatus::InvalidTitle:
            return "日记标题无效。";
        case DiaryServiceStatus::InvalidContent:
            return "日记内容无效。";
        case DiaryServiceStatus::InvalidState:
            return "该日记状态不允许此操作，请刷新后重试。";
        case DiaryServiceStatus::AlreadyLiked:
            return "当前学生已经点赞。";
        case DiaryServiceStatus::LikeNotFound:
            return "当前没有可取消的点赞。";
        case DiaryServiceStatus::PersistenceFailure:
            return "保存失败，系统已恢复到操作前状态。";
        case DiaryServiceStatus::SeverePersistenceFailure:
            return "数据保存发生严重错误。请停止写入并重启应用。";
        case DiaryServiceStatus::Success:
            return {};
        }
        return "日记操作失败，请刷新后重试。";
    }

    void showDiaryServiceFailure(
        QWidget *parent,
        DiaryServiceStatus status)
    {
        const QString message = diaryServiceFailureMessage(status);
        if (status == DiaryServiceStatus::SeverePersistenceFailure)
        {
            QMessageBox::critical(parent, "数据保存严重错误", message);
            QCoreApplication::exit(1);
            return;
        }
        QMessageBox::warning(parent, "日记操作失败", message);
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
      detailReviewerLabel(nullptr),
      detailScoreLabel(nullptr),
      detailFinalFactsLabel(nullptr),
      detailReviewNoteLabel(nullptr),
      reviewFinalCategoryCombo(nullptr),
      reviewFinalDurationSpin(nullptr),
      reviewNoteEdit(nullptr),
      reviewPreviewScoreLabel(nullptr),
      reviewInputsContainer(nullptr),
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
      profileAccountLabel(nullptr),
      operationLogPage(nullptr),
      operationLogTypeFilter(nullptr),
      operationLogTargetTypeFilter(nullptr),
      operationLogTargetIdEdit(nullptr),
      operationLogTable(nullptr),
      operationLogEmptyLabel(nullptr),
      operationLogModel(nullptr),
      diaryManagementPage(nullptr),
      diaryStatusFilter(nullptr),
      diaryManagementTable(nullptr),
      diaryManagementEmptyLabel(nullptr),
      diaryManagementDetail(nullptr),
      diaryApproveButton(nullptr),
      diaryRejectButton(nullptr),
      diaryTakeDownButton(nullptr)
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

    navigationList->addItem(
        "操作日志");

    navigationList->addItem(
        "日记管理");

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
    buildOperationLogPage();
    buildDiaryManagementPage();

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

    contentStack->addWidget(
        operationLogPage);

    contentStack->addWidget(
        diaryManagementPage);

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

    if (row == 6)
    {
        refreshOperationLogPage();
        contentStack->setCurrentWidget(operationLogPage);
        return;
    }

    if (row == 7)
    {
        refreshDiaryManagementPage();
        contentStack->setCurrentWidget(diaryManagementPage);
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
    detailReviewerLabel = new QLabel;
    detailScoreLabel = new QLabel;
    detailFinalFactsLabel = new QLabel;
    detailReviewNoteLabel = new QLabel;

    for (QLabel *fieldLabel :
         {detailStudentLabel,
          detailCategoryLabel,
          detailDateLabel,
          detailDurationLabel,
          detailPlaceLabel,
          detailWitnessLabel,
          detailDescriptionLabel,
          detailStatusLabel,
          detailReviewerLabel,
          detailScoreLabel,
          detailFinalFactsLabel,
          detailReviewNoteLabel})
    {
        fieldLabel->setObjectName("adminReviewField");
    }

    detailStatusLabel->setObjectName(
        "adminReviewDetailStatus");
    detailDescriptionLabel->setWordWrap(true);
    detailDescriptionLabel->setTextInteractionFlags(
        Qt::TextSelectableByMouse);
    detailFinalFactsLabel->setWordWrap(true);
    detailReviewNoteLabel->setWordWrap(true);

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
    detailGrid->addWidget(detailFinalFactsLabel, 5, 0, 1, 2);
    detailGrid->addWidget(detailReviewerLabel, 6, 0, 1, 2);
    detailGrid->addWidget(detailReviewNoteLabel, 7, 0, 1, 2);
    detailLayout->addLayout(detailGrid);

    reviewInputsContainer = new QWidget;
    reviewInputsContainer->setObjectName(
        "adminReviewInputsContainer");
    QFormLayout *reviewInputLayout = new QFormLayout(
        reviewInputsContainer);
    reviewInputLayout->setHorizontalSpacing(16);
    reviewInputLayout->setVerticalSpacing(10);

    reviewFinalCategoryCombo = new QComboBox;
    reviewFinalCategoryCombo->setObjectName(
        "adminReviewFinalCategory");
    reviewFinalCategoryCombo->setMinimumHeight(38);
    if (dataManager != nullptr)
    {
        for (const VolunteerCategory &category :
             dataManager->getCategories())
        {
            reviewFinalCategoryCombo->addItem(
                QString::fromStdString(category.getName()),
                QString::fromStdString(category.getCategoryId()));
        }
    }

    reviewFinalDurationSpin = new QDoubleSpinBox;
    reviewFinalDurationSpin->setObjectName(
        "adminReviewFinalDuration");
    reviewFinalDurationSpin->setRange(0.0, 10000.0);
    reviewFinalDurationSpin->setDecimals(2);
    reviewFinalDurationSpin->setSingleStep(0.5);
    reviewFinalDurationSpin->setSuffix(" 小时");
    reviewFinalDurationSpin->setMinimumHeight(38);

    reviewNoteEdit = new QLineEdit;
    reviewNoteEdit->setObjectName("adminReviewNote");
    reviewNoteEdit->setPlaceholderText(
        "修正类别/时长时必填；驳回时填写驳回原因");
    reviewNoteEdit->setMinimumHeight(38);

    reviewPreviewScoreLabel = new QLabel("预估积分：—");
    reviewPreviewScoreLabel->setObjectName(
        "adminReviewPreviewScore");

    reviewInputLayout->addRow("最终类别", reviewFinalCategoryCombo);
    reviewInputLayout->addRow("最终时长", reviewFinalDurationSpin);
    reviewInputLayout->addRow("审核意见 / 驳回原因", reviewNoteEdit);
    reviewInputLayout->addRow("审核预览", reviewPreviewScoreLabel);
    detailLayout->addWidget(reviewInputsContainer);

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

    connect(
        reviewFinalCategoryCombo,
        QOverload<int>::of(&QComboBox::currentIndexChanged),
        this,
        &AdministratorMainWindow::updateReviewPreview);
    connect(
        reviewFinalDurationSpin,
        QOverload<double>::of(&QDoubleSpinBox::valueChanged),
        this,
        &AdministratorMainWindow::updateReviewPreview);
    connect(
        reviewNoteEdit,
        &QLineEdit::textChanged,
        this,
        &AdministratorMainWindow::updateReviewPreview);

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
                    record.getAppliedCategoryId())));

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
                    record.getAppliedDuration(),
                    'g',
                    15) +
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
        "学生申请类别：" +
        categoryName(record->getAppliedCategoryId()));

    detailDateLabel->setText(
        "服务日期：" +
        QString::fromStdString(
            record->getDate()));

    detailDurationLabel->setText(
        "服务时长：" +
        QString::number(
            record->getAppliedDuration(),
            'g',
            15) +
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

    detailReviewerLabel->hide();
    if (record->getStatus() == RecordStatus::Approved ||
        record->getStatus() == RecordStatus::Rejected)
    {
        if (record->getReviewerAccountId().has_value())
        {
            detailReviewerLabel->setText(
                "审核人：" + QString::fromStdString(
                    *record->getReviewerAccountId()));
            detailReviewerLabel->show();
        }
        else if (record->isLegacyCompatibilityRecord())
        {
            detailReviewerLabel->setText(
                "审核人：历史记录未保存");
            detailReviewerLabel->show();
        }
    }

    if (record->getStatus() ==
        RecordStatus::Approved)
    {
        detailScoreLabel->setText(
            "最终积分：" + QString::number(
                record->getFinalScore().value_or(0.0), 'f', 1));

        detailFinalFactsLabel->setText(
            "最终类别：" + categoryName(
                record->getFinalCategoryId().value_or(
                    record->getAppliedCategoryId())) +
            "；最终时长：" + QString::number(
                record->getFinalDuration().value_or(
                    record->getAppliedDuration()), 'g', 15) +
            " 小时");
        detailFinalFactsLabel->show();
        detailScoreLabel->show();

        if (record->getReviewNote().has_value())
        {
            detailReviewNoteLabel->setText(
                "审核意见：" + QString::fromStdString(
                    *record->getReviewNote()));
            detailReviewNoteLabel->show();
        }
        else
        {
            detailReviewNoteLabel->hide();
        }
    }
    else if (record->getStatus() == RecordStatus::Rejected)
    {
        detailFinalFactsLabel->hide();
        detailScoreLabel->hide();
        if (record->getReviewNote().has_value())
        {
            detailReviewNoteLabel->setText(
                "驳回原因：" + QString::fromStdString(
                    *record->getReviewNote()));
            detailReviewNoteLabel->show();
        }
        else if (record->isLegacyCompatibilityRecord())
        {
            detailReviewNoteLabel->setText(
                "历史记录未保存审核意见");
            detailReviewNoteLabel->show();
        }
        else
        {
            detailReviewNoteLabel->hide();
        }
    }
    else
    {
        detailFinalFactsLabel->hide();
        detailScoreLabel->hide();
        detailReviewNoteLabel->hide();
    }

    bool canReview =
        record->getStatus() ==
        RecordStatus::Pending;

    {
        const QSignalBlocker categoryBlocker(reviewFinalCategoryCombo);
        const QSignalBlocker durationBlocker(reviewFinalDurationSpin);
        const QSignalBlocker noteBlocker(reviewNoteEdit);
        const int categoryIndex = reviewFinalCategoryCombo->findData(
            QString::fromStdString(record->getAppliedCategoryId()));
        reviewFinalCategoryCombo->setCurrentIndex(categoryIndex);
        reviewFinalDurationSpin->setValue(record->getAppliedDuration());
        reviewNoteEdit->clear();
    }
    reviewInputsContainer->setVisible(canReview);

    approveButton->setVisible(canReview);
    rejectButton->setVisible(canReview);
    reviewDetailFrame->show();
    if (canReview)
    {
        updateReviewPreview();
    }
}

void AdministratorMainWindow::updateReviewPreview()
{
    if (dataManager == nullptr || selectedRecordId.empty() ||
        reviewFinalCategoryCombo == nullptr ||
        reviewFinalCategoryCombo->currentIndex() < 0)
    {
        if (reviewPreviewScoreLabel != nullptr)
        {
            reviewPreviewScoreLabel->setText("预估积分：—");
        }
        return;
    }

    VolunteerApprovalInput input;
    input.finalCategoryId = reviewFinalCategoryCombo->currentData()
                                .toString().toStdString();
    input.finalDuration = reviewFinalDurationSpin->value();
    input.reviewNote = reviewNoteEdit->text().toStdString();

    VolunteerReviewService service(*dataManager);
    const VolunteerReviewOutcome preview =
        service.previewApproval(selectedRecordId, input);
    if (preview.succeeded() && preview.approvalScore.has_value())
    {
        reviewPreviewScoreLabel->setText(
            "预估积分：" + QString::number(
                *preview.approvalScore, 'f', 1));
    }
    else if (preview.status == VolunteerReviewStatus::ReviewNoteRequired)
    {
        reviewPreviewScoreLabel->setText("预估积分：修正时请填写审核意见");
    }
    else
    {
        reviewPreviewScoreLabel->setText(
            "预估积分：" + reviewFailureMessage(preview.status));
    }
}

void AdministratorMainWindow::showReviewFailure(
    VolunteerReviewStatus status)
{
    if (status == VolunteerReviewStatus::PersistenceFailure)
    {
        refreshReviewPage();
        QMessageBox::warning(
            this,
            "保存失败",
            "审核状态未能保存，系统已恢复到提交前状态。");
        return;
    }
    if (status == VolunteerReviewStatus::SeverePersistenceFailure)
    {
        QMessageBox::critical(
            this,
            "数据保存严重错误",
            "记录与审计日志可能未能完整保存。应用即将退出，请检查数据文件。");
        QCoreApplication::exit(1);
        return;
    }

    QMessageBox::warning(
        this,
        "审核失败",
        reviewFailureMessage(status));
}

void AdministratorMainWindow::approveSelectedRecord()
{
    if (dataManager == nullptr ||
        selectedRecordId.empty())
    {
        return;
    }

    if (reviewFinalCategoryCombo->currentIndex() < 0)
    {
        QMessageBox::warning(this, "审核失败", "请选择有效的最终类别。");
        return;
    }
    VolunteerApprovalInput input;
    input.finalCategoryId = reviewFinalCategoryCombo->currentData()
                                .toString().toStdString();
    input.finalDuration = reviewFinalDurationSpin->value();
    input.reviewNote = reviewNoteEdit->text().toStdString();

    VolunteerReviewService reviewService(*dataManager);
    const VolunteerReviewOutcome preview =
        reviewService.previewApproval(selectedRecordId, input);
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
                    1),
            QMessageBox::Yes |
                QMessageBox::No,
            QMessageBox::No);

    if (result != QMessageBox::Yes)
    {
        return;
    }

    const VolunteerReviewOutcome approval =
        reviewService.approve(accountId, selectedRecordId, input);
    if (!approval.succeeded())
    {
        showReviewFailure(approval.status);
        return;
    }

    refreshReviewPage();
    refreshHomePage();
    refreshOperationLogPage();

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

    const QString reason = reviewNoteEdit->text().trimmed();
    if (reason.isEmpty())
    {
        QMessageBox::warning(this, "无法驳回", "请填写非空的驳回原因。");
        return;
    }
    if (containsInvalidPersistenceCharacter(reason))
    {
        QMessageBox::warning(
            this, "无法驳回", "驳回原因不能包含竖线或换行符。");
        return;
    }

    QMessageBox::StandardButton result =
        QMessageBox::question(
            this,
            "确认驳回",
            "确定驳回记录 " +
                QString::fromStdString(selectedRecordId) +
                " 吗？\n\n驳回原因：" + reason,
            QMessageBox::Yes |
                QMessageBox::No,
            QMessageBox::No);

    if (result != QMessageBox::Yes)
    {
        return;
    }

    VolunteerReviewService reviewService(*dataManager);
    const VolunteerReviewOutcome rejection =
        reviewService.reject(
            accountId, selectedRecordId, reason.toStdString());
    if (!rejection.succeeded())
    {
        showReviewFailure(rejection.status);
        return;
    }

    refreshReviewPage();
    refreshHomePage();
    refreshOperationLogPage();

    QMessageBox::information(
        this,
        "审核完成",
        "该志愿记录已驳回。");
}

void AdministratorMainWindow::buildOperationLogPage()
{
    operationLogPage = new QWidget;
    operationLogPage->setObjectName(
        "administratorOperationLogPage");
    operationLogPage->setStyleSheet(
        StyleHelper::administratorOperationLogPage());

    QVBoxLayout *mainLayout = new QVBoxLayout(operationLogPage);
    mainLayout->setContentsMargins(22, 20, 22, 22);
    mainLayout->setSpacing(16);

    QLabel *titleLabel = new QLabel("操作日志");
    titleLabel->setObjectName("adminOperationLogTitle");
    QLabel *subtitleLabel = new QLabel(
        "查看管理员对志愿记录执行的审核操作及审计信息。");
    subtitleLabel->setObjectName("adminOperationLogSubtitle");
    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(subtitleLabel);
    mainLayout->addWidget(buildOperationLogFilterCard());
    mainLayout->addWidget(buildOperationLogTableCard(), 1);
    refreshOperationLogPage();
}

QFrame *AdministratorMainWindow::buildOperationLogFilterCard()
{
    QFrame *filterCard = new QFrame;
    filterCard->setObjectName("adminOperationLogFilterCard");
    QHBoxLayout *filterLayout = new QHBoxLayout(filterCard);
    filterLayout->setContentsMargins(16, 14, 16, 14);
    filterLayout->setSpacing(10);
    addOperationLogTypeFilter(filterLayout);
    addOperationLogTargetFilter(filterLayout);

    QPushButton *clearButton = new QPushButton("清除筛选");
    clearButton->setObjectName("adminOperationLogClearButton");
    clearButton->setCursor(Qt::PointingHandCursor);
    QPushButton *refreshButton = new QPushButton("查询 / 刷新");
    refreshButton->setObjectName("adminOperationLogRefreshButton");
    refreshButton->setCursor(Qt::PointingHandCursor);
    filterLayout->addWidget(clearButton);
    filterLayout->addWidget(refreshButton);
    connectOperationLogFilters(clearButton, refreshButton);
    return filterCard;
}

void AdministratorMainWindow::addOperationLogTypeFilter(
    QHBoxLayout *layout)
{
    QLabel *label = new QLabel("操作类型");
    label->setObjectName("adminOperationLogFieldLabel");
    operationLogTypeFilter = new QComboBox;
    operationLogTypeFilter->setObjectName(
        "adminOperationLogTypeFilter");
    operationLogTypeFilter->addItem("全部", -1);
    operationLogTypeFilter->addItem(
        "审核通过",
        static_cast<int>(OperationType::VolunteerRecordApproved));
    operationLogTypeFilter->addItem(
        "审核驳回",
        static_cast<int>(OperationType::VolunteerRecordRejected));
    operationLogTypeFilter->addItem(
        "日记展示审核通过",
        static_cast<int>(OperationType::DiaryDisplayApproved));
    operationLogTypeFilter->addItem(
        "日记展示审核未通过",
        static_cast<int>(OperationType::DiaryDisplayRejected));
    operationLogTypeFilter->addItem(
        "日记下架",
        static_cast<int>(OperationType::DiaryTakenDown));
    operationLogTypeFilter->setMinimumWidth(150);
    layout->addWidget(label);
    layout->addWidget(operationLogTypeFilter);
}

void AdministratorMainWindow::addOperationLogTargetFilter(
    QHBoxLayout *layout)
{
    QLabel *typeLabel = new QLabel("目标类型");
    typeLabel->setObjectName("adminOperationLogFieldLabel");
    operationLogTargetTypeFilter = new QComboBox;
    operationLogTargetTypeFilter->setObjectName(
        "adminOperationLogTargetTypeFilter");
    operationLogTargetTypeFilter->addItem("全部目标类型", -1);
    operationLogTargetTypeFilter->addItem(
        "志愿记录",
        static_cast<int>(OperationTargetType::VolunteerRecord));
    operationLogTargetTypeFilter->addItem(
        "日记",
        static_cast<int>(OperationTargetType::DiaryPost));

    QLabel *label = new QLabel("目标编号");
    label->setObjectName("adminOperationLogFieldLabel");
    operationLogTargetIdEdit = new QLineEdit;
    operationLogTargetIdEdit->setObjectName(
        "adminOperationLogTargetFilter");
    operationLogTargetIdEdit->setPlaceholderText("输入志愿记录编号");
    operationLogTargetIdEdit->setMinimumWidth(190);
    layout->addWidget(typeLabel);
    layout->addWidget(operationLogTargetTypeFilter);
    layout->addWidget(label);
    layout->addWidget(operationLogTargetIdEdit, 1);
}

void AdministratorMainWindow::connectOperationLogFilters(
    QPushButton *clearButton,
    QPushButton *refreshButton)
{
    connect(
        operationLogTypeFilter,
        QOverload<int>::of(&QComboBox::currentIndexChanged),
        this,
        &AdministratorMainWindow::applyOperationLogFilter);
    connect(
        operationLogTargetIdEdit,
        &QLineEdit::returnPressed,
        this,
        &AdministratorMainWindow::applyOperationLogFilter);
    connect(
        operationLogTargetTypeFilter,
        QOverload<int>::of(&QComboBox::currentIndexChanged),
        this,
        [this](int index)
        {
            const QString placeholder = index == 2
                ? QStringLiteral("输入日记编号")
                : (index == 1
                       ? QStringLiteral("输入志愿记录编号")
                       : QStringLiteral("输入编号（默认按志愿记录筛选）"));
            operationLogTargetIdEdit->setPlaceholderText(placeholder);
            applyOperationLogFilter();
        });
    connect(
        refreshButton,
        &QPushButton::clicked,
        this,
        &AdministratorMainWindow::refreshOperationLogPage);
    connect(
        clearButton,
        &QPushButton::clicked,
        this,
        [this]()
        {
            operationLogTypeFilter->setCurrentIndex(0);
            operationLogTargetTypeFilter->setCurrentIndex(0);
            operationLogTargetIdEdit->clear();
            refreshOperationLogPage();
        });
}

QFrame *AdministratorMainWindow::buildOperationLogTableCard()
{
    QFrame *tableCard = new QFrame;
    tableCard->setObjectName("adminOperationLogTableCard");
    QVBoxLayout *tableLayout = new QVBoxLayout(tableCard);
    tableLayout->setContentsMargins(16, 14, 16, 16);
    tableLayout->setSpacing(10);

    QLabel *tableTitle = new QLabel("审核操作记录");
    tableTitle->setObjectName("adminOperationLogSectionTitle");
    tableLayout->addWidget(tableTitle);

    configureOperationLogTable();
    tableLayout->addWidget(operationLogTable, 1);
    tableLayout->addWidget(operationLogEmptyLabel);
    return tableCard;
}

void AdministratorMainWindow::configureOperationLogTable()
{
    operationLogModel = new OperationLogTableModel(this);
    operationLogTable = new QTableView;
    operationLogTable->setObjectName("adminOperationLogTable");
    operationLogTable->setModel(operationLogModel);
    operationLogTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers);
    operationLogTable->setSelectionBehavior(
        QAbstractItemView::SelectRows);
    operationLogTable->setSelectionMode(
        QAbstractItemView::SingleSelection);
    operationLogTable->setAlternatingRowColors(true);
    operationLogTable->setShowGrid(false);
    operationLogTable->verticalHeader()->setVisible(false);
    operationLogTable->verticalHeader()->setDefaultSectionSize(42);
    operationLogTable->horizontalHeader()->setSectionResizeMode(
        QHeaderView::ResizeToContents);
    operationLogTable->horizontalHeader()->setSectionResizeMode(
        6, QHeaderView::Stretch);
    operationLogTable->setMinimumHeight(300);

    operationLogEmptyLabel = new QLabel(
        "当前暂无符合条件的操作日志");
    operationLogEmptyLabel->setObjectName(
        "adminOperationLogEmptyState");
    operationLogEmptyLabel->setAlignment(Qt::AlignCenter);
}

void AdministratorMainWindow::applyOperationLogFilter()
{
    refreshOperationLogPage();
}

void AdministratorMainWindow::refreshOperationLogPage()
{
    if (dataManager == nullptr || operationLogModel == nullptr)
    {
        return;
    }

    OperationLogService service(*dataManager);
    std::vector<OperationLogView> results = service.query(
        operationLogQueryFrom(
            operationLogTypeFilter,
            operationLogTargetTypeFilter,
            operationLogTargetIdEdit));
    operationLogEmptyLabel->setVisible(results.empty());
    operationLogModel->setResults(std::move(results));
}

void AdministratorMainWindow::buildDiaryManagementPage()
{
    diaryManagementPage = new QWidget;
    diaryManagementPage->setObjectName(
        "administratorDiaryManagementPage");
    diaryManagementPage->setStyleSheet(
        StyleHelper::administratorDiaryManagementPage());

    QVBoxLayout *mainLayout = new QVBoxLayout(diaryManagementPage);
    mainLayout->setContentsMargins(22, 20, 22, 22);
    mainLayout->setSpacing(14);

    QLabel *titleLabel = new QLabel("日记管理");
    titleLabel->setObjectName("adminDiaryPageTitle");
    QLabel *subtitleLabel = new QLabel(
        "审核学生提交的展示申请，并管理当前公开的校园日记。");
    subtitleLabel->setObjectName("adminDiaryPageSubtitle");
    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(subtitleLabel);

    QFrame *filterCard = new QFrame;
    filterCard->setObjectName("adminDiaryFilterCard");
    QHBoxLayout *filterLayout = new QHBoxLayout(filterCard);
    filterLayout->setContentsMargins(16, 12, 16, 12);
    filterLayout->setSpacing(10);
    QLabel *filterLabel = new QLabel("展示状态");
    filterLabel->setObjectName("adminDiaryFieldLabel");
    diaryStatusFilter = new QComboBox;
    diaryStatusFilter->setObjectName("adminDiaryStatusFilter");
    diaryStatusFilter->addItem("全部", -1);
    diaryStatusFilter->addItem(
        "待展示审核",
        static_cast<int>(DiaryDisplayStatus::PendingDisplayReview));
    diaryStatusFilter->addItem(
        "已展示",
        static_cast<int>(DiaryDisplayStatus::Displayed));
    diaryStatusFilter->addItem(
        "审核未通过",
        static_cast<int>(DiaryDisplayStatus::Rejected));
    diaryStatusFilter->addItem(
        "已下架",
        static_cast<int>(DiaryDisplayStatus::TakenDown));
    diaryStatusFilter->setMinimumWidth(180);

    QPushButton *refreshButton = new QPushButton("刷新日记");
    refreshButton->setObjectName("adminDiaryRefreshButton");
    refreshButton->setCursor(Qt::PointingHandCursor);
    filterLayout->addWidget(filterLabel);
    filterLayout->addWidget(diaryStatusFilter);
    filterLayout->addStretch();
    filterLayout->addWidget(refreshButton);
    mainLayout->addWidget(filterCard);

    QHBoxLayout *contentLayout = new QHBoxLayout;
    contentLayout->setSpacing(14);

    QFrame *tableCard = new QFrame;
    tableCard->setObjectName("adminDiaryTableCard");
    QVBoxLayout *tableCardLayout = new QVBoxLayout(tableCard);
    tableCardLayout->setContentsMargins(14, 12, 14, 14);
    tableCardLayout->setSpacing(8);
    QLabel *tableTitle = new QLabel("日记列表");
    tableTitle->setObjectName("adminDiarySectionTitle");
    tableCardLayout->addWidget(tableTitle);

    diaryManagementTable = new QTableWidget;
    diaryManagementTable->setObjectName("adminDiaryTable");
    diaryManagementTable->setColumnCount(5);
    diaryManagementTable->setHorizontalHeaderLabels(
        {"日记编号", "学生", "标题", "志愿记录", "状态"});
    diaryManagementTable->setEditTriggers(
        QAbstractItemView::NoEditTriggers);
    diaryManagementTable->setSelectionBehavior(
        QAbstractItemView::SelectRows);
    diaryManagementTable->setSelectionMode(
        QAbstractItemView::SingleSelection);
    diaryManagementTable->setAlternatingRowColors(true);
    diaryManagementTable->setShowGrid(false);
    diaryManagementTable->verticalHeader()->setVisible(false);
    diaryManagementTable->verticalHeader()->setDefaultSectionSize(40);
    diaryManagementTable->horizontalHeader()->setSectionResizeMode(
        QHeaderView::ResizeToContents);
    diaryManagementTable->horizontalHeader()->setSectionResizeMode(
        2, QHeaderView::Stretch);
    tableCardLayout->addWidget(diaryManagementTable, 1);
    diaryManagementEmptyLabel = new QLabel("当前筛选条件下没有日记。");
    diaryManagementEmptyLabel->setObjectName("adminDiaryEmptyState");
    diaryManagementEmptyLabel->setAlignment(Qt::AlignCenter);
    tableCardLayout->addWidget(diaryManagementEmptyLabel);

    QFrame *detailCard = new QFrame;
    detailCard->setObjectName("adminDiaryDetailCard");
    QVBoxLayout *detailLayout = new QVBoxLayout(detailCard);
    detailLayout->setContentsMargins(16, 12, 16, 14);
    detailLayout->setSpacing(9);
    QLabel *detailTitle = new QLabel("日记详情");
    detailTitle->setObjectName("adminDiarySectionTitle");
    detailLayout->addWidget(detailTitle);

    diaryManagementDetail = new QTextEdit;
    diaryManagementDetail->setObjectName("adminDiaryDetailText");
    diaryManagementDetail->setReadOnly(true);
    diaryManagementDetail->setPlaceholderText("选择一篇日记查看完整信息。");
    detailLayout->addWidget(diaryManagementDetail, 1);

    QHBoxLayout *actionLayout = new QHBoxLayout;
    actionLayout->setSpacing(8);
    diaryApproveButton = new QPushButton("审核通过");
    diaryApproveButton->setObjectName("adminDiaryApproveButton");
    diaryRejectButton = new QPushButton("审核未通过");
    diaryRejectButton->setObjectName("adminDiaryRejectButton");
    diaryTakeDownButton = new QPushButton("下架日记");
    diaryTakeDownButton->setObjectName("adminDiaryTakeDownButton");
    for (QPushButton *button : {
             diaryApproveButton,
             diaryRejectButton,
             diaryTakeDownButton})
    {
        button->setCursor(Qt::PointingHandCursor);
        button->setMinimumHeight(38);
        button->setVisible(false);
        actionLayout->addWidget(button);
    }
    detailLayout->addLayout(actionLayout);

    contentLayout->addWidget(tableCard, 3);
    contentLayout->addWidget(detailCard, 2);
    mainLayout->addLayout(contentLayout, 1);

    connect(
        diaryStatusFilter,
        QOverload<int>::of(&QComboBox::currentIndexChanged),
        this,
        &AdministratorMainWindow::refreshDiaryManagementPage);
    connect(
        refreshButton,
        &QPushButton::clicked,
        this,
        &AdministratorMainWindow::refreshDiaryManagementPage);
    connect(
        diaryManagementTable,
        &QTableWidget::itemSelectionChanged,
        this,
        &AdministratorMainWindow::showSelectedDiaryDetail);
    connect(
        diaryApproveButton,
        &QPushButton::clicked,
        this,
        &AdministratorMainWindow::approveSelectedDiary);
    connect(
        diaryRejectButton,
        &QPushButton::clicked,
        this,
        &AdministratorMainWindow::rejectSelectedDiary);
    connect(
        diaryTakeDownButton,
        &QPushButton::clicked,
        this,
        &AdministratorMainWindow::takeDownSelectedDiary);

    refreshDiaryManagementPage();
}

std::string AdministratorMainWindow::selectedDiaryId() const
{
    if (diaryManagementTable == nullptr)
    {
        return {};
    }
    const int row = diaryManagementTable->currentRow();
    QTableWidgetItem *identityItem =
        row >= 0 ? diaryManagementTable->item(row, 0) : nullptr;
    if (identityItem == nullptr)
    {
        return {};
    }
    return identityItem->data(Qt::UserRole).toString().toStdString();
}

void AdministratorMainWindow::refreshDiaryManagementPage()
{
    if (dataManager == nullptr || diaryManagementTable == nullptr ||
        diaryStatusFilter == nullptr)
    {
        return;
    }

    std::optional<DiaryDisplayStatus> status;
    const int selectedStatus = diaryStatusFilter->currentData().toInt();
    if (selectedStatus >= 0)
    {
        status = static_cast<DiaryDisplayStatus>(selectedStatus);
    }
    DiaryService service(*dataManager);
    const std::vector<DiaryModerationView> diaries =
        service.queryModeration(status);

    diaryManagementTable->setRowCount(
        static_cast<int>(diaries.size()));
    diaryManagementEmptyLabel->setVisible(diaries.empty());
    diaryManagementTable->setVisible(!diaries.empty());

    for (int row = 0; row < static_cast<int>(diaries.size()); ++row)
    {
        const DiaryModerationView &diary =
            diaries[static_cast<size_t>(row)];
        const QString title = diary.title.empty()
            ? QStringLiteral("历史日记（无标题）")
            : QString::fromStdString(diary.title);
        const QString student =
            QString::fromStdString(diary.studentName) +
            "（" + QString::fromStdString(diary.studentAccountId) + "）";
        const QStringList values = {
            QString::fromStdString(diary.diaryId),
            student,
            title,
            QString::fromStdString(diary.recordId),
            diaryDisplayStatusText(diary.displayStatus)};
        for (int column = 0; column < values.size(); ++column)
        {
            QTableWidgetItem *item = new QTableWidgetItem(values[column]);
            if (column == 0)
            {
                item->setData(
                    Qt::UserRole,
                    QString::fromStdString(diary.diaryId));
            }
            diaryManagementTable->setItem(row, column, item);
        }
    }

    if (diaries.empty())
    {
        diaryManagementDetail->clear();
        diaryApproveButton->setVisible(false);
        diaryRejectButton->setVisible(false);
        diaryTakeDownButton->setVisible(false);
        return;
    }
    diaryManagementTable->setCurrentCell(0, 0);
    showSelectedDiaryDetail();
}

void AdministratorMainWindow::showSelectedDiaryDetail()
{
    if (dataManager == nullptr || diaryManagementDetail == nullptr)
    {
        return;
    }
    const std::string diaryId = selectedDiaryId();
    if (diaryId.empty())
    {
        diaryManagementDetail->clear();
        diaryApproveButton->setVisible(false);
        diaryRejectButton->setVisible(false);
        diaryTakeDownButton->setVisible(false);
        return;
    }

    DiaryService service(*dataManager);
    const std::vector<DiaryModerationView> diaries =
        service.queryModeration();
    const auto selected = std::find_if(
        diaries.begin(), diaries.end(),
        [&diaryId](const DiaryModerationView &diary)
        {
            return diary.diaryId == diaryId;
        });
    if (selected == diaries.end())
    {
        diaryManagementDetail->setPlainText("所选日记已不存在，请刷新列表。");
        diaryApproveButton->setVisible(false);
        diaryRejectButton->setVisible(false);
        diaryTakeDownButton->setVisible(false);
        return;
    }

    const DiaryModerationView &diary = *selected;
    const QString title = diary.title.empty()
        ? QStringLiteral("历史日记（无标题）")
        : QString::fromStdString(diary.title);
    const QString publishedAt = diary.publishedAt.has_value()
        ? QString::fromStdString(*diary.publishedAt)
        : (diary.displayStatus == DiaryDisplayStatus::Displayed ||
                   diary.displayStatus == DiaryDisplayStatus::TakenDown
               ? QStringLiteral("历史发布时间未保存")
               : QStringLiteral("尚未展示"));
    QStringList lines = {
        "学生：" + QString::fromStdString(diary.studentName) +
            "（" + QString::fromStdString(diary.studentAccountId) + "）",
        "日记编号：" + QString::fromStdString(diary.diaryId),
        "志愿记录编号：" + QString::fromStdString(diary.recordId),
        "标题：" + title,
        "日记内容：" + QString::fromStdString(diary.content),
        "最终类别：" + QString::fromStdString(diary.categoryName),
        "服务日期：" + QString::fromStdString(diary.serviceDate),
        "最终时长：" + QString::number(diary.durationHours, 'f', 1) + " 小时",
        "最终积分：" + QString::number(diary.score, 'f', 1),
        "服务地点：" + QString::fromStdString(diary.place),
        "展示状态：" + diaryDisplayStatusText(diary.displayStatus),
        "展示时间：" + publishedAt};
    diaryManagementDetail->setPlainText(lines.join("\n"));

    const bool pending = diary.displayStatus ==
        DiaryDisplayStatus::PendingDisplayReview;
    const bool displayed = diary.displayStatus ==
        DiaryDisplayStatus::Displayed;
    diaryApproveButton->setVisible(pending);
    diaryRejectButton->setVisible(pending);
    diaryTakeDownButton->setVisible(displayed);
}

void AdministratorMainWindow::approveSelectedDiary()
{
    if (dataManager == nullptr)
    {
        return;
    }
    const std::string diaryId = selectedDiaryId();
    if (diaryId.empty())
    {
        return;
    }
    if (QMessageBox::question(
            this,
            "确认展示",
            "审核通过后，该日记将展示在校园日记墙。确定继续吗？",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No) != QMessageBox::Yes)
    {
        return;
    }

    DiaryService service(*dataManager);
    const DiaryServiceOutcome outcome =
        service.approveDisplay(accountId, diaryId);
    if (!outcome.succeeded())
    {
        showDiaryServiceFailure(this, outcome.status);
        refreshDiaryManagementPage();
        return;
    }
    refreshDiaryManagementPage();
    refreshOperationLogPage();
    QMessageBox::information(this, "审核完成", "日记展示申请已通过。");
}

void AdministratorMainWindow::rejectSelectedDiary()
{
    if (dataManager == nullptr)
    {
        return;
    }
    const std::string diaryId = selectedDiaryId();
    if (diaryId.empty())
    {
        return;
    }
    if (QMessageBox::question(
            this,
            "确认审核未通过",
            "确定将这篇日记标记为审核未通过吗？",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No) != QMessageBox::Yes)
    {
        return;
    }

    DiaryService service(*dataManager);
    const DiaryServiceOutcome outcome =
        service.rejectDisplay(accountId, diaryId);
    if (!outcome.succeeded())
    {
        showDiaryServiceFailure(this, outcome.status);
        refreshDiaryManagementPage();
        return;
    }
    refreshDiaryManagementPage();
    refreshOperationLogPage();
    QMessageBox::information(this, "审核完成", "日记已标记为审核未通过。");
}

void AdministratorMainWindow::takeDownSelectedDiary()
{
    if (dataManager == nullptr)
    {
        return;
    }
    const std::string diaryId = selectedDiaryId();
    if (diaryId.empty())
    {
        return;
    }
    if (QMessageBox::question(
            this,
            "确认下架",
            "下架后，这篇日记将不再出现在日记墙。确定继续吗？",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No) != QMessageBox::Yes)
    {
        return;
    }

    DiaryService service(*dataManager);
    const DiaryServiceOutcome outcome =
        service.takeDown(accountId, diaryId);
    if (!outcome.succeeded())
    {
        showDiaryServiceFailure(this, outcome.status);
        refreshDiaryManagementPage();
        return;
    }
    refreshDiaryManagementPage();
    refreshOperationLogPage();
    QMessageBox::information(this, "日记已下架", "该日记已从日记墙下架。");
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
        const std::string finalCategoryId =
            record.getFinalCategoryId().value_or(
                record.getAppliedCategoryId());
        const double finalDuration = record.getFinalDuration().value_or(
            record.getAppliedDuration());
        const double finalScore = record.getFinalScore().value_or(0.0);
        totalDuration += finalDuration;

        int index = -1;

        if (finalCategoryId == "C01")
        {
            index = 0;
        }
        else if (finalCategoryId == "C02")
        {
            index = 1;
        }
        else if (finalCategoryId == "C03")
        {
            index = 2;
        }

        if (index >= 0)
        {
            ++categoryCount[index];
            categoryDuration[index] += finalDuration;
            categoryScore[index] += finalScore;
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
                    1)));
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
                    1)));
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
