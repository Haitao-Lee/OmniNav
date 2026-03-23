#ifndef SOPHIAR_IOUTILITY_H
#define SOPHIAR_IOUTILITY_H

#include "EasyBuffer.h"
#include "DataStream.hpp"

#include <QDataStream>
#include <QIODevice>


#include <memory>
#include <span>
#define nonstd std

namespace IOUtility {

    // The returned span is always a slice of the buffer.
    // On error, Data() returns nullptr.
    nonstd::span<char> ReadPacket(DataStream &dataStream, EasyBuffer &buffer);

    nonstd::span<char> ReadPacket(QIODevice *device, EasyBuffer &buffer);

    nonstd::span<char> ReadPacketAndCheckCRC32(DataStream &dataStream, EasyBuffer &buffer);

    bool WritePacket(DataStream &dataStream, nonstd::span<const char> dataSpan);

    bool WritePacket(QIODevice *device, nonstd::span<const char> dataSpan);

    bool WritePacketAndCRC32(DataStream &dataStream, nonstd::span<const char> dataSpan);

    void TryFlushIODevice(QIODevice *device);

    // Return true when all data is read; out is filled with the actual length.
    // If device->waitForReadyRead always returns false, pass false for the last parameter.
    bool TryReadAll(QIODevice *device, char *data, size_t len, size_t *out = nullptr, bool ableToWait = true);

    bool TryWriteAll(QIODevice *device, const char *data, size_t len, size_t *out = nullptr, bool ableToWait = true);

    // Similar to string join in other languages.
    // TODO: replace with absl::StrJoin()
    nonstd::span<char> JoinAndWrite(const std::vector<std::string> &strList, char connector, EasyBuffer &buffer);
}


#endif //SOPHIAR_IOUTILITY_H

