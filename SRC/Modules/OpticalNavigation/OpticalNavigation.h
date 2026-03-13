#pragma once

#include "BaseModule.h"
#include "ui_OpticalNavigation.h"
#include "NDICombinedWorker.h"
#include "Tool.h"
#include "Landmark.h"
#include <QThread>
#include <QElapsedTimer>
#include <vector>
#include <QJsonObject>
#include <QSplitter>
#include <QVector>
#include <Eigen/Dense>
#include <Eigen/Geometry>

class OpticalNavigation : public BaseModule
{
    Q_OBJECT

public:
    explicit OpticalNavigation(QWidget* parent = nullptr);
    ~OpticalNavigation() override;

    Ui::OpticalNavigationClass& getUi() { return ui; }
    
    void init() override;
    void initSplitters() override;
    void initButton() override;
    void createActions() override;
    
    NDICombinedWorker* getNDIWorker() const { return m_ndiWorker; }
    void setNDIWorker(NDICombinedWorker* worker) { m_ndiWorker = worker; }

    QWidget* getCentralwidget() override { return ui.centralwidget; };

    QJsonObject getConfig() const { return m_config; }
    void setConfig(const QJsonObject& config) { m_config = config; }

    QSplitter* getVsplitter() const { return m_vsplitter; }
    void setVsplitter(QSplitter* splitter) { m_vsplitter = splitter; }

    QThread* getWorkerThread() const { return m_workerThread; }
    void setWorkerThread(QThread* thread) { m_workerThread = thread; }

    bool getIsSampling() const { return m_isSampling; }
    void setIsSampling(bool isSampling) { m_isSampling = isSampling; }

    QString getProbeName() const { return m_ProbeName; }
    void setProbeName(const QString& name) { m_ProbeName = name; }

    QString getInputPointSetName() const { return m_inputPointSetName; }
    void setInputPointSetName(const QString& name) { m_inputPointSetName = name; }

    QString getOutputPointSetName() const { return m_outputPointSetName; }
    void setOutputPointSetName(const QString& name) { m_outputPointSetName = name; }

    int getTargetPointCount() const { return m_targetPointCount; }
    void setTargetPointCount(int count) { m_targetPointCount = count; }

    const std::vector<Eigen::Vector3d>& getStabilityBuffer() const { return m_stabilityBuffer; }
    void setStabilityBuffer(const std::vector<Eigen::Vector3d>& buffer) { m_stabilityBuffer = buffer; }

    const QElapsedTimer& getStabilityTimer() const { return m_stabilityTimer; }
    void setStabilityTimer(const QElapsedTimer& timer) { m_stabilityTimer = timer; }

    bool getWaitingForExit() const { return m_waitingForExit; }
    void setWaitingForExit(bool waiting) { m_waitingForExit = waiting; }

    Eigen::Vector3d getLastCapturedPoint() const { return m_lastCapturedPoint; }
    void setLastCapturedPoint(const Eigen::Vector3d& point) { m_lastCapturedPoint = point; }

    double getTolerance() const { return m_tolerance; }
    void setTolerance(double tolerance) { m_tolerance = tolerance; }

    int getStableTime() const { return m_stable_time; }
    void setStableTime(int time) { m_stable_time = time; }

signals:
    void reqConnectDevice(QString ip);
    void reqDisconnectDevice();
    void reqAddTools(const QMap<QString, QString>& toolsConfig);
    void reqStartCollecting();
    void reqStopTracking();
    void reqGetTools(std::vector<Tool*>& tools);
    void reqGetLandmarks(std::vector<Landmark*>& landmarks, QString pointSetName);
    void reqLandmarksColorUpdate();
    void handleUpdated(TrackedTool data);
    void reqAddLandmark(QString name, double x, double y, double z);
    void reqUpdateTransform(QString name, Eigen::Matrix4d matrix);
    void setToolRegisTransform(QString name, Eigen::Matrix4d matrix);
    void setToolCalibrationTransform(QString name, Eigen::Matrix4d matrix);
    void signalStopTracking();
    void signalSamplingStatus(QString statusText);
    void signalAllToolsUpdated(const QVector<TrackedTool>& tools);

public slots:
    void onConnectBtnClicked();
    void onTrackingBtnClicked();
    void onSamplingPointBtnClicked();
    void onRegistration();
    void onCalibration();

    void onToolDataReceived(const QVector<TrackedTool>& tools);
    void onStatusReceived(QString msg);
    void onErrorReceived(QString err);
    
    void onRomChanged(const QString& text);
    void onToolChanged(const QString& text);
    void onToolsAdded(QMap<int, QString> tools);

private:
    Ui::OpticalNavigationClass ui;
    QJsonObject m_config;
    QSplitter* m_vsplitter = nullptr;

    NDICombinedWorker* m_ndiWorker = nullptr;
    QThread* m_workerThread = nullptr;

    void loadConfig();
    void initWorker();
    void applyTheme(QPushButton* btn, const QString& baseStyle, const QString& pressedStyle, bool extraBorders = false);

    // --- Sampling State ---
    bool m_isSampling = false;
    double m_tolerance = 1.0; 
    int m_stable_time = 1500;
    QString m_ProbeName;
    QString m_inputPointSetName;
    QString m_outputPointSetName;
    int m_targetPointCount = 0;

    std::vector<Eigen::Vector3d> m_stabilityBuffer;
    QElapsedTimer m_stabilityTimer;
    bool m_waitingForExit = false;
    Eigen::Vector3d m_lastCapturedPoint;

    // --- Calibration State ---
    bool m_isCalibrating = false;            
    int m_caliSampleCount = 0;       
    bool m_isPivotCalibration = true;        
    const int m_caliTargetSamples = 300;      
    std::vector<Eigen::Matrix4d> m_caliBufferProbe;
    std::vector<Eigen::Matrix4d> m_caliBufferTool;

    void finishCalibrationCalculation();
    void performTipToTipCalibration();
    void performProbePivotCalibration();
    void stopSamplingLogic();
    void addPointToDataManager(const Eigen::Vector3d& point);
    Eigen::Matrix4d calculateRegistrationFromLandmarks(
        const std::vector<Landmark*>& sourceList, 
        const std::vector<Landmark*>& targetList
    );
    
    Eigen::Matrix4d computeAverageMatrix(const std::vector<Eigen::Matrix4d>& matrices);
    void updateToolMatrices(const QVector<TrackedTool>& tools, const std::vector<Tool*>& allTools);
    void handleCalibrationLogic(const QVector<TrackedTool>& tools, const std::vector<Tool*>& allTools);
    void handleSamplingLogic(const QVector<TrackedTool>& tools, const std::vector<Tool*>& allTools);
    void processSamplingData(const TrackedTool& data, Tool* probeTool);
};