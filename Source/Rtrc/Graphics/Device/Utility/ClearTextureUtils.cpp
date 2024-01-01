#include <Rtrc/Graphics/Device/Utility/ClearTextureUtils.h>
#include <Rtrc/Graphics/Shader/StandaloneCompiler.h>

RTRC_BEGIN

namespace ClearTextureUtilsDetail
{
    const char *SHADER_SOURCE_CLEAR = R"___(
rtrc_comp(CSMain)
rtrc_group(Pass)
{
    rtrc_define(RWTexture2D<OUTPUT_TYPE>, Output)
    rtrc_uniform(uint2, resolution)
    rtrc_uniform(VALUE_TYPE, value)
};
[numthreads(8, 8, 1)]
void CSMain(uint2 tid : SV_DispatchThreadID)
{
    if(all(tid < Pass.resolution))
        Output[tid] = Pass.value;
}
)___";

    template<typename T>
    rtrc_group(Pass)
    {
        rtrc_define(RWTexture2D, Output);
        rtrc_uniform(uint2, resolution);
        rtrc_uniform(T, value);
    };

}

ClearTextureUtils::ClearTextureUtils(Ref<Device> device)
    : device_(device)
{
    StandaloneShaderCompiler shaderCompiler;
    shaderCompiler.SetDevice(device);

    const std::string source = ClearTextureUtilsDetail::SHADER_SOURCE_CLEAR;
    clearFloat4Shader_ = shaderCompiler.Compile(
        source, { { "OUTPUT_TYPE", "float4" }, { "VALUE_TYPE", "float4" } }, RTRC_DEBUG);
    clearFloat2Shader_ = shaderCompiler.Compile(
        source, { { "OUTPUT_TYPE", "float2" }, { "VALUE_TYPE", "float2" } }, RTRC_DEBUG);
    clearFloatShader_ = shaderCompiler.Compile(
        source, { { "OUTPUT_TYPE", "float" }, { "VALUE_TYPE", "float" } }, RTRC_DEBUG);
    clearUIntShader_ = shaderCompiler.Compile(
        source, { { "OUTPUT_TYPE", "uint" }, { "VALUE_TYPE", "uint" } }, RTRC_DEBUG);
    clearUInt4Shader_ = shaderCompiler.Compile(
        source, { { "OUTPUT_TYPE", "uint4" }, { "VALUE_TYPE", "uint4" } }, RTRC_DEBUG);
    clearUNorm4Shader_ = shaderCompiler.Compile(
        source, { { "OUTPUT_TYPE", "unorm float4" }, { "VALUE_TYPE", "float4" } }, RTRC_DEBUG);
    clearUNorm2Shader_ = shaderCompiler.Compile(
        source, { { "OUTPUT_TYPE", "unorm float2" }, { "VALUE_TYPE", "float2" } }, RTRC_DEBUG);
    clearUNormShader_ = shaderCompiler.Compile(
        source, { { "OUTPUT_TYPE", "unorm float" }, { "VALUE_TYPE", "float" } }, RTRC_DEBUG);
}

void ClearTextureUtils::ClearRWTexture2D(
    CommandBuffer &commandBuffer, const TextureUav &uav, const Vector4f &value) const
{
    RHI::Format format = uav.GetRHIObject()->GetDesc().format;
    if(format == RHI::Format::Unknown)
    {
        format = uav.GetTexture()->GetFormat();
    }
    switch(format)
    {
    case RHI::Format::B8G8R8A8_UNorm:
    case RHI::Format::R8G8B8A8_UNorm:
        return ClearRWTexture2DImpl(clearUNorm4Shader_, commandBuffer, uav, value);
    case RHI::Format::R32_Float:
        return ClearRWTexture2DImpl(clearFloatShader_, commandBuffer, uav, value.x);
    case RHI::Format::R32G32_Float:
        return ClearRWTexture2DImpl(clearFloat2Shader_, commandBuffer, uav, Vector2f(value.x, value.y));
    case RHI::Format::R32G32B32A32_Float:
        return ClearRWTexture2DImpl(clearFloat4Shader_, commandBuffer, uav, value);
    case RHI::Format::R16G16_Float:
        return ClearRWTexture2DImpl(clearFloat2Shader_, commandBuffer, uav, Vector2f(value.x, value.y));
    case RHI::Format::R8_UNorm:
    case RHI::Format::R16_UNorm:
        return ClearRWTexture2DImpl(clearUNormShader_, commandBuffer, uav, value.x);
    case RHI::Format::R16G16_UNorm:
        return ClearRWTexture2DImpl(clearUNorm2Shader_, commandBuffer, uav, Vector2f(value.x, value.y));
    default:
        throw Exception(fmt::format(
            "ClearTextureUtils::ClearRWTexture2D(float): unsupported uav format {}", GetFormatName(format)));
    }
}

void ClearTextureUtils::ClearRWTexture2D(
    CommandBuffer &commandBuffer, const TextureUav &uav, const Vector4u &value) const
{
    RHI::Format format = uav.GetRHIObject()->GetDesc().format;
    if(format == RHI::Format::Unknown)
    {
        format = uav.GetTexture()->GetFormat();
    }
    switch(format)
    {
    case RHI::Format::R16_UInt:
    case RHI::Format::R32_UInt:
        return ClearRWTexture2DImpl(clearUIntShader_, commandBuffer, uav, value.x);
    case RHI::Format::R32G32B32A32_UInt:
        return ClearRWTexture2DImpl(clearUInt4Shader_, commandBuffer, uav, value);
    default:
        throw Exception(fmt::format(
            "ClearTextureUtils::ClearRWTexture2D(uint): unsupported uav format {}", GetFormatName(format)));
    }
}

void ClearTextureUtils::ClearRWTexture2D(
    CommandBuffer &commandBuffer, const TextureUav &uav, const Vector4i &value) const
{
    RHI::Format format = uav.GetRHIObject()->GetDesc().format;
    if(format == RHI::Format::Unknown)
    {
        format = uav.GetTexture()->GetFormat();
    }
    throw Exception(fmt::format(
        "ClearTextureUtils::ClearRWTexture2D(int): unsupported uav format {}", GetFormatName(format)));
}

template<typename ValueType>
void ClearTextureUtils::ClearRWTexture2DImpl(
    const RC<Shader> &shader, CommandBuffer &commandBuffer, const TextureUav &uav, const ValueType &value) const
{
    ClearTextureUtilsDetail::Pass<ValueType> passData;
    passData.Output     = uav;
    passData.resolution = uav.GetTexture()->GetSize();
    passData.value      = value;
    auto passGroup = device_->CreateBindingGroupWithCachedLayout(passData);
    commandBuffer.BindComputePipeline(shader->GetComputePipeline());
    commandBuffer.BindComputeGroup(0, passGroup);
    commandBuffer.DispatchWithThreadCount(passData.resolution);
}

RTRC_END
