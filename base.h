#pragma once
#ifndef MEIKA_BASE
#define MEIKA_BASE

#include <cstdlib>
#include <cstring>
#include <utility>

namespace meika {
using real = double;
inline auto parseFloat(const char *s, char **e, float *) -> float {
    return std::strtof(s, e);
}
inline auto parseFloat(const char *s, char **e, double *) -> double {
    return std::strtod(s, e);
}
inline auto parseFloat(const char *s, char **e, long double *) -> long double {
    return std::strtold(s, e);
}

inline auto toInt(const char *s, char **endptr) -> int {
    return static_cast<int>(std::strtol(s, endptr, 10)); // 10进制
}

inline auto toInt(const char *s) -> int { return toInt(s, nullptr); }

inline auto toInt(const char *s, std::size_t n) -> int {
    char temp[16];
    std::size_t len = (n < 15) ? n : 15;
    std::memcpy(temp, s, len);
    temp[len] = '\0';
    return toInt(temp);
}

template <typename T>
inline auto toReal(T &&value) -> typename std::enable_if<
    std::is_arithmetic<typename std::remove_reference<T>::type>::value,
    real>::type {
    // 常规算术类型
    return static_cast<real>(std::forward<T>(value));
}

inline auto toReal(const char *s, char **endptr) -> real {
    return static_cast<real>(
        parseFloat(s, endptr, static_cast<real *>(nullptr)));
}

inline auto toReal(const char *s) -> real { return toReal(s, nullptr); }

inline auto toReal(const char *s, std::size_t n) -> real {
    // 由于 C 的转换函数通常需要 \0 结尾，
    // 如果你的输入确定是 std::string，直接用 toReal(s.c_str()) 即可。
    // 但如果是在处理 buffer 的中间部分，可以手动处理：
    char temp[64]; // 浮点数通常不会超过这个长度
    std::size_t len = (n < 63) ? n : 63;
    std::memcpy(temp, s, len);
    temp[len] = '\0';
    return toReal(temp);
}
} // namespace meika

#endif
