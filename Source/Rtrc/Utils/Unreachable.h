#pragma once

#include <Rtrc/Common.h>

RTRC_BEGIN

[[noreturn]] inline void Unreachable()
{
#if RTRC_DEBUG
    std::terminate();
#else
#if defined(_MSC_VER)
    __assume(0);
#elif defined(__clang__) || defined(__GNUC__)
    __builtin_unreachable();
#else
    std::terminate();
#endif
#endif
}

RTRC_END
