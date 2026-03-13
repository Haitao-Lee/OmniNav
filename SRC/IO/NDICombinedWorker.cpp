#include "NDICombinedWorker.h"
#include <QDebug>
#include <cstdio>

NDICombinedWorker::NDICombinedWorker(QObject *parent)
    : QObject(parent),
      m_isConnected(false),
      FPS(60),
      m_timer(nullptr),
      m_monitorTimer(nullptr)
{
    m_monitorTimer = new QTimer(this);
    connect(m_monitorTimer, &QTimer::timeout, this, &NDICombinedWorker::monitorConnection);
}

NDICombinedWorker::~NDICombinedWorker() {
    disconnectDevice();
}

void NDICombinedWorker::connectDevice(QString ipAddress) {
    if (m_isConnected) {
        emit statusMessage("Already connected.");
        return;
    }

    emit statusMessage("Connecting to " + ipAddress + "...");

    int ret = capi.connect(ipAddress.toStdString());
    if (ret != 0) {
        emit errorOccurred(QString("Connection failed. Error code: %1").arg(ret));
        return;
    }

    emit statusMessage("Initializing System...");
    ret = capi.initialize();
    if (ret < 0) {
        emit errorOccurred("System Initialization Failed! Check connection.");
        return;
    }

    m_isConnected = true;
    
    m_dataWatchdog.start(); 
    m_monitorTimer->start(1000);
    
    emit statusMessage("Connected & Initialized via Combined API");
}

void NDICombinedWorker::disconnectDevice() {
    if (m_monitorTimer) m_monitorTimer->stop(); 

    stopTracking();

    if (m_isConnected) {
        m_isConnected = false;
        
        {
            QMutexLocker locker(&m_mutex);
            m_handleMap.clear();
        }
        
        emit statusMessage("Device Disconnected");
    }
}

void NDICombinedWorker::monitorConnection() {
    if (m_timer && m_timer->isActive()) {
        if (m_dataWatchdog.elapsed() > 2000) {
             qWarning() << "[CAPI] Watchdog Timeout! No data for 2s. Assuming Disconnection.";
             disconnectDevice();
             emit errorOccurred("Connection Lost: Data Timeout (Cable Unplugged?)");
        }
        return;
    }

    int ph = capi.portHandleRequest();

    if (ph >= 0) {
        char buffer[8];
        std::sprintf(buffer, "%02X", ph);
        std::string portHandleStr(buffer);
        capi.portHandleFree(portHandleStr);
        
        m_dataWatchdog.restart(); 
    } else {
        qWarning() << "[CAPI] Keep-alive Ping failed. Error:" << ph;
        disconnectDevice();
        emit errorOccurred("Connection Lost: Device Unplugged (Idle).");
    }
}

void NDICombinedWorker::setTools(const QMap<QString, QString>& toolsConfig) {
    if (!m_isConnected) {
        emit errorOccurred("Device not connected");
        return;
    }

    if (toolsConfig.isEmpty()) {
        emit errorOccurred("No tools configuration provided.");
        return;
    }

    freeAllPorts();

    QMap<int, QString> loadedTools;
    QMapIterator<QString, QString> it(toolsConfig);
    
    while (it.hasNext()) {
        it.next();
        QString name = it.key();
        QString romPath = it.value();

        emit statusMessage("Loading ROM: " + name);

        int portHandle = capi.portHandleRequest();
        if (portHandle < 0) {
            emit errorOccurred("PHRQ Failed for " + name);
            continue;
        }

        char buffer[8];
        std::sprintf(buffer, "%02X", portHandle); 
        std::string portHandleStr(buffer); 

        capi.loadSromToPort(romPath.toStdString().c_str(), portHandle); 
        capi.portHandleInitialize(portHandleStr);
        capi.portHandleEnable(portHandleStr);

        loadedTools.insert(portHandle, name);
        qDebug() << "[CAPI] Loaded" << name << "at Port" << portHandle;
    }

    if (loadedTools.isEmpty()) {
        emit errorOccurred("No tools were loaded successfully!");
        return;
    }

    {
        QMutexLocker locker(&m_mutex);
        m_handleMap = loadedTools;
    }

    emit onBatchToolsLoaded(loadedTools);
    emit statusMessage(QString("Batch load complete. %1 tools ready.").arg(loadedTools.size()));
}

void NDICombinedWorker::startCollecting() {
    if (!m_isConnected) return;
    if (m_timer && m_timer->isActive()) return;

    if (!m_timer) {
        m_timer = new QTimer(this);
        connect(m_timer, &QTimer::timeout, this, &NDICombinedWorker::doTrackingWork);
    }

    int ret = capi.startTracking();
    if (ret < 0) {
        emit errorOccurred("Failed to Start Tracking");
        return;
    }

    m_dataWatchdog.restart();
    
    qDebug() << "[CAPI] Tracking started.";
    m_timer->start(1000 / FPS);
    emit statusMessage("Tracking started");
}

void NDICombinedWorker::doTrackingWork() {
    std::vector<ToolData> toolDataList = capi.getTrackingDataBX();

    QMutexLocker locker(&m_mutex);

    if (toolDataList.empty()) {
        if (!m_handleMap.isEmpty()) {
            qWarning() << "[CAPI] CRITICAL: Empty data received during tracking!";
            locker.unlock(); 
            disconnectDevice();
            emit errorOccurred("Connection Lost: API returned empty (Tracking).");
            return;
        }
    }
    
    m_dataWatchdog.restart(); 
    
    QVector<TrackedTool> frameData; 
    frameData.reserve(toolDataList.size());

    for (const auto& tool : toolDataList) {
        int handle = tool.transform.toolHandle;

        if (m_handleMap.contains(handle)) {
            TrackedTool data;
            data.name = m_handleMap[handle];
            data.portHandle = handle;
            
            if (tool.transform.isMissing()) {
                data.isVisible = false;
                data.error = 0.0;
                data.matrix = Eigen::Matrix4d::Identity();
            } else {
                data.isVisible = true;
                data.error = tool.transform.error;

                Eigen::Quaterniond q(
                    tool.transform.q0, 
                    tool.transform.qx, 
                    tool.transform.qy, 
                    tool.transform.qz
                );
                
                data.matrix = Eigen::Matrix4d::Identity();
                data.matrix.block<3,3>(0,0) = q.normalized().toRotationMatrix();
                
                data.matrix(0,3) = tool.transform.tx;
                data.matrix(1,3) = tool.transform.ty;
                data.matrix(2,3) = tool.transform.tz;
            }

            frameData.append(data);
        }
    }

    if (!frameData.isEmpty()) {
        emit allToolsUpdated(frameData);
    }
}

void NDICombinedWorker::freeAllPorts() {
    QMutexLocker locker(&m_mutex);
    if (m_handleMap.isEmpty()) return;
    qDebug() << "[CAPI] Freeing existing port handles...";
    QMapIterator<int, QString> it(m_handleMap);
    while (it.hasNext()) {
        it.next();
        int handle = it.key();
        char buffer[8];
        std::sprintf(buffer, "%02X", handle);
        std::string portHandleStr(buffer);
        capi.portHandleFree(portHandleStr); 
    }
    m_handleMap.clear();
}

void NDICombinedWorker::stopTracking() {
    if (m_timer) m_timer->stop();
    if (m_isConnected) {
        capi.stopTracking();
        emit statusMessage("Tracking stopped");
    }
}