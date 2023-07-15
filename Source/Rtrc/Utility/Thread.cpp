#include <cassert>

#include <Rtrc/Utility/Thread.h>

RTRC_BEGIN

namespace ThreadUtility
{

    ThreadMode gThreadMode = ThreadMode::Simple;
    thread_local ThreadIdentifier gThreadIdentifier = ThreadIdentifier::Unknown;

    void SetThreadMode(ThreadMode mode)
    {
        assert(gThreadMode == ThreadMode::Simple);
        gThreadMode = mode;
    }

    void SetThreadIndentifier(ThreadIdentifier identifier)
    {
        assert(gThreadIdentifier == ThreadIdentifier::Unknown);
        if(gThreadMode == ThreadMode::Standard)
        {
            gThreadIdentifier = identifier;
        }
    }

    bool IsStandardThreadMode()
    {
        return gThreadMode == ThreadMode::Standard;
    }

    bool IsMainThread()
    {
        return gThreadMode == ThreadMode::Simple || gThreadIdentifier == ThreadIdentifier::Main;
    }

    bool IsRenderThread()
    {
        return gThreadMode == ThreadMode::Simple || gThreadIdentifier == ThreadIdentifier::Render;
    }

} // namespace ThreadUtility

RTRC_END
