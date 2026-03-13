#include "IOUtility.h"
#include "ByteOrderConvert.hpp"
#include "EasyDataIO.h"
#include "scope_guard.hpp"

#include <QAbstractSocket>

#include <spdlog/spdlog.h>

#include <utility>

namespace IOUtility {

    nonstd::span<char> ReadPacket(DataStream &dataStream, EasyBuffer &buffer) {
        uint16_t packetLength = -1;
//    dataStream >> packetLength;
        { // 替代上面那句话
            static thread_local char packetLengthBuf[2];
            auto ret = dataStream.readRawData(packetLengthBuf, 2);
            if (ret != 2) {
                SPDLOG_WARN("Failed to get length of packet.");
                return {};
            }
            packetLength = net2loc<uint16_t>({packetLengthBuf, packetLengthBuf + 2});
        }

        packetLength -= 2;
        buffer.Ensure(packetLength);
        auto ret = dataStream.readRawData(buffer.Data(), packetLength);
        if (ret != packetLength) return {};

        return buffer.GetSpan(0, packetLength);
    }

    nonstd::span<char> ReadPacket(QIODevice *device, EasyBuffer &buffer) {
        uint16_t packetLength = -1;

        // 获取报文长度
        static thread_local char packetLengthBuf[2];
        auto ret = TryReadAll(device, packetLengthBuf, 2);
        if (!ret) [[unlikely]] {
            SPDLOG_WARN("Failed to get length of packet.");
            return {};
        }
        packetLength = net2loc<uint16_t>({packetLengthBuf, packetLengthBuf + 2});

        packetLength -= 2;
        buffer.Ensure(packetLength);
        ret = TryReadAll(device, buffer.Data(), packetLength);
        if (!ret) [[unlikely]] {
            SPDLOG_WARN("Failed to read packet.");
            return {};
        }

        return buffer.GetSpan(0, packetLength);
    }

    bool WritePacket(DataStream &dataStream, nonstd::span<const char> dataSpan) {
        static thread_local EasyBuffer buffer(1024);
        static thread_local EasyDataIO dio;

        auto packetLength = dataSpan.size() + 2;
        buffer.Ensure(packetLength);
        dio.SetBuffer(buffer.GetSpan());

        dio.Write((uint16_t) packetLength);
        dio.WriteRawBytes(dataSpan);

        return dataStream.writeRawData(buffer.Data(), dio.GetWritePos()) != -1;
    }

    bool WritePacket(QIODevice *device, nonstd::span<const char> dataSpan) {
        static thread_local EasyBuffer buffer(1024);
        static thread_local EasyDataIO dio;

        auto packetLength = dataSpan.size() + 2;
        buffer.Ensure(packetLength);
        dio.SetBuffer(buffer.GetSpan());

        dio.Write((uint16_t) packetLength);
        dio.WriteRawBytes(dataSpan);

        return TryWriteAll(device, buffer.Data(), dio.GetWritePos());
    }

    bool TryReadAll(QIODevice *device, char *data, size_t len, size_t *out, bool ableToWait) {
        size_t cur = 0, ret;
        auto closer = sg::make_scope_guard([&]() { if (out) *out = cur; });
        while (cur != len) {
            if (device->bytesAvailable() <= 0 && ableToWait) { // TODO: 增加当不支持 wait 时，手动等待 readyRead() 的功能
                auto ok = device->waitForReadyRead(-1);
                if (!ok) [[unlikely]] {
                    SPDLOG_WARN("Failed while waiting for more data to read: {}",
                                qPrintable(device->errorString()));
                    return false;
                }
            }
            ret = device->read(data + cur, len - cur);
            if (ret <= 0) [[unlikely]] {
                SPDLOG_WARN("Failed to read data from data stream: {}",
                            qPrintable(device->errorString()));
                return false;
            }
            cur += ret;
        }
        return true;
    }

    bool TryWriteAll(QIODevice *device, const char *data, size_t len, size_t *out, bool ableToWait) {
        size_t cur = 0, ret;
        auto closer = sg::make_scope_guard([&]() { if (out) *out = cur; });
        while (cur != len) {
            if (device->bytesToWrite() > 0 && ableToWait) {
                auto ok = device->waitForBytesWritten(-1);
                if (!ok) [[unlikely]] {
                    SPDLOG_WARN("Failed while waiting for cur data to be written: {}",
                                qPrintable(device->errorString()));
                    return false;
                }
            }
            ret = device->write(data + cur, len - cur);
            if (ret <= 0) [[unlikely]] {
                SPDLOG_WARN("Failed to write data to data stream: {}", qPrintable(device->errorString()));
                return true;
            }
            cur += ret;
        }
        return true;
    }

    void TryFlushIODevice(QIODevice *device) {
        if (auto real = dynamic_cast<QAbstractSocket *>(device)) {
            bool ret = real->flush();
            if (!ret) {
                SPDLOG_WARN("No data to send. flush() returned false.");
            }
        }
    }

    nonstd::span<char> JoinAndWrite(const std::vector<std::string> &strList, char connector, EasyBuffer &buffer) {
        static thread_local EasyDataIO dio;
        size_t totalLength = strList.size() - 1;
        for (const auto &str: strList) {
            totalLength += str.length();
        }
        buffer.Ensure(totalLength);
        dio.SetBuffer(buffer.GetSpan());
        for (auto iter = strList.begin(); iter != strList.end(); ++iter) {
            nonstd::span<const char> data{iter->data(), iter->data() + iter->length()};
            dio.WriteRawBytes(data);
            if (iter + 1 != strList.end()) {
                dio.Write(connector);
            }
        }
        return buffer.GetSpan(0, totalLength);
    }
}