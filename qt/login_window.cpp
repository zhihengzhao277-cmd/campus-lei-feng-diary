#include "login_window.h"

#include <QFrame>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include "administrator.h"
#include "administrator_main_window.h"
#include "student.h"
#include "student_main_window.h"
#include "style_helper.h"

LoginWindow::LoginWindow(QWidget *parent)
    : QWidget(parent),
      accountEdit(nullptr),
      passwordEdit(nullptr),
      loginButton(nullptr),
      messageLabel(nullptr),
      systemTitleLabel(nullptr),
      systemSubtitleLabel(nullptr),
      loginCard(nullptr)
{
    data.loadAll();

    setWindowTitle("校园雷锋日记 - 登录");
    resize(500, 520);

    setStyleSheet(
        StyleHelper::pageBackground());

    systemTitleLabel =
        new QLabel("校园雷锋日记");

    QFont titleFont =
        systemTitleLabel->font();

    titleFont.setPointSize(30);
    titleFont.setBold(true);

    systemTitleLabel->setFont(titleFont);
    systemTitleLabel->setAlignment(Qt::AlignCenter);
    systemTitleLabel->setStyleSheet(
        "color:#B91C3A;"
        "background:transparent;");

    systemSubtitleLabel =
        new QLabel("记录善行  传播温暖");

    systemSubtitleLabel->setAlignment(
        Qt::AlignCenter);

    systemSubtitleLabel->setStyleSheet(
        "color:#888888;"
        "font-size:16px;"
        "background:transparent;");

    loginCard = new QFrame;
    loginCard->setMaximumWidth(380);
    loginCard->setStyleSheet(
        StyleHelper::card());

    QVBoxLayout *cardLayout =
        new QVBoxLayout(loginCard);

    cardLayout->setContentsMargins(
        35,
        35,
        35,
        35);

    cardLayout->setSpacing(18);

    accountEdit =
        new QLineEdit;

    passwordEdit =
        new QLineEdit;

    loginButton =
        new QPushButton("登录");

    messageLabel =
        new QLabel;

    accountEdit->setPlaceholderText(
        "请输入账号");

    passwordEdit->setPlaceholderText(
        "请输入密码");

    passwordEdit->setEchoMode(
        QLineEdit::Password);

    accountEdit->setStyleSheet(
        StyleHelper::input());

    passwordEdit->setStyleSheet(
        StyleHelper::input());

    accountEdit->setMinimumHeight(42);
    passwordEdit->setMinimumHeight(42);

    loginButton->setStyleSheet(
        StyleHelper::primaryButton());

    loginButton->setMinimumHeight(45);

    QLabel *tipLabel =
        new QLabel(
            "学生与管理员均可使用账号登录");

    tipLabel->setAlignment(
        Qt::AlignCenter);

    tipLabel->setStyleSheet(
        "color:#999999;"
        "font-size:13px;");

    cardLayout->addWidget(accountEdit);
    cardLayout->addWidget(passwordEdit);
    cardLayout->addWidget(loginButton);
    cardLayout->addWidget(tipLabel);

    messageLabel =
        new QLabel;

    messageLabel->setAlignment(
        Qt::AlignCenter);

    messageLabel->setStyleSheet(
        "color:#B91C3A;"
        "background:transparent;");

    QVBoxLayout *layout =
        new QVBoxLayout(this);

    layout->setContentsMargins(
        24,
        32,
        24,
        32);

    layout->setSpacing(10);
    layout->addStretch();
    layout->addWidget(systemTitleLabel);
    layout->addWidget(systemSubtitleLabel);
    layout->addSpacing(18);
    layout->addWidget(
        loginCard,
        0,
        Qt::AlignHCenter);
    layout->addWidget(messageLabel);
    layout->addStretch();

    connect(
        loginButton,
        &QPushButton::clicked,
        this,
        &LoginWindow::handleLogin);
}

void LoginWindow::handleLogin()
{
    std::string accountId =
        accountEdit->text().toStdString();

    std::string password =
        passwordEdit->text().toStdString();

    Student *student =
        data.findStudent(accountId);

    if (student != nullptr &&
        student->checkPassword(password))
    {
        studentMainWindow =
            new StudentMainWindow(
                accountId,
                &data);

        connect(
            studentMainWindow,
            &StudentMainWindow::logoutRequested,
            this,
            &LoginWindow::handleStudentLogout);

        studentMainWindow->show();

        hide();

        return;
    }

    Administrator *administrator =
        data.findAdministrator(accountId);

    if (administrator != nullptr &&
        administrator->checkPassword(password))
    {
        administratorMainWindow =
            new AdministratorMainWindow(
                accountId,
                &data);

        connect(
            administratorMainWindow,
            &AdministratorMainWindow::logoutRequested,
            this,
            &LoginWindow::handleAdministratorLogout);

        administratorMainWindow->show();

        hide();

        return;
    }

    messageLabel->setText(
        "账号或密码错误");
}

void LoginWindow::handleStudentLogout()
{
    if (studentMainWindow != nullptr)
    {
        studentMainWindow->close();
        studentMainWindow->deleteLater();
        studentMainWindow = nullptr;
    }

    accountEdit->clear();
    passwordEdit->clear();
    messageLabel->clear();

    show();
}

void LoginWindow::handleAdministratorLogout()
{
    if (administratorMainWindow != nullptr)
    {
        administratorMainWindow->close();
        administratorMainWindow->deleteLater();
        administratorMainWindow = nullptr;
    }

    accountEdit->clear();
    passwordEdit->clear();
    messageLabel->clear();

    show();
}