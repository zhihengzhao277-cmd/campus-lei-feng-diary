#ifndef LOGIN_WINDOW_H
#define LOGIN_WINDOW_H

#include <filesystem>

#include <QWidget>

#include "data_manager.h"

class QLineEdit;
class QPushButton;
class QLabel;
class QFrame;

class StudentMainWindow;
class AdministratorMainWindow;

class LoginWindow : public QWidget
{
    Q_OBJECT

private:
    QLineEdit *accountEdit;
    QLineEdit *passwordEdit;
    QPushButton *loginButton;
    QLabel *messageLabel;
    QLabel *systemTitleLabel;
    QLabel *systemSubtitleLabel;
    QFrame *loginCard;

    DataManager data;
    bool dataLoaded_ = false;

    StudentMainWindow *studentMainWindow = nullptr;

    AdministratorMainWindow *administratorMainWindow =
        nullptr;

public:
    explicit LoginWindow(
        std::filesystem::path dataRoot,
        QWidget *parent = nullptr);

    bool dataLoaded() const;

private slots:
    void handleLogin();

    void handleStudentLogout();

    void handleAdministratorLogout();
};

#endif
