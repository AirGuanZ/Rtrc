#pragma once

#if !RTRC_REFLECTION_TOOL
#include <cista.h>
#endif

#include <Core/Macro/MacroForEach.h>
#include <Core/SelfType.h>
#include <Core/SourceWriter.h>

RTRC_BEGIN

#define RTRC_SER_SINGLE(X) ser(X, #X);
#define RTRC_SER(...) (([&]{ RTRC_MACRO_FOREACH_1(RTRC_SER_SINGLE, __VA_ARGS__); })())

#define RTRC_AUTO_SERIALIZE(...)                                                     \
    template<typename Ser> void Serialize(Ser &ser)       { RTRC_SER(__VA_ARGS__); } \
    template<typename Ser> void Serialize(Ser &ser) const { RTRC_SER(__VA_ARGS__); } \
    static constexpr int _rtrc_auto_serialize_need_semicolon_1 = 1

#define RTRC_AUTO_SERIALIZE_WITH_MEMBER_COUNT_CHECK(...)                                     \
    RTRC_AUTO_SERIALIZE(__VA_ARGS__);                                                        \
    RTRC_DEFINE_SELF_TYPE(_rtrc_auto_serialize_self_t)                                       \
    void _rtrc_auto_serialize_check_arg_count()                                              \
    {                                                                                        \
        constexpr int N = RTRC_MACRO_OVERLOADING_COUNT_ARGS(__VA_ARGS__);                    \
        static_assert(                                                                       \
            cista::arity<_rtrc_auto_serialize_self_t>() == N,                                \
            "RTRC_AUTO_SERIALIZE_WITH_ARG_COUNT_CHECK fails due to unmatched member count"); \
    } static constexpr int _rtrc_auto_serialize_need_semicolon_2 = 2

RTRC_END
