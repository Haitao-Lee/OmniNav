#ifndef SOPHIAR_DATASTREAM_HPP
#define SOPHIAR_DATASTREAM_HPP

#include <QDataStream>

#include <spdlog/spdlog.h>

// The original QDataStream does not guarantee full reads/writes.
// Here we change this behavior: read/write RawData either succeeds or returns -1.
// PS: Do not use >>; the built-in QDataStream implementation is unreliable.
class DataStream : public QDataStream {
public:
    using QDataStream::QDataStream;

    int readRawData(char *s, int len) {
        auto device = QDataStream::device();
        int cur = 0, ret;
        while (cur != len) {
            if (device->bytesAvailable() <= 0) {
                device->waitForReadyRead(-1); // TODO: In the default QIODevice implementation, this always returns -1; consider deprecating this class.
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

