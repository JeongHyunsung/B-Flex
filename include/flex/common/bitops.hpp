#pragma once 
#include <cstdint>
#include <type_traits>

namespace flex{

inline int popcnt_u32(std::uint32_t x) {
#if defined(__GNUG__) || defined(__clang__)
    return __builtin_popcount(x);
#else
    int c = 0;
    while (x) { x &= (x - 1); ++c; }
    return c;
#endif
}

inline int popcnt_u64(std::uint64_t x) {
#if defined(__GNUG__) || defined(__clang__)
    return __builtin_popcountll(x);
#else
    int c = 0;
    while (x) { x &= (x - 1); ++c; }
    return c;
#endif
}

template <class T>
inline int popcnt(T x){
    static_assert(std::is_unsigned_v<T>, "popcnt expects an unsigned integer type");
    if constexpr (sizeof(T) <= 4) {
        return popcnt_u32(static_cast<std::uint32_t>(x));
    } else return popcnt_u64(static_cast<std::uint64_t>(x));   
}

template <class T>
inline int hamming(T a, T b){
    static_assert(std::is_unsigned_v<T>, "hamming expects an unsigned integer type");
    return popcnt(static_cast<T>(a ^ b));
}

}