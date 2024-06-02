#pragma once

#include <Rtrc/Core/Struct.h>
#include <Rtrc/Core/TemplateStringParameter.h>

#include "../eVariable.h"
#include "../NativeTypeMapping.h"

RTRC_EDSL_BEGIN

namespace eDSLStructDetail
{

    template<typename C, TemplateStringParameter Name>
    struct CommonBase : eVariable<C>
    {
        struct eDSLStructTypeFlag { };

        static const char *GetStaticTypeName()
        {
            return static_cast<const char *>(Name);
        }

        CommonBase();
    };

    template<typename T>
    concept RtrcDSLStruct = requires{ typename T::eDSLStructTypeFlag; };

    template<typename C, int MemberIndex, TemplateStringParameter Name>
    struct PostMember
    {
        PostMember()
        {
            if constexpr(StructDetail::MemberCount<C> == MemberIndex + 1)
            {
                PopConstructParentVariable();
            }
        }

        PostMember(const PostMember &) : PostMember() { }

        PostMember &operator=(const PostMember &)
        {
            if constexpr(StructDetail::MemberCount<C> == MemberIndex + 1)
            {
                PopCopyParentVariable();
            }
            return *this;
        }
    };

    struct DSLStructBuilder
    {
        template<typename C, TemplateStringParameter Name>
        using Base = CommonBase<C, Name>;

        template<typename T>
        using Member = NativeTypeToDSLType<T>;

        template<typename C, typename T, int MemberIndex, TemplateStringParameter Name>
        using PreMember = MemberVariableNameInitializer<Name>;

        template<typename C, typename T, int MemberIndex>
        struct PostMember
        {
            PostMember()
            {
                if constexpr(StructDetail::MemberCount<C> == MemberIndex + 1)
                {
                    PopConstructParentVariable();
                }
            }

            PostMember(const PostMember &) : PostMember() { }

            PostMember &operator=(const PostMember &)
            {
                if constexpr(StructDetail::MemberCount<C> == MemberIndex + 1)
                {
                    PopCopyParentVariable();
                }
                return *this;
            }
        };
    };

} // namespace eDSLStructDetail

using eDSLStructDetail::RtrcDSLStruct;

RTRC_EDSL_END
