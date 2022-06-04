#pragma once

#include <Rtrc/RHI/RHI.h>
#include <Rtrc/Utils/EnumFlags.h>
#include <Rtrc/Utils/MacroForEach.h>
#include <Rtrc/Utils/String.h>
#include <Rtrc/Utils/Struct.h>
#include <Rtrc/Utils/TemplateStringParameter.h>

RTRC_BEGIN

namespace ShaderDetail
{

    enum class MemberTag : StructDetail::TagUnderlyingType
    {
        None         = 0b0000000,
        Binding      = 0b0000001,
        CBuffer      = 0b0000010,
        StructMember = 0b0000100,
        All          = 0b1111111
    };

    RTRC_DEFINE_ENUM_FLAGS(MemberTag)

    template<typename Ret>
    constexpr Ret BoolListToValue()
    {
        return Ret{};
    }

    template<typename Ret, typename V1, typename...Vs>
    constexpr Ret BoolListToValue(bool isFirst, V1 value1, Vs...vs)
    {
        return isFirst ? Ret(value1) : BoolListToValue<Ret>(vs...);
    }

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
        static constexpr auto IsArray      = ExtractIsArray<ParamWrappers...>();
        static constexpr auto ArraySize    = IsArray ? ExtractArraySize<ParamWrappers...>() : -1;
        static constexpr auto ShaderStages = ExtractShaderStages(ParamWrappers...);
    };

} // namespace ShaderDetail

// ===================== struct =====================

#define RTRC_STRUCT_BEGIN RTRC_META_STRUCT_BEGIN
#define RTRC_STRUCT_END   RTRC_META_STRUCT_END

#define RTRC_STRUCT_MEMBER(TYPE, NAME)                                                  \
    RTRC_META_STRUCT_PRE_MEMBER(::Rtrc::ShaderDetail::MemberTag::StructMember, NAME)    \
    {                                                                                   \
        f.template operator()(&_rtrcSelf::NAME, #NAME);                                 \
    }                                                                                   \
    RTRC_META_STRUCT_POST_MEMBER(NAME)                                                  \
    using _rtrcType##NAME = TYPE;                                                       \
    _rtrcType##NAME NAME;

// ===================== cbuffer =====================

#define RTRC_CBUFFER_USING_EXTERNAL_TYPE(TYPE, NAME, ...)                                           \
    using _rtrcBindingParamExtractor##NAME = ::Rtrc::ShaderDetail::BindingParamExtractor<           \
        RTRC_MACRO_FOREACH_1(RTRC_BINDING_PARAMETER_WRAPPER, __VA_ARGS__)                           \
            ::Rtrc::ShaderDetail::DUMMY_BINDING_PARAM_WRAPPER>;                                     \
    static constexpr auto _rtrcShaderStages##NAME = _rtrcBindingParamExtractor##NAME::ShaderStages; \
    RTRC_META_STRUCT_PRE_MEMBER(::Rtrc::ShaderDetail::MemberTag::CBuffer, NAME)                     \
    {                                                                                               \
        f.template operator()(&_rtrcSelf::NAME, #NAME, _rtrcShaderStages##NAME);                    \
    }                                                                                               \
    RTRC_META_STRUCT_POST_MEMBER(NAME)                                                              \
    using _rtrcType##NAME = TYPE;                                                                   \
    _rtrcType##NAME NAME;

// ===================== binding =====================

template<
    TemplateStringParameter TypeNameParam,
    TemplateStringParameter NameParam,
    int                     ArraySizeParam,
    RHI::ShaderStageFlag       TShaderStages>
class BindingSlot
{
public:

    static constexpr bool IsBuffer             = StartsWith(TypeNameParam.GetString(), "Buffer");
    static constexpr bool IsTexture2D          = StartsWith(TypeNameParam.GetString(), "Texture2D");
    static constexpr bool IsStructuredBuffer   = StartsWith(TypeNameParam.GetString(), "StructuredBuffer");
    static constexpr bool IsRWBuffer           = StartsWith(TypeNameParam.GetString(), "RWBuffer");
    static constexpr bool IsRWTexture2D        = StartsWith(TypeNameParam.GetString(), "RWTexture2D");
    static constexpr bool IsRWStructuredBuffer = StartsWith(TypeNameParam.GetString(), "RWStructuredBuffer");
    static constexpr bool IsSampler            = StartsWith(TypeNameParam.GetString(), "Sampler");

    static_assert(
        IsBuffer   || IsTexture2D   || IsStructuredBuffer   ||
        IsRWBuffer || IsRWTexture2D || IsRWStructuredBuffer ||
        IsSampler, "unknown binding type");

    static constexpr RHI::BindingType BindingType = ShaderDetail::BoolListToValue<RHI::BindingType>(
        IsBuffer,             RHI::BindingType::Buffer,
        IsTexture2D,          RHI::BindingType::Texture,
        IsStructuredBuffer,   RHI::BindingType::StructuredBuffer,
        IsRWBuffer,           RHI::BindingType::RWBuffer,
        IsRWTexture2D,        RHI::BindingType::RWTexture,
        IsRWStructuredBuffer, RHI::BindingType::RWStructuredBuffer,
        IsSampler,            RHI::BindingType::Sampler);

    static constexpr std::string_view     TypeName     = TypeNameParam.GetString();
    static constexpr std::string_view     Name         = NameParam.GetString();
    static constexpr bool                 IsArray      = ArraySizeParam >= 0;
    static constexpr int                  ArraySize    = ArraySizeParam;
    static constexpr RHI::ShaderStageFlag ShaderStages = TShaderStages;

    using Value = Variant<std::monostate, RC<RHI::Texture2DSRV>>;

    RC<RHI::Texture2DSRV> GetTexture2DSRV() const
    {
        return value_.As<RC<RHI::Texture2DSRV>>();
    }

    BindingSlot &operator=(const Value &value)
    {
        assert(!IsTexture2D || value.Is<RC<RHI::Texture2DSRV>>());
        value_ = value;
        return *this;
    }

private:

    Value value_ = std::monostate{};
};

#define RTRC_BINDING_GROUP_BEGIN(NAME)                             \
    RTRC_META_STRUCT_BEGIN(NAME)                                   \
    struct _rtrcBindingGroupTag { };                               \
    static constexpr auto VS  = ::Rtrc::RHI::ShaderStageFlags::VS; \
    static constexpr auto PS  = ::Rtrc::RHI::ShaderStageFlags::PS; \
    static constexpr auto All = ::Rtrc::RHI::ShaderStageFlags::All;
#define RTRC_BINDING_GROUP_END() RTRC_META_STRUCT_END()

template<typename T>
concept BindingGroupStruct = requires{ typename T::_rtrcBindingGroupTag; };

#define RTRC_BINDING_PARAMETER_WRAPPER(X) ::Rtrc::ShaderDetail::BindingParamWrapper(X),

#define RTRC_BINDING_GROUP_MEMBER(TYPE, NAME, ...)                                                                 \
    using _rtrcBindingParamExtractor##NAME =                                                                       \
        ::Rtrc::ShaderDetail::BindingParamExtractor<                                                               \
            RTRC_MACRO_FOREACH_1(RTRC_BINDING_PARAMETER_WRAPPER, __VA_ARGS__)                                      \
                ::Rtrc::ShaderDetail::DUMMY_BINDING_PARAM_WRAPPER>;                                                \
    static constexpr auto _rtrcArraySize##NAME = _rtrcBindingParamExtractor##NAME::ArraySize;                      \
    static constexpr auto _rtrcShaderStages##NAME = _rtrcBindingParamExtractor##NAME::ShaderStages;                \
    RTRC_META_STRUCT_PRE_MEMBER(::Rtrc::ShaderDetail::MemberTag::Binding, NAME)                                    \
    {                                                                                                              \
        f.template operator()<::Rtrc::BindingSlot<#TYPE, #NAME, _rtrcArraySize##NAME, _rtrcShaderStages##NAME>>(); \
    }                                                                                                              \
    RTRC_META_STRUCT_POST_MEMBER(NAME)                                                                             \
    ::Rtrc::BindingSlot<#TYPE, #NAME, _rtrcArraySize##NAME, _rtrcShaderStages##NAME> NAME;

#define $group_begin  RTRC_BINDING_GROUP_BEGIN
#define $group_end    RTRC_BINDING_GROUP_END
#define $binding      RTRC_BINDING_GROUP_MEMBER
#define $struct_begin RTRC_STRUCT_BEGIN
#define $struct_end   RTRC_STRUCT_END
#define $variable     RTRC_STRUCT_MEMBER
#define $cbuffer      RTRC_CBUFFER_USING_EXTERNAL_TYPE

template<BindingGroupStruct Struct>
const RHI::BindingGroupLayoutDesc &GetBindingGroupLayoutDesc();

template<BindingGroupStruct Struct, typename Member>
void ModifyBindingGroupInstance(
    const RC<RHI::BindingGroupInstance> &instance,
    Member Struct::                     *member,
    const RC<RHI::Texture2DSRV>         &srv);

RTRC_END

#include <Rtrc/RHI/Shader/Type.inl>
