#ifndef TYPES_H
#define TYPES_H

#include <cstddef>
#include <iostream>
#include <type_traits>
#include <vector>

#if defined _WIN32 || defined _WIN64
    #define WINDOWS
#endif

#define _MAKE_STRING(x) #x
#define MAKE_STRING(x)  _MAKE_STRING(x)

#define CURRENT_CONTEXT                                                  \
    "[XLib][" __FILE__ ":" MAKE_STRING(__LINE__) "] -> "

namespace XLib
{
    using ptr_t      = void*;
    using byte_t     = unsigned char;
    using data_t     = byte_t*;
    using bytes_t    = std::vector<byte_t>;
    using safesize_t = int32_t;
    /* linux pid_t */
    using pid_t = uint32_t;
#define __pid_t_defined

    class bits_t
    {
    };

    template <typename T>
    struct type_wrapper_t
    {
        using type = T;
    };

    template <typename T>
    constexpr inline type_wrapper_t<T> type_wrapper {};

    template <typename T1, typename T2>
    auto view_as(T2 var)
    {
        return (T1)(var);
    }
} // namespace XLib

#define ConsoleOutput(format) std::cout << "[XLib] -> " << format

#endif // TYPES_H