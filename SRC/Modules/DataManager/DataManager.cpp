#include "DataManager.h"
#include <QMessageBox>
#include <QCloseEvent>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>
#include <QFile>
#include <QCoreApplication>
#include <QRegularExpression> 
#include <QFileDialog>
#include <QStandardPaths>
#include <QColorDialog>
#include <QListView>
#include <vtkNIFTIImageWriter.h>
#include <vtkMetaImageWriter.h>
#include <vtksys/SystemTools.hxx>
#include <vtkSTLWriter.h>
#include <vtkOBJWriter.h>
#include <vtkPLYWriter.h>
#include <vtkPolyDataWriter.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>


DataManager::DataManager(QWidget* parent)
    : BaseModule(parent)
{
    ui.setupUi(this);
    loadConfig();
    init();
    createActions();
}

DataManager::~DataManager()
{
    for (auto img : m_images) delete img;
    for (auto mesh : m_meshes) delete mesh;
    for (auto lm : m_landmarks) delete lm;
    for (auto tool : m_tools) delete tool;
}

void DataManager::init() {
    initButton();
    initSplitters();
    initDataWidget();
    initProperty();
}

void DataManager::initButton()
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
        "border-radius:20px;"
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

    applyTheme(ui.connect_btn, textBaseStyle, pressedGreen, true);
    applyTheme(ui.registration_btn, textBaseStyle, pressedGreen, true);
    applyTheme(ui.crossline_btn, textBaseStyle, pressedGreen, true);
    applyTheme(ui.reset_btn, textBaseStyle, pressedGreen, true);
    applyTheme(ui.measure_btn, textBaseStyle, pressedGreen, true);
    applyTheme(ui.volume_btn, textBaseStyle, pressedGreen, true);
    applyTheme(ui.project_btn, textBaseStyle, pressedGreen, true);

    applyTheme(ui.callibration_btn, textBaseStyle, pressedDark, true);
    applyTheme(ui.robotics_btn, textBaseStyle, pressedDark, true);
    applyTheme(ui.location_btn, textBaseStyle, pressedDark, true);

    applyTheme(ui.add_img_btn, iconBaseStyle, pressedDark, true);
    applyTheme(ui.delete_img_btn, iconBaseStyle, pressedDark, true);
    applyTheme(ui.export_img_btn, iconBaseStyle, pressedDark, true);

    applyTheme(ui.add_landmark_btn, iconBaseStyle, pressedDark, true);
    applyTheme(ui.delete_landmark_btn, iconBaseStyle, pressedDark, true);
    applyTheme(ui.export_landmark_btn, iconBaseStyle, pressedDark, true);

    applyTheme(ui.add_model_btn, iconBaseStyle, pressedDark, true);
    applyTheme(ui.delete_model_btn, iconBaseStyle, pressedDark, true);
    applyTheme(ui.export_model_btn, iconBaseStyle, pressedDark, true);

    applyTheme(ui.add_tool_btn, iconBaseStyle, pressedDark, true);
    applyTheme(ui.delete_tool_btn, iconBaseStyle, pressedDark, true);
    applyTheme(ui.export_tool_btn, iconBaseStyle, pressedDark, true);

    applyTheme(ui.add_transform_btn, iconBaseStyle, pressedDark, true);
    applyTheme(ui.delete_transform_btn, iconBaseStyle, pressedDark, true);
    applyTheme(ui.export_transform_btn, iconBaseStyle, pressedDark, true);
}


void DataManager::initSplitters()
{
    m_vsplitter = new QSplitter(Qt::Vertical, this);

    m_vsplitter->addWidget(ui.operation_box);
    m_vsplitter->addWidget(ui.databox);
    m_vsplitter->addWidget(ui.tabWidget);

    m_vsplitter->setHandleWidth(0);
    m_vsplitter->setContentsMargins(0, 0, 0, 0);
    m_vsplitter->setChildrenCollapsible(true);

    m_vsplitter->setStretchFactor(0, 30);
    m_vsplitter->setStretchFactor(1, 70);
    m_vsplitter->setStretchFactor(2, 60);
    m_vsplitter->setSizes({400, 1400, 1200});

    if (ui.centralwidget->layout()) {
        ui.centralwidget->layout()->setContentsMargins(0, 0, 0, 0);
        ui.centralwidget->layout()->setSpacing(0);
        ui.centralwidget->layout()->replaceWidget(ui.module_widget, m_vsplitter);
    }

    ui.module_widget->hide();
    m_vsplitter->setMouseTracking(true);
}


void DataManager::initDataWidget()
{
    ui.image_tw->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui.mesh_tw->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui.landmark_tw->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    ui.tool_tw->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui.transform_tw->setEditTriggers(QAbstractItemView::NoEditTriggers);   
}


void DataManager::initProperty()
{    
    m_lut2d = vtkSmartPointer<vtkLookupTable>::New();
    m_lut2d->SetRange(m_lower2Dvalue, m_upper2Dvalue);
    m_lut2d->SetValueRange(0.0, 1.0);
    m_lut2d->SetSaturationRange(0.0, 0.0);
    m_lut2d->SetRampToLinear();
    m_lut2d->Build();

    m_ctf3d = vtkSmartPointer<vtkColorTransferFunction>::New();
    m_ctf3d->AddRGBPoint(m_lower3Dvalue, m_volColors[0][0], m_volColors[0][1], m_volColors[0][2]);
    m_ctf3d->AddRGBPoint((m_lower3Dvalue + m_upper3Dvalue) / 2.0, m_volColors[1][0], m_volColors[1][1], m_volColors[1][2]);
    m_ctf3d->AddRGBPoint(m_upper3Dvalue, m_volColors[2][0], m_volColors[2][1], m_volColors[2][2]);

    m_pwf3d = vtkSmartPointer<vtkPiecewiseFunction>::New();
    m_pwf3d->AddPoint(m_lower3Dvalue, 1.0);
    m_pwf3d->AddPoint(m_lower3Dvalue + 1.0, 0.5 * m_volOpacity);
    m_pwf3d->AddPoint((m_lower3Dvalue + m_upper3Dvalue) / 2.0, 0.7 * m_volOpacity);
    m_pwf3d->AddPoint(m_upper3Dvalue - 1.0, 0.8 * m_volOpacity);
    m_pwf3d->AddPoint(m_upper3Dvalue, m_volOpacity);
    m_pwf3d->ClampingOff();

    const QString bgColor = "rgba(40, 44, 52, 255)";
    const QString focusBgColor = "rgba(20, 20, 25, 255)";
    const QString borderColor = "rgba(255, 255, 255, 35)";
    const QString hoverBorderColor = "rgba(255, 255, 255, 100)";
    const QString accentColor = "#00FF7F";
    const QString selectionBg = "rgba(0, 255, 127, 100)";
    const QString iosGreen = "#30D158"; 
    const QString iosText = "#FFFFFF";

    const QString commonStyle = QString(
        "QAbstractSpinBox, QLineEdit, QComboBox {"
        "    background-color: %1;"
        "    border: 1px solid %3;"
        "    border-radius: 4px;"
        "    color: #E0E0E0;"
        "    padding: 4px 8px;"
        "    selection-background-color: %6;"
        "    selection-color: white;"
        "    min-height: 20px;"
        "}"
        "QAbstractSpinBox:hover, QLineEdit:hover, QComboBox:hover {"
        "    border: 1px solid %4;"
        "    background-color: rgba(55, 60, 68, 255);"
        "}"
        "QAbstractSpinBox:focus, QLineEdit:focus, QComboBox:focus {"
        "    border: 1px solid %5;"
        "    background-color: %2;"
        "}"
        "QAbstractSpinBox::up-button, QAbstractSpinBox::down-button {"
        "    background-color: transparent;"
        "    border-left: 1px solid rgba(255, 255, 255, 10);"
        "    width: 16px;"
        "    border-top-right-radius: 4px;"
        "    border-bottom-right-radius: 4px;"
        "}"
        "QAbstractSpinBox::up-button:hover, QAbstractSpinBox::down-button:hover {"
        "    background-color: rgba(255, 255, 255, 20);"
        "}"
        "QAbstractSpinBox::up-arrow, QAbstractSpinBox::down-arrow {"
        "    width: 0px;"
        "    height: 0px;"
        "    border: none;"
        "}"
        "QComboBox::drop-down {"
        "    border: none;"
        "    border-left: 1px solid rgba(255, 255, 255, 10);"
        "    width: 20px;"
        "}"
        "QComboBox::down-arrow {"
        "    width: 0px;"
        "    height: 0px;"
        "    border: none;"
        "}"
        "QComboBox QAbstractItemView {"
        "    background-color: #2D2D33;"
        "    border: 1px solid #454545;"
        "    selection-background-color: %6;"
        "    selection-color: white;"
        "    outline: 0px;"
        "}"
    ).arg(bgColor, focusBgColor, borderColor, hoverBorderColor, accentColor, selectionBg);

    const QString checkboxStyle = QString(
        "QCheckBox {"
        "    color: #E0E0E0;"
        "    spacing: 8px;"
        "}"
        "QCheckBox::indicator {"
        "    width: 16px;"
        "    height: 16px;"
        "    border: 1px solid %1;"
        "    border-radius: 3px;"
        "    background-color: rgba(0, 0, 0, 40);"
        "}"
        "QCheckBox::indicator:hover {"
        "    border: 1px solid %2;"
        "    background-color: rgba(255, 255, 255, 10);"
        "}"
        "QCheckBox::indicator:checked {"
        "    background-color: %3;"
        "    border: 1px solid %3;"
        "}"
        "QCheckBox::indicator:checked:hover {"
        "    background-color: #00CC66;"
        "    border: 1px solid #00CC66;"
        "}"
    ).arg(borderColor, hoverBorderColor, accentColor);

    const QString scrollbarStyle = QString(
        "QScrollBar:vertical {"
        "    border: none;"
        "    background: #18181C;"
        "    width: 10px;"
        "    margin: 0px;"
        "    border-radius: 5px;"
        "}"
        "QScrollBar::handle:vertical {"
        "    background: #4A4A55;"
        "    min-height: 30px;"
        "    border-radius: 3px;"
        "    margin: 2px;"
        "}"
        "QScrollBar::handle:vertical:hover {"
        "    background: #6A6A75;"
        "}"
        "QScrollBar::handle:vertical:pressed {"
        "    background: %1;"
        "}"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
        "    height: 0px;"
        "}"
        "QScrollBar:horizontal {"
        "    border: none;"
        "    background: #18181C;"
        "    height: 10px;"
        "    margin: 0px;"
        "    border-radius: 5px;"
        "}"
        "QScrollBar::handle:horizontal {"
        "    background: #4A4A55;"
        "    min-width: 30px;"
        "    border-radius: 3px;"
        "    margin: 2px;"
        "}"
        "QScrollBar::handle:horizontal:hover {"
        "    background: #6A6A75;"
        "}"
        "QScrollBar::handle:horizontal:pressed {"
        "    background: %1;"
        "}"
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {"
        "    width: 0px;"
        "}"
    ).arg(accentColor);

    const QString toggleStyle = QString(
        "QCheckBox {"
        "    color: %1;"
        "    spacing: 10px;"
        "    font-weight: 500;"
        "}"
        "QCheckBox::indicator {"
        "    width: 36px;"
        "    height: 20px;"
        "    border-radius: 10px;"
        "    border: none;"
        "}"
        "QCheckBox::indicator:unchecked {"
        "    background-color: #39393D;"
        "}"
        "QCheckBox::indicator:checked {"
        "    background-color: %2;"
        "}"
        "QCheckBox::indicator:unchecked:hover {"
        "    background-color: #48484A;"
        "}"
    ).arg(iosText, iosGreen);

    const QString colorBtnStyle = QString(
        "QPushButton {"
        "    background-color: %1;"
        "    color: #000000;"
        "    border-radius: 12px;"
        "    padding: 6px 16px;"
        "    font-weight: 500;"
        "    border: 2px solid transparent;"
        "}"
        "QPushButton:hover {"
        "    border: 2px solid #333333;"
        "}"
        "QPushButton:pressed {"
        "    padding-left: 17px;"
        "    padding-top: 7px;"
        "}"
    ).arg(iosGreen);

    ui.lower2Dbox->setStyleSheet(commonStyle);
    ui.upper2Dbox->setStyleSheet(commonStyle);
    ui.lower3Dbox->setStyleSheet(commonStyle);
    ui.upper3Dbox->setStyleSheet(commonStyle);

    ui.lower2Dbar->setStyleSheet(scrollbarStyle);
    ui.upper2Dbar->setStyleSheet(scrollbarStyle);
    ui.lower3Dbar->setStyleSheet(scrollbarStyle);
    ui.upper3Dbar->setStyleSheet(scrollbarStyle);

    ui.volume_cbox->setStyleSheet(toggleStyle);

    ui.color3D_btn_top->setStyleSheet(colorBtnStyle);
    ui.color3D_btn_bot->setStyleSheet(colorBtnStyle);

    if (m_config.isEmpty() || !m_config.contains("DataManager")) {
        return;
    }

    QJsonObject dmObj = m_config["DataManager"].toObject();

    int minRange = -2048; 
    int maxRange = 3071;

    if (dmObj.contains("range") && dmObj["range"].isObject()) {
        QJsonObject rangeObj = dmObj["range"].toObject();
        minRange = rangeObj["minimum"].toInt(minRange);
        maxRange = rangeObj["maximum"].toInt(maxRange);
    }

    ui.lower2Dbox->setRange(minRange, maxRange);
    ui.lower2Dbar->setRange(minRange, maxRange);
    ui.upper2Dbox->setRange(minRange, maxRange);
    ui.upper2Dbar->setRange(minRange, maxRange);

    ui.lower3Dbox->setRange(minRange, maxRange);
    ui.lower3Dbar->setRange(minRange, maxRange);
    ui.upper3Dbox->setRange(minRange, maxRange);
    ui.upper3Dbar->setRange(minRange, maxRange);

    if (dmObj.contains("thresholds") && dmObj["thresholds"].isObject()) {
        QJsonObject thObj = dmObj["thresholds"].toObject();

        m_lower2Dvalue = thObj["lower_2d_value"].toInt(m_lower2Dvalue);
        m_upper2Dvalue = thObj["upper_2d_value"].toInt(m_upper2Dvalue);
        m_lower3Dvalue = thObj["lower_3d_value"].toInt(m_lower3Dvalue);
        m_upper3Dvalue = thObj["upper_3d_value"].toInt(m_upper3Dvalue);
    }

    ui.lower2Dbox->setValue(m_lower2Dvalue);
    ui.lower2Dbar->setValue(m_lower2Dvalue);
    
    ui.upper2Dbox->setValue(m_upper2Dvalue);
    ui.upper2Dbar->setValue(m_upper2Dvalue);

    ui.lower3Dbox->setValue(m_lower3Dvalue);
    ui.lower3Dbar->setValue(m_lower3Dvalue);

    ui.upper3Dbox->setValue(m_upper3Dvalue);
    ui.upper3Dbar->setValue(m_upper3Dvalue);

    const QString darkComboBoxStyle = 
        "QComboBox {"
        "background-color: rgba(45, 45, 48, 220);"
        "border: 1px solid rgba(255, 255, 255, 30);"
        "border-radius: 15px;"
        "padding: 6px 10px 6px 20px;"
        "color: rgba(255, 255, 255, 230);"
        "font-weight: bold;"
        "}"
        
        "QComboBox:hover {"
        "background-color: rgba(60, 60, 65, 255);"
        "border-color: rgba(255, 255, 255, 80);"
        "}"

        "QComboBox:on {"
        "background-color: rgba(30, 30, 35, 255);"
        "border-color: rgba(255, 255, 255, 100);"
        "}"

        "QComboBox::drop-down {"
        "subcontrol-origin: padding;"
        "subcontrol-position: top right;"
        "width: 30px;"
        "border-left-width: 0px;"
        "border-top-right-radius: 15px;"
        "border-bottom-right-radius: 15px;"
        "background: transparent;"
        "}"
        
        "QComboBox QAbstractItemView {"
        "background-color: rgb(35, 35, 40);"
        "color: rgba(255, 255, 255, 200);"
        "border: 1px solid rgba(80, 80, 80, 255);"
        "selection-background-color: rgba(80, 80, 90, 255);"
        "selection-color: white;"
        "outline: 0;"
        "border-radius: 8px;"
        "padding: 4px;"
        "}";

    if (ui.orientation_cbb) {
        ui.orientation_cbb->setStyleSheet(darkComboBoxStyle);
        ui.orientation_cbb->setView(new QListView());
    }
}


void DataManager::loadConfig()
{
    QString path = QCoreApplication::applicationDirPath() + "/DataManager_config.json";
    QFile file(path);

    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        
        if (doc.isObject()) {
            m_config = doc.object();
            QJsonObject dmConfig = m_config["DataManager"].toObject();
            QJsonObject thresholds = dmConfig["thresholds"].toObject();
            QJsonObject volProp = dmConfig["volume_property"].toObject();
            
            m_volOpacity = volProp["opacity"].toDouble(1.0);

            m_lower2Dvalue = thresholds["lower_2d_value"].toDouble(-800);
            m_upper2Dvalue = thresholds["upper_2d_value"].toDouble(500);
            m_lower3Dvalue = thresholds["lower_3d_value"].toDouble(-600);
            m_upper3Dvalue = thresholds["upper_3d_value"].toDouble(2000);

            QJsonArray colorsArr = volProp["colors"].toArray();
            if (colorsArr.size() >= 3) {
                auto parseColor = [&](const QJsonArray& arr, int idx) {
                    if (arr.size() >= 3) {
                        m_volColors[idx][0] = arr[0].toDouble(1.0);
                        m_volColors[idx][1] = arr[1].toDouble(1.0);
                        m_volColors[idx][2] = arr[2].toDouble(1.0);
                    }
                };
                parseColor(colorsArr[0].toArray(), 0);
                parseColor(colorsArr[1].toArray(), 1);
                parseColor(colorsArr[2].toArray(), 2);
            }
        }
        file.close();
    } else {
        qWarning() << "Failed to load config file:" << path;
    }
}


void DataManager::createActions()
{ 
    connect(ui.add_img_btn, &QPushButton::clicked, this, &DataManager::onAddImage);
    connect(ui.delete_img_btn, &QPushButton::clicked, this, &DataManager::onDeleteImage);
    connect(ui.export_img_btn, &QPushButton::clicked, this, &DataManager::onExportImage);

    connect(ui.add_model_btn, &QPushButton::clicked, this, &DataManager::onAdd3Dmodel);
    connect(ui.delete_model_btn, &QPushButton::clicked, this, &DataManager::onDelete3Dmodel);
    connect(ui.export_model_btn, &QPushButton::clicked, this, &DataManager::onExport3Dmodel);

    connect(ui.add_landmark_btn, &QPushButton::clicked, this, &DataManager::onAddLandmark);
    connect(ui.delete_landmark_btn, &QPushButton::clicked, this, &DataManager::onDeleteLandmark);
    connect(ui.export_landmark_btn, &QPushButton::clicked, this, &DataManager::onExportLandmark);

    connect(ui.add_tool_btn, &QPushButton::clicked, this, &DataManager::onAddTool);
    connect(ui.delete_tool_btn, &QPushButton::clicked, this, &DataManager::onDeleteTool);
    connect(ui.export_tool_btn, &QPushButton::clicked, this, &DataManager::onExportTool);

    connect(ui.add_transform_btn, &QPushButton::clicked, this, &DataManager::onAddOmniTransform);
    connect(ui.delete_transform_btn, &QPushButton::clicked, this, &DataManager::onDeleteOmniTransform);
    connect(ui.export_transform_btn, &QPushButton::clicked, this, &DataManager::onExportOmniTransform);
    
    connect(ui.landmark_tw, &QTableWidget::itemChanged, this, &DataManager::onEditLandmark);
    connect(ui.tool_tw, &QTableWidget::itemChanged, this, &DataManager::onEditTool);
    connect(ui.transform_tw, &QTableWidget::itemChanged, this, &DataManager::onEditOmniTransform);
    connect(ui.image_tw, &QTableWidget::itemChanged, this, &DataManager::onImageTableItemChanged);
    connect(ui.image_tw, &QTableWidget::itemSelectionChanged, this, &DataManager::onImageTableSelectionChanged);

    connect(ui.lower2Dbox, QOverload<int>::of(&QSpinBox::valueChanged), this, &DataManager::onLower2DChanged);
    connect(ui.lower2Dbar, &QSlider::valueChanged, this, &DataManager::onLower2DChanged);

    connect(ui.upper2Dbox, QOverload<int>::of(&QSpinBox::valueChanged), this, &DataManager::onUpper2DChanged);
    connect(ui.upper2Dbar, &QSlider::valueChanged, this, &DataManager::onUpper2DChanged);

    connect(ui.lower3Dbox, QOverload<int>::of(&QSpinBox::valueChanged), this, &DataManager::onLower3DChanged);
    connect(ui.lower3Dbar, &QSlider::valueChanged, this, &DataManager::onLower3DChanged);

    connect(ui.upper3Dbox, QOverload<int>::of(&QSpinBox::valueChanged), this, &DataManager::onUpper3DChanged);
    connect(ui.upper3Dbar, &QSlider::valueChanged, this, &DataManager::onUpper3DChanged);

    connect(ui.volume_cbox, &QCheckBox::toggled, this, &DataManager::onVolumeCboxChanged);

    connect(ui.color3D_btn_top, &QPushButton::clicked, this, &DataManager::onTop3DColorBtnClicked);
    connect(ui.color3D_btn_bot, &QPushButton::clicked, this, &DataManager::onBottom3DColorBtnClicked);

    connect(ui.image_tw, &QTableWidget::itemDoubleClicked, this, &DataManager::onImageTableDoubleClicked);

    connect(this, &DataManager::signalAddTool, [=](Tool* newTool) {
        if (!newTool) return;
        QString newToolName = QString::fromStdString(newTool->getName());
        for (int row = 0; row < ui.mesh_tw->rowCount(); ++row) {
            QWidget* container = ui.mesh_tw->cellWidget(row, 4);
            if (!container) continue;
            QComboBox* cbb = container->findChild<QComboBox*>();
            if (cbb) {
                if (cbb->findText(newToolName) == -1) {
                    cbb->addItem(newToolName);
                }
            }
        }

        for (int row = 0; row < ui.landmark_tw->rowCount(); ++row) {
            QWidget* container = ui.landmark_tw->cellWidget(row, 6);
            if (!container) continue;
            QComboBox* cbb = container->findChild<QComboBox*>();
            if (cbb) {
                if (cbb->findText(newToolName) == -1) {
                    cbb->addItem(newToolName);
                }
            }
        }


    });

    connect(this, &DataManager::signalDeleteTool, [=](Tool* deletedTool) {
        if (!deletedTool) return;
        QString toolName = QString::fromStdString(deletedTool->getName());
        for (int row = 0; row < ui.mesh_tw->rowCount(); ++row) {
            QWidget* container = ui.mesh_tw->cellWidget(row, 4);
            if (!container) continue;
            QComboBox* cbb = container->findChild<QComboBox*>();
            if (cbb) {
                int index = cbb->findText(toolName);
                if (index != -1) {
                    if (cbb->currentIndex() == index) {
                        cbb->setCurrentIndex(0); 
                    }
                    cbb->removeItem(index);
                }
            }
        }

        for (int row = 0; row < ui.landmark_tw->rowCount(); ++row) {
            QWidget* container = ui.landmark_tw->cellWidget(row, 6);
            if (!container) continue;
            QComboBox* cbb = container->findChild<QComboBox*>();
            if (cbb) {
                int index = cbb->findText(toolName);
                if (index != -1) {
                    if (cbb->currentIndex() == index) {
                        cbb->setCurrentIndex(0); 
                    }
                    cbb->removeItem(index);
                }
            }
        }
    });




}


bool DataManager::addImage(std::string filePath)
{
    // 注意：这里使用了你代码片段中传入 orientation 的构造函数
    Image* newImage = new Image(filePath, ui.orientation_cbb->currentText().toStdString());
    
    if (!newImage->getImageData()) {
        QMessageBox::warning(this, "Error", "Failed to load image data.");
        delete newImage;
        return false;
    }

    newImage->createActors(m_lut2d, m_ctf3d, m_pwf3d);
    m_images.push_back(newImage);

    int row = ui.image_tw->rowCount();
    ui.image_tw->insertRow(row);

    // Column 0: Name
    QTableWidgetItem* nameItem = new QTableWidgetItem(QString::fromStdString(newImage->getName()));
    nameItem->setCheckState(Qt::Checked);
    nameItem->setTextAlignment(Qt::AlignCenter);
    nameItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
    ui.image_tw->setItem(row, 0, nameItem);

    // Column 1: Age
    QTableWidgetItem* ageItem = new QTableWidgetItem(QString::fromStdString(newImage->getAge()));
    ageItem->setTextAlignment(Qt::AlignCenter);
    ageItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    ui.image_tw->setItem(row, 1, ageItem);

    // Column 2: Resolution
    const int* dim = newImage->getResolution();
    QString resStr = QString("%1, %2, %3").arg(dim[0]).arg(dim[1]).arg(dim[2]);
    QTableWidgetItem* resItem = new QTableWidgetItem(resStr);
    resItem->setTextAlignment(Qt::AlignCenter);
    resItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    ui.image_tw->setItem(row, 2, resItem);

    // Column 3: Spacing
    const double* sp = newImage->getSpacing();
    QString spacingStr = QString("%1, %2, %3").arg(sp[0], 0, 'f', 2).arg(sp[1], 0, 'f', 2).arg(sp[2], 0, 'f', 2);
    QTableWidgetItem* spacingItem = new QTableWidgetItem(spacingStr);
    spacingItem->setTextAlignment(Qt::AlignCenter);
    spacingItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    ui.image_tw->setItem(row, 3, spacingItem);

    // =========================================================
    // Column 4: Orientation (新增列)
    // =========================================================
    QTableWidgetItem* orientItem = new QTableWidgetItem(QString::fromStdString(newImage->getOrientation()));
    orientItem->setTextAlignment(Qt::AlignCenter);
    orientItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    ui.image_tw->setItem(row, 4, orientItem);

    // =========================================================
    // Column 5: Path (原 Column 4 移动到这里)
    // =========================================================
    QTableWidgetItem* pathItem = new QTableWidgetItem(QString::fromStdString(newImage->getPath()));
    pathItem->setTextAlignment(Qt::AlignCenter);
    pathItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    pathItem->setToolTip(QString::fromStdString(newImage->getPath()));
    ui.image_tw->setItem(row, 5, pathItem); // 注意这里变成了 5

    ui.image_tw->selectRow(row);
    m_currentImageIndex = row;

    ui.image_tw->resizeColumnsToContents();

    return true;
}


bool DataManager::deleteImage(int index)
{
    if (index < 0 || index >= m_images.size()) return false;

    Image* img = m_images[index];

    emit signalDeleteImage(img);

    delete img;
    m_images.erase(m_images.begin() + index);

    ui.image_tw->removeRow(index);

    if (m_images.empty()) {
        m_currentImageIndex = -1;
    } else {
        if (m_currentImageIndex > index) {
            m_currentImageIndex--;
        }
        if (m_currentImageIndex >= m_images.size()) {
            m_currentImageIndex = m_images.size() - 1;
        }
        ui.image_tw->selectRow(m_currentImageIndex);
    }

    return true;
}


bool DataManager::exportImage(int index, std::string savePath)
{
    if (index < 0 || index >= m_images.size()) return false;

    vtkSmartPointer<vtkImageData> imageData = m_images[index]->getImageData();
    if (!imageData) return false;

    if (m_images[index]->getOrientation() == "RAS")
    {
        int dims[3];
        double spacing[3];
        double origin[3];
        imageData->GetDimensions(dims);
        imageData->GetSpacing(spacing);
        imageData->GetOrigin(origin);

        vtkSmartPointer<vtkImageFlip> flipX = vtkSmartPointer<vtkImageFlip>::New();
        flipX->SetInputData(imageData);
        flipX->SetFilteredAxis(0);
        flipX->Update();
        
        vtkSmartPointer<vtkImageFlip> flipY = vtkSmartPointer<vtkImageFlip>::New();
        flipY->SetInputData(flipX->GetOutput());
        flipY->SetFilteredAxis(1);
        flipY->Update();

        vtkSmartPointer<vtkImageData> finalImage = flipY->GetOutput();

        double maxBounds[3];
        maxBounds[0] = origin[0] + (dims[0] - 1) * spacing[0];
        maxBounds[1] = origin[1] + (dims[1] - 1) * spacing[1];
        maxBounds[2] = origin[2];

        double newOrigin[3];
        newOrigin[0] = -maxBounds[0];
        newOrigin[1] = -maxBounds[1];
        newOrigin[2] = origin[2];

        finalImage->SetOrigin(newOrigin);
        imageData = finalImage;
    }

    std::string ext = vtksys::SystemTools::GetFilenameLastExtension(savePath);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == ".nii" || ext == ".nii.gz") {
        vtkSmartPointer<vtkNIFTIImageWriter> writer = vtkSmartPointer<vtkNIFTIImageWriter>::New();
        writer->SetFileName(savePath.c_str());
        writer->SetInputData(imageData);
        writer->SetQFac(m_images[index]->getSpacing()[2] < 0 ? -1 : 1);
        writer->Write();
        return true;
    }
    else if (ext == ".mhd" || ext == ".mha") {
        vtkSmartPointer<vtkMetaImageWriter> writer = vtkSmartPointer<vtkMetaImageWriter>::New();
        writer->SetFileName(savePath.c_str());
        writer->SetInputData(imageData);
        writer->Write();
        return true;
    }

    return false;
}


void DataManager::onAddImage()
{
    QString path = QFileDialog::getExistingDirectory(
        this, 
        tr("Open DICOM Directory"), 
        QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );

    if (path.isEmpty()) {
        path = QFileDialog::getOpenFileName(
            this,
            tr("Open Image File"),
            QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
            tr("Medical Images (*.dcm *.nii *.nii.gz *.nrrd *.mhd)")
        );
    }

    if (!path.isEmpty()) {
        if (addImage(path.toStdString()))
            printInfo("Open \"" + path + "\"");
        else
            printInfo("\"" + path + "\" is invalid!");
    }
    else {
        printInfo("Open canceled (No file selected)!");
    }
}

void DataManager::onDeleteImage()
{
    int currentRow = ui.image_tw->currentRow();
    if (currentRow >= 0) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Delete Image", "Are you sure you want to delete this image?",
                                      QMessageBox::Yes|QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            deleteImage(currentRow);
        }
    } else {
        QMessageBox::information(this, "Info", "Please select an image to delete.");
    }
}

void DataManager::onExportImage()
{
    int currentRow = ui.image_tw->currentRow();
    if (currentRow < 0) {
        QMessageBox::information(this, "Info", "Please select an image to export.");
        return;
    }

    QString savePath = QFileDialog::getSaveFileName(
        this,
        tr("Save Image"),
        QString::fromStdString(m_images[currentRow]->getName()),
        tr("NIfTI Files (*.nii.gz);;NRRD Files (*.nrrd)")
    );

    if (!savePath.isEmpty()) {
        if (exportImage(currentRow, savePath.toStdString())) {
            QMessageBox::information(this, "Success", "Image exported successfully.");
        } else {
            QMessageBox::critical(this, "Error", "Failed to export image.");
        }
    }
}

void DataManager::onImageTableItemChanged(QTableWidgetItem* item)
{
    if (item->column() == 0) {
        int row = item->row();
        if (row >= 0 && row < m_images.size()) {
            bool visible = (item->checkState() == Qt::Checked);
            auto actors = m_images[row]->getActors();
            for (auto actor : actors) {
                actor->SetVisibility(visible);
            }
        }
    }
    emit signalUpdateViews();
}

void DataManager::onImageTableSelectionChanged()
{
    int currentRow = ui.image_tw->currentRow();
    if (currentRow >= 0) {
        m_currentImageIndex = currentRow;
    }
}


bool DataManager::add3Dmodel(std::string filePath, std::string orientation_sign)
{
    // 1. 创建模型对象
    if (orientation_sign.empty()) {
        orientation_sign = ui.orientation_cbb->currentText().toStdString();
    }

    Model3D* newMesh = new Model3D(filePath, orientation_sign);
    if (!newMesh->getPolydata()) {
        delete newMesh;
        return false;
    }

    QFileInfo fileInfo(QString::fromStdString(filePath));
    newMesh->setFilePath(filePath);
    newMesh->setName(fileInfo.fileName().toStdString());

    // 2. 读取配置 (透明度、可见性、颜色循环)
    QJsonObject dmConfig = m_config["DataManager"].toObject();
    QJsonObject meshConfig = dmConfig["meshes"].toObject();

    newMesh->setOpacity(meshConfig["opacity"].toDouble());
    
    int visibleVal = meshConfig["visible"].toInt();
    newMesh->setVisible(visibleVal != 0);

    QJsonArray colorList = meshConfig["colors"].toArray();
    double r, g, b;
    if (!colorList.isEmpty()) {
        int index = m_meshes.size() % colorList.size();
        QJsonArray rgb = colorList[index].toArray();
        r = rgb[0].toDouble();
        g = rgb[1].toDouble();
        b = rgb[2].toDouble();
        double c[3] = { r, g, b };
        newMesh->setColor(c);
    } else {
        r = 1.0; g = 1.0; b = 1.0;
        double c[3] = { r, g, b };
        newMesh->setColor(c);
    }

    newMesh->createActor();
    m_meshes.push_back(newMesh);

    // 3. 在表格中插入新行
    int row = ui.mesh_tw->rowCount();
    ui.mesh_tw->insertRow(row);

    // ---------------------------------------------------------
    // Column 0: Name
    // ---------------------------------------------------------
    QTableWidgetItem* nameItem = new QTableWidgetItem(QString::fromStdString(newMesh->getName()));
    nameItem->setTextAlignment(Qt::AlignCenter);
    nameItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    ui.mesh_tw->setItem(row, 0, nameItem);

    // ---------------------------------------------------------
    // Column 1: Visible (Button)
    // ---------------------------------------------------------
    QWidget* visContainer = new QWidget();
    visContainer->setStyleSheet("background-color: transparent;");
    QHBoxLayout* visLayout = new QHBoxLayout(visContainer);
    visLayout->setContentsMargins(0, 0, 0, 0);
    visLayout->setAlignment(Qt::AlignCenter);
    
    QPushButton* visBtn = new QPushButton();
    visBtn->setFlat(true);
    visBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    bool isVis = newMesh->getVisible();
    
    // 使用你代码中的样式定义
    QString visStyle = "QPushButton { image: url(:/DataManager/visible.png); border: none; }";
    QString unvisStyle = "QPushButton { image: url(:/DataManager/unvisible.png); border: none; }";

    visBtn->setStyleSheet(isVis ? visStyle : unvisStyle);

    connect(visBtn, &QPushButton::clicked, [=]() {
        bool currentState = newMesh->getVisible();
        bool newState = !currentState;
        newMesh->setVisible(newState);
        
        visBtn->setStyleSheet(newState ? visStyle : unvisStyle);
        
        emit signalUpdateViews();
    });

    visLayout->addWidget(visBtn);
    ui.mesh_tw->setCellWidget(row, 1, visContainer);

    // ---------------------------------------------------------
    // Column 2: Opacity (Slider)
    // ---------------------------------------------------------
    QWidget* opContainer = new QWidget();
    opContainer->setStyleSheet("background-color: transparent;");
    QHBoxLayout* opLayout = new QHBoxLayout(opContainer);
    opLayout->setContentsMargins(4, 0, 4, 0);
    opLayout->setAlignment(Qt::AlignCenter);
    
    QSlider* slider = new QSlider(Qt::Horizontal);
    slider->setRange(0, 100);
    slider->setValue(static_cast<int>(newMesh->getOpacity() * 100));
    slider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    
    connect(slider, &QSlider::valueChanged, [=](int value) {
        double opacity = value / 100.0;
        newMesh->setOpacity(opacity);
        emit signalUpdateViews();
    });

    opLayout->addWidget(slider);
    ui.mesh_tw->setCellWidget(row, 2, opContainer);

    // ---------------------------------------------------------
    // Column 3: Color (Button)
    // ---------------------------------------------------------
    QWidget* colorContainer = new QWidget();
    QHBoxLayout* colorLayout = new QHBoxLayout(colorContainer);
    colorContainer->setStyleSheet("background-color: transparent;");
    colorLayout->setContentsMargins(2, 2, 2, 2);
    colorLayout->setAlignment(Qt::AlignCenter);

    QPushButton* colorBtn = new QPushButton();
    colorBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QString colorStyle = QString("background-color: rgb(%1, %2, %3); border: 1px solid gray; border-radius: 4px;")
        .arg(r * 255).arg(g * 255).arg(b * 255);
    colorBtn->setStyleSheet(colorStyle);

    connect(colorBtn, &QPushButton::clicked, [=]() {
        const double* init_c = newMesh->getColor();
        QColor initColor = QColor::fromRgbF(init_c[0], init_c[1], init_c[2]);

        QColor newColor = QColorDialog::getColor(initColor, this, "Select Model Color");

        if (newColor.isValid()) {
            double rgb[3] = { newColor.redF(), newColor.greenF(), newColor.blueF() };
            newMesh->setColor(rgb);
            QString newStyle = QString("background-color: %1; border: 1px solid gray; border-radius: 4px;").arg(newColor.name());
            colorBtn->setStyleSheet(newStyle);
        }
        emit signalUpdateViews();
        });

    colorLayout->addWidget(colorBtn);
    ui.mesh_tw->setCellWidget(row, 3, colorContainer);


    // ---------------------------------------------------------
    // Column 4: Tool (ComboBox) [新增功能]
    // ---------------------------------------------------------
    QWidget* toolContainer = new QWidget();
    toolContainer->setStyleSheet("background-color: transparent;");
    QHBoxLayout* toolLayout = new QHBoxLayout(toolContainer);
    toolLayout->setContentsMargins(2, 0, 2, 0); 
    toolLayout->setAlignment(Qt::AlignCenter);

    QComboBox* toolCbb = new QComboBox();
    toolCbb->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    
    // 1. 添加默认选项 None (Index 0)
    toolCbb->addItem("None"); 

    // 2. 遍历 m_tools 添加选项
    // 假设 m_tools 存储的是 Tool* 指针
    for (const auto& tool : m_tools) {
        if (tool) {
            toolCbb->addItem(QString::fromStdString(tool->getName())); 
        }
    }

    toolLayout->addWidget(toolCbb);
    ui.mesh_tw->setCellWidget(row, 4, toolContainer); // 设置在第 4 列

    // ---------------------------------------------------------
    // Column 5: Orientation (Text)
    // ---------------------------------------------------------
    QTableWidgetItem* orientItem = new QTableWidgetItem(QString::fromStdString(newMesh->getOrientation()));
    orientItem->setTextAlignment(Qt::AlignCenter);
    orientItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    ui.mesh_tw->setItem(row, 5, orientItem);

    // ---------------------------------------------------------
    // Column 6: Path (Text) [原 Column 4]
    // ---------------------------------------------------------
    QTableWidgetItem* pathItem = new QTableWidgetItem(QString::fromStdString(filePath));
    pathItem->setTextAlignment(Qt::AlignCenter);
    pathItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    pathItem->setToolTip(QString::fromStdString(filePath));
    ui.mesh_tw->setItem(row, 6, pathItem); // 移动到第 6 列

    ui.mesh_tw->resizeColumnsToContents();

    emit signalAddMesh(newMesh);

    return true;
}


bool DataManager::delete3Dmodel(int index)
{
    if (index < 0 || index >= m_meshes.size()) return false;

    Model3D* model = m_meshes[index];

    emit signalDeleteMesh(model);

    ui.mesh_tw->removeRow(index);

    delete model;
    m_meshes.erase(m_meshes.begin() + index);

    return true;
}

bool DataManager::export3Dmodel(int index, std::string savePath)
{
    if (index < 0 || index >= m_meshes.size()) return false;

    vtkSmartPointer<vtkPolyData> polyData = m_meshes[index]->getPolydata();
    if (!polyData) return false;

    if (m_meshes[index]->getOrientation() == "RAS")
    {
        vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
        transform->RotateZ(180.0);

        vtkSmartPointer<vtkTransformPolyDataFilter> filter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
        filter->SetInputData(polyData);
        filter->SetTransform(transform);
        filter->Update();

        polyData = filter->GetOutput();
    }

    std::string ext = vtksys::SystemTools::GetFilenameLastExtension(savePath);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == ".stl") {
        vtkSmartPointer<vtkSTLWriter> writer = vtkSmartPointer<vtkSTLWriter>::New();
        writer->SetFileName(savePath.c_str());
        writer->SetInputData(polyData);
        writer->Write();
        return true;
    }
    else if (ext == ".obj") {
        vtkSmartPointer<vtkOBJWriter> writer = vtkSmartPointer<vtkOBJWriter>::New();
        writer->SetFileName(savePath.c_str());
        writer->SetInputData(polyData);
        writer->Write();
        return true;
    }
    else if (ext == ".ply") {
        vtkSmartPointer<vtkPLYWriter> writer = vtkSmartPointer<vtkPLYWriter>::New();
        writer->SetFileName(savePath.c_str());
        writer->SetInputData(polyData);
        writer->Write();
        return true;
    }
    else if (ext == ".vtk") {
        vtkSmartPointer<vtkPolyDataWriter> writer = vtkSmartPointer<vtkPolyDataWriter>::New();
        writer->SetFileName(savePath.c_str());
        writer->SetInputData(polyData);
        writer->Write();
        return true;
    }

    return false;
}

void DataManager::onAdd3Dmodel()
{
    QString path = QFileDialog::getOpenFileName(
        this,
        tr("Open 3D Model File"),
        QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
        tr("3D Models (*.stl *.obj *.ply *.vtk *.vtp *.g)")
    );

    if (!path.isEmpty()) {
        if (add3Dmodel(path.toStdString()))
            printInfo("Open \"" + path + "\"");
        else
            printInfo("\"" + path + "\" is invalid!");
    }
    else {
        printInfo("Open canceled (No file selected)!");
    }
}

void DataManager::onDelete3Dmodel()
{
    int currentRow = ui.mesh_tw->currentRow();
    if (currentRow >= 0) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Delete Model", "Are you sure you want to delete this 3D model?",
                                      QMessageBox::Yes|QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            delete3Dmodel(currentRow);
        }
    } else {
        QMessageBox::information(this, "Info", "Please select a model to delete.");
    }
}

void DataManager::onExport3Dmodel()
{
    int currentRow = ui.mesh_tw->currentRow();
    if (currentRow < 0) {
        QMessageBox::information(this, "Info", "Please select a model to export.");
        return;
    }

    QString savePath = QFileDialog::getSaveFileName(
        this,
        tr("Save 3D Model"),
        QString::fromStdString(m_meshes[currentRow]->getName()),
        tr("STL Files (*.stl);;OBJ Files (*.obj);;PLY Files (*.ply);;VTK Files (*.vtk)")
    );

    if (!savePath.isEmpty()) {
        if (export3Dmodel(currentRow, savePath.toStdString())) {
            QMessageBox::information(this, "Success", "Model exported successfully.");
        } else {
            QMessageBox::critical(this, "Error", "Failed to export model.");
        }
    }
}


bool DataManager::addLandmark(double* coordinates, std::string name, std::string orientation_sign)
{
    double defaultCoords[3] = {0.0, 0.0, 0.0};
    if (coordinates == nullptr) {
        coordinates = defaultCoords;
    }

    if (name.empty()) {
        name = "Manually add";
    }

    if (orientation_sign.empty()) {
        orientation_sign = ui.orientation_cbb->currentText().toStdString();
    }

    Landmark* newLandmark = new Landmark(coordinates, name, orientation_sign);
    
    if (!newLandmark->getActor()) {
        delete newLandmark;
        return false;
    }

    QJsonObject dmConfig = m_config["DataManager"].toObject();
    QJsonObject lmConfig = dmConfig["landmarks"].toObject();

    double initRadius = lmConfig["radius"].toDouble(2.0);
    newLandmark->setRadius(initRadius);
    newLandmark->setOpacity(lmConfig["opacity"].toDouble(1.0));
    newLandmark->setVisible(lmConfig["visible"].toInt(1) != 0);

    QJsonArray colorList = lmConfig["colors"].toArray();
    double r, g, b;
    if (!colorList.isEmpty()) {
        int index = m_landmarks.size() % colorList.size();
        QJsonArray rgb = colorList[index].toArray();
        r = rgb[0].toDouble();
        g = rgb[1].toDouble();
        b = rgb[2].toDouble();
    } else {
        r = 1.0; g = 0.0; b = 0.0;
    }
    double c[3] = { r, g, b };
    newLandmark->setColor(c);

    m_landmarks.push_back(newLandmark);

    int row = ui.landmark_tw->rowCount();
    ui.landmark_tw->insertRow(row);

    // Column 0: Point set (Name)
    QTableWidgetItem* nameItem = new QTableWidgetItem(QString::fromStdString(newLandmark->getPointset()));
    nameItem->setTextAlignment(Qt::AlignCenter);
    nameItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
    ui.landmark_tw->setItem(row, 0, nameItem);

    // Column 1: Scalar (Coordinates)
    QString coordStr = QString("%1, %2, %3")
        .arg(newLandmark->getCoordinates()[0], 0, 'f', 2)
        .arg(newLandmark->getCoordinates()[1], 0, 'f', 2)
        .arg(newLandmark->getCoordinates()[2], 0, 'f', 2);
    QTableWidgetItem* scalarItem = new QTableWidgetItem(coordStr);
    scalarItem->setTextAlignment(Qt::AlignCenter);
    scalarItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
    ui.landmark_tw->setItem(row, 1, scalarItem);

    // Column 2: Color (Widget)
    QWidget* colorContainer = new QWidget();
    QHBoxLayout* colorLayout = new QHBoxLayout(colorContainer);
    colorContainer->setStyleSheet("background-color: transparent;");
    colorLayout->setContentsMargins(4, 4, 4, 4);
    colorLayout->setAlignment(Qt::AlignCenter);

    QPushButton* colorBtn = new QPushButton();
    colorBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QString colorStyle = QString("background-color: rgb(%1, %2, %3); border: 1px solid gray; border-radius: 4px;")
        .arg(r * 255).arg(g * 255).arg(b * 255);
    colorBtn->setStyleSheet(colorStyle);

    connect(colorBtn, &QPushButton::clicked, [=]() {
        const double* init_c = newLandmark->getColor();
        QColor initColor = QColor::fromRgbF(init_c[0], init_c[1], init_c[2]);
        QColor newColor = QColorDialog::getColor(initColor, this, "Select Landmark Color");

        if (newColor.isValid()) {
            double rgb[3] = { newColor.redF(), newColor.greenF(), newColor.blueF() };
            newLandmark->setColor(rgb);
            colorBtn->setStyleSheet(QString("background-color: %1; border: 1px solid gray; border-radius: 4px;").arg(newColor.name()));
            emit signalUpdateViews();
        }
    });
    colorLayout->addWidget(colorBtn);
    ui.landmark_tw->setCellWidget(row, 2, colorContainer);

    // Column 3: Visible (Widget)
    QWidget* visContainer = new QWidget();
    visContainer->setStyleSheet("background-color: transparent;");
    QHBoxLayout* visLayout = new QHBoxLayout(visContainer);
    visLayout->setContentsMargins(0, 0, 0, 0);
    visLayout->setAlignment(Qt::AlignCenter);
    
    QPushButton* visBtn = new QPushButton();
    visBtn->setFlat(true);
    visBtn->setFixedSize(24, 24);
    bool isVis = newLandmark->getVisible();
    QString visStyle = "QPushButton { image: url(:/DataManager/visible.png); border: none; }";
    QString unvisStyle = "QPushButton { image: url(:/DataManager/unvisible.png); border: none; }";
    visBtn->setStyleSheet(isVis ? visStyle : unvisStyle);

    connect(visBtn, &QPushButton::clicked, [=]() {
        bool currentState = newLandmark->getVisible();
        newLandmark->setVisible(!currentState);
        visBtn->setStyleSheet(!currentState ? visStyle : unvisStyle);
        emit signalUpdateViews();
    });
    visLayout->addWidget(visBtn);
    ui.landmark_tw->setCellWidget(row, 3, visContainer);

    // Column 4: Opacity (Widget)
    QWidget* opContainer = new QWidget();
    opContainer->setStyleSheet("background-color: transparent;");
    QHBoxLayout* opLayout = new QHBoxLayout(opContainer);
    opLayout->setContentsMargins(4, 0, 4, 0);
    opLayout->setAlignment(Qt::AlignCenter);
    
    QSlider* slider = new QSlider(Qt::Horizontal);
    slider->setRange(0, 100);
    slider->setValue(static_cast<int>(newLandmark->getOpacity() * 100));
    slider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    
    connect(slider, &QSlider::valueChanged, [=](int value) {
        newLandmark->setOpacity(value / 100.0);
        emit signalUpdateViews();
    });
    opLayout->addWidget(slider);
    ui.landmark_tw->setCellWidget(row, 4, opContainer);

    // Column 5: Radius (Widget)
    QWidget* radContainer = new QWidget();
    radContainer->setStyleSheet("background-color: transparent;");
    QHBoxLayout* radLayout = new QHBoxLayout(radContainer);
    radLayout->setContentsMargins(2, 0, 2, 0);
    radLayout->setAlignment(Qt::AlignCenter);

    QDoubleSpinBox* radSpinBox = new QDoubleSpinBox();
    radSpinBox->setRange(0.1, 100.0);
    radSpinBox->setSingleStep(0.5);
    radSpinBox->setValue(initRadius);
    radSpinBox->setDecimals(1);
    radSpinBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    connect(radSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [=](double value){
        newLandmark->setRadius(value);
        emit signalUpdateViews();
    });

    radLayout->addWidget(radSpinBox);
    ui.landmark_tw->setCellWidget(row, 5, radContainer);

    // =========================================================
    // Column 6: Tool (ComboBox) - [Group Synchronization Added]
    // =========================================================
    QWidget* toolContainer = new QWidget();
    toolContainer->setStyleSheet("background-color: transparent;");
    QHBoxLayout* toolLayout = new QHBoxLayout(toolContainer);
    toolLayout->setContentsMargins(2, 0, 2, 0); 
    toolLayout->setAlignment(Qt::AlignCenter);

    QComboBox* toolCbb = new QComboBox();
    toolCbb->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    
    toolCbb->addItem("None"); 

    for (const auto& tool : m_tools) {
        if (tool) {
            toolCbb->addItem(QString::fromStdString(tool->getName())); 
        }
    }

    connect(toolCbb, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index) {
        // 1. 找到当前触发信号的行和其 Point set Name
        int currentRow = -1;
        QString currentPointSet;

        for(int i = 0; i < ui.landmark_tw->rowCount(); ++i) {
            QWidget* widget = ui.landmark_tw->cellWidget(i, 6);
            if(widget) {
                QComboBox* cb = widget->findChild<QComboBox*>();
                if(cb == toolCbb) {
                    currentRow = i;
                    if(ui.landmark_tw->item(i, 0)) {
                        currentPointSet = ui.landmark_tw->item(i, 0)->text();
                    }
                    break;
                }
            }
        }

        if(currentRow == -1 || currentPointSet.isEmpty()) return;

        // 2. 遍历表格，同步所有相同 Point set 的下拉框
        for(int i = 0; i < ui.landmark_tw->rowCount(); ++i) {
            if(i == currentRow) continue; 

            if(ui.landmark_tw->item(i, 0) && ui.landmark_tw->item(i, 0)->text() == currentPointSet) {
                QWidget* widget = ui.landmark_tw->cellWidget(i, 6);
                if(widget) {
                    QComboBox* otherCb = widget->findChild<QComboBox*>();
                    if(otherCb) {
                        otherCb->blockSignals(true); 
                        otherCb->setCurrentIndex(index);
                        otherCb->blockSignals(false);
                    }
                }
            }
        }
    });

    toolLayout->addWidget(toolCbb);
    ui.landmark_tw->setCellWidget(row, 6, toolContainer);

    // Column 7: Orientation
    QTableWidgetItem* orientItem = new QTableWidgetItem(QString::fromStdString(newLandmark->getOrientation()));
    orientItem->setTextAlignment(Qt::AlignCenter);
    orientItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    ui.landmark_tw->setItem(row, 7, orientItem);

    ui.landmark_tw->resizeColumnsToContents();

    emit signalAddLandmark(newLandmark);
    return true;
}

void DataManager::onAddLandmark()
{
    double defaultCoords[3] = { 0.0, 0.0, 0.0 };
    addLandmark(defaultCoords);
}

bool DataManager::deleteLandmark(int index)
{
    if (index < 0 || index >= m_landmarks.size()) return false;

    Landmark* lm = m_landmarks[index];
    emit signalDeleteLandmark(lm);
    ui.landmark_tw->removeRow(index);

    delete lm;
    m_landmarks.erase(m_landmarks.begin() + index);

    emit signalUpdateViews();
    return true;
}

void DataManager::onDeleteLandmark()
{
    int currentRow = ui.landmark_tw->currentRow();
    if (currentRow >= 0) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Delete Landmark", 
            "Are you sure you want to delete this landmark?",
            QMessageBox::Yes | QMessageBox::No);
            
        if (reply == QMessageBox::Yes) {
            deleteLandmark(currentRow);
        }
    } else {
        QMessageBox::information(this, "Info", "Please select a landmark to delete.");
    }
}

bool DataManager::exportLandmark(std::string savePath)
{
    if (m_landmarks.empty()) return false;

    std::string ext = vtksys::SystemTools::GetFilenameLastExtension(savePath);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == ".json") {
        QFile file(QString::fromStdString(savePath));
        if (file.open(QIODevice::WriteOnly)) {
            
            QMap<QString, QJsonArray> groupedPoints;

            for (auto* lm : m_landmarks) {
                QString groupName = QString::fromStdString(lm->getPointset());
                
                const double* rawCoords = lm->getCoordinates();
                QJsonArray pointCoord;

                if (lm->getOrientation() == "RAS") {
                    pointCoord.append(-rawCoords[0]); 
                    pointCoord.append(-rawCoords[1]); 
                    pointCoord.append(rawCoords[2]);
                } else {
                    pointCoord.append(rawCoords[0]); 
                    pointCoord.append(rawCoords[1]); 
                    pointCoord.append(rawCoords[2]);
                }

                groupedPoints[groupName].append(pointCoord);
            }

            QJsonObject landmarkData;
            for (auto it = groupedPoints.begin(); it != groupedPoints.end(); ++it) {
                landmarkData.insert(it.key(), it.value());
            }

            QJsonObject rootObj;
            rootObj.insert("Landmark", landmarkData);

            QJsonDocument doc(rootObj);
            file.write(doc.toJson(QJsonDocument::Indented)); 
            file.close();
            return true;
        }
    }
    
    return false;
}


void DataManager::onExportLandmark()
{
    if (m_landmarks.empty()) {
        QMessageBox::warning(this, "Export Warning", "No landmarks to export.");
        return;
    }

    QString defaultName = "All_Landmarks";

    QString filePath = QFileDialog::getSaveFileName(
        this,
        "Export All Landmarks",
        defaultName,
        "JSON Files (*.json);;STL Files (*.stl);;VTK Files (*.vtk)"
    );

    if (filePath.isEmpty()) {
        return;
    }

    bool success = exportLandmark(filePath.toStdString());

    if (success) {
        QMessageBox::information(this, "Success", "All landmarks exported successfully!");
    } else {
        QMessageBox::critical(this, "Export Failed", "Failed to save the file. Please check permissions or path.");
    }
}

void DataManager::onEditLandmark(QTableWidgetItem* item)
{
    if (!item) return;

    int row = item->row();
    int col = item->column();

    if (row < 0 || row >= m_landmarks.size()) return;

    Landmark* lm = m_landmarks[row];

    if (col == 0) {
        lm->setPointset(item->text().toStdString());
    }
    else if (col == 1) {
        QString text = item->text();
        text = text.replace("，", ",");
        item->setText(text);
        QStringList parts = text.split(",");

        bool isValid = false;

        if (parts.size() == 3) {
            bool ok1, ok2, ok3;
            double x = parts[0].toDouble(&ok1);
            double y = parts[1].toDouble(&ok2);
            double z = parts[2].toDouble(&ok3);

            if (ok1 && ok2 && ok3) {
                double coords[3] = {x, y, z};
                lm->setCoordinates(coords);
                emit signalUpdateViews();
                isValid = true;
            }
        }

        if (!isValid) {
            const double* oldCoords = lm->getCoordinates();
            QString oldText = QString("%1, %2, %3")
                .arg(oldCoords[0], 0, 'f', 2)
                .arg(oldCoords[1], 0, 'f', 2)
                .arg(oldCoords[2], 0, 'f', 2);

            ui.landmark_tw->blockSignals(true);
            item->setText(oldText);
            ui.landmark_tw->blockSignals(false);
        }
    }
}

bool DataManager::loadLandmark(std::string filePath)
{
    QFile file(QString::fromStdString(filePath));
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) return false;

    QJsonObject rootObj = doc.object();

    // 关键：只有识别到 "Landmark" 关键字才继续处理
    if (!rootObj.contains("Landmark")) {
        return false;
    }

    ui.landmark_tw->setSortingEnabled(false);

    QJsonObject lmData = rootObj["Landmark"].toObject();

    // 遍历组名
    for (auto it = lmData.begin(); it != lmData.end(); ++it) {
        std::string groupName = it.key().toStdString();
        QJsonArray pointsArr = it.value().toArray();

        // 遍历该组下的所有点
        for (const QJsonValue& pv : pointsArr) {
            if (pv.isArray()) {
                QJsonArray cArr = pv.toArray();
                if (cArr.size() >= 3) {
                    double coords[3];
                    coords[0] = cArr[0].toDouble();
                    coords[1] = cArr[1].toDouble();
                    coords[2] = cArr[2].toDouble();
                    
                    addLandmark(coords, groupName);
                }
            }
        }
    }

    ui.landmark_tw->setSortingEnabled(true);
    ui.landmark_tw->resizeColumnsToContents();
    
    emit signalUpdateViews(); 
    return true;
}

bool DataManager::addTool(QString path)
{
    if (path.isEmpty()) return false;

    Tool* newTool = new Tool(path.toStdString());
    
    if (!newTool->getActor()) {
        delete newTool;
        return false;
    }

    double initRadius = newTool->getRadius();
    double initLength = newTool->getLength();

    m_tools.push_back(newTool);

    int row = ui.tool_tw->rowCount();
    ui.tool_tw->insertRow(row);

    // ---------------------------------------------------------
    // Column 0: Name
    // ---------------------------------------------------------
    ui.tool_tw->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);
    QTableWidgetItem* nameItem = new QTableWidgetItem(QString::fromStdString(newTool->getName()));
    nameItem->setTextAlignment(Qt::AlignCenter);
    nameItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
    ui.tool_tw->setItem(row, 0, nameItem);

    // ---------------------------------------------------------
    // Column 1: Visible (Button)
    // ---------------------------------------------------------
    QWidget* visContainer = new QWidget();
    visContainer->setStyleSheet("background-color: transparent;");
    QHBoxLayout* visLayout = new QHBoxLayout(visContainer);
    visLayout->setContentsMargins(0, 0, 0, 0);
    visLayout->setAlignment(Qt::AlignCenter);
    
    QPushButton* visBtn = new QPushButton();
    visBtn->setFlat(true);
    visBtn->setFixedSize(24, 24);
    
    QString visStyle = "QPushButton { image: url(:/DataManager/visible.png); border: none; }";
    QString unvisStyle = "QPushButton { image: url(:/DataManager/unvisible.png); border: none; }";
    visBtn->setStyleSheet(newTool->getVisible() ? visStyle : unvisStyle);

    connect(visBtn, &QPushButton::clicked, [=]() {
        newTool->changeVisible();
        bool currentState = newTool->getVisible();
        visBtn->setStyleSheet(currentState ? visStyle : unvisStyle);
        emit signalUpdateViews();
    });
    visLayout->addWidget(visBtn);
    ui.tool_tw->setCellWidget(row, 1, visContainer);

    // ---------------------------------------------------------
    // Column 2: Opacity (Slider)
    // ---------------------------------------------------------
    QWidget* opContainer = new QWidget();
    opContainer->setStyleSheet("background-color: transparent;");
    QHBoxLayout* opLayout = new QHBoxLayout(opContainer);
    opLayout->setContentsMargins(4, 0, 4, 0);
    opLayout->setAlignment(Qt::AlignCenter);
    
    QSlider* slider = new QSlider(Qt::Horizontal);
    slider->setRange(0, 100);
    slider->setValue(static_cast<int>(newTool->getOpacity() * 100));
    slider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    
    connect(slider, &QSlider::valueChanged, [=](int value) {
        newTool->setOpacity(value / 100.0);
        emit signalUpdateViews();
    });
    opLayout->addWidget(slider);
    ui.tool_tw->setCellWidget(row, 2, opContainer);

    // ---------------------------------------------------------
    // Column 3: Color (Button) [新增列]
    // ---------------------------------------------------------
    QWidget* colorContainer = new QWidget();
    colorContainer->setStyleSheet("background-color: transparent;");
    QHBoxLayout* colorLayout = new QHBoxLayout(colorContainer);
    colorLayout->setContentsMargins(4, 4, 4, 4);
    colorLayout->setAlignment(Qt::AlignCenter);

    QPushButton* colorBtn = new QPushButton();
    colorBtn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    double c[3];
    newTool->getColor(c);
    QString colorStyle = QString("background-color: rgb(%1, %2, %3); border: 1px solid gray; border-radius: 4px;")
        .arg(c[0] * 255).arg(c[1] * 255).arg(c[2] * 255);
    colorBtn->setStyleSheet(colorStyle);

    connect(colorBtn, &QPushButton::clicked, [=]() {
        double currentC[3];
        newTool->getColor(currentC);
        QColor initColor = QColor::fromRgbF(currentC[0], currentC[1], currentC[2]);
        QColor newColor = QColorDialog::getColor(initColor, this, "Select Tool Color");

        if (newColor.isValid()) {
            double rgb[3] = { newColor.redF(), newColor.greenF(), newColor.blueF() };
            newTool->setColor(rgb);
            colorBtn->setStyleSheet(QString("background-color: %1; border: 1px solid gray; border-radius: 4px;").arg(newColor.name()));
            emit signalUpdateViews();
        }
    });
    colorLayout->addWidget(colorBtn);
    ui.tool_tw->setCellWidget(row, 3, colorContainer);

    // ---------------------------------------------------------
    // Column 4: Radius (SpinBox)
    // ---------------------------------------------------------
    QWidget* radContainer = new QWidget();
    radContainer->setStyleSheet("background-color: transparent;");
    QHBoxLayout* radLayout = new QHBoxLayout(radContainer);
    radLayout->setContentsMargins(2, 0, 2, 0);
    radLayout->setAlignment(Qt::AlignCenter);

    QDoubleSpinBox* radSpinBox = new QDoubleSpinBox();
    radSpinBox->setRange(0.1, 100.0);
    radSpinBox->setSingleStep(0.1);
    radSpinBox->setValue(initRadius);
    radSpinBox->setDecimals(1);
    radSpinBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    // 修复：添加 QObject::connect 解决重载二义性错误
    QObject::connect(radSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [=](double value){
        newTool->setRadius(value);
        emit signalUpdateViews();
    });
    radLayout->addWidget(radSpinBox);
    ui.tool_tw->setCellWidget(row, 4, radContainer);

    // ---------------------------------------------------------
    // Column 5: Length (SpinBox)
    // ---------------------------------------------------------
    QWidget* lenContainer = new QWidget();
    lenContainer->setStyleSheet("background-color: transparent;");
    QHBoxLayout* lenLayout = new QHBoxLayout(lenContainer);
    lenLayout->setContentsMargins(2, 0, 2, 0);
    lenLayout->setAlignment(Qt::AlignCenter);

    QDoubleSpinBox* lenSpinBox = new QDoubleSpinBox();
    lenSpinBox->setRange(1.0, 500.0);
    lenSpinBox->setSingleStep(1.0);
    lenSpinBox->setValue(initLength);
    lenSpinBox->setDecimals(1);
    lenSpinBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    QObject::connect(lenSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [=](double value){
        newTool->setLength(value);
        emit signalUpdateViews();
    });
    lenLayout->addWidget(lenSpinBox);
    ui.tool_tw->setCellWidget(row, 5, lenContainer);

    // ---------------------------------------------------------
    // Column 6: Direction (ComboBox) [修改列]
    // ---------------------------------------------------------
    QWidget* dirContainer = new QWidget();
    dirContainer->setStyleSheet("background-color: transparent;");
    QHBoxLayout* dirLayout = new QHBoxLayout(dirContainer);
    dirLayout->setContentsMargins(4, 4, 4, 4);
    dirLayout->setAlignment(Qt::AlignCenter);

    QComboBox* dirCombo = new QComboBox();
    dirCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed); // ComboBox 通常垂直方向固定高度
    
    QStringList directions;
    directions << "Z-" << "Z+" << "Y-" << "Y+" << "X-" << "X+";
    dirCombo->addItems(directions);

    QString currentDir = QString::fromStdString(newTool->getDirection());
    int idx = dirCombo->findText(currentDir);
    if (idx != -1) {
        dirCombo->setCurrentIndex(idx);
    } else {
        dirCombo->setCurrentIndex(0); 
    }

    connect(dirCombo, QOverload<const QString &>::of(&QComboBox::currentTextChanged), 
        [=](const QString &text){
            newTool->setDirection(text.toStdString()); 
            emit signalUpdateViews(); 
        });

    dirLayout->addWidget(dirCombo);
    ui.tool_tw->setCellWidget(row, 6, dirContainer);

    // ---------------------------------------------------------
    // Column 7: Transform (Text + Tooltip)
    // ---------------------------------------------------------
    vtkSmartPointer<vtkMatrix4x4> mat = newTool->getFinalMatrix();
    QString transStr = QString("%1, %2, %3")
        .arg(mat->GetElement(0, 3), 0, 'f', 2)
        .arg(mat->GetElement(1, 3), 0, 'f', 2)
        .arg(mat->GetElement(2, 3), 0, 'f', 2);

    QTableWidgetItem* transItem = new QTableWidgetItem(transStr);
    transItem->setTextAlignment(Qt::AlignCenter);

    QString fullMatStr;
    for(int i=0; i<4; i++) {
        fullMatStr += QString("%1  %2  %3  %4\n")
            .arg(mat->GetElement(i,0), 6, 'f', 2)
            .arg(mat->GetElement(i,1), 6, 'f', 2)
            .arg(mat->GetElement(i,2), 6, 'f', 2)
            .arg(mat->GetElement(i,3), 6, 'f', 2);
    }
    transItem->setToolTip(fullMatStr);
    transItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    ui.tool_tw->setItem(row, 7, transItem);

    // ---------------------------------------------------------
    // Column 8: Handle (Text)
    // ---------------------------------------------------------
    QString handleStr = (newTool->getHandle() == -1) ? "N/A" : QString::number(newTool->getHandle());
    QTableWidgetItem* handleItem = new QTableWidgetItem(handleStr);
    handleItem->setTextAlignment(Qt::AlignCenter);
    handleItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    ui.tool_tw->setItem(row, 8, handleItem);

    // ---------------------------------------------------------
    // Column 9: RMSE (Text)
    // ---------------------------------------------------------
    QString rmseStr = QString::number(newTool->getRMSE(), 'f', 4);
    QTableWidgetItem* rmseItem = new QTableWidgetItem(rmseStr);
    rmseItem->setTextAlignment(Qt::AlignCenter);
    rmseItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    ui.tool_tw->setItem(row, 9, rmseItem);

    // ---------------------------------------------------------
    // Column 10: Path (Text)
    // ---------------------------------------------------------
    QTableWidgetItem* pathItem = new QTableWidgetItem(QString::fromStdString(newTool->getPath()));
    pathItem->setToolTip(QString::fromStdString(newTool->getPath())); 
    pathItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled); 
    ui.tool_tw->setItem(row, 10, pathItem);

    ui.tool_tw->resizeColumnsToContents();

    buildCache();
    
    emit signalAddTool(newTool);
    return true;
}

void DataManager::onAddTool()
{
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "Load Tool File",
        "",
        "Tool Files (*.rom)"
    );

    if (filePath.isEmpty()) {
        return;
    }

    if (addTool(filePath)) {
        std::string msg = "Tool added: " + vtksys::SystemTools::GetFilenameName(filePath.toStdString());
        printInfo(QString::fromStdString(msg));
        emit signalUpdateViews();
    } else {
        QMessageBox::warning(this, "Error", "Failed to load tool.");
    }
}

bool DataManager::deleteTool(int index)
{
    if (index < 0 || index >= m_tools.size()) return false;

    Tool* tool = m_tools[index];
    emit signalDeleteTool(tool);
    ui.tool_tw->removeRow(index);

    buildCache();

    delete tool;
    m_tools.erase(m_tools.begin() + index);

    emit signalUpdateViews();
    return true;
}

void DataManager::onDeleteTool()
{
    int currentRow = ui.tool_tw->currentRow();
    if (currentRow >= 0) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Delete Tool", 
            "Are you sure you want to delete this tool?",
            QMessageBox::Yes | QMessageBox::No);
            
        if (reply == QMessageBox::Yes) {
            deleteTool(currentRow);
        }
    } else {
        QMessageBox::information(this, "Info", "Please select a tool to delete.");
    }
}

void DataManager::onEditTool(QTableWidgetItem* item)
{
    if (!item) return;

    int row = item->row();
    int col = item->column();

    if (row < 0 || row >= m_tools.size()) return;

    Tool* tool = m_tools[row];

    if (col == 0) {
        std::string newName = item->text().toStdString();
        if (tool->getName() != newName) {
            tool->setName(newName);
        }
    }
}


bool DataManager::exportTool(std::string savePath)
{
    if (m_tools.empty()) return false;

    std::string ext = vtksys::SystemTools::GetFilenameLastExtension(savePath);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == ".json") {
        QFile file(QString::fromStdString(savePath));
        if (file.open(QIODevice::WriteOnly)) {
            QJsonObject rootObj;
            QJsonArray toolsArray;

            for (auto* tool : m_tools) {
                QJsonObject toolObj;
                toolObj["name"] = QString::fromStdString(tool->getName());
                toolObj["path"] = QString::fromStdString(tool->getPath());
                toolObj["radius"] = tool->getRadius();
                toolObj["length"] = tool->getLength();
                toolObj["visible"] = tool->getVisible();
                toolObj["opacity"] = tool->getOpacity();

                QJsonArray matArray;
                vtkSmartPointer<vtkMatrix4x4> mat = tool->getFinalMatrix();
                if (mat) {
                    for (int i = 0; i < 4; ++i) {
                        for (int j = 0; j < 4; ++j) {
                            matArray.append(mat->GetElement(i, j));
                        }
                    }
                }
                toolObj["matrix"] = matArray;
                
                double color[3];
                tool->getColor(color);
                QJsonArray colorArray;
                colorArray.append(color[0]);
                colorArray.append(color[1]);
                colorArray.append(color[2]);
                toolObj["color"] = colorArray;

                toolsArray.append(toolObj);
            }

            rootObj["tools"] = toolsArray;

            QJsonDocument doc(rootObj);
            file.write(doc.toJson(QJsonDocument::Indented));
            file.close();
            return true;
        }
    }
    return false;
}

void DataManager::onExportTool()
{
    if (m_tools.empty()) {
        QMessageBox::warning(this, "Export Warning", "No tools to export.");
        return;
    }

    QString defaultName = "Tools_Config";
    QString filePath = QFileDialog::getSaveFileName(
        this,
        "Export Tools",
        defaultName,
        "JSON Files (*.json)"
    );

    if (filePath.isEmpty()) {
        return;
    }

    bool success = exportTool(filePath.toStdString());

    if (success) {
        QMessageBox::information(this, "Success", "Tools exported successfully!");
    } else {
        QMessageBox::critical(this, "Export Failed", "Failed to save the file.");
    }
}


bool DataManager::setOmniTransform(QString name, Eigen::Matrix4d value)
{
    for (int i = 0; i < m_transforms.size(); ++i) {
        if (m_transforms[i]->getName() == name) {
            m_transforms[i]->setValue(value);

            QStringList parts;
            for (int r = 0; r < 4; ++r) {
                QStringList rowParts;
                for (int c = 0; c < 4; ++c) {
                    rowParts << QString::number(value(r, c), 'f', 2);
                }
                parts << rowParts.join(", ");
            }
            QString matStr = "[" + parts.join("; ") + "]";

            QTableWidgetItem* item = ui.transform_tw->item(i, 1);
            if (item) {
                item->setText(matStr);
                item->setToolTip(matStr.replace("; ", "\n"));
            }

            return true;
        }
    }

    return addOmniTransform(name, value);
}


bool DataManager::addOmniTransform(QString name, Eigen::Matrix4d value)
{
    if (name.isEmpty()) {
        name = QString("Transform-%1").arg(m_transforms.size() + 1);
    }

    QString baseName = name;
    int suffixIndex = 1;
    while (true) {
        bool exists = false;
        for (const auto& transform : m_transforms) {
            if (transform->getName() == name) {
                exists = true;
                break;
            }
        }
        
        if (!exists) break;

        name = QString("%1_%2").arg(baseName).arg(suffixIndex++);
    }

    OmniTransform* newOmniTransform = new OmniTransform(name, value);
    m_transforms.push_back(newOmniTransform);

    int row = ui.transform_tw->rowCount();
    ui.transform_tw->insertRow(row);
    ui.transform_tw->setEditTriggers(QAbstractItemView::DoubleClicked | QAbstractItemView::EditKeyPressed);

    QTableWidgetItem* nameItem = new QTableWidgetItem(newOmniTransform->getName());
    nameItem->setTextAlignment(Qt::AlignCenter);
    nameItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
    ui.transform_tw->setItem(row, 0, nameItem);

    QStringList parts;
    for (int i = 0; i < 4; ++i) {
        QStringList rowParts;
        for (int j = 0; j < 4; ++j) {
            rowParts << QString::number(value(i, j), 'f', 2);
        }
        parts << rowParts.join(", ");
    }
    QString matStr = "[" + parts.join("; ") + "]";

    QTableWidgetItem* valueItem = new QTableWidgetItem(matStr);
    valueItem->setTextAlignment(Qt::AlignCenter);
    valueItem->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable);
    valueItem->setToolTip(matStr.replace("; ", "\n"));
    ui.transform_tw->setItem(row, 1, valueItem);

    ui.transform_tw->resizeColumnsToContents();

    emit signalAddOmniTransform(newOmniTransform);

    return true;
}


void DataManager::onAddOmniTransform()
{
    if (addOmniTransform("Manually add", Eigen::Matrix4d::Identity())) {
        // Log info if needed
    }
}

bool DataManager::deleteOmniTransform(int index)
{
    if (index < 0 || index >= m_transforms.size()) return false;

    OmniTransform* t = m_transforms[index];
    emit signalDeleteOmniTransform(t);
    ui.transform_tw->removeRow(index);

    delete t;
    m_transforms.erase(m_transforms.begin() + index);

    emit signalUpdateViews();
    return true;
}

void DataManager::onDeleteOmniTransform()
{
    int currentRow = ui.transform_tw->currentRow();
    if (currentRow >= 0) {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Delete OmniTransform",
            "Are you sure you want to delete this transform?",
            QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            deleteOmniTransform(currentRow);
        }
    } else {
        QMessageBox::information(this, "Info", "Please select a transform to delete.");
    }
}

bool DataManager::exportOmniTransform(std::string savePath)
{
    if (m_transforms.empty()) return false;

    QFile file(QString::fromStdString(savePath));
    if (!file.open(QIODevice::WriteOnly)) return false;

    QJsonObject transformData;

    for (const auto* t : m_transforms) {
        QJsonArray matrixRows;
        Eigen::Matrix4d mat = t->getValue();

        for (int i = 0; i < 4; ++i) {
            QJsonArray rowArr;
            for (int j = 0; j < 4; ++j) {
                rowArr.append(mat(i, j));
            }
            matrixRows.append(rowArr);
        }
        transformData.insert(t->getName(), matrixRows);
    }

    QJsonObject rootObj;
    rootObj.insert("Transform", transformData);

    QJsonDocument doc(rootObj);
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    return true;
}

void DataManager::onExportOmniTransform()
{
    if (m_transforms.empty()) {
        QMessageBox::warning(this, "Export Warning", "No transforms to export.");
        return;
    }

    QString defaultName = "All_Transforms";
    QString filePath = QFileDialog::getSaveFileName(
        this,
        "Export All Transforms",
        defaultName,
        "JSON Files (*.json)"
    );

    if (filePath.isEmpty()) return;

    if (exportOmniTransform(filePath.toStdString())) {
        QMessageBox::information(this, "Success", "All transforms exported successfully!");
    } else {
        QMessageBox::critical(this, "Export Failed", "Failed to save file.");
    }
}

void DataManager::onEditOmniTransform(QTableWidgetItem* item)
{
    if (!item) return;

    int row = item->row();
    int col = item->column();

    if (row < 0 || row >= m_transforms.size()) return;

    OmniTransform* t = m_transforms[row];

    if (col == 0) {
        t->setName(item->text());
    }
    else if (col == 1) {
        QString text = item->text();
        QString cleanText = text;
        cleanText.remove('[').remove(']').replace(';', ' ').replace(',', ' ');
        QStringList parts = cleanText.split(QRegExp("\\s+"), QString::SkipEmptyParts);

        bool isValid = false;
        if (parts.size() == 16) {
            Eigen::Matrix4d mat;
            bool ok = true;
            for (int i = 0; i < 16; ++i) {
                double val = parts[i].toDouble(&ok);
                if (!ok) break;
                mat(i / 4, i % 4) = val;
            }

            if (ok) {
                t->setValue(mat);
                isValid = true;
                emit signalUpdateViews();
            }
        }

        if (isValid) {
            Eigen::Matrix4d val = t->getValue();
            QStringList matParts;
            for (int i = 0; i < 4; ++i) {
                QStringList rowParts;
                for (int j = 0; j < 4; ++j) {
                    rowParts << QString::number(val(i, j), 'f', 2);
                }
                matParts << rowParts.join(", ");
            }
            QString matStr = "[" + matParts.join("; ") + "]";
            
            ui.transform_tw->blockSignals(true);
            item->setText(matStr);
            item->setToolTip(matStr.replace("; ", "\n"));
            ui.transform_tw->blockSignals(false);
        } else {
            Eigen::Matrix4d oldMat = t->getValue();
            QStringList oldParts;
            for (int i = 0; i < 4; ++i) {
                QStringList rowParts;
                for (int j = 0; j < 4; ++j) {
                    rowParts << QString::number(oldMat(i, j), 'f', 2);
                }
                oldParts << rowParts.join(", ");
            }
            QString oldStr = "[" + oldParts.join("; ") + "]";

            ui.transform_tw->blockSignals(true);
            item->setText(oldStr);
            ui.transform_tw->blockSignals(false);
        }
    }
}


bool DataManager::loadOmniTransform(std::string filePath)
{
    QFile file(QString::fromStdString(filePath));
    if (!file.open(QIODevice::ReadOnly)) return false;

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) return false;

    QJsonObject rootObj = doc.object();

    // 关键：只有识别到 "Transform" 关键字才继续
    if (!rootObj.contains("Transform")) {
        return false;
    }

    QJsonObject transformData = rootObj["Transform"].toObject();

    // 遍历 OmniTransform (Key: Name, Value: 4x4 Array)
    for (auto it = transformData.begin(); it != transformData.end(); ++it) {
        QString name = it.key();
        QJsonArray matrixRows = it.value().toArray();

        if (matrixRows.size() == 4) {
            Eigen::Matrix4d mat;
            bool ok = true;
            
            for (int i = 0; i < 4; ++i) {
                QJsonArray rowArr = matrixRows[i].toArray();
                if (rowArr.size() != 4) {
                    ok = false; 
                    break;
                }
                for (int j = 0; j < 4; ++j) {
                    mat(i, j) = rowArr[j].toDouble();
                }
            }

            if (ok) {
                addOmniTransform(name, mat);
            }
        }
    }

    return true;
}

void DataManager::buildCache() {
    m_toolCache.clear();
    m_tableToolRowCache.clear();
    qDebug()<<"m_toolCache and  m_tableToolRowCache clear!";
    
    // 缓存 Tool 指针
    for (Tool* tool : m_tools) {
        if (tool) m_toolCache.insert(QString::fromStdString(tool->getName()), tool);
        qDebug()<<QString::fromStdString(tool->getName());
    }
    
    // 缓存 tool UI 行号
    for (int i = 0; i < ui.tool_tw->rowCount(); ++i) {
        if (auto item = ui.tool_tw->item(i, 0)) {
            m_tableToolRowCache.insert(item->text(), i);
        }
    }
}

void DataManager::onLower2DChanged(int value)
{
    m_lower2Dvalue = value;
    if (ui.lower2Dbox->value() != value) ui.lower2Dbox->setValue(value);
    if (ui.lower2Dbar->value() != value) ui.lower2Dbar->setValue(value);

    if (m_lower2Dvalue > m_upper2Dvalue) {
        ui.upper2Dbox->setValue(m_lower2Dvalue);
    }
    updateProperty();
}

void DataManager::onUpper2DChanged(int value)
{
    m_upper2Dvalue = value;
    if (ui.upper2Dbox->value() != value) ui.upper2Dbox->setValue(value);
    if (ui.upper2Dbar->value() != value) ui.upper2Dbar->setValue(value);

    if (m_upper2Dvalue < m_lower2Dvalue) {
        ui.lower2Dbox->setValue(m_upper2Dvalue);
    }
    updateProperty();
}

void DataManager::onLower3DChanged(int value)
{
    m_lower3Dvalue = value;
    if (ui.lower3Dbox->value() != value) ui.lower3Dbox->setValue(value);
    if (ui.lower3Dbar->value() != value) ui.lower3Dbar->setValue(value);

    if (m_lower3Dvalue > m_upper3Dvalue) {
        ui.upper3Dbox->setValue(m_lower3Dvalue);
    }
    updateProperty();
}

void DataManager::onUpper3DChanged(int value)
{
    m_upper3Dvalue = value;
    if (ui.upper3Dbox->value() != value) ui.upper3Dbox->setValue(value);
    if (ui.upper3Dbar->value() != value) ui.upper3Dbar->setValue(value);

    if (m_upper3Dvalue < m_lower3Dvalue) {
        ui.lower3Dbox->setValue(m_upper3Dvalue);
    }
    updateProperty();
}

void DataManager::onTop3DColorBtnClicked()
{
    QColor color = QColorDialog::getColor(m_top3DColor, this);
    if (color.isValid()) {
        m_top3DColor = color;

        QString style = ui.color3D_btn_top->styleSheet();
        QString newProp = QString("background-color: %1;").arg(color.name());
        QRegularExpression re("background-color\\s*:\\s*[^;]+;?", QRegularExpression::CaseInsensitiveOption);

        if (style.contains(re)) {
            style.replace(re, newProp);
        } else {
            style += QString(" QPushButton { %1 }").arg(newProp);
        }
        ui.color3D_btn_top->setStyleSheet(style);

        emit color3DChanged(m_top3DColor, m_bottom3DColor);
    }
}

void DataManager::onBottom3DColorBtnClicked()
{
    QColor color = QColorDialog::getColor(m_bottom3DColor, this);
    if (color.isValid()) {
        m_bottom3DColor = color;

        QString style = ui.color3D_btn_bot->styleSheet();
        QString newProp = QString("background-color: %1;").arg(color.name());
        QRegularExpression re("background-color\\s*:\\s*[^;]+;?", QRegularExpression::CaseInsensitiveOption);

        if (style.contains(re)) {
            style.replace(re, newProp);
        } else {
            style += QString(" QPushButton { %1 }").arg(newProp);
        }
        ui.color3D_btn_bot->setStyleSheet(style);

        emit color3DChanged(m_top3DColor, m_bottom3DColor);
    }
}


void DataManager::updateProperty()
{
    m_lower2Dvalue = ui.lower2Dbox->value();
    m_upper2Dvalue = ui.upper2Dbox->value();
    m_lower3Dvalue = ui.lower3Dbox->value();
    m_upper3Dvalue = ui.upper3Dbox->value();

    if (m_lut2d) {
        m_lut2d->SetRange(m_lower2Dvalue, m_upper2Dvalue);
    }

    if (m_ctf3d) {
        m_ctf3d->RemoveAllPoints();
        m_ctf3d->AddRGBPoint(m_lower3Dvalue, m_volColors[0][0], m_volColors[0][1], m_volColors[0][2]);
        m_ctf3d->AddRGBPoint((m_lower3Dvalue + m_upper3Dvalue) / 2.0, m_volColors[1][0], m_volColors[1][1], m_volColors[1][2]);
        m_ctf3d->AddRGBPoint(m_upper3Dvalue, m_volColors[2][0], m_volColors[2][1], m_volColors[2][2]);
    }

    if (m_pwf3d) {
        m_pwf3d->RemoveAllPoints();
        m_pwf3d->AddPoint(m_lower3Dvalue, 1.0);
        m_pwf3d->AddPoint(m_lower3Dvalue + 1.0, 0.5 * m_volOpacity);
        m_pwf3d->AddPoint((m_lower3Dvalue + m_upper3Dvalue) / 2.0, 0.7 * m_volOpacity);
        m_pwf3d->AddPoint(m_upper3Dvalue - 1.0, 0.8 * m_volOpacity);
        m_pwf3d->AddPoint(m_upper3Dvalue, m_volOpacity);
        m_pwf3d->ClampingOff();
    }

    emit signalUpdateViews();
}


void DataManager::onImageTableDoubleClicked(QTableWidgetItem* item)
{
    int row = item->row();
    m_currentImageIndex = row;

    if (m_currentVisualImageIndex != -1 && m_currentVisualImageIndex == row) {
        return;
    }
    QCoreApplication::processEvents();

    if (row >= 0 && row < m_images.size()) {
        emit signalAddImage(m_images[row], row, m_currentVisualImageIndex);
        m_currentVisualImageIndex = row;
    }
}


void DataManager::onVolumeCboxChanged(bool state)
{
    emit signalChangeVolumeVisualState(state);
}

void DataManager::importFiles(const QString& specificType)
{
    QString filter = "All Files (*)";
    if (!specificType.isEmpty()) {
        filter = QString("%1 (*.%2);;All Files (*)").arg(specificType.toUpper(), specificType.toLower());
    }

    QString fileName = QFileDialog::getOpenFileName(
        this,
        specificType.isEmpty() ? "Select a file" : "Select a " + specificType + " file",
        QStandardPaths::writableLocation(QStandardPaths::DesktopLocation),
        filter
    );

    if (fileName.isEmpty()) {
        printInfo("No document was selected!");
        return;
    }

    QFileInfo fileInfo(fileName);
    QString extension = fileInfo.suffix().toLower();
    std::string stdPath = fileName.toStdString();
    bool success = false;

    if (extension == "stl" || extension == "obj" || extension == "ply" || extension == "vtk" || extension == "vtp" || extension == "g") {
        success = add3Dmodel(stdPath);
    }
    else if (extension == "json") {
        success = loadLandmark(stdPath) || loadOmniTransform(stdPath);
    }
    else if (extension == "nii" || extension == "gz" || extension == "nrrd" || extension == "mhd" || extension == "mha" || extension == "dcm") {
        success = addImage(stdPath);
    }
    else if (extension == "txt") {
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            QString content = "\n*******************************************************************************\n";
            content += in.readAll();
            content += "\n*******************************************************************************";
            
            printInfo(content); 
            success = true;
        }
    }
    else {
        QMessageBox::warning(this, "Warning", "The file is not appropriate!", QMessageBox::Ok);
        return;
    }

    if (success) {
        printInfo("Import file: " + QString::fromStdString(stdPath));
    } else {
        printInfo("Failed to import file: " + QString::fromStdString(stdPath));
    }
}