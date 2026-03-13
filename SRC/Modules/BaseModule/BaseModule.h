#pragma once
#include <QMainWindow>
#include <QCloseEvent>
#include <QSplitter>
#include <QJsonObject>
#include "ui_BaseModule.h" 

class BaseModule : public QMainWindow
{
    Q_OBJECT

public:
    explicit BaseModule(QWidget* parent = nullptr);
    ~BaseModule() override;

    BaseModule(const BaseModule&) = delete;
    BaseModule& operator=(const BaseModule&) = delete;

    virtual void init();
    Ui::BaseModuleClass& getUi() { return ui;}
    virtual QWidget* getCentralwidget() {
        return ui.centralwidget;
    };

protected:
    virtual void initSplitters();
    virtual void createActions();
    virtual void initButton(); 
    virtual void loadConfig();
    void closeEvent(QCloseEvent* event) override;

private:
    Ui::BaseModuleClass ui; 
    QJsonObject m_config;
    QSplitter* m_vsplitter = nullptr;
};