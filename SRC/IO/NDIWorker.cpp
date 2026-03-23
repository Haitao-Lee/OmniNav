// #include "NDIWorker.h"

// #include <QThread>
// #include <QDebug>
// #include <QElapsedTimer>
// #include <QFile>
// #include <QMutexLocker>
// #include <cstdio>
// #include <vector>
// #include <algorithm>

// // Compatibility helper for ndicapi const char* conversion.
// #define NDI_C(s) const_cast<char*>(s)

// NDIWorker::NDIWorker(QObject *parent)
//     : QObject(parent),
//       device(nullptr),
//       m_keepRunning(false),
//       FPS(60) // Default 60 Hz sampling.
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

//     // After a successful connection, send TSTOP to quiet the device and avoid stale data.
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
//     // ★★★ 2. Key change: force-purge the error stack (mimic Combined API). ★★★
//     // ============================================================
//     // If the device shows an orange warning, read all errors to unlock PHRQ.
//     qDebug() << "[NDI] Purging system errors/warnings...";
    
//     for (int i = 0; i < 10; ++i) {
//         // Read errors (SEER:0).
//         const char* errResp = ndiCommand(device, NDI_C("SEER:0"));
//         // Read status (SSTAT:0) - important for clearing the "Status Changed" flag.
//         ndiCommand(device, NDI_C("SSTAT:0"));
        
//         // If it returns 0000, the error stack is empty and we can stop.
//         if (errResp && strstr(errResp, "0000")) {
//             qDebug() << "[NDI] Error stack cleared.";
//             break;
//         }
//         // If there is an error, log it and keep reading.
//         if (errResp) qDebug() << "[NDI] Ack Error:" << errResp;
//         QThread::msleep(50);
//     }
//     // Give the status a bit more time to reset.
//     QThread::msleep(200);
//     // ============================================================

//     // 3. Preventive cleanup (PHF).
//     for (int h = 1; h <= 20; ++h) {
//         char cmd[16];
//         sprintf(cmd, "PHF:%02X", h);
//         ndiCommand(device, cmd);
//         QThread::msleep(20);
//     }
//     QThread::msleep(100);

//     // 4. Load tools.
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

//         // --- Request a handle (PHRQ). ---
//         const char* phrqResp = ndiCommand(device, NDI_C("PHRQ:********"));
        
//         // If the purge succeeded, we should be able to get a handle.
//         if (!phrqResp || strstr(phrqResp, "ERROR")) {
//             // If it still errors after purging, log the error.
//             emit errorOccurred(QString("PHRQ failed for %1: %2").arg(toolName).arg(phrqResp));
//             continue;
//         }

//         unsigned int handle = 0;
//         sscanf(phrqResp, "%02X", &handle);

//         // --- Upload ROM (PVWR). ---
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
//         if (!resp || strstr(resp, "0000")) break; // No more errors.
//         QThread::msleep(50);
//     }
    
//     // Clear any stuck Warning flags.
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

//     // Compute the frame interval (ms) from FPS.
//     const int interval = 1000 / (FPS > 0 ? FPS : 60);
//     QElapsedTimer timer;

//     while (m_keepRunning) {
//         timer.restart();

//         // Check device connection status.
//         if (!device) {
//             emit errorOccurred("Connection lost during tracking.");
//             break;
//         }

//         // Copy the current tool list to minimize lock hold time.
//         QMap<int, QString> toolsSnapshot;
//         {
//             QMutexLocker locker(&m_mutex);
//             toolsSnapshot = m_tools;
//         }

//         // Query each tool (GX).
//         for (auto it = toolsSnapshot.begin(); it != toolsSnapshot.end(); ++it) {
//             if (!m_keepRunning) break;

//             char cmd[16];
//             sprintf(cmd, "GX:%02X", it.key());

//             const char* reply = ndiCommand(device, cmd);

//             ToolData data;
//             data.handle = it.key();
//             data.name   = it.value();

//             // Parse the GX response.
//             if (!parseNDIResponse(reply, data)) {
//                 data.isVisible = false;
//             }

//             emit toolUpdated(data);
//         }

//         // Frame-rate control.
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
//     // Basic validation: empty or too short.
//     if (!reply || strlen(reply) < 30) return false;

//     // The GX reply format is typically:
//     // HHHH SSSS Q0Q0Q0 QxQxQx QyQyQy QzQzQz TxTxTx TyTyTy TzTzTz EEEE ...
//     // HHHH: handle, SSSS: status (0000=Missing, 0001=OK).
    
//     // Check the status field (offset 4, length 4).
//     // "0001" means visible and within measurement tolerance.
//     // Note: some devices may return other status codes; only standard visible is handled here.
//     if (strncmp(reply + 4, "0001", 4) != 0) return false;

//     int q0, qx, qy, qz;
//     int tx, ty, tz, err;

//     // Parse quaternion (Q), translation (T), and error (RMS).
//     // NDI values are fixed-point and usually need division by 10000 or 100.
//     if (sscanf(reply + 8,
//                "%6d%6d%6d%6d%7d%7d%7d%5d",
//                &q0, &qx, &qy, &qz,
//                &tx, &ty, &tz, &err) != 8) {
//         return false;
//     }

//     out.isVisible = true;
//     out.rmsError  = err / 10000.0;

//     // Construct and normalize the quaternion.
//     Eigen::Quaterniond q(
//         q0 / 10000.0,
//         qx / 10000.0,
//         qy / 10000.0,
//         qz / 10000.0);
    
//     // Build the 4x4 transform matrix.
//     out.matrix = Eigen::Matrix4d::Identity();
//     out.matrix.block<3,3>(0,0) = q.normalized().toRotationMatrix();
    
//     // Convert units (NDI is typically 0.01 mm, i.e., 1/100).
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
    
//     // Give the tracking loop time to exit.
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

