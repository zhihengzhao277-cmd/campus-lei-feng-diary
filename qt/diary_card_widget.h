#ifndef DIARY_CARD_WIDGET_H
#define DIARY_CARD_WIDGET_H

#include <QFrame>

#include "diary_post.h"

class QLabel;
class QPushButton;

class DiaryCardWidget : public QFrame
{
    Q_OBJECT

public:
    explicit DiaryCardWidget(
        DiaryPost *diary,
        const std::string &studentName,
        const std::string &currentStudentId,
        QWidget *parent = nullptr);

signals:

    void likeChanged();

private slots:

    void handleLike();

private:
    DiaryPost *diary;

    std::string currentStudentId;

    QLabel *studentLabel;

    QLabel *contentLabel;

    QLabel *recordLabel;

    QPushButton *likeButton;

    void refreshLikeButton();
};

#endif