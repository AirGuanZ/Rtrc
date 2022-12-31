#include <Rtrc/Math/LowDiscrepancy.h>
#include <Rtrc/Renderer/AtmosphereRenderer.h>

RTRC_BEGIN

namespace AtmosphereDetail
{

    rtrc_group(TransmittanceLutPass, Pass)
    {
        rtrc_define(RWTexture2D<float4>,   TransmittanceTextureRW);
        rtrc_uniform(int2,                 outputResolution);
        rtrc_uniform(AtmosphereProperties, atmosphere);
    };

    rtrc_group(MultiScatterLutPass, Pass)
    {
        rtrc_define(RWTexture2D<float4>,      MultiScatterTextureRW);
        rtrc_define(StructuredBuffer<float2>, RawDirSamples);
        rtrc_define(Texture2D<float3>,        TransmittanceLut);
        rtrc_uniform(int2,                    outputResolution);
        rtrc_uniform(int,                     dirSampleCount);
        rtrc_uniform(int,                     rayMarchStepCount);
        rtrc_uniform(AtmosphereProperties,    atmosphere);
        rtrc_uniform(float3,                  terrainAlbedo);
    };

    rtrc_group(SkyLutPass, Pass)
    {
        rtrc_define(RWTexture2D<float4>,   SkyLutTextureRW);
        rtrc_define(Texture2D<float3>,     TransmittanceLutTexture);
        rtrc_define(Texture2D<float3>,     MultiScatterLutTexture);
        rtrc_uniform(AtmosphereProperties, atmosphere);
        rtrc_uniform(int2,                 outputResolution);
        rtrc_uniform(int,                  rayMarchStepCount);
        rtrc_uniform(float3,               eyePos);
        rtrc_uniform(float3,               sunDirection);
        rtrc_uniform(float3,               sunIntensity);
    };

} // namespace AtmosphereDetail

AtmosphereDetail::TransmittanceLut::TransmittanceLut(
    const BuiltinResourceManager &builtinResources,
    const AtmosphereProperties   &atmosphere,
    const Vector2i               &resolution)
{
    Device &device = builtinResources.GetDevice();

    RC<Texture> lut = device.CreateTexture(RHI::TextureDesc
    {
        .dim                  = RHI::TextureDimension::Tex2D,
        .format               = RHI::Format::R32G32B32A32_Float,
        .width                = static_cast<uint32_t>(resolution.x),
        .height               = static_cast<uint32_t>(resolution.y),
        .arraySize            = 1,
        .mipLevels            = 1,
        .sampleCount          = 1,
        .usage                = RHI::TextureUsage::UnorderAccess | RHI::TextureUsage::ShaderResource,
        .initialLayout        = RHI::TextureLayout::Undefined,
        .concurrentAccessMode = RHI::QueueConcurrentAccessMode::Exclusive
    });
    srv_ = lut->CreateSRV();
    auto lutUav = lut->CreateUAV();

    auto material = builtinResources.GetBuiltinMaterial(BuiltinMaterial::Atmosphere);
    auto matPass = material->GetPassByTag("GenerateTransmittanceLUT");
    auto shader = matPass->GetShader();
    auto pipeline = shader->GetComputePipeline();

    TransmittanceLutPass passData;
    passData.TransmittanceTextureRW = lutUav;
    passData.outputResolution = resolution;
    passData.atmosphere = atmosphere;
    auto passGroup = device.CreateBindingGroup(passData);

    device.ExecuteBarrier(lut, RHI::TextureLayout::Undefined, RHI::TextureLayout::ShaderRWTexture);
    device.ExecuteAndWait([&](CommandBuffer &cmd)
    {
        cmd.BindPipeline(pipeline);
        cmd.BindComputeGroup(0, passGroup);
        cmd.Dispatch(shader->ComputeThreadGroupCount(Vector3i(resolution, 1)));
    });
    device.ExecuteBarrier(lut, RHI::TextureLayout::ShaderRWTexture, RHI::TextureLayout::ShaderTexture);
}

AtmosphereDetail::MultiScatterLut::MultiScatterLut(
    const BuiltinResourceManager &builtinResources,
    const TransmittanceLut       &transmittanceLut,
    const AtmosphereProperties   &properties,
    const Vector2i               &resolution)
{
    Device &device = builtinResources.GetDevice();

    RC<Buffer> poissonDiskSamples;
    {
        auto data = GeneratePoissonDiskSamples(DIR_SAMPLE_COUNT, 42);
        poissonDiskSamples = device.GetCopyContext().CreateBuffer(
            RHI::BufferDesc
            {
                .size = sizeof(Vector2f) * DIR_SAMPLE_COUNT,
                .usage = RHI::BufferUsage::ShaderStructuredBuffer,
                .hostAccessType = RHI::BufferHostAccessType::SequentialWrite
            }, data.data());
    }
    auto poissonDiskSamplesSrv = poissonDiskSamples->GetSRV();

    auto lut = device.CreateTexture(RHI::TextureDesc
    {
        .dim                  = RHI::TextureDimension::Tex2D,
        .format               = RHI::Format::R32G32B32A32_Float,
        .width                = static_cast<uint32_t>(resolution.x),
        .height               = static_cast<uint32_t>(resolution.y),
        .arraySize            = 1,
        .mipLevels            = 1,
        .sampleCount          = 1,
        .usage                = RHI::TextureUsage::ShaderResource | RHI::TextureUsage::UnorderAccess,
        .initialLayout        = RHI::TextureLayout::Undefined,
        .concurrentAccessMode = RHI::QueueConcurrentAccessMode::Exclusive
    });
    srv_ = lut->CreateSRV();

    auto material = builtinResources.GetBuiltinMaterial(BuiltinMaterial::Atmosphere);
    auto matPass = material->GetPassByTag("GenerateMultiScatterLUT");
    auto shader = matPass->GetShader();
    auto pipeline = shader->GetComputePipeline();

    MultiScatterLutPass passData;
    passData.MultiScatterTextureRW = lut->CreateUAV();
    passData.RawDirSamples = poissonDiskSamplesSrv;
    passData.TransmittanceLut = transmittanceLut.GetLut();
    passData.outputResolution = resolution;
    passData.dirSampleCount = DIR_SAMPLE_COUNT;
    passData.rayMarchStepCount = RAY_MARCH_STEP_COUNT;
    passData.atmosphere = properties;
    auto passGroup = device.CreateBindingGroup(passData);

    device.ExecuteBarrier(lut, RHI::TextureLayout::Undefined, RHI::TextureLayout::ShaderRWTexture);
    device.ExecuteAndWait([&](CommandBuffer &cmd)
    {
        cmd.BindPipeline(pipeline);
        cmd.BindComputeGroup(0, passGroup);
        cmd.Dispatch(shader->ComputeThreadGroupCount(Vector3i(resolution, 1)));
    });
    device.ExecuteBarrier(lut, RHI::TextureLayout::ShaderRWTexture, RHI::TextureLayout::ShaderTexture);
}

AtmosphereDetail::SkyLut::SkyLut(const BuiltinResourceManager &builtinResources)
    : device_(builtinResources.GetDevice()), properties_(nullptr)
{
    auto material = builtinResources.GetBuiltinMaterial(BuiltinMaterial::Atmosphere);
    auto pass = material->GetPassByTag("GenerateSkyLut");
    shader_ = pass->GetShader();
}

void AtmosphereDetail::SkyLut::SetRayMarchingStepCount(int stepCount)
{
    stepCount_ = stepCount;
}

void AtmosphereDetail::SkyLut::SetOutputResolution(const Vector2i &res)
{
    lutRes_ = res;
}

void AtmosphereDetail::SkyLut::SetCamera(const Vector3f &eyePos)
{
    eyePos_ = eyePos;
}

void AtmosphereDetail::SkyLut::SetAtomsphere(const AtmosphereProperties *properties)
{
    properties_ = properties;
}

void AtmosphereDetail::SkyLut::SetSun(const Vector3f &direction, const Vector3f &intensity)
{
    direction_ = direction;
    intensity_ = intensity;
}

AtmosphereDetail::SkyLut::RenderGraphInterface AtmosphereDetail::SkyLut::AddToRenderGraph(
    RG::RenderGraph              *renderGraph,
    const TransmittanceLut       &transmittanceLut,
    const MultiScatterLut        &multiScatterLut) const
{
    auto lut = renderGraph->CreateTexture2D(RHI::TextureDesc
    {
        .dim                  = RHI::TextureDimension::Tex2D,
        .format               = RHI::Format::R32G32B32A32_Float,
        .width                = static_cast<uint32_t>(lutRes_.x),
        .height               = static_cast<uint32_t>(lutRes_.y),
        .arraySize            = 1,
        .mipLevels            = 1,
        .sampleCount          = 1,
        .usage                = RHI::TextureUsage::UnorderAccess | RHI::TextureUsage::ShaderResource,
        .initialLayout        = RHI::TextureLayout::Undefined,
        .concurrentAccessMode = RHI::QueueConcurrentAccessMode::Exclusive
    });

    auto pass = renderGraph->CreatePass("Generate Sky Lut");
    pass->Use(lut, RG::COMPUTE_UNORDERED_ACCESS_WRITE);
    pass->SetCallback([&](RG::PassContext &passCtx)
    {
        SkyLutPass passGroupData;
        passGroupData.SkyLutTextureRW         = lut->Get();
        passGroupData.TransmittanceLutTexture = transmittanceLut.GetLut();
        passGroupData.MultiScatterLutTexture  = multiScatterLut.GetLut();
        passGroupData.atmosphere              = *properties_;
        passGroupData.outputResolution        = lutRes_;
        passGroupData.rayMarchStepCount       = stepCount_;
        passGroupData.eyePos                  = eyePos_;
        passGroupData.sunDirection            = direction_;
        passGroupData.sunIntensity            = intensity_;
        auto passGroup = device_.CreateBindingGroup(passGroupData);

        auto &cmd = passCtx.GetCommandBuffer();
        cmd.BindPipeline(shader_->GetComputePipeline());
        cmd.BindComputeGroup(0, passGroup);
        if(shader_->GetBindingGroupForInlineSamplers())
        {
            cmd.BindComputeGroup(
                shader_->GetBindingGroupIndexForInlineSamplers(), shader_->GetBindingGroupForInlineSamplers());
        }
        cmd.Dispatch(shader_->ComputeThreadGroupCount(Vector3i(lutRes_, 1)));
    });

    RenderGraphInterface ret;
    ret.inPass = pass;
    ret.outPass = pass;
    ret.skyLut = lut;
    return ret;
}

RTRC_END
