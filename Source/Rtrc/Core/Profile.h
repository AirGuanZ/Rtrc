#pragma once

#include <Rtrc/Core/Common.h>

#if RTRC_ENABLE_TRACY
#include <tracy/Tracy.hpp>
#endif

RTRC_BEGIN

#if RTRC_ENABLE_TRACY

#define RTRC_PROFILER_UNIQUE_STRING_POINTER(NAME) RTRC_PROFILER_UNIQUE_STRING_POINTER_IMPL(NAME)
#define RTRC_PROFILER_UNIQUE_STRING_POINTER_IMPL(NAME) ([]{ return NAME; }())

#define RTRC_PROFILER_SCOPE_CPU(NAME) RTRC_PROFILER_SCOPE_CPU_IMPL(NAME)
#define RTRC_PROFILER_SCOPE_CPU_IMPL(NAME) ZoneScopedN(RTRC_PROFILER_UNIQUE_STRING_POINTER(NAME))

#else

#define RTRC_PROFILER_SCOPE_CPU(NAME) do { } while(false)

#endif

RTRC_END