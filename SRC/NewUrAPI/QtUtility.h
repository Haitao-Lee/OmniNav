#ifndef SOPHIAR_QTUTILITY_H
#define SOPHIAR_QTUTILITY_H

#include <QObject>

#include <chrono>
#include <memory>
#include <any>

using namespace std::chrono_literals;

namespace QtUtility {

    // 消费者调用 WaitForResult 等待结果
    // 生产者调用 Notify 通知结果
    class ResultWaiter : public QObject {
    Q_OBJECT
    public:
        ResultWaiter();

        ~ResultWaiter() override;

        std::any WaitForResult(std::chrono::milliseconds timeoutMs = 500ms, bool *ok = nullptr);

        void Reset();

    signals:

        void Notify(std::any value);

    private:
        struct impl;
        std::unique_ptr<impl> pImpl;
    };

}


#endif //SOPHIAR_QTUTILITY_H
