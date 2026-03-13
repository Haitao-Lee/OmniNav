// #ifndef NDIWORKER_H
// #define NDIWORKER_H

// #include <QObject>
// #include <QMap>
// #include <QString>
// #include <QMutex>
// #include <Eigen/Dense>
// #include <vector>
// #include "ndicapi.h"

// struct ToolData {
//     QString name;
//     int handle;
//     Eigen::Matrix4d matrix;
//     double rmsError;
//     bool isVisible;
// };

// class NDIWorker : public QObject {
//     Q_OBJECT

// public:
//     explicit NDIWorker(QObject *parent = nullptr);
//     ~NDIWorker();

// public slots:
//     void connectDevice(QString ipAddress, int port = 8765);
//     void addTools(const QMap<QString, QString>& toolsConfig);
//     void startCollecting();
//     void stop();
//     void purgeSystemErrors();

// signals:
//     void toolUpdated(ToolData data);
//     void statusMessage(QString msg);
//     void errorOccurred(QString err);
//     void onBatchToolsLoaded(QMap<int, QString> tools);

// private:
//     ndicapi* device;
//     bool m_keepRunning;
//     QMutex m_mutex;
//     QMap<int, QString> m_tools;

//     bool parseNDIResponse(const char* reply, ToolData& out);
//     int FPS;
// };

// #endif // NDIWORKER_H