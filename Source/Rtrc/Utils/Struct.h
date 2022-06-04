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

    using TagUnderlyingType = uint32_t;

    template<TagUnderlyingType RequiredTag, typename T, typename F, int I>
    void CallForMatchedTag(const F &f)
    {
        using MemberDesc = std::remove_pointer_t<decltype(T::_rtrcMemberIndexToDesc(static_cast<Int2Type<I> *>(0)))>;
        constexpr TagUnderlyingType ActualTag = static_cast<TagUnderlyingType>(MemberDesc::Tag);
        if constexpr(ActualTag != 0 && (RequiredTag & ActualTag) == ActualTag)
            MemberDesc::template Process<F>(f);
    }

    template<TagUnderlyingType Tag, typename T, typename F, int...Is>
    void ForEachMemberAux(const F &f, std::integer_sequence<int, Is...>)
    {
        (CallForMatchedTag<Tag, T, F, Is>(f), ...);
    }

    template<TagUnderlyingType Tag, typename T, typename F>
    void ForEachMember(const F &f)
    {
        StructDetail::ForEachMemberAux<Tag, T, F>(f, std::make_integer_sequence<int, STRUCT_MAX_MEMBER_COUNT>());
    }

} // namespace StructDetail

#define RTRC_META_STRUCT_BEGIN(NAME)                                          \
    struct NAME                                                               \
    {                                                                         \
        using _rtrcSelf = NAME;                                               \
        static ::Rtrc::StructDetail::Sizer<1> _rtrcMemberCounter(...);        \
        struct _rtrcEmptyDesc                                                 \
        {                                                                     \
            static constexpr ::Rtrc::StructDetail::TagUnderlyingType Tag = 0; \
            template<typename F>                                              \
            static void Process(const F &f) { }                               \
        };                                                                    \
        static _rtrcEmptyDesc *_rtrcMemberIndexToDesc(...);

#define RTRC_META_STRUCT_END()                                              \
        template<::Rtrc::StructDetail::TagUnderlyingType Tag, typename F>   \
        static void ForEachMember(const F &f)                               \
        {                                                                   \
            ::Rtrc::StructDetail::ForEachMember<Tag, _rtrcSelf>(f);         \
        }                                                                   \
        template<typename F>                                                \
        static void ForEachMember(const F &f)                               \
        {                                                                   \
            ::Rtrc::StructDetail::ForEachMember<0xffffffff, _rtrcSelf>(f);  \
        }                                                                   \
    };

#define RTRC_META_STRUCT_PRE_MEMBER(TAG, NAME)                                      \
    static constexpr int _rtrcMemberCounter##NAME =                                 \
        sizeof(_rtrcMemberCounter((::Rtrc::StructDetail::Int<                       \
            ::Rtrc::StructDetail::STRUCT_MAX_MEMBER_COUNT >*)nullptr)) - 1;         \
    static ::Rtrc::StructDetail::Sizer<_rtrcMemberCounter##NAME + 2>                \
        _rtrcMemberCounter(::Rtrc::StructDetail::Int<_rtrcMemberCounter##NAME>*);   \
    struct _rtrcMemberDesc##NAME                                                    \
    {                                                                               \
        static constexpr auto Tag = TAG;                                            \
        template<typename F>                                                        \
        static void Process(const F &f)                                             \
        {                                                                           \

#define RTRC_META_STRUCT_POST_MEMBER(NAME)                          \
        }                                                           \
    };                                                              \
    static _rtrcMemberDesc##NAME *_rtrcMemberIndexToDesc(           \
        ::Rtrc::StructDetail::Int2Type<_rtrcMemberCounter##NAME>*);

RTRC_END
