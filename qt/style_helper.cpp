#include "style_helper.h"

QString StyleHelper::pageBackground()
{
    return "QWidget {"
           "background-color:#F7F7F7;"
           "}";
}

QString StyleHelper::card()
{
    return "QFrame {"
           "background-color:white;"
           "border:1px solid #EAEAEA;"
           "border-radius:16px;"
           "}";
}

QString StyleHelper::primaryButton()
{
    return "QPushButton {"
           "background-color:#B91C3A;"
           "color:white;"
           "border:none;"
           "border-radius:10px;"
           "font-weight:bold;"
           "padding:8px 18px;"
           "}"
           "QPushButton:hover {"
           "background-color:#991B32;"
           "}";
}

QString StyleHelper::secondaryButton()
{
    return "QPushButton {"
           "background-color:white;"
           "color:#555555;"
           "border:1px solid #DDDDDD;"
           "border-radius:10px;"
           "padding:8px 18px;"
           "}"
           "QPushButton:hover {"
           "background-color:#F5F5F5;"
           "}";
}

QString StyleHelper::input()
{
    return "QLineEdit,"
           "QTextEdit,"
           "QComboBox {"
           "background:white;"
           "border:1px solid #DDDDDD;"
           "border-radius:10px;"
           "padding:8px 12px;"
           "font-size:14px;"
           "}"
           "QLineEdit:focus,"
           "QTextEdit:focus,"
           "QComboBox:focus {"
           "border:1px solid #B91C3A;"
           "}";
}

QString StyleHelper::table()
{
    return "QTableWidget {"
           "background:white;"
           "border:1px solid #EAEAEA;"
           "border-radius:12px;"
           "gridline-color:#EEEEEE;"
           "}"
           "QTableWidget::item {"
           "padding:7px;"
           "}"
           "QTableWidget::item:selected {"
           "background-color:#FFF1F3;"
           "color:#333333;"
           "}"
           "QHeaderView::section {"
           "background:#FAFAFA;"
           "border:none;"
           "border-bottom:1px solid #EEEEEE;"
           "padding:8px;"
           "font-weight:bold;"
           "}";
}

QString StyleHelper::navigation()
{
    return "QListWidget {"
           "background:white;"
           "border:1px solid #EAEAEA;"
           "border-radius:14px;"
           "padding:8px;"
           "outline:none;"
           "}"
           "QListWidget::item {"
           "height:44px;"
           "padding-left:12px;"
           "border-radius:9px;"
           "margin:2px;"
           "}"
           "QListWidget::item:selected {"
           "background-color:#FFF1F3;"
           "color:#B91C3A;"
           "font-weight:bold;"
           "}"
           "QListWidget::item:hover {"
           "background-color:#F7F7F7;"
           "}";
}

QString StyleHelper::title()
{
    return "QLabel {"
           "color:#222222;"
           "font-size:20px;"
           "font-weight:bold;"
           "background:transparent;"
           "}";
}

QString StyleHelper::subtitle()
{
    return "QLabel {"
           "color:#888888;"
           "font-size:14px;"
           "background:transparent;"
           "}";
}