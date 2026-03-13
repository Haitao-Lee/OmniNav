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

    // 返回的 span 一定是 buffer 中的一部分
    // 出错时返回的 Data() 是 nullptr
    nonstd::span<char> ReadPacket(DataStream &dataStream, EasyBuffer &buffer);

    nonstd::span<char> ReadPacket(QIODevice *device, EasyBuffer &buffer);

    nonstd::span<char> ReadPacketAndCheckCRC32(DataStream &dataStream, EasyBuffer &buffer);

    bool WritePacket(DataStream &dataStream, nonstd::span<const char> dataSpan);

    bool WritePacket(QIODevice *device, nonstd::span<const char> dataSpan);

    bool WritePacketAndCRC32(DataStream &dataStream, nonstd::span<const char> dataSpan);

    void TryFlushIODevice(QIODevice *device);

    // 返回 true 表示全部读完, out 中填充实际读取的长度
    // 如果传入的 device 中 waitForReadyRead 永远返回 false, 那么最后一个参数应该填 false
    bool TryReadAll(QIODevice *device, char *data, size_t len, size_t *out = nullptr, bool ableToWait = true);

    bool TryWriteAll(QIODevice *device, const char *data, size_t len, size_t *out = nullptr, bool ableToWait = true);

    // 类似其他语言中字符串的 join
    // TODO: replace with absl::StrJoin()
    nonstd::span<char> JoinAndWrite(const std::vector<std::string> &strList, char connector, EasyBuffer &buffer);
}


#endif //SOPHIAR_IOUTILITY_H
