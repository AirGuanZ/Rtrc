#pragma once

#include <Core/Common.h>

RTRC_BEGIN

namespace SelfTypeDetail
{

    template<typename T>
    struct SelfTypeReader
    {
        friend auto GetSelfTypeADL(SelfTypeReader);
    };

    template<typename T, typename U>
    struct SelfTypeWriter
    {
        friend auto GetSelfTypeADL(SelfTypeReader<T>) { return U{}; }
    };

    void GetSelfTypeADL();

    template<typename T>
    using SelfTypeResult = std::remove_pointer_t<decltype(GetSelfTypeADL(SelfTypeReader<T>{})) > ;

#define RTRC_DEFINE_SELF_TYPE(RESULT_NAME)                                                                            \
    struct _rtrcSelfTypeTag##RESULT_NAME {};                                                                          \
    constexpr auto _rtrcSelfTypeHelper##RESULT_NAME() ->                                                              \
        decltype(::Rtrc::SelfTypeDetail::SelfTypeWriter<_rtrcSelfTypeTag##RESULT_NAME, decltype(this)>{}, void()) { } \
    using RESULT_NAME = ::Rtrc::SelfTypeDetail::SelfTypeResult<_rtrcSelfTypeTag##RESULT_NAME>;


} // namespace SelfTypeDetail

RTRC_END
