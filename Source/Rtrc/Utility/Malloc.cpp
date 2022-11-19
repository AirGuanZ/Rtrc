#include <Rtrc/Utility/Malloc.h>

#if defined(_MSC_VER) && RTRC_DEBUG
#include <crtdbg.h>
#endif

RTRC_BEGIN

void EnableMemoryLeakReporter()
{
#if defined(_MSC_VER) && RTRC_DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
}

RTRC_END

#if !(defined(_MSC_VER) && RTRC_DEBUG)
#include <mimalloc-new-delete.h>
#endif
