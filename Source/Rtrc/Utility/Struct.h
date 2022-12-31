#pragma once

#include <Rtrc/Common.h>

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
    using MemberDesc = std::remove_pointer_t<decltype(T::_rtrcMemberIndexToDesc(static_cast<Int2Type<I> *>(0)))>;

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

#define RTRC_META_STRUCT_BEGIN(NAME)                                          \
    struct NAME                                                               \
    {                                                                         \
        using _rtrcSelf = NAME;                                               \
        static constexpr std::string_view _rtrcSelfName = #NAME;              \
        static ::Rtrc::StructDetail::Sizer<1> _rtrcMemberCounter(...);

#define RTRC_META_STRUCT_END()                                 \
        template<typename F>                                   \
        static constexpr void ForEachMember(const F &f)        \
        {                                                      \
            ::Rtrc::StructDetail::ForEachMember<_rtrcSelf>(f); \
        }                                                      \
    };

#define RTRC_META_STRUCT_PRE_MEMBER(NAME)                                           \
    static constexpr int _rtrcMemberCounter##NAME =                                 \
        sizeof(_rtrcMemberCounter((::Rtrc::StructDetail::Int<                       \
            ::Rtrc::StructDetail::STRUCT_MAX_MEMBER_COUNT >*)nullptr)) - 1;         \
    static ::Rtrc::StructDetail::Sizer<_rtrcMemberCounter##NAME + 2>                \
        _rtrcMemberCounter(::Rtrc::StructDetail::Int<_rtrcMemberCounter##NAME>*);   \
    struct _rtrcMemberDesc##NAME                                                    \
    {                                                                               \
        template<typename F>                                                        \
        static constexpr void Process(const F &f)                                   \
        {                                                                           \

#define RTRC_META_STRUCT_POST_MEMBER(NAME)                          \
        }                                                           \
    };                                                              \
    static _rtrcMemberDesc##NAME *_rtrcMemberIndexToDesc(           \
        ::Rtrc::StructDetail::Int2Type<_rtrcMemberCounter##NAME>*);

template<typename T>
concept RtrcStruct = requires { typename T::_rtrcStructTypeFlag; };

#define rtrc_struct(NAME)                                                 \
    struct NAME;                                                          \
    struct _rtrcCBufferBase##NAME                                         \
    {                                                                     \
        using _rtrcSelf = NAME;                                           \
        struct _rtrcStructTypeFlag{};                                     \
        static constexpr std::string_view _rtrcSelfName = #NAME;          \
        static ::Rtrc::StructDetail::Sizer<1> _rtrcMemberCounter(...);    \
        template<typename F>                                              \
        static constexpr void ForEachMember(const F &f)                   \
        {                                                                 \
            ::Rtrc::StructDetail::ForEachMember<_rtrcSelf>(f);            \
        }                                                                 \
        auto operator<=>(const _rtrcCBufferBase##NAME &) const = default; \
    };                                                                    \
    struct NAME : _rtrcCBufferBase##NAME

#define rtrc_var(TYPE, NAME)                            \
    RTRC_META_STRUCT_PRE_MEMBER(NAME)                   \
        f.template operator()(&_rtrcSelf::NAME, #NAME); \
    RTRC_META_STRUCT_POST_MEMBER(NAME)                  \
    using _rtrcMemberType##NAME = TYPE;                 \
    _rtrcMemberType##NAME NAME

RTRC_END
