#pragma once

#include <Rtrc/Core/Struct.h>
#include <Rtrc/Core/TemplateStringParameter.h>

#include "eVariable.h"

RTRC_EDSL_BEGIN

namespace ClassDetail
{

    struct CommonBase
    {
        struct eDSLTypeFlag {};
        static StructDetail::Sizer<1> _rtrcMemberCounter(...) { return {}; }
    };

    template<typename T>
    concept RtrcDSL_Struct = requires{ typename T::eDSLTypeFlag; };

    template<TemplateStringParameter Str>
    struct StaticTypeNameBase
    {
        static constexpr const char *GetStaticTypeName()
        {
            return static_cast<const char *>(Str);
        }
    };

    template<typename T>
    constexpr int ComputeMemberCount()
    {
        int ret = 0;
        StructDetail::ForEachMember<T>([&ret](auto, auto){ ++ret; });
        return ret;
    }

    template<typename T>
    constexpr int MemberCount = ComputeMemberCount<T>();

    template<typename ClassType, int MemberIndex>
    struct MemberVariablePostInitializer
    {
        MemberVariablePostInitializer()
        {
            if constexpr(ClassDetail::MemberCount<ClassType> == MemberIndex + 1)
            {
                PopConstructParentVariable();
            }
        }

        MemberVariablePostInitializer(const MemberVariablePostInitializer &) : MemberVariablePostInitializer() { }

        MemberVariablePostInitializer &operator=(const MemberVariablePostInitializer &)
        {
            if constexpr(ClassDetail::MemberCount<ClassType> == MemberIndex + 1)
            {
                PopCopyParentVariable();
            }
            return *this;
        }
    };

    template<typename ClassType>
    struct RegisterStructToRecordContext
    {
        RegisterStructToRecordContext();
    };

    struct Dummy { int dummy; };

} // namespace ClassDetail

using ClassDetail::RtrcDSL_Struct;

#define RTRC_EDSL_STRUCT(NAME)                                \
    struct NAME :                                             \
        ::Rtrc::eDSL::eVariable<NAME>,                        \
        ::Rtrc::eDSL::ClassDetail::CommonBase,                \
        ::Rtrc::eDSL::ClassDetail::StaticTypeNameBase<#NAME>, \
        ::Rtrc::eDSL::ClassDetail::RegisterStructToRecordContext<NAME>

#if defined(__INTELLISENSE__) || defined(__RSCPP_VERSION)
#define RTRC_EDSL_VAR(TYPE, NAME) \
    TYPE NAME;                    \
    using _requireComma##NAME = int
#else
#define RTRC_EDSL_VAR(TYPE, NAME)                                       \
    RTRC_DEFINE_SELF_TYPE(_rtrcSelf##NAME)                              \
    Rtrc::eDSL::MemberVariableNameInitializer<#NAME> _rtrcInit##NAME;   \
    TYPE NAME;                                                          \
    RTRC_META_STRUCT_PRE_MEMBER(NAME)                                   \
        f.template operator()(&_rtrcSelf##NAME::NAME, #NAME);           \
    RTRC_META_STRUCT_POST_MEMBER(NAME)                                  \
    Rtrc::eDSL::ClassDetail::MemberVariablePostInitializer<             \
        _rtrcSelf##NAME, _rtrcMemberCounter##NAME> _rtrcPostInit##NAME; \
    using _requireComma##NAME = int
#endif

RTRC_EDSL_END
