#include "OmniNav.h"
#include <QCloseEvent>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>
#include <vtkVolumeCollection.h>
#include <QFile>
#include <QCoreApplication>
#include "DataManager.h"
#include "PunctureRobot.h"
#include "OpticalNavigation.h"
#include "Tool.h"
#include "ElectromagneticNavigation.h"
#include <map>
#include <vtkMatrix4x4.h>
#include "VisualizationUtils.h"
#include <vtkSTLWriter.h>

OmniNav::OmniNav()
{
    loadConfig();
    addModule();
    init();
    createActions();
}

OmniNav::~OmniNav()
{
    m_Modules.clear();
}

void OmniNav::init()
{
    auto dataManager = getModule<DataManager>();

    if (!dataManager) return;

    QJsonObject base_config = getConfig();
    QJsonObject displayConfig = base_config["display"].toObject();
    QJsonObject colorsConfig = displayConfig["colors"].toObject();

    QColor topColor(Qt::white); 
    if (colorsConfig.contains("top_3d_color")) {
        QJsonArray topColorArr = colorsConfig["top_3d_color"].toArray();
        if (topColorArr.size() >= 3) {
            topColor.setRgbF(
                topColorArr.at(0).toDouble(1.0),
                topColorArr.at(1).toDouble(1.0),
                topColorArr.at(2).toDouble(1.0)
            );
        }
    }

    QColor botColor(Qt::white); 
    if (colorsConfig.contains("bottom_3d_color")) {
        QJsonArray botColorArr = colorsConfig["bottom_3d_color"].toArray();
        if (botColorArr.size() >= 3) {
            botColor.setRgbF(
                botColorArr.at(0).toDouble(1.0),
                botColorArr.at(1).toDouble(1.0),
                botColorArr.at(2).toDouble(1.0)
            );
        }
    }

    auto updateBtnColor = [](QPushButton* btn, const QString& colorName) {
        QString style = btn->styleSheet();
        QString newProp = QString("background-color: %1;").arg(colorName);
        QRegularExpression re("background-color\\s*:\\s*[^;]+;?", QRegularExpression::CaseInsensitiveOption);
        
        if (style.contains(re)) {
            style.replace(re, newProp);
        } else {
            style += QString(" QPushButton { %1 }").arg(newProp);
        }
        btn->setStyleSheet(style);
    };

    dataManager->setTop3DColor(topColor);
    dataManager->setBottom3DColor(botColor);

    updateBtnColor(dataManager->getUi().color3D_btn_top, topColor.name());
    updateBtnColor(dataManager->getUi().color3D_btn_bot, botColor.name());

    on3DColorChanged(topColor, botColor);
}

void OmniNav::loadConfig()
{
    QString path = QCoreApplication::applicationDirPath() + "/OmniNav_config.json";
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

void OmniNav::addModule()
{
    m_Modules.insert({ "Welcome to OmniNavigator", std::make_shared<BaseModule>() });    
    onModuleSelectionChanged("Welcome to OmniNavigator");
    
    if (!m_config.contains("modules")) {
        qWarning() << "Config JSON missing 'modules' array.";
        return;
    }

    QJsonArray modulesArray = m_config["modules"].toArray();

    for (const QJsonValue &value : modulesArray) {
        QString moduleName = value.toString();

        if (m_Modules.contains(moduleName)) {
            qWarning() << "Module already exists:" << moduleName;
            continue;
        }

        std::shared_ptr<BaseModule> module = nullptr;

        if (moduleName == "DataManager") {
            module = std::make_shared<DataManager>();
        }
        else if (moduleName == "OpticalNavigation") {
            module = std::make_shared<OpticalNavigation>();
        }
        else if (moduleName == "ElectromagneticNavigation") {
            module = std::make_shared<ElectromagneticNavigation>();
        }
        else if (moduleName == "PunctureRobot") {
            module = std::make_shared<PunctureRobot>();
        }
        else {
            qWarning() << "Unknown module name:" << moduleName;
            continue;
        }

        m_Modules.insert({ moduleName, module });

        ui.modules_cbb->addItem(moduleName);
        qDebug() << "Module Loaded:" << moduleName;
    }
}


void OmniNav::createActions()
{
    connect(ui.modules_cbb, &QComboBox::currentTextChanged, this, &OmniNav::onModuleSelectionChanged);
    connect(ui.file_btn, &QPushButton::clicked, this, &OmniNav::onOpenFiles);

    connect(m_uiDisplays[0]->getUi().scrollbar, &QSlider::valueChanged, this, [this](int value) { this->onUpdateSlice(0); });
    connect(m_uiDisplays[1]->getUi().scrollbar, &QSlider::valueChanged, this, &OmniNav::onUpdate3DOpacity);
    connect(m_uiDisplays[2]->getUi().scrollbar, &QSlider::valueChanged, this, [this](int value) { this->onUpdateSlice(2); });
    connect(m_uiDisplays[3]->getUi().scrollbar, &QSlider::valueChanged, this, [this](int value) { this->onUpdateSlice(3); });

    for (int i = 0; i < m_irens.size(); ++i)
    {
        if (!m_irens[i]) continue;

        if (i == 0) {
            vtkNew<vtkCallbackCommand> callback;
            callback->SetClientData(this);
            callback->SetCallback([](vtkObject* caller, unsigned long eventId, void* clientData, void* callData){
                static_cast<OmniNav*>(clientData)->updateSliceByLeftClicking0(caller, eventId, callData);
            });
            m_irens[i]->AddObserver(vtkCommand::LeftButtonPressEvent, callback);
        }
        else if (i == 1) {
            vtkNew<vtkCallbackCommand> callbackL;
            callbackL->SetClientData(this);
            callbackL->SetCallback([](vtkObject* caller, unsigned long eventId, void* clientData, void* callData){
                static_cast<OmniNav*>(clientData)->updateSliceByLeftClicking1(caller, eventId, callData);
            });
            m_irens[i]->AddObserver(vtkCommand::LeftButtonPressEvent, callbackL);

            vtkNew<vtkCallbackCommand> callbackR;
            callbackR->SetClientData(this);
            callbackR->SetCallback([](vtkObject* caller, unsigned long eventId, void* clientData, void* callData){
                static_cast<OmniNav*>(clientData)->updateSliceByRightClicking1(caller, eventId, callData);
            });
            m_irens[i]->AddObserver(vtkCommand::RightButtonPressEvent, callbackR);
        }
        else if (i == 2) {
            vtkNew<vtkCallbackCommand> callback;
            callback->SetClientData(this);
            callback->SetCallback([](vtkObject* caller, unsigned long eventId, void* clientData, void* callData){
                static_cast<OmniNav*>(clientData)->updateSliceByLeftClicking2(caller, eventId, callData);
            });
            m_irens[i]->AddObserver(vtkCommand::LeftButtonPressEvent, callback);
        }
        else if (i == 3) {
            vtkNew<vtkCallbackCommand> callback;
            callback->SetClientData(this);
            callback->SetCallback([](vtkObject* caller, unsigned long eventId, void* clientData, void* callData){
                static_cast<OmniNav*>(clientData)->updateSliceByLeftClicking3(caller, eventId, callData);
            });
            m_irens[i]->AddObserver(vtkCommand::LeftButtonPressEvent, callback);
        }
    }

    auto dataManager = getModule<DataManager>();
    if (dataManager) {
        connect(dataManager.get(), &DataManager::color3DChanged, this, &OmniNav::on3DColorChanged);
        connect(dataManager.get(), &DataManager::signalUpdateViews, this, &OmniNav::updateViews);
        connect(dataManager.get(), &DataManager::signalAddImage, this, &OmniNav::onAddImage);
        connect(dataManager.get(), &DataManager::signalDeleteImage, this, &OmniNav::onDeleteImage);
        connect(dataManager.get(), &DataManager::signalAddMesh, this, &OmniNav::onAddMesh);
        connect(dataManager.get(), &DataManager::signalDeleteMesh, this, &OmniNav::onDeleteMesh);
        connect(dataManager.get(), &DataManager::signalAddLandmark, this, &OmniNav::onAddLandmark);
        connect(dataManager.get(), &DataManager::signalDeleteLandmark, this, &OmniNav::onDeleteLandmark);
        connect(dataManager.get(), &DataManager::signalAddTool, this, &OmniNav::onAddTool);
        connect(dataManager.get(), &DataManager::signalDeleteTool, this, &OmniNav::onDeleteTool);
        connect(dataManager.get(), &DataManager::signalAddOmniTransform, this, &OmniNav::onAddTransform);
        connect(dataManager.get(), &DataManager::signalDeleteOmniTransform, this, &OmniNav::onDeleteTransform);
        connect(dataManager.get(), &DataManager::signalChangeVolumeVisualState, this, &OmniNav::onChangeVolumeVisualState);
    }

    auto opticalNavigation = getModule<OpticalNavigation>();
    if (opticalNavigation) {
        connect(opticalNavigation.get(), &OpticalNavigation::signalAllToolsUpdated, this, &OmniNav::onUpdateNavigationInfo2DataManager);
        connect(opticalNavigation.get(), &OpticalNavigation::signalStopTracking, this, &OmniNav::onStopTracking);
        connect(opticalNavigation.get(), &OpticalNavigation::signalSamplingStatus, ui.visual_label, &QLabel::setText);
    }

    if (dataManager && opticalNavigation) {
        connect(opticalNavigation.get(), &OpticalNavigation::reqGetTools, [=](std::vector<Tool*>& tools){
            tools = dataManager->getTools(); 
        });

        connect(opticalNavigation.get(), &OpticalNavigation::reqAddLandmark, [=](QString name, double x, double y, double z){
            double coords[3] = {x, y, z};
            dataManager->addLandmark(coords, name.toStdString(), "IJK");
            updateViews();
        });

        connect(opticalNavigation.get(), &OpticalNavigation::reqGetLandmarks, [=](std::vector<Landmark*>& landmarks, QString pointSetName){
            landmarks.clear();
            const std::vector<Landmark*>& allLandmarks = dataManager->getLandmarks();
            
            for (auto lm : allLandmarks) {
                if (lm && QString::fromStdString(lm->getPointset()) == pointSetName) {
                    landmarks.push_back(lm);
                }
            }
        });

        connect(opticalNavigation.get(), &OpticalNavigation::reqLandmarksColorUpdate, [=](){
            QString inputPSName = opticalNavigation->getInputPointSetName();
            QString outputPSName = opticalNavigation->getOutputPointSetName();
            
            if (inputPSName.isEmpty() || outputPSName.isEmpty()) return;

            const std::vector<Landmark*>& allLandmarks = dataManager->getLandmarks();
            auto table = dataManager->getUi().landmark_tw;

            int countN = 0;
            for (auto lm : allLandmarks) {
                if (lm && QString::fromStdString(lm->getPointset()) == outputPSName) {
                    countN++;
                }
            }

            int currentInputIndex = 0;
            for (size_t i = 0; i < allLandmarks.size(); ++i) {
                Landmark* lm = allLandmarks[i];
                if (!lm) continue;

                if (QString::fromStdString(lm->getPointset()) != inputPSName) continue;

                currentInputIndex++;

                QString style = "";
                double color[3] = {1.0, 0.0, 0.0}; 

                if (currentInputIndex <= countN) {
                    color[0] = 0.0; color[1] = 1.0; color[2] = 0.0;
                    style = "background-color: #00ff00";
                } else if (currentInputIndex == countN + 1) {
                    color[0] = 1.0; color[1] = 1.0; color[2] = 0.0;
                    style = "background-color: #ffff00";
                } else {
                    style = "background-color: #ff0000";
                }

                lm->setColor(color);

                if (!style.isEmpty() && i < table->rowCount()) {
                    QWidget* container = table->cellWidget(i, 2);
                    if (container) {
                        QPushButton* btn = container->findChild<QPushButton*>();
                        if (btn) {
                            btn->setStyleSheet(style + "; border: 1px solid gray; border-radius: 4px;");
                        }
                    }
                }
            }
            updateViews();
        });

        connect(opticalNavigation.get(), &OpticalNavigation::setToolRegisTransform, [=](QString name, Eigen::Matrix4d mat){
            auto toolCache = dataManager->getToolCache(); 
            if (toolCache.contains(name)) {
                vtkSmartPointer<vtkMatrix4x4> vtkMat = vtkSmartPointer<vtkMatrix4x4>::New();
                for (int r = 0; r < 4; ++r) {
                    for (int c = 0; c < 4; ++c) {
                        vtkMat->SetElement(r, c, mat(r, c));
                    }
                }
                Tool* tool = toolCache[name];
                if(tool) tool->setRegistrationMatrix(vtkMat);
                qDebug() << "Registration Matrix updated for tool:" << name;
            }
        });

        connect(opticalNavigation.get(), &OpticalNavigation::reqUpdateTransform, [=](QString name, Eigen::Matrix4d mat){
            dataManager->addOmniTransform(name, mat);
        });

        connect(opticalNavigation.get(), &OpticalNavigation::setToolCalibrationTransform, [=](QString name, Eigen::Matrix4d mat){
            auto toolCache = dataManager->getToolCache(); 
            if (toolCache.contains(name)) {
                vtkSmartPointer<vtkMatrix4x4> vtkMat = vtkSmartPointer<vtkMatrix4x4>::New();
                for (int r = 0; r < 4; ++r) {
                    for (int c = 0; c < 4; ++c) {
                        vtkMat->SetElement(r, c, mat(r, c));
                    }
                }
                Tool* tool = toolCache[name];
                if(tool) tool->setCalibrationMatrix(vtkMat);
                qDebug() << "Calibration Matrix updated for tool:" << name;
            }
        });
        
    }

    auto punctureRobot = getModule<PunctureRobot>();
    if (punctureRobot && opticalNavigation && dataManager) 
    {
        connect(opticalNavigation.get(), &OpticalNavigation::signalAllToolsUpdated, punctureRobot.get(), &PunctureRobot::onToolsUpdated);
        connect(punctureRobot.get(), &PunctureRobot::reqAdd3DTrajectory, this, [=](const double entry[3], const double target[3], QString pointSetName) {
            Eigen::Vector3d pStart(entry[0], entry[1], entry[2]);
            Eigen::Vector3d pEnd(target[0], target[1], target[2]);
            // if (dataManager->getUi().orientation_cbb->currentText() == "RAS") { 
            //     Eigen::Vector3d pStart(-entry[0], -entry[1], entry[2]);
            //     Eigen::Vector3d pEnd(-target[0], -target[1], target[2]);
            // }

            auto tubePoly = VisualizationUtils::getTubePolyData(pStart, pEnd, 2.0);
            QString fullPath = QCoreApplication::applicationDirPath() + "/" + pointSetName + ".stl";
            auto writer = vtkSmartPointer<vtkSTLWriter>::New();
            writer->SetFileName(fullPath.toStdString().c_str());
            writer->SetInputData(tubePoly);
            writer->Write();
            dataManager->add3Dmodel(fullPath.toStdString(), "IJK");
            updateViews();
        });

        connect(punctureRobot.get(), &PunctureRobot::reqGetLandmarks, [=](std::vector<Landmark*>& landmarks, QString pointSetName){
            landmarks.clear();
            const std::vector<Landmark*>& allLandmarks = dataManager->getLandmarks();
            
            for (auto lm : allLandmarks) {
                if (lm && QString::fromStdString(lm->getPointset()) == pointSetName) {
                    landmarks.push_back(lm);
                }
            }
        });

        connect(punctureRobot.get(), &PunctureRobot::reqAddLandmark, [=](QString name, double x, double y, double z){
            double coords[3] = {x, y, z};
            dataManager->addLandmark(coords, name.toStdString(), "IJK");
            updateViews();
        });

        connect(punctureRobot.get(), &PunctureRobot::reqGetTools, [=](std::vector<Tool*>& tools){
            tools = dataManager->getTools(); 
        });
    }
}


void OmniNav::onModuleSelectionChanged(const QString &moduleName)
{
    if (!m_Modules.contains(moduleName)) {
        return;
    }

    auto module = m_Modules[moduleName];
    if (!module) return;

    QWidget* newView = module->getCentralwidget();
    if (!newView) {
        qWarning() << "Module" << moduleName << "returned a null widget.";
        return;
    }

    QWidget* container = ui.module_widget;
    
    if (!container->layout()) {
        QVBoxLayout* layout = new QVBoxLayout(container);
        layout->setContentsMargins(0, 0, 0, 0); 
        layout->setSpacing(0);
        container->setLayout(layout);
    }

    QLayout* layout = container->layout();
    QLayoutItem* item;
    while ((item = layout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->setParent(nullptr); 
            item->widget()->hide();         
        }
        delete item;
    }

    layout->addWidget(newView);
    newView->show();
    
    qDebug() << "Switched to module:" << moduleName;
}

void OmniNav::onOpenFiles()
{
    auto dataManager = getModule<DataManager>();
    if (dataManager) {
        dataManager->importFiles();
    }
}


void OmniNav::on3DColorChanged(QColor top, QColor bottom)
{
    int view3DIndex = Base::VIEW3D;

    if (getRenderers().size() > view3DIndex && getRenderers()[view3DIndex]) {
        QString styleSheet = QString("background-color: %1;").arg(top.name());
        m_uiDisplays[view3DIndex]->getUi().box->setStyleSheet(styleSheet);
        getRenderers()[view3DIndex]->SetBackground(bottom.redF(), bottom.greenF(), bottom.blueF());
        getRenderers()[view3DIndex]->SetBackground2(top.redF(), top.greenF(), top.blueF());
        getRenderers()[view3DIndex]->SetGradientBackground(true);
    }

    if (getVtkRenderWindows().size() > view3DIndex && getVtkRenderWindows()[view3DIndex]) {
        getVtkRenderWindows()[view3DIndex]->Render();
    }
}


void OmniNav::onAddImage(Image* dicom, int row, int last)
{
    auto dataManager = getModule<DataManager>();
    if (!dataManager)  return;
    dataManager->setCurrentImageIndex(row);

    vtkImageData* vtk_img = dicom->getImageData();
    int extent[6];
    vtk_img->GetExtent(extent);
    double spacing[3];
    vtk_img->GetSpacing(spacing);
    double origin[3];
    vtk_img->GetOrigin(origin);

    int extent_range[4];
    extent_range[0] = extent[5] - extent[4];
    extent_range[1] = 100;
    extent_range[2] = extent[1] - extent[0];
    extent_range[3] = extent[3] - extent[2];

    dataManager->getUi().volume_cbox->setChecked(true);


    if (last != -1 && last < dataManager->getImages().size()) {
        QCoreApplication::processEvents();
        std::vector<vtkSmartPointer<vtkProp>> lastActors = dataManager->getImages()[last]->getActors();
        if (lastActors.size() > 1) {
            m_renderers[1]->RemoveViewProp(lastActors[1]);
        }
    }

    int views2D[] = {0, 2, 3};
    for (int i : views2D) {
        QCoreApplication::processEvents();
        m_renderers[i]->RemoveAllViewProps();
    }

    std::vector<vtkSmartPointer<vtkProp>> currentActors = dicom->getActors();
    int viewOrder[] = {0, 2, 3, 1};
    
    double viewUp[4][3] = {
        {0.0, -1.0, 0.0},
        {0.0, 0.0, -1.0},
        {0.0, 0.0, 1.0},
        {0.0, 0.0, 1.0}
    };

    for (int i : viewOrder) {
        QCoreApplication::processEvents();
        if (i == 1) {
            resetCamera(m_renderers[0], 1, viewUp[0]);
            resetCamera(m_renderers[2], 1, viewUp[2]);
            resetCamera(m_renderers[3], 1, viewUp[3]);
            QCoreApplication::processEvents();
            m_renderers[i]->AddViewProp(currentActors[i]);
            QCoreApplication::processEvents();
            
            int num_of_actors = m_renderers[i]->GetViewProps()->GetNumberOfItems();
            if (num_of_actors > 0) {
                resetCamera(m_renderers[i], 0, viewUp[i]);
            }
        } else {
            QCoreApplication::processEvents();
            m_uiDisplays[i]->getUi().scrollbar->setMaximum(extent_range[i]);
            QCoreApplication::processEvents();
            m_uiDisplays[i]->getUi().scrollbar->setValue(extent_range[i] / 2);
            QCoreApplication::processEvents();
            m_renderers[i]->AddViewProp(currentActors[i]);
            QCoreApplication::processEvents();
        }
        m_vtkRenderWindows[i]->Render();
    }

    double tmp_lineCenter[3] = {0.0, 0.0, 0.0};
    tmp_lineCenter[2] = spacing[2] * (m_uiDisplays[0]->getUi().scrollbar->value() + extent[4]) + origin[2];
    tmp_lineCenter[0] = spacing[0] * (m_uiDisplays[2]->getUi().scrollbar->value() + extent[0]) + origin[0];
    tmp_lineCenter[1] = spacing[1] * (m_uiDisplays[3]->getUi().scrollbar->value() + extent[2]) + origin[1];

    double diff[3];
    diff[0] = tmp_lineCenter[0] - m_lineCenter[0];
    diff[1] = tmp_lineCenter[1] - m_lineCenter[1];
    diff[2] = tmp_lineCenter[2] - m_lineCenter[2];

    for (int i = 0; i < 3; ++i) {
        if (m_lineActors.size() > i && m_lineActors[i]) {
            m_lineActors[i]->AddPosition(diff[0], diff[1], diff[2]);
        }
    }

    m_lineCenter[0] = tmp_lineCenter[0];
    m_lineCenter[1] = tmp_lineCenter[1];
    m_lineCenter[2] = tmp_lineCenter[2];

    reposition2DLineActors();
    
    onResetCammera0();
    onResetCammera2();
    onResetCammera3();
    onResetCammera1();

    updateViews();
}

void OmniNav::onDeleteImage(Image* dicom)
{
    // todo
}


void OmniNav::onAddMesh(Model3D* Mesh)
{
    m_renderers[1]->AddActor(Mesh->getActor());
    m_vtkRenderWindows[1]->Render();
    updateViews();
}

void OmniNav::onDeleteMesh(Model3D* Mesh)
{
    if (!Mesh) {
        return;
    }

    if (m_renderers.size() > 1 && m_renderers[1]) {
        m_renderers[1]->RemoveActor(Mesh->getActor());
    }

    const auto& prjActors = Mesh->getPrjActors();

    if (m_contourRenderers.size() >= 4) {
        if (prjActors.size() > 0 && prjActors[0] && m_contourRenderers[2]) {
            m_contourRenderers[2]->RemoveActor(prjActors[0]);
        }

        if (prjActors.size() > 1 && prjActors[1] && m_contourRenderers[3]) {
            m_contourRenderers[3]->RemoveActor(prjActors[1]);
        }

        if (prjActors.size() > 2 && prjActors[2] && m_contourRenderers[0]) {
            m_contourRenderers[0]->RemoveActor(prjActors[2]);
        }
    }
    updateViews();
}

void OmniNav::onAddLandmark(Landmark* Landmark)
{
    m_renderers[1]->AddActor(Landmark->getActor());
    m_vtkRenderWindows[1]->Render();
    onUpdateInfo2OpticalNavigation();
    onUpdateInfo2PunctureRobot();
    updateViews();
}

void OmniNav::onDeleteLandmark(Landmark* landmark)
{
    if (!landmark) {
        return;
    }

    if (m_renderers.size() > 1 && m_renderers[1]) {
        m_renderers[1]->RemoveActor(landmark->getActor());
    }

    const auto& prjActors = landmark->getPrjActors();

    if (m_contourRenderers.size() >= 4) {
        if (prjActors.size() > 0 && prjActors[0] && m_contourRenderers[2]) {
            m_contourRenderers[2]->RemoveActor(prjActors[0]);
        }

        if (prjActors.size() > 1 && prjActors[1] && m_contourRenderers[3]) {
            m_contourRenderers[3]->RemoveActor(prjActors[1]);
        }

        if (prjActors.size() > 2 && prjActors[2] && m_contourRenderers[0]) {
            m_contourRenderers[0]->RemoveActor(prjActors[2]);
        }
    }

    onUpdateInfo2OpticalNavigation();
    onUpdateInfo2PunctureRobot();
    updateViews();
}

void OmniNav::onAddTool(Tool* tool)
{
    if (m_renderers[1] && tool->getActor()) {
        m_renderers[1]->AddActor(tool->getActor());
    }
    if (m_vtkRenderWindows[1]) {
        m_vtkRenderWindows[1]->Render();
    }
    
    onUpdateInfo2OpticalNavigation();
    onUpdateInfo2PunctureRobot();
    updateViews();
}

void OmniNav::onDeleteTool(Tool* tool)
{
    if (!tool) {
        return;
    }

    if (m_renderers.size() > 1 && m_renderers[1]) {
        m_renderers[1]->RemoveActor(tool->getActor());
    }

    const auto& prjActors = tool->getPrjActors();

    if (m_contourRenderers.size() >= 4) {
        if (prjActors.size() > 0 && prjActors[0] && m_contourRenderers[2]) {
            m_contourRenderers[2]->RemoveActor(prjActors[0]);
        }

        if (prjActors.size() > 1 && prjActors[1] && m_contourRenderers[3]) {
            m_contourRenderers[3]->RemoveActor(prjActors[1]);
        }

        if (prjActors.size() > 2 && prjActors[2] && m_contourRenderers[0]) {
            m_contourRenderers[0]->RemoveActor(prjActors[2]);
        }
    }

    onUpdateInfo2OpticalNavigation();
    onUpdateInfo2PunctureRobot();
    updateViews();
}


void OmniNav::onAddTransform(OmniTransform* transform)
{    
    onUpdateInfo2OpticalNavigation();
    onUpdateInfo2PunctureRobot();
    updateViews();
}


void OmniNav::onDeleteTransform(OmniTransform* transform)
{
    onUpdateInfo2OpticalNavigation();
    onUpdateInfo2PunctureRobot();
    updateViews();
}

void OmniNav::onUpdateInfo2OpticalNavigation()
{
    auto opticalNavigation = getModule<OpticalNavigation>();
    auto dataManager = getModule<DataManager>();
    if (!dataManager || !opticalNavigation) return;

    auto syncComboBox = [](QComboBox* cb, const QStringList& validNames, int fixedCount) {
        QString currentText = cb->currentText();
        for (const QString& name : validNames) {
            if (cb->findText(name) == -1) cb->addItem(name);
        }
        for (int i = cb->count() - 1; i >= fixedCount; --i) {
            if (!validNames.contains(cb->itemText(i))) cb->removeItem(i);
        }
        int index = cb->findText(currentText);
        if (index != -1) cb->setCurrentIndex(index);
    };

    auto getNames = [](QTableWidget* table) {
        QStringList names;
        for (int i = 0; i < table->rowCount(); ++i) {
            auto item = table->item(i, 0);
            if (item) names.append(item->text());
        }
        return names;
    };

    QStringList landmarkNames = getNames(dataManager->getUi().landmark_tw);
    QStringList toolNames = getNames(dataManager->getUi().tool_tw);
    QStringList transformNames = getNames(dataManager->getUi().transform_tw);

    auto& ui = opticalNavigation->getUi();
    syncComboBox(ui.probe_sp_cbb, toolNames, 1);
    syncComboBox(ui.input_sp_cbb, landmarkNames, 1);
    syncComboBox(ui.output_sp_cbb, landmarkNames, 2);
    syncComboBox(ui.source_regis_cbb, landmarkNames, 1);
    syncComboBox(ui.target_regis_cbb, landmarkNames, 1);
    syncComboBox(ui.output_regis_cbb, transformNames, 2);
    syncComboBox(ui.probe_cali_cbb, toolNames, 1);
    syncComboBox(ui.tool_cali_cbb, toolNames, 1);
    syncComboBox(ui.output_cali_cbb, transformNames, 2);
}

void OmniNav::onUpdateInfo2PunctureRobot()
{
    auto punctureRobot = getModule<PunctureRobot>();
    auto dataManager = getModule<DataManager>();
    if (!dataManager || !punctureRobot) return;

    auto syncComboBox = [](QComboBox* cb, const QStringList& validNames, int fixedCount) {
        QString currentText = cb->currentText();
        for (const QString& name : validNames) {
            if (cb->findText(name) == -1) cb->addItem(name);
        }
        for (int i = cb->count() - 1; i >= fixedCount; --i) {
            if (!validNames.contains(cb->itemText(i))) cb->removeItem(i);
        }
        int index = cb->findText(currentText);
        if (index != -1) cb->setCurrentIndex(index);
    };

    auto getNames = [](QTableWidget* table) {
        QStringList names;
        for (int i = 0; i < table->rowCount(); ++i) {
            auto item = table->item(i, 0);
            if (item) names.append(item->text());
        }
        return names;
    };

    QStringList landmarkNames = getNames(dataManager->getUi().landmark_tw);
    QStringList toolNames = getNames(dataManager->getUi().tool_tw);

    auto& ui = punctureRobot->getUi();
    syncComboBox(ui.probeTool_cbb, toolNames, 1);
    syncComboBox(ui.efToolPrepare_cbb, toolNames, 1);
    syncComboBox(ui.baseTool_cbb, toolNames, 1);
    syncComboBox(ui.patientTool_cbb, toolNames, 1);
    syncComboBox(ui.efTool_cbb, toolNames, 1);
    syncComboBox(ui.trajPointset_cbb, landmarkNames, 1);
}


void OmniNav::onUpdateNavigationInfo2DataManager(const QVector<TrackedTool>& tools)
{
    auto dataManager = getModule<DataManager>();
    if (!dataManager) return;

    auto toolCache = dataManager->getToolCache(); 
    auto tableToolRowCache = dataManager->getTableToolRowCache(); 
    auto table = dataManager->getUi().tool_tw;
    auto meshTable = dataManager->getUi().mesh_tw;
    auto landmarkTable = dataManager->getUi().landmark_tw;
    
    const std::vector<Model3D*>& meshes = dataManager->getMeshes();
    const std::vector<Landmark*>& landmarks = dataManager->getLandmarks();

    // -------------------------------------------------------------
    // Part 1: 更新 Tool Table (UI 显示部分，保持不变)
    // -------------------------------------------------------------
    for (const auto& data : tools) {
        if (tableToolRowCache.contains(data.name)) {
            int row = tableToolRowCache[data.name];

            // 1. 更新 Handle
            QTableWidgetItem* handleItem = table->item(row, 8);
            if (!handleItem) { 
                handleItem = new QTableWidgetItem();
                handleItem->setTextAlignment(Qt::AlignCenter);
                table->setItem(row, 8, handleItem);
            }
            QString newHandle = QString::number(data.portHandle);
            if (handleItem->text() != newHandle) handleItem->setText(newHandle);

            // 2. 更新 RMSE
            QTableWidgetItem* rmseItem = table->item(row, 9); 
            if (!rmseItem) {
                rmseItem = new QTableWidgetItem();
                rmseItem->setTextAlignment(Qt::AlignCenter);
                table->setItem(row, 9, rmseItem);
            }
            QString newRMSE = QString::number(data.error, 'f', 4);
            if (rmseItem->text() != newRMSE) {
                rmseItem->setText(newRMSE);
                rmseItem->setForeground(data.error > 0.5 ? Qt::red : Qt::white);
            }

            // 3. 更新矩阵显示 (Tooltip)
            QTableWidgetItem* transItem = table->item(row, 7);
            if (!transItem) {
                transItem = new QTableWidgetItem();
                transItem->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
                table->setItem(row, 7, transItem);
            }

            // 优先使用 toolCache 中校准后的矩阵 (FinalMatrix)
            double displayMatrix[4][4];
            // 默认用原始数据填充
            for(int i=0; i<4; i++) 
                for(int j=0; j<4; j++) 
                    displayMatrix[i][j] = data.matrix(i,j);

            // 如果 Cache 里有（通常都有），用处理过（Flip/Pivot）的矩阵覆盖
            if (toolCache.contains(data.name)) {
                Tool* tool = toolCache[data.name];
                vtkMatrix4x4* finalMat = tool->getFinalMatrix(); 
                if (finalMat) {
                    for(int i=0; i<4; i++) 
                        for(int j=0; j<4; j++) 
                            displayMatrix[i][j] = finalMat->GetElement(i,j);
                }
            }

            QString posStr = QString("T:[%1, %2, %3]")
                             .arg(displayMatrix[0][3], 0, 'f', 2)
                             .arg(displayMatrix[1][3], 0, 'f', 2)
                             .arg(displayMatrix[2][3], 0, 'f', 2);

            if (transItem->text() != posStr) {
                transItem->setText(posStr);
                QString fullMatStr = QString(
                    "R0: [%1, %2, %3, %4]\n"
                    "R1: [%5, %6, %7, %8]\n"
                    "R2: [%9, %10, %11, %12]\n"
                    "R3: [0, 0, 0, 1]")
                    .arg(displayMatrix[0][0], 0,'f',2).arg(displayMatrix[0][1], 0,'f',2).arg(displayMatrix[0][2], 0,'f',2).arg(displayMatrix[0][3], 0,'f',2)
                    .arg(displayMatrix[1][0], 0,'f',2).arg(displayMatrix[1][1], 0,'f',2).arg(displayMatrix[1][2], 0,'f',2).arg(displayMatrix[1][3], 0,'f',2)
                    .arg(displayMatrix[2][0], 0,'f',2).arg(displayMatrix[2][1], 0,'f',2).arg(displayMatrix[2][2], 0,'f',2).arg(displayMatrix[2][3], 0,'f',2);
                transItem->setToolTip(fullMatStr);
            }

            // 4. 更新在线状态颜色
            QTableWidgetItem* nameItem = table->item(row, 0);
            if (nameItem) {
                 QBrush targetColor = data.isVisible ? QBrush(Qt::green) : QBrush(Qt::red);
                 if (nameItem->foreground() != targetColor) {
                     nameItem->setForeground(targetColor);
                 }
            }
        }
    }

    // // -------------------------------------------------------------
    // // Part 2: 更新 Mesh Transform (保持 SetUserMatrix)
    // // Mesh 通常比较大，直接设置 Matrix 性能更好
    // // -------------------------------------------------------------
    // for (int i = 0; i < meshTable->rowCount(); ++i) {
    //     if (i >= meshes.size()) break;

    //     Model3D* mesh = meshes[i];
    //     QWidget* widget = meshTable->cellWidget(i, 5); // Column 5 is Tool ComboBox
    //     if (!widget) continue;

    //     QComboBox* cbb = widget->findChild<QComboBox*>();
    //     if (!cbb) continue;

    //     QString selectedToolName = cbb->currentText();
    //     vtkSmartPointer<vtkMatrix4x4> mat = vtkSmartPointer<vtkMatrix4x4>::New(); // Identity

    //     if (selectedToolName != "None") {
    //         if (toolCache.contains(selectedToolName)) {
    //             Tool* tool = toolCache[selectedToolName];
    //             vtkMatrix4x4* toolMat = tool->getFinalMatrix();
    //             if (toolMat) {
    //                 mat->DeepCopy(toolMat);
    //             }
    //         }
    //     }
        
    //     if (mesh->getActor()) {
    //         mesh->getActor()->SetUserMatrix(mat);
    //     }
    // }

    // // -------------------------------------------------------------
    // // Part 3: 更新 Landmark Coordinates (直接修改数值，Actor 会移动)
    // // -------------------------------------------------------------
    // for (int i = 0; i < landmarkTable->rowCount(); ++i) {
    //     if (i >= landmarks.size()) break;

    //     Landmark* lm = landmarks[i];
    //     QWidget* widget = landmarkTable->cellWidget(i, 6); // Column 6 is Tool ComboBox
    //     if (!widget) continue;

    //     QComboBox* cbb = widget->findChild<QComboBox*>();
    //     if (!cbb) continue;

    //     QString selectedToolName = cbb->currentText();
        
    //     // 只有当绑定了 Tool 时才进行坐标更新
    //     if (selectedToolName != "None") {
    //         if (toolCache.contains(selectedToolName)) {
    //             Tool* tool = toolCache[selectedToolName];
    //             vtkMatrix4x4* toolMat = tool->getFinalMatrix();
                
    //             if (toolMat) {
    //                 // [关键修改] 
    //                 // 假设 Landmark 代表 Tool 的尖端，即局部坐标 (0,0,0)
    //                 // 如果 Landmark 有固定的局部偏移，这里应该用 lm->getLocalCoordinates() 
    //                 // 但通常逻辑是：由 Tool 驱动的点，其世界坐标 = Tool矩阵的平移部分
                    
    //                 double x = toolMat->GetElement(0, 3);
    //                 double y = toolMat->GetElement(1, 3);
    //                 double z = toolMat->GetElement(2, 3);
                    
    //                 // 1. 设置 Actor 变换为 Identity (因为我们直接修改了顶点坐标，不需要再叠加矩阵)
    //                 vtkSmartPointer<vtkMatrix4x4> identity = vtkSmartPointer<vtkMatrix4x4>::New();
    //                 if (lm->getActor()) {
    //                     lm->getActor()->SetUserMatrix(identity);
    //                 }

    //                 // 2. 直接修改 Landmark 的坐标数据 -> Actor 会随之移动
    //                 double newCoords[3] = {x, y, z};
    //                 lm->setCoordinates(newCoords); 

    //                 // 3. 同步更新表格 UI (需要 blockSignals 防止循环触发 edit)
    //                 QTableWidgetItem* coordItem = landmarkTable->item(i, 1);
    //                 if (coordItem) {
    //                     QString newCoordStr = QString("%1, %2, %3")
    //                         .arg(x, 0, 'f', 2)
    //                         .arg(y, 0, 'f', 2)
    //                         .arg(z, 0, 'f', 2);
                        
    //                     if (coordItem->text() != newCoordStr) {
    //                         landmarkTable->blockSignals(true); // 暂停信号
    //                         coordItem->setText(newCoordStr);
    //                         landmarkTable->blockSignals(false); // 恢复信号
    //                     }
    //                 }
    //             }
    //         }
    //     } 
    //     else {
    //         // [新增] 如果 Name 是 None，设置为 Identity (重置)
    //         // 这里的语义通常是：如果不跟随 Tool，保持在该位置，或者重置回原点？
    //         // 既然代码要求 Identity，我们就确保 Actor 没有残留的 UserMatrix
    //         vtkSmartPointer<vtkMatrix4x4> identity = vtkSmartPointer<vtkMatrix4x4>::New();
    //         if (lm->getActor()) {
    //             lm->getActor()->SetUserMatrix(identity);
    //         }
    //         // 注意：这里没有重置 setCoordinates，意味着它会停留在最后一次跟随的位置
    //         // 如果你想让它回原点 (0,0,0)，可以在这里 lm->setCoordinates({0,0,0})
    //     }
    // }

    updateViews();
}


void OmniNav::onStopTracking()
{
    auto dataManager = getModule<DataManager>();
    if (!dataManager) return;

    auto table = dataManager->getUi().tool_tw;
    table->blockSignals(true);

    for (int i = 0; i < table->rowCount(); ++i) {
        QTableWidgetItem* nameItem = table->item(i, 0);
        if (nameItem) {
            nameItem->setForeground(QBrush(Qt::black));
        }

        QTableWidgetItem* handleItem = table->item(i, 6);
        if (handleItem) {
            handleItem->setText("N/A");
        }
    }

    table->blockSignals(false);
}

void OmniNav::onUpdateSlice(int view_num)
{
    auto dataManager = getModule<DataManager>();
    if (!dataManager)  return;
    if (dataManager->getCurrentVisualImageIndex() == -1)
    {
        return;
    }

    vtkImageData* vtk_img = dataManager->getImages()[dataManager->getCurrentVisualImageIndex()]->getImageData();

    int extent[6];
    double spacing[3];
    double origin[3];

    vtk_img->GetExtent(extent);
    vtk_img->GetSpacing(spacing);
    vtk_img->GetOrigin(origin);

    double tmp_lineCenter[3] = {
        spacing[0] * (m_uiDisplays[3]->getUi().scrollbar->value() + extent[0]) + origin[0],
        spacing[1] * (m_uiDisplays[2]->getUi().scrollbar->value() + extent[2]) + origin[1],
        spacing[2] * (m_uiDisplays[0]->getUi().scrollbar->value() + extent[4]) + origin[2]
    };

    double diff[3] = {
        tmp_lineCenter[0] - m_lineCenter[0],
        tmp_lineCenter[1] - m_lineCenter[1],
        tmp_lineCenter[2] - m_lineCenter[2]
    };

    for (int i = 0; i < 3; i++)
    {
        m_lineActors[i]->AddPosition(diff[0], diff[1], diff[2]);
    }

    m_lineActors[3]->AddPosition(diff[0], 0, 0);
    m_lineActors[4]->AddPosition(0, -diff[1], 0);
    m_lineActors[5]->AddPosition(0, diff[2], 0);
    m_lineActors[6]->AddPosition(-diff[0], 0, 0);
    m_lineActors[7]->AddPosition(0, diff[2], 0);
    m_lineActors[8]->AddPosition(-diff[1], 0, 0);

    m_lineCenter[0] = tmp_lineCenter[0];
    m_lineCenter[1] = tmp_lineCenter[1];
    m_lineCenter[2] = tmp_lineCenter[2];

    double adjustPos[3] = { m_lineCenter[0], m_lineCenter[1], m_lineCenter[2] };
    dataManager->getImages()[dataManager->getCurrentVisualImageIndex()]->adjustActors(adjustPos);

    updateView(view_num);

    // updateProject2D(this, view_num);
    //updateViews();
}


void OmniNav::onUpdate3DOpacity()
{
}

void OmniNav::updateSliceByLeftClicking0(vtkObject* caller, unsigned long eventId, void* callData)
{
    auto dataManager = getModule<DataManager>();
    if (!dataManager)  return;
    if (dataManager->getCurrentVisualImageIndex() == -1 || m_cross_line_sign == 0)
    {
        return;
    }

    int* click_pos = m_irens[0]->GetEventPosition();
    m_picker->Pick(click_pos[0], click_pos[1], 0, m_renderers[0]);

    double pos[3];
    m_picker->GetPickPosition(pos);

    vtkImageData* vtk_img = dataManager->getImages()[dataManager->getCurrentVisualImageIndex()]->getImageData();
    int extent[6];
    double spacing[3];
    vtk_img->GetExtent(extent);
    vtk_img->GetSpacing(spacing);

    int val2 = static_cast<int>((extent[3] - extent[2]) / 2.0 + (-pos[1]) / spacing[1]);
    int val3 = static_cast<int>((extent[1] - extent[0]) / 2.0 + (pos[0]) / spacing[0]);

    m_uiDisplays[2]->getUi().scrollbar->setValue(val2);
    m_uiDisplays[3]->getUi().scrollbar->setValue(val3);
}

void OmniNav::updateSliceByLeftClicking1(vtkObject* caller, unsigned long eventId, void* callData)
{
    // if (m_ui->location_btn->isChecked())
    // {
    //     int* click_pos = m_irens[1]->GetEventPosition();
    //     m_picker->Pick(click_pos[0], click_pos[1], 0, m_renderers[1]);

    //     double pos[3];
    //     m_picker->GetPickPosition(pos);

    //     qDebug() << "Picking the point at: (" 
    //              << pos[0] << "," 
    //              << pos[1] << "," 
    //              << pos[2] << ")";
    //     m_localizationActor->SetPosition(pos);
    //     m_views[1]->update();
    // }

    auto dataManager = getModule<DataManager>();
    if (!dataManager) return;
    if (dataManager->getCurrentVisualImageIndex() == -1 || m_cross_line_sign != 2) return;

    int* click_pos = m_irens[1]->GetEventPosition();
    m_picker->Pick(click_pos[0], click_pos[1], 0, m_renderers[1]);

    double pos[3];
    m_picker->GetPickPosition(pos);

    vtkImageData* vtk_img = dataManager->getImages()[dataManager->getCurrentVisualImageIndex()]->getImageData();
    double spacing[3];
    vtk_img->GetSpacing(spacing);

    int val0 = m_uiDisplays[0]->getUi().scrollbar->value() + static_cast<int>((pos[2] - m_lineCenter[2]) / spacing[2]);
    int val2 = m_uiDisplays[2]->getUi().scrollbar->value() + static_cast<int>((pos[1] - m_lineCenter[1]) / spacing[1]);
    int val3 = m_uiDisplays[3]->getUi().scrollbar->value() + static_cast<int>((pos[0] - m_lineCenter[0]) / spacing[0]);

    m_uiDisplays[0]->getUi().scrollbar->setValue(val0);
    m_uiDisplays[2]->getUi().scrollbar->setValue(val2);
    m_uiDisplays[3]->getUi().scrollbar->setValue(val3);
}



void OmniNav::updateSliceByRightClicking1(vtkObject* caller, unsigned long eventId, void* callData)
{
}


void OmniNav::updateSliceByLeftClicking2(vtkObject* caller, unsigned long eventId, void* callData)
{
    auto dataManager = getModule<DataManager>();
    if (!dataManager) return;
    if (dataManager->getCurrentVisualImageIndex() == -1 || m_cross_line_sign == 0) return;

    int* click_pos = m_irens[2]->GetEventPosition();
    m_picker->Pick(click_pos[0], click_pos[1], 0, m_renderers[2]);

    double pos[3];
    m_picker->GetPickPosition(pos);

    vtkImageData* vtk_img = dataManager->getImages()[dataManager->getCurrentVisualImageIndex()]->getImageData();
    int extent[6];
    double spacing[3];
    vtk_img->GetExtent(extent);
    vtk_img->GetSpacing(spacing);

    int val3 = static_cast<int>((extent[3] - extent[2]) / 2.0 - pos[0] / spacing[1]);
    int val0 = static_cast<int>((extent[5] - extent[4]) / 2.0 + pos[1] / spacing[2]);

    m_uiDisplays[3]->getUi().scrollbar->setValue(val3);
    m_uiDisplays[0]->getUi().scrollbar->setValue(val0);
}



void OmniNav::updateSliceByLeftClicking3(vtkObject* caller, unsigned long eventId, void* callData)
{
    auto dataManager = getModule<DataManager>();
    if (!dataManager) return;
    if (dataManager->getCurrentVisualImageIndex() == -1 || m_cross_line_sign == 0) return;

    int* click_pos = m_irens[3]->GetEventPosition();
    m_picker->Pick(click_pos[0], click_pos[1], 0, m_renderers[3]);

    double pos[3];
    m_picker->GetPickPosition(pos);

    vtkImageData* vtk_img = dataManager->getImages()[dataManager->getCurrentVisualImageIndex()]->getImageData();
    int extent[6];
    double spacing[3];
    vtk_img->GetExtent(extent);
    vtk_img->GetSpacing(spacing);

    int val0 = static_cast<int>((extent[5] - extent[4]) / 2.0 + pos[1] / spacing[2]);
    int val2 = static_cast<int>((extent[1] - extent[0]) / 2.0 - pos[0] / spacing[0]);

    m_uiDisplays[0]->getUi().scrollbar->setValue(val0);
    m_uiDisplays[2]->getUi().scrollbar->setValue(val2);
}

void OmniNav::onChangeVolumeVisualState(bool state)
{
    if (!m_renderers[1]) return;

    vtkVolumeCollection* volumes = m_renderers[1]->GetVolumes();
    volumes->InitTraversal();
    vtkVolume* volume = volumes->GetNextVolume();

    while (volume)
    {
        if (state)
            volume->VisibilityOn();
        else
            volume->VisibilityOff();

        volume = volumes->GetNextVolume();
    }

    updateViews();
}

