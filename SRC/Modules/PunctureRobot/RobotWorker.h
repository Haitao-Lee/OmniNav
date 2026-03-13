#pragma once

#include <QObject>
#include <QTimer>
#include <QMutex>
#include <atomic>
#include "URController.h" 

extern "C" {
    #include "usb_device.h"
    #include "io.h"
    #include "pwm.h"
}

class RobotWorker : public QObject
{
    Q_OBJECT

public:
    explicit RobotWorker(QObject *parent = nullptr);
    ~RobotWorker();

public slots:
    void init(); 
    void disconnectAll();
    void connectRobot(const QString& ip);
    void connectStepper(const QString& portName);
    void updateRobotPose();

    void startDynamicPuncture();
    void stopAll(); 

    // Receive Target from PunctureRobot (Thread Safe)
    void updateDynamicTarget(double tx, double ty, double tz, 
                             double rx, double ry, double rz, 
                             double needleVel); 

    void setFreeDriveMode(bool enable);
    void startJog(int axis, int direction);
    void stopJog();
    void manualControlStepper(double velocity_mm_s);

signals:
    void logMessage(QString msg);
    void robotConnStatus(bool connected);
    void stepperConnStatus(bool connected);
    void robotPoseUpdated(double x, double y, double z, double rx, double ry, double rz);
    void punctureFinished(bool success);

private slots:
    void onControlLoop();      
    void onJogTimerTimeout();  

private:
    URController* m_urRobot = nullptr;
    QString m_robotIp;
    int m_stepperSN = -1;
    
    QTimer* m_controlTimer = nullptr;     
    QTimer* m_jogTimer = nullptr;  

    bool m_isRobotConnected = false;
    bool m_isStepperConnected = false;
    std::atomic<bool> m_stopFlag;

    QMutex m_dataMutex; 
    double m_targetPose[6]; 
    double m_targetNeedleVel = 0.0; 
    bool m_hasNewTarget = false; // Flag: Only move robot if new target arrives

    // Stepper
    double m_lastAppliedFreq = 0.0;
    const double MOTOR_PITCH_MM = 2.0;       
    const double MOTOR_STEPS_PER_REV = 200.0;
    const double MOTOR_MICROSTEPS = 8.0;     

    int m_currentJogAxis = -1;     
    int m_currentJogDir = 0;       

    // Helpers
    void driver_StepperMove(double velocity_mm_s); 
    void driver_StepperStop();
    void sendURScript(const QString& script);
};