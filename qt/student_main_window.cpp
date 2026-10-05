#include "student_main_window.h"
#include "style_helper.h"
#include "badge_icon_mapper.h"

#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QFont>
#include <QTextEdit>
#include <QComboBox>
#include <QDateEdit>
#include <QCalendarWidget>
#include <QDate>
#include <QBrush>
#include <QColor>
#include <QAbstractItemView>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QList>
#include <QPainter>
#include <QPushButton>
#include <QStyle>
#include <QStyleOptionViewItem>
#include <QStyledItemDelegate>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QInputDialog>
#include <QMessageBox>
#include <QScrollArea>
#include <QFrame>
#include <QGraphicsOpacityEffect>
#include <QIcon>
#include <QPixmap>
#include <QProgressBar>
#include <QSize>
#include <QStringList>
#include <cmath>

#include "data_manager.h"
#include "diary_post.h"
#include "student.h"
#include "volunteer_record.h"

namespace
{
    void addRankingTopStudentFields(QVBoxLayout *cardLayout);
    void configureRankingTable(QTableWidget *table);
    void addRankingTopCardContainer(QVBoxLayout *sectionLayout);

    class RecordsNoCellFocusDelegate final :
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

    bool containsInvalidPersistenceCharacter(
        const QString &text)
    {
        return text.contains('|') ||
               text.contains('\n') ||
               text.contains('\r');
    }

    QLabel *createAchievementBadgeIcon(
        const QString &categoryId,
        int level)
    {
        QLabel *iconLabel = new QLabel;
        iconLabel->setObjectName(
            QString("achievementBadgeIcon_%1_%2")
                .arg(categoryId)
                .arg(level));
        iconLabel->setAlignment(Qt::AlignCenter);
        iconLabel->setFixedSize(88, 88);

        const BadgeIconMapper::Level badgeLevel =
            static_cast<BadgeIconMapper::Level>(level);
        const QString resourcePath =
            BadgeIconMapper::resourcePath(
                categoryId.toStdString(),
                badgeLevel);
        iconLabel->setPixmap(
            QIcon(resourcePath).pixmap(QSize(84, 84)));

        QGraphicsOpacityEffect *opacityEffect =
            new QGraphicsOpacityEffect(iconLabel);
        opacityEffect->setOpacity(0.34);
        iconLabel->setGraphicsEffect(opacityEffect);
        return iconLabel;
    }

    QLabel *createAchievementBadgeState(
        const QString &categoryId,
        int level)
    {
        QLabel *stateLabel = new QLabel("未获得");
        stateLabel->setObjectName(
            QString("achievementBadgeState_%1_%2")
                .arg(categoryId)
                .arg(level));
        stateLabel->setProperty("earned", false);
        stateLabel->setAlignment(Qt::AlignCenter);
        return stateLabel;
    }

    QFrame *createAchievementBadgeTile(
        const QString &categoryId,
        int level,
        const QString &levelName)
    {
        QFrame *badgeTile = new QFrame;
        badgeTile->setObjectName("achievementBadgeTile");
        QVBoxLayout *tileLayout = new QVBoxLayout(badgeTile);
        tileLayout->setContentsMargins(6, 8, 6, 8);
        tileLayout->setSpacing(3);
        tileLayout->addWidget(
            createAchievementBadgeIcon(categoryId, level),
            0,
            Qt::AlignCenter);
        QLabel *levelLabel = new QLabel(levelName);
        levelLabel->setObjectName("achievementBadgeLevel");
        levelLabel->setAlignment(Qt::AlignCenter);
        tileLayout->addWidget(levelLabel);
        tileLayout->addWidget(
            createAchievementBadgeState(categoryId, level));
        return badgeTile;
    }

    void addAchievementHeading(
        QVBoxLayout *cardLayout,
        const QString &categoryId,
        const QString &categoryName)
    {
        QHBoxLayout *headingLayout = new QHBoxLayout;
        QLabel *titleLabel = new QLabel(categoryName);
        titleLabel->setObjectName("achievementCategoryTitle");
        headingLayout->addWidget(titleLabel);
        headingLayout->addStretch();

        QLabel *durationLabel = new QLabel;
        durationLabel->setObjectName(
            "achievementDuration_" + categoryId);
        headingLayout->addWidget(durationLabel);
        cardLayout->addLayout(headingLayout);
    }

    void addAchievementProgress(
        QVBoxLayout *cardLayout,
        const QString &categoryId)
    {
        QLabel *currentLevelLabel = new QLabel;
        currentLevelLabel->setObjectName(
            "achievementCurrentLevel_" + categoryId);
        cardLayout->addWidget(currentLevelLabel);
        QHBoxLayout *progressLayout = new QHBoxLayout;
        QProgressBar *progressBar = new QProgressBar;
        progressBar->setObjectName(
            "achievementProgress_" + categoryId);
        progressBar->setRange(0, 100);
        progressBar->setTextVisible(false);
        progressLayout->addWidget(progressBar, 1);

        QLabel *nextGoalLabel = new QLabel;
        nextGoalLabel->setObjectName(
            "achievementNextGoal_" + categoryId);
        progressLayout->addWidget(nextGoalLabel);
        cardLayout->addLayout(progressLayout);
    }

    void addAchievementBadgeRow(
        QVBoxLayout *cardLayout,
        const QString &categoryId)
    {
        QHBoxLayout *badgeLayout = new QHBoxLayout;
        badgeLayout->setSpacing(10);
        const QStringList levelNames = {
            "铜级", "银级", "金级"};
        for (int level = 1; level <= levelNames.size(); ++level)
        {
            badgeLayout->addWidget(
                createAchievementBadgeTile(
                    categoryId,
                    level,
                levelNames.at(level - 1)));
        }
        cardLayout->addLayout(badgeLayout);
    }

    QFrame *createAchievementCategoryCard(
        const QString &categoryId,
        const QString &categoryName)
    {
        QFrame *categoryCard = new QFrame;
        categoryCard->setObjectName("achievementCategoryCard");
        QVBoxLayout *cardLayout = new QVBoxLayout(categoryCard);
        cardLayout->setContentsMargins(18, 16, 18, 16);
        cardLayout->setSpacing(10);
        addAchievementHeading(cardLayout, categoryId, categoryName);
        addAchievementProgress(cardLayout, categoryId);
        addAchievementBadgeRow(cardLayout, categoryId);
        return categoryCard;
    }

    QHBoxLayout *createAchievementHeader(
        QPushButton **refreshButton)
    {
        QHBoxLayout *headerLayout = new QHBoxLayout;
        QVBoxLayout *headingLayout = new QVBoxLayout;
        QLabel *titleLabel = new QLabel("我的徽章");
        titleLabel->setObjectName("achievementTitle");
        headingLayout->addWidget(titleLabel);
        QLabel *subtitleLabel = new QLabel(
            "根据已审核通过的志愿服务时长解锁徽章。");
        subtitleLabel->setObjectName("achievementSubtitle");
        headingLayout->addWidget(subtitleLabel);
        headerLayout->addLayout(headingLayout);
        headerLayout->addStretch();
        *refreshButton = new QPushButton("刷新徽章");
        (*refreshButton)->setObjectName("achievementRefreshButton");
        headerLayout->addWidget(*refreshButton);
        return headerLayout;
    }

    void addAchievementCategoryCards(QVBoxLayout *contentLayout)
    {
        const QStringList categoryIds = {"C01", "C02", "C03"};
        const QStringList categoryNames = {
            "劳动先锋", "环保卫士", "互助之星"};
        for (int index = 0; index < 3; ++index)
        {
            contentLayout->addWidget(
                createAchievementCategoryCard(
                    categoryIds.at(index),
                    categoryNames.at(index)));
        }
        contentLayout->addStretch();
    }

    QWidget *createAchievementContent(
        QPushButton **refreshButton)
    {
        QWidget *contentWidget = new QWidget;
        contentWidget->setObjectName("achievementContent");
        QVBoxLayout *contentLayout =
            new QVBoxLayout(contentWidget);
        contentLayout->setContentsMargins(24, 22, 24, 22);
        contentLayout->setSpacing(16);
        contentLayout->addLayout(
            createAchievementHeader(refreshButton));
        addAchievementCategoryCards(contentLayout);
        return contentWidget;
    }

    QScrollArea *createAchievementScrollArea(
        QPushButton **refreshButton)
    {
        QScrollArea *scrollArea = new QScrollArea;
        scrollArea->setObjectName("achievementScrollArea");
        scrollArea->setWidgetResizable(true);
        scrollArea->setFrameShape(QFrame::NoFrame);
        scrollArea->setWidget(
            createAchievementContent(refreshButton));
        return scrollArea;
    }

    void updateAchievementSummary(
        QWidget *page,
        const QString &categoryKey,
        double duration,
        int earnedLevel,
        const QString &currentBadge)
    {
        QLabel *durationLabel = page->findChild<QLabel *>(
            "achievementDuration_" + categoryKey);
        QLabel *currentLabel = page->findChild<QLabel *>(
            "achievementCurrentLevel_" + categoryKey);
        durationLabel->setText(
            QString("累计已通过服务时长：%1 小时")
                .arg(duration, 0, 'f', 1));
        currentLabel->setText(
            earnedLevel == 0
                ? "当前：暂无徽章"
                : "当前：" + currentBadge);
    }

    void setNextAchievementGoal(
        QProgressBar *progressBar,
        QLabel *nextGoalLabel,
        double duration,
        double target,
        const QString &levelName)
    {
        const double remaining = target - duration;
        const double progressDuration =
            duration < target ? duration : target;
        progressBar->setValue(
            static_cast<int>(progressDuration / target * 100.0));
        nextGoalLabel->setText(
            QString("距离%1还需 %2 小时")
                .arg(levelName)
                .arg(QString::number(remaining, 'f', 1)));
    }

    void updateAchievementProgress(
        QWidget *page,
        const QString &categoryKey,
        double duration,
        int earnedLevel,
        const double *thresholds,
        const QString *levelNames)
    {
        QProgressBar *progressBar = page->findChild<QProgressBar *>(
            "achievementProgress_" + categoryKey);
        QLabel *nextGoalLabel = page->findChild<QLabel *>(
            "achievementNextGoal_" + categoryKey);
        if (earnedLevel >= 3)
        {
            progressBar->setValue(100);
            nextGoalLabel->setText("已达到最高等级");
            return;
        }
        setNextAchievementGoal(
            progressBar,
            nextGoalLabel,
            duration,
            thresholds[earnedLevel],
            levelNames[earnedLevel]);
    }

    void updateAchievementBadgeTile(
        QWidget *page,
        const QString &categoryKey,
        int level,
        bool earned)
    {
        const QString suffix =
            QString("%1_%2").arg(categoryKey).arg(level);
        QLabel *iconLabel = page->findChild<QLabel *>(
            "achievementBadgeIcon_" + suffix);
        QLabel *stateLabel = page->findChild<QLabel *>(
            "achievementBadgeState_" + suffix);
        stateLabel->setText(earned ? "已获得" : "未获得");
        stateLabel->setProperty("earned", earned);
        stateLabel->style()->unpolish(stateLabel);
        stateLabel->style()->polish(stateLabel);

        QGraphicsOpacityEffect *opacityEffect =
            qobject_cast<QGraphicsOpacityEffect *>(
                iconLabel->graphicsEffect());
        opacityEffect->setOpacity(earned ? 1.0 : 0.34);
    }

    void updateAchievementCategory(
        QWidget *page,
        const QString &categoryKey,
        double duration,
        int earnedLevel,
        const QString &currentBadge,
        const double *thresholds,
        const QString *levelNames)
    {
        updateAchievementSummary(
            page, categoryKey, duration, earnedLevel, currentBadge);
        updateAchievementProgress(
            page, categoryKey, duration, earnedLevel,
            thresholds, levelNames);
        for (int level = 1; level <= 3; ++level)
        {
            updateAchievementBadgeTile(
                page, categoryKey, level, level <= earnedLevel);
        }
    }

    QPushButton *createRankingHeader(QVBoxLayout *contentLayout)
    {
        QHBoxLayout *headerLayout = new QHBoxLayout;
        QVBoxLayout *titleLayout = new QVBoxLayout;
        QLabel *title = new QLabel("排行榜");
        title->setObjectName("rankingTitle");
        titleLayout->addWidget(title);
        QLabel *subtitle = new QLabel("按当前累计积分展示志愿服务排名。");
        subtitle->setObjectName("rankingSubtitle");
        titleLayout->addWidget(subtitle);
        headerLayout->addLayout(titleLayout);
        headerLayout->addStretch();
        QPushButton *refreshButton = new QPushButton("刷新排行榜");
        refreshButton->setObjectName("rankingRefreshButton");
        headerLayout->addWidget(refreshButton);
        contentLayout->addLayout(headerLayout);
        return refreshButton;
    }

    QScrollArea *createRankingScrollArea()
    {
        QScrollArea *scrollArea = new QScrollArea;
        scrollArea->setObjectName("rankingScrollArea");
        scrollArea->setWidgetResizable(true);
        scrollArea->setFrameShape(QFrame::NoFrame);
        QWidget *content = new QWidget;
        content->setObjectName("rankingContent");
        QVBoxLayout *contentLayout = new QVBoxLayout(content);
        contentLayout->setContentsMargins(24, 22, 24, 22);
        contentLayout->setSpacing(16);
        scrollArea->setWidget(content);
        return scrollArea;
    }

    QLabel *createRankingStarIcon()
    {
        QLabel *icon = new QLabel;
        icon->setObjectName("rankingStarIcon");
        icon->setFixedSize(40, 40);
        icon->setAlignment(Qt::AlignCenter);
        icon->setPixmap(
            QIcon(BadgeIconMapper::leiFengStarResourcePath())
                .pixmap(QSize(36, 36)));
        icon->setAccessibleName("雷锋之星");
        icon->setToolTip("雷锋之星");
        return icon;
    }

    QFrame *createRankingTopCard(int rank)
    {
        QFrame *card = new QFrame;
        card->setObjectName("rankingTopCard");
        card->setProperty(
            "rankBand",
            rank == 1
                ? QStringLiteral("first")
                : rank == 2
                      ? QStringLiteral("second")
                      : QStringLiteral("third"));
        card->setProperty("currentStudent", false);
        QVBoxLayout *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(14, 12, 14, 12);
        cardLayout->setSpacing(5);
        QLabel *rankLabel = new QLabel;
        rankLabel->setObjectName("rankingTopRank");
        cardLayout->addWidget(rankLabel);
        QHBoxLayout *honorLayout = new QHBoxLayout;
        honorLayout->addWidget(createRankingStarIcon());
        QLabel *starText = new QLabel("雷锋之星");
        starText->setObjectName("rankingStarText");
        honorLayout->addWidget(starText);
        honorLayout->addStretch();
        cardLayout->addLayout(honorLayout);
        addRankingTopStudentFields(cardLayout);
        return card;
    }

    void addRankingTopStudentFields(QVBoxLayout *cardLayout)
    {
        const QString objectNames[] = {
            "rankingTopStudent",
            "rankingTopAccount",
            "rankingTopScore"};
        for (const QString &objectName : objectNames)
        {
            QLabel *field = new QLabel;
            field->setObjectName(objectName);
            cardLayout->addWidget(field);
        }
    }

    void updateRankingTopCard(
        QFrame *card,
        const RankingItem &item,
        int rank,
        const std::string &currentAccountId)
    {
        card->findChild<QLabel *>("rankingTopRank")
            ->setText(QString("第 %1 名").arg(rank));
        card->findChild<QLabel *>("rankingTopStudent")
            ->setText(QString::fromStdString(item.studentName));
        card->findChild<QLabel *>("rankingTopAccount")
            ->setText(QString::fromStdString(item.studentId));
        card->findChild<QLabel *>("rankingTopScore")
            ->setText(QString("积分 %1").arg(item.score, 0, 'f', 2));
        card->setProperty(
            "currentStudent",
            item.studentId == currentAccountId);
        card->style()->unpolish(card);
        card->style()->polish(card);
        card->show();
    }

    void refreshRankingTopCards(
        QWidget *rankingPage,
        const std::vector<RankingItem> &ranking,
        const std::string &currentAccountId)
    {
        QWidget *container = rankingPage->findChild<QWidget *>(
            "rankingTopThreeContainer");
        const QList<QFrame *> cards =
            container->findChildren<QFrame *>(
                "rankingTopCard",
                Qt::FindDirectChildrenOnly);
        for (int rank = 1; rank <= 3; ++rank)
        {
            QFrame *card = cards.at(rank - 1);
            if (rank <= static_cast<int>(ranking.size()))
            {
                updateRankingTopCard(
                    card, ranking[rank - 1], rank, currentAccountId);
            }
            else
            {
                card->hide();
            }
        }
        container->setVisible(!ranking.empty());
        rankingPage->findChild<QLabel *>("rankingEmptyState")
            ->setVisible(ranking.empty());
    }

    void addRankingTableTextItem(
        QTableWidget *table,
        int row,
        int column,
        const QString &text,
        const QString &studentId,
        bool isCurrentStudent)
    {
        QTableWidgetItem *item = new QTableWidgetItem(text);
        item->setData(Qt::UserRole, studentId);
        if (column == 0 || column == 3)
        {
            item->setTextAlignment(Qt::AlignCenter);
        }
        if (isCurrentStudent)
        {
            item->setBackground(QColor("#FCEAED"));
            QFont font = item->font();
            font.setBold(true);
            item->setFont(font);
        }
        table->setItem(row, column, item);
    }

    void addRankingTableSection(
        QVBoxLayout *contentLayout,
        QTableWidget **rankingTable)
    {
        QFrame *tableCard = new QFrame;
        tableCard->setObjectName("rankingTableCard");
        QVBoxLayout *tableLayout = new QVBoxLayout(tableCard);
        tableLayout->setContentsMargins(16, 14, 16, 16);
        tableLayout->setSpacing(10);
        QLabel *title = new QLabel("完整排行榜");
        title->setObjectName("rankingTableTitle");
        tableLayout->addWidget(title);
        *rankingTable = new QTableWidget;
        configureRankingTable(*rankingTable);
        tableLayout->addWidget(*rankingTable);
        contentLayout->addWidget(tableCard);
    }

    void configureRankingTable(QTableWidget *table)
    {
        table->setObjectName("rankingTable");
        table->setItemDelegate(
            new RecordsNoCellFocusDelegate(table));
        table->setColumnCount(5);
        table->setHorizontalHeaderLabels(
            {"排名", "学号", "姓名", "积分", "专项徽章"});
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setSelectionMode(QAbstractItemView::SingleSelection);
        table->setSortingEnabled(false);
        table->setAlternatingRowColors(true);
        table->setShowGrid(false);
        table->verticalHeader()->setVisible(false);
        table->verticalHeader()->setDefaultSectionSize(48);
        table->horizontalHeader()->setSectionResizeMode(
            QHeaderView::Stretch);
        table->setMinimumHeight(240);
    }

    void addRankingTopThreeSection(QVBoxLayout *contentLayout)
    {
        QFrame *section = new QFrame;
        section->setObjectName("rankingTopSection");
        QVBoxLayout *sectionLayout = new QVBoxLayout(section);
        sectionLayout->setContentsMargins(16, 14, 16, 16);
        sectionLayout->setSpacing(10);
        QLabel *title = new QLabel("当前 Top 3 · 雷锋之星");
        title->setObjectName("rankingSectionTitle");
        sectionLayout->addWidget(title);
        QLabel *caption = new QLabel(
            "此荣誉根据当前排行榜前三名动态展示。");
        caption->setObjectName("rankingSectionCaption");
        sectionLayout->addWidget(caption);
        addRankingTopCardContainer(sectionLayout);
        contentLayout->addWidget(section);
    }

    void addRankingTopCardContainer(QVBoxLayout *sectionLayout)
    {
        QLabel *emptyState = new QLabel("暂无排行榜数据");
        emptyState->setObjectName("rankingEmptyState");
        emptyState->setAlignment(Qt::AlignCenter);
        sectionLayout->addWidget(emptyState);
        QWidget *container = new QWidget;
        container->setObjectName("rankingTopThreeContainer");
        QHBoxLayout *cardsLayout = new QHBoxLayout(container);
        cardsLayout->setContentsMargins(0, 0, 0, 0);
        cardsLayout->setSpacing(12);
        for (int rank = 1; rank <= 3; ++rank)
        {
            QFrame *card = createRankingTopCard(rank);
            cardsLayout->addWidget(card, 1);
            card->hide();
        }
        sectionLayout->addWidget(container);
    }

    void addRankingSpecialtyIcon(
        QHBoxLayout *layout,
        const std::string &categoryId,
        int earnedLevel,
        const QString &badgeLabel)
    {
        const BadgeIconMapper::Level level =
            static_cast<BadgeIconMapper::Level>(earnedLevel);
        QLabel *icon = new QLabel;
        icon->setObjectName("rankingSpecialtyBadge");
        icon->setFixedSize(30, 30);
        icon->setAlignment(Qt::AlignCenter);
        icon->setPixmap(
            QIcon(BadgeIconMapper::resourcePath(categoryId, level))
                .pixmap(QSize(26, 26)));
        icon->setAccessibleName(badgeLabel);
        icon->setAccessibleDescription(badgeLabel);
        icon->setToolTip(badgeLabel);
        layout->addWidget(icon);
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
      dashboardGreetingLabel(nullptr),
      dashboardScoreLabel(nullptr),
      dashboardRankLabel(nullptr),
      dashboardApprovedRecordsLabel(nullptr),
      dashboardEmptyBadgesLabel(nullptr),
      recordsPage(nullptr),
      recordsTable(nullptr),
      recordsEmptyLabel(nullptr),
      categoryFilter(nullptr),
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
    homePage->setObjectName("studentDashboard");
    homePage->setStyleSheet(
        StyleHelper::studentDashboard());

    QVBoxLayout *pageLayout = new QVBoxLayout(homePage);
    pageLayout->setContentsMargins(0, 0, 0, 0);

    QScrollArea *scrollArea = new QScrollArea(homePage);
    scrollArea->setObjectName("dashboardScrollArea");
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(
        Qt::ScrollBarAlwaysOff);

    QWidget *content = new QWidget;
    content->setObjectName("dashboardContent");
    QVBoxLayout *layout = new QVBoxLayout(content);
    layout->setContentsMargins(22, 20, 22, 22);
    layout->setSpacing(16);

    QVBoxLayout *headingLayout = new QVBoxLayout;
    headingLayout->setSpacing(4);
    dashboardGreetingLabel = new QLabel("你好");
    dashboardGreetingLabel->setObjectName("dashboardGreeting");
    QLabel *descriptionLabel = new QLabel(
        "查看你的志愿服务进展，继续校园公益行动。");
    descriptionLabel->setObjectName("dashboardDescription");
    headingLayout->addWidget(dashboardGreetingLabel);
    headingLayout->addWidget(descriptionLabel);
    layout->addLayout(headingLayout);

    QHBoxLayout *metricsLayout = new QHBoxLayout;
    metricsLayout->setSpacing(12);
    const auto addMetric = [metricsLayout](
                               const QString &title,
                               const QString &objectName,
                               QLabel **valueLabel)
    {
        QFrame *card = new QFrame;
        card->setObjectName("dashboardStatCard");
        QVBoxLayout *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(16, 13, 16, 14);
        cardLayout->setSpacing(5);
        QLabel *caption = new QLabel(title);
        caption->setObjectName("dashboardStatCaption");
        *valueLabel = new QLabel("—");
        (*valueLabel)->setObjectName(objectName);
        cardLayout->addWidget(caption);
        cardLayout->addWidget(*valueLabel);
        metricsLayout->addWidget(card, 1);
    };

    addMetric("累计积分", "dashboardScoreValue",
              &dashboardScoreLabel);
    addMetric("当前排名", "dashboardRankValue",
              &dashboardRankLabel);
    addMetric("审核通过记录", "dashboardApprovedRecordsValue",
              &dashboardApprovedRecordsLabel);
    layout->addLayout(metricsLayout);

    QFrame *actionsSection = new QFrame;
    actionsSection->setObjectName("dashboardSection");
    QVBoxLayout *actionsLayout = new QVBoxLayout(actionsSection);
    actionsLayout->setContentsMargins(16, 13, 16, 14);
    actionsLayout->setSpacing(10);
    QLabel *actionsTitle = new QLabel("常用入口");
    actionsTitle->setObjectName("dashboardSectionTitle");
    QHBoxLayout *actionsButtons = new QHBoxLayout;
    actionsButtons->setSpacing(8);
    QPushButton *recordsButton = new QPushButton("我的志愿记录");
    recordsButton->setObjectName("dashboardActionButton");
    QPushButton *submitButton = new QPushButton("提交志愿记录");
    submitButton->setObjectName("dashboardPrimaryActionButton");
    QPushButton *rankingButton = new QPushButton("查看排行榜");
    rankingButton->setObjectName("dashboardActionButton");
    for (QPushButton *button :
         {recordsButton, submitButton, rankingButton})
    {
        button->setCursor(Qt::PointingHandCursor);
        button->setMinimumHeight(38);
        actionsButtons->addWidget(button);
    }
    actionsLayout->addWidget(actionsTitle);
    actionsLayout->addLayout(actionsButtons);
    layout->addWidget(actionsSection);

    connect(recordsButton, &QPushButton::clicked,
            this, [this]() { navigationList->setCurrentRow(1); });
    connect(submitButton, &QPushButton::clicked,
            this, [this]() { navigationList->setCurrentRow(2); });
    connect(rankingButton, &QPushButton::clicked,
            this, [this]() { navigationList->setCurrentRow(4); });

    QFrame *badgesSection = new QFrame;
    badgesSection->setObjectName("dashboardSection");
    QVBoxLayout *badgesLayout = new QVBoxLayout(badgesSection);
    badgesLayout->setContentsMargins(16, 13, 16, 14);
    badgesLayout->setSpacing(10);
    QLabel *badgesTitle = new QLabel("我的荣誉");
    badgesTitle->setObjectName("dashboardSectionTitle");
    badgesLayout->addWidget(badgesTitle);

    QHBoxLayout *badgeCardsLayout = new QHBoxLayout;
    badgeCardsLayout->setSpacing(10);
    const QStringList badgeCategories = {"C01", "C02", "C03"};
    for (const QString &categoryId : badgeCategories)
    {
        QFrame *badgeCard = new QFrame;
        badgeCard->setObjectName(
            "dashboardBadgeCard_" + categoryId);
        badgeCard->setProperty("kind", "dashboardBadgeCard");
        QVBoxLayout *badgeCardLayout = new QVBoxLayout(badgeCard);
        badgeCardLayout->setContentsMargins(10, 10, 10, 10);
        badgeCardLayout->setSpacing(5);
        QLabel *iconLabel = new QLabel;
        iconLabel->setObjectName(
            "dashboardBadgeIcon_" + categoryId);
        iconLabel->setAlignment(Qt::AlignCenter);
        iconLabel->setFixedSize(54, 54);
        QLabel *textLabel = new QLabel;
        textLabel->setObjectName(
            "dashboardBadgeText_" + categoryId);
        textLabel->setAlignment(Qt::AlignCenter);
        textLabel->setWordWrap(true);
        badgeCardLayout->addWidget(iconLabel, 0, Qt::AlignHCenter);
        badgeCardLayout->addWidget(textLabel);
        badgeCardsLayout->addWidget(badgeCard, 1);
        badgeCard->hide();
    }
    badgesLayout->addLayout(badgeCardsLayout);

    dashboardEmptyBadgesLabel = new QLabel(
        "完成并通过志愿服务记录后，这里会展示你获得的徽章。");
    dashboardEmptyBadgesLabel->setObjectName("dashboardEmptyBadges");
    dashboardEmptyBadgesLabel->setAlignment(Qt::AlignCenter);
    dashboardEmptyBadgesLabel->setWordWrap(true);
    badgesLayout->addWidget(dashboardEmptyBadgesLabel);
    layout->addWidget(badgesSection);
    layout->addStretch(1);

    scrollArea->setWidget(content);
    pageLayout->addWidget(scrollArea);
}

void StudentMainWindow::refreshDashboard()
{
    if (dataManager == nullptr || homePage == nullptr)
    {
        return;
    }

    Student *student = dataManager->findStudent(accountId);
    if (dashboardGreetingLabel != nullptr)
    {
        const QString name = student == nullptr
                                 ? QStringLiteral("同学")
                                 : QString::fromStdString(student->getName());
        dashboardGreetingLabel->setText("你好，" + name);
    }

    if (dashboardScoreLabel != nullptr)
    {
        dashboardScoreLabel->setText(
            QString::number(
                dataManager->calculateStudentScore(accountId), 'f', 2));
    }

    int approvedRecordCount = 0;
    for (const VolunteerRecord &record : dataManager->getRecords())
    {
        if (record.getStudentId() == accountId &&
            record.getStatus() == RecordStatus::Approved)
        {
            ++approvedRecordCount;
        }
    }
    if (dashboardApprovedRecordsLabel != nullptr)
    {
        dashboardApprovedRecordsLabel->setText(
            QString::number(approvedRecordCount));
    }

    QString rankText = "—";
    const std::vector<RankingItem> ranking =
        dataManager->generateRanking();
    for (size_t i = 0; i < ranking.size(); ++i)
    {
        if (ranking[i].studentId == accountId)
        {
            rankText = QString("第 %1 名")
                           .arg(static_cast<qulonglong>(i + 1));
            break;
        }
    }
    if (dashboardRankLabel != nullptr)
    {
        dashboardRankLabel->setText(rankText);
    }

    const QStringList badgeCategories = {"C01", "C02", "C03"};
    bool hasEarnedBadge = false;
    for (const QString &categoryId : badgeCategories)
    {
        const std::string category = categoryId.toStdString();
        const double duration =
            dataManager->calculateStudentDurationByCategory(
                accountId, category);
        int earnedLevel = 0;
        const QString label =
            badgeText(category, duration, &earnedLevel);
        QFrame *badgeCard = homePage->findChild<QFrame *>(
            "dashboardBadgeCard_" + categoryId);
        QLabel *iconLabel = homePage->findChild<QLabel *>(
            "dashboardBadgeIcon_" + categoryId);
        QLabel *textLabel = homePage->findChild<QLabel *>(
            "dashboardBadgeText_" + categoryId);

        if (badgeCard == nullptr || iconLabel == nullptr ||
            textLabel == nullptr)
        {
            continue;
        }

        if (earnedLevel == 0)
        {
            badgeCard->hide();
            iconLabel->clear();
            textLabel->clear();
            continue;
        }

        const auto level =
            static_cast<BadgeIconMapper::Level>(earnedLevel);
        const QString iconPath =
            BadgeIconMapper::resourcePath(category, level);
        const QIcon icon(iconPath);
        iconLabel->setPixmap(icon.pixmap(QSize(48, 48)));
        textLabel->setText(label);
        badgeCard->show();
        hasEarnedBadge = true;
    }

    if (dashboardEmptyBadgesLabel != nullptr)
    {
        dashboardEmptyBadgesLabel->setVisible(!hasEarnedBadge);
    }
}

void StudentMainWindow::buildRecordsPage()
{
    recordsPage = new QWidget;
    recordsPage->setObjectName("studentRecordsPage");
    recordsPage->setStyleSheet(
        StyleHelper::studentRecordsPage());

    QVBoxLayout *layout =
        new QVBoxLayout(recordsPage);
    layout->setContentsMargins(22, 20, 22, 22);
    layout->setSpacing(16);

    QHBoxLayout *titleLayout =
        new QHBoxLayout;
    titleLayout->setSpacing(16);

    QVBoxLayout *headingLayout =
        new QVBoxLayout;
    headingLayout->setSpacing(4);

    QLabel *titleLabel =
        new QLabel("我的志愿记录");
    titleLabel->setObjectName("studentRecordsTitle");

    QLabel *subtitleLabel = new QLabel(
        "查询个人志愿服务记录与审核状态。");
    subtitleLabel->setObjectName("studentRecordsSubtitle");
    headingLayout->addWidget(titleLabel);
    headingLayout->addWidget(subtitleLabel);

    QPushButton *refreshButton =
        new QPushButton("刷新");
    refreshButton->setObjectName("studentRecordsRefreshButton");
    refreshButton->setCursor(Qt::PointingHandCursor);
    refreshButton->setMinimumHeight(38);

    titleLayout->addLayout(headingLayout, 1);
    titleLayout->addStretch();
    titleLayout->addWidget(refreshButton);
    layout->addLayout(titleLayout);

    QFrame *filterCard = new QFrame;
    filterCard->setObjectName("studentRecordsFilterCard");
    QVBoxLayout *filterCardLayout =
        new QVBoxLayout(filterCard);
    filterCardLayout->setContentsMargins(16, 14, 16, 16);
    filterCardLayout->setSpacing(10);

    QLabel *filterTitle = new QLabel("筛选记录");
    filterTitle->setObjectName("studentRecordsSectionTitle");
    filterCardLayout->addWidget(filterTitle);

    QHBoxLayout *filterLayout =
        new QHBoxLayout;
    filterLayout->setSpacing(10);

    categoryFilter =
        new QComboBox;
    categoryFilter->setObjectName("studentRecordsCategoryFilter");

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

    startDateEdit =
        new QDateEdit;
    startDateEdit->setObjectName("studentRecordsStartDate");

    endDateEdit =
        new QDateEdit;
    endDateEdit->setObjectName("studentRecordsEndDate");

    startDateEdit->setCalendarPopup(true);
    endDateEdit->setCalendarPopup(true);
    startDateEdit->calendarWidget()->setStyleSheet(
        StyleHelper::studentRecordsCalendarPopup());
    endDateEdit->calendarWidget()->setStyleSheet(
        StyleHelper::studentRecordsCalendarPopup());

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
    searchButton->setObjectName("studentRecordsSearchButton");
    searchButton->setCursor(Qt::PointingHandCursor);
    searchButton->setMinimumHeight(38);

    QPushButton *clearButton =
        new QPushButton("清除筛选");
    clearButton->setObjectName("studentRecordsClearButton");
    clearButton->setCursor(Qt::PointingHandCursor);
    clearButton->setMinimumHeight(38);

    const auto addFilterField = [&filterLayout](
                                    const QString &labelText,
                                    QWidget *control,
                                    int stretch)
    {
        QVBoxLayout *fieldLayout = new QVBoxLayout;
        fieldLayout->setSpacing(5);
        QLabel *fieldLabel = new QLabel(labelText);
        fieldLabel->setObjectName("studentRecordsFieldLabel");
        fieldLayout->addWidget(fieldLabel);
        fieldLayout->addWidget(control);
        filterLayout->addLayout(fieldLayout, stretch);
    };

    addFilterField("志愿类别", categoryFilter, 3);
    addFilterField("开始日期", startDateEdit, 2);
    addFilterField("结束日期", endDateEdit, 2);
    filterLayout->addWidget(searchButton, 0, Qt::AlignBottom);
    filterLayout->addWidget(clearButton, 0, Qt::AlignBottom);
    filterCardLayout->addLayout(filterLayout);
    layout->addWidget(filterCard);

    QFrame *tableCard = new QFrame;
    tableCard->setObjectName("studentRecordsTableCard");
    QVBoxLayout *tableCardLayout =
        new QVBoxLayout(tableCard);
    tableCardLayout->setContentsMargins(16, 14, 16, 16);
    tableCardLayout->setSpacing(10);

    QHBoxLayout *tableHeaderLayout = new QHBoxLayout;
    QLabel *tableTitle = new QLabel("记录明细");
    tableTitle->setObjectName("studentRecordsSectionTitle");
    tableHeaderLayout->addWidget(tableTitle);
    tableHeaderLayout->addStretch();

    QPushButton *modifyButton =
        new QPushButton("修改选中记录");
    modifyButton->setObjectName("studentRecordsModifyButton");
    modifyButton->setCursor(Qt::PointingHandCursor);
    modifyButton->setMinimumHeight(38);

    QPushButton *deleteButton =
        new QPushButton("删除选中记录");
    deleteButton->setObjectName("studentRecordsDeleteButton");
    deleteButton->setCursor(Qt::PointingHandCursor);
    deleteButton->setMinimumHeight(38);
    tableHeaderLayout->addWidget(modifyButton);
    tableHeaderLayout->addWidget(deleteButton);
    tableCardLayout->addLayout(tableHeaderLayout);

    recordsTable =
        new QTableWidget;
    recordsTable->setObjectName("studentRecordsTable");
    recordsTable->setItemDelegate(
        new RecordsNoCellFocusDelegate(recordsTable));

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
    recordsTable->verticalHeader()
        ->setDefaultSectionSize(42);

    recordsTable->horizontalHeader()
        ->setSectionResizeMode(
            QHeaderView::Stretch);
    recordsTable->setAlternatingRowColors(true);
    recordsTable->setShowGrid(false);
    recordsTable->setMinimumHeight(230);

    recordsEmptyLabel = new QLabel(
        "当前筛选条件下没有匹配的志愿记录。");
    recordsEmptyLabel->setObjectName("studentRecordsEmptyState");
    recordsEmptyLabel->setAlignment(Qt::AlignCenter);
    recordsEmptyLabel->setWordWrap(true);
    recordsEmptyLabel->setMinimumHeight(230);

    recordsTable->hide();
    tableCardLayout->addWidget(recordsTable, 1);
    tableCardLayout->addWidget(recordsEmptyLabel, 1);
    layout->addWidget(tableCard, 1);

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
        refreshDashboard();
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
        refreshDiaryWall();
        contentStack->setCurrentWidget(
            diaryWallPage);

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
    submitPage->setObjectName("studentSubmitPage");
    submitPage->setStyleSheet(
        StyleHelper::studentRemainingPages());

    QVBoxLayout *mainLayout =
        new QVBoxLayout(submitPage);
    mainLayout->setContentsMargins(28, 24, 28, 24);
    mainLayout->setSpacing(16);

    QLabel *titleLabel =
        new QLabel("提交志愿");
    titleLabel->setObjectName("studentSubmitPageTitle");
    mainLayout->addWidget(titleLabel);

    QLabel *subtitleLabel = new QLabel(
        "填写本次志愿服务信息并提交审核");
    subtitleLabel->setObjectName(
        "studentRemainingPageSubtitle");
    mainLayout->addWidget(subtitleLabel);

    QFrame *formCard = new QFrame;
    formCard->setObjectName("studentSubmitFormCard");
    QVBoxLayout *formCardLayout =
        new QVBoxLayout(formCard);
    formCardLayout->setContentsMargins(24, 22, 24, 24);
    formCardLayout->setSpacing(18);

    QFormLayout *formLayout = new QFormLayout;
    formLayout->setLabelAlignment(
        Qt::AlignLeft | Qt::AlignVCenter);
    formLayout->setHorizontalSpacing(20);
    formLayout->setVerticalSpacing(16);
    formLayout->setFieldGrowthPolicy(
        QFormLayout::AllNonFixedFieldsGrow);

    submitCategoryCombo =
        new QComboBox;
    submitCategoryCombo->setObjectName(
        "studentSubmitCategory");

    submitCategoryCombo->addItem(
        "劳动服务",
        "C01");

    submitCategoryCombo->addItem(
        "环保服务",
        "C02");

    submitCategoryCombo->addItem(
        "互助服务",
        "C03");

    submitDateEdit =
        new QDateEdit;
    submitDateEdit->setObjectName(
        "studentSubmitDate");
    submitDateEdit->setCalendarPopup(true);
    submitDateEdit->calendarWidget()->setStyleSheet(
        StyleHelper::studentRecordsCalendarPopup());

    submitDateEdit->setDisplayFormat(
        "yyyy/MM/dd");

    submitDateEdit->setDate(
        QDate::currentDate());

    submitDurationSpin =
        new QDoubleSpinBox;
    submitDurationSpin->setObjectName(
        "studentSubmitDuration");

    submitDurationSpin->setRange(
        0.0,
        10000.0);

    submitDurationSpin->setDecimals(1);
    submitDurationSpin->setSingleStep(0.5);
    submitDurationSpin->setValue(0.0);
    submitDurationSpin->setSuffix(" 小时");

    submitPlaceEdit =
        new QLineEdit;
    submitPlaceEdit->setObjectName(
        "studentSubmitPlace");

    submitPlaceEdit->setPlaceholderText(
        "请输入服务地点");

    submitWitnessEdit =
        new QLineEdit;
    submitWitnessEdit->setObjectName(
        "studentSubmitWitness");

    submitWitnessEdit->setPlaceholderText(
        "请输入证明人");

    submitDescriptionEdit =
        new QTextEdit;
    submitDescriptionEdit->setObjectName(
        "studentSubmitDescription");

    submitDescriptionEdit->setPlaceholderText(
        "请输入志愿服务内容");

    submitDescriptionEdit->setFixedHeight(120);

    QLabel *categoryLabel = new QLabel("志愿类别");
    categoryLabel->setObjectName(
        "studentSubmitFieldLabel");
    QLabel *dateLabel = new QLabel("服务日期");
    dateLabel->setObjectName(
        "studentSubmitFieldLabel");
    QLabel *durationLabel = new QLabel("服务时长");
    durationLabel->setObjectName(
        "studentSubmitFieldLabel");
    QLabel *placeLabel = new QLabel("服务地点");
    placeLabel->setObjectName(
        "studentSubmitFieldLabel");
    QLabel *witnessLabel = new QLabel("证明人");
    witnessLabel->setObjectName(
        "studentSubmitFieldLabel");
    QLabel *descriptionLabel = new QLabel("服务描述");
    descriptionLabel->setObjectName(
        "studentSubmitFieldLabel");

    formLayout->addRow(categoryLabel, submitCategoryCombo);
    formLayout->addRow(dateLabel, submitDateEdit);
    formLayout->addRow(durationLabel, submitDurationSpin);
    formLayout->addRow(placeLabel, submitPlaceEdit);
    formLayout->addRow(witnessLabel, submitWitnessEdit);
    formLayout->addRow(
        descriptionLabel,
        submitDescriptionEdit);

    formCardLayout->addLayout(formLayout);

    QPushButton *submitButton =
        new QPushButton("提交志愿记录");
    submitButton->setObjectName(
        "studentSubmitActionButton");
    submitButton->setMinimumSize(180, 44);
    submitButton->setCursor(Qt::PointingHandCursor);
    formCardLayout->addWidget(
        submitButton,
        0,
        Qt::AlignRight);

    mainLayout->addWidget(formCard);
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
    scorePage->setObjectName("studentScorePage");
    scorePage->setStyleSheet(
        StyleHelper::studentRemainingPages());

    QVBoxLayout *mainLayout =
        new QVBoxLayout(scorePage);
    mainLayout->setContentsMargins(28, 24, 28, 24);
    mainLayout->setSpacing(16);

    QLabel *titleLabel =
        new QLabel("我的积分");
    titleLabel->setObjectName("studentScorePageTitle");
    mainLayout->addWidget(titleLabel);

    QLabel *subtitleLabel = new QLabel(
        "查看累计积分，并按月份或日期范围查询积分");
    subtitleLabel->setObjectName(
        "studentRemainingPageSubtitle");
    mainLayout->addWidget(subtitleLabel);

    QFrame *totalCard = new QFrame;
    totalCard->setObjectName("studentScoreTotalCard");
    QHBoxLayout *totalLayout =
        new QHBoxLayout(totalCard);
    totalLayout->setContentsMargins(24, 20, 24, 20);
    totalLayout->setSpacing(18);

    QVBoxLayout *totalTextLayout = new QVBoxLayout;
    totalTextLayout->setSpacing(6);
    QLabel *totalTitle = new QLabel("总积分");
    totalTitle->setObjectName("studentScoreMetricLabel");

    totalScoreLabel =
        new QLabel("0.00");
    totalScoreLabel->setObjectName(
        "studentScoreMetricValue");

    QPushButton *refreshButton =
        new QPushButton("刷新积分");
    refreshButton->setObjectName(
        "studentScoreRefreshButton");
    refreshButton->setMinimumHeight(40);
    refreshButton->setCursor(Qt::PointingHandCursor);

    totalTextLayout->addWidget(totalTitle);
    totalTextLayout->addWidget(totalScoreLabel);
    totalLayout->addLayout(totalTextLayout);
    totalLayout->addStretch();
    totalLayout->addWidget(refreshButton);
    mainLayout->addWidget(totalCard);

    QFrame *monthCard = new QFrame;
    monthCard->setObjectName("studentScoreQueryCard");
    QVBoxLayout *monthCardLayout =
        new QVBoxLayout(monthCard);
    monthCardLayout->setContentsMargins(24, 20, 24, 22);
    monthCardLayout->setSpacing(14);

    QLabel *monthTitle = new QLabel("月度积分");
    monthTitle->setObjectName("studentScoreSectionTitle");
    monthCardLayout->addWidget(monthTitle);

    monthDateEdit =
        new QDateEdit;
    monthDateEdit->setObjectName(
        "studentScoreMonthDate");
    monthDateEdit->setCalendarPopup(true);
    monthDateEdit->calendarWidget()->setStyleSheet(
        StyleHelper::studentRecordsCalendarPopup());

    monthDateEdit->setDisplayFormat(
        "yyyy/MM");

    monthDateEdit->setDate(
        QDate::currentDate());

    QPushButton *monthButton =
        new QPushButton("查询月度积分");
    monthButton->setObjectName(
        "studentScoreQueryButton");
    monthButton->setMinimumHeight(40);
    monthButton->setCursor(Qt::PointingHandCursor);

    monthlyScoreLabel =
        new QLabel("尚未查询");
    monthlyScoreLabel->setObjectName(
        "studentScoreResult");

    QFormLayout *monthLayout =
        new QFormLayout;
    monthLayout->setLabelAlignment(
        Qt::AlignLeft | Qt::AlignVCenter);
    monthLayout->setHorizontalSpacing(18);
    monthLayout->setVerticalSpacing(14);

    monthLayout->addRow(
        "查询月份：",
        monthDateEdit);

    QHBoxLayout *monthResultLayout = new QHBoxLayout;
    monthResultLayout->setSpacing(14);
    monthResultLayout->addWidget(monthButton);
    monthResultLayout->addWidget(monthlyScoreLabel);
    monthResultLayout->addStretch();
    monthLayout->addRow(monthResultLayout);

    monthCardLayout->addLayout(monthLayout);
    mainLayout->addWidget(monthCard);

    QLabel *semesterTitle =
        new QLabel("学期积分");
    semesterTitle->setObjectName(
        "studentScoreSectionTitle");

    QFrame *semesterCard = new QFrame;
    semesterCard->setObjectName("studentScoreQueryCard");
    QVBoxLayout *semesterCardLayout =
        new QVBoxLayout(semesterCard);
    semesterCardLayout->setContentsMargins(
        24,
        20,
        24,
        22);
    semesterCardLayout->setSpacing(8);
    semesterCardLayout->addWidget(semesterTitle);

    QLabel *semesterDescription = new QLabel(
        "按当前选择的日期范围查询");
    semesterDescription->setObjectName(
        "studentRemainingPageSubtitle");
    semesterCardLayout->addWidget(semesterDescription);

    semesterStartEdit =
        new QDateEdit;
    semesterStartEdit->setObjectName(
        "studentScoreRangeStartDate");

    semesterEndEdit =
        new QDateEdit;
    semesterEndEdit->setObjectName(
        "studentScoreRangeEndDate");

    semesterStartEdit->setCalendarPopup(true);
    semesterEndEdit->setCalendarPopup(true);
    semesterStartEdit->calendarWidget()->setStyleSheet(
        StyleHelper::studentRecordsCalendarPopup());
    semesterEndEdit->calendarWidget()->setStyleSheet(
        StyleHelper::studentRecordsCalendarPopup());

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
    semesterButton->setObjectName(
        "studentScoreQueryButton");
    semesterButton->setMinimumHeight(40);
    semesterButton->setCursor(Qt::PointingHandCursor);

    semesterScoreLabel =
        new QLabel("尚未查询");
    semesterScoreLabel->setObjectName(
        "studentScoreResult");

    QFormLayout *semesterLayout =
        new QFormLayout;
    semesterLayout->setLabelAlignment(
        Qt::AlignLeft | Qt::AlignVCenter);
    semesterLayout->setHorizontalSpacing(18);
    semesterLayout->setVerticalSpacing(14);

    semesterLayout->addRow(
        "开始日期：",
        semesterStartEdit);

    semesterLayout->addRow(
        "结束日期：",
        semesterEndEdit);

    QHBoxLayout *semesterResultLayout =
        new QHBoxLayout;
    semesterResultLayout->setSpacing(14);
    semesterResultLayout->addWidget(semesterButton);
    semesterResultLayout->addWidget(semesterScoreLabel);
    semesterResultLayout->addStretch();
    semesterLayout->addRow(semesterResultLayout);

    semesterCardLayout->addLayout(semesterLayout);
    mainLayout->addWidget(semesterCard);

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

    connect(
        monthDateEdit,
        &QDateEdit::dateChanged,
        this,
        [this](const QDate &) {
            if (monthlyScoreLabel->text().startsWith("月度积分："))
            {
                monthlyScoreLabel->setText("请点击查询");
            }
        });

    const auto resetDateRangeScoreResult =
        [this](const QDate &) {
            if (semesterScoreLabel->text().startsWith("日期范围积分："))
            {
                semesterScoreLabel->setText("请点击查询");
            }
        };

    connect(
        semesterStartEdit,
        &QDateEdit::dateChanged,
        this,
        resetDateRangeScoreResult);

    connect(
        semesterEndEdit,
        &QDateEdit::dateChanged,
        this,
        resetDateRangeScoreResult);

    refreshScorePage();
}

void StudentMainWindow::buildRankingPage()
{
    rankingPage = new QWidget;
    rankingPage->setObjectName("studentRankingPage");
    rankingPage->setStyleSheet(StyleHelper::studentRankingPage());
    QVBoxLayout *pageLayout = new QVBoxLayout(rankingPage);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    QScrollArea *scrollArea = createRankingScrollArea();
    QWidget *content = scrollArea->widget();
    QVBoxLayout *contentLayout =
        qobject_cast<QVBoxLayout *>(content->layout());
    QPushButton *refreshButton = createRankingHeader(contentLayout);
    addRankingTopThreeSection(contentLayout);
    addRankingTableSection(contentLayout, &rankingTable);
    pageLayout->addWidget(scrollArea);

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
    badgePage->setObjectName("studentAchievementPage");
    badgePage->setStyleSheet(
        StyleHelper::studentAchievementPage());
    QPushButton *refreshButton = nullptr;
    QVBoxLayout *pageLayout = new QVBoxLayout(badgePage);
    pageLayout->setContentsMargins(0, 0, 0, 0);
    pageLayout->addWidget(
        createAchievementScrollArea(&refreshButton));

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
    diaryPage->setObjectName("studentDiaryPage");
    diaryPage->setStyleSheet(
        StyleHelper::studentDiaryPages());

    QVBoxLayout *mainLayout =
        new QVBoxLayout(diaryPage);
    mainLayout->setContentsMargins(28, 24, 28, 24);
    mainLayout->setSpacing(16);

    QLabel *titleLabel =
        new QLabel("发布志愿日记");
    titleLabel->setObjectName("studentDiaryPageTitle");

    mainLayout->addWidget(titleLabel);

    QLabel *tipLabel =
        new QLabel(
            "分享你的志愿服务经历，"
            "记录每一次有意义的行动。");
    tipLabel->setObjectName("studentDiaryPageSubtitle");

    mainLayout->addWidget(tipLabel);

    QFrame *publishFrame =
        new QFrame;
    publishFrame->setObjectName("studentDiaryPublishCard");
    publishFrame->setMaximumWidth(820);

    QVBoxLayout *publishLayout =
        new QVBoxLayout(publishFrame);
    publishLayout->setContentsMargins(24, 22, 24, 22);
    publishLayout->setSpacing(14);

    QLabel *publishTitle =
        new QLabel("分享我的志愿日记");
    publishTitle->setObjectName("studentDiaryCardTitle");

    QLabel *recordTip =
        new QLabel("选择已审核通过的志愿记录");
    recordTip->setObjectName("studentDiaryFieldLabel");

    diaryRecordCombo =
        new QComboBox;
    diaryRecordCombo->setMinimumHeight(42);
    diaryRecordCombo->setObjectName(
        "studentDiaryRecordSelector");

    diaryMessageEdit =
        new QTextEdit;
    diaryMessageEdit->setPlaceholderText(
        "记录这次志愿服务中的故事和感受……");
    diaryMessageEdit->setMinimumHeight(180);
    diaryMessageEdit->setObjectName(
        "studentDiaryMessageEditor");

    QPushButton *publishButton =
        new QPushButton("发布日记");
    publishButton->setMinimumSize(120, 42);
    publishButton->setCursor(
        Qt::PointingHandCursor);
    publishButton->setObjectName(
        "studentDiaryPrimaryButton");

    QLabel *messageLabel =
        new QLabel("日记内容");
    messageLabel->setObjectName("studentDiaryFieldLabel");

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

    QHBoxLayout *cardRow = new QHBoxLayout;
    cardRow->addStretch();
    cardRow->addWidget(publishFrame);
    cardRow->addStretch();
    mainLayout->addLayout(cardRow);

    QPushButton *backToWallButton =
        new QPushButton("← 返回日记墙");
    backToWallButton->setMinimumHeight(40);
    backToWallButton->setCursor(Qt::PointingHandCursor);
    backToWallButton->setObjectName(
        "studentDiarySecondaryButton");
    mainLayout->addWidget(
        backToWallButton,
        0,
        Qt::AlignLeft);

    connect(
        publishButton,
        &QPushButton::clicked,
        this,
        &StudentMainWindow::publishDiary);

    connect(
        backToWallButton,
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
    diaryWallPage->setObjectName("studentDiaryWallPage");
    diaryWallPage->setStyleSheet(
        StyleHelper::studentDiaryPages());

    QVBoxLayout *mainLayout =
        new QVBoxLayout(diaryWallPage);
    mainLayout->setContentsMargins(28, 24, 28, 20);
    mainLayout->setSpacing(16);

    QHBoxLayout *topLayout =
        new QHBoxLayout;

    QVBoxLayout *headingLayout =
        new QVBoxLayout;
    headingLayout->setSpacing(4);

    QLabel *titleLabel =
        new QLabel("校园日记墙");
    titleLabel->setObjectName("studentDiaryPageTitle");

    QLabel *subtitleLabel = new QLabel(
        "记录志愿服务故事，分享校园里的温暖行动。");
    subtitleLabel->setObjectName("studentDiaryPageSubtitle");
    headingLayout->addWidget(titleLabel);
    headingLayout->addWidget(subtitleLabel);

    QPushButton *publishButton =
        new QPushButton("发布日记");
    publishButton->setMinimumHeight(40);
    publishButton->setCursor(Qt::PointingHandCursor);
    publishButton->setObjectName(
        "studentDiaryPrimaryButton");

    QPushButton *refreshButton =
        new QPushButton("刷新");
    refreshButton->setMinimumHeight(40);
    refreshButton->setCursor(Qt::PointingHandCursor);
    refreshButton->setObjectName(
        "studentDiarySecondaryButton");

    topLayout->addLayout(headingLayout, 1);
    topLayout->addWidget(publishButton);
    topLayout->addWidget(refreshButton);
    mainLayout->addLayout(topLayout);

    diaryScrollArea = new QScrollArea;
    diaryScrollArea->setObjectName(
        "studentDiaryScrollArea");
    diaryScrollArea->setWidgetResizable(true);
    diaryScrollArea->setFrameShape(QFrame::NoFrame);

    diaryContainer = new QWidget;
    diaryContainer->setObjectName(
        "studentDiaryFeedContainer");

    diaryFeedLayout =
        new QVBoxLayout(diaryContainer);
    diaryFeedLayout->setContentsMargins(0, 8, 0, 8);
    diaryFeedLayout->setSpacing(14);
    diaryFeedLayout->setAlignment(Qt::AlignTop);

    diaryScrollArea->setWidget(diaryContainer);
    mainLayout->addWidget(diaryScrollArea);

    connect(
        publishButton,
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
    profilePage->setObjectName("studentProfilePage");
    profilePage->setStyleSheet(
        StyleHelper::profilePages());

    QVBoxLayout *mainLayout =
        new QVBoxLayout(profilePage);
    mainLayout->setContentsMargins(28, 24, 28, 24);
    mainLayout->setSpacing(16);

    QLabel *titleLabel =
        new QLabel("个人信息");
    titleLabel->setObjectName("profilePageTitle");

    QLabel *tipLabel =
        new QLabel("查看学生基本资料与账号安全设置");
    tipLabel->setObjectName("profilePageSubtitle");

    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(tipLabel);

    QFrame *identityCard = new QFrame;
    identityCard->setObjectName("profileIdentityCard");

    QVBoxLayout *cardLayout =
        new QVBoxLayout(identityCard);
    cardLayout->setContentsMargins(24, 20, 24, 20);
    cardLayout->setSpacing(16);

    QHBoxLayout *userLayout = new QHBoxLayout;
    QLabel *avatarLabel = new QLabel("志");
    avatarLabel->setObjectName("profileIdentityAvatar");
    avatarLabel->setFixedSize(64, 64);
    avatarLabel->setAlignment(Qt::AlignCenter);

    QVBoxLayout *nameLayout = new QVBoxLayout;
    profileNameLabel = new QLabel;
    profileNameLabel->setObjectName("profileIdentityName");

    profileAccountLabel = new QLabel;
    profileAccountLabel->setObjectName("profileIdentityAccount");

    nameLayout->addWidget(profileNameLabel);
    nameLayout->addWidget(profileAccountLabel);
    userLayout->addWidget(avatarLabel);
    userLayout->addSpacing(14);
    userLayout->addLayout(nameLayout);
    userLayout->addStretch();
    cardLayout->addLayout(userLayout);
    mainLayout->addWidget(identityCard);

    QFrame *profileCard = new QFrame;
    profileCard->setObjectName("profileInfoCard");

    QVBoxLayout *infoCardLayout =
        new QVBoxLayout(profileCard);
    infoCardLayout->setContentsMargins(24, 20, 24, 22);
    infoCardLayout->setSpacing(16);

    QLabel *infoTitle = new QLabel("基本信息");
    infoTitle->setObjectName("profileSectionTitle");
    infoCardLayout->addWidget(infoTitle);

    QFormLayout *infoLayout =
        new QFormLayout;
    infoLayout->setLabelAlignment(
        Qt::AlignLeft | Qt::AlignVCenter);
    infoLayout->setHorizontalSpacing(30);
    infoLayout->setVerticalSpacing(14);

    QLabel *accountTitle =
        new QLabel("学号 / 账号 ID");
    QLabel *nameTitle = new QLabel("姓名");
    QLabel *classTitle = new QLabel("班级");
    QLabel *majorTitle = new QLabel("专业");
    accountTitle->setObjectName("profileFieldLabel");
    nameTitle->setObjectName("profileFieldLabel");
    classTitle->setObjectName("profileFieldLabel");
    majorTitle->setObjectName("profileFieldLabel");

    QLabel *accountDetailLabel = new QLabel;
    QLabel *nameDetailLabel = new QLabel;
    profileClassLabel = new QLabel;
    profileMajorLabel = new QLabel;
    accountDetailLabel->setObjectName(
        "profileAccountDetail");
    nameDetailLabel->setObjectName(
        "profileNameDetail");
    profileClassLabel->setObjectName("profileFieldValue");
    profileMajorLabel->setObjectName("profileFieldValue");

    infoLayout->addRow(accountTitle, accountDetailLabel);
    infoLayout->addRow(nameTitle, nameDetailLabel);
    infoLayout->addRow(classTitle, profileClassLabel);
    infoLayout->addRow(majorTitle, profileMajorLabel);
    infoCardLayout->addLayout(infoLayout);
    mainLayout->addWidget(profileCard);

    QFrame *securityCard = new QFrame;
    securityCard->setObjectName("profileSecurityCard");

    QHBoxLayout *securityLayout =
        new QHBoxLayout(securityCard);
    securityLayout->setContentsMargins(24, 18, 24, 18);
    securityLayout->setSpacing(16);

    QVBoxLayout *securityTextLayout = new QVBoxLayout;
    securityTextLayout->setSpacing(5);
    QLabel *securityTitle = new QLabel("账号安全");
    securityTitle->setObjectName("profileSectionTitle");

    QLabel *securityDescription =
        new QLabel("建议定期修改登录密码，保护账号安全。");
    securityDescription->setObjectName(
        "profileSecurityDescription");

    securityTextLayout->addWidget(securityTitle);
    securityTextLayout->addWidget(securityDescription);

    QPushButton *passwordButton =
        new QPushButton("修改密码");
    passwordButton->setObjectName(
        "profilePasswordButton");
    passwordButton->setMinimumSize(110, 40);
    passwordButton->setCursor(Qt::PointingHandCursor);

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
        recordsTable->item(row, 0)->setTextAlignment(
            Qt::AlignLeft | Qt::AlignVCenter);

        recordsTable->setItem(
            row,
            1,
            new QTableWidgetItem(
                categoryName(
                    record->getCategoryId())));
        recordsTable->item(row, 1)->setTextAlignment(
            Qt::AlignLeft | Qt::AlignVCenter);

        recordsTable->setItem(
            row,
            2,
            new QTableWidgetItem(
                recordDate));
        recordsTable->item(row, 2)->setTextAlignment(
            Qt::AlignCenter);

        QTableWidgetItem *durationItem = new QTableWidgetItem(
            QString::number(
                record->getDuration(),
                'f',
                1) +
            " 小时");
        durationItem->setTextAlignment(
            Qt::AlignRight | Qt::AlignVCenter);
        recordsTable->setItem(
            row,
            3,
            durationItem);

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

        QTableWidgetItem *statusItem =
            new QTableWidgetItem(status);
        QFont statusFont = statusItem->font();
        statusFont.setBold(true);
        statusItem->setFont(statusFont);
        statusItem->setTextAlignment(Qt::AlignCenter);

        if (record->getStatus() == RecordStatus::Pending)
        {
            statusItem->setForeground(
                QBrush(QColor("#B7791F")));
            statusItem->setBackground(
                QBrush(QColor("#FFF8E9")));
        }
        else if (record->getStatus() == RecordStatus::Approved)
        {
            statusItem->setForeground(
                QBrush(QColor("#2F855A")));
            statusItem->setBackground(
                QBrush(QColor("#EAF5EF")));
        }
        else
        {
            statusItem->setForeground(
                QBrush(QColor("#C2413A")));
            statusItem->setBackground(
                QBrush(QColor("#FBECEB")));
        }

        recordsTable->setItem(row, 4, statusItem);

        recordsTable->setItem(
            row,
            5,
            new QTableWidgetItem(
                QString::number(
                    record->getScore(),
                    'f',
                    2)));
        recordsTable->item(row, 5)->setTextAlignment(
            Qt::AlignRight | Qt::AlignVCenter);
    }

    const bool hasRecords = recordsTable->rowCount() > 0;
    recordsTable->setVisible(hasRecords);
    recordsEmptyLabel->setVisible(!hasRecords);
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
            0.0,
            10000.0,
            1,
            &ok,
            Qt::WindowFlags(),
            0.5);

    if (!ok)
    {
        return;
    }

    const double halfHourUnits = newDuration * 2.0;
    if (newDuration <= 0.0 ||
        std::abs(halfHourUnits - std::round(halfHourUnits)) > 1e-9)
    {
        QMessageBox::information(
            this,
            "提示",
            "服务时长必须大于 0，并以 0.5 小时为单位。");

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

    const double halfHourUnits = duration * 2.0;
    if (duration <= 0.0 ||
        std::abs(halfHourUnits - std::round(halfHourUnits)) > 1e-9)
    {
        QMessageBox::information(
            this,
            "提示",
            "服务时长必须大于 0，并以 0.5 小时为单位。");

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

    submitDurationSpin->setValue(0.0);

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
        QString("月度积分：%1").arg(score, 0, 'f', 2));
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
        QString("日期范围积分：%1").arg(score, 0, 'f', 2));
}

void StudentMainWindow::refreshRankingPage()
{
    if (dataManager == nullptr ||
        rankingPage == nullptr ||
        rankingTable == nullptr)
    {
        return;
    }
    const std::vector<RankingItem> ranking =
        dataManager->generateRanking();
    refreshRankingTopCards(rankingPage, ranking, accountId);
    rankingTable->setVisible(!ranking.empty());
    rankingTable->setRowCount(static_cast<int>(ranking.size()));
    for (int row = 0; row < static_cast<int>(ranking.size()); ++row)
    {
        const RankingItem &item = ranking[row];
        addRankingTableRow(
            row,
            row + 1,
            item.studentId,
            item.studentName,
            item.score);
    }
}

void StudentMainWindow::addRankingTableRow(
    int row,
    int rank,
    const std::string &studentId,
    const std::string &studentName,
    double score)
{
    const QString id = QString::fromStdString(studentId);
    const bool isCurrentStudent = studentId == accountId;
    addRankingTableTextItem(
        rankingTable, row, 0, QString::number(rank), id, isCurrentStudent);
    addRankingTableTextItem(
        rankingTable, row, 1, id, id, isCurrentStudent);
    addRankingTableTextItem(
        rankingTable,
        row,
        2,
        QString::fromStdString(studentName) +
            (isCurrentStudent ? "（你）" : ""),
        id,
        isCurrentStudent);
    addRankingTableTextItem(
        rankingTable,
        row,
        3,
        QString::number(score, 'f', 2),
        id,
        isCurrentStudent);
    rankingTable->setCellWidget(
        row,
        4,
        createRankingSpecialtyBadgeStrip(studentId));
}

QWidget *StudentMainWindow::createRankingSpecialtyBadgeStrip(
    const std::string &studentId) const
{
    QWidget *strip = new QWidget;
    strip->setObjectName("rankingSpecialtyBadgeStrip");
    strip->setProperty("currentStudent", studentId == accountId);
    QHBoxLayout *layout = new QHBoxLayout(strip);
    layout->setContentsMargins(6, 0, 6, 0);
    layout->setSpacing(6);
    for (const std::string &categoryId : {"C01", "C02", "C03"})
    {
        appendRankingSpecialtyBadge(layout, studentId, categoryId);
    }
    if (layout->count() == 0)
    {
        QLabel *emptyLabel = new QLabel("暂无专项徽章");
        emptyLabel->setObjectName("rankingNoBadgeState");
        layout->addWidget(emptyLabel);
    }
    return strip;
}

void StudentMainWindow::appendRankingSpecialtyBadge(
    QHBoxLayout *layout,
    const std::string &studentId,
    const std::string &categoryId) const
{
    const double duration =
        dataManager->calculateStudentDurationByCategory(
            studentId, categoryId);
    int earnedLevel = 0;
    const QString label = badgeText(
        categoryId, duration, &earnedLevel);
    if (earnedLevel > 0)
    {
        addRankingSpecialtyIcon(
            layout, categoryId, earnedLevel, label);
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
                "暂时还没有志愿日记。\n"
                "分享一次志愿行动，让校园里的温暖被看见。");

        emptyLabel->setObjectName(
            "studentDiaryEmptyState");
        emptyLabel->setAlignment(Qt::AlignCenter);
        emptyLabel->setMinimumHeight(150);
        emptyLabel->setMinimumWidth(560);
        emptyLabel->setMaximumWidth(780);
        diaryFeedLayout->addWidget(
            emptyLabel,
            0,
            Qt::AlignHCenter);
        diaryFeedLayout->addStretch();

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
        card->setObjectName("studentDiaryPostCard");
        card->setFrameShape(QFrame::NoFrame);
        card->setMinimumWidth(560);
        card->setMaximumWidth(780);

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
        avatarLabel->setObjectName("studentDiaryAvatar");
        avatarLabel->setFixedSize(38, 38);
        avatarLabel->setAlignment(
            Qt::AlignCenter);

        QString authorName =
            "未知学生";

        if (author != nullptr)
        {
            authorName =
                QString::fromStdString(
                    author->getName());
        }

        QVBoxLayout *authorTextLayout =
            new QVBoxLayout;
        authorTextLayout->setSpacing(2);

        QLabel *authorLabel =
            new QLabel(authorName);
        authorLabel->setObjectName(
            "studentDiaryAuthorName");

        QLabel *authorIdLabel =
            new QLabel(QString::fromStdString(
                diary.getStudentId()));
        authorIdLabel->setObjectName(
            "studentDiaryAuthorId");
        authorTextLayout->addWidget(authorLabel);
        authorTextLayout->addWidget(authorIdLabel);

        authorLayout->addWidget(
            avatarLabel);

        authorLayout->addLayout(authorTextLayout);

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
            recordLabel->setObjectName(
                "studentDiaryFact");
            cardLayout->addWidget(recordLabel);

            QLabel *placeLabel =
                new QLabel(
                    "地点：" +
                    QString::fromStdString(
                        record->getPlace()));
            placeLabel->setObjectName(
                "studentDiaryFact");
            cardLayout->addWidget(placeLabel);
        }

        QLabel *messageLabel =
            new QLabel(
                QString::fromStdString(
                    diary.getMessage()));
        messageLabel->setObjectName(
            "studentDiaryMessage");
        messageLabel->setWordWrap(true);
        messageLabel->setTextInteractionFlags(
            Qt::TextSelectableByMouse);
        cardLayout->addWidget(messageLabel);

        QHBoxLayout *bottomLayout =
            new QHBoxLayout;

        bottomLayout->addStretch();

        bool alreadyLiked =
            diary.hasLiked(accountId);

        QPushButton *likeButton =
            new QPushButton;
        likeButton->setObjectName(
            "studentDiaryLikeButton");
        likeButton->setFixedSize(34, 34);
        likeButton->setCursor(Qt::PointingHandCursor);
        likeButton->setFlat(true);
        likeButton->setAccessibleName(
            alreadyLiked ? "已点赞" : "点赞");
        likeButton->setProperty(
            "liked",
            alreadyLiked);

        QLabel *likeCountLabel =
            new QLabel(
                QString::number(
                    diary.getLikeCount()));
        likeCountLabel->setObjectName(
            "studentDiaryLikeCount");
        likeCountLabel->setProperty(
            "liked",
            alreadyLiked);

        likeButton->setText(
            alreadyLiked ? "♥" : "♡");

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
        diaryFeedLayout->addWidget(
            card,
            0,
            Qt::AlignHCenter);
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
        "学号：" + studentAccount);
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
    double duration,
    int *earnedLevel) const
{
    if (earnedLevel != nullptr)
    {
        *earnedLevel = 0;
    }

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
        if (earnedLevel != nullptr) *earnedLevel = 3;
        return "金级 · " + badgeName;
    }

    if (duration >= 30.0)
    {
        if (earnedLevel != nullptr) *earnedLevel = 2;
        return "银级 · " + badgeName;
    }

    if (duration >= 10.0)
    {
        if (earnedLevel != nullptr) *earnedLevel = 1;
        return "铜级 · " + badgeName;
    }

    return "暂无徽章";
}

void StudentMainWindow::refreshBadgePage()
{
    if (dataManager == nullptr || badgePage == nullptr)
    {
        return;
    }

    const char *categoryIds[] = {"C01", "C02", "C03"};
    const double thresholds[] = {10.0, 30.0, 60.0};
    const QString levelNames[] = {"铜级", "银级", "金级"};
    for (const char *categoryId : categoryIds)
    {
        refreshAchievementCategory(
            categoryId, thresholds, levelNames);
    }
}

void StudentMainWindow::refreshAchievementCategory(
    const char *categoryId,
    const double *thresholds,
    const QString *levelNames)
{
    const std::string category(categoryId);
    const QString categoryKey = QString::fromLatin1(categoryId);
    const double duration =
        dataManager->calculateStudentDurationByCategory(
            accountId,
            category);
    int earnedLevel = 0;
    const QString currentBadge =
        badgeText(category, duration, &earnedLevel);
    updateAchievementCategory(
        badgePage,
        categoryKey,
        duration,
        earnedLevel,
        currentBadge,
        thresholds,
        levelNames);
}
