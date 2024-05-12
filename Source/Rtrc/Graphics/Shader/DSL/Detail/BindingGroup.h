#pragma once

#include <Rtrc/Core/Struct.h>
#include <Rtrc/Core/TemplateStringParameter.h>

#include "Common.h"

RTRC_EDSL_BEGIN

namespace BindingGroupDetail
{

    struct CommonBase
    {
        struct eDSLBindingGroupFlag {};
        StructDetail::Sizer<1> _rtrcMemberCounter(...) { return {}; }
    };

    template<TemplateStringParameter Str>
    struct GetNameBase
    {
        static constexpr std::string_view GetBindingGroupName()
        {
            return static_cast<std::string_view>(Str);
        }
    };

} // namespace BindingGroupDetail

#define RTRC_EDSL_BINDING_GROUP(NAME)               \
    struct NAME :                                   \
        Rtrc::eDSL::BindingGroupDetail::CommonBase, \
        Rtrc::eDSL::BindingGroupDetail::GetNameBase<#NAME>

#define RTRC_EDSL_DEFINE(TYPE, NAME)                           \
    RTRC_DEFINE_SELF_TYPE(_rtrcSelf##NAME)                     \
    TYPE NAME;                                                 \
    RTRC_META_STRUCT_PRE_MEMBER(NAME)                          \
        f.template operator()(&_rtrcSelf##NAME::NAME, #NAME) ; \
    RTRC_META_STRUCT_POST_MEMBER(NAME)                         \
    using _requireComma##NAME = int

RTRC_EDSL_END
