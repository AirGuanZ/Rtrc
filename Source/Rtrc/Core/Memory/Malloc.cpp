#include <Rtrc/Core/Memory/Malloc.h>

#if defined(_MSC_VER) && RTRC_DEBUG
#include <crtdbg.h>
#endif

#if RTRC_ENABLE_MIMALLOC
#include <mimalloc.h>
#endif

RTRC_BEGIN

void EnableMemoryLeakReporter()
{
#if defined(_MSC_VER) && RTRC_DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif
}

void *AllocateMemory(size_t size)
{
#if RTRC_ENABLE_MIMALLOC
    return ::mi_malloc(size);
#else
    return std::malloc(size);
#endif
}

void FreeMemory(void *pointer)
{
#if RTRC_ENABLE_MIMALLOC
    ::mi_free(pointer);
#else
    std::free(pointer);
#endif
}

void *AllocateAlignedMemory(size_t size, size_t align)
{
#if RTRC_ENABLE_MIMALLOC
    return ::mi_aligned_alloc(align, size);
#elif _WIN32
    return _aligned_malloc(size, align);
#else
    return std::aligned_alloc(align, size);
#endif
}

void FreeAlignedMemory(void *pointer, size_t align)
{
#if RTRC_ENABLE_MIMALLOC
    ::mi_free_aligned(pointer, align);
#elif _WIN32
    _aligned_free(pointer);
#else
    std::free(pointer);
#endif
}

RTRC_END

#if RTRC_ENABLE_MIMALLOC && !(defined(_MSC_VER) && RTRC_DEBUG)
#include <mimalloc-new-delete.h>
#endif
