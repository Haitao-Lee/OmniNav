#pragma once

#include "Modules/BaseModule/BaseModule.h"
#include "ui_PunctureRobot.h"
#include <QThread>
#include <QJsonObject>
#include <QSplitter>
#include <QTimer>
#include <QMap>
#include <Eigen/Dense>
#include "OpticalNavigation.h"

class RobotWorker;
class Tool;     
class Landmark; 

// Sub-state for Needle Control
enum class NeedleState {
    IDLE,
    INSERTING,  
    RETRACTING  
};

// Main State Machine
enum PuncturePhaseEnum { 
    PHASE_IDLE = 0,
    PHASE_MOVING_TO_APPROACH = 1, 
    PHASE_MOVING_TO_ENTRY = 2,    
    PHASE_READY = 3,              // [修复 C2065] 补上了这个状态
    PHASE_PUNCTURING = 4,         
    PHASE_RETRACTING = 5          
};

class PunctureRobot : public BaseModule
{
    Q_OBJECT

public:
    explicit PunctureRobot(QWidget* parent = nullptr);
    ~PunctureRobot() override;

    Ui::PunctureRobotClass& getUi() { return ui; }
    
    void init() override;
    void initSplitters() override;
    void initButton() override;
    void createActions() override;

    void printInfo(const QString& message);

    QWidget* getCentralwidget() override { return ui.centralwidget; };
    QJsonObject getConfig() const { return m_config; }
    void setConfig(const QJsonObject& config) { m_config = config; }

signals:
    // Hardware
    void reqConnectRobot(QString ip);
    void reqConnectEndEffector(QString port);
    void reqDisconnectAll();
    
    // Control
    void reqStartDynamicPuncture();
    void reqStopAll();

    // Trigger Robot Move (One-shot)
    void reqUpdateTargetPose(double tx, double ty, double tz, 
                             double rx, double ry, double rz, 
                             double needleVel);

    // Manual / Data
    void reqSetFreeDrive(bool enable);
    void reqStartJog(int axis, int direction); 
    void reqStopJog();
    void reqManualStepper(double velocity); 
    void reqGetLandmarks(std::vector<Landmark*>& landmarks, QString pointSetName);
    void reqAddLandmark(QString name, double x, double y, double z);
    void reqGetTools(std::vector<Tool*>& tools);
    void reqAdd3DTrajectory(const double entry[3], const double target[3], QString pointSetName);
    void reqRobotPoseUpdated();

public slots:
    void on_connectRobot_btn_clicked();
    void on_connectEndeffector_btn_clicked();
    void on_URAction_btn_clicked(); 
    void on_calibrate_btn_clicked(); 
    
    void on_EFAction_btn_clicked();      
    void on_RetractAction_btn_clicked(); 

    void on_btnFreeDrive_toggled(bool checked);
    void onJogBtnPressed();
    void onJogBtnReleased(); 

    void onToolsUpdated(const QVector<TrackedTool>& tools);

    void onWorkerLog(QString msg);
    void onRobotStatusChanged(bool connected);
    void onEndEffectorStatusChanged(bool connected);
    void onRobotPoseUpdated(double x, double y, double z, double rx, double ry, double rz);
    void onPunctureFinished(bool success);

private:
    Ui::PunctureRobotClass ui;
    QJsonObject m_config;
    QThread* m_workerThread = nullptr;
    RobotWorker* m_worker = nullptr;
    QSplitter* m_vsplitter = nullptr;
    double m_lastDistToTarget = 99999.0;
    
    // Calibration
    Eigen::Matrix4d m_matFlangeToTCP; 
    Eigen::Matrix4d m_matWorldToBase = Eigen::Matrix4d::Identity();
    bool m_isCalibrated = false;

    // Calibration Collection
    bool m_isCollectingCalib = false;
    int m_calibFrameCounter = 0;
    const int PARAM_CALIB_FRAMES = 50; 
    QList<Eigen::Matrix4d> m_bufNeedleFrames;
    QList<Eigen::Matrix4d> m_bufProbeFrames;

    // State
    QMap<QString, TrackedTool> m_toolCache;
    bool isConnectedRobot = false;
    bool isConnectedEndEffector = false;
    
    int m_puncturePhase = 0; 
    NeedleState m_needleState = NeedleState::IDLE;

    // Computed Targets (Base Frame)
    Eigen::Vector3d m_targetApproach; 
    Eigen::Vector3d m_targetEntry;    
    Eigen::Vector3d m_targetRotVec;   

    // Config Names
    QString m_baseToolName;
    QString m_patientToolName;
    QString m_efToolName;
    QString m_trajectoryPointsetName;

    Eigen::Vector3d m_vecLocalEntry;
    Eigen::Vector3d m_vecLocalTarget;
    Eigen::Vector3d m_finalTargetW_mm;
    Eigen::Matrix4d m_matCurrRobotBasePose = Eigen::Matrix4d::Identity();
    
    QList<Eigen::Vector3d> m_plannedPathLocal; 

    // Constants
    const double PARAM_APPROACH_DISTANCE = 0.300; // 100mm = 0.1m
    const double PARAM_MOTION_TOLERANCE = 0.002;  // 2mm tolerance
    const double PARAM_NEEDLE_VEL_INSERT = 15.0;   
    const double PARAM_NEEDLE_VEL_RETRACT = 5.0;

    // Helpers
    void loadConfig();
    void initWorker();
    void applyTheme(QPushButton* btn, const QString& baseStyle, const QString& pressedStyle, bool extraBorders = false);

    const TrackedTool* findToolByName(const QVector<TrackedTool>& tools, const QString& name);
    bool getLandmarksAndTrajectory(Eigen::Vector3d& pEntryW, Eigen::Vector3d& pTargetW);
    Eigen::Matrix4d toolToEigen(const TrackedTool& tool); 
    Eigen::Matrix4d calcAverageMatrix(const QList<Eigen::Matrix4d>& matrices);
    void performHandEyeCalibration(const Eigen::Matrix4d& T_World_Needle_Avg, 
                                   const Eigen::Matrix4d& T_World_Probe_Avg);

    // [修复 C2039] 必须声明这些函数，因为 .cpp 文件里有它们的空实现
    // 虽然逻辑上不怎么用了，但为了编译通过必须保留声明
    bool getLogicTools(TrackedTool& tPat, TrackedTool& tRob);
    bool syncRawToLogicTools(const QVector<TrackedTool>& rawTools, TrackedTool& logicPat, TrackedTool& logicRob);
    void solveAndEmitKinematics(const Eigen::Matrix4d& T_W_Rob_M, const Eigen::Vector3d& targetPosW_M, const Eigen::Vector3d& pEntryW_M, const Eigen::Vector3d& pTargetW_M);
    void calculatePatientLocalLandmarks(const Eigen::Matrix4d& T_W_Pat, const Eigen::Vector3d& pEntryW, const Eigen::Vector3d& pTargetW);
    void generateTwoPhasePath(const Eigen::Vector3d& pRobM, const Eigen::Vector3d& pEntryM, const Eigen::Vector3d& pTargetM, const Eigen::Matrix4d& T_W_Pat);
    void triggerEmergencyReplan(const Eigen::Vector3d& pRobM, const Eigen::Matrix4d& T_W_Pat);
    Eigen::Vector3d getToolCurrentAxis(const QString& toolName);
};