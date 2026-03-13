#ifndef SOPHIAR_NETWORKUTILITY_H
#define SOPHIAR_NETWORKUTILITY_H

#include <QTcpSocket>
#include <QString>

#include <memory>

namespace NetworkUtility {
    std::unique_ptr<QTcpSocket> NewSocketAsClient(const QString &host, quint16 port);

    std::unique_ptr<QTcpSocket> NewSocketAsServer(quint16 listenPort, int timeoutMs = 5000);
}


#endif //SOPHIAR_NETWORKUTILITY_H
