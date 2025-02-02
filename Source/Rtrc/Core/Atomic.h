#pragma once

#include <atomic>

#include <Rtrc/Core/Common.h>

RTRC_BEGIN

template <typename T>
void AtomicMax(std::atomic<T> &x, T y)
{
    auto z = x.load();
    while(z < y && !x.compare_exchange_weak(z, y))
    {
        
    }
}

RTRC_END
