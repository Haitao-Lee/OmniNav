#include "Base.h"
#include <Eigen/Dense>
#include <QCloseEvent>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>
#include <QFile>
#include <vtkProperty.h>
#include <vtkMapper.h>
#include <QElapsedTimer>

Base::Base(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);
    loadConfig();
    init();
    createActions();
}

Base::~Base()
{
    if (m_renderTimer) {
        m_renderTimer->stop();
    }

    qDeleteAll(m_uiDisplays);
    m_uiDisplays.clear();

    m_regisMarks.clear();
}


void Base::init()
{
    initSplitters();
    initVTKviews();
    initVTKProperty();
    initLights();
    initRenderTimer();
}


void Base::initRenderTimer()
{
    m_renderTimer = new QTimer(this);

    m_renderTimer->setInterval(1000/60); 
    m_renderTimer->setSingleShot(true);
    // 定时器超时执行真正的渲染
    connect(m_renderTimer, &QTimer::timeout, this, &Base::doThrottledRender);
    // 启动定时器，让它一直循环检查（或者也可以做成单次触发，循环检查更简单）
    m_renderTimer->start();
}

void Base::loadConfig()
{
    QString path = QCoreApplication::applicationDirPath() + "/base_config.json";
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

void Base::initSplitters()
{
    // --- Initialize Splitters ---
    oper_vis_hsplitter = new QSplitter(Qt::Horizontal, this);
    whole_vsplitter = new QSplitter(Qt::Vertical, this);

    // --- Configure Module Widget (Left Panel) ---
    if (ui.module_widget->layout()) {
        ui.module_widget->layout()->setContentsMargins(0, 0, 0, 0);
        ui.module_widget->layout()->setSpacing(0);
    }

    // --- Configure Display Widget (Right Panel) ---
    if (ui.display_widget->layout()) {
        ui.display_widget->layout()->setContentsMargins(0, 0, 0, 0);
        ui.display_widget->layout()->setSpacing(0);
    }

    // --- Add widgets to Horizontal Splitter ---
    oper_vis_hsplitter->addWidget(ui.module_widget);
    oper_vis_hsplitter->addWidget(ui.display_widget);

    // Configure splitter properties
    oper_vis_hsplitter->setHandleWidth(0);
    oper_vis_hsplitter->setChildrenCollapsible(true);
    
    // Set ratio: 1:3 for Operation vs Visualization
    oper_vis_hsplitter->setStretchFactor(0, 1);
    oper_vis_hsplitter->setStretchFactor(1, 3);
    oper_vis_hsplitter->setSizes({1000, 3000});

    // --- Configure Menu Widget (Top Panel) ---
    if (ui.manu_widget->layout()) {
        ui.manu_widget->layout()->setContentsMargins(0, 0, 0, 0);
        ui.manu_widget->layout()->setSpacing(0);
    }

    // --- Add widgets to Main Vertical Splitter ---
    whole_vsplitter->addWidget(ui.manu_widget);
    whole_vsplitter->addWidget(oper_vis_hsplitter);

    whole_vsplitter->setHandleWidth(0);
    whole_vsplitter->setChildrenCollapsible(true);

    // Set ratio: Menu is fixed/small, content takes the rest
    whole_vsplitter->setStretchFactor(0, 4);
    whole_vsplitter->setStretchFactor(1, 96);
    whole_vsplitter->setSizes({400, 9600});

    setCentralWidget(whole_vsplitter);
}

void Base::initVTKviews()
{
    // Clear existing displays
    qDeleteAll(m_uiDisplays);
    m_uiDisplays.clear();

    // Create 4 display instances
    for (int i = 0; i < 4; ++i) {
        m_uiDisplays.append(new Display(this));
    }

    m_viewMode = ALLWIN;

    // Helper lambda to safely add a display to a container with a clean layout
    auto addDisplayToBox = [](QWidget* container, QWidget* content) {
        if (!container || !content) return;

        QVBoxLayout* layout = qobject_cast<QVBoxLayout*>(container->layout());

        if (!layout) {
            // If existing layout is not compatible, remove it
            if (container->layout()) {
                delete container->layout();
            }
            layout = new QVBoxLayout(container);
        }

        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(0);
        layout->addWidget(content);
    };

    // Map logic: Display instances -> UI Containers
    // Note: Indices 2 and 3 are swapped based on UI requirement
    addDisplayToBox(ui.view_box0, m_uiDisplays[0]->getBox()); // Axial
    addDisplayToBox(ui.view_box1, m_uiDisplays[1]->getBox()); // 3D
    addDisplayToBox(ui.view_box2, m_uiDisplays[2]->getBox()); // Coronal (mapped to box2)
    addDisplayToBox(ui.view_box3, m_uiDisplays[3]->getBox()); // Sagittal (mapped to box3)

    // --- Configure Labels ---
    // Axial (Red)
    m_uiDisplays[0]->getLabel()->setStyleSheet("color: red; background-color: transparent;");
    m_uiDisplays[0]->getLabel()->setText("Axial");

    // 3D (Yellow)
    m_uiDisplays[1]->getLabel()->setStyleSheet("color: yellow; background-color: transparent;");
    m_uiDisplays[1]->getLabel()->setText("3D");

    // Coronal (Blue)
    m_uiDisplays[2]->getLabel()->setStyleSheet("color: blue; background-color: transparent;");
    m_uiDisplays[2]->getLabel()->setText("Coronal");

    // Sagittal (Green)
    m_uiDisplays[3]->getLabel()->setStyleSheet("color: green; background-color: transparent;");
    m_uiDisplays[3]->getLabel()->setText("Sagittal");

    // --- Apply Configuration (Colors) ---
    if (m_config.contains("display")) {
        QJsonArray colorArr = m_config["display"].toObject()["colors"].toObject()["top_3d_color"].toArray();

        if (colorArr.size() >= 3) {
            int r = static_cast<int>(colorArr.at(0).toDouble(0.0) * 255);
            int g = static_cast<int>(colorArr.at(1).toDouble(0.0) * 255);
            int b = static_cast<int>(colorArr.at(2).toDouble(0.0) * 255);

            // TODO: Apply this style to a specific button if needed
            QString style = QString("background-color: rgb(%1, %2, %3);").arg(r).arg(g).arg(b);
            // ui.color3D_btn_top->setStyleSheet(style);
        }
    }
    for(int i = 0; i < m_vtkRenderWindows.size(); ++i)
    {
        if (m_vtkRenderWindows[i]) {
            m_vtkRenderWindows[i]->SetMultiSamples(0); 
        }
    }
    update();
}

void Base::initVTKProperty()
{
    m_vtkRenderWindows.clear();
    m_renderers.clear();
    m_contourRenderers.clear();
    m_irens.clear();
    m_styles.clear();
    m_lineActors.clear();

    QJsonObject displayObj = m_config["display"].toObject();
    QJsonObject colorsObj = displayObj["colors"].toObject();

    QJsonArray botArr = colorsObj["bottom_3d_color"].toArray();
    double bot_color[3] = { botArr[0].toDouble(), botArr[1].toDouble(), botArr[2].toDouble() };

    QJsonArray topArr = colorsObj["top_3d_color"].toArray();
    double top_color[3] = { topArr[0].toDouble(), topArr[1].toDouble(), topArr[2].toDouble() };

    double box_color[3] = { top_color[0] * 255.0, top_color[1] * 255.0, top_color[2] * 255.0 };

    QJsonArray init2DArr = colorsObj["initial_2d_color"].toArray();
    double initial_2Dcolor[3] = { init2DArr[0].toDouble(), init2DArr[1].toDouble(), init2DArr[2].toDouble() };

    m_lineCenter[0] = 0.0;
    m_lineCenter[1] = 0.0;
    m_lineCenter[2] = 0.0;
    
    m_picker = vtkSmartPointer<vtkCellPicker>::New();

    double crossLineLength = 100.0;
    double zOffset = 0.0;

    if (m_config.contains("display") && m_config["display"].isObject()) {
        QJsonObject displayObj = m_config["display"].toObject();
        
        if (displayObj.contains("geometry") && displayObj["geometry"].isObject()) {
            QJsonObject geoObj = displayObj["geometry"].toObject();
            
            if (geoObj.contains("cross_line_length")) {
                crossLineLength = geoObj["cross_line_length"].toDouble();
            }
            if (geoObj.contains("line_actor_z_offset")) {
                zOffset = geoObj["line_actor_z_offset"].toDouble();
            }
        }
    }

    double cx = m_lineCenter[0];
    double cy = m_lineCenter[1];
    double cz = m_lineCenter[2];
    double L8 = crossLineLength / 8.0;
    double L2 = crossLineLength / 2.0;

    double colorGreen[3] = { 0.0, 1.0, 0.0 };
    double colorBlue[3]  = { 0.0, 0.0, 1.0 };
    double colorRed[3]   = { 1.0, 0.0, 0.0 };

    double p1[3], p2[3];

    p1[0] = cx - L8; p1[1] = cy; p1[2] = cz;
    p2[0] = cx + L8; p2[1] = cy; p2[2] = cz;
    m_lineActors.append(VisualizationUtils::getLineActor(Eigen::Vector3d(p1[0], p1[1], p1[2]), Eigen::Vector3d(p2[0], p2[1], p2[2]), colorGreen));

    p1[0] = cx; p1[1] = cy - L8; p1[2] = cz;
    p2[0] = cx; p2[1] = cy + L8; p2[2] = cz;
    m_lineActors.append(VisualizationUtils::getLineActor(Eigen::Vector3d(p1[0], p1[1], p1[2]), Eigen::Vector3d(p2[0], p2[1], p2[2]), colorBlue));

    p1[0] = cx; p1[1] = cy; p1[2] = cz - L8;
    p2[0] = cx; p2[1] = cy; p2[2] = cz + L8;
    m_lineActors.append(VisualizationUtils::getLineActor(Eigen::Vector3d(p1[0], p1[1], p1[2]), Eigen::Vector3d(p2[0], p2[1], p2[2]), colorRed));

    p1[0] = cx; p1[1] = cy - L2; p1[2] = zOffset;
    p2[0] = cx; p2[1] = cy + L2; p2[2] = zOffset;
    m_lineActors.append(VisualizationUtils::getLineActor(Eigen::Vector3d(p1[0], p1[1], p1[2]), Eigen::Vector3d(p2[0], p2[1], p2[2]), colorGreen, 0.5));

    p1[0] = cx - L2; p1[1] = cy; p1[2] = zOffset;
    p2[0] = cx + L2; p2[1] = cy; p2[2] = zOffset;
    m_lineActors.append(VisualizationUtils::getLineActor(Eigen::Vector3d(p1[0], p1[1], p1[2]), Eigen::Vector3d(p2[0], p2[1], p2[2]), colorBlue, 0.5));

    p1[0] = cy - L2; p1[1] = cz; p1[2] = zOffset;
    p2[0] = cy + L2; p2[1] = cz; p2[2] = zOffset;
    m_lineActors.append(VisualizationUtils::getLineActor(Eigen::Vector3d(p1[0], p1[1], p1[2]), Eigen::Vector3d(p2[0], p2[1], p2[2]), colorRed, 0.5));

    p1[0] = cy; p1[1] = cz - L2; p1[2] = zOffset;
    p2[0] = cy; p2[1] = cz + L2; p2[2] = zOffset;
    m_lineActors.append(VisualizationUtils::getLineActor(Eigen::Vector3d(p1[0], p1[1], p1[2]), Eigen::Vector3d(p2[0], p2[1], p2[2]), colorGreen, 0.5));

    p1[0] = cx - L2; p1[1] = cz; p1[2] = zOffset;
    p2[0] = cx + L2; p2[1] = cz; p2[2] = zOffset;
    m_lineActors.append(VisualizationUtils::getLineActor(Eigen::Vector3d(p1[0], p1[1], p1[2]), Eigen::Vector3d(p2[0], p2[1], p2[2]), colorRed, 0.5));

    p1[0] = cx; p1[1] = cz - L2; p1[2] = zOffset;
    p2[0] = cx; p2[1] = cz + L2; p2[2] = zOffset;
    m_lineActors.append(VisualizationUtils::getLineActor(Eigen::Vector3d(p1[0], p1[1], p1[2]), Eigen::Vector3d(p2[0], p2[1], p2[2]), colorBlue, 0.5));

    for (int i = 0; i < 4; i++)
    {
        vtkSmartPointer<vtkRenderWindow> rw = m_uiDisplays[i]->getUi().view->GetRenderWindow();
        m_vtkRenderWindows.append(rw);
        
        m_renderers.append(vtkSmartPointer<vtkRenderer>::New());
        m_contourRenderers.append(vtkSmartPointer<vtkRenderer>::New());
        m_irens.append(rw->GetInteractor());

        if (i == 1)
        {
            m_styles.append(vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New());
            
            QString styleSheet = QString("border:None; background-color: rgb(%1, %2, %3);")
                                .arg((int)box_color[0]).arg((int)box_color[1]).arg((int)box_color[2]);
            m_uiDisplays[i]->getUi().box->setStyleSheet(styleSheet);

            m_renderers[i]->SetBackground(bot_color);
            m_renderers[i]->SetBackground2(top_color);
            m_renderers[i]->GradientBackgroundOn();
            // m_renderers[i]->SetUseDepthPeeling(1);
            // m_renderers[i]->SetMaximumNumberOfPeels(4);
            // m_renderers[i]->SetOcclusionRatio(0.1);

            // m_contourRenderers[i]->SetBackground(bot_color);
            // m_contourRenderers[i]->SetBackground2(top_color);
            // m_contourRenderers[i]->GradientBackgroundOn();

            m_irens[i]->SetInteractorStyle(m_styles[i]);
            m_irens[i]->SetRenderWindow(m_vtkRenderWindows[i]);
            m_vtkRenderWindows[i]->AddRenderer(m_renderers[i]);

            // m_contourRenderers[i]->SetActiveCamera(m_renderers[i]->GetActiveCamera());
            // m_vtkRenderWindows[i]->AddRenderer(m_contourRenderers[i]);
            
            m_vtkRenderWindows[i]->SetNumberOfLayers(1);
            // m_contourRenderers[i]->SetLayer(1);
            // m_renderers[i]->SetLayer(0);
            
            m_vtkRenderWindows[i]->Render();
            m_irens[i]->Initialize();
        }
        else
        {
            m_styles.append(vtkSmartPointer<vtkInteractorStyleImage>::New());
            m_renderers[i]->SetBackground(initial_2Dcolor);
            
            m_irens[i]->SetInteractorStyle(m_styles[i]);
            m_irens[i]->SetRenderWindow(m_vtkRenderWindows[i]);
            
            m_vtkRenderWindows[i]->AddRenderer(m_renderers[i]);
            
            m_contourRenderers[i]->SetActiveCamera(m_renderers[i]->GetActiveCamera());
            m_vtkRenderWindows[i]->AddRenderer(m_contourRenderers[i]);
            
            m_vtkRenderWindows[i]->SetNumberOfLayers(2);
            m_contourRenderers[i]->SetLayer(1);
            m_renderers[i]->SetLayer(0);
            
            m_vtkRenderWindows[i]->Render();
        }
    }

    if (m_lineActors.size() >= 3) {
        m_renderers[1]->AddActor(m_lineActors[0]);
        m_renderers[1]->AddActor(m_lineActors[1]);
        m_renderers[1]->AddActor(m_lineActors[2]);
    }

    add2DLineActors();

    // for (int i = 0; i < 3; ++i)
    // {
    //     if (!m_lineActors[i]) continue;
    //     vtkActor* lineActor = m_lineActors[i];
    //     if (lineActor) {
    //         // 1. 关闭光照
    //         if (lineActor->GetProperty()) {
    //             lineActor->GetProperty()->SetLighting(false);
    //         }

    //         // 2. 【替代方案】让线在深度上“向前”偏移，从而显示在最上面
    //         vtkMapper* mapper = lineActor->GetMapper();
    //         if (mapper) {
    //             // 开启多边形偏移（解决共面闪烁）
    //             mapper->SetResolveCoincidentTopologyToPolygonOffset();
    //             // 参数1：Factor，参数2：Units（负数表示拉向摄像机）
    //             // 设置一个较大的负数，强行让线画在前面
    //             mapper->SetRelativeCoincidentTopologyPolygonOffsetParameters(0, -66000);
    //         }
    //     }
    // }

    m_3DOrientationWidget = VisualizationUtils::getOrientationMarker(m_irens[1]);
    m_cross_line_sign = 0;
    onCrossLinesUpdate();    
    updateViews();
}


void Base::initLights()
{
    m_regisMarks.clear();

    // Find and store registration mark labels (0 to 14)
    for (int i = 0; i < 15; ++i) {
        QString name = QString("regis_mark%1").arg(i);
        QLabel* label = this->findChild<QLabel*>(name);        
        if (label) {
            m_regisMarks.append(label);
        }
    }

    // Hide all marks initially
    // for (QLabel* r : m_regisMarks) {
    //     if (r) r->setVisible(false);
    // }
}


void Base::createActions()
{
    connect(ui.layout_cbb, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &Base::onLayoutChanged);
    ui.layout_cbb->setCurrentIndex(2); //default layout
    connect(ui.cross_line_btn, &QPushButton::clicked, this, &Base::onCrossLinesUpdate);
    connect(m_uiDisplays[0]->getUi().resetCamera_btn, &QPushButton::clicked, this, &Base::onResetCammera0);
    connect(m_uiDisplays[1]->getUi().resetCamera_btn, &QPushButton::clicked, this, &Base::onResetCammera1);
    connect(m_uiDisplays[2]->getUi().resetCamera_btn, &QPushButton::clicked, this, &Base::onResetCammera2);
    connect(m_uiDisplays[3]->getUi().resetCamera_btn, &QPushButton::clicked, this, &Base::onResetCammera3);
    connect(m_uiDisplays[0]->getUi().rotate90_btn, &QPushButton::clicked, this, &Base::onRotateView0);
    connect(m_uiDisplays[1]->getUi().rotate90_btn, &QPushButton::clicked, this, &Base::onRotateView1);
    connect(m_uiDisplays[2]->getUi().rotate90_btn, &QPushButton::clicked, this, &Base::onRotateView2);
    connect(m_uiDisplays[3]->getUi().rotate90_btn, &QPushButton::clicked, this, &Base::onRotateView3);
    connect(m_uiDisplays[0]->getUi().zoom_btn, &QPushButton::clicked, this, &Base::onZoomView0);
    connect(m_uiDisplays[1]->getUi().zoom_btn, &QPushButton::clicked, this, &Base::onZoomView1);
    connect(m_uiDisplays[2]->getUi().zoom_btn, &QPushButton::clicked, this, &Base::onZoomView2);
    connect(m_uiDisplays[3]->getUi().zoom_btn, &QPushButton::clicked, this, &Base::onZoomView3);
    connect(m_uiDisplays[0]->getUi().scrollbar, &QSlider::sliderPressed, [=]() { setInteractiveRendering(true);});
    connect(m_uiDisplays[1]->getUi().scrollbar, &QSlider::sliderPressed, [=]() { setInteractiveRendering(true);});
    connect(m_uiDisplays[2]->getUi().scrollbar, &QSlider::sliderPressed, [=]() { setInteractiveRendering(true);});
    connect(m_uiDisplays[3]->getUi().scrollbar, &QSlider::sliderPressed, [=]() { setInteractiveRendering(true);});
    connect(m_uiDisplays[0]->getUi().scrollbar, &QSlider::sliderReleased, [=]() {
        setInteractiveRendering(false); // 恢复高质量
    });
    connect(m_uiDisplays[1]->getUi().scrollbar, &QSlider::sliderReleased, [=]() {
        setInteractiveRendering(false); // 恢复高质量
    });
    connect(m_uiDisplays[2]->getUi().scrollbar, &QSlider::sliderReleased, [=]() {
        setInteractiveRendering(false); // 恢复高质量
    });
    connect(m_uiDisplays[3]->getUi().scrollbar, &QSlider::sliderReleased, [=]() {
        setInteractiveRendering(false); // 恢复高质量
    });
    connect(m_uiDisplays[0]->getUi().scrollbar, &QAbstractSlider::actionTriggered, [=](int action) {
        if (action == QAbstractSlider::SliderSingleStepAdd || 
            action == QAbstractSlider::SliderSingleStepSub) 
        {
            updateViews(); 
        }
    });
    connect(m_uiDisplays[1]->getUi().scrollbar, &QAbstractSlider::actionTriggered, [=](int action) {
        if (action == QAbstractSlider::SliderSingleStepAdd || 
            action == QAbstractSlider::SliderSingleStepSub) 
        {
            updateViews(); 
        }
    });
    connect(m_uiDisplays[2]->getUi().scrollbar, &QAbstractSlider::actionTriggered, [=](int action) {
        if (action == QAbstractSlider::SliderSingleStepAdd || 
            action == QAbstractSlider::SliderSingleStepSub) 
        {
            updateViews(); 
        }
    });
    connect(m_uiDisplays[3]->getUi().scrollbar, &QAbstractSlider::actionTriggered, [=](int action) {
        if (action == QAbstractSlider::SliderSingleStepAdd || 
            action == QAbstractSlider::SliderSingleStepSub) 
        {
            updateViews(); 
        }
    });
}



void Base::closeEvent(QCloseEvent* e)
{
    QMessageBox::StandardButton resBtn = QMessageBox::question(
        this, 
        "Confirm Exit",
        tr("Are you sure you want to exit the navigation system?\n"),
        QMessageBox::No | QMessageBox::Yes,
        QMessageBox::Yes
    );

    if (resBtn == QMessageBox::Yes) {
        e->accept();
    } else {
        e->ignore();
    }
}

void Base::onLayoutChanged(int index)
{
    QGridLayout* layout = qobject_cast<QGridLayout*>(ui.display_widget->layout());
    if (!layout) return;

    m_viewMode = ALLWIN;

    QWidget* win3D = ui.view_box1; 
    QList<QWidget*> others = { ui.view_box0, ui.view_box2, ui.view_box3 };

    win3D->hide();
    for(auto* w : others) w->hide();

    while (QLayoutItem* item = layout->takeAt(0)) {
        delete item; 
    }
    
    for (int i = 0; i < layout->rowCount(); ++i) layout->setRowStretch(i, 0);
    for (int j = 0; j < layout->columnCount(); ++j) layout->setColumnStretch(j, 0);

    switch (index) {
    case 0: 
        layout->addWidget(win3D,   0, 0);
        layout->addWidget(others[0], 0, 1);
        layout->addWidget(others[1], 1, 0);
        layout->addWidget(others[2], 1, 1);
        break;
    case 1: 
        layout->addWidget(others[0], 0, 0);
        layout->addWidget(others[1], 0, 1);
        layout->addWidget(win3D,   1, 0);
        layout->addWidget(others[2], 1, 1);
        break;
    case 2: 
        layout->addWidget(others[0], 0, 0);
        layout->addWidget(win3D,   0, 1);
        layout->addWidget(others[1], 1, 0);
        layout->addWidget(others[2], 1, 1);
        break;
    
    case 3: 
        layout->addWidget(others[0], 0, 0);
        layout->addWidget(others[1], 0, 1);
        layout->addWidget(others[2], 1, 0);
        layout->addWidget(win3D,   1, 1);
        break;

    case 4: 
        layout->addWidget(win3D,   0, 0, 1, 3); 
        layout->addWidget(others[0], 1, 0);
        layout->addWidget(others[1], 1, 1);
        layout->addWidget(others[2], 1, 2);
        layout->setRowStretch(0, 3);
        layout->setRowStretch(1, 1);
        break;

    case 5:
        layout->addWidget(others[0], 0, 0);
        layout->addWidget(others[1], 0, 1);
        layout->addWidget(others[2], 0, 2);
        layout->addWidget(win3D,   1, 0, 1, 3); 
        layout->setRowStretch(0, 1);
        layout->setRowStretch(1, 3);
        break;

    case 6:
        layout->addWidget(win3D,   0, 0, 3, 1); 
        layout->addWidget(others[0], 0, 1);
        layout->addWidget(others[1], 1, 1);
        layout->addWidget(others[2], 2, 1);
        layout->setColumnStretch(0, 3);
        layout->setColumnStretch(1, 1);
        break;

    case 7:
        layout->addWidget(others[0], 0, 0);
        layout->addWidget(others[1], 1, 0);
        layout->addWidget(others[2], 2, 0);
        layout->addWidget(win3D,   0, 1, 3, 1); 
        layout->setColumnStretch(0, 1);
        layout->setColumnStretch(1, 3);
        break;
    }
    
    win3D->setAutoFillBackground(true);
    win3D->show();
    
    for(auto* w : others) {
        w->setAutoFillBackground(true);
        w->show();
    }

    updateViews();
}

void Base::onResetCammera0()
{
    if (m_renderers.isEmpty() || !m_renderers[0]) return;

    double viewUp[3] = { 0.0, -1.0, 0.0 };

    if (m_config.contains("display") && m_config["display"].isObject()) {
        QJsonObject displayObj = m_config["display"].toObject();
        if (displayObj.contains("camera") && displayObj["camera"].isObject()) {
            QJsonObject camObj = displayObj["camera"].toObject();
            if (camObj.contains("view_up") && camObj["view_up"].isArray()) {
                QJsonArray outerArr = camObj["view_up"].toArray();
                if (outerArr.size() > 0 && outerArr[0].isArray()) {
                    QJsonArray innerArr = outerArr[0].toArray();
                    if (innerArr.size() >= 3) {
                        viewUp[0] = innerArr[0].toDouble();
                        viewUp[1] = innerArr[1].toDouble();
                        viewUp[2] = innerArr[2].toDouble();
                    }
                }
            }
        }
    }

    resetCamera(m_renderers[0], 1, viewUp);

    if (m_vtkRenderWindows.size() > 0 && m_vtkRenderWindows[0]) {
        m_vtkRenderWindows[0]->Render();
    }
    
    if (m_uiDisplays.size() > 0) {
        m_uiDisplays[0]->getUi().view->update();
    }
}

void Base::onResetCammera1()
{
    if (m_renderers.size() <= 1 || !m_renderers[1]) return;

    double viewUp[3] = { 0.0, 0.0, 1.0 };
    bool useViewUp = true;

    if (m_config.contains("display") && m_config["display"].isObject()) {
        QJsonObject displayObj = m_config["display"].toObject();
        if (displayObj.contains("camera") && displayObj["camera"].isObject()) {
            QJsonObject camObj = displayObj["camera"].toObject();
            if (camObj.contains("view_up") && camObj["view_up"].isArray()) {
                QJsonArray outerArr = camObj["view_up"].toArray();
                if (outerArr.size() > 1) {
                    if (outerArr[1].isArray()) {
                        QJsonArray innerArr = outerArr[1].toArray();
                        if (innerArr.size() >= 3) {
                            viewUp[0] = innerArr[0].toDouble();
                            viewUp[1] = innerArr[1].toDouble();
                            viewUp[2] = innerArr[2].toDouble();
                        }
                    } else if (outerArr[1].isNull()) {
                        useViewUp = false;
                    }
                }
            }
        }
    }

    resetCamera(m_renderers[1], 0, useViewUp ? viewUp : nullptr);

    if (m_vtkRenderWindows.size() > 1 && m_vtkRenderWindows[1]) {
        m_vtkRenderWindows[1]->Render();
    }
    
    if (m_uiDisplays.size() > 1) {
        m_uiDisplays[1]->getUi().view->update();
    }
}

void Base::onResetCammera2()
{
    if (m_renderers.size() <= 2 || !m_renderers[2]) return;

    double viewUp[3] = { 0.0, 1.0, 0.0 };

    if (m_config.contains("display") && m_config["display"].isObject()) {
        QJsonObject displayObj = m_config["display"].toObject();
        if (displayObj.contains("camera") && displayObj["camera"].isObject()) {
            QJsonObject camObj = displayObj["camera"].toObject();
            if (camObj.contains("view_up") && camObj["view_up"].isArray()) {
                QJsonArray outerArr = camObj["view_up"].toArray();
                if (outerArr.size() > 2 && outerArr[2].isArray()) {
                    QJsonArray innerArr = outerArr[2].toArray();
                    if (innerArr.size() >= 3) {
                        viewUp[0] = innerArr[0].toDouble();
                        viewUp[1] = innerArr[1].toDouble();
                        viewUp[2] = innerArr[2].toDouble();
                    }
                }
            }
        }
    }

    resetCamera(m_renderers[2], 1, viewUp);

    if (m_vtkRenderWindows.size() > 2 && m_vtkRenderWindows[2]) {
        m_vtkRenderWindows[2]->Render();
    }
    
    if (m_uiDisplays.size() > 2) {
        m_uiDisplays[2]->getUi().view->update();
    }
}

void Base::onResetCammera3()
{
    if (m_renderers.size() <= 3 || !m_renderers[3]) return;

    double viewUp[3] = { 0.0, 1.0, 0.0 };

    if (m_config.contains("display") && m_config["display"].isObject()) {
        QJsonObject displayObj = m_config["display"].toObject();
        if (displayObj.contains("camera") && displayObj["camera"].isObject()) {
            QJsonObject camObj = displayObj["camera"].toObject();
            if (camObj.contains("view_up") && camObj["view_up"].isArray()) {
                QJsonArray outerArr = camObj["view_up"].toArray();
                if (outerArr.size() > 3 && outerArr[3].isArray()) {
                    QJsonArray innerArr = outerArr[3].toArray();
                    if (innerArr.size() >= 3) {
                        viewUp[0] = innerArr[0].toDouble();
                        viewUp[1] = innerArr[1].toDouble();
                        viewUp[2] = innerArr[2].toDouble();
                    }
                }
            }
        }
    }

    resetCamera(m_renderers[3], 1, viewUp);

    if (m_vtkRenderWindows.size() > 3 && m_vtkRenderWindows[3]) {
        m_vtkRenderWindows[3]->Render();
    }
    
    if (m_uiDisplays.size() > 3) {
        m_uiDisplays[3]->getUi().view->update();
    }
}


void Base::onRotateView0()
{

}

void Base::onRotateView1()
{
    
}

void Base::onRotateView2()
{
    
}

void Base::onRotateView3()
{
    
}

void Base::onZoomView0()
{
    if (m_viewMode == ALLWIN) {
        m_viewMode = AXIAL;

        if(ui.view_box0) ui.view_box0->hide();
        if(ui.view_box1) ui.view_box1->hide();
        if(ui.view_box2) ui.view_box2->hide();
        if(ui.view_box3) ui.view_box3->hide();

        QGridLayout* layout = qobject_cast<QGridLayout*>(ui.display_widget->layout());

        while (QLayoutItem* item = layout->takeAt(0)) {
            delete item; 
        }

        for (int i = 0; i < layout->rowCount(); ++i) layout->setRowStretch(i, 0);
        for (int j = 0; j < layout->columnCount(); ++j) layout->setColumnStretch(j, 0);

        layout->addWidget(ui.view_box0);
        ui.view_box0->show();

        if(m_uiDisplays[0]->getUi().zoom_btn) 
            m_uiDisplays[0]->getUi().zoom_btn->setStyleSheet("image: url(:/Display/zoom_in.png);");

    } else {
        m_viewMode = ALLWIN;

        QString fullScreenStyle = "image: url(:/Display/full_screen.png);";
        if(m_uiDisplays[0]->getUi().zoom_btn) 
            m_uiDisplays[0]->getUi().zoom_btn->setStyleSheet(fullScreenStyle);
            
        onLayoutChanged(ui.layout_cbb->currentIndex());
    }
}

void Base::onZoomView1()
{
    if (m_viewMode == ALLWIN) {
        m_viewMode = VIEW3D;

        if(ui.view_box0) ui.view_box0->hide();
        if(ui.view_box1) ui.view_box1->hide();
        if(ui.view_box2) ui.view_box2->hide();
        if(ui.view_box3) ui.view_box3->hide();

        QGridLayout* layout = qobject_cast<QGridLayout*>(ui.display_widget->layout());
        
        while (QLayoutItem* item = layout->takeAt(0)) {
            delete item; 
        }

        for (int i = 0; i < layout->rowCount(); ++i) layout->setRowStretch(i, 0);
        for (int j = 0; j < layout->columnCount(); ++j) layout->setColumnStretch(j, 0);

        layout->addWidget(ui.view_box1);
        ui.view_box1->show();

        if(m_uiDisplays[1]->getUi().zoom_btn) 
            m_uiDisplays[1]->getUi().zoom_btn->setStyleSheet("image: url(:/Display/zoom_in.png);");

    } else {
        m_viewMode = ALLWIN;

        QString fullScreenStyle = "image: url(:/Display/full_screen.png);";
        if(m_uiDisplays[1]->getUi().zoom_btn) 
            m_uiDisplays[1]->getUi().zoom_btn->setStyleSheet(fullScreenStyle);
            
        onLayoutChanged(ui.layout_cbb->currentIndex());
    }
}

void Base::onZoomView2()
{
    if (m_viewMode == ALLWIN) {
        m_viewMode = SAGITA;

        if(ui.view_box0) ui.view_box0->hide();
        if(ui.view_box1) ui.view_box1->hide();
        if(ui.view_box2) ui.view_box2->hide();
        if(ui.view_box3) ui.view_box3->hide();

        QGridLayout* layout = qobject_cast<QGridLayout*>(ui.display_widget->layout());
        
        while (QLayoutItem* item = layout->takeAt(0)) {
            delete item; 
        }

        for (int i = 0; i < layout->rowCount(); ++i) layout->setRowStretch(i, 0);
        for (int j = 0; j < layout->columnCount(); ++j) layout->setColumnStretch(j, 0);

        layout->addWidget(ui.view_box2);
        ui.view_box2->show();

        if(m_uiDisplays[2]->getUi().zoom_btn) 
            m_uiDisplays[2]->getUi().zoom_btn->setStyleSheet("image: url(:/Display/zoom_in.png);");

    } else {
        m_viewMode = ALLWIN;

        QString fullScreenStyle = "image: url(:/Display/full_screen.png);";
        if(m_uiDisplays[2]->getUi().zoom_btn) 
            m_uiDisplays[2]->getUi().zoom_btn->setStyleSheet(fullScreenStyle);
            
        onLayoutChanged(ui.layout_cbb->currentIndex());
    }
}

void Base::onZoomView3()
{
    if (m_viewMode == ALLWIN) {
        m_viewMode = CORNAL;

        if(ui.view_box0) ui.view_box0->hide();
        if(ui.view_box1) ui.view_box1->hide();
        if(ui.view_box2) ui.view_box2->hide();
        if(ui.view_box3) ui.view_box3->hide();

        QGridLayout* layout = qobject_cast<QGridLayout*>(ui.display_widget->layout());
        
        while (QLayoutItem* item = layout->takeAt(0)) {
            delete item; 
        }

        for (int i = 0; i < layout->rowCount(); ++i) layout->setRowStretch(i, 0);
        for (int j = 0; j < layout->columnCount(); ++j) layout->setColumnStretch(j, 0);

        layout->addWidget(ui.view_box3);
        ui.view_box3->show();

        if(m_uiDisplays[3]->getUi().zoom_btn) 
            m_uiDisplays[3]->getUi().zoom_btn->setStyleSheet("image: url(:/Display/zoom_in.png);");

    } else {
        m_viewMode = ALLWIN;

        QString fullScreenStyle = "image: url(:/Display/full_screen.png);";
        if(m_uiDisplays[3]->getUi().zoom_btn) 
            m_uiDisplays[3]->getUi().zoom_btn->setStyleSheet(fullScreenStyle);
            
        onLayoutChanged(ui.layout_cbb->currentIndex());
    }
}


void Base::updateView(int view_num)
{
    if (m_cross_line_sign % 3 == 0)
    {
        setNextRenderTargets({view_num});
    }

    else if (m_cross_line_sign % 3 == 1)
    {
        if (view_num != 1)
            setNextRenderTargets({0,2,3});
    }
    else if (m_cross_line_sign % 3 == 2)
    {
        if (view_num != 1)
            setNextRenderTargets({0,1,2,3});
    }
    updateViews(); 
}


void Base::updateViews()
{
    m_renderPending = true;

    //防止阻塞
    if (m_renderTimer && !m_renderTimer->isActive()) {
        m_renderTimer->start();
    }
}


void Base::add2DLineActors()
{
    if (m_contourRenderers.size() <= 3 || m_lineActors.size() <= 8) return;

    m_contourRenderers[0]->AddActor(m_lineActors[3]);
    m_contourRenderers[0]->AddActor(m_lineActors[4]);

    m_contourRenderers[2]->AddActor(m_lineActors[5]);
    m_contourRenderers[2]->AddActor(m_lineActors[6]);

    m_contourRenderers[3]->AddActor(m_lineActors[7]);
    m_contourRenderers[3]->AddActor(m_lineActors[8]);

    updateViews();
}


void Base::reposition2DLineActors()
{
    double zOffset = 0.0;
    
    if (m_config.contains("display") && m_config["display"].isObject()) {
        QJsonObject displayObj = m_config["display"].toObject();
        
        if (displayObj.contains("geometry") && displayObj["geometry"].isObject()) {
            QJsonObject geoObj = displayObj["geometry"].toObject();
            
            if (geoObj.contains("line_actor_z_offset")) {
                zOffset = geoObj["line_actor_z_offset"].toDouble();
            }
        }
    }
    
    for (int i = 3; i <= 8; ++i) {
        if (i < m_lineActors.size() && m_lineActors[i]) {
            m_lineActors[i]->SetPosition(0.0, 0.0, zOffset);
        }
    }
}


void Base::onCrossLinesUpdate()
{
    m_cross_line_sign = (m_cross_line_sign + 1) % 3;

    for (int i = 0; i < m_lineActors.size(); ++i)
    {
        if (!m_lineActors[i]) continue;

        if (m_cross_line_sign == 0)
            m_lineActors[i]->SetVisibility(0);
        else if (m_cross_line_sign == 1)
            m_lineActors[i]->SetVisibility(i >= 3 ? 1 : 0);
        else if (m_cross_line_sign == 2)
            m_lineActors[i]->SetVisibility(1);

        // 恢复 1.0，但关闭光照，防止核显在计算线段阴影时出错
        m_lineActors[i]->GetProperty()->SetOpacity(1.0);
        m_lineActors[i]->GetProperty()->SetLighting(false);
        
        // 关键：不要让线参与 Bounds 计算
        m_lineActors[i]->SetUseBounds(false);
    }

    updateViews();
}


void Base::resetCamera(vtkRenderer* renderer, int parallelProjection, double* viewUp)
{
    if (!renderer) return;

    renderer->GetActiveCamera()->SetParallelProjection(parallelProjection);
    renderer->ResetCamera();

    double zoom = 1.0;

    if (m_config.contains("display") && m_config["display"].isObject()) {
        QJsonObject displayObj = m_config["display"].toObject();
        if (displayObj.contains("camera") && displayObj["camera"].isObject()) {
            QJsonObject cameraObj = displayObj["camera"].toObject();
            if (cameraObj.contains("zoom")) {
                zoom = cameraObj["zoom"].toDouble();
            }
        }
    }

    if (zoom <= 0.0001) zoom = 1.0;

    renderer->GetActiveCamera()->Zoom(zoom);

    if (viewUp) {
        renderer->GetActiveCamera()->SetViewUp(viewUp);
    }
}

void Base::doThrottledRender()
{
    if (m_renderPending)
    {
        QElapsedTimer timer;
        timer.start();
        for (int viewId : m_renderViewIds) 
        {
            if (viewId >= 0 && viewId < m_vtkRenderWindows.size())
            {
                auto rw = m_vtkRenderWindows[viewId];
                if (rw) rw->Render();
            }
        }
        // qDebug() << "Render Time Cost:" << timer.elapsed() << "ms";
        m_renderPending = false;
    }
    setNextRenderTargets({0, 1, 2, 3});
}


void Base::setInteractiveRendering(bool enable)
{
    // 1. 设置极高的请求帧率 (迫使 VTK 内部自动降级)
    double updateRate = enable ? 30.0 : 0.0001;

    for (auto& iren : m_irens) {
        if (!iren) continue;
        iren->SetDesiredUpdateRate(updateRate);

        // 【提速大招 1】交互时彻底关闭抗锯齿 (MSAA)
        // 拖动时边缘会有锯齿，但显卡负载大幅降低
        if (auto renWin = iren->GetRenderWindow()) {
            renWin->SetMultiSamples(enable ? 0 : 4); 
        }
    }
    
    // 2. 3D 窗口专用优化
    if (m_renderers[VIEW3D]) {
        // 关闭深度剥离 (透明度计算非常耗时)
        m_renderers[VIEW3D]->SetUseDepthPeeling(enable ? 0 : 1);

        if (!enable) {
            // 静止时恢复高质量
            m_renderers[VIEW3D]->SetMaximumNumberOfPeels(4);
            m_renderers[VIEW3D]->SetOcclusionRatio(0.1);
        }
    }

    // 3. 松手时强制刷新回高质量
    if (!enable) {
        updateViews(); 
    }
}