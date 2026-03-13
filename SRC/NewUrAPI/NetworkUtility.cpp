#include "NetworkUtility.h"

#include <spdlog/spdlog.h>

#include <QTcpServer>

namespace NetworkUtility {
    class TcpServer : public QTcpServer {
    Q_OBJECT
    protected:
        // QTcpServer 原始的实现会将自己设置 QTcpSocket 的父亲
        // 这会导致 QTcpServer 析构的时候所有的 QTcpSocket 也被析构
        // 因此需要重写这个方法来改变这一行为
        void incomingConnection(qintptr handle) override {
            auto socket = new QTcpSocket();
            socket->setSocketDescriptor(handle);
            addPendingConnection(socket);
            emit newConnection();
        }
    };
}

#include "NetworkUtility.moc"

std::unique_ptr<QTcpSocket> NetworkUtility::NewSocketAsClient(const QString &host, quint16 port) {
    auto socket = std::make_unique<QTcpSocket>();
    socket->connectToHost(host, port);
    bool ok = socket->waitForConnected();
    if (!ok) {
        SPDLOG_ERROR("Failed to connect to server in 30s.");
        return nullptr;
    }
    return socket;
}

std::unique_ptr<QTcpSocket> NetworkUtility::NewSocketAsServer(quint16 listenPort, int timeoutMs) {
    auto server = std::make_unique<TcpServer>();
    bool ok = server->listen(QHostAddress::Any, listenPort);
    if (!ok) {
        SPDLOG_ERROR("Cannot listen on port {}.", listenPort);
        return nullptr;
    }
    ok = server->waitForNewConnection(timeoutMs);
    if (!ok) {
        SPDLOG_ERROR("Error while waiting for incoming connection: {}", qPrintable(server->errorString()));
        return nullptr;
    }
    return std::unique_ptr<QTcpSocket>(server->nextPendingConnection());
}
