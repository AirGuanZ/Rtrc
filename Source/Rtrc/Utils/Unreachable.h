#pragma once

#include <Rtrc/Common.h>

RTRC_BEGIN

[[noreturn]] inline void Unreachable()
{
#if defined(_MSC_VER)
    __assume(0);
#elif defined(__clang__) || defined(__GNUC__)
    __builtin_unreachable();
#else
    std::terminate();
#endif
}

RTRC_END
