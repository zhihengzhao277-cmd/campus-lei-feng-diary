#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QMessageBox>

#include <cstdlib>
#include <filesystem>

#include "login_window.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    const QString dataDirectory =
        QDir(QCoreApplication::applicationDirPath())
            .filePath("data");
    const std::filesystem::path dataRoot(
        dataDirectory.toStdWString());

    LoginWindow window(dataRoot);
    if (!window.dataLoaded())
    {
        QMessageBox::critical(
            nullptr,
            "启动失败",
            QString(
                "无法加载运行数据目录：\n%1\n\n"
                "请确认目录中包含并可读取 students.txt、"
                "administrators.txt、records.txt 和 diaries.txt。")
                .arg(QDir::toNativeSeparators(dataDirectory)));
        return EXIT_FAILURE;
    }

    window.show();

    return app.exec();
}
