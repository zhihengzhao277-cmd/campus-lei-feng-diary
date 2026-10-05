#ifndef BADGE_ICON_MAPPER_H
#define BADGE_ICON_MAPPER_H

#include <QString>

#include <string>

class BadgeIconMapper
{
public:
    enum class Level
    {
        None = 0,
        Bronze = 1,
        Silver = 2,
        Gold = 3
    };

    static QString resourcePath(
        const std::string &categoryId,
        Level level)
    {
        if (categoryId == "C01")
        {
            return levelPath("C01", level);
        }
        if (categoryId == "C02")
        {
            return levelPath("C02", level);
        }
        if (categoryId == "C03")
        {
            return levelPath("C03", level);
        }
        return QString();
    }

private:
    static QString levelPath(
        const char *category,
        Level level)
    {
        const char *levelName = nullptr;
        switch (level)
        {
        case Level::Bronze:
            levelName = "bronze";
            break;
        case Level::Silver:
            levelName = "silver";
            break;
        case Level::Gold:
            levelName = "gold";
            break;
        case Level::None:
            return QString();
        }
        return QString(":/badges/%1/%2.svg")
            .arg(QString::fromLatin1(category),
                 QString::fromLatin1(levelName));
    }
};

#endif
