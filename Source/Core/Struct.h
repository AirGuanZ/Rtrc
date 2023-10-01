#pragma once

#include <Core/SelfType.h>

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
    };                                                                                         \
    static _rtrcMemberDesc##NAME *_rtrcMemberIndexToDesc(                                      \
        ::Rtrc::StructDetail::Int2Type<(_rtrcMemberCounter##NAME)>*);

RTRC_END
