#include <QApplication>

#include "login_window.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    LoginWindow window;
    window.show();

    return app.exec();
}