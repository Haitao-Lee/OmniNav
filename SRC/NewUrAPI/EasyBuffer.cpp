#include "EasyBuffer.h"

struct EasyBuffer::impl {
    std::unique_ptr<char[]> buffer;
    size_t capacity = 0;

    void setCapacity(size_t size) {
        auto newBuffer = std::make_unique<char[]>(size);
        if (buffer) {
            std::copy_n(buffer.get(), capacity, newBuffer.get());
        }
        buffer = std::move(newBuffer);
        capacity = size;
    }

    nonstd::span<char> getSpan(size_t startPos, size_t endPos) const {
        if (endPos == -1 || endPos > capacity) {
            endPos = capacity;
        }
        if (startPos < 0) {
            startPos = 0;
        }
        if (startPos >= endPos) {
            return {};
        }
        return {buffer.get() + startPos, buffer.get() + endPos};
    }
};

EasyBuffer::EasyBuffer(size_t initSize)
        : pImpl(std::make_unique<impl>()) {
    if (initSize <= 0) return;
    pImpl->capacity = initSize;
    pImpl->buffer = std::make_unique<char[]>(initSize);
}

void EasyBuffer::Ensure(size_t size) {
    if (size <= pImpl->capacity) return;
#ifdef __GNUG__
    if (__builtin_clz(size) >= 2) {
#else
        if ((size << 1) > 0) {
#endif
        pImpl->setCapacity(size << 1);
    } else {
        pImpl->setCapacity(size);
    }
}

char *EasyBuffer::Data() {
    return pImpl->buffer.get();
}

nonstd::span<char> EasyBuffer::GetSpan(size_t startPos, size_t endPos) {
    return pImpl->getSpan(startPos, endPos);
}

EasyBuffer::~EasyBuffer() =
default;



