#include "URController.h"
#include "NetworkUtility.h"
#include "DataStream.hpp"
#include "EasyBuffer.h"
#include "EasyDataIO.h"
#include "IOUtility.h"
#include "QtUtility.h"
#include "scope_guard.hpp"
#include "static_block.hpp"

#include <QCoreApplication>
#include <QTcpSocket>
#include <QThread>
#include <QFile>

#include <format>
#include <spdlog/spdlog.h>

#include <atomic>
#include <shared_mutex>
#include <vector>
#include <string>
#include <atomic>
#include <qDebug.h>

static_block {
    qRegisterMetaType<URStatus>("URStatus");
    qRegisterMetaType<PURStatusT>("PURStatusT");
};

namespace URControllerInner {

    struct Packet {
        enum PacketType {
            RTDE_REQUEST_PROTOCOL_VERSION = 86,
            RTDE_GET_URCONTROL_VERSION = 118,
            RTDE_TEXT_MESSAGE = 77,
            RTDE_DATA_PACKAGE = 85,
            RTDE_CONTROL_PACKAGE_SETUP_OUTPUTS = 79,
            RTDE_CONTROL_PACKAGE_SETUP_INPUTS = 73,
            RTDE_CONTROL_PACKAGE_START = 83,
            RTDE_CONTROL_PACKAGE_PAUSE = 80
        };

        PacketType type;
        std::unique_ptr<char[]> data;
        uint16_t data_len;

        nonstd::span<char> getSpan() const {
            return {data.get(), data.get() + data_len};
        }

        void initData(uint16_t length) {
            data_len = length;
            if (length > 0) {
                data = std::make_unique<char[]>(length);
            } else {
                data = nullptr;
            }
        }

        static Packet *newPacketFromRaw(nonstd::span<const char> dataSpan) {
            auto ret = new Packet();
            ret->type = (PacketType) dataSpan[0];
            ret->initData(dataSpan.size() - 1);
            std::copy_n(dataSpan.begin() + 1, ret->data_len, ret->data.get());
            return ret;
        }
    };

    struct cmdPacket {
        bool isTargetPose = false;
        int32_t moveType = 0;
        double target[6] = {0};
        double maxPosV = 0.1; // 10 mm/s
        double maxAngV = 0.1; // 0.1 rad/s
        double maxPosA = 0.5; // 50 mm/s^2
        double maxAngA = 0.5; // 0.5 rad/s^2
        double timeSlice = 0.01; // 100Hz
        double pTerm = 1.0; // 比例放大系数
    };
}

struct URController::impl {

    QString host;
    QTcpSocket *socket;
    std::unique_ptr<DataStream> dataStream;

    URController *qThis = nullptr;

    uint8_t outputRecipeId = -1;
    uint8_t inputRecipeId = -1;
    std::atomic_bool isValid, isSendingStatus;
    QMetaObject::Connection newDataPacketConnection;
    QMetaObject::Connection newStatusConnection;

    PURStatusT lastStatus;
    std::shared_mutex rwMu;

    using PacketT = URControllerInner::Packet;
    using PPacketT = std::shared_ptr<PacketT>;
    using CmdPacketT = URControllerInner::cmdPacket;
    using PCmdPacketT = std::shared_ptr<CmdPacketT>;

    bool checkThread() {
        if (QThread::currentThread() != qThis->thread()) {
            SPDLOG_ERROR("This function should be called in the thread which creates URController.");
            return false;
        }
        return true;
    }

    bool writePacket(PPacketT packet) const {
        static thread_local EasyBuffer buffer(1024);
        buffer.Ensure(packet->data_len + 1);
        buffer.Data()[0] = packet->type;
        auto dataSpan = packet->getSpan();
        std::copy(dataSpan.begin(), dataSpan.end(), buffer.Data() + 1);
        auto ret = IOUtility::WritePacket(*dataStream,
                                          buffer.GetSpan(0, packet->data_len + 1));
        IOUtility::TryFlushIODevice(socket);
        return ret;
    }

    void onNewOutPacket(void *pPacket) const {
        auto realPacket = std::shared_ptr<PacketT>((PacketT *) pPacket);
        auto ok = writePacket(std::move(realPacket));
        if (!ok) {
            SPDLOG_WARN("Failed to send packet.");
        }
    }

    void writeCmdPacket(PCmdPacketT packet) const {
        static thread_local EasyDataIO dio;
        auto outPacket = std::make_unique<PacketT>();
        outPacket->type = URControllerInner::Packet::RTDE_DATA_PACKAGE;
        outPacket->initData(102);
        dio.SetBuffer(outPacket->getSpan());
        dio.Write(inputRecipeId); // 1 byte
        dio.Write(packet->isTargetPose); // 1 byte
        dio.Write(packet->moveType); // 4 bytes
        for (const auto &value: packet->target) { // 6 * 8 = 48 bytes
            dio.Write(value);
        }
        dio.Write(packet->maxPosV); // 8 bytes
        dio.Write(packet->maxAngV); // 8 bytes
        dio.Write(packet->maxPosA); // 8 bytes
        dio.Write(packet->maxAngA); // 8 bytes
        dio.Write(packet->timeSlice); // 8 bytes
        dio.Write(packet->pTerm); // 8 bytes
        emit qThis->newOutPacket(outPacket.release());
    }

    PPacketT sendPacketAndGetReply(PPacketT packet) const {
        static thread_local auto pWaiter = new QtUtility::ResultWaiter();

        pWaiter->Reset();
        auto connection =
                QObject::connect(qThis, &URController::newPacket,
                                 [type = packet->type, waiter = pWaiter](void *packet) {
                                     auto realPacket = (PacketT *) packet;
                                     if (realPacket->type != type) return;
                                     emit waiter->Notify(std::shared_ptr<PacketT>(realPacket));
                                 });
        auto closer = sg::make_scope_guard([&]() { QObject::disconnect(connection); });

        bool okRet = writePacket(packet);
        if (!okRet) {
            SPDLOG_ERROR("Failed to send packet");
            return nullptr;
        }

        okRet = false;
        auto replyAny = pWaiter->WaitForResult(500ms, &okRet);
        if (!okRet) {
            SPDLOG_ERROR("Failed to get packet reply.");
            return nullptr;
        }

        return std::any_cast<PPacketT>(replyAny);
    }

    void tryReadPacket() const {
        static thread_local EasyBuffer buffer(1024);
        while (socket->bytesAvailable() > 0) {
            auto data = IOUtility::ReadPacket(*dataStream, buffer);
            auto packet = PacketT::newPacketFromRaw(data);
            emit qThis->newPacket(packet); // 收到之后立刻应立刻包装进智能指针
        }
    }

    // Use protocol version 2
    bool negotiateProtocolVersion() const {
        auto packet = std::make_shared<PacketT>();
        packet->type = PacketT::RTDE_REQUEST_PROTOCOL_VERSION;
        packet->initData(2);
        loc2net((uint16_t) 2, packet->getSpan());

        auto reply = sendPacketAndGetReply(packet);
        return reply && reply->data[0] == 1; // 1 means success
    }

    bool setupOutput(double frequency) {
        static std::vector<std::string> outputNameList = {
                "timestamp", // DOUBLE
                "target_q", // VECTOR6D
                "target_qd", // VECTOR6D
                "target_qdd", // VECTOR6D
                "actual_q", // VECTOR6D
                "actual_qd", // VECTOR6D
                "actual_TCP_pose", // VECTOR6D
                "actual_TCP_speed", // VECTOR6D
                "target_TCP_pose", // VECTOR6D
                "target_TCP_speed", // VECTOR6D
                "actual_TCP_force",
                "robot_mode", // INT32
                "runtime_state", // UINT32
                "safety_mode", // INT32
                "output_bit_register_64", // BOOL, is moving
        };

        static thread_local EasyBuffer buffer(1024);
        static thread_local EasyDataIO dio;

        auto dataSpan = IOUtility::JoinAndWrite(outputNameList, ',', buffer);

        auto packet = std::make_shared<PacketT>();
        packet->type = PacketT::RTDE_CONTROL_PACKAGE_SETUP_OUTPUTS;
        packet->initData(dataSpan.size() + 8);
        dio.SetBuffer(packet->getSpan());
        dio.Write(frequency); // output frequency
        dio.WriteRawBytes(dataSpan); // variable names

        auto reply = sendPacketAndGetReply(packet);
        if (!reply) return false;
        outputRecipeId = reply->data[0];
        if (outputRecipeId == 0) {
            SPDLOG_ERROR("Recipe invalid.");
            return false;
        }

        return true;
    }

    bool setupInput() {
        static std::vector<std::string> inputNameList = {
                "input_bit_register_65", // target 是否表示为 pose 的形式
                "input_int_register_25", // 控制模式 Stop = 0, Hard = 1, Soft = 2, SoftPosOnly = 3
                "input_double_register_24", // 6 个参数表示 target
                "input_double_register_25",
                "input_double_register_26",
                "input_double_register_27",
                "input_double_register_28",
                "input_double_register_29",
                "input_double_register_30", // 线速度上限
                "input_double_register_31", // 角速度上限
                "input_double_register_32", // 线加速度上限
                "input_double_register_33", // 角加速度上限
                "input_double_register_34", // Speed 系列函数单次占用时间
                "input_double_register_35", // P-term
        };

        static thread_local EasyBuffer buffer(1024);

        auto dataSpan = IOUtility::JoinAndWrite(inputNameList, ',', buffer);

        auto packet = std::make_shared<PacketT>();
        packet->type = PacketT::RTDE_CONTROL_PACKAGE_SETUP_INPUTS;
        packet->initData(dataSpan.size());
        std::copy_n(dataSpan.data(), dataSpan.size(), packet->data.get());

        auto reply = sendPacketAndGetReply(packet);
        if (!reply) return false;
        inputRecipeId = reply->data[0];
        if (inputRecipeId == 0) {
            SPDLOG_ERROR("Recipe invalid.");
            return false;
        }

        return true;
    }

    bool installProgram() const {
        auto cmdPacket = std::make_shared<URControllerInner::cmdPacket>();
        cmdPacket->moveType = 0;
        writeCmdPacket(std::move(cmdPacket));
        QCoreApplication::processEvents();

        auto sSocket = NetworkUtility::NewSocketAsClient(host, 30002);
        if (!sSocket) {
            SPDLOG_ERROR("Failed to connect URScript port {}.", 30002);
            return false;
        }

        QString scriptPath = QCoreApplication::applicationDirPath() + "/URScript.script";
    
        auto script = QFile(scriptPath);
        if (!script.exists()) {
            SPDLOG_ERROR("Failed to open file URScript.script.");
            return false;
        }
        script.open(QIODevice::ReadOnly);

        auto scriptData = script.readAll();
        auto ret = IOUtility::TryWriteAll(sSocket.release(), scriptData.data(), scriptData.size());
        if (!ret) {
            SPDLOG_ERROR("Failed to install URScript.");
            return false;
        }

        return true;
    }

    bool startSending() const {
        auto packet = std::make_shared<PacketT>();
        packet->type = PacketT::RTDE_CONTROL_PACKAGE_START;
        packet->initData(0);

        auto reply = sendPacketAndGetReply(packet);
        return reply && reply->data[0] == 1; // 1 means success
    }

    bool pauseSending() const {
        auto packet = std::make_shared<PacketT>();
        packet->type = PacketT::RTDE_CONTROL_PACKAGE_PAUSE;
        packet->initData(0);

        auto reply = sendPacketAndGetReply(packet);
        return reply && reply->data[0] == 1; // 1 means success
    }

    void onNewDataPackage(void *packet) const {
        static thread_local EasyDataIO dio;

        auto realPacket = PPacketT((PacketT *) packet);
        if (realPacket->type != decltype(realPacket->type)::RTDE_DATA_PACKAGE) return;
        dio.SetBuffer(realPacket->getSpan());

        uint8_t recipeID;
        dio.Read(recipeID);
        if (recipeID != outputRecipeId) {
            SPDLOG_WARN("Recipe id mismatched.");
            return;
        }

        auto status = std::make_shared<URStatus>();
        dio.Read(status->timestamp);
        ReadRobotVector(dio, status->target_q);
        ReadRobotVector(dio, status->target_qd);
        ReadRobotVector(dio, status->target_qdd);
        ReadRobotVector(dio, status->actual_q);
        ReadRobotVector(dio, status->actual_qd);
        ReadRobotVector(dio, status->actual_TCP_pose);
        ReadRobotVector(dio, status->actual_TCP_speed);
        ReadRobotVector(dio, status->actual_TCP_force);

        ReadRobotVector(dio, status->target_TCP_pose);
        ReadRobotVector(dio, status->target_TCP_speed);

        static thread_local int32_t robotModeValue;
        dio.Read(robotModeValue);
        status->robot_mode = (URStatus::RobotModeT) robotModeValue;

        static thread_local uint32_t runtimeStatusValue;
        dio.Read(runtimeStatusValue);
        status->runtime_state = (URStatus::RuntimeStatusT) runtimeStatusValue;

        static thread_local int32_t safetyModeValue;
        dio.Read(safetyModeValue);
        status->safety_mode = (URStatus::SafetyModeT) safetyModeValue;

        dio.Read(status->isMoving);

        emit qThis->NewStatus(std::move(status));
    }

    void onNewStatus(PURStatusT status) {
        std::unique_lock lock(rwMu);
        lastStatus = std::move(status);
    }

    void onIODeviceAboutToClose() {
        isSendingStatus = false;
        isValid = false;
   
     
    }
};

URController::URController(const QString &host) :
        pImpl(std::make_unique<impl>()) {
    pImpl->host = host;
    pImpl->qThis = this;
}

URController::~URController() = default;

bool URController::Init(double frequency) {
    if (!pImpl->checkThread()) return false;
    if (pImpl->isValid.exchange(true)) {
        SPDLOG_WARN("Status reporter is initialized.");
        return false;
    }
    auto closer = sg::make_scope_guard([&]() { pImpl->isValid = false; });

    auto pSocket = NetworkUtility::NewSocketAsClient(pImpl->host, 30004);
    if (!pSocket) return false;
    pImpl->socket = pSocket.release();
    pImpl->socket->setParent(this);
    QObject::connect(pImpl->socket, &QIODevice::readyRead,
                     std::bind(&impl::tryReadPacket, pImpl.get()));
    QObject::connect(pImpl->socket, &QIODevice::aboutToClose,
                     std::bind(&impl::onIODeviceAboutToClose, pImpl.get()));

    pImpl->dataStream = std::make_unique<DataStream>(pImpl->socket);

    auto okRet = pImpl->negotiateProtocolVersion();
    if (!okRet) {
        SPDLOG_ERROR("Failed to negotiate RTDE protocol version.");
        return false;
    }

    okRet = pImpl->setupOutput(frequency);
    if (!okRet) {
        SPDLOG_ERROR("Failed to setup output.");
        return false;
    }

    okRet = pImpl->setupInput();
    if (!okRet) {
        SPDLOG_ERROR("Failed to setup input.");
        return false;
    }

    using namespace std::placeholders;
    pImpl->newStatusConnection =
        QObject::connect(this, &URController::NewStatus,
            std::bind(&impl::onNewStatus, pImpl.get(), _1));
    QObject::connect(this, &URController::newOutPacket, this,
        std::bind(&impl::onNewOutPacket, pImpl.get(), _1));

    okRet = pImpl->installProgram();
    if (!okRet) {
        SPDLOG_ERROR("Failed to install URScript.");
        return false;
    }

    if (!StartSendingStatus()) return false;
    auto status = GetNextStatus();
    PauseSendingStatus();
    if (!status) {
        SPDLOG_ERROR("Failed to sync status.");
        return false;
    }

    closer.dismiss();
    return true;
}


bool URController::StartSendingStatus() {
    if (!pImpl->checkThread()) return false;
    if (pImpl->isSendingStatus.exchange(true)) {
        SPDLOG_WARN("Status reporter is running.");
        return false;
    }
    auto closer = sg::make_scope_guard([&]() { pImpl->isSendingStatus = false; });

    if (!pImpl->isValid) {
        SPDLOG_ERROR("Call Init() before StartSendingStatus().");
        return false;
    }

    auto okRet = pImpl->startSending();
    if (!okRet) {
        SPDLOG_ERROR("Failed to startSending sending output.");
        return false;
    }

    using namespace std::placeholders;
    pImpl->newDataPacketConnection =
            QObject::connect(this, &URController::newPacket,
                             std::bind(&impl::onNewDataPackage, pImpl.get(), _1));

    closer.dismiss();
    return true;
}

bool URController::PauseSendingStatus() {
    if (!pImpl->checkThread()) return false;
    if (!pImpl->isSendingStatus.exchange(false)) {
        SPDLOG_WARN("Status reporter is not running.");
        return false;
    }
    QObject::disconnect(pImpl->newDataPacketConnection);
    pImpl->pauseSending();
    return true;
}

PURStatusT URController::GetLastStatus() {
    std::shared_lock lock(pImpl->rwMu);
    return pImpl->lastStatus;
}

PURStatusT URController::GetNextStatus() {
    static thread_local auto pWaiter = new QtUtility::ResultWaiter();

    if (!pImpl->isSendingStatus) {
        SPDLOG_WARN("Status reporter is not running. Cannot get next status.");
        return nullptr;
    }

    pWaiter->Reset();
    auto connection =
            QObject::connect(this, &URController::NewStatus, [waiter = pWaiter](PURStatusT status) {
                waiter->Notify(std::move(status));
            });
    auto closer = sg::make_scope_guard([&]() { QObject::disconnect(connection); });

    bool okRet = false;
    auto replyAny = pWaiter->WaitForResult(500ms, &okRet);
    if (!okRet) {
        SPDLOG_ERROR("Failed to get new status in 500ms.");
        return nullptr;
    }

    return std::any_cast<PURStatusT>(replyAny);
}

void URController::MoveHardPose(const RobotVector6 &pose, double tv, double ta) {
    auto cmdPacket = std::make_shared<impl::CmdPacketT>();
    cmdPacket->isTargetPose = true;
    cmdPacket->moveType = 1;
    std::copy_n(pose.begin(), 6, cmdPacket->target);
    cmdPacket->maxPosV = tv;
    cmdPacket->maxPosA = ta;
    pImpl->writeCmdPacket(std::move(cmdPacket));
}

void URController::MoveHardJoint(const RobotVector6 &joint, double jv, double ja) {
    auto cmdPacket = std::make_shared<impl::CmdPacketT>();
    cmdPacket->isTargetPose = false;
    cmdPacket->moveType = 1;
    std::copy_n(joint.begin(), 6, cmdPacket->target);
    cmdPacket->maxAngV = jv;
    cmdPacket->maxAngA = ja;
    pImpl->writeCmdPacket(std::move(cmdPacket));
}

void URController::MoveSoftPose(const RobotVector6 &pose, double kp, double jv, double ja, double timeSlice) {
    //SPDLOG_ERROR("pose: {}", pose);
    for (auto& v : pose)
        if (isnan(v))
            return;
    auto cmdPacket = std::make_shared<impl::CmdPacketT>();
    cmdPacket->isTargetPose = true;
    cmdPacket->moveType = 2;
    std::copy_n(pose.begin(), 6, cmdPacket->target);
    cmdPacket->maxAngV = jv;
    cmdPacket->maxAngA = ja;
    cmdPacket->timeSlice = timeSlice;
    cmdPacket->pTerm = kp;
    pImpl->writeCmdPacket(std::move(cmdPacket));
}

void URController::MoveSoftJoint(const RobotVector6 &joint, double kp, double jv, double ja, double timeSlice) {
    auto cmdPacket = std::make_shared<impl::CmdPacketT>();
    cmdPacket->isTargetPose = false;
    cmdPacket->moveType = 2;
    std::copy_n(joint.begin(), 6, cmdPacket->target);
    cmdPacket->maxAngV = jv;
    cmdPacket->maxAngA = ja;
    cmdPacket->timeSlice = timeSlice;
    cmdPacket->pTerm = kp;
    pImpl->writeCmdPacket(std::move(cmdPacket));
}

void URController::MoveSoftPosOnly(const RobotVector6 &pose, double kp, double tv, double ta,
                                   double ja, double timeSlice) {
    auto cmdPacket = std::make_shared<impl::CmdPacketT>();
    cmdPacket->isTargetPose = true;
    cmdPacket->moveType = 3;
    std::copy_n(pose.begin(), 6, cmdPacket->target);
    cmdPacket->maxPosV = tv;
    cmdPacket->maxPosA = ta;
    cmdPacket->maxAngA = ja;
    cmdPacket->timeSlice = timeSlice;
    cmdPacket->pTerm = kp;
    pImpl->writeCmdPacket(std::move(cmdPacket));
}

void URController::MoveSpeedPosOnly(const RobotVector6& speed, double kp, double tv, double ta,
                                    double ja, double timeSlice) {
	auto cmdPacket = std::make_shared<impl::CmdPacketT>();
    cmdPacket->isTargetPose = true;
	cmdPacket->moveType = 4;
	std::copy_n(speed.begin(), 3, cmdPacket->target);
	cmdPacket->maxPosV = tv;
	cmdPacket->maxPosA = ta;
	cmdPacket->maxAngA = ja;
	cmdPacket->timeSlice = timeSlice;
	cmdPacket->pTerm = kp;
	pImpl->writeCmdPacket(std::move(cmdPacket));
}

void URController::Stop() {
    if (!pImpl->checkThread()) return;

    auto cmdPacket = std::make_shared<URControllerInner::cmdPacket>();
    cmdPacket->moveType = 0; // 停止运动
    pImpl->writeCmdPacket(std::move(cmdPacket));
    QCoreApplication::processEvents(); // 手动调用事件循环，使得命令发出
}

bool URController::WaitForMotionEnd(std::chrono::milliseconds timeout) {
    static thread_local auto pWaiter = new QtUtility::ResultWaiter();

    auto lastStatus = GetNextStatus();
    if (lastStatus && !lastStatus->isMoving)
        return true;

    pWaiter->Reset();
    auto connection =
            QObject::connect(this, &URController::NewStatus, [waiter = pWaiter](PURStatusT status) {
                if (!status->isMoving)
                    waiter->Notify(0);
            });
    auto closer = sg::make_scope_guard([&]() { QObject::disconnect(connection); });

    bool okRet;
    auto curCmdIdAny = pWaiter->WaitForResult(timeout, &okRet);
    return okRet;
}

RobotVector6 URController::GetJointPos() {
    std::shared_lock lock(pImpl->rwMu);
    return pImpl->lastStatus->actual_q;
}
