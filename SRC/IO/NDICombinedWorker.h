#ifndef NDICOMBINEDWORKER_H
#define NDICOMBINEDWORKER_H

#include <QObject>
#include <QMap>
#include <QString>
#include <QMutex>
#include <QTimer>
#include <QVector> 
#include <QElapsedTimer>
#include <Eigen/Dense>
#include "CombinedApi.h"
#include "ToolData.h"

struct TrackedTool {
    QString name;
    int portHandle;
    Eigen::Matrix4d matrix;
    double error;
    bool isVisible;
};

Q_DECLARE_METATYPE(TrackedTool)
Q_DECLARE_METATYPE(QVector<TrackedTool>)

class NDICombinedWorker : public QObject {
    Q_OBJECT

public:
    explicit NDICombinedWorker(QObject *parent = nullptr);
    ~NDICombinedWorker();

public slots:
    void connectDevice(QString ipAddress);
    void disconnectDevice();
    void setTools(const QMap<QString, QString>& toolsConfig);
    void startCollecting();
    void stopTracking();

private slots:
    void doTrackingWork();
    void monitorConnection(); 

signals:
    void allToolsUpdated(const QVector<TrackedTool>& tools);
    void statusMessage(QString msg);
    void errorOccurred(QString err);
    void onBatchToolsLoaded(QMap<int, QString> tools);

private:
    void freeAllPorts();

    CombinedApi capi;
    bool m_isConnected;
    QMutex m_mutex;
    QMap<int, QString> m_handleMap;
    int FPS;
    QTimer* m_timer;         
    QTimer* m_monitorTimer;  
    QElapsedTimer m_dataWatchdog;
};

#endif // NDICOMBINEDWORKER_H