#pragma once

#include <Rtrc/Core/Math/Vector2.h>
#include <Rtrc/Core/Math/Vector3.h>
#include <Rtrc/Core/Math/Vector4.h>
#include <Rtrc/Core/SelfType.h>

RTRC_BEGIN

namespace eDSL
{

    namespace eDSLStructDetail
    {

        struct DSLStructBuilder;

    } // namespace eDSLStructDetail

} // namespace eDSL

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
    constexpr int ComputeMemberCount()
    {
        int ret = 0;
        StructDetail::ForEachMember<T>([&ret](auto, auto) { ++ret; });
        return ret;
    }

    template<typename T>
    constexpr int MemberCount = ComputeMemberCount<T>();

    template<typename T>
    concept RtrcStruct = requires { typename T::_rtrcStructTypeFlag; };

    struct _rtrcStructBase
    {
        static Sizer<1> _rtrcMemberCounter(...);
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

    struct DefaultStructBase
    {
        struct _rtrcStructTypeFlag {};
    };

    struct DefaultStructBuilder
    {
        template<typename C, TemplateStringParameter Name>
        using Base = DefaultStructBase;

        template<typename T>
        using Member = T;

        template<typename C, typename T, int MemberIndex, TemplateStringParameter Name>
        struct PreMember
        {

        };

        template<typename C, typename T, int MemberIndex>
        struct PostMember
        {

        };
    };

    template<template<typename> typename Sketch>
    struct SketchRebindBase
    {
        template<typename B>
        using _rtrcRebindStructBuilder = Sketch<B>;
    };

    template<RtrcStruct T, typename B>
    using RebindSketchBuilder = typename T::template _rtrcRebindStructBuilder<B>;

} // namespace StructDetail

using StructDetail::RtrcStruct;

#define RTRC_META_STRUCT_SETUP_MEMBER(NAME)                                         \
    enum { _rtrcMemberCounter##NAME =                                               \
        sizeof(_rtrcSelf##NAME::_rtrcMemberCounter((::Rtrc::StructDetail::Int<      \
            ::Rtrc::StructDetail::STRUCT_MAX_MEMBER_COUNT >*)nullptr)) - 1 };       \
    static ::Rtrc::StructDetail::Sizer<(_rtrcMemberCounter##NAME) + 2>              \
        _rtrcMemberCounter(::Rtrc::StructDetail::Int<(_rtrcMemberCounter##NAME)>*);

#define RTRC_META_STRUCT_PRE_MEMBER_ACCESS(NAME) \
    using _rtrcU##NAME = decltype([]<typename F>(const F &f) constexpr {

#define RTRC_META_STRUCT_POST_MEMBER_ACCESS(NAME)                                              \
    });                                                                                        \
    struct _rtrcMemberDesc##NAME : ::Rtrc::StructDetail::TemplateProcessProvider<_rtrcU##NAME> \
    {                                                                                          \
        using Rtrc::StructDetail::TemplateProcessProvider<_rtrcU##NAME>::Process;              \
        static constexpr bool isValidMember = true;                                            \
    };                                                                                         \
    static _rtrcMemberDesc##NAME *_rtrcMemberIndexToDesc(                                      \
        ::Rtrc::StructDetail::Int2Type<(_rtrcMemberCounter##NAME)>*);

#define rtrc_struct(NAME)                                                                      \
    template<typename B> struct _rtrcStructSketch##NAME;                                       \
    using NAME = _rtrcStructSketch##NAME<::Rtrc::StructDetail::DefaultStructBuilder>;          \
    using e##NAME = _rtrcStructSketch##NAME<::Rtrc::eDSL::eDSLStructDetail::DSLStructBuilder>; \
    template<typename B>                                                                       \
    struct _rtrcStructSketch##NAME :                                                           \
        ::Rtrc::StructDetail::_rtrcStructBase,                                                 \
        ::Rtrc::StructDetail::SketchRebindBase<_rtrcStructSketch##NAME>,                       \
        B::template Base<_rtrcStructSketch##NAME<B>, #NAME>

#define rtrc_struct_name(NAME) _rtrcStructSketch##NAME

#if defined(__INTELLISENSE__) || defined(__RSCPP_VERSION)
#define rtrc_var(TYPE, NAME)                                         \
    using _rtrcMemberType##NAME = typename B::template Member<TYPE>; \
    _rtrcMemberType##NAME NAME
#else
#define rtrc_var(TYPE, NAME)                                                          \
    RTRC_DEFINE_SELF_TYPE(_rtrcSelf##NAME)                                            \
    RTRC_META_STRUCT_SETUP_MEMBER(NAME)                                               \
    using _rtrcMemberType##NAME = typename B::template Member<TYPE>;                  \
    [[non_unique_address]] B::template PreMember<                                     \
        _rtrcSelf##NAME, TYPE, _rtrcMemberCounter##NAME, #NAME> _rtrcPreMember##NAME; \
    _rtrcMemberType##NAME NAME;                                                       \
    RTRC_META_STRUCT_PRE_MEMBER_ACCESS(NAME)                                          \
        f.template operator()(&_rtrcSelf##NAME::NAME, #NAME);                         \
    RTRC_META_STRUCT_POST_MEMBER_ACCESS(NAME)                                         \
    [[non_unique_address]] B::template PostMember<                                    \
        _rtrcSelf##NAME, TYPE, _rtrcMemberCounter##NAME> _rtrcPostMember##NAME;       \
    using _rtrcRequireComma##NAME = int
#endif

RTRC_END
