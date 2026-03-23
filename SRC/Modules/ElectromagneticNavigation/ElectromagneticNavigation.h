#pragma once

#include "Modules/BaseModule/BaseModule.h"
#include "ui_ElectromagneticNavigation.h" 

class ElectromagneticNavigation : public BaseModule
{
    Q_OBJECT

public:
    explicit ElectromagneticNavigation(QWidget* parent = nullptr);
    ~ElectromagneticNavigation() override;

    // Get a reference to the UI instance.
    Ui::ElectromagneticNavigationClass& getUi() {return ui;}
    void init() override;
    QWidget* getCentralwidget() {
        return ui.centralwidget;
    };

private:
    Ui::ElectromagneticNavigationClass ui;
};
