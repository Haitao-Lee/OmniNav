#ifndef SOPHIAR_EASYDATAIO_H
#define SOPHIAR_EASYDATAIO_H

#include "ByteOrderConvert.hpp"

#include <span>
#include <memory>
#define nonstd std

// 不支持多线程
// 读写有各自的位置指针
class EasyDataIO {
public:
    explicit EasyDataIO(nonstd::span<char> buffer = {});

    ~EasyDataIO();

    // 会重置读写指针的位置
    void SetBuffer(nonstd::span<char> buffer);

    // 重置读写指针的位置
    void Reset();

    // 读写当前读指针的位置
    void SetReadPos(size_t pos);

    size_t GetReadPos();

    // 读写当前写指针的位置
    void SetWritePos(size_t pos);

    size_t GetWritePos();

    // 获取当前持有 buffer 的总长度
    size_t Size();

    // 失败则返回 false
    bool ReadRawBytes(nonstd::span<char> buffer);

    bool WriteRawBytes(nonstd::span<const char> buffer);

    // 如果输入的 buffer 空间不够就会返回 false
    bool ReadRestBytes(nonstd::span<char> buffer);

    template<typename T>
    inline bool Read(T &value) {
        static thread_local char locBytes[sizeof(T)];
        static thread_local nonstd::span<char> locBuffer{locBytes, locBytes + sizeof(T)};
        auto ok = ReadRawBytes(locBuffer);
        if (!ok) return false;
        value = net2loc<T>(locBuffer);
        return true;
    }

    template<typename T>
    inline bool Write(T value) {
        static thread_local char locBytes[sizeof(T)];
        static thread_local nonstd::span<char> locBuffer{locBytes, locBytes + sizeof(T)};
        loc2net(value, locBuffer);
        return WriteRawBytes(locBuffer);
    }

private:
    struct impl;
    std::shared_ptr<impl> pImpl;
};


#endif //SOPHIAR_EASYDATAIO_H
