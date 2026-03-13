#include "EasyDataIO.h"

struct EasyDataIO::impl {
    nonstd::span<char> buffer;
    size_t readPos = 0, writePos = 0;

    bool readRawBytes(nonstd::span<char> buffer_out) {
        if (readPos + buffer_out.size() > buffer.size()) return false;
        std::copy_n(buffer.data() + readPos, buffer_out.size(), buffer_out.data());
        readPos += buffer_out.size();
        return true;
    }

    bool writeRawBytes(nonstd::span<const char> buffer_in) {
        if (writePos + buffer_in.size() > buffer.size()) return false;
        std::copy_n(buffer_in.data(), buffer_in.size(), buffer.data() + writePos);
        writePos += buffer_in.size();
        return true;
    }

    bool readRestBytes(nonstd::span<char> buffer_out) {
        if (readPos + buffer_out.size() < buffer.size()) return false;
        std::copy(buffer.data() + readPos, buffer.data() + buffer.size(), buffer_out.data());
        readPos = buffer.size();
        return true;
    }
};

EasyDataIO::EasyDataIO(nonstd::span<char> buffer)
        : pImpl(std::make_shared<impl>()) {
    SetBuffer(buffer);
}

void EasyDataIO::SetBuffer(nonstd::span<char> buffer) {
    pImpl->buffer = buffer;
    Reset();
}

void EasyDataIO::Reset() {
    pImpl->readPos = 0;
    pImpl->writePos = 0;
}

void EasyDataIO::SetReadPos(size_t pos) {
    pImpl->readPos = pos;
}

size_t EasyDataIO::GetReadPos() {
    return pImpl->readPos;
}

void EasyDataIO::SetWritePos(size_t pos) {
    pImpl->writePos = pos;
}

size_t EasyDataIO::GetWritePos() {
    return pImpl->writePos;
}

size_t EasyDataIO::Size() {
    return pImpl->buffer.size();
}

bool EasyDataIO::ReadRawBytes(nonstd::span<char> buffer) {
    return pImpl->readRawBytes(buffer);
}

bool EasyDataIO::WriteRawBytes(nonstd::span<const char> buffer) {
    return pImpl->writeRawBytes(buffer);
}

bool EasyDataIO::ReadRestBytes(nonstd::span<char> buffer) {
    return pImpl->readRestBytes(buffer);
}

EasyDataIO::~EasyDataIO() = default;



