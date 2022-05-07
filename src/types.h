#ifndef TYPES_H
#define TYPES_H

#include <sys/cdefs.h>
#define _MAKE_STRING(x) #x
#define MAKE_STRING(x)  _MAKE_STRING(x)

#define CURRENT_CONTEXT                                                  \
    "[XKLib][" __FILE__ ":" MAKE_STRING(__LINE__) "] -> "

namespace XKLib
{
    using ptr_t   = void*;
    using byte_t  = unsigned char;
    using data_t  = byte_t*;
    using bytes_t = std::vector<byte_t>;
#ifndef WINDOWS
    /* linux pid_t */
    using process_id_t = std::int32_t;
#else
    /* windows uses dword urgh */
    using process_id_t = std::int64_t;
#endif

    template <typename T>
    struct type_wrapper_t
    {
        using type = T;
    };

    template <typename T>
    inline constexpr type_wrapper_t<T> type_wrapper {};

    template <typename T>
    __attribute__((__always_inline__)) constexpr auto view_as(auto var)
    {
        return (T)(var);
    }
} // namespace XKLib

#endif // TYPES_H
