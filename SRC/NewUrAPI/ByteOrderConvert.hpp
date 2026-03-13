#ifndef SOPHIAR_BYTEORDERCONVERT_HPP
#define SOPHIAR_BYTEORDERCONVERT_HPP

#include <algorithm>
#include <type_traits>
#include <span>
#define nonstd std

namespace ByteOrderConvertInner {
    inline uint16_t test_num = 0x1234;
    inline bool is_big_endian = (*(char *) &test_num == 0x12);
}

template<typename T, typename = std::enable_if_t<std::is_fundamental<T>::value>>
inline void swap_byte_order(T &value) {
#ifdef __GNUG__
    switch (sizeof(T)) {
        case 1:
            break;
        case 2: {
            static thread_local uint16_t locValue;
            locValue = __builtin_bswap16(*(uint16_t *) &value);
            value = *(T *) &locValue;
            break;
        }
        case 4: {
            static thread_local uint32_t locValue;
            locValue = __builtin_bswap32(*(uint32_t *) &value);
            value = *(T *) &locValue;
            break;
        }
        case 8: {
            static thread_local uint64_t locValue;
            locValue = __builtin_bswap64(*(uint64_t *) &value);
            value = *(T *) &locValue;
            break;
        }
        default: {
            auto pStart = (char *) &value;
            auto pEnd = pStart + sizeof(T);
            std::reverse(pStart, pEnd);
            break;
        }
    }
#else
    auto pStart = (char *) &value;
    auto pEnd = pStart + sizeof(T);
    std::reverse(pStart, pEnd);
#endif
}

template<typename T, typename = std::enable_if_t<std::is_fundamental<T>::value>>
inline T net2loc(nonstd::span<const char> bytes_in) {
    auto value = *(T *) bytes_in.data();
    if (!ByteOrderConvertInner::is_big_endian)
        swap_byte_order<T>(value);
    return value;
}

template<typename T, typename = std::enable_if_t<std::is_fundamental<T>::value>>
inline void loc2net(T value, nonstd::span<char> bytes_out) {
    if (!ByteOrderConvertInner::is_big_endian)
        swap_byte_order<T>(value);
    *(T *) bytes_out.data() = value;
}

#endif //SOPHIAR_BYTEORDERCONVERT_HPP
