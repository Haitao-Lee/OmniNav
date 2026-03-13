#include "BaseModule.h"
#include <QMessageBox>
#include <QCloseEvent>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>
#include <QFile>
#include <QCoreApplication>
#include <QRegularExpression> 

BaseModule::BaseModule(QWidget* parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
    loadConfig();
    init();
    createActions();
}

BaseModule::~BaseModule()
{
}

void BaseModule::loadConfig()
{
    QString path = QCoreApplication::applicationDirPath() + "/BaseModule_config.json";
    QFile file(path);

    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        
        if (doc.isObject()) {
            m_config = doc.object();
        }
        file.close();
    } else {
        qWarning() << "Failed to load config file:" << path;
    }
}

void BaseModule::init()
{  
    initSplitters();  
    initButton();        
}

void BaseModule::initSplitters()
{
}

void BaseModule::createActions()
{
}

void BaseModule::initButton()
{
    const QString toolTipStyle = 
        "QToolTip {"
        "color: black;"
        "background-color: #ffffff;"
        "border: 1px solid gray;"
        "}";

    const QString textBaseStyle = 
        "QPushButton{"
        "background-color:rgba(77,77,115,136);"
        "border-style:outset;"
        "border-width:1px;"
        "border-radius:10px;"
        "border-color:rgba(255,255,255,30);"
        "color:rgba(255,255,255,255);"
        "padding:6px;"; 

    const QString iconBaseStyle = 
        "QPushButton{"
        "background-color:rgba(77,77,115,136);"
        "border-style:outset;"
        "border-width:1px;"
        "border-radius:15px;"
        "border-color:rgba(255,255,255,30);"
        "color:rgba(255,255,255,255);"
        "padding:6px;"; 

    const QString hoverStyle = 
        "QPushButton:hover{"
        "background-color:rgba(200,200,200,100);"
        "border-color:rgba(255,255,255,200);"
        "color:rgba(255,255,255,255);"
        "}";

    const QString pressedGreen = 
        "QPushButton:pressed{"
        "background-color:rgba(100,255,100,200);"
        "border-color:rgba(255,255,255,30);"
        "border-style:inset;"
        "color:rgba(255,255,255,255);"
        "}";

    const QString pressedDark = 
        "QPushButton:pressed{"
        "background-color:rgba(20,20,20,0);"
        "border-color:rgba(255,255,255,30);"
        "border-style:inset;"
        "color:rgba(255,255,255,255);"
        "}";

    auto applyTheme = [&](QPushButton* btn, const QString& baseStyle, const QString& pressedStyle, bool extraBorders) {
        if (!btn) return;

        QString finalStyle = baseStyle;

        QString currentSheet = btn->styleSheet();
        QRegularExpression regex("image\\s*:\\s*url\\([^)]+\\);", QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch match = regex.match(currentSheet);
        if (match.hasMatch()) {
            finalStyle += match.captured(0);
        }

        if (extraBorders) {
            finalStyle += "border-top:1px solid rgb(237, 238, 241);"
                          "border-bottom:1px solid rgb(237, 238, 241);";
        }
        finalStyle += "}";

        finalStyle += pressedStyle + hoverStyle + toolTipStyle;
        btn->setStyleSheet(finalStyle);
    };

    if (ui.about_btn) applyTheme(ui.about_btn, textBaseStyle, pressedGreen, true);
    if (ui.acknowledgement_btn) applyTheme(ui.acknowledgement_btn, textBaseStyle, pressedGreen, true);
}

void BaseModule::closeEvent(QCloseEvent* event)
{
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        tr("Confirm Exit"),
        tr("Are you sure you want to exit this module?"),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        event->accept();
    } else {
        event->ignore();
    }
}