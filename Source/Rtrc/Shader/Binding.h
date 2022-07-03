#pragma once

#include <optional>

#include <Rtrc/RHI/RHI.h>
#include <Rtrc/Shader/Struct.h>
#include <Rtrc/Utils/EnumFlags.h>
#include <Rtrc/Utils/MacroForEach.h>
#include <Rtrc/Utils/TemplateStringParameter.h>

RTRC_BEGIN

namespace ShaderDetail
{

    template<typename T>
    struct GetArraySizeAux
    {

    };

    template<typename T, int N>
    struct GetArraySizeAux<T[N]>
    {
        static constexpr int Value = N;
    };

    struct BindingParamWrapper
    {
        constexpr BindingParamWrapper()
            : isArraySize(false), isShaderStages(false), arraySize(0), shaderStages(RHI::ShaderStageFlags::All)
        {

        }

        constexpr BindingParamWrapper(int arraySize)
            : isArraySize(true), isShaderStages(false), arraySize(arraySize), shaderStages(RHI::ShaderStageFlags::All)
        {

        }

        constexpr BindingParamWrapper(RHI::ShaderStageFlag shaderStages)
            : isArraySize(false), isShaderStages(true), arraySize(0), shaderStages(shaderStages)
        {

        }

        bool                 isArraySize;
        bool                 isShaderStages;
        int                  arraySize;
        RHI::ShaderStageFlag shaderStages;
    };

    constexpr auto DUMMY_BINDING_PARAM_WRAPPER = BindingParamWrapper();

    template<BindingParamWrapper...ParamWrappers>
    constexpr bool ExtractIsArray()
    {
        return (ParamWrappers.isArraySize || ...);
    }

    template<BindingParamWrapper...ParamWrappers>
    constexpr int ExtractArraySize()
    {
        return (std::max)({ ParamWrappers.arraySize... });
    }

    constexpr RHI::ShaderStageFlag ExtractShaderStages()
    {
        return RHI::ShaderStageFlags::All;
    }

    template<typename...ParamWrappers>
    constexpr RHI::ShaderStageFlag ExtractShaderStages(BindingParamWrapper wrapper, ParamWrappers...wrappers)
    {
        if(wrapper.isShaderStages)
        {
            return wrapper.shaderStages;
        }
        return ExtractShaderStages(wrappers...);
    }

    template<BindingParamWrapper...ParamWrappers>
    struct BindingParamExtractor
    {
        static constexpr auto IsArray = ExtractIsArray<ParamWrappers...>();
        static constexpr auto ArraySize = IsArray ? ExtractArraySize<ParamWrappers...>() : -1;
        static constexpr auto ShaderStages = ExtractShaderStages(ParamWrappers...);
    };

    template<typename T>
    struct _rtrcBindingTemplateParameterExtractorBuffer
    {
        using Type = T;
        static constexpr RHI::BindingType BindingType = RHI::BindingType::Buffer;
    };

    template<typename T>
    struct _rtrcBindingTemplateParameterExtractorTexture2D
    {
        using Type = T;
        static constexpr RHI::BindingType BindingType = RHI::BindingType::Texture2D;
    };

    template<typename T>
    struct _rtrcBindingTemplateParameterExtractorStructuredBuffer
    {
        using Type = T;
        static constexpr RHI::BindingType BindingType = RHI::BindingType::StructuredBuffer;
    };

    template<typename T>
    struct _rtrcBindingTemplateParameterExtractorRWBuffer
    {
        using Type = T;
        static constexpr RHI::BindingType BindingType = RHI::BindingType::RWBuffer;
    };

    template<typename T>
    struct _rtrcBindingTemplateParameterExtractorRWTexture2D
    {
        using Type = T;
        static constexpr RHI::BindingType BindingType = RHI::BindingType::RWTexture2D;
    };

    template<typename T>
    struct _rtrcBindingTemplateParameterExtractorRWStructuredBuffer
    {
        using Type = T;
        static constexpr RHI::BindingType BindingType = RHI::BindingType::RWStructuredBuffer;
    };

    template<typename T>
    struct _rtrcBindingTemplateParameterExtractorConstantBuffer
    {
        using Type = T;
        static constexpr RHI::BindingType BindingType = RHI::BindingType::ConstantBuffer;
    };

    struct _rtrcBindingTemplateParameterExtractorSampler
    {
        using Type = void;
        static constexpr RHI::BindingType BindingType = RHI::BindingType::Sampler;
    };

    template<RHI::BindingType T>
    struct BindingTypeToValueType
    {
        using Type = std::monostate;
    };

    template<>
    struct BindingTypeToValueType<RHI::BindingType::Buffer>
    {
        using Type = RC<RHI::BufferSRV>;
    };

    template<>
    struct BindingTypeToValueType<RHI::BindingType::Texture2D>
    {
        using Type = RC<RHI::Texture2DSRV>;
    };

    template<>
    struct BindingTypeToValueType<RHI::BindingType::StructuredBuffer>
    {
        using Type = RC<RHI::BufferSRV>;
    };

    template<>
    struct BindingTypeToValueType<RHI::BindingType::Sampler>
    {
        using Type = RC<RHI::Sampler>;
    };

} // namespace ShaderDetail

template<
    typename             TemplateParam,
    RHI::BindingType     BindingTypeParam,
    int                  ArraySizeParam,
    RHI::ShaderStageFlag ShaderStagesParam>
class BindingSlot
{
public:
    
    static constexpr RHI::BindingType BindingType = BindingTypeParam;

    using TemplateParameter = TemplateParam;

    static constexpr bool                 IsArray      = ArraySizeParam > 0;
    static constexpr int                  ArraySize    = ArraySizeParam;
    static constexpr RHI::ShaderStageFlag ShaderStages = ShaderStagesParam;

    using ValueType = typename ShaderDetail::BindingTypeToValueType<BindingType>::Type;

    const ValueType &GetValue() const
    {
        return value_;
    }

    BindingSlot &operator=(const ValueType &value)
    {
        value_ = value;
        return *this;
    }

private:

    ValueType value_;
};

#define RTRC_BINDING_GROUP_BEGIN(NAME)                              \
    RTRC_META_STRUCT_BEGIN(NAME)                                    \
    struct _rtrcBindingGroupTag { };                                \
    static constexpr auto VS  = ::Rtrc::RHI::ShaderStageFlags::VS;  \
    static constexpr auto FS  = ::Rtrc::RHI::ShaderStageFlags::FS;  \
    static constexpr auto CS  = ::Rtrc::RHI::ShaderStageFlags::CS;  \
    static constexpr auto All = ::Rtrc::RHI::ShaderStageFlags::All; \
    using float2              = ::Rtrc::Vector2f;                   \
    using float3              = ::Rtrc::Vector3f;                   \
    using float4              = ::Rtrc::Vector4f;                   \
    using int2                = ::Rtrc::Vector2i;                   \
    using int3                = ::Rtrc::Vector3i;                   \
    using int4                = ::Rtrc::Vector4i;                   \
    using uint                = uint32_t;                           \
    using uint2               = ::Rtrc::Vector2u;                   \
    using uint3               = ::Rtrc::Vector3u;                   \
    using uint4               = ::Rtrc::Vector4u;

#define RTRC_BINDING_GROUP_END() RTRC_META_STRUCT_END()

template<typename T>
concept BindingGroupStruct = requires{ typename T::_rtrcBindingGroupTag; };

#define RTRC_BINDING_PARAMETER_WRAPPER(X) ::Rtrc::ShaderDetail::BindingParamWrapper(X),

#define RTRC_BINDING_GROUP_MEMBER(TYPE, NAME, ...)                                                  \
    using _rtrcBindingParamExtractor##NAME =                                                        \
        ::Rtrc::ShaderDetail::BindingParamExtractor<                                                \
            RTRC_MACRO_FOREACH_1(RTRC_BINDING_PARAMETER_WRAPPER, __VA_ARGS__)                       \
                ::Rtrc::ShaderDetail::DUMMY_BINDING_PARAM_WRAPPER>;                                 \
    static constexpr auto _rtrcArraySize##NAME = _rtrcBindingParamExtractor##NAME::ArraySize;       \
    static constexpr auto _rtrcShaderStages##NAME = _rtrcBindingParamExtractor##NAME::ShaderStages; \
    using _rtrcBindingTemplateParam##NAME =                                                         \
        typename ::Rtrc::ShaderDetail::_rtrcBindingTemplateParameterExtractor##TYPE::Type;          \
    static constexpr auto _rtrcBindingTypeParam##NAME =                                             \
        typename ::Rtrc::ShaderDetail::_rtrcBindingTemplateParameterExtractor##TYPE::BindingType;   \
    RTRC_META_STRUCT_PRE_MEMBER(NAME)                                                               \
    {                                                                                               \
        f(&_rtrcSelf::NAME, #NAME);                                                                 \
    }                                                                                               \
    RTRC_META_STRUCT_POST_MEMBER(NAME)                                                              \
    ::Rtrc::BindingSlot<                                                                            \
        _rtrcBindingTemplateParam##NAME,                                                            \
        _rtrcBindingTypeParam##NAME,                                                                \
        _rtrcArraySize##NAME,                                                                       \
        _rtrcShaderStages##NAME> NAME;

#define $group_begin RTRC_BINDING_GROUP_BEGIN
#define $group_end   RTRC_BINDING_GROUP_END
#define $binding     RTRC_BINDING_GROUP_MEMBER

struct BindingInfo
{
    std::string                       name;
    RHI::BindingType                  type;
    RHI::ShaderStageFlag              shaderStages;
    std::optional<uint32_t>           arraySize;
    const StructInfo                 *structInfo;
    RHI::BindingTemplateParameterType templateParameter;
};

using AliasedBindingsInfo = std::vector<BindingInfo>;

struct BindingGroupLayoutInfo
{
    std::vector<AliasedBindingsInfo> bindings;
    TypeIndex groupStructType;
};

template<BindingGroupStruct Struct>
const RHI::BindingGroupLayoutDesc *GetBindingGroupLayoutDesc()
{
    static const RHI::BindingGroupLayoutDesc result = []
    {
        RHI::BindingGroupLayoutDesc groupLayoutDesc;
        Struct::template ForEachMember([&]<typename Member>(Member Struct:: *, const char *name)
        {
            RHI::BindingDesc desc = {
                .name         = name,
                .type         = Member::BindingType,
                .shaderStages = Member::ShaderStages
            };
            if constexpr(Member::IsArray)
            {
                desc.arraySize = static_cast<uint32_t>(Member::ArraySize);
            }
            groupLayoutDesc.bindings.push_back({ desc });
        });
        return groupLayoutDesc;
    }();
    return &result;
}

template<BindingGroupStruct Struct>
const BindingGroupLayoutInfo *GetBindingGroupLayoutInfo()
{
    static const BindingGroupLayoutInfo result = []
    {
        BindingGroupLayoutInfo groupLayoutDesc;
        Struct::template ForEachMember([&]<typename Member>(Member Struct:: *, const char *name)
        {
            BindingInfo desc = {
                .name         = name,
                .type         = Member::BindingType,
                .shaderStages = Member::ShaderStages
            };
            if constexpr(Member::IsArray)
            {
                desc.arraySize = static_cast<uint32_t>(Member::ArraySize);
            }
#define ADD_CASE(TYPE, VAL)                                                        \
            if constexpr(std::is_same_v<typename Member::TemplateParameter, TYPE>) \
            {                                                                      \
                desc.templateParameter = RHI::BindingTemplateParameterType::VAL;   \
            }
            ADD_CASE(void,     Undefined) else
            ADD_CASE(int32_t,  Int)       else
            ADD_CASE(Vector2i, Int2)      else
            ADD_CASE(Vector3i, Int3)      else
            ADD_CASE(Vector4i, Int4)      else
            ADD_CASE(uint32_t, UInt)      else
            ADD_CASE(Vector2u, UInt2)     else
            ADD_CASE(Vector3u, UInt3)     else
            ADD_CASE(Vector4u, UInt4)     else
            ADD_CASE(float,    Float)     else
            ADD_CASE(Vector2f, Float2)    else
            ADD_CASE(Vector3f, Float3)    else
            ADD_CASE(Vector4f, Float4)
#undef ADD_CASE
            if constexpr(RtrcStructType<typename Member::TemplateParameter>)
            {
                desc.structInfo = GetRtrcStructInfo<Member::TemplateParameter>();
                desc.templateParameter = RHI::BindingTemplateParameterType::Struct;
            }
            groupLayoutDesc.bindings.push_back({ desc });
        });
        groupLayoutDesc.groupStructType = TypeIndex::Get<Struct>();
        return groupLayoutDesc;
    }();
    return &result;
}

template<BindingGroupStruct Struct, typename Member>
void ModifyBindingGroup(
    RHI::BindingGroup *instance,
    Member Struct::* member,
    const RC<RHI::BufferSRV> &srv)
{
    int index = 0;
    auto processBinding = [&]<typename Binding>(Binding Struct::* p, const char*)
    {
        if constexpr(std::is_same_v<Binding, Member>)
        {
            if(GetMemberOffset(p) == GetMemberOffset(member))
            {
                instance->ModifyMember(index, srv);
            }
        }
        ++index;
    };
    Struct::ForEachMember(processBinding);
}

template<BindingGroupStruct Struct, typename Member>
void ModifyBindingGroup(
    RHI::BindingGroup *instance,
    Member Struct::* member,
    const RC<RHI::BufferUAV> &uav)
{
    int index = 0;
    auto processBinding = [&]<typename Binding>(Binding Struct:: * p, const char *)
    {
        if constexpr(std::is_same_v<Binding, Member>)
        {
            if(GetMemberOffset(p) == GetMemberOffset(member))
            {
                instance->ModifyMember(index, uav);
            }
        }
        ++index;
    };
    Struct::ForEachMember(processBinding);
}

template<BindingGroupStruct Struct, typename Member>
void ModifyBindingGroup(
    RHI::BindingGroup *instance,
    Member Struct::* member,
    const RC<RHI::Texture2DSRV> &srv)
{
    int index = 0;
    auto processBinding = [&]<typename Binding>(Binding Struct::* p, const char *)
    {
        if constexpr(std::is_same_v<Binding, Member>)
        {
            if(GetMemberOffset(p) == GetMemberOffset(member))
            {
                instance->ModifyMember(index, srv);
            }
        }
        ++index;
    };
    Struct::ForEachMember(processBinding);
}

template<BindingGroupStruct Struct, typename Member>
void ModifyBindingGroup(
    RHI::BindingGroup *instance,
    Member Struct::* member,
    const RC<RHI::Texture2DUAV> &uav)
{
    int index = 0;
    auto processBinding = [&]<typename Binding>(Binding Struct::* p, const char *)
    {
        if constexpr(std::is_same_v<Binding, Member>)
        {
            if(GetMemberOffset(p) == GetMemberOffset(member))
            {
                instance->ModifyMember(index, uav);
            }
        }
        ++index;
    };
    Struct::ForEachMember(processBinding);
}

template<BindingGroupStruct Struct, typename Member>
void ModifyBindingGroup(
    RHI::BindingGroup *instance,
    Member Struct::*member,
    const RC<RHI::Sampler> &sampler)
{
    int index = 0;
    auto processBinding = [&]<typename Binding>(Binding Struct:: * p, const char *)
    {
        if constexpr(std::is_same_v<Binding, Member>)
        {
            if(GetMemberOffset(p) == GetMemberOffset(member))
            {
                instance->ModifyMember(index, sampler);
            }
        }
        ++index;
    };
    Struct::ForEachMember(processBinding);
}

template<BindingGroupStruct Struct, typename Member>
void ModifyBindingGroup(
    RHI::BindingGroup *instance,
    Member Struct:: *member,
    const RC<RHI::Buffer> &uniformBuffer,
    size_t offset,
    size_t range)
{
    int index = 0;
    auto processBinding = [&]<typename Binding>(Binding Struct:: * p, const char *)
    {
        if constexpr(std::is_same_v<Binding, Member>)
        {
            if(GetMemberOffset(p) == GetMemberOffset(member))
            {
                instance->ModifyMember(index, uniformBuffer, offset, range);
            }
        }
        ++index;
    };
    Struct::ForEachMember(processBinding);
}

RTRC_END
