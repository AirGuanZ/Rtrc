#pragma once

#include <Rtrc/Core/Math/Vector2.h>
#include <Rtrc/Core/Math/Vector3.h>
#include <Rtrc/Core/Math/Vector4.h>
#include <Rtrc/Core/SelfType.h>

RTRC_BEGIN

namespace StructDetail
{

    constexpr int STRUCT_MAX_MEMBER_COUNT = 128;

    template<int N>
    struct Int : Int<N - 1> { };

    template<>
    struct Int<0> { };

    template<int N>
    struct Int2Type { };

    template<int N>
    struct Sizer { char _data[N]; };

    static_assert(sizeof(Sizer<19>) == 19);

    template<typename T, int I>
    using MemberDesc = std::remove_pointer_t<decltype(T::_rtrcMemberIndexToDesc(static_cast<Int2Type<I> *>(nullptr)))>;

    template<typename U>
    struct TemplateProcessProvider
    {
        template<typename F>
        static constexpr void Process(const F &f)
        {
            U{}.template operator()<F>(f);
        }

        static constexpr bool isValidMember = false;
    };
    
    template<typename T, typename F, int I>
    constexpr void CallForMember(const F &f)
    {
        if constexpr(requires { typename MemberDesc<T, I>; })
        {
            MemberDesc<T, I>::template Process<F>(f);
        }
    }

    template<typename T, typename F, int...Is>
    constexpr void ForEachMemberAux(const F &f, std::integer_sequence<int, Is...>)
    {
        (CallForMember<T, F, Is>(f), ...);
    }

    template<typename T, typename F>
    constexpr void ForEachMember(const F &f)
    {
        StructDetail::ForEachMemberAux<T, F>(f, std::make_integer_sequence<int, STRUCT_MAX_MEMBER_COUNT>());
    }

    template<typename T>
    concept RtrcStruct = requires { typename T::_rtrcStructTypeFlag; };

    struct _rtrcStructBase
    {
        struct _rtrcStructTypeFlag {};
        static StructDetail::Sizer<1> _rtrcMemberCounter(...);
        auto operator<=>(const _rtrcStructBase &) const = default;
        using float2 = Vector2f;
        using float3 = Vector3f;
        using float4 = Vector4f;
        using int2 = Vector2i;
        using int3 = Vector3i;
        using int4 = Vector4i;
        using uint = uint32_t;
        using uint2 = Vector2u;
        using uint3 = Vector3u;
        using uint4 = Vector4u;
        using float4x4 = Matrix4x4f;
    };

} // namespace StructDetail

#define RTRC_META_STRUCT_PRE_MEMBER(NAME)                                           \
    enum { _rtrcMemberCounter##NAME =                                               \
        sizeof(_rtrcSelf##NAME::_rtrcMemberCounter((::Rtrc::StructDetail::Int<      \
            ::Rtrc::StructDetail::STRUCT_MAX_MEMBER_COUNT >*)nullptr)) - 1 };       \
    static ::Rtrc::StructDetail::Sizer<(_rtrcMemberCounter##NAME) + 2>              \
        _rtrcMemberCounter(::Rtrc::StructDetail::Int<(_rtrcMemberCounter##NAME)>*); \
    using _rtrcU##NAME = decltype([]<typename F>(const F &f) constexpr {

#define RTRC_META_STRUCT_POST_MEMBER(NAME)                                                     \
    });                                                                                        \
    struct _rtrcMemberDesc##NAME : ::Rtrc::StructDetail::TemplateProcessProvider<_rtrcU##NAME> \
    {                                                                                          \
        using Rtrc::StructDetail::TemplateProcessProvider<_rtrcU##NAME>::Process;              \
        static constexpr bool isValidMember = true;                                            \
    };                                                                                         \
    static _rtrcMemberDesc##NAME *_rtrcMemberIndexToDesc(                                      \
        ::Rtrc::StructDetail::Int2Type<(_rtrcMemberCounter##NAME)>*);

using StructDetail::RtrcStruct;

#define rtrc_struct(NAME) struct NAME : ::Rtrc::StructDetail::_rtrcStructBase

#if defined(__INTELLISENSE__) || defined(__RSCPP_VERSION)
#define rtrc_var(TYPE, NAME)            \
    using _rtrcMemberType##NAME = TYPE; \
    _rtrcMemberType##NAME NAME
#else
#define rtrc_var(TYPE, NAME)                                  \
    RTRC_DEFINE_SELF_TYPE(_rtrcSelf##NAME)                    \
    using _rtrcMemberType##NAME = TYPE;                       \
    _rtrcMemberType##NAME NAME;                               \
    RTRC_META_STRUCT_PRE_MEMBER(NAME)                         \
        f.template operator()(&_rtrcSelf##NAME::NAME, #NAME); \
    RTRC_META_STRUCT_POST_MEMBER(NAME)                        \
    using _rtrcRequiresComma##NAME = int
#endif

RTRC_END
