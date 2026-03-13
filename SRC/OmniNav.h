#pragma once

#include <QtWidgets/QMainWindow>
#include <vector>
#include <memory> 
#include <QVector>
#include <QJsonObject>
#include <unordered_map>
#include <QColor>
#include "Base/Base.h"
#include "BaseModule.h"
#include "DataManager.h" 
#include "OpticalNavigation.h" 

class OmniNav : public Base
{
    Q_OBJECT

public:
    OmniNav();
    ~OmniNav() override;

    OmniNav(const OmniNav&) = delete;
    OmniNav& operator=(const OmniNav&) = delete;

    void init() override;

    template <typename T>
    std::shared_ptr<T> getModule() {
        for (auto& kv : m_Modules) {
            auto casted = std::dynamic_pointer_cast<T>(kv.second);
            if (casted) return casted;
        }
        return nullptr;
    }

private:
    QJsonObject m_config;
    std::unordered_map<QString, std::shared_ptr<BaseModule>> m_Modules;   
    void loadConfig();
    void addModule();
    void createActions();
    void updateSliceByLeftClicking0(vtkObject* caller, unsigned long eventId, void* callData);
    void updateSliceByLeftClicking1(vtkObject* caller, unsigned long eventId, void* callData);
    void updateSliceByRightClicking1(vtkObject* caller, unsigned long eventId, void* callData);
    void updateSliceByLeftClicking2(vtkObject* caller, unsigned long eventId, void* callData);
    void updateSliceByLeftClicking3(vtkObject* caller, unsigned long eventId, void* callData);

private slots:
    void onModuleSelectionChanged(const QString &moduleName); 
    void on3DColorChanged(QColor top, QColor bottom);
    void onAddImage(Image* img, int row, int last = -1);
    void onDeleteImage(Image* img);
    void onAddMesh(Model3D* Mesh);
    void onDeleteMesh(Model3D* Mesh);
    void onAddLandmark(Landmark* Landmark);
    void onDeleteLandmark(Landmark* landmark);
    void onAddTool(Tool* tool);
    void onDeleteTool(Tool* tool);
    void onAddTransform(OmniTransform* transform);
    void onDeleteTransform(OmniTransform* transform);
    void onChangeVolumeVisualState(bool state);
    void onUpdateSlice(int view_num);
    void onUpdate3DOpacity();
    void onUpdateInfo2OpticalNavigation();
    void onUpdateInfo2PunctureRobot();
    void onUpdateNavigationInfo2DataManager(const QVector<TrackedTool>& tools);
    void onOpenFiles();
    void onStopTracking();
};