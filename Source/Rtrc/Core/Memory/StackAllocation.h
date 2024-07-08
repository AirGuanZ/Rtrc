#pragma once

#include <Rtrc/Core/Common.h>

#if RTRC_WIN32
#include <malloc.h>
#else
#error "StackAllocation is not implemented"
#endif

RTRC_BEGIN

inline void* AllocateOnStack(size_t size)
{
#if RTRC_WIN32
    return _alloca(size);
#else
#error "StackAllocation is not implemented"
#endif
}

RTRC_END
