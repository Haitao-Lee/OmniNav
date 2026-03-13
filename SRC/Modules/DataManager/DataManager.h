#pragma once
#include <QSplitter>
#include <vector>
#include <QSlider>
#include <QJsonObject>
#include <QTableWidgetItem>
#include <QColor>
#include <QHash>
#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>
#include <vtkLookupTable.h>
#include "BaseModule.h"
#include "ui_DataManager.h"
#include "Image.h"
#include "Model3D.h"
#include "Landmark.h"
#include "Tool.h"
#include "OmniTransform.h"

class DataManager : public BaseModule
{
    Q_OBJECT

public:
    explicit DataManager(QWidget* parent = nullptr);
    ~DataManager() override;

    Ui::DataManagerClass& getUi() { return ui;}
    QWidget* getCentralwidget() {
        return ui.centralwidget;
    };
    void importFiles(const QString& specificType = "");

    const QJsonObject& getConfig() const { return m_config; }
    QSplitter* getVSplitter() const { return m_vsplitter; }
    QSplitter* getModuleVSplitter() const { return module_vsplitter; }

    const std::vector<Image*>& getImages() const { return m_images; }
    const std::vector<Model3D*>& getMeshes() const { return m_meshes; }
    const std::vector<Tool*>& getTools() const { return m_tools; }
    const std::vector<Landmark*>& getLandmarks() const { return m_landmarks; }
    const std::vector<OmniTransform*>& getOmniTransforms() const { return m_transforms; }

    const QHash<QString, Tool*>& getToolCache() const { 
        return m_toolCache; 
    }

    const QHash<QString, int>& getTableToolRowCache() const { 
        return m_tableToolRowCache; 
    }

    int getCurrentImageIndex() const { return m_currentImageIndex; }
    void setCurrentImageIndex(int index) {m_currentImageIndex = index;}
    int getCurrentVisualImageIndex() const { return m_currentVisualImageIndex; }
    int getCurrentMeshIndex() const { return m_currentMeshIndex; }
    int getCurrentImplantIndex() const { return m_currentImplantIndex; }
    int getCurrentToolIndex() const { return m_currentToolIndex; }
    int getCurrentLandmarkIndex() const { return m_currentLandmarkIndex; }
    int getCurrentRomIndex() const { return m_currentRomIndex; }

    int getLower2DValue() const { return m_lower2Dvalue; }
    int getUpper2DValue() const { return m_upper2Dvalue; }
    int getLower3DValue() const { return m_lower3Dvalue; }
    int getUpper3DValue() const { return m_upper3Dvalue; }

    const QColor& getTop3DColor() const { return m_top3DColor; }
    void setTop3DColor(const QColor& top3DColor) { m_top3DColor = top3DColor; }
    const QColor& getBottom3DColor() const { return m_bottom3DColor; }
    void setBottom3DColor(const QColor& bottom3DColor) { m_bottom3DColor = bottom3DColor; }
    void printInfo(const QString& message) { 
        QString htmlContent = QString(
            "<div style=\"line-height: 150%; margin-bottom: 10px; color: red;\">"
            "--%1"
            "</div>"
        ).arg(message.toHtmlEscaped());
        ui.info_te->appendHtml(htmlContent);
    }

    bool addImage(std::string filePath);
    bool deleteImage(int index);
    bool exportImage(int index, std::string savePath);
    bool add3Dmodel(std::string filePath, std::string orientation_sign = "");
    bool delete3Dmodel(int index);
    bool export3Dmodel(int index, std::string savePath);
    bool addLandmark(double* coordinates = nullptr, std::string name = "", std::string orientation_sign = "");
    bool deleteLandmark(int index);
    bool exportLandmark(std::string savePath);
    bool loadLandmark(std::string filePath);
    bool addTool(QString path);
    bool deleteTool(int index);
    bool exportTool(std::string savePath);
    bool setOmniTransform(QString name, Eigen::Matrix4d value);
    bool addOmniTransform(QString name = "", Eigen::Matrix4d value = Eigen::Matrix4d::Identity());
    bool deleteOmniTransform(int index);
    bool exportOmniTransform(std::string savePath);
    bool loadOmniTransform(std::string filePath);

signals:
    void color3DChanged(QColor top, QColor bottom);
    void signalUpdateViews();
    void signalAddImage(Image* dicom, int row, int last);
    void signalDeleteImage(Image* dicom);
    void signalAddMesh(Model3D* Mesh);
    void signalDeleteMesh(Model3D* Mesh);
    void signalAddLandmark(Landmark* landmark);
    void signalDeleteLandmark(Landmark* landmark);
    void signalAddTool(Tool* tool);
    void signalDeleteTool(Tool* tool);
    void signalAddOmniTransform(OmniTransform* transform);
    void signalDeleteOmniTransform(OmniTransform* transform);
    void signalChangeVolumeVisualState(bool state);

private:
    Ui::DataManagerClass ui;
    QJsonObject m_config;
    QSplitter* m_vsplitter = nullptr;
    QSplitter* module_vsplitter = nullptr;

    vtkSmartPointer<vtkLookupTable> m_lut2d = nullptr;
    vtkSmartPointer<vtkColorTransferFunction> m_ctf3d = nullptr;
    vtkSmartPointer<vtkPiecewiseFunction> m_pwf3d = nullptr;

    std::vector<Image*> m_images;
    std::vector<Model3D*> m_meshes;
    std::vector<Tool*> m_tools;
    std::vector<Landmark*> m_landmarks;
    std::vector<OmniTransform*> m_transforms;

    QHash<QString, Tool*> m_toolCache; 
    QHash<QString, int> m_tableToolRowCache; 

    int m_currentImageIndex = -1;
    int m_currentVisualImageIndex = -1;
    int m_currentMeshIndex = -1;
    int m_currentImplantIndex = -1;
    int m_currentToolIndex = -1;
    int m_currentLandmarkIndex = -1;
    int m_currentRomIndex = -1;

    int m_lower2Dvalue = 0;
    int m_upper2Dvalue = 0;
    int m_lower3Dvalue = 0;
    int m_upper3Dvalue = 0;

    double m_volOpacity = 1.0;
    double m_volColors[3][3];

    QColor m_top3DColor;
    QColor m_bottom3DColor;

    void init() override;
    void initSplitters() override;
    void initButton() override;
    void initProperty();
    void initDataWidget();
    void createActions() override;
    void loadConfig() override;
    void updateProperty();

private slots:
    void onAddImage();
    void onDeleteImage();
    void onExportImage();

    void onAdd3Dmodel();
    void onDelete3Dmodel();
    void onExport3Dmodel();

    void onAddLandmark();
    void onDeleteLandmark();
    void onExportLandmark();
    void onEditLandmark(QTableWidgetItem* item);

    void onAddTool();
    void onDeleteTool();
    void onExportTool();
    void onEditTool(QTableWidgetItem* item);

    void onAddOmniTransform();
    void onDeleteOmniTransform();
    void onExportOmniTransform();
    void onEditOmniTransform(QTableWidgetItem* item);

    void buildCache();
    
    void onImageTableItemChanged(QTableWidgetItem* item);
    void onImageTableSelectionChanged();

    void onLower2DChanged(int value);
    void onUpper2DChanged(int value);
    void onLower3DChanged(int value);
    void onUpper3DChanged(int value);

    void onTop3DColorBtnClicked();
    void onBottom3DColorBtnClicked();
    void onImageTableDoubleClicked(QTableWidgetItem* item); 
    void onVolumeCboxChanged(bool state);
};