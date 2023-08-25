#pragma once

#include <Rtrc/Core/Common.h>

RTRC_BEGIN

namespace ThreadUtility
{

    enum class ThreadMode
    {
        Simple,   // Everything is in a single thread
        Standard, // Main thread for updating, one thread for rendering
    };

    enum class ThreadIdentifier
    {
        Unknown = 0,
        Main,
        Render
    };

    void SetThreadMode(ThreadMode mode);
    void SetThreadIndentifier(ThreadIdentifier identifier);

    bool IsStandardThreadMode();
    bool IsMainThread();
    bool IsRenderThread();

    inline void AssertIsMainThread() { assert(IsMainThread()); }
    inline void AssertIsRenderThread() { assert(IsRenderThread()); }

} // namespace ThreadUtility

using ThreadUtility::AssertIsMainThread;
using ThreadUtility::AssertIsRenderThread;

RTRC_END
