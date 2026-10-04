#include "diary_card_widget.h"

#include "style_helper.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

DiaryCardWidget::
    DiaryCardWidget(
        DiaryPost *diary,
        const std::string &studentName,
        const std::string &currentStudentId,
        QWidget *parent)
    : QFrame(parent),
      diary(diary),
      currentStudentId(currentStudentId)
{

    setStyleSheet(
        StyleHelper::card());

    QVBoxLayout *mainLayout =
        new QVBoxLayout(this);

    mainLayout->setContentsMargins(
        20,
        18,
        20,
        18);

    mainLayout->setSpacing(12);

    // 顶部用户信息

    studentLabel =
        new QLabel;

    studentLabel->setText(
        QString::fromStdString(
            studentName));

    QFont font =
        studentLabel->font();

    font.setBold(true);

    font.setPointSize(15);

    studentLabel->setFont(font);

    mainLayout->addWidget(
        studentLabel);

    // 正文

    contentLabel =
        new QLabel;

    contentLabel->setText(
        QString::fromStdString(
            diary->getMessage()));

    contentLabel->setWordWrap(true);

    contentLabel->setStyleSheet(
        "color:#333333;"
        "font-size:15px;"
        "background:transparent;");

    mainLayout->addWidget(
        contentLabel);

    // 志愿信息

    recordLabel =
        new QLabel;

    recordLabel->setText(
        "关联志愿记录：" +
        QString::fromStdString(
            diary->getRecordId()));

    recordLabel->setStyleSheet(
        "color:#888888;"
        "background:transparent;");

    mainLayout->addWidget(
        recordLabel);

    // 点赞

    QHBoxLayout *bottom =
        new QHBoxLayout;

    bottom->addStretch();

    likeButton =
        new QPushButton;

    likeButton->setCursor(
        Qt::PointingHandCursor);

    likeButton->setStyleSheet(
        "QPushButton{"
        "border:none;"
        "font-size:18px;"
        "background:transparent;"
        "}");

    bottom->addWidget(
        likeButton);

    mainLayout->addLayout(
        bottom);

    connect(
        likeButton,
        &QPushButton::clicked,
        this,
        &DiaryCardWidget::
            handleLike);

    refreshLikeButton();
}

void DiaryCardWidget::
    refreshLikeButton()
{

    bool liked =
        diary->hasLiked(
            currentStudentId);

    QString icon;

    if (liked)
    {
        icon = "♥ ";
    }
    else
    {
        icon = "♡ ";
    }

    likeButton->setText(
        icon +
        QString::number(
            diary->getLikeCount()));

    if (liked)
    {
        likeButton->setStyleSheet(
            "QPushButton{"
            "color:#B91C3A;"
            "font-size:18px;"
            "border:none;"
            "background:transparent;"
            "}");
    }
    else
    {
        likeButton->setStyleSheet(
            "QPushButton{"
            "color:#555555;"
            "font-size:18px;"
            "border:none;"
            "background:transparent;"
            "}");
    }
}

void DiaryCardWidget::
    handleLike()
{

    diary->addLike(
        currentStudentId);

    refreshLikeButton();

    emit likeChanged();
}