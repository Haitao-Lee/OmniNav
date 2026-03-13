#pragma once

#include <QMainWindow>
#include "ui_display.h" 

class Display : public QMainWindow
{
    Q_OBJECT

public:
    explicit Display(QWidget *parent = nullptr);
    ~Display() override;
    QLabel* getLabel() const {return ui.label;}
    QWidget* getBox() const {return ui.box;}
    Ui::DisplayClass& getUi() { return ui; }

private:
    Ui::DisplayClass ui;
};