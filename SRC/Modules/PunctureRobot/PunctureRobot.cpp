#include "PunctureRobot.h"
#include "RobotWorker.h"
#include "OpticalNavigation.h"
#include <QMessageBox>
#include <QDebug>
#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QListView>
#include <Eigen/Dense>
#include <cmath>

#ifndef PI
#define PI 3.14159265358979323846
#endif

namespace {
    const QString toolTipStyle = "QToolTip { color: black; background-color: #ffffff; border: 1px solid gray; }";
    const QString textBaseStyle = "QPushButton{ background-color:rgba(77,77,115,136); border-style:outset; border-width:1px; border-radius:15px; border-color:rgba(255,255,255,30); color:rgba(255,255,255,255); padding:6px; }";
    const QString hoverStyle = "QPushButton:hover{ background-color:rgba(200,200,200,100); border-color:rgba(255,255,255,200); color:rgba(255,255,255,255); }";
    const QString pressedGreen = "QPushButton:pressed{ background-color:rgba(100,255,100,200); border-color:rgba(255,255,255,30); border-style:inset; color:rgba(255,255,255,255); }";
    const QString activeStyle = "QPushButton { background-color: rgba(46, 204, 113, 255); border-style: outset; border-width: 1px; border-radius: 15px; border-color: rgba(255, 255, 255, 50); color: white; padding: 6px; }";
    const QString connectingStyle = "QPushButton { background-color: rgba(241, 196, 15, 200); border-style: outset; border-width: 1px; border-radius: 15px; border-color: rgba(255, 255, 255, 150); color: white; padding: 6px; }";
    const QString labelStyle = "QLabel{ background-color:rgba(77,77,115,136); border-style:outset; border-width:1px; border-radius:15px; border-color:rgba(255,255,255,30); color:rgba(255,255,255,255); padding:6px; }";
    const QString darkComboBoxStyle = "QComboBox { background-color: rgba(45, 45, 48, 220); border: 1px solid rgba(255, 255, 255, 30); border-radius: 15px; padding: 6px; color: white; }";
    const QString darkSpinBoxStyle = 
        "QAbstractSpinBox { background-color: rgba(45, 45, 48, 220); border: 1px solid rgba(255, 255, 255, 30); border-radius: 10px; padding: 4px 10px; color: white; }"
        "QAbstractSpinBox::up-button { width: 20px; border-left: 1px solid rgba(255, 255, 255, 30); border-top-right-radius: 10px; background-color: rgba(77,77,115,50); }"
        "QAbstractSpinBox::up-button:hover { background-color: rgba(200,200,200,100); }"
        "QAbstractSpinBox::down-button { width: 20px; border-left: 1px solid rgba(255, 255, 255, 30); border-bottom-right-radius: 10px; background-color: rgba(77,77,115,50); }"
        "QAbstractSpinBox::down-button:hover { background-color: rgba(200,200,200,100); }";
}

PunctureRobot::PunctureRobot(QWidget* parent) : BaseModule(parent) {
    ui.setupUi(this);
    loadConfig();
    init();
    createActions();
}

PunctureRobot::~PunctureRobot() {
    emit reqDisconnectAll();
    if (m_workerThread) {
        m_workerThread->quit();
        m_workerThread->wait();
    }
}

void PunctureRobot::printInfo(const QString& message) { 
    QString htmlContent = QString("<div style=\"line-height: 150%; margin-bottom: 10px; color: red;\">--%1</div>").arg(message.toHtmlEscaped());
    ui.info_te->appendHtml(htmlContent);
}

void PunctureRobot::loadConfig() {
    QString path = QCoreApplication::applicationDirPath() + "/PunctureRobot_config.json";
    QFile file(path);
    m_matFlangeToTCP = Eigen::Matrix4d::Identity();
    
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        if (doc.isObject()) {
            m_config = doc.object();
            
            // 1. Parse the flange matrix.
            if (m_config.contains("flange2endeffector")) {
                QStringList list = m_config["flange2endeffector"].toString().split(",");
                if (list.size() == 16) {
                    int idx = 0;
                    for(int r=0; r<4; ++r) for(int c=0; c<4; ++c) 
                        m_matFlangeToTCP(r, c) = list[idx++].toDouble();
                }
            }

            // 2. Parse motion and safety parameters (input in mm, store internally as m).
            if (m_config.contains("ApproachDistance_mm")) {
                // Read 300.0; divide by 1000 to get 0.3 m.
                PARAM_APPROACH_DISTANCE = m_config["ApproachDistance_mm"].toDouble() / 1000.0;
            }
            if (m_config.contains("MotionTolerance_mm")) {
                PARAM_MOTION_TOLERANCE = m_config["MotionTolerance_mm"].toDouble() / 1000.0;
            }
            if (m_config.contains("NeedleVelInsert_mm_s")) {
                // Speed is already in mm/s; no conversion needed.
                PARAM_NEEDLE_VEL_INSERT = m_config["NeedleVelInsert_mm_s"].toDouble();
            }
            if (m_config.contains("NeedleVelRetract_mm_s")) {
                PARAM_NEEDLE_VEL_RETRACT = m_config["NeedleVelRetract_mm_s"].toDouble();
            }
            if (m_config.contains("SafetyMargin_mm")) {
                // Read 5.5; divide by 1000 to get 0.0055 m.
                SAFETY_MARGIN_M = m_config["SafetyMargin_mm"].toDouble() / 1000.0;
            }
        }
        file.close();
    } else {
        qDebug() << "Warning: Cannot open config file. Using default parameters.";
    }
    if (ui.app_sb) ui.app_sb->setValue(PARAM_APPROACH_DISTANCE * 1000.0);
    if (ui.tol_sb) ui.tol_sb->setValue(PARAM_MOTION_TOLERANCE * 1000.0);
    if (ui.vel1_sb) ui.vel1_sb->setValue(PARAM_NEEDLE_VEL_INSERT);
    if (ui.vel2_sb) ui.vel2_sb->setValue(PARAM_NEEDLE_VEL_RETRACT);
    if (ui.saf_sb) ui.saf_sb->setValue(SAFETY_MARGIN_M * 1000.0);
}

void PunctureRobot::init() {
    initWorker();
    initButton();
    initSplitters();
}

void PunctureRobot::initSplitters() {
    m_vsplitter = new QSplitter(Qt::Vertical, this);
    m_vsplitter->addWidget(ui.init_lb); m_vsplitter->addWidget(ui.init_box);
    m_vsplitter->addWidget(ui.calibrate_lb); m_vsplitter->addWidget(ui.calibrate_box);
    m_vsplitter->addWidget(ui.setting_lb); m_vsplitter->addWidget(ui.set_box);
    m_vsplitter->addWidget(ui.freeeDrive_lb); m_vsplitter->addWidget(ui.freeDrive_box);
    m_vsplitter->addWidget(ui.info_lb); m_vsplitter->addWidget(ui.info_te);
    m_vsplitter->setHandleWidth(0); 
    m_vsplitter->setContentsMargins(0, 0, 0, 0); 
    m_vsplitter->setChildrenCollapsible(true);
    m_vsplitter->setSizes({100, 400, 100, 600, 100, 1200, 100, 800, 100, 1200});
    ui.centralwidget->layout()->replaceWidget(ui.module_widget, m_vsplitter);
    ui.module_widget->hide(); 
    m_vsplitter->setMouseTracking(true);
}

void PunctureRobot::initWorker() {
    m_workerThread = new QThread(this);
    m_worker = new RobotWorker(); 
    m_worker->moveToThread(m_workerThread);
    connect(m_workerThread, &QThread::started, m_worker, &RobotWorker::init);
    connect(m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);
    
    // Connections
    connect(this, &PunctureRobot::reqConnectRobot, m_worker, &RobotWorker::connectRobot);
    connect(this, &PunctureRobot::reqConnectEndEffector, m_worker, &RobotWorker::connectStepper);
    connect(this, &PunctureRobot::reqDisconnectAll, m_worker, &RobotWorker::disconnectAll);
    connect(this, &PunctureRobot::reqStartDynamicPuncture, m_worker, &RobotWorker::startDynamicPuncture);
    connect(this, &PunctureRobot::reqUpdateTargetPose, m_worker, &RobotWorker::updateDynamicTarget);
    connect(this, &PunctureRobot::reqStopAll, m_worker, &RobotWorker::stopAll);
    connect(this, &PunctureRobot::reqSetFreeDrive, m_worker, &RobotWorker::setFreeDriveMode);
    connect(this, &PunctureRobot::reqStartJog, m_worker, &RobotWorker::startJog);
    connect(this, &PunctureRobot::reqStopJog, m_worker, &RobotWorker::stopJog);
    connect(this, &PunctureRobot::reqRobotPoseUpdated, m_worker, &RobotWorker::updateRobotPose);
    connect(this, &PunctureRobot::reqManualStepper, m_worker, &RobotWorker::manualControlStepper);

    // Feedbacks
    connect(m_worker, &RobotWorker::logMessage, this, &PunctureRobot::onWorkerLog);
    connect(m_worker, &RobotWorker::robotConnStatus, this, &PunctureRobot::onRobotStatusChanged);
    connect(m_worker, &RobotWorker::stepperConnStatus, this, &PunctureRobot::onEndEffectorStatusChanged);
    connect(m_worker, &RobotWorker::robotPoseUpdated, this, &PunctureRobot::onRobotPoseUpdated);
    connect(m_worker, &RobotWorker::punctureFinished, this, &PunctureRobot::onPunctureFinished);
    
    m_workerThread->start();
}

void PunctureRobot::initButton() {
    applyTheme(ui.connectRobot_btn, textBaseStyle, pressedGreen, true);
    applyTheme(ui.connectEndeffector_btn, textBaseStyle, pressedGreen, true);
    applyTheme(ui.calibrate_btn, textBaseStyle, pressedGreen, true);
    applyTheme(ui.URAction_btn, textBaseStyle, pressedGreen, true);
    applyTheme(ui.EFAction_btn, textBaseStyle, pressedGreen, true);
    applyTheme(ui.ReturnAction_btn, textBaseStyle, pressedGreen, true);
    
    applyTheme(ui.urXPlus_btn, textBaseStyle, pressedGreen, true);
    applyTheme(ui.urXMinus_btn, textBaseStyle, pressedGreen, true);
    applyTheme(ui.urYPlus_btn, textBaseStyle, pressedGreen, true);
    applyTheme(ui.urYMinus_btn, textBaseStyle, pressedGreen, true);
    applyTheme(ui.urZPlus_btn, textBaseStyle, pressedGreen, true);
    applyTheme(ui.urZMinus_btn, textBaseStyle, pressedGreen, true);
    applyTheme(ui.urXRotationPlus_btn, textBaseStyle, pressedGreen, true);
    applyTheme(ui.urXRotationMinus_btn, textBaseStyle, pressedGreen, true);
    applyTheme(ui.urYRotationPlus_btn, textBaseStyle, pressedGreen, true);
    applyTheme(ui.urYRotationMinus_btn, textBaseStyle, pressedGreen, true);
    applyTheme(ui.urZRotationPlus_btn, textBaseStyle, pressedGreen, true);
    applyTheme(ui.urZRotationMinus_btn, textBaseStyle, pressedGreen, true);
    applyTheme(ui.efForward_btn, textBaseStyle, pressedGreen, true);
    applyTheme(ui.efBackward_btn, textBaseStyle, pressedGreen, true);

    auto setupCombo = [](QComboBox* cb) { if(cb) { cb->setStyleSheet(darkComboBoxStyle); cb->setView(new QListView()); }};
    setupCombo(ui.probeTool_cbb); setupCombo(ui.efToolPrepare_cbb); setupCombo(ui.baseTool_cbb); setupCombo(ui.patientTool_cbb); setupCombo(ui.efTool_cbb); setupCombo(ui.trajPointset_cbb);
    
    auto setupLabel = [](QLabel* lbl) { if(lbl) { lbl->setStyleSheet(labelStyle); lbl->setAlignment(Qt::AlignCenter); }};
    setupLabel(ui.probeTool_lb); setupLabel(ui.efToolPrepare_lb); setupLabel(ui.baseTool_lb); setupLabel(ui.patientTool_lb); setupLabel(ui.efTool_lb); setupLabel(ui.trajPointset_lb);
    setupLabel(ui.app_lb); setupLabel(ui.tol_lb); setupLabel(ui.vel1_lb); setupLabel(ui.vel2_lb); setupLabel(ui.efTool_lb); setupLabel(ui.saf_lb);
    
    auto setupSpinBox = [](QAbstractSpinBox* sb) { if(sb) { sb->setStyleSheet(darkSpinBoxStyle); sb->setAlignment(Qt::AlignCenter); }};
    setupSpinBox(ui.app_sb); setupSpinBox(ui.tol_sb); setupSpinBox(ui.vel1_sb); setupSpinBox(ui.vel2_sb); setupSpinBox(ui.saf_sb);
}

void PunctureRobot::applyTheme(QPushButton* btn, const QString& baseStyle, const QString& pressedStyle, bool extraBorders) {
    if (!btn) return;
    QString finalStyle = baseStyle + pressedStyle + hoverStyle + toolTipStyle;
    btn->setStyleSheet(finalStyle);
}

void PunctureRobot::createActions() {
    auto connectJog = [&](QPushButton* btn, int axis, int dir) {
        connect(btn, &QPushButton::pressed, [=](){ emit reqStartJog(axis, dir); });
        connect(btn, &QPushButton::released, this, &PunctureRobot::onJogBtnReleased);
    };
    connectJog(ui.urXPlus_btn, 0, 1); connectJog(ui.urXMinus_btn, 0, -1);
    connectJog(ui.urYPlus_btn, 1, 1); connectJog(ui.urYMinus_btn, 1, -1);
    connectJog(ui.urZPlus_btn, 2, 1); connectJog(ui.urZMinus_btn, 2, -1);
    connectJog(ui.urXRotationPlus_btn, 3, 1); connectJog(ui.urXRotationMinus_btn, 3, -1);
    connectJog(ui.urYRotationPlus_btn, 4, 1); connectJog(ui.urYRotationMinus_btn, 4, -1);
    connectJog(ui.urZRotationPlus_btn, 5, 1); connectJog(ui.urZRotationMinus_btn, 5, -1);

    const double MANUAL_SPEED = 15.0; // mm/s
    connect(ui.efForward_btn, &QPushButton::pressed, this, [=](){ if(isConnectedEndEffector) emit reqManualStepper(MANUAL_SPEED); });
    connect(ui.efForward_btn, &QPushButton::released, this, [=](){ if(isConnectedEndEffector) emit reqManualStepper(0.0); });
    connect(ui.efBackward_btn, &QPushButton::pressed, this, [=](){ if(isConnectedEndEffector) emit reqManualStepper(-MANUAL_SPEED); });
    connect(ui.efBackward_btn, &QPushButton::released, this, [=](){ if(isConnectedEndEffector) emit reqManualStepper(0.0); });
    if (ui.app_sb) {
        connect(ui.app_sb, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [=](double val){
            PARAM_APPROACH_DISTANCE = val / 1000.0; // UI(mm) -> Backend(m)
        });
    }
    
    if (ui.tol_sb) {
        connect(ui.tol_sb, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [=](double val){
            PARAM_MOTION_TOLERANCE = val / 1000.0;  // UI(mm) -> Backend(m)
        });
    }
    
    if (ui.vel1_sb) {
        connect(ui.vel1_sb, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [=](double val){
            PARAM_NEEDLE_VEL_INSERT = val;          // mm/s unchanged.
        });
    }
    
    if (ui.vel2_sb) {
        connect(ui.vel2_sb, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [=](double val){
            PARAM_NEEDLE_VEL_RETRACT = val;         // mm/s unchanged.
        });
    }
    
    if (ui.saf_sb) {
        connect(ui.saf_sb, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [=](double val){
            SAFETY_MARGIN_M = val / 1000.0;         // UI(mm) -> Backend(m)
        });
    }
}

// ---------------------------------------------------------
// Calibration: Static Probe Method
// ---------------------------------------------------------
void PunctureRobot::on_calibrate_btn_clicked()
{
    if (!isConnectedRobot) { 
        QMessageBox::warning(this, "Warning", "Please Connect Robot first."); return; 
    }
    
    if (ui.calibrate_btn->text().contains("STOP", Qt::CaseInsensitive)) { return; }

    QString probeName = ui.probeTool_cbb->currentText();
    QString needleName = ui.efToolPrepare_cbb->currentText();
    
    if (!m_toolCache.contains(needleName) || !m_toolCache.contains(probeName)) {
        QMessageBox::warning(this, "Calibration Error", "Tools not found."); return;
    }

    TrackedTool tNeedle = m_toolCache[needleName];
    TrackedTool tProbe = m_toolCache[probeName];

    if (!tNeedle.isVisible || !tProbe.isVisible) {
        QMessageBox::warning(this, "Calibration Error", "Ensure both tools visible."); return;
    }

    // Start Collection
    m_bufNeedleFrames.clear(); m_bufProbeFrames.clear();
    m_calibFrameCounter = 0; m_isCollectingCalib = true;
    
    emit reqRobotPoseUpdated(); // Ensure valid base pose
    ui.URAction_btn->setEnabled(false);
    applyTheme(ui.calibrate_btn, connectingStyle, pressedGreen, true);
    printInfo("Collecting Calibration Data... Hold Still.");
}

Eigen::Matrix4d PunctureRobot::calcAverageMatrix(const QList<Eigen::Matrix4d>& matrices)
{
    if (matrices.isEmpty()) return Eigen::Matrix4d::Identity();
    Eigen::Vector3d sumPos = Eigen::Vector3d::Zero();
    Eigen::Vector4d sumQuat = Eigen::Vector4d::Zero(); 
    Eigen::Quaterniond refQ(matrices.first().block<3,3>(0,0));

    for (const auto& mat : matrices) {
        sumPos += mat.block<3,1>(0,3);
        Eigen::Quaterniond q(mat.block<3,3>(0,0));
        if (q.dot(refQ) < 0.0) q.coeffs() = -q.coeffs();
        sumQuat += q.coeffs();
    }
    
    Eigen::Matrix4d avgMat = Eigen::Matrix4d::Identity();
    avgMat.block<3,3>(0,0) = Eigen::Quaterniond(sumQuat.normalized()).toRotationMatrix();
    avgMat.block<3,1>(0,3) = sumPos / (double)matrices.size();
    return avgMat;
}

void PunctureRobot::performHandEyeCalibration(const Eigen::Matrix4d& T_World_Needle_Avg, 
                                              const Eigen::Matrix4d& T_World_Probe_Avg)
{
    // [CRITICAL FIX] Retrieve true axes based on Tool definitions, not hardcoded columns.
    QString pName = ui.probeTool_cbb->currentText();
    QString nName = ui.efToolPrepare_cbb->currentText();

    // Helper to extract axis from averaged matrix based on Tool config
    auto getAxisFromMat = [&](const Eigen::Matrix4d& mat, const QString& name) -> Eigen::Vector3d {
        // Find Tool object to get defined direction (e.g. "Z-", "X+")
        std::string dirStr = "Z+"; // Default
        std::vector<Tool*> tools; emit reqGetTools(tools);
        for(auto* t : tools) if(t && t->getName() == name.toStdString()) { dirStr = t->getDirection(); break; }

        Eigen::Matrix3d R = mat.block<3,3>(0,0);
        if (dirStr == "X+") return R.col(0);
        if (dirStr == "X-") return -R.col(0);
        if (dirStr == "Y+") return R.col(1);
        if (dirStr == "Y-") return -R.col(1);
        if (dirStr == "Z+") return R.col(2);
        if (dirStr == "Z-") return -R.col(2);
        return R.col(2); // Fallback
    }; 

    // 1. Extract physical vectors in World Frame
    // Probe is aligned with Flange X
    Eigen::Vector3d vecX_W = getAxisFromMat(T_World_Probe_Avg, pName).normalized();
    // Needle is aligned with Flange Y
    Eigen::Vector3d vecY_W = getAxisFromMat(T_World_Needle_Avg, nName).normalized();

    // 2. Gram-Schmidt Orthogonalization (X cross Y = Z)
    Eigen::Vector3d vecZ_W = vecX_W.cross(vecY_W).normalized(); 
    // Recompute Y to ensure perfect orthogonality (Y = Z cross X)
    vecY_W = vecZ_W.cross(vecX_W).normalized(); 

    // 3. Construct Rotation: Flange -> World
    Eigen::Matrix3d R_Flange_in_World;
    R_Flange_in_World.col(0) = vecX_W;
    R_Flange_in_World.col(1) = vecY_W;
    R_Flange_in_World.col(2) = vecZ_W;

    // 4. Solve Base -> World
    if (m_matCurrRobotBasePose.isIdentity()) {
        printInfo("Error: Robot Pose Invalid. Connect Robot."); return;
    }
    Eigen::Matrix3d R_Flange_in_Base = m_matCurrRobotBasePose.block<3,3>(0,0);

    // R_Base_World = R_Flange_in_Base * (R_Flange_in_World)^T
    Eigen::Matrix3d R_Base_World = R_Flange_in_Base * R_Flange_in_World.transpose();

    m_matWorldToBase = Eigen::Matrix4d::Identity();
    m_matWorldToBase.block<3,3>(0,0) = R_Base_World;

    m_isCalibrated = true;
    printInfo("Calibration Success! Matrix Updated with correct Tool Axes.");
    applyTheme(ui.calibrate_btn, textBaseStyle, pressedGreen, true);
}

// ---------------------------------------------------------
// ROBOT MOTION LOGIC (Sequence Control)
// ---------------------------------------------------------
void PunctureRobot::on_URAction_btn_clicked() {
    // 0. Safety Checks
    if (!isConnectedRobot || !m_isCalibrated) {
        QMessageBox::warning(this, "Error", "Check Robot Connection/Calibration."); return;
    }
    
    // Handle STOP action
    if (ui.URAction_btn->text().contains("STOP")) {
        emit reqStopAll();
        m_puncturePhase = PHASE_IDLE;
        ui.URAction_btn->setText("Start Robot Action");
        applyTheme(ui.URAction_btn, textBaseStyle, pressedGreen, true);
        return;
    }

    m_hasRecordedZeroPos = false;

    // [CONFIG] Needle Offset Vector (in Flange Frame)
    // Extracted from config: (0, 0.1875, 0.1475)
    Eigen::Vector3d vOffset_Flange = m_matFlangeToTCP.block<3,1>(0,3);

    // 1. Get NDI Points (mm)
    QString efToolName = ui.efTool_cbb->currentText();
    m_trajectoryPointsetName = ui.trajPointset_cbb->currentText();
    
    Eigen::Vector3d pEntryW_mm, pTargetW_mm;
    if (!getLandmarksAndTrajectory(pEntryW_mm, pTargetW_mm)) return;

    // Save final target for Needle Servoing later
    m_finalTargetW_mm = pTargetW_mm;

    if (!m_toolCache.contains(efToolName)) return;
    
    // Current Tip Position (NDI World)
    Eigen::Vector3d pCurrentTipW_mm = toolToEigen(m_toolCache[efToolName]).block<3,1>(0,3);

    // 2. Unit Conversion (mm -> m)
    Eigen::Vector3d pEntryW_m = pEntryW_mm * 0.001;
    Eigen::Vector3d pTargetW_m = pTargetW_mm * 0.001;
    Eigen::Vector3d pCurrentTipW_m = pCurrentTipW_mm * 0.001;

    // 3. Calculate Key Points (World, Meters)
    Eigen::Vector3d vecPath = (pTargetW_m - pEntryW_m).normalized();
    
    // Point A: Approach (100mm back)
    Eigen::Vector3d pApproachW_m = pEntryW_m - vecPath * PARAM_APPROACH_DISTANCE;
    
    // Point B: Ready/Safety Point (5mm back from Entry)
    // The robot will stop here to avoid squeezing the patient.
    Eigen::Vector3d pReadyW_m = pEntryW_m - vecPath * SAFETY_MARGIN_M;

    // 4. Calculate Orientation (Base Frame)
    if (m_matCurrRobotBasePose.isIdentity()) {
        printInfo("Error: Robot Pose Invalid."); return;
    }
    Eigen::Matrix3d R_BW = m_matWorldToBase.block<3,3>(0,0);
    Eigen::Matrix3d R_CurrentBase = m_matCurrRobotBasePose.block<3,3>(0,0);

    // Calc rotation to align Needle with Path
    // Using Tool definition to get the correct axis (Z-, X+, etc)
    Eigen::Vector3d currNeedleDir_W = getToolCurrentAxis(efToolName);
    Eigen::Quaterniond qDelta = Eigen::Quaterniond::FromTwoVectors(currNeedleDir_W, vecPath);
    
    // Apply Delta: R_Target = R_BW * qDelta * R_WB * R_Current
    Eigen::Matrix3d R_TargetBase = R_BW * qDelta.toRotationMatrix() * R_BW.transpose() * R_CurrentBase;
    
    Eigen::AngleAxisd aa(R_TargetBase);
    m_targetRotVec = aa.axis() * aa.angle();

    // 5. Calculate Target Flange Position (Compensating 3D Lever Arm)
    // ------------------------------------------------------------------
    // Formula: Flange_Target = Tip_Target - (R_Target * vOffset_Flange)
    
    // Step A: Current Tip in Base Frame (Virtual calculation)
    Eigen::Vector3d pFlangeCurr_Base = m_matCurrRobotBasePose.block<3,1>(0,3);
    Eigen::Vector3d pTipCurr_Base = pFlangeCurr_Base + R_CurrentBase * vOffset_Flange;

    // Step B: Target Tip Positions in Base Frame
    // Tip_Target = Tip_Current + R_BW * (Target_W - Current_W)
    Eigen::Vector3d pTipApproach_Base = pTipCurr_Base + R_BW * (pApproachW_m - pCurrentTipW_m);
    Eigen::Vector3d pTipReady_Base    = pTipCurr_Base + R_BW * (pReadyW_m - pCurrentTipW_m);

    // Step C: Back-calculate Flange Positions
    // Must use R_TargetBase because the robot rotates!
    Eigen::Vector3d vOffset_Rotated_Target = R_TargetBase * vOffset_Flange;

    m_targetApproach = pTipApproach_Base - vOffset_Rotated_Target;
    m_targetEntry    = pTipReady_Base    - vOffset_Rotated_Target; // Stop at Ready Point (5mm out)

    // Pre-Check Distance
    Eigen::Vector3d pCurrentBase_m = m_matCurrRobotBasePose.block<3,1>(0,3);
    double distM = (m_targetApproach - pCurrentBase_m).norm();
    
    printInfo(QString("<b>[Pre-Check] Planned Dist: %1 mm</b>")
              .arg(distM * 1000.0, 0, 'f', 2));
    
    qDebug() << "--------------------------------------------------";
    qDebug() << "Pre-Move Check:";
    qDebug() << "Current Base: " << pCurrentBase_m.x() << pCurrentBase_m.y() << pCurrentBase_m.z();
    qDebug() << "Target Base : " << m_targetApproach.x() << m_targetApproach.y() << m_targetApproach.z();
    qDebug() << "Distance    : " << distM * 1000.0 << " mm";
    qDebug() << "--------------------------------------------------";

    // 6. Execute Sequence
    emit reqStartDynamicPuncture();
    m_puncturePhase = PHASE_MOVING_TO_APPROACH;
    printInfo("Moving to APPROACH Point...");

    // Record trajectory for viz
    emit reqAdd3DTrajectory(pEntryW_mm.data(), pTargetW_mm.data(), m_trajectoryPointsetName);
    // Eigen::Vector3d pApproachW_mm = pApproachW_m * 1000.0;
    // Eigen::Vector3d pReadyW_mm    = pReadyW_m * 1000.0;
    // emit reqAdd3DTrajectory(pApproachW_mm.data(), pReadyW_mm.data(), "test_traj");

    // Send 1st Command (Approach)
    emit reqUpdateTargetPose(m_targetApproach.x(), m_targetApproach.y(), m_targetApproach.z(),
                             m_targetRotVec.x(), m_targetRotVec.y(), m_targetRotVec.z(), 0.0);

    ui.URAction_btn->setText("STOP Robot Action");
    applyTheme(ui.URAction_btn, activeStyle, pressedGreen, true);
}


// ---------------------------------------------------------
// ROBOT FEEDBACK LOOP (State Monitor)
// ---------------------------------------------------------
void PunctureRobot::onRobotPoseUpdated(double x, double y, double z, double rx, double ry, double rz)
{
    // Cache current pose
    Eigen::Vector3d t(x, y, z);
    Eigen::Vector3d rot(rx, ry, rz);
    double angle = rot.norm();
    Eigen::Matrix3d R;
    if (angle < 1e-6) R = Eigen::Matrix3d::Identity();
    else R = Eigen::AngleAxisd(angle, rot.normalized()).toRotationMatrix();
    
    m_matCurrRobotBasePose = Eigen::Matrix4d::Identity();
    m_matCurrRobotBasePose.block<3,3>(0,0) = R;
    m_matCurrRobotBasePose.block<3,1>(0,3) = t;

    // Sequence Logic
    if (m_puncturePhase == PHASE_MOVING_TO_APPROACH) {
        double dist = (t - m_targetApproach).norm();
        if (dist < PARAM_MOTION_TOLERANCE) {
            printInfo("Reached APPROACH. Moving to ENTRY...");
            m_puncturePhase = PHASE_MOVING_TO_ENTRY;
            // Send 2nd COMMAND (One-shot)
            emit reqUpdateTargetPose(m_targetEntry.x(), m_targetEntry.y(), m_targetEntry.z(),
                                     m_targetRotVec.x(), m_targetRotVec.y(), m_targetRotVec.z(), 0.0);
        }
    }
    else if (m_puncturePhase == PHASE_MOVING_TO_ENTRY) {
        double dist = (t - m_targetEntry).norm();
        if (dist < PARAM_MOTION_TOLERANCE) {
            printInfo("Reached ENTRY Point. Ready.");
            m_puncturePhase = PHASE_READY; // [C2065 Fix] Now recognized correctly.
        }
    }
    else if (m_puncturePhase == PHASE_RETURNING_ROBOT) {
        double dist = (t - m_targetApproach).norm();
        if (dist < PARAM_MOTION_TOLERANCE) {
            printInfo("<b><font color='green'>[Auto Return] Complete! System completely reset and ready.</font></b>");
            m_puncturePhase = PHASE_IDLE; // End of the full workflow.
        }
    }
}

// ---------------------------------------------------------
// NEEDLE CONTROL
// ---------------------------------------------------------
void PunctureRobot::on_EFAction_btn_clicked() {
    if (!isConnectedEndEffector) return;
    if (m_needleState != NeedleState::IDLE) { // Stop
        m_needleState = NeedleState::IDLE;
        emit reqUpdateTargetPose(0,0,0,0,0,0, 0.0); // Stop stepper by vel=0
        ui.EFAction_btn->setText("Start Needle");
        return;
    }

    if (!m_hasRecordedZeroPos) {
        QString efToolName = ui.efTool_cbb->currentText();
        if (m_toolCache.contains(efToolName) && m_toolCache[efToolName].isVisible) {
            m_needleZeroPosW_mm = toolToEigen(m_toolCache[efToolName]).block<3,1>(0,3);
            
            m_needleZeroDirW = getToolCurrentAxis(efToolName);
            
            m_hasRecordedZeroPos = true;
            printInfo("<b>[System] Rail Physical Zero Position Recorded.</b>");
        } else {
            printInfo("Error: Cannot start. Needle tracking lost!");
            return; 
        }
    }

    m_needleState = NeedleState::INSERTING;
    m_lastDistToTarget = 99999.0;
    ui.EFAction_btn->setText("STOP Needle");
    
    // Send Velocity Command
    emit reqUpdateTargetPose(m_targetEntry.x(), m_targetEntry.y(), m_targetEntry.z(),
                             m_targetRotVec.x(), m_targetRotVec.y(), m_targetRotVec.z(), PARAM_NEEDLE_VEL_INSERT);
}

void PunctureRobot::on_ReturnAction_btn_clicked() {
    if (!isConnectedRobot || !isConnectedEndEffector) {
        printInfo("Error: Robot or EndEffector not connected."); return;
    }
    if (!m_hasRecordedZeroPos) {
        printInfo("Error: Needle zero position not recorded. Cannot auto-return."); return;
    }

    // 1. Stop other ongoing actions.
    if (m_needleState != NeedleState::IDLE) {
        m_needleState = NeedleState::IDLE;
        ui.EFAction_btn->setText("Start Needle");
    }

    // 2. Switch state to the "retracting needle" phase.
    printInfo("<b>[Auto Return] Step 1: Retracting Needle...</b>");
    m_puncturePhase = PHASE_RETURNING_NEEDLE; // Mark the system as entering the auto-return flow.
    m_needleState = NeedleState::RETRACTING;
    m_lastDistToTarget = 99999.0;
    
    // 3. Send the retract command (robot stays at Entry; feed axis reverses).
    emit reqUpdateTargetPose(m_targetEntry.x(), m_targetEntry.y(), m_targetEntry.z(),
                             m_targetRotVec.x(), m_targetRotVec.y(), m_targetRotVec.z(), 
                             -PARAM_NEEDLE_VEL_RETRACT);
}

// ---------------------------------------------------------
// DATA UPDATE
// ---------------------------------------------------------
void PunctureRobot::onToolsUpdated(const QVector<TrackedTool>& tools) {
    // 1. Update the tool cache.
    for (const auto& tool : tools) m_toolCache[tool.name] = tool;

    // 2. Update UI combo-box colors (based on visibility).
    auto updateCbbColor = [&](QComboBox* cbb, const TrackedTool& data) {
        if (cbb->currentText() == data.name) {
            QString colorStr = data.isVisible ? "rgb(0, 255, 0)" : "rgb(255, 0, 0)";
            QString weightStr = data.isVisible ? "bold" : "normal";
            QString newStyle = darkComboBoxStyle + QString("QComboBox { color: %1; font-weight: %2; }").arg(colorStr, weightStr);
            if (cbb->styleSheet() != newStyle) cbb->setStyleSheet(newStyle);
        }
    };

    for (const auto& data : tools) {
        updateCbbColor(ui.probeTool_cbb, data);
        updateCbbColor(ui.efToolPrepare_cbb, data);
        updateCbbColor(ui.baseTool_cbb, data);
        updateCbbColor(ui.patientTool_cbb, data);
        updateCbbColor(ui.efTool_cbb, data);
    }

    // =========================================================
    // State machine: closed-loop monitoring and virtual limits for insertion/retraction.
    // =========================================================
    if (m_needleState == NeedleState::INSERTING || m_needleState == NeedleState::RETRACTING) {
        
        QString efToolName = ui.efTool_cbb->currentText();
        bool isNeedleVisible = m_toolCache.contains(efToolName) && m_toolCache[efToolName].isVisible;

        if (isNeedleVisible) {
            // Get the current needle tip position.
            Eigen::Vector3d currentTipW_mm = toolToEigen(m_toolCache[efToolName]).block<3,1>(0,3);

            // ---------------------------------------------------------
            // Global safety wall: virtual limit switch.
            // ---------------------------------------------------------
            if (m_hasRecordedZeroPos) {
                // Compute the current physical extension of the lead screw.
                double currentRailTravel = (currentTipW_mm - m_needleZeroPosW_mm).norm();
                
                // 1. Forward limit: max 13.8 cm (138.0 mm).
                if (m_needleState == NeedleState::INSERTING && currentRailTravel >= 138.0) {
                    printInfo(QString("<b><font color='red'>HARDWARE LIMIT REACHED (138mm)! Forced Stop.</font></b>"));
                    m_needleState = NeedleState::IDLE;
                    ui.EFAction_btn->setText("Start Needle");
                    emit reqUpdateTargetPose(0,0,0,0,0,0, 0.0); // Emergency stop.
                    return; // Block all subsequent motion commands.
                }
                
                // 2. Rear limit: prevent base collision during retraction (tolerance 1.5 mm).
                if (m_needleState == NeedleState::RETRACTING && currentRailTravel <= 1.5) {
                    printInfo("<b><font color='green'>Needle Fully Retracted. Auto Stop.</font></b>");
                    m_needleState = NeedleState::IDLE;
                    if (m_puncturePhase == PHASE_RETURNING_NEEDLE) {
                        printInfo("<b>[Auto Return] Step 2: Needle safe. UR Robot returning to Approach Point...</b>");
                        m_puncturePhase = PHASE_RETURNING_ROBOT; // Switch to the robot retreat phase.
                        
                        // Send command: robot returns to the Approach point, needle speed set to 0.
                        emit reqUpdateTargetPose(m_targetApproach.x(), m_targetApproach.y(), m_targetApproach.z(),
                                                 m_targetRotVec.x(), m_targetRotVec.y(), m_targetRotVec.z(), 0.0);
                    } else {
                        // For a manual retraction, just send zero speed to stop.
                        emit reqUpdateTargetPose(0,0,0,0,0,0, 0.0);
                    }
                    return; 
                }
            }
            // ---------------------------------------------------------
            // Standard insertion closed-loop control (use 1D projected distance to prevent overshoot).
            if (m_needleState == NeedleState::INSERTING) {
                // 1. Compute the vector from the current tip to the target point.
                Eigen::Vector3d vecToTarget = m_finalTargetW_mm - currentTipW_mm;
                
                // 2. [Core fix] Compute the remaining projected distance along the insertion direction (1D depth).
                // m_needleZeroDirW is the axial direction vector for insertion.
                // This asks: along the needle direction, how far am I from the target plane?
                double distToGo1D = vecToTarget.dot(m_needleZeroDirW);

                // 3. Decision logic.
                double cmdSpeed = 0.0;
                bool shouldStop = false;

                // [Hard stop wall] If remaining projected distance <= 0, the tip has passed the target depth.
                // Regardless of lateral error, stop once depth is reached.
                if (distToGo1D <= 0.0) {
                    printInfo(QString("Passed Target Plane! Overshoot: %1 mm").arg(-distToGo1D));
                    shouldStop = true;
                }

                // Segmented deceleration logic.
                if (!shouldStop) {
                    if (distToGo1D < 0.5) { 
                        // Tighten the threshold to 0.5 mm since 1D distance is unaffected by lateral error.
                        shouldStop = true; 
                    } 
                    else if (distToGo1D < 15.0) {
                        // The closer the distance, the slower the speed; allow a minimum of 15% to counter inertia.
                        double factor = (distToGo1D - 0.5) / 14.5; 
                        factor = std::max(0.15, factor); 
                        cmdSpeed = PARAM_NEEDLE_VEL_INSERT * factor; 
                    } 
                    else {
                        cmdSpeed = PARAM_NEEDLE_VEL_INSERT; // Full speed.
                    }
                }

                // 4. Execute commands.
                if (shouldStop) {
                    // Compute the true 3D final error and print it for reference.
                    double final3DError = (m_finalTargetW_mm - currentTipW_mm).norm();
                    printInfo(QString("<b>Target Reached! 3D Error: %1 mm</b>").arg(final3DError));
                    
                    m_needleState = NeedleState::IDLE;
                    ui.EFAction_btn->setText("Start Needle");
                    
                    // [Emergency stop] Send zero speed.
                    emit reqUpdateTargetPose(0,0,0,0,0,0, 0.0); 
                }
                else {
                    emit reqUpdateTargetPose(m_targetEntry.x(), m_targetEntry.y(), m_targetEntry.z(),
                                             m_targetRotVec.x(), m_targetRotVec.y(), m_targetRotVec.z(), 
                                             cmdSpeed); 
                }
            }
        }
        else {
            // Safety protection: tracking lost.
            printInfo("Safety Stop: Needle Tracking Lost!");
            m_needleState = NeedleState::IDLE;
            ui.EFAction_btn->setText("Start Needle"); // Restore button text.
            emit reqUpdateTargetPose(0,0,0,0,0,0, 0.0);
        }
    }

    // =========================================================
    // Calibration matrix data collection.
    // =========================================================
    if (m_isCollectingCalib) {
        QString pName = ui.probeTool_cbb->currentText();
        QString nName = ui.efToolPrepare_cbb->currentText();
        if (m_toolCache.contains(pName) && m_toolCache[pName].isVisible &&
            m_toolCache.contains(nName) && m_toolCache[nName].isVisible) {
            
            m_bufProbeFrames.append(toolToEigen(m_toolCache[pName]));
            m_bufNeedleFrames.append(toolToEigen(m_toolCache[nName]));
            m_calibFrameCounter++;
            
            if (m_calibFrameCounter >= PARAM_CALIB_FRAMES) {
                m_isCollectingCalib = false;
                Eigen::Matrix4d avgN = calcAverageMatrix(m_bufNeedleFrames);
                Eigen::Matrix4d avgP = calcAverageMatrix(m_bufProbeFrames);
                performHandEyeCalibration(avgN, avgP);
                ui.URAction_btn->setEnabled(true);
                ui.calibrate_btn->setEnabled(true);
            }
        }
    }
}

// Helpers Implementation
const TrackedTool* PunctureRobot::findToolByName(const QVector<TrackedTool>& tools, const QString& name) {
    for (const auto& tool : tools) if (tool.name == name) return &tool;
    return nullptr;
}

// [C2039 Fix] Declarations added in .h; implementation here.
bool PunctureRobot::getLogicTools(TrackedTool& tPat, TrackedTool& tRob) { return false; }
bool PunctureRobot::syncRawToLogicTools(const QVector<TrackedTool>& rawTools, TrackedTool& logicPat, TrackedTool& logicRob) { return false; }

bool PunctureRobot::getLandmarksAndTrajectory(Eigen::Vector3d& pEntryW, Eigen::Vector3d& pTargetW) {
    std::vector<Landmark*> l; 
    emit reqGetLandmarks(l, m_trajectoryPointsetName);
    if (l.size() < 2) return false;
    
    // [C2440 Fix] Add the const keyword.
    const double* e = l[0]->getCoordinates(); 
    const double* t = l[1]->getCoordinates();
    
    pEntryW << e[0], e[1], e[2]; pTargetW << t[0], t[1], t[2];
    return true;
}

Eigen::Matrix4d PunctureRobot::toolToEigen(const TrackedTool& t) {
    std::vector<Tool*> tools; emit reqGetTools(tools);
    for(auto* tool : tools) if(tool->getName() == t.name.toStdString()) {
        vtkSmartPointer<vtkMatrix4x4> v = tool->getFinalMatrix();
        Eigen::Matrix4d m; for(int r=0;r<4;++r) for(int c=0;c<4;++c) m(r,c)=v->GetElement(r,c);
        return m;
    }
    return Eigen::Matrix4d::Identity();
}

// [C2039 Fix] These functions are unused but kept to avoid connection errors in untouched code.
void PunctureRobot::solveAndEmitKinematics(const Eigen::Matrix4d&, const Eigen::Vector3d&, const Eigen::Vector3d&, const Eigen::Vector3d&) {}
void PunctureRobot::calculatePatientLocalLandmarks(const Eigen::Matrix4d&, const Eigen::Vector3d&, const Eigen::Vector3d&) {}
void PunctureRobot::generateTwoPhasePath(const Eigen::Vector3d&, const Eigen::Vector3d&, const Eigen::Vector3d&, const Eigen::Matrix4d&) {}
void PunctureRobot::triggerEmergencyReplan(const Eigen::Vector3d&, const Eigen::Matrix4d&) {}

void PunctureRobot::onJogBtnPressed() {}
void PunctureRobot::onJogBtnReleased() { emit reqStopJog(); }
void PunctureRobot::on_btnFreeDrive_toggled(bool c) { emit reqSetFreeDrive(c); }
void PunctureRobot::onWorkerLog(QString m) { printInfo(m); }
void PunctureRobot::onRobotStatusChanged(bool c) { 
    ui.connectRobot_btn->setText(c ? "Disconnect from UR" : "Connect to UR"); isConnectedRobot = c; 
    applyTheme(ui.connectRobot_btn, c ? activeStyle : textBaseStyle, pressedGreen, true);
}
void PunctureRobot::onEndEffectorStatusChanged(bool c) { 
    ui.connectEndeffector_btn->setText(c ? "Disconnect from Endeffector" : "Connect to Endeffector"); isConnectedEndEffector = c; 
    applyTheme(ui.connectEndeffector_btn, c ? activeStyle : textBaseStyle, pressedGreen, true);
}
void PunctureRobot::onPunctureFinished(bool s) { /* ... */ }
void PunctureRobot::on_connectRobot_btn_clicked() { 
    if (ui.connectRobot_btn->text().contains("Disconnect")) { emit reqDisconnectAll(); return; }
    emit reqConnectRobot(m_config.contains("Device_IP") ? m_config["Device_IP"].toString() : "192.168.1.15");
    ui.connectRobot_btn->setText("Connecting"); ui.connectRobot_btn->setStyleSheet(connectingStyle);
}
void PunctureRobot::on_connectEndeffector_btn_clicked() {
    if (ui.connectEndeffector_btn->text().contains("Disconnect")) { emit reqDisconnectAll(); return; }
    emit reqConnectEndEffector(m_config.contains("EndEffectorSerial") ? m_config["EndEffectorSerial"].toString() : "590422944");
}

Eigen::Vector3d PunctureRobot::getToolCurrentAxis(const QString& toolName)
{
    // 1. Get the Tool pointer (needs Tool::m_direction and Tool::m_finalMatrix).
    std::vector<Tool*> tools;
    emit reqGetTools(tools);
    
    Tool* pTool = nullptr;
    for (auto* t : tools) {
        if (t && t->getName() == toolName.toStdString()) {
            pTool = t;
            break;
        }
    }

    if (!pTool) {
        printInfo("Error: Tool object not found!");
        return Eigen::Vector3d::UnitZ(); // Default to Z to avoid a crash.
    }

    // 2. Get the current rotation matrix (world frame).
    // Note: use the tool FinalMatrix directly; it is more current and accurate than toolCache.
    vtkSmartPointer<vtkMatrix4x4> vtkMat = pTool->getFinalMatrix();
    Eigen::Matrix3d rotMat;
    for(int r=0; r<3; r++) for(int c=0; c<3; c++) rotMat(r,c) = vtkMat->GetElement(r,c);

    // 3. Resolve the local vector from m_direction.
    std::string dirStr = pTool->getDirection(); // e.g. "Z-", "X+"
    Eigen::Vector3d localAxis;

    if      (dirStr == "X+") localAxis = Eigen::Vector3d::UnitX();
    else if (dirStr == "X-") localAxis = -Eigen::Vector3d::UnitX();
    else if (dirStr == "Y+") localAxis = Eigen::Vector3d::UnitY();
    else if (dirStr == "Y-") localAxis = -Eigen::Vector3d::UnitY();
    else if (dirStr == "Z+") localAxis = Eigen::Vector3d::UnitZ();
    else if (dirStr == "Z-") localAxis = -Eigen::Vector3d::UnitZ();
    else {
        // Default case (your Tool code defaults to Z+).
        localAxis = Eigen::Vector3d::UnitZ(); 
    }

    // 4. Transform to the world coordinate system.
    // World_Vector = Rotation_Matrix * Local_Vector
    return (rotMat * localAxis).normalized();
}
