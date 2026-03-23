#ifndef SOPHIAR_EASYDATAIO_H
#define SOPHIAR_EASYDATAIO_H

#include "ByteOrderConvert.hpp"

#include <span>
#include <memory>
#define nonstd std

// Not thread-safe.
// Read and write have separate position pointers.
class EasyDataIO {
public:
    explicit EasyDataIO(nonstd::span<char> buffer = {});

    ~EasyDataIO();

    // Resets the read/write positions.
    void SetBuffer(nonstd::span<char> buffer);

    // Reset the read/write positions.
    void Reset();

    // Set the current read position.
    void SetReadPos(size_t pos);

    size_t GetReadPos();

    // Set the current write position.
    void SetWritePos(size_t pos);

    size_t GetWritePos();

    // Get the total length of the current buffer.
    size_t Size();

    // Return false on failure.
    bool ReadRawBytes(nonstd::span<char> buffer);

    bool WriteRawBytes(nonstd::span<const char> buffer);

    // Return false if the input buffer is too small.
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

