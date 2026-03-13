#ifndef SOPHIAR_DATASTREAM_HPP
#define SOPHIAR_DATASTREAM_HPP

#include <QDataStream>

#include <spdlog/spdlog.h>

// 原始的 QDataStream 不保证读写一定全部完成
// 这里改变这一行为，读写 RawData 的操作要么成功，要么返回-1
// PS: 不要尝试使用 >> , QDataStream 自带的实现不靠谱
class DataStream : public QDataStream {
public:
    using QDataStream::QDataStream;

    int readRawData(char *s, int len) {
        auto device = QDataStream::device();
        int cur = 0, ret;
        while (cur != len) {
            if (device->bytesAvailable() <= 0) {
                device->waitForReadyRead(-1); // TODO: QIODevice 的默认实现中该函数永远返回 -1，考虑弃用该类
            }
            ret = QDataStream::readRawData(s + cur, len - cur);
            if (ret <= 0) [[unlikely]] {
                SPDLOG_WARN("Failed to read data from data stream: {}", qPrintable(device->errorString()));
                return -1;
            }
            cur += ret;
        }
        return len;
    }

    int writeRawData(const char *s, int len) {
        auto device = QDataStream::device();
        int cur = 0, ret;
        while (cur != len) {
            if (device->bytesToWrite() > 0) {
                device->waitForBytesWritten(-1);
            }
            ret = QDataStream::writeRawData(s + cur, len - cur);
            if (ret <= 0) [[unlikely]] {
                SPDLOG_WARN("Failed to write data to data stream: {}", qPrintable(device->errorString()));
                return -1;
            }
            cur += ret;
        }
        return len;
    };
};

#endif //SOPHIAR_DATASTREAM_HPP
