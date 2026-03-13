// #include "NDIWorker.h"

// #include <QThread>
// #include <QDebug>
// #include <QElapsedTimer>
// #include <QFile>
// #include <QMutexLocker>
// #include <cstdio>
// #include <vector>
// #include <algorithm>

// // 兼容 ndicapi 的 const char* 转换
// #define NDI_C(s) const_cast<char*>(s)

// NDIWorker::NDIWorker(QObject *parent)
//     : QObject(parent),
//       device(nullptr),
//       m_keepRunning(false),
//       FPS(60) // 默认 60Hz 采集
// {
// }

// NDIWorker::~NDIWorker() {
//     stop();
// }

// /* =========================================================
//  * Device Connection
//  * ========================================================= */
// void NDIWorker::connectDevice(QString ipAddress, int port) {
//     if (device) {
//         ndiCloseNetwork(device);
//         device = nullptr;
//     }

//     qDebug() << "[NDI_WORKER] Connecting to" << ipAddress << ":" << port;

//     QByteArray ipBytes = ipAddress.toLatin1();
//     device = ndiOpenNetwork(NDI_C(ipBytes.data()), port);

//     if (!device) {
//         emit errorOccurred(QString("Failed to connect to NDI at %1:%2").arg(ipAddress).arg(port));
//         return;
//     }

//     int err = ndiGetError(device);
//     if (err != NDI_OKAY) {
//         emit errorOccurred(QString("NDI connection error code: %1").arg(err));
//         ndiCloseNetwork(device);
//         device = nullptr;
//         return;
//     }

//     // 连接成功后，先发一个 TSTOP 确保设备安静，防止残留数据干扰
//     ndiCommand(device, NDI_C("TSTOP:"));
    
//     emit statusMessage(QString("NDI Connected via IP %1:%2").arg(ipAddress).arg(port));
// }


// void NDIWorker::addTools(const QMap<QString, QString>& toolsConfig) {
//     if (!device) {
//         emit errorOccurred("NDI device not connected.");
//         return;
//     }
//     if (toolsConfig.isEmpty()) return;

//     qDebug() << "[NDI] Starting tool initialization with Error Purging...";

//     // 1. TSTOP & INIT
//     ndiCommand(device, NDI_C("TSTOP:"));
//     QThread::msleep(100);

//     const char* initResp = ndiCommand(device, NDI_C("INIT:"));
//     if (!initResp || strstr(initResp, "ERROR")) {
//         emit errorOccurred(QString("NDI INIT failed: %1").arg(initResp ? initResp : "NULL"));
//         return;
//     }
//     QThread::msleep(200);

//     // ============================================================
//     // ★★★ 2. 关键修改：强行清洗错误栈 (Mimic Combined API) ★★★
//     // ============================================================
//     // 如果设备有橙灯报警，必须把错误读完，才能解锁 PHRQ
//     qDebug() << "[NDI] Purging system errors/warnings...";
    
//     for (int i = 0; i < 10; ++i) {
//         // 读取错误 (SEER:0)
//         const char* errResp = ndiCommand(device, NDI_C("SEER:0"));
//         // 读取状态 (SSTAT:0) - 这一步很重要，用于清除 "Status Changed" 标志
//         ndiCommand(device, NDI_C("SSTAT:0"));
        
//         // 如果返回 0000，说明错误栈空了，可以停止
//         if (errResp && strstr(errResp, "0000")) {
//             qDebug() << "[NDI] Error stack cleared.";
//             break;
//         }
//         // 如果有错误，打印出来看看，但继续读
//         if (errResp) qDebug() << "[NDI] Ack Error:" << errResp;
//         QThread::msleep(50);
//     }
//     // 再给一点时间让状态复位
//     QThread::msleep(200);
//     // ============================================================

//     // 3. 预防性清理 (PHF)
//     for (int h = 1; h <= 20; ++h) {
//         char cmd[16];
//         sprintf(cmd, "PHF:%02X", h);
//         ndiCommand(device, cmd);
//         QThread::msleep(20);
//     }
//     QThread::msleep(100);

//     // 4. 加载工具
//     QMapIterator<QString, QString> it(toolsConfig);
//     while (it.hasNext()) {
//         it.next();
//         const QString toolName = it.key();
//         const QString romPath = it.value();

//         QFile romFile(romPath);
//         if (!romFile.open(QIODevice::ReadOnly)) {
//             emit errorOccurred(QString("Failed to open ROM: %1").arg(romPath));
//             continue;
//         }
//         QByteArray romData = romFile.readAll();
//         romFile.close();

//         if (romData.isEmpty()) {
//             emit errorOccurred(QString("ROM is empty: %1").arg(romPath));
//             continue;
//         }

//         // --- 申请句柄 (PHRQ) ---
//         const char* phrqResp = ndiCommand(device, NDI_C("PHRQ:********"));
        
//         // 如果清洗成功，应该能拿到句柄了！
//         if (!phrqResp || strstr(phrqResp, "ERROR")) {
//             // 如果清洗了还报错，打印出来看看是什么
//             emit errorOccurred(QString("PHRQ failed for %1: %2").arg(toolName).arg(phrqResp));
//             continue;
//         }

//         unsigned int handle = 0;
//         sscanf(phrqResp, "%02X", &handle);

//         // --- 上传 ROM (PVWR) ---
//         bool writeOk = true;
//         const int chunkSize = 64;
        
//         for (int offset = 0; offset < romData.size(); offset += chunkSize) {
//             int len = std::min(chunkSize, romData.size() - offset);
//             QByteArray chunk = romData.mid(offset, len);
//             QString hex = chunk.toHex().toUpper();

//             const char* pvwrResp = ndiCommand(device,
//                                               NDI_C("PVWR:%02X%04X%s"),
//                                               handle, offset, hex.toStdString().c_str());

//             if (!pvwrResp || strstr(pvwrResp, "ERROR")) {
//                 emit errorOccurred(QString("PVWR failed at offset %1: %2").arg(offset).arg(pvwrResp));
//                 writeOk = false;
//                 break;
//             }
//             QThread::msleep(30); 
//         }

//         if (!writeOk) {
//             ndiCommand(device, NDI_C("PHF:%02X"), handle);
//             continue;
//         }

//         ndiCommand(device, NDI_C("PINIT:%02X"), handle);
//         ndiCommand(device, NDI_C("PENA:%02X%c"), handle, 'D');

//         {
//             QMutexLocker locker(&m_mutex);
//             m_tools[handle] = toolName;
//         }

//         emit statusMessage(QString("Tool loaded: %1 (Handle %2)").arg(toolName).arg(handle));
//     }

//     emit onBatchToolsLoaded(m_tools);
// }


// void NDIWorker::purgeSystemErrors() {
//     if (!device) return;

//     for (int i = 0; i < 10; ++i) {
//         const char* resp = ndiCommand(device, NDI_C("SEER:0"));
//         if (!resp || strstr(resp, "0000")) break; // 没错误了
//         QThread::msleep(50);
//     }
    
//     // 清除可能卡住的 Warning 标志
//     ndiCommand(device, NDI_C("SSTAT:0")); 
// }


// /* =========================================================
//  * Tracking Loop
//  * ========================================================= */
// void NDIWorker::startCollecting() {
//     if (!device) {
//         emit errorOccurred("Cannot start tracking: Device null.");
//         return;
//     }
//     if (m_keepRunning) return;

//     qDebug() << "[NDI_WORKER] Starting tracking loop...";
//     ndiCommand(device, NDI_C("TSTART:"));
//     m_keepRunning = true;

//     // 根据 FPS 计算帧间隔 (毫秒)
//     const int interval = 1000 / (FPS > 0 ? FPS : 60);
//     QElapsedTimer timer;

//     while (m_keepRunning) {
//         timer.restart();

//         // 检查设备连接状态
//         if (!device) {
//             emit errorOccurred("Connection lost during tracking.");
//             break;
//         }

//         // 复制当前工具列表，减小锁的持有时间
//         QMap<int, QString> toolsSnapshot;
//         {
//             QMutexLocker locker(&m_mutex);
//             toolsSnapshot = m_tools;
//         }

//         // 遍历查询每个工具 (GX)
//         for (auto it = toolsSnapshot.begin(); it != toolsSnapshot.end(); ++it) {
//             if (!m_keepRunning) break;

//             char cmd[16];
//             sprintf(cmd, "GX:%02X", it.key());

//             const char* reply = ndiCommand(device, cmd);

//             ToolData data;
//             data.handle = it.key();
//             data.name   = it.value();

//             // 解析 GX 返回的数据
//             if (!parseNDIResponse(reply, data)) {
//                 data.isVisible = false;
//             }

//             emit toolUpdated(data);
//         }

//         // 帧率控制
//         qint64 sleepTime = interval - timer.elapsed();
//         if (sleepTime > 0) {
//             QThread::msleep(sleepTime);
//         }
//     }

//     qDebug() << "[NDI_WORKER] Tracking loop stopped.";
// }

// /* =========================================================
//  * Response Parsing (Standard NDI ASCII)
//  * ========================================================= */
// bool NDIWorker::parseNDIResponse(const char* reply, ToolData& out) {
//     // 基本校验：长度不够或为空
//     if (!reply || strlen(reply) < 30) return false;

//     // GX 回复格式通常为:
//     // HHHH SSSS Q0Q0Q0 QxQxQx QyQyQy QzQzQz TxTxTx TyTyTy TzTzTz EEEE ...
//     // HHHH: 句柄, SSSS: 状态 (0000=Missing, 0001=OK)
    
//     // 检查状态位 (偏移量 4, 长度 4)
//     // "0001" 表示可见且在测量容差内
//     // 注意：部分设备可能返回其他状态码，这里只处理标准可见
//     if (strncmp(reply + 4, "0001", 4) != 0) return false;

//     int q0, qx, qy, qz;
//     int tx, ty, tz, err;

//     // 解析四元数(Q)和平移(T)以及误差(RMS)
//     // NDI 的数值是定点数，通常需要除以 10000 或 100
//     if (sscanf(reply + 8,
//                "%6d%6d%6d%6d%7d%7d%7d%5d",
//                &q0, &qx, &qy, &qz,
//                &tx, &ty, &tz, &err) != 8) {
//         return false;
//     }

//     out.isVisible = true;
//     out.rmsError  = err / 10000.0;

//     // 构造四元数并归一化
//     Eigen::Quaterniond q(
//         q0 / 10000.0,
//         qx / 10000.0,
//         qy / 10000.0,
//         qz / 10000.0);
    
//     // 构建 4x4 变换矩阵
//     out.matrix = Eigen::Matrix4d::Identity();
//     out.matrix.block<3,3>(0,0) = q.normalized().toRotationMatrix();
    
//     // 转换单位 (NDI 通常是 0.01mm，即 1/100)
//     out.matrix(0,3) = tx / 100.0;
//     out.matrix(1,3) = ty / 100.0;
//     out.matrix(2,3) = tz / 100.0;

//     return true;
// }

// /* =========================================================
//  * Stop & Cleanup
//  * ========================================================= */
// void NDIWorker::stop() {
//     m_keepRunning = false;
    
//     // 给一点时间让 tracking loop 退出
//     QThread::msleep(100);

//     if (device) {
//         qDebug() << "[NDI_WORKER] Stopping device...";
//         ndiCommand(device, NDI_C("TSTOP:"));
//         ndiCloseNetwork(device);
//         device = nullptr;
//     }

//     QMutexLocker locker(&m_mutex);
//     m_tools.clear();
// }