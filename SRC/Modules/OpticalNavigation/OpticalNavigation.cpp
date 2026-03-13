#define _USE_MATH_DEFINES 
#include "OpticalNavigation.h"
#include "GeometryUtils.h"
#include <vtkMatrix4x4.h>
#include <vtkTransform.h>
#include <QMessageBox>
#include <QDebug>
#include <QListView>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <vector>
#include <QFile>
#include <QInputDialog>
#include <QLineEdit>
#include <QRegularExpression>
#include <QApplication> 
#include <Eigen/Geometry>
#include <cmath>
#include <math.h>

namespace {
    const QString toolTipStyle = "QToolTip { color: black; background-color: #ffffff; border: 1px solid gray; }";
    const QString textBaseStyle = "QPushButton{ background-color:rgba(77,77,115,136); border-style:outset; border-width:1px; border-radius:15px; border-color:rgba(255,255,255,30); color:rgba(255,255,255,255); padding:6px; }";
    const QString hoverStyle = "QPushButton:hover{ background-color:rgba(200,200,200,100); border-color:rgba(255,255,255,200); color:rgba(255,255,255,255); }";
    const QString pressedGreen = "QPushButton:pressed{ background-color:rgba(100,255,100,200); border-color:rgba(255,255,255,30); border-style:inset; color:rgba(255,255,255,255); }";
    
    const QString activeStyle = "QPushButton { background-color: rgba(46, 204, 113, 255); border-style: outset; border-width: 1px; border-radius: 15px; border-color: rgba(255, 255, 255, 50); color: white; padding: 6px; }"
                                "QPushButton:hover { background-color: rgba(60, 230, 130, 255); } QPushButton:pressed { background-color: rgba(30, 160, 90, 255); border-style: inset; }";
    
    const QString connectingStyle = "QPushButton { background-color: rgba(241, 196, 15, 200); border-style: outset; border-width: 1px; border-radius: 15px; border-color: rgba(255, 255, 255, 150); color: white; padding: 6px; }"
                                    "QPushButton:hover { background-color: rgba(243, 156, 18, 230); } QPushButton:pressed { background-color: rgba(230, 126, 34, 255); border-style: inset; }";

    const QString arrowImage = "url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAwAAAAMCAYAAABWdVznAAAACXBIWXMAAA7EAAAOxAGVKw4bAAAAWklEQVQokZWRQQ4AIAgD+f+j24uH1qpL2QAJ2NhmZ2Z24GECiAl78A3wFzL47kIuYwfwBCZfQ0fXwYfL0I5L6H0QOQfBfRHOQ/B+CDmHkPsQuQfBfRA5h5B7EAD01h7t502LyAAAAABJRU5ErkJggg==)";

    const QString darkComboBoxStyle = 
        "QComboBox { background-color: rgba(45, 45, 48, 220); border: 1px solid rgba(255, 255, 255, 30); border-radius: 15px; padding: 6px 10px 6px 20px; color: rgba(255, 255, 255, 230); font-weight: bold; }"
        "QComboBox:hover { background-color: rgba(60, 60, 65, 255); border-color: rgba(255, 255, 255, 80); }"
        "QComboBox:on { background-color: rgba(30, 30, 35, 255); border-color: rgba(255, 255, 255, 100); }"
        "QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: top right; width: 30px; border-left-width: 0px; border-top-right-radius: 15px; border-bottom-right-radius: 15px; background: transparent; }"
        "QComboBox::down-arrow { image: " + arrowImage + "; width: 12px; height: 12px; }"
        "QComboBox QAbstractItemView { background-color: rgb(35, 35, 40); color: rgba(255, 255, 255, 200); border: 1px solid rgba(80, 80, 80, 255); selection-background-color: rgba(80, 80, 90, 255); selection-color: white; outline: 0; border-radius: 8px; padding: 4px; }";

    const QString labelStyle = "QLabel{ background-color:rgba(77,77,115,136); border-style:outset; border-width:1px; border-radius:15px; border-color:rgba(255,255,255,30); color:rgba(255,255,255,255); padding:6px; }";
}

OpticalNavigation::OpticalNavigation(QWidget* parent) : BaseModule(parent)
{
    ui.setupUi(this);
    qRegisterMetaType<TrackedTool>("TrackedTool");
    qRegisterMetaType<TrackedTool>("TrackedTool");
    qRegisterMetaType<QVector<TrackedTool>>("QVector<TrackedTool>");
    loadConfig();
    init();
    createActions();
}

OpticalNavigation::~OpticalNavigation()
{
    if (m_ndiWorker) emit reqDisconnectDevice();
    if (m_workerThread) {
        m_workerThread->quit();
        m_workerThread->wait();
    }
}

void OpticalNavigation::init()
{
    initWorker();
    initButton();
    initSplitters();
}

void OpticalNavigation::applyTheme(QPushButton* btn, const QString& baseStyle, const QString& pressedStyle, bool extraBorders)
{
    if (!btn) return;
    QString finalStyle = baseStyle;
    
    // 保留原有背景图
    QString currentSheet = btn->styleSheet();
    QRegularExpression regex("image\\s*:\\s*url\\([^)]+\\);", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = regex.match(currentSheet);
    
    QString imageStyle = "";
    if (match.hasMatch()) imageStyle = match.captured(0);

    // [修复]：如果需要 extraBorders，必须将其插入到 baseStyle 的 '}' 之前
    if (extraBorders) {
        QString borderStyle = "border-top:1px solid rgb(237, 238, 241); border-bottom:1px solid rgb(237, 238, 241); ";
        int lastBrace = finalStyle.lastIndexOf('}');
        if (lastBrace != -1) {
            finalStyle.insert(lastBrace, borderStyle);
        } else {
            finalStyle += " { " + borderStyle + " }"; // 如果格式不对，尝试补救
        }
    }

    // 确保把背景图也塞进去（如果有）
    if (!imageStyle.isEmpty()) {
        int lastBrace = finalStyle.lastIndexOf('}');
        if (lastBrace != -1) finalStyle.insert(lastBrace, imageStyle);
    }
    
    // 追加其他状态的样式（这些是独立的块，可以直接 +=）
    finalStyle += pressedStyle + hoverStyle + toolTipStyle;
    btn->setStyleSheet(finalStyle);
}

void OpticalNavigation::initButton()
{
    applyTheme(ui.connect_btn, textBaseStyle, pressedGreen, true);
    applyTheme(ui.track_btn, textBaseStyle, pressedGreen, true);
    applyTheme(ui.apply_regis_btn, textBaseStyle, pressedGreen, true);
    applyTheme(ui.apply_cali_btn, textBaseStyle, pressedGreen, true);
    applyTheme(ui.start_sp_btn, textBaseStyle, pressedGreen, true);
    
    auto setupLabel = [](QLabel* lbl) { if(lbl) { lbl->setStyleSheet(labelStyle); lbl->setAlignment(Qt::AlignCenter); }};
    setupLabel(ui.probe_sp_lb);
    setupLabel(ui.input_sp_lb);
    setupLabel(ui.output_sp_lb);
    setupLabel(ui.source_regis_lb);
    setupLabel(ui.target_regis_lb);
    setupLabel(ui.output_regis_lb);
    setupLabel(ui.probe_cali_lb);
    setupLabel(ui.tool_cali_lb);
    setupLabel(ui.output_cali_lb);

    auto setupCombo = [](QComboBox* cb) { if(cb) { cb->setStyleSheet(darkComboBoxStyle); cb->setView(new QListView()); }};
    setupCombo(ui.probe_sp_cbb);
    setupCombo(ui.input_sp_cbb);
    setupCombo(ui.output_sp_cbb);
    setupCombo(ui.source_regis_cbb);
    setupCombo(ui.target_regis_cbb);
    setupCombo(ui.output_regis_cbb);
    setupCombo(ui.probe_cali_cbb);
    setupCombo(ui.tool_cali_cbb);
    setupCombo(ui.output_cali_cbb);
}

// ... initSplitters, loadConfig, initWorker, createActions 等保持不变 ...

void OpticalNavigation::initSplitters()
{
    m_vsplitter = new QSplitter(Qt::Vertical, this);
    m_vsplitter->addWidget(ui.init_lb);
    m_vsplitter->addWidget(ui.init_box);
    m_vsplitter->addWidget(ui.samp_lb);
    m_vsplitter->addWidget(ui.sp_box);
    m_vsplitter->addWidget(ui.regis_lb);
    m_vsplitter->addWidget(ui.regis_box);
    m_vsplitter->addWidget(ui.cali_lb);
    m_vsplitter->addWidget(ui.cali_box);
    
    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    m_vsplitter->addWidget(spacer);

    m_vsplitter->setHandleWidth(0);
    m_vsplitter->setContentsMargins(0, 0, 0, 0);
    m_vsplitter->setChildrenCollapsible(true);
    m_vsplitter->setSizes({100, 400, 100, 800, 100, 800, 100, 800, 900});

    ui.centralwidget->layout()->setContentsMargins(0, 0, 0, 0);
    ui.centralwidget->layout()->setSpacing(0);
    ui.centralwidget->layout()->replaceWidget(ui.module_widget, m_vsplitter);

    ui.module_widget->hide();
    m_vsplitter->setMouseTracking(true);
}

void OpticalNavigation::loadConfig()
{
    QString path = QCoreApplication::applicationDirPath() + "/OpticalNavigation_config.json";
    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        if (doc.isObject()) m_config = doc.object();
        file.close();
    } else {
        qWarning() << "Failed to load config file:" << path;
    }
}

void OpticalNavigation::initWorker()
{
    m_workerThread = new QThread(this);
    m_ndiWorker = new NDICombinedWorker();
    m_ndiWorker->moveToThread(m_workerThread);

    connect(this, &OpticalNavigation::reqConnectDevice, m_ndiWorker, &NDICombinedWorker::connectDevice);
    connect(this, &OpticalNavigation::reqDisconnectDevice, m_ndiWorker, &NDICombinedWorker::disconnectDevice);
    connect(this, &OpticalNavigation::reqAddTools, m_ndiWorker, &NDICombinedWorker::setTools);
    connect(this, &OpticalNavigation::reqStartCollecting, m_ndiWorker, &NDICombinedWorker::startCollecting);
    connect(this, &OpticalNavigation::reqStopTracking, m_ndiWorker, &NDICombinedWorker::stopTracking);

    connect(m_ndiWorker, &NDICombinedWorker::allToolsUpdated, this, &OpticalNavigation::onToolDataReceived);
    connect(m_ndiWorker, &NDICombinedWorker::statusMessage, this, &OpticalNavigation::onStatusReceived);
    connect(m_ndiWorker, &NDICombinedWorker::errorOccurred, this, &OpticalNavigation::onErrorReceived);
    connect(m_ndiWorker, &NDICombinedWorker::onBatchToolsLoaded, this, &OpticalNavigation::onToolsAdded);

    connect(m_workerThread, &QThread::finished, m_ndiWorker, &QObject::deleteLater);
    connect(m_workerThread, &QThread::finished, m_workerThread, &QObject::deleteLater);
    m_workerThread->start();
}

void OpticalNavigation::onConnectBtnClicked()
{
    if (ui.connect_btn->text() == "Start Connection") {
        ui.connect_btn->setText("Connecting");
        ui.connect_btn->setStyleSheet(connectingStyle);
        
        QString ip = m_config.contains("Device_IP") ? m_config["Device_IP"].toString() : "192.168.1.202";
        emit reqConnectDevice(ip); 
    } else {
        emit reqDisconnectDevice();
    }
}

void OpticalNavigation::onTrackingBtnClicked()
{
    if (ui.track_btn->text() == "Start Tracking") {
        std::vector<Tool*> tools;
        emit reqGetTools(tools);

        QMap<QString, QString> toolsConfig;
        for (Tool* tool : tools) {
            if (tool) toolsConfig.insert(QString::fromStdString(tool->getName()), QString::fromStdString(tool->getPath()));
        }

        if (!toolsConfig.isEmpty()) emit reqAddTools(toolsConfig);
    } else {
        emit reqStopTracking();
    }
}

void OpticalNavigation::onStatusReceived(QString msg)
{
    if (msg.contains("Connected")) {
        ui.connect_btn->setText("Stop Connection");
        ui.connect_btn->setStyleSheet(activeStyle);
    } else if (msg.contains("Tracking started")) {
        ui.track_btn->setText("Stop Tracking");
        ui.track_btn->setStyleSheet(activeStyle);
    } else if (msg.contains("Tracking stopped")) {
        ui.track_btn->setText("Start Tracking");
        applyTheme(ui.track_btn, textBaseStyle, pressedGreen, true);
        ui.probe_sp_cbb->setStyleSheet(darkComboBoxStyle);
        ui.probe_cali_cbb->setStyleSheet(darkComboBoxStyle);
        ui.tool_cali_cbb->setStyleSheet(darkComboBoxStyle);
        emit signalStopTracking();
    } else if (msg.contains("Device Disconnected")) {
        ui.connect_btn->setText("Start Connection");
        applyTheme(ui.connect_btn, textBaseStyle, pressedGreen, true);
        ui.track_btn->setText("Start Tracking");
        applyTheme(ui.track_btn, textBaseStyle, pressedGreen, true);
        ui.probe_sp_cbb->setStyleSheet(darkComboBoxStyle);
        ui.probe_cali_cbb->setStyleSheet(darkComboBoxStyle);
        ui.tool_cali_cbb->setStyleSheet(darkComboBoxStyle);
        emit signalStopTracking();
    }
    qDebug() << msg;
}

void OpticalNavigation::createActions()
{
    connect(ui.connect_btn, &QPushButton::clicked, this, &OpticalNavigation::onConnectBtnClicked);
    connect(ui.track_btn, &QPushButton::clicked, this, &OpticalNavigation::onTrackingBtnClicked);
    connect(ui.start_sp_btn, &QPushButton::clicked, this, &OpticalNavigation::onSamplingPointBtnClicked);
    connect(ui.apply_regis_btn, &QPushButton::clicked, this, &OpticalNavigation::onRegistration);
    connect(ui.apply_cali_btn, &QPushButton::clicked, this, &OpticalNavigation::onCalibration);

    auto handleNewItem = [this](QComboBox* cbb, const QString& placeholder, const QString& title) {
        if (cbb->currentText() == placeholder) {
            bool ok;
            QString text = QInputDialog::getText(this, title, "Name:", QLineEdit::Normal, title, &ok);
            if (ok && !text.isEmpty()) {
                QString originalText = text;
                int counter = 1;
                while (cbb->findText(text) != -1) text = originalText + "_" + QString::number(counter++);
                cbb->addItem(text);
                cbb->setCurrentIndex(cbb->count() - 1);
            } else {
                cbb->setCurrentIndex(0);
            }
        }
    };

    connect(ui.output_sp_cbb, QOverload<int>::of(&QComboBox::activated), this, [=](int index) {
        handleNewItem(ui.output_sp_cbb, "Create a new point set as ...", "New Point Set");
    });

    connect(ui.output_regis_cbb, QOverload<int>::of(&QComboBox::activated), this, [=](int index) {
        handleNewItem(ui.output_regis_cbb, "Create a new transform as ...", "New Registration Transform");
    });

    connect(ui.output_cali_cbb, QOverload<int>::of(&QComboBox::activated), this, [=](int index) {
        handleNewItem(ui.output_cali_cbb, "Create a new transform as ...", "New Calibration Transform");
    });
}

void OpticalNavigation::onRomChanged(const QString& text)
{
    if (text.isEmpty()) return;
    qDebug() << "ROM Selection Changed to:" << text;
}

void OpticalNavigation::onToolChanged(const QString& text)
{
    if (text.isEmpty()) return;
    qDebug() << "Tool Selection Changed to:" << text;
}

void OpticalNavigation::onErrorReceived(QString err)
{
    qDebug() << err;
    QMessageBox::critical(this, "NDI Error", err);
    if (err.contains("System Initialization Failed!") || err.contains("Connection failed")){
        if (ui.connect_btn->text() != "Start Connection") {
            ui.connect_btn->setText("Start Connection");
            applyTheme(ui.connect_btn, textBaseStyle, pressedGreen, true);
        }
    }
}

void OpticalNavigation::onToolsAdded(QMap<int, QString> tools)
{
    emit reqStartCollecting();
}

void OpticalNavigation::onSamplingPointBtnClicked()
{
    if (ui.start_sp_btn->text() == "Stop Sampling") {
        stopSamplingLogic();
        return;
    }
    if (ui.track_btn->text() == "Start Tracking") {
        QMessageBox::warning(this, "Warning", "Please start tracking first!");
        return;
    }
    if (ui.probe_sp_cbb->currentIndex() == 0) {
        QMessageBox::warning(this, "Warning", "Please select a probe!");
        return;
    }
    if (ui.input_sp_cbb->currentIndex() == 0) {
        QMessageBox::warning(this, "Warning", "Please select an input point set!");
        return;
    }
    if (ui.output_sp_cbb->currentIndex() == 0) {
        QMessageBox::warning(this, "Warning", "Please select an output point set!");
        return;
    }

    m_ProbeName = ui.probe_sp_cbb->currentText();
    m_inputPointSetName = ui.input_sp_cbb->currentText();
    m_outputPointSetName = ui.output_sp_cbb->currentText();

    std::vector<Landmark*> landmarks;
    emit reqGetLandmarks(landmarks, m_inputPointSetName);
    emit reqLandmarksColorUpdate();
    
    if (landmarks.empty()) {
        QMessageBox::warning(this, "Error", "Input point set is empty or not found!");
        return;
    }
    m_targetPointCount = landmarks.size(); 

    m_isSampling = true;
    m_stabilityBuffer.clear();
    m_waitingForExit = false;
    m_stabilityTimer.invalidate();

    ui.start_sp_btn->setText("Stop Sampling");
    applyTheme(ui.start_sp_btn, textBaseStyle, pressedGreen, true);
    qDebug() << "Started sampling for" << m_ProbeName << "Target count:" << m_targetPointCount;
}

void OpticalNavigation::stopSamplingLogic()
{
    m_isSampling = false;
    m_stabilityBuffer.clear();
    ui.start_sp_btn->setText("Start Sampling");
    applyTheme(ui.start_sp_btn, textBaseStyle, hoverStyle, true); 
    qDebug() << "Sampling stopped.";
}

void OpticalNavigation::addPointToDataManager(const Eigen::Vector3d& point)
{
    qDebug() << "!!! EXECUTE BEEP !!!"; 
    emit reqAddLandmark(m_outputPointSetName, point.x(), point.y(), point.z());
    emit reqLandmarksColorUpdate();

    QApplication::beep();
}


void OpticalNavigation::onToolDataReceived(const QVector<TrackedTool>& tools)
{
    // 1. UI Updates
    auto updateCbbColor = [&](QComboBox* cbb, const TrackedTool& data) {
        if (cbb->currentText() == data.name) {
            QString colorStr = data.isVisible ? "rgb(0, 255, 0)" : "rgb(255, 0, 0)";
            QString weightStr = data.isVisible ? "bold" : "normal";
            QString newStyle = darkComboBoxStyle + QString("QComboBox { color: %1; font-weight: %2; }").arg(colorStr, weightStr);
            if (cbb->styleSheet() != newStyle) cbb->setStyleSheet(newStyle);
        }
    };

    for (const auto& data : tools) {
        updateCbbColor(ui.probe_sp_cbb, data);
        updateCbbColor(ui.probe_cali_cbb, data);
        updateCbbColor(ui.tool_cali_cbb, data);
    }

    std::vector<Tool*> allTools;
    emit reqGetTools(allTools);

    // 2. Update Tool Matrices
    updateToolMatrices(tools, allTools);

    emit signalAllToolsUpdated(tools);

    // 3. Handle Calibration
    if (m_isCalibrating) {
        handleCalibrationLogic(tools, allTools);
        return;
    }

    // 4. Handle Point Sampling
    if (m_isSampling) {
        handleSamplingLogic(tools, allTools);
    } else {
        emit signalSamplingStatus("");
    }
}

void OpticalNavigation::updateToolMatrices(const QVector<TrackedTool>& tools, const std::vector<Tool*>& allTools)
{
    for (const auto& data : tools) {
        for (Tool* t : allTools) {
            if (t && QString::fromStdString(t->getName()) == data.name) {
                vtkSmartPointer<vtkMatrix4x4> vtkMat = vtkSmartPointer<vtkMatrix4x4>::New();
                for(int r=0; r<4; r++) for(int c=0; c<4; c++) vtkMat->SetElement(r, c, data.matrix(r, c));
                t->setHandle(data.portHandle);
                t->setRawMatrix(vtkMat); 
                t->setRMSE(data.error);
                break;
            }
        }
    }
}

void OpticalNavigation::handleCalibrationLogic(const QVector<TrackedTool>& tools, const std::vector<Tool*>& allTools)
{
    QString probeName = ui.probe_cali_cbb->currentText();
    QString targetToolName = ui.tool_cali_cbb->currentText();

    ui.apply_cali_btn->setText(QString("Keep Steady! %1/%2 ")
                               .arg(m_caliSampleCount)
                               .arg(m_caliTargetSamples));

    if (m_isPivotCalibration) {
        const TrackedTool* targetData = nullptr;
        for (const auto& d : tools) {
            if (d.name == targetToolName) targetData = &d;
        }

        if (targetData && targetData->isVisible) {
             Tool* targetTool = nullptr;
             for (auto t : allTools) 
                 if (QString::fromStdString(t->getName()) == targetToolName) targetTool = t;

             if (targetTool) {
                 vtkMatrix4x4* vtkT = targetTool->getRawMatrix();
                 Eigen::Matrix4d matT;
                 for(int i=0; i<4; i++) for(int j=0; j<4; j++) matT(i,j) = vtkT->GetElement(i,j);

                 m_caliBufferProbe.push_back(matT); 
                 m_caliSampleCount++;
             }
        }
    } else {
        const TrackedTool* probeData = nullptr;
        const TrackedTool* targetData = nullptr;

        for (const auto& d : tools) {
            if (d.name == probeName) probeData = &d;
            if (d.name == targetToolName) targetData = &d;
        }

        if (probeData && probeData->isVisible && targetData && targetData->isVisible) {
            Tool* ptrProbe = nullptr;
            Tool* ptrTarget = nullptr;

            for (auto t : allTools) {
                QString tName = QString::fromStdString(t->getName());
                if (tName == probeName) ptrProbe = t;
                if (tName == targetToolName) ptrTarget = t;
            }

            if (ptrProbe && ptrTarget) {
                vtkMatrix4x4* vtkP = ptrProbe->getFinalMatrix();
                Eigen::Matrix4d matP;
                for(int i=0; i<4; i++) for(int j=0; j<4; j++) matP(i,j) = vtkP->GetElement(i,j);

                vtkMatrix4x4* vtkT = ptrTarget->getRawMatrix();
                Eigen::Matrix4d matT;
                for(int i=0; i<4; i++) for(int j=0; j<4; j++) matT(i,j) = vtkT->GetElement(i,j);

                if (!m_caliBufferProbe.empty()) {
                    Eigen::Vector3d startPosP = m_caliBufferProbe[0].block<3, 1>(0, 3);
                    Eigen::Vector3d currPosP = matP.block<3, 1>(0, 3);
                    double distP = (currPosP - startPosP).norm();

                    Eigen::Vector3d startPosT = m_caliBufferTool[0].block<3, 1>(0, 3);
                    Eigen::Vector3d currPosT = matT.block<3, 1>(0, 3);
                    double distT = (currPosT - startPosT).norm();

                    if (distP > 3.0 || distT > 3.0) {
                        m_caliBufferProbe.clear();
                        m_caliBufferTool.clear();
                        m_caliSampleCount = 0;
                        
                        ui.apply_cali_btn->setText("Moved! Restarting...");
                        return; 
                    }
                }

                m_caliBufferProbe.push_back(matP);
                m_caliBufferTool.push_back(matT);
                m_caliSampleCount++;
            }
        }
    }

    if (m_caliSampleCount >= m_caliTargetSamples) {
        m_isCalibrating = false; 
        finishCalibrationCalculation(); 
        
        ui.apply_cali_btn->setText("Apply Calibration");
        ui.apply_cali_btn->setEnabled(true);
        applyTheme(ui.apply_cali_btn, textBaseStyle, pressedGreen, true);
    }
}

void OpticalNavigation::handleSamplingLogic(const QVector<TrackedTool>& tools, const std::vector<Tool*>& allTools)
{
    const TrackedTool* currentProbeData = nullptr;
    for (const auto& d : tools) {
        if (d.name == m_ProbeName) {
            currentProbeData = &d;
            break;
        }
    }

    if (!currentProbeData) return;

    if (!currentProbeData->isVisible) {
        m_stabilityBuffer.clear();
        emit signalSamplingStatus("Missing");
        return;
    }

    Tool* probeTool = nullptr;
    for(auto t : allTools) if(QString::fromStdString(t->getName()) == m_ProbeName) probeTool = t;

    processSamplingData(*currentProbeData, probeTool);
}

void OpticalNavigation::processSamplingData(const TrackedTool& data, Tool* probeTool)
{
    Eigen::Vector3d currentPos;
    if (probeTool && probeTool->getRawMatrix()) {
        vtkMatrix4x4* finalMat = probeTool->getRawMatrix();
        currentPos[0] = finalMat->GetElement(0, 3);
        currentPos[1] = finalMat->GetElement(1, 3);
        currentPos[2] = finalMat->GetElement(2, 3);
    } else {
        currentPos[0] = data.matrix(0, 3); 
        currentPos[1] = data.matrix(1, 3); 
        currentPos[2] = data.matrix(2, 3); 
    }

    if (m_waitingForExit) {
        emit signalSamplingStatus("Move Away");
        if ((currentPos - m_lastCapturedPoint).norm() > m_tolerance * 5.0) {
            m_waitingForExit = false; 
            m_stabilityBuffer.clear();
            emit signalSamplingStatus("Ready");
        }
        return; 
    }

    if (m_stabilityBuffer.empty()) {
        m_stabilityBuffer.push_back(currentPos);
        m_stabilityTimer.start(); 
        emit signalSamplingStatus(QString("0 ms / %1 ms").arg(m_stable_time));
    } else {
        double dist = (currentPos - m_stabilityBuffer.front()).norm();
        if (dist > m_tolerance) {
            m_stabilityBuffer.clear();
            m_stabilityBuffer.push_back(currentPos); 
            m_stabilityTimer.restart();
            emit signalSamplingStatus(QString("0 ms / %1 ms").arg(m_stable_time));
        } else {
            m_stabilityBuffer.push_back(currentPos);
            
            int elapsedMs = m_stabilityTimer.elapsed();
            int displayMs = (elapsedMs > m_stable_time) ? m_stable_time : elapsedMs;
            emit signalSamplingStatus(QString("%1 ms / %2 ms").arg(displayMs).arg(m_stable_time));

            if (elapsedMs >= m_stable_time) {
                emit signalSamplingStatus("Captured!");

                Eigen::Vector3d avgPos = Eigen::Vector3d::Zero();
                for (const auto& p : m_stabilityBuffer) avgPos += p;
                avgPos /= static_cast<double>(m_stabilityBuffer.size());

                addPointToDataManager(avgPos);
                m_lastCapturedPoint = avgPos;

                std::vector<Landmark*> sourceLandmarks;
                emit reqGetLandmarks(sourceLandmarks, m_outputPointSetName);
                std::vector<Landmark*> targetLandmarks;
                emit reqGetLandmarks(targetLandmarks, m_inputPointSetName);

                try {
                    Eigen::Matrix4d resultMat = calculateRegistrationFromLandmarks(sourceLandmarks, targetLandmarks);
                    qDebug() << "Calculation done. Translation:" << resultMat(0,3) << resultMat(1,3) << resultMat(2,3);
                    emit setToolRegisTransform(m_ProbeName, resultMat);
                } catch (const std::exception& e) {
                    qDebug() << "Registration Exception:" << e.what();
                    QMessageBox::critical(this, "Error", QString("Registration failed: %1").arg(e.what()));
                } catch (...) {
                    qDebug() << "Unknown crash occurred in registration!";
                }

                m_waitingForExit = true;
                m_stabilityBuffer.clear();

                std::vector<Landmark*> currentLandmarks;
                emit reqGetLandmarks(currentLandmarks, m_outputPointSetName);
                if (currentLandmarks.size() >= m_targetPointCount) {
                    QMessageBox::information(this, "Finished", "Sampling completed!");
                    stopSamplingLogic();
                }
            }
        }
    }
}


void OpticalNavigation::onRegistration()
{
    qDebug() << ">>> Entering onRegistration function...";

    if (ui.source_regis_cbb->currentIndex() <= 0 || ui.target_regis_cbb->currentIndex() <= 0 || ui.output_regis_cbb->currentIndex() <= 0) {
        QMessageBox::warning(this, "Warning", "Please select Source, Target and Output!");
        return;
    }

    QString sourceName = ui.source_regis_cbb->currentText();
    QString targetName = ui.target_regis_cbb->currentText();
    QString outputName = ui.output_regis_cbb->currentText();

    std::vector<Landmark*> sourceLandmarks;
    emit reqGetLandmarks(sourceLandmarks, sourceName);
    std::vector<Landmark*> targetLandmarks;
    emit reqGetLandmarks(targetLandmarks, targetName);

    try {
        Eigen::Matrix4d resultMat = calculateRegistrationFromLandmarks(sourceLandmarks, targetLandmarks);
        qDebug() << "Calculation done. Translation:" << resultMat(0,3) << resultMat(1,3) << resultMat(2,3);
        emit reqUpdateTransform(outputName, resultMat);
        
        size_t validCount = std::min(sourceLandmarks.size(), targetLandmarks.size());
        QString successMsg;
        if (validCount == 1) successMsg = "Success: Point-to-Point Translation applied (1 point).";
        else if (validCount == 2) successMsg = "Success: Line Alignment (Translation + Rotation) applied (2 points).";
        else successMsg = QString("Success: Full Rigid Body Registration applied (%1 points).").arg(validCount);

        QMessageBox::information(this, "Registration Finished", successMsg);

    } catch (const std::exception& e) {
        qDebug() << "Registration Exception:" << e.what();
        QMessageBox::critical(this, "Error", QString("Registration failed: %1").arg(e.what()));
    } catch (...) {
        qDebug() << "Unknown crash occurred in registration!";
        QMessageBox::critical(this, "Error", "Unknown error occurred during registration.");
    }
}

Eigen::Matrix4d OpticalNavigation::calculateRegistrationFromLandmarks(const std::vector<Landmark*>& sourceList, const std::vector<Landmark*>& targetList)
{
    size_t validCount = std::min(sourceList.size(), targetList.size());
    qDebug() << "Calculating Registration. Source:" << sourceList.size() << " Target:" << targetList.size() << " -> Valid Count:" << validCount;

    if (validCount < 1) throw std::runtime_error("No common points found (Count is 0).");

    auto landmarksToEigen = [](const std::vector<Landmark*>& landmarks, size_t maxCount) -> Eigen::MatrixXd {
        Eigen::MatrixXd mat(maxCount, 3);
        for (size_t i = 0; i < maxCount; ++i) {
            Landmark* lm = landmarks[i];
            if (!lm) throw std::runtime_error("Null landmark pointer detected.");
            const double* coords = lm->getCoordinates();
            if (!coords) throw std::runtime_error("Null coordinates data detected.");
            mat(i, 0) = coords[0]; mat(i, 1) = coords[1]; mat(i, 2) = coords[2];
        }
        return mat;
    };

    Eigen::MatrixXd sourceMat = landmarksToEigen(sourceList, validCount);
    Eigen::MatrixXd targetMat = landmarksToEigen(targetList, validCount);

    if (sourceMat.rows() == 0 || targetMat.rows() == 0) throw std::runtime_error("Matrix conversion resulted in empty data.");
    return GeometryUtils::computeRegistration(sourceMat, targetMat);
}

Eigen::Matrix4d OpticalNavigation::computeAverageMatrix(const std::vector<Eigen::Matrix4d>& matrices)
{
    if (matrices.empty()) return Eigen::Matrix4d::Identity();

    Eigen::Vector3d sumPos = Eigen::Vector3d::Zero();
    Eigen::Vector4d sumQuat = Eigen::Vector4d::Zero(); 

    for (const auto& mat : matrices) {
        sumPos += mat.block<3, 1>(0, 3);
        Eigen::Matrix3d rot = mat.block<3, 3>(0, 0);
        Eigen::Quaterniond q(rot);
        
        if (!matrices.empty()) {
            Eigen::Matrix3d firstRot = matrices[0].block<3, 3>(0, 0);
            Eigen::Quaterniond firstQ(firstRot);
            if (q.dot(firstQ) < 0.0) {
                q.coeffs() = -q.coeffs();
            }
        }
        sumQuat += q.coeffs();
    }

    sumPos /= static_cast<double>(matrices.size());
    sumQuat.normalize(); 
    Eigen::Quaterniond avgQ(sumQuat);

    Eigen::Matrix4d avgMat = Eigen::Matrix4d::Identity();
    avgMat.block<3, 3>(0, 0) = avgQ.toRotationMatrix();
    avgMat.block<3, 1>(0, 3) = sumPos;

    return avgMat;
}

void OpticalNavigation::onCalibration()
{
    // Basic checks
    if (ui.track_btn->text() == "Start Tracking") {
        QMessageBox::warning(this, "Warning", "Please start tracking first!");
        return;
    }
    if (ui.tool_cali_cbb->currentIndex() == 0) {
        QMessageBox::warning(this, "Warning", "Please select the Target Tool to calibrate!");
        return;
    }
    if (ui.output_cali_cbb->currentIndex() == 0) {
        QMessageBox::warning(this, "Warning", "Please select an output transform!");
        return;
    }

    QString probeName = ui.probe_cali_cbb->currentText();
    QString targetToolName = ui.tool_cali_cbb->currentText();

    // 1. Determine Mode based on name equality
    // If names are same -> Pivot Mode (Self). If different -> Tip-to-Tip.
    m_isPivotCalibration = (probeName == targetToolName); 

    std::vector<Tool*> allTools;
    emit reqGetTools(allTools); 
    
    bool probeFound = false;
    bool targetFound = false;
    for(Tool* t : allTools) {
        if (!t) continue;
        if(QString::fromStdString(t->getName()) == probeName) probeFound = true;
        if(QString::fromStdString(t->getName()) == targetToolName) targetFound = true;
    }

    // 2. Validate Tools
    if (m_isPivotCalibration) {
        // In Pivot mode, we only need the Target Tool
        if (!targetFound) {
            QMessageBox::warning(this, "Error", "Target Tool not found!");
            return;
        }
    } else {
        // In Tip-to-Tip mode, both must exist
        if (!probeFound || !targetFound) {
            QMessageBox::warning(this, "Error", "Selected tools not found!");
            return;
        }
    }

    // 3. Initialize State
    m_isCalibrating = true;
    m_caliSampleCount = 0;
    m_caliBufferProbe.clear();
    m_caliBufferTool.clear();

    ui.apply_cali_btn->setEnabled(false); 
    applyTheme(ui.apply_cali_btn, textBaseStyle, "background-color: #e67e22;", true); 

    // 4. Update UI Text based on Mode
    if (m_isPivotCalibration) {
        // Mode A: Self Pivot
        ui.apply_cali_btn->setText("Pivoting... Rotate Tool!"); 
        qDebug() << "Calibration Started: Pivot Mode (Single Tool)";
    } else {
        // Mode B: Tip-to-Tip
        ui.apply_cali_btn->setText("Calibrating... Keep Steady!");
        qDebug() << "Calibration Started: Tip-to-Tip Mode (Two Tools)";
    }
}


void OpticalNavigation::finishCalibrationCalculation()
{
    // Route to the correct algorithm
    if (m_isPivotCalibration) {
        // 1. Self Pivot (SVD Algorithm)
        performProbePivotCalibration(); 
    } else {
        // 2. Tip-to-Tip (Iterative Averaging + Forced Straight)
        performTipToTipCalibration(); 
    }
}


void OpticalNavigation::performProbePivotCalibration()
{
    // 0. Check data sufficiency
    if (m_caliBufferProbe.empty()) {
        QMessageBox::warning(this, "Error", "No samples collected! Please hold the tip and rotate the probe.");
        return;
    }
    
    // We need at least quite a few samples for a good pivot fit
    if (m_caliBufferProbe.size() < 50) {
        QMessageBox::warning(this, "Warning", "Not enough samples. Please rotate more to cover a larger range.");
        return;
    }

    size_t n = m_caliBufferProbe.size();

    // 1. Setup Least Squares System: Ax = b
    // Equation: R_i * v_tip + t_i = p_pivot
    // Rearrange: R_i * v_tip - I * p_pivot = -t_i
    // Unknowns x (6x1): [v_tip_x, v_tip_y, v_tip_z, p_pivot_x, p_pivot_y, p_pivot_z]
    
    Eigen::MatrixXd A(3 * n, 6);
    Eigen::VectorXd b(3 * n);

    for (size_t i = 0; i < n; ++i) {
        Eigen::Matrix4d mat = m_caliBufferProbe[i];
        Eigen::Matrix3d R = mat.block<3, 3>(0, 0);
        Eigen::Vector3d t = mat.block<3, 1>(0, 3);

        // Fill A matrix: [R, -I]
        A.block<3, 3>(3 * i, 0) = R;
        A.block<3, 3>(3 * i, 3) = -Eigen::Matrix3d::Identity();

        // Fill b vector: -t
        b.segment<3>(3 * i) = -t;
    }

    // 2. Solve Ax = b using SVD or QR decomposition
    // x = [tip_offset(3) | pivot_pos(3)]
    Eigen::VectorXd x = A.colPivHouseholderQr().solve(b);

    Eigen::Vector3d tipOffsetLocal = x.head<3>();
    Eigen::Vector3d pivotPosWorld = x.tail<3>();

    // 3. Calculate RMSE (Root Mean Square Error) to verify quality
    double sumSquaredError = 0.0;
    for (size_t i = 0; i < n; ++i) {
        Eigen::Matrix4d mat = m_caliBufferProbe[i];
        Eigen::Vector3d currentTipWorld = (mat * tipOffsetLocal.homogeneous()).head<3>();
        double dist = (currentTipWorld - pivotPosWorld).norm();
        sumSquaredError += dist * dist;
    }
    double rmse = std::sqrt(sumSquaredError / n);

    qDebug() << "Pivot Calibration RMSE:" << rmse;

    // 4. Quality Check (e.g., threshold 0.5mm)
    if (rmse > 2.0) { // 2.0mm threshold, adjust as needed
        QMessageBox::warning(this, "Calibration Failed", 
            QString("High Error (RMSE: %1 mm).\nPlease keep the tip steady and rotate gently.").arg(rmse));
        return;
    }

    // 5. Construct Final Calibration Matrix
    // Pivot calibration only gives Translation. Rotation is usually Identity (Shaft aligned).
    Eigen::Matrix4d caliMat = Eigen::Matrix4d::Identity();
    
    // Set the calculated offset
    caliMat(0, 3) = tipOffsetLocal.x();
    caliMat(1, 3) = tipOffsetLocal.y();
    caliMat(2, 3) = tipOffsetLocal.z();

    // 6. Apply Results
    QString probeName = ui.probe_cali_cbb->currentText(); // Or however you get the probe name
    QString outputName = ui.output_cali_cbb->currentText();
    
    emit reqUpdateTransform(outputName, caliMat);
    emit setToolCalibrationTransform(probeName, caliMat);

    QApplication::beep();
    QMessageBox::information(this, "Success", 
        QString("Probe Calibrated!\nRMSE: %1 mm\nOffset: [%2, %3, %4]")
        .arg(rmse, 0, 'f', 3)
        .arg(tipOffsetLocal.x(), 0, 'f', 2)
        .arg(tipOffsetLocal.y(), 0, 'f', 2)
        .arg(tipOffsetLocal.z(), 0, 'f', 2));
}


// void OpticalNavigation::performTipToTipCalibration()
// {
//     if (m_caliBufferProbe.empty() || m_caliBufferTool.empty()) return;

//     size_t count = std::min(m_caliBufferProbe.size(), m_caliBufferTool.size());
    
//     Eigen::Vector3d sumTranslation = Eigen::Vector3d::Zero();
//     Eigen::Matrix3d avgRotation = Eigen::Matrix3d::Identity();

//     // 定义翻转矩阵：让 Tool 与 Probe 针尖对针尖 (Z轴相反)
//     Eigen::Matrix4d flipMat = Eigen::Matrix4d::Identity();
//     flipMat(0, 0) = 1.0; 
//     flipMat(1, 1) = 1.0; 
//     flipMat(2, 2) = -1.0; 

//     for (size_t i = 0; i < count; ++i) {
//         Eigen::Matrix4d probeFinal = m_caliBufferProbe[i]; 
//         Eigen::Matrix4d targetNoReg = m_caliBufferTool[i]; 
//         Eigen::Matrix4d targetGoal = probeFinal * flipMat;
//         Eigen::Matrix4d targetInv = targetNoReg.inverse();
//         Eigen::Matrix4d regStep = targetGoal * targetInv;

//         sumTranslation += regStep.block<3, 1>(0, 3);
//         avgRotation = regStep.block<3, 3>(0, 0);
//     }

//     Eigen::Matrix4d finalRegMat = Eigen::Matrix4d::Identity();
//     finalRegMat.block<3, 3>(0, 0) = avgRotation; 
//     finalRegMat.block<3, 1>(0, 3) = sumTranslation / static_cast<double>(count);


//     QString targetToolName = ui.tool_cali_cbb->currentText();
//     QString outputName = ui.output_cali_cbb->currentText();
    
//     emit reqUpdateTransform(outputName, finalRegMat);
//     emit setToolRegisTransform(targetToolName, finalRegMat);

//     QApplication::beep();
//     QMessageBox::information(this, "Registration Finished", 
//         QString("Tip-to-Tip Registration Applied.\n"
//                 "Tool is now registered to Probe's Tip (Opposite).\n"
//                 "Reg Offset: [%1, %2, %3]")
//         .arg(finalRegMat(0,3), 0, 'f', 2)
//         .arg(finalRegMat(1,3), 0, 'f', 2)
//         .arg(finalRegMat(2,3), 0, 'f', 2));
// }


// void OpticalNavigation::performTipToTipCalibration()
// {
//     if (m_caliBufferProbe.empty() || m_caliBufferTool.empty()) return;

//     std::vector<Tool*> allTools;
//     emit reqGetTools(allTools);

//     Tool* probeTool = nullptr;
//     for (auto t : allTools) {
//         if (QString::fromStdString(t->getName()) == m_ProbeName) {
//             probeTool = t;
//             break;
//         }
//     }

//     Eigen::Matrix4d probeReg = Eigen::Matrix4d::Identity();
//     Eigen::Matrix4d probeRegInv = Eigen::Matrix4d::Identity();

//     if (probeTool) {
//         vtkSmartPointer<vtkMatrix4x4> vtkReg = probeTool->getRegistrationMatrix();
//         if (vtkReg) {
//             for(int i=0; i<4; i++) {
//                 for(int j=0; j<4; j++) {
//                     probeReg(i, j) = vtkReg->GetElement(i, j);
//                 }
//             }
//             probeRegInv = probeReg.inverse();
//         }
//     }

//     size_t count = std::min(m_caliBufferProbe.size(), m_caliBufferTool.size());
//     Eigen::Vector3d sumOffsetLocal = Eigen::Vector3d::Zero();

//     for (size_t i = 0; i < count; ++i) {
//         Eigen::Vector4d probeFinalPos(
//             m_caliBufferProbe[i](0, 3), 
//             m_caliBufferProbe[i](1, 3), 
//             m_caliBufferProbe[i](2, 3), 
//             1.0);

//         Eigen::Vector4d probeWorldPos = probeRegInv * probeFinalPos;

//         Eigen::Matrix4d targetRaw = m_caliBufferTool[i];
//         Eigen::Matrix4d targetInv = targetRaw.inverse();
        
//         Eigen::Vector4d offsetLocal4 = targetInv * probeWorldPos;
//         sumOffsetLocal += offsetLocal4.head<3>();
//     }

//     Eigen::Vector3d avgOffset = sumOffsetLocal / static_cast<double>(count);

//     Eigen::Matrix4d caliMat = Eigen::Matrix4d::Identity();
//     caliMat(0, 3) = avgOffset.x();
//     caliMat(1, 3) = avgOffset.y();
//     caliMat(2, 3) = avgOffset.z();

//     QString targetToolName = ui.tool_cali_cbb->currentText();
//     QString outputName = ui.output_cali_cbb->currentText();
    
//     emit reqUpdateTransform(outputName, caliMat);
//     emit setToolCalibrationTransform(targetToolName, caliMat);
//     emit setToolRegisTransform(targetToolName, probeReg);

//     QApplication::beep();
//     QMessageBox::information(this, "Success", "Tool Calibrated & Registered via Tip-Touch!");
// }



void OpticalNavigation::performTipToTipCalibration()
{
    if (m_caliBufferProbe.empty() || m_caliBufferTool.empty()) return;

    std::vector<Tool*> allTools;
    emit reqGetTools(allTools);

    Tool* probeTool = nullptr;
    for (auto t : allTools) {
        if (QString::fromStdString(t->getName()) == m_ProbeName) {
            probeTool = t;
            break;
        }
    }

    Eigen::Matrix4d probeReg = Eigen::Matrix4d::Identity();
    Eigen::Matrix4d probeRegInv = Eigen::Matrix4d::Identity();

    if (probeTool) {
        vtkSmartPointer<vtkMatrix4x4> vtkReg = probeTool->getRegistrationMatrix();
        if (vtkReg) {
            for(int i=0; i<4; i++) {
                for(int j=0; j<4; j++) {
                    probeReg(i, j) = vtkReg->GetElement(i, j);
                }
            }
            probeRegInv = probeReg.inverse();
        }
    }

    size_t count = std::min(m_caliBufferProbe.size(), m_caliBufferTool.size());
    
    Eigen::Vector3d sumOffsetLocal = Eigen::Vector3d::Zero();
    Eigen::Vector4d sumQuaternion = Eigen::Vector4d::Zero(); // x, y, z, w

    // Flip matrix: Rotate 180 degrees around X-axis
    Eigen::Matrix3d flipRot = Eigen::AngleAxisd(M_PI, Eigen::Vector3d::UnitX()).toRotationMatrix();

    for (size_t i = 0; i < count; ++i) {
        // --- 1. Translation Calculation (Existing Logic) ---
        Eigen::Vector4d probeFinalPos(
            m_caliBufferProbe[i](0, 3), m_caliBufferProbe[i](1, 3), m_caliBufferProbe[i](2, 3), 1.0);

        Eigen::Vector4d probeWorldPos = probeRegInv * probeFinalPos;

        Eigen::Matrix4d targetRaw = m_caliBufferTool[i];
        Eigen::Matrix4d targetInv = targetRaw.inverse();
        
        Eigen::Vector4d offsetLocal4 = targetInv * probeWorldPos;
        sumOffsetLocal += offsetLocal4.head<3>();

        // --- 2. Rotation Calculation (New Logic) ---
        Eigen::Matrix4d probeTrackerMat = probeRegInv * m_caliBufferProbe[i]; 
        Eigen::Matrix3d probeRotTracker = probeTrackerMat.block<3, 3>(0, 0);

        // Target should be opposite to Probe
        Eigen::Matrix3d targetRotDesired = probeRotTracker * flipRot;
        
        // Calculate Cali Rotation: RawInv * Desired
        Eigen::Matrix3d targetRawRot = targetRaw.block<3, 3>(0, 0);
        Eigen::Matrix3d caliRotSample = targetRawRot.inverse() * targetRotDesired;

        // Accumulate Quaternion
        Eigen::Quaterniond q(caliRotSample);
        if (sumQuaternion.norm() > 0.0) {
             Eigen::Vector4d currentQ(q.x(), q.y(), q.z(), q.w());
             if (sumQuaternion.dot(currentQ) < 0.0) {
                 q.w() = -q.w(); q.x() = -q.x(); q.y() = -q.y(); q.z() = -q.z();
             }
        }
        sumQuaternion += Eigen::Vector4d(q.x(), q.y(), q.z(), q.w());
    }

    // --- Average ---
    Eigen::Vector3d avgOffset = sumOffsetLocal / static_cast<double>(count);
    
    sumQuaternion.normalize();
    Eigen::Quaterniond avgRotation(sumQuaternion(3), sumQuaternion(0), sumQuaternion(1), sumQuaternion(2)); // w, x, y, z

    // --- Result ---
    Eigen::Matrix4d caliMat = Eigen::Matrix4d::Identity();
    caliMat.block<3, 3>(0, 0) = avgRotation.toRotationMatrix();
    caliMat(0, 3) = avgOffset.x();
    caliMat(1, 3) = avgOffset.y();
    caliMat(2, 3) = avgOffset.z();

    QString targetToolName = ui.tool_cali_cbb->currentText();
    QString outputName = ui.output_cali_cbb->currentText();
    
    emit reqUpdateTransform(outputName, caliMat);
    emit setToolCalibrationTransform(targetToolName, caliMat);
    emit setToolRegisTransform(targetToolName, probeReg);

    QApplication::beep();
    QMessageBox::information(this, "Success", "6-DOF Calibration Complete.");
}