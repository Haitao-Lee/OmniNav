#ifndef SOPHIAR_URCONTROLLER_H
#define SOPHIAR_URCONTROLLER_H

#include "RobotTypes.h"

#include <memory>
#include <span>
#include <chrono>

#include <QObject>
#include <QString>

using namespace std::chrono_literals;

struct URStatus {
    // https://forum.universal-robots.com/t/rtde-runtime-state-enumeration/6634
    enum RuntimeStatusT {
        RUNTIME_STATUS_STOPPING = 0,
        RUNTIME_STATUS_STOPPED = 1,
        RUNTIME_STATUS_PLAYING = 2,
        RUNTIME_STATUS_PAUSING = 3,
        RUNTIME_STATUS_PAUSED = 4,
        RUNTIME_STATUS_RESUMING = 5
    };

    enum RobotModeT {
        ROBOT_MODE_NO_CONTROLLER = -1,
        ROBOT_MODE_DISCONNECTED = 0,
        ROBOT_MODE_CONFIRM_SAFETY = 1,
        ROBOT_MODE_BOOTING = 2,
        ROBOT_MODE_POWER_OFF = 3,
        ROBOT_MODE_POWER_ON = 4,
        ROBOT_MODE_IDLE = 5,
        ROBOT_MODE_BACKDRIVE = 6,
        ROBOT_MODE_RUNNING = 7,
        ROBOT_MODE_UPDATING_FIRMWARE = 8
    };

    enum SafetyModeT {
        SAFETY_MODE_NORMAL = 1,
        SAFETY_MODE_REDUCED = 2,
        SAFETY_MODE_PROTECTIVE_STOP = 3,
        SAFETY_MODE_RECOVERY = 4,
        SAFETY_MODE_SAFEGUARD_STOP = 5,
        SAFETY_MODE_SYSTEM_EMERGENCY_STOP = 6,
        SAFETY_MODE_ROBOT_EMERGENCY_STOP = 7,
        SAFETY_MODE_VIOLATION = 8,
        SAFETY_MODE_FAULT = 9,
        SAFETY_MODE_VALIDATE_JOINT_ID = 10,
        SAFETY_MODE_UNDEFINED_SAFETY_MODE = 11
    };

    double timestamp;
    RobotVector6 target_q;
    RobotVector6 target_qd;
    RobotVector6 target_qdd;
    RobotVector6 actual_q;
    RobotVector6 actual_qd;
    RobotVector6 actual_TCP_pose;
    RobotVector6 actual_TCP_speed;
    RobotVector6 target_TCP_pose;
    RobotVector6 target_TCP_speed;
    RobotVector6 actual_TCP_force;
    RobotModeT robot_mode;
    RuntimeStatusT runtime_state;
    SafetyModeT safety_mode;
    bool isMoving;
};
using PURStatusT = std::shared_ptr<URStatus>;

class URController : public QObject {
Q_OBJECT
public:
    explicit URController(const QString &host);

    ~URController();

    bool Init(double frequency = 125);

    bool StartSendingStatus();

    bool PauseSendingStatus();

    // 获取已经收到的最近一个状态
    PURStatusT GetLastStatus();

    // 等待并返回下一个状态
    PURStatusT GetNextStatus();

    static constexpr double a_joint_default = 1.4; // rad/s^2
    static constexpr double a_tool_default = 1.2; // m/s^2
    static constexpr double v_joint_default = 1.05; // rad/s
    static constexpr double v_tool_default = 0.25; // m/s
    static constexpr double time_slice_default = 0.008; // s

    void Stop(); // 急停
    RobotVector6 GetJointPos(); // 获取当前关节位置

    // 使用 move 系列指令移动
    // pose: m, tv: m/s, ta: m/s^2
    void MoveHardPose(const RobotVector6 &pose, double tv = v_tool_default, double ta = a_tool_default);

    void MoveHardJoint(const RobotVector6 &joint, double jv = v_joint_default, double ja = a_joint_default);

    // 使用 speed 系列指令移动
    // timeSlice: s
    void MoveSoftPose(const RobotVector6 &pose, double kp = 1.0,
                      double jv = v_joint_default, double ja = a_joint_default,
                      double timeSlice = time_slice_default);

    void MoveSoftJoint(const RobotVector6 &joint, double kp = 1.0,
                       double jv = v_joint_default, double ja = a_joint_default,
                       double timeSlice = time_slice_default);

    // 使用 speed 系列指令移动, 魔改过了，可以旋转了
    void MoveSoftPosOnly(const RobotVector6 &pose, double kp = 1.0,
                         double tv = v_tool_default, double ta = a_tool_default,
                         double ja = a_joint_default, double timeSlice = time_slice_default);

    //使用 speedl 指令移动，只平移，传入的speed参数直接是速度值
	void MoveSpeedPosOnly(const RobotVector6& speed, double kp =1.0, double tv = v_tool_default, 
        double ta = a_tool_default, double ja = a_joint_default, double timeSlice = time_slice_default);

    // 在指定时间内机器人停止了就返回 true
    bool WaitForMotionEnd(std::chrono::milliseconds timeout = 10s);

signals:

    void newPacket(void *pPacket); // 内部使用
    void newOutPacket(void *pPacket); // 内部使用
    void NewStatus(PURStatusT status);

private:
    struct impl;
    std::unique_ptr<impl> pImpl;
};



#endif //SOPHIAR_URCONTROLLER