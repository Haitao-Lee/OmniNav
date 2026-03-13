#include "RobotWorker.h"
#include <QThread>
#include <QDebug>
#include <cmath>
#include <QTcpSocket>

#ifndef PI
#define PI 3.14159265358979323846
#endif

RobotWorker::RobotWorker(QObject *parent) : QObject(parent) {
    m_stopFlag = false;
    for(int i=0; i<6; ++i) m_targetPose[i] = 0.0;
}

RobotWorker::~RobotWorker() { disconnectAll(); }

void RobotWorker::init() {
    m_controlTimer = new QTimer(this);
    m_controlTimer->setInterval(20); // 50Hz Loop
    connect(m_controlTimer, &QTimer::timeout, this, &RobotWorker::onControlLoop);
    
    m_jogTimer = new QTimer(this);
    m_jogTimer->setInterval(10); 
    connect(m_jogTimer, &QTimer::timeout, this, &RobotWorker::onJogTimerTimeout);
}

void RobotWorker::connectRobot(const QString& ip) {
    if(m_isRobotConnected) return;
    m_robotIp = ip; 
    emit logMessage("Worker: Connecting to Robot at " + ip);
    m_urRobot = new URController(ip);
    if(m_urRobot->Init()) { 
        m_urRobot->StartSendingStatus();
        m_isRobotConnected = true;
        emit robotConnStatus(true);
        emit logMessage("Worker: UR Connected");
    } else {
        delete m_urRobot; m_urRobot = nullptr;
        emit robotConnStatus(false);
        emit logMessage("Worker: UR Connection Failed.");
    }
}

void RobotWorker::connectStepper(const QString& portName) {
    if(m_isStepperConnected) return;
    int serialNumbers[16];
    int count = UsbDevice_Scan(serialNumbers);
    m_stepperSN = -1;
    
    // Auto-scan logic simplified
    if(count > 0) m_stepperSN = serialNumbers[0];
    else { emit stepperConnStatus(false); return; }

    IO_InitPin(m_stepperSN, 1, 1, 0);
    IO_InitPin(m_stepperSN, 2, 1, 0);
    PWM_Init(m_stepperSN, 0, 2, 45000, 22500, 0, 0);
    
    m_isStepperConnected = true;
    emit stepperConnStatus(true);
    emit logMessage(QString("Worker: Stepper Init (SN: %1)").arg(m_stepperSN));
}

void RobotWorker::disconnectAll() {
    stopAll();
    if(m_urRobot) { delete m_urRobot; m_urRobot = nullptr; m_isRobotConnected=false; emit robotConnStatus(false); }
    if(m_isStepperConnected) { driver_StepperStop(); m_isStepperConnected=false; emit stepperConnStatus(false); }
}

void RobotWorker::startDynamicPuncture() {
    if(!m_isRobotConnected) return;
    m_stopFlag = false;
    m_hasNewTarget = false; 
    if(!m_controlTimer->isActive()) m_controlTimer->start();
    emit logMessage("Worker: Loop Started.");
}

void RobotWorker::stopAll() {
    m_stopFlag = true;
    m_controlTimer->stop();
    if(m_urRobot) {
        RobotVector6 zeros = {0};
        try { m_urRobot->MoveSpeedPosOnly(zeros, 5.0, 0.0, 0.1, 0.0, 0.0); m_urRobot->Stop(); } catch(...){}
    }
    driver_StepperStop();
    emit logMessage("Worker: Stopped.");
}

void RobotWorker::updateDynamicTarget(double tx, double ty, double tz, 
                                      double rx, double ry, double rz, 
                                      double needleVel)
{
    QMutexLocker locker(&m_dataMutex);
    m_targetPose[0] = tx; m_targetPose[1] = ty; m_targetPose[2] = tz;
    m_targetPose[3] = rx; m_targetPose[4] = ry; m_targetPose[5] = rz;
    m_targetNeedleVel = needleVel;
    
    // IMPORTANT: Set flag so onControlLoop knows to execute MoveSoftPose
    // If needleVel is 0, it means Robot Motion Command.
    // If needleVel != 0, it is Needle Motion Command.
    m_hasNewTarget = true; 
}

void RobotWorker::updateRobotPose() {
    if(!m_urRobot || !m_isRobotConnected) return;
    auto s = m_urRobot->GetLastStatus();
    if(s) emit robotPoseUpdated(s->actual_TCP_pose[0], s->actual_TCP_pose[1], s->actual_TCP_pose[2], s->actual_TCP_pose[3], s->actual_TCP_pose[4], s->actual_TCP_pose[5]);
}

// =========================================================
// MAIN LOOP: Executioner
// =========================================================
void RobotWorker::onControlLoop()
{
    if (m_stopFlag || !m_urRobot || !m_isRobotConnected) return;

    // 1. Feedback
    auto status = m_urRobot->GetLastStatus();
    if (status) {
        emit robotPoseUpdated(status->actual_TCP_pose[0], status->actual_TCP_pose[1], status->actual_TCP_pose[2],
                              status->actual_TCP_pose[3], status->actual_TCP_pose[4], status->actual_TCP_pose[5]);
    }

    // 2. Read Data
    RobotVector6 cmdPose;
    double cmdVel = 0.0;
    bool triggerRobotMove = false;

    {
        QMutexLocker locker(&m_dataMutex);
        cmdVel = m_targetNeedleVel;
        
        // Execute Robot Move ONLY if new target flag is set
        if (m_hasNewTarget) {
            for(int i=0; i<6; ++i) cmdPose[i] = m_targetPose[i];
            m_hasNewTarget = false; // Clear flag
            triggerRobotMove = true;
        }
    }

    // 3. Robot Execution
    if (triggerRobotMove) {
        try {
            // Using MoveSoftPose as requested. 
            // Since we only call this once per phase (not 50Hz), it will execute properly.
            if (status) {
                double curX = status->actual_TCP_pose[0];
                double curY = status->actual_TCP_pose[1];
                double curZ = status->actual_TCP_pose[2];

                double tgtX = cmdPose[0];
                double tgtY = cmdPose[1];
                double tgtZ = cmdPose[2];

                double distSq = std::pow(tgtX - curX, 2) + 
                                std::pow(tgtY - curY, 2) + 
                                std::pow(tgtZ - curZ, 2);
                double distM = std::sqrt(distSq);
                
                emit logMessage(QString("--------------------------------------------------"));
                emit logMessage(QString("Worker: MOTION PLAN"));
                emit logMessage(QString("   Start: [%1, %2, %3]")
                                .arg(curX, 0, 'f', 4).arg(curY, 0, 'f', 4).arg(curZ, 0, 'f', 4));
                emit logMessage(QString("   Target: [%1, %2, %3]")
                                .arg(tgtX, 0, 'f', 4).arg(tgtY, 0, 'f', 4).arg(tgtZ, 0, 'f', 4));
                
                // 重点看这一行！如果是 Approach 阶段，这里应该是 100mm 左右
                emit logMessage(QString("   DISTANCE: %1 mm").arg(distM * 1000.0, 0, 'f', 2));
                emit logMessage(QString("--------------------------------------------------"));

                // [可选] 软件限位保护：如果距离太远（比如超过 30cm），禁止移动
                if (distM > 0.50) {
                    emit logMessage("Worker SAFETY STOP: Distance too large (>30cm)! Command Ignored.");
                    return; 
                }
            }
            m_urRobot->MoveSoftPose(cmdPose, 1.0, 0.2); 
        } catch (...) {
            emit logMessage("Worker Error: Robot Move failed.");
        }
    }

    // 4. Stepper Execution (Continuous)
    if (std::abs(cmdVel) > 0.001) {
        driver_StepperMove(cmdVel);
    } else {
        driver_StepperStop();
    }
}

// =========================================================
// Stepper Driver
// =========================================================
void RobotWorker::driver_StepperMove(double velocity_mm_s)
{
    if (!m_isStepperConnected || m_stepperSN < 0) return;
    if (std::abs(velocity_mm_s) < 0.1) { driver_StepperStop(); return; }

    if (velocity_mm_s > 0) { IO_WritePin(m_stepperSN, 1, 1); IO_WritePin(m_stepperSN, 2, 0); } 
    else { IO_WritePin(m_stepperSN, 1, 0); IO_WritePin(m_stepperSN, 2, 0); }

    double absVel = std::abs(velocity_mm_s);
    double targetFreq = (absVel / MOTOR_PITCH_MM) * MOTOR_STEPS_PER_REV * MOTOR_MICROSTEPS;
    if (targetFreq > 6400.0) targetFreq = 6400.0; 
    if (targetFreq < 20.0) targetFreq = 20.0;

    if (std::abs(targetFreq - m_lastAppliedFreq) < 5.0) return;

    int precision = (int)(1000000.0 / targetFreq);
    if (precision > 65535) precision = 65535;
    if (precision < 2) precision = 2;

    int ret = PWM_Init(m_stepperSN, 0, 72, precision, precision/2, 0, 0);
    if (ret >= 0) { PWM_Start(m_stepperSN, 0, 0); m_lastAppliedFreq = targetFreq; }
}

void RobotWorker::driver_StepperStop() {
    if (!m_isStepperConnected) return;
    PWM_Stop(m_stepperSN, 0); m_lastAppliedFreq = 0.0;
}

// =========================================================
// Jog & Manual
// =========================================================
void RobotWorker::setFreeDriveMode(bool e) { if(m_isRobotConnected) sendURScript(e ? "def f():\n freedrive_mode()\nend\n" : "def f():\n end_freedrive_mode()\nend\n"); }
void RobotWorker::sendURScript(const QString& s) { QTcpSocket sock; sock.connectToHost(m_robotIp, 30002); if(sock.waitForConnected(200)) { sock.write(s.toUtf8()); sock.close(); } }
void RobotWorker::startJog(int a, int d) { m_currentJogAxis=a; m_currentJogDir=d; m_jogTimer->start(); }
void RobotWorker::stopJog() { m_jogTimer->stop(); if(m_urRobot) { RobotVector6 z={0}; m_urRobot->MoveSpeedPosOnly(z,5,0,0.1,0,0); } }
void RobotWorker::manualControlStepper(double v) { 
    QMutexLocker locker(&m_dataMutex); m_targetNeedleVel = v; 
    if(abs(v)>0.001) driver_StepperMove(v); else driver_StepperStop();
}
void RobotWorker::onJogTimerTimeout() {
    if(!m_urRobot) return;
    RobotVector6 s = {0};
    double v = (m_currentJogAxis < 3) ? 0.05 : 0.15;
    s[m_currentJogAxis] = v * m_currentJogDir;
    try { m_urRobot->MoveSpeedPosOnly(s, 2.0, 0.0, 0.02, 0.0, 0.02); } catch(...) { stopJog(); }
}