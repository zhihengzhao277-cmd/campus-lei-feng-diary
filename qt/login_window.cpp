#include "login_window.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include "administrator.h"
#include "administrator_main_window.h"
#include "student.h"
#include "student_main_window.h"
#include "style_helper.h"

LoginWindow::LoginWindow(
    std::filesystem::path dataRoot,
    QWidget *parent)
    : QWidget(parent),
      accountEdit(nullptr),
      passwordEdit(nullptr),
      loginButton(nullptr),
      messageLabel(nullptr),
      systemTitleLabel(nullptr),
      systemSubtitleLabel(nullptr),
      loginCard(nullptr),
      data(dataRoot)
{
    dataLoaded_ = data.loadAll();

    setWindowTitle("校园雷锋日记 - 登录");
    setMinimumSize(800, 520);
    resize(880, 560);
    setObjectName("loginWindow");

    setStyleSheet(
        StyleHelper::loginScreen());

    QFrame *brandPanel =
        new QFrame;

    brandPanel->setObjectName(
        "loginBrandPanel");

    brandPanel->setMinimumWidth(300);
    brandPanel->setMaximumWidth(340);

    QVBoxLayout *brandLayout =
        new QVBoxLayout(brandPanel);

    brandLayout->setContentsMargins(
        34,
        38,
        34,
        30);

    brandLayout->setSpacing(16);

    QLabel *brandEyebrow =
        new QLabel("CAMPUS VOLUNTEER SERVICE");

    brandEyebrow->setObjectName(
        "brandEyebrow");

    systemTitleLabel =
        new QLabel("校园雷锋日记");

    systemTitleLabel->setObjectName(
        "brandTitle");

    systemTitleLabel->setWordWrap(true);

    systemSubtitleLabel =
        new QLabel("记录志愿服务，传递校园善意");

    systemSubtitleLabel->setObjectName(
        "brandSubtitle");

    systemSubtitleLabel->setWordWrap(true);

    QLabel *brandFooter =
        new QLabel("学生 · 管理员统一登录");

    brandFooter->setObjectName(
        "brandFooter");

    brandLayout->addWidget(brandEyebrow);
    brandLayout->addSpacing(8);
    brandLayout->addWidget(systemTitleLabel);
    brandLayout->addWidget(systemSubtitleLabel);
    brandLayout->addStretch();
    brandLayout->addWidget(brandFooter);

    loginCard = new QFrame;
    loginCard->setObjectName("loginCard");
    loginCard->setMinimumWidth(380);
    loginCard->setMaximumWidth(440);

    QVBoxLayout *cardLayout =
        new QVBoxLayout(loginCard);

    cardLayout->setContentsMargins(
        34,
        32,
        34,
        28);

    cardLayout->setSpacing(12);

    QLabel *formTitle =
        new QLabel("欢迎登录");

    formTitle->setObjectName(
        "loginHeading");

    QLabel *formSubtitle =
        new QLabel("请输入校园账号与密码");

    formSubtitle->setObjectName(
        "loginSubheading");

    QLabel *accountLabel =
        new QLabel("账号 ID");

    accountLabel->setObjectName(
        "accountLabel");

    accountEdit =
        new QLineEdit;

    accountEdit->setObjectName(
        "accountEdit");

    passwordEdit =
        new QLineEdit;

    passwordEdit->setObjectName(
        "passwordEdit");

    QLabel *passwordLabel =
        new QLabel("密码");

    passwordLabel->setObjectName(
        "passwordLabel");

    accountLabel->setBuddy(accountEdit);
    passwordLabel->setBuddy(passwordEdit);

    loginButton =
        new QPushButton("登录");

    loginButton->setObjectName(
        "loginButton");

    messageLabel = new QLabel;
    messageLabel->setObjectName(
        "loginMessage");

    accountEdit->setPlaceholderText(
        "请输入账号");

    passwordEdit->setPlaceholderText(
        "请输入密码");

    passwordEdit->setEchoMode(
        QLineEdit::Password);

    accountEdit->setMinimumHeight(46);
    passwordEdit->setMinimumHeight(46);
    loginButton->setMinimumHeight(48);

    QLabel *tipLabel =
        new QLabel(
            "学生与管理员均可使用账号登录");

    tipLabel->setObjectName("loginHint");

    messageLabel->setMinimumHeight(22);
    messageLabel->setAlignment(
        Qt::AlignLeft | Qt::AlignVCenter);

    cardLayout->addWidget(formTitle);
    cardLayout->addWidget(formSubtitle);
    cardLayout->addSpacing(10);
    cardLayout->addWidget(accountLabel);
    cardLayout->addWidget(accountEdit);
    cardLayout->addWidget(passwordLabel);
    cardLayout->addWidget(passwordEdit);
    cardLayout->addWidget(messageLabel);
    cardLayout->addSpacing(4);
    cardLayout->addWidget(loginButton);
    cardLayout->addWidget(
        tipLabel,
        0,
        Qt::AlignHCenter);

    QHBoxLayout *layout =
        new QHBoxLayout(this);

    layout->setContentsMargins(
        30,
        28,
        30,
        28);

    layout->setSpacing(20);
    layout->addStretch();
    layout->addWidget(
        brandPanel,
        0,
        Qt::AlignVCenter);
    layout->addWidget(
        loginCard,
        0,
        Qt::AlignVCenter);
    layout->addStretch();

    connect(
        loginButton,
        &QPushButton::clicked,
        this,
        &LoginWindow::handleLogin);
}

bool LoginWindow::dataLoaded() const
{
    return dataLoaded_;
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
