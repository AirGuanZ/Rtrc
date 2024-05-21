#pragma once

#include <Rtrc/Graphics/Shader/DSL/BindingGroup/ResourceProxy.h>

RTRC_BEGIN

namespace BindingGroupDetail
{

    enum class BindingFlagBit : uint8_t
    {
        Bindless = 1 << 0,
        VariableArraySize = 1 << 1,
    };
    RTRC_DEFINE_ENUM_FLAGS(BindingFlagBit)
    using BindingFlags = EnumFlagsBindingFlagBit;

    template<typename T>
    concept RtrcGroupStruct = requires { typename T::_rtrcGroupTypeFlag; };

    template<template<typename> typename Sketch>
    struct SketchRebindBase
    {
        template<typename B>
        using _rtrcRebindStructBuilder = Sketch<B>;
    };

    template<RtrcGroupStruct T, typename B>
    using RebindSketchBuilder = typename T::template _rtrcRebindStructBuilder<B>;

    template<RHI::ShaderStageFlags DefaultStages>
    struct DefaultStagesBase
    {
        static constexpr uint32_t _rtrcGroupDefaultStages = DefaultStages;
    };

    struct IndependentBase
    {
        static StructDetail::Sizer<1> _rtrcMemberCounter(...);
        auto operator<=>(const IndependentBase &) const = default;
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

    constexpr BindingFlags CreateBindingFlags(bool isBindless, bool hasVariableArraySize)
    {
        BindingFlags ret = 0;
        if(isBindless)
        {
            ret |= BindingFlagBit::Bindless;
        }
        if(hasVariableArraySize)
        {
            ret |= BindingFlagBit::VariableArraySize;
        }
        return ret;
    }

    struct BindingGroupStructBuilder_Default
    {
        struct Base
        {
            struct _rtrcGroupTypeFlag {};
        };

        template<typename T>
        using Member = T;
    };

    struct BindingGroupBuilder_eDSL;

    template<typename T>
    T instantiateWithVoidIfPossible();

    template<template<typename> typename T>
    T<void> instantiateWithVoidIfPossible();

} // namespace BindingGroupDetail

using BindingGroupDetail::RtrcGroupStruct;

#define rtrc_group(...)           RTRC_MACRO_OVERLOADING(rtrc_group, __VA_ARGS__)
#define rtrc_group1(NAME)         rtrc_group2(NAME, ::Rtrc::RHI::ShaderStage::All)
#define rtrc_group2(NAME, STAGES) RTRC_DEFINE_BINDING_GROUP(NAME, RTRC_INLINE_STAGE_EXPRESSION(STAGES))

#define rtrc_define(...)                 RTRC_MACRO_OVERLOADING(rtrc_define, __VA_ARGS__)
#define rtrc_define2(TYPE, NAME)         RTRC_DEFINE_BINDING_GROUP_VAR(TYPE, NAME, , _rtrcSelf##NAME::_rtrcGroupDefaultStages, false, false)
#define rtrc_define3(TYPE, NAME, STAGES) RTRC_DEFINE_BINDING_GROUP_VAR(TYPE, NAME, , RTRC_INLINE_STAGE_EXPRESSION(STAGES), false, false)

#define rtrc_define_array(...)                       RTRC_MACRO_OVERLOADING(rtrc_define, __VA_ARGS__)
#define rtrc_define_array3(TYPE, NAME, SIZE)         RTRC_DEFINE_BINDING_GROUP_VAR(TYPE, NAME, SIZE, _rtrcSelf##NAME::_rtrcGroupDefaultStages, false, false)
#define rtrc_define_array4(TYPE, NAME, SIZE, STAGES) RTRC_DEFINE_BINDING_GROUP_VAR(TYPE, NAME, SIZE, RTRC_INLINE_STAGE_EXPRESSION(STAGES), false, false)

#define rtrc_bindless(...)                       RTRC_MACRO_OVERLOADING(rtrc_bindless, __VA_ARGS__)
#define rtrc_bindless3(TYPE, NAME, SIZE)         RTRC_DEFINE_BINDING_GROUP_VAR(TYPE, NAME, SIZE, _rtrcSelf##NAME::_rtrcGroupDefaultStages, true, false)
#define rtrc_bindless4(TYPE, NAME, SIZE, STAGES) RTRC_DEFINE_BINDING_GROUP_VAR(TYPE, NAME, SIZE, RTRC_INLINE_STAGE_EXPRESSION(STAGES), true, false)

#define rtrc_bindless_varsize(...)                       RTRC_MACRO_OVERLOADING(rtrc_bindless_varsize, __VA_ARGS__)
#define rtrc_bindless_varsize3(TYPE, NAME, SIZE)         RTRC_DEFINE_BINDING_GROUP_VAR(TYPE, NAME, SIZE, _rtrcSelf##NAME::_rtrcGroupDefaultStages, true, true)
#define rtrc_bindless_varsize4(TYPE, NAME, SIZE, STAGES) RTRC_DEFINE_BINDING_GROUP_VAR(TYPE, NAME, SIZE, RTRC_INLINE_STAGE_EXPRESSION(STAGES), true, true)

#define rtrc_uniform(TYPE, NAME) RTRC_DEFINE_BINDING_GROUP_UNIFORM(TYPE, NAME)

#define rtrc_inline(...)                 RTRC_MACRO_OVERLOADING(rtrc_inline, __VA_ARGS__)
#define rtrc_inline2(TYPE, NAME)         RTRC_INLINE_BINDING_GROUP(TYPE, NAME, _rtrcSelf##NAME::_rtrcGroupDefaultStages)
#define rtrc_inline3(TYPE, NAME, STAGES) RTRC_INLINE_BINDING_GROUP(TYPE, NAME, RTRC_INLINE_STAGE_EXPRESSION(STAGES))

#define RTRC_INSTANTIATE_WITH_VOID_IF_POSSIBLE(T) \
    decltype(::Rtrc::BindingGroupDetail::instantiateWithVoidIfPossible<T>())

#define RTRC_DEFINE_BINDING_GROUP(NAME, DEFAULT_STAGES)                                                 \
    template<typename B> struct _rtrcGroupSketch##NAME;                                                 \
    using NAME = _rtrcGroupSketch##NAME<::Rtrc::BindingGroupDetail::BindingGroupStructBuilder_Default>; \
    using e##NAME = _rtrcGroupSketch##NAME<::Rtrc::BindingGroupDetail::BindingGroupBuilder_eDSL>;       \
    template<typename B>                                                                                \
    struct _rtrcGroupSketch##NAME :                                                                     \
        ::Rtrc::BindingGroupDetail::DefaultStagesBase<DEFAULT_STAGES>,                                  \
        ::Rtrc::BindingGroupDetail::IndependentBase,                                                    \
        ::Rtrc::BindingGroupDetail::SketchRebindBase<_rtrcGroupSketch##NAME>,                           \
        B::Base

// Translate expression like 'VS|FS' into RHI::ShaderStageFlags
#define RTRC_INLINE_STAGE_EXPRESSION(EXPR)                                   \
    ([]{                                                                     \
        using ::Rtrc::RHI::ShaderStage::VS;                                  \
        using ::Rtrc::RHI::ShaderStage::FS;                                  \
        using ::Rtrc::RHI::ShaderStage::CS;                                  \
        using ::Rtrc::RHI::ShaderStage::TS;                                  \
        using ::Rtrc::RHI::ShaderStage::MS;                                  \
        using ::Rtrc::RHI::ShaderStage::RT_RGS;                              \
        using ::Rtrc::RHI::ShaderStage::RT_MS;                               \
        using ::Rtrc::RHI::ShaderStage::RT_CHS;                              \
        using ::Rtrc::RHI::ShaderStage::RT_IS;                               \
        using ::Rtrc::RHI::ShaderStage::RT_AHS;                              \
        constexpr auto PS = ::Rtrc::RHI::ShaderStage::FS;                    \
        constexpr auto Classical = ::Rtrc::RHI::ShaderStage::AllClassical;   \
        constexpr auto Mesh      = ::Rtrc::RHI::ShaderStage::AllMesh;        \
        constexpr auto Callable  = ::Rtrc::RHI::ShaderStage::CallableShader; \
        constexpr auto RTCommon  = ::Rtrc::RHI::ShaderStage::AllRTCommon;    \
        constexpr auto RTHit     = ::Rtrc::RHI::ShaderStage::AllRTHit;       \
        constexpr auto RT        = ::Rtrc::RHI::ShaderStage::AllRT;          \
        using ::Rtrc::RHI::ShaderStage::All;                                 \
        return (EXPR);                                                       \
    }())

// Member access function f is called with f<isUniform>(memberPtr, name, stages, bindingFlags)
#define RTRC_DEFINE_BINDING_GROUP_VAR(TYPE, NAME, ARRAY_SPEC, STAGES, BINDLESS, VARSIZE)        \
    RTRC_DEFINE_SELF_TYPE(_rtrcSelf##NAME)                                                      \
    RTRC_META_STRUCT_SETUP_MEMBER(NAME)                                                         \
    using _rtrcRawMemberType##NAME =                                                            \
        RTRC_INSTANTIATE_WITH_VOID_IF_POSSIBLE(::Rtrc::BindingGroupDetail::MemberProxy_##TYPE); \
    using _rtrcMemberType##NAME =                                                               \
        typename B::template Member<_rtrcRawMemberType##NAME>ARRAY_SPEC;                        \
    _rtrcMemberType##NAME NAME;                                                                 \
    RTRC_META_STRUCT_PRE_MEMBER_ACCESS(NAME)                                                    \
        f.template operator()<false>(                                                           \
            &_rtrcSelf##NAME::NAME, #NAME, STAGES,                                              \
            ::Rtrc::BindingGroupDetail::CreateBindingFlags(BINDLESS, VARSIZE));                 \
    RTRC_META_STRUCT_POST_MEMBER_ACCESS(NAME)                                                   \
    using _requireComma##NAME = int

#define RTRC_DEFINE_BINDING_GROUP_UNIFORM(TYPE, NAME)                      \
    RTRC_DEFINE_SELF_TYPE(_rtrcSelf##NAME)                                 \
    RTRC_META_STRUCT_SETUP_MEMBER(NAME)                                    \
    using _rtrcMemberType##NAME = typename B::template Member<TYPE>;       \
    _rtrcMemberType##NAME NAME;                                            \
    RTRC_META_STRUCT_PRE_MEMBER_ACCESS(NAME)                               \
        f.template operator()<true>(                                       \
            &_rtrcSelf##NAME::NAME, #NAME,                                 \
            _rtrcSelf##NAME::_rtrcGroupDefaultStages,                      \
            ::Rtrc::BindingGroupDetail::CreateBindingFlags(false, false)); \
    RTRC_META_STRUCT_POST_MEMBER_ACCESS(NAME)                              \
    using _requireComma##NAME = int

#define RTRC_INLINE_BINDING_GROUP(TYPE, NAME, STAGES)                      \
    RTRC_DEFINE_SELF_TYPE(_rtrcSelf##NAME)                                 \
    RTRC_META_STRUCT_SETUP_MEMBER(NAME)                                    \
    using _rtrcMemberType##NAME = typename B::template Member<TYPE>;       \
    _rtrcMemberType##NAME NAME;                                            \
    RTRC_META_STRUCT_PRE_MEMBER_ACCESS(NAME)                               \
        f.template operator()<false>(                                      \
            &_rtrcSelf##NAME::NAME, #NAME, STAGES,                         \
            ::Rtrc::BindingGroupDetail::CreateBindingFlags(false, false)); \
    RTRC_META_STRUCT_POST_MEMBER_ACCESS(NAME)                              \
    using _requireComma##NAME = int

RTRC_END
