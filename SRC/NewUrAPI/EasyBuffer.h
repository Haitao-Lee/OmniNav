#ifndef SOPHIAR_EASYBUFFER_H
#define SOPHIAR_EASYBUFFER_H

#include <memory>
#include <span>
#define nonstd std

class EasyBuffer {
public:
    explicit EasyBuffer(size_t initSize);

    ~EasyBuffer();

    void Ensure(size_t size);

    // 返回当前所持有的缓存起始地址
    char *Data();

    nonstd::span<char> GetSpan(size_t startPos = 0, size_t endPos = -1);

private:
    struct impl;
    std::unique_ptr<impl> pImpl;
};


#endif //SOPHIAR_EASYBUFFER_H
