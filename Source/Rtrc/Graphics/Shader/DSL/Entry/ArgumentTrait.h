#pragma once

#include <magic_enum.hpp>

#include <Rtrc/Graphics/Device/BindingGroup.h>
#include <Rtrc/Graphics/Shader/DSL/eDSL.h>
#include <Rtrc/Graphics/Shader/DSL/Entry/ArgumentWrapper.h>
#include <Rtrc/ShaderCommon/Preprocess/ShaderPreprocessing.h>

RTRC_EDSL_BEGIN

namespace ArgumentTrait
{
    
    template<typename T>
    struct eResourceTrait
    {
        static constexpr bool IsResource = false;
    };

    template<TemplateBufferType>
    struct TemplateBufferTypeToArgumentWrapper;

#define ADD_BUFFER_ARGUMENT_WRAPPER(KEY, VALUE)                                        \
    template<>                                                                         \
    struct TemplateBufferTypeToArgumentWrapper<TemplateBufferType::KEY>                \
    {                                                                                  \
        using Type = ArgumentWrapper::ResourceArgumentWrapper<ArgumentWrapper::VALUE>; \
    }

    ADD_BUFFER_ARGUMENT_WRAPPER(Buffer,              BufferProxy<void>);
    ADD_BUFFER_ARGUMENT_WRAPPER(RWBuffer,            RWBufferProxy<void>);
    ADD_BUFFER_ARGUMENT_WRAPPER(StructuredBuffer,    StructuredBufferProxy<void>);
    ADD_BUFFER_ARGUMENT_WRAPPER(RWStructuredBuffer,  RWStructuredBufferProxy<void>);
    ADD_BUFFER_ARGUMENT_WRAPPER(ByteAddressBuffer,   ByteAddressBufferProxy);
    ADD_BUFFER_ARGUMENT_WRAPPER(RWByteAddressBuffer, RWByteAddressBufferProxy);

#undef ADD_BUFFER_ARGUMENT_WRAPPER

    template<typename T, TemplateBufferType Type>
    struct eResourceTrait<TemplateBuffer<T, Type>>
    {
        static constexpr bool IsResource = true;

        using ArgumentWrapperType = typename TemplateBufferTypeToArgumentWrapper<Type>::Type;

        static BindingGroupLayout::BindingDesc GetBindingDesc()
        {
            constexpr RHI::BindingType TYPE_MAP[] =
            {
                RHI::BindingType::Buffer,
                RHI::BindingType::StructuredBuffer,
                RHI::BindingType::ByteAddressBuffer,
                RHI::BindingType::RWBuffer,
                RHI::BindingType::RWStructuredBuffer,
                RHI::BindingType::RWByteAddressBuffer
            };
            static_assert(GetArraySize(TYPE_MAP) == magic_enum::enum_count<TemplateBufferType>());

            BindingGroupLayout::BindingDesc ret;
            ret.type = TYPE_MAP[std::to_underlying(Type)];
            return ret;
        }
    };

    template<typename T, bool IsRW>
    struct eResourceTrait<TemplateTexture1D<T, IsRW>>
    {
        static constexpr bool IsResource = true;

        using ArgumentWrapperType = std::conditional_t<
            IsRW,
            ArgumentWrapper::ResourceArgumentWrapper<ArgumentWrapper::RWTexture1DProxy<void>>,
            ArgumentWrapper::ResourceArgumentWrapper<ArgumentWrapper::Texture1DProxy<void>>>;

        static BindingGroupLayout::BindingDesc GetBindingDesc()
        {
            BindingGroupLayout::BindingDesc ret;
            ret.type = IsRW ? RHI::BindingType::Texture : RHI::BindingType::RWTexture;
            return ret;
        }
    };

    template<typename T, bool IsRW>
    struct eResourceTrait<TemplateTexture2D<T, IsRW>>
    {
        static constexpr bool IsResource = true;

        using ArgumentWrapperType = std::conditional_t<
            IsRW,
            ArgumentWrapper::ResourceArgumentWrapper<ArgumentWrapper::RWTexture2DProxy<void>>,
            ArgumentWrapper::ResourceArgumentWrapper<ArgumentWrapper::Texture2DProxy<void>>>;

        static BindingGroupLayout::BindingDesc GetBindingDesc()
        {
            BindingGroupLayout::BindingDesc ret;
            ret.type = IsRW ? RHI::BindingType::Texture : RHI::BindingType::RWTexture;
            return ret;
        }
    };

    template<typename T, bool IsRW>
    struct eResourceTrait<TemplateTexture3D<T, IsRW>>
    {
        static constexpr bool IsResource = true;

        using ArgumentWrapperType = std::conditional_t<
            IsRW,
            ArgumentWrapper::ResourceArgumentWrapper<ArgumentWrapper::RWTexture3DProxy<void>>,
            ArgumentWrapper::ResourceArgumentWrapper<ArgumentWrapper::Texture3DProxy<void>>>;

        static BindingGroupLayout::BindingDesc GetBindingDesc()
        {
            BindingGroupLayout::BindingDesc ret;
            ret.type = IsRW ? RHI::BindingType::Texture : RHI::BindingType::RWTexture;
            return ret;
        }
    };

    template<typename T, bool IsRW>
    struct eResourceTrait<TemplateTexture1DArray<T, IsRW>>
    {
        static constexpr bool IsResource = true;

        using ArgumentWrapperType = std::conditional_t<
            IsRW,
            ArgumentWrapper::ResourceArgumentWrapper<ArgumentWrapper::RWTexture1DArrayProxy<void>>,
            ArgumentWrapper::ResourceArgumentWrapper<ArgumentWrapper::Texture1DArrayProxy<void>>>;

        static BindingGroupLayout::BindingDesc GetBindingDesc()
        {
            BindingGroupLayout::BindingDesc ret;
            ret.type = IsRW ? RHI::BindingType::Texture : RHI::BindingType::RWTexture;
            return ret;
        }
    };

    template<typename T, bool IsRW>
    struct eResourceTrait<TemplateTexture2DArray<T, IsRW>>
    {
        static constexpr bool IsResource = true;

        using ArgumentWrapperType = std::conditional_t<
            IsRW,
            ArgumentWrapper::ResourceArgumentWrapper<ArgumentWrapper::RWTexture2DArrayProxy<void>>,
            ArgumentWrapper::ResourceArgumentWrapper<ArgumentWrapper::Texture2DArrayProxy<void>>>;

        static BindingGroupLayout::BindingDesc GetBindingDesc()
        {
            BindingGroupLayout::BindingDesc ret;
            ret.type = IsRW ? RHI::BindingType::Texture : RHI::BindingType::RWTexture;
            return ret;
        }
    };

    template<typename T, bool IsRW>
    struct eResourceTrait<TemplateTexture3DArray<T, IsRW>>
    {
        static constexpr bool IsResource = true;

        using ArgumentWrapperType = std::conditional_t<
            IsRW,
            ArgumentWrapper::ResourceArgumentWrapper<ArgumentWrapper::RWTexture3DArrayProxy<void>>,
            ArgumentWrapper::ResourceArgumentWrapper<ArgumentWrapper::Texture3DArrayProxy<void>>>;

        static BindingGroupLayout::BindingDesc GetBindingDesc()
        {
            BindingGroupLayout::BindingDesc ret;
            ret.type = IsRW ? RHI::BindingType::Texture : RHI::BindingType::RWTexture;
            return ret;
        }
    };

    template<typename T>
    struct eResourceTrait<eConstantBuffer<T>>
    {
        static constexpr bool IsResource = true;

        using ArgumentWrapperType =
            ArgumentWrapper::ResourceArgumentWrapper<ArgumentWrapper::ConstantBufferProxy<void>>;

        static BindingGroupLayout::BindingDesc GetBindingDesc()
        {
            BindingGroupLayout::BindingDesc ret;
            ret.type = RHI::BindingType::ConstantBuffer;
            return ret;
        }
    };

    template<>
    struct eResourceTrait<eRaytracingAccelerationStructure>
    {
        static constexpr bool IsResource = true;

        using ArgumentWrapperTrait =
            ArgumentWrapper::ResourceArgumentWrapper<ArgumentWrapper::RaytracingAccelerationStructureProxy>;

        static BindingGroupLayout::BindingDesc GetBindingDesc()
        {
            BindingGroupLayout::BindingDesc ret;
            ret.type = RHI::BindingType::AccelerationStructure;
            return ret;
        }
    };

    template<>
    struct eResourceTrait<eSamplerState>
    {
        static constexpr bool IsResource = true;

        using ArgumentWrapperTrait = ArgumentWrapper::ResourceArgumentWrapper<ArgumentWrapper::SamplerStateProxy>;

        static BindingGroupLayout::BindingDesc GetBindingDesc()
        {
            BindingGroupLayout::BindingDesc ret;
            ret.type = RHI::BindingType::Sampler;
            return ret;
        }
    };

    template<typename T>
    struct eValueTrait
    {
        static constexpr bool IsValue = false;
    };

#define ADD_EVALUE_TRAIT(ETYPE, TYPE, NTYPE)                               \
    template<>                                                             \
    struct eValueTrait<ETYPE>                                              \
    {                                                                      \
        static constexpr bool IsValue = true;                              \
        static constexpr ShaderUniformType Type = ShaderUniformType::TYPE; \
        using NativeType = NTYPE;                                          \
    }

    ADD_EVALUE_TRAIT(i32,   Int,  int32_t);
    ADD_EVALUE_TRAIT(i32x2, Int2, Vector2i);
    ADD_EVALUE_TRAIT(i32x3, Int3, Vector3i);
    ADD_EVALUE_TRAIT(i32x4, Int4, Vector4i);

    ADD_EVALUE_TRAIT(u32,   UInt,  uint32_t);
    ADD_EVALUE_TRAIT(u32x2, UInt2, Vector2u);
    ADD_EVALUE_TRAIT(u32x3, UInt3, Vector3u);
    ADD_EVALUE_TRAIT(u32x4, UInt4, Vector4u);

    ADD_EVALUE_TRAIT(f32,   Float,  float);
    ADD_EVALUE_TRAIT(f32x2, Float2, Vector2f);
    ADD_EVALUE_TRAIT(f32x3, Float3, Vector3f);
    ADD_EVALUE_TRAIT(f32x4, Float4, Vector4f);

    ADD_EVALUE_TRAIT(float4x4, Float4x4, Matrix4x4f);

#undef ADD_EVALUE_TRAIT

} // namespace ArgumentTrait

RTRC_EDSL_END
