#include "QtUtility.h"

#include <QTimer>
#include <QEventLoop>
#include <QMetaObject>

#include <spdlog/spdlog.h>

#include <mutex>

struct QtUtility::ResultWaiter::impl {
    std::any retValue;
    std::mutex mu;
    std::unique_ptr<QTimer> timer;
    std::unique_ptr<QEventLoop> eventLoop;

    void onNotify(std::any value) {
        {
            std::lock_guard lock(mu);
            retValue = std::move(value);
        }
        eventLoop->quit();
    }
};

std::any QtUtility::ResultWaiter::WaitForResult(std::chrono::milliseconds timeoutMs, bool *ok) {
    std::unique_lock lock(pImpl->mu);

    if (pImpl->retValue.has_value()) {
        if (ok) *ok = true;
        return std::move(pImpl->retValue);
    }

    pImpl->timer->start(timeoutMs);

    QMetaObject::invokeMethod(this, [&]() {
        pImpl->mu.unlock();
    });
    lock.release();
    pImpl->eventLoop->exec();

    if (pImpl->timer->isActive()) {
        pImpl->timer->stop();
        if (ok) *ok = true;
    } else {
        SPDLOG_WARN("Failed to get result, timeout occurred.");
        if (ok) *ok = false;
    }

    return std::move(pImpl->retValue);
}

QtUtility::ResultWaiter::ResultWaiter() :
        pImpl(std::make_unique<impl>()) {
    pImpl->timer = std::make_unique<QTimer>(this);
    pImpl->timer->setSingleShot(true);
    pImpl->eventLoop = std::make_unique<QEventLoop>(this);
    using namespace std::placeholders;
    QObject::connect(this, &ResultWaiter::Notify,
                     [&](std::any value) { pImpl->onNotify(std::move(value)); });
    QObject::connect(pImpl->timer.get(), &QTimer::timeout,
                     pImpl->eventLoop.get(), &QEventLoop::quit);
}

void QtUtility::ResultWaiter::Reset() {
    std::lock_guard lock(pImpl->mu);
    pImpl->retValue.reset();
}

QtUtility::ResultWaiter::~ResultWaiter() = default;
