#include <Rtrc/Math/LowDiscrepancy.h>
#include <Rtrc/Renderer/Atmosphere/AtmosphereRenderer.h>

RTRC_RENDERER_BEGIN

namespace AtmosphereDetail
{

    rtrc_group(TransmittanceLutPass)
    {
        rtrc_define(RWTexture2D,           TransmittanceTextureRW);
        rtrc_uniform(int2,                 outputResolution);
        rtrc_uniform(AtmosphereProperties, atmosphere);
    };

    rtrc_group(MultiScatterLutPass)
    {
        rtrc_define(RWTexture2D,           MultiScatterTextureRW);
        rtrc_define(StructuredBuffer,      RawDirSamples);
        rtrc_define(Texture2D,             TransmittanceLut);
        rtrc_uniform(int2,                 outputResolution);
        rtrc_uniform(int,                  dirSampleCount);
        rtrc_uniform(int,                  rayMarchStepCount);
        rtrc_uniform(AtmosphereProperties, atmosphere);
        rtrc_uniform(float3,               terrainAlbedo);
    };

    rtrc_group(SkyLutPass)
    {
        rtrc_define(RWTexture2D,           SkyLutTextureRW);
        rtrc_define(Texture2D,             SkyHistoryLutTexture);
        rtrc_define(Texture2D,             TransmittanceLutTexture);
        rtrc_define(Texture2D,             MultiScatterLutTexture);
        rtrc_uniform(AtmosphereProperties, atmosphere);
        rtrc_uniform(int2,                 outputResolution);
        rtrc_uniform(int,                  rayMarchStepCount);
        rtrc_uniform(float,                frameRandom01);
        rtrc_uniform(float,                eyePosY);
        rtrc_uniform(float3,               sunDirection);
        rtrc_uniform(float,                lerpFactor);
        rtrc_uniform(float3,               sunIntensity);
    };

} // namespace AtmosphereDetail

AtmosphereDetail::TransmittanceLut::TransmittanceLut(
    const BuiltinResourceManager &builtinResources,
    const AtmosphereProperties   &atmosphere,
    const Vector2i               &resolution)
{
    Device &device = *builtinResources.GetDevice();

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
    lut->SetName("TransmittanceLut");
    srv_ = lut->CreateSrv();
    auto lutUav = lut->CreateUav();

    auto material = builtinResources.GetBuiltinMaterial(BuiltinMaterial::Atmosphere);
    auto matPass = material->GetPassByTag("GenerateTransmittanceLut");
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
        cmd.BindComputePipeline(pipeline);
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
    Device &device = *builtinResources.GetDevice();

    RC<Buffer> poissonDiskSamples;
    {
        auto data = GeneratePoissonDiskSamples(DIR_SAMPLE_COUNT, 42);
        poissonDiskSamples = device.CreateAndUploadBuffer(
            RHI::BufferDesc
            {
                .size = sizeof(Vector2f) * DIR_SAMPLE_COUNT,
                .usage = RHI::BufferUsage::ShaderStructuredBuffer,
                .hostAccessType = RHI::BufferHostAccessType::SequentialWrite
            }, data.data());
    }
    poissonDiskSamples->SetName("PoissonDiskSamples");
    poissonDiskSamples->SetDefaultStructStride(sizeof(Vector2f));
    auto poissonDiskSamplesSrv = poissonDiskSamples->GetStructuredSrv();

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
    lut->SetName("MultiScatterLut");
    srv_ = lut->CreateSrv();

    auto material = builtinResources.GetBuiltinMaterial(BuiltinMaterial::Atmosphere);
    auto matPass = material->GetPassByTag("GenerateMultiScatterLut");
    auto shader = matPass->GetShader();
    auto pipeline = shader->GetComputePipeline();

    MultiScatterLutPass passData;
    passData.MultiScatterTextureRW = lut->CreateUav();
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
        cmd.BindComputePipeline(pipeline);
        cmd.BindComputeGroup(0, passGroup);
        cmd.Dispatch(shader->ComputeThreadGroupCount(Vector3i(resolution, 1)));
    });
    device.ExecuteBarrier(lut, RHI::TextureLayout::ShaderRWTexture, RHI::TextureLayout::ShaderTexture);
}

AtmosphereDetail::SkyLut::SkyLut(const BuiltinResourceManager &builtinResources)
    : device_(builtinResources.GetDevice()), randomEngine_(42), stepCount_(0)
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

AtmosphereDetail::SkyLut::RenderGraphInterface AtmosphereDetail::SkyLut::AddToRenderGraph(
    const AtmosphereFrameParameters *parameters,
    RG::RenderGraph                 *renderGraph,
    const TransmittanceLut          &transmittanceLut,
    const MultiScatterLut           &multiScatterLut)
{
    std::swap(prevLut_, currLut_);

    auto prevLutRG = PrepareLut(renderGraph, prevLut_);
    auto currLutRG = PrepareLut(renderGraph, currLut_);

    auto prevLut = prevLutRG.lut;
    auto currLut = currLutRG.lut;
    
    auto pass = renderGraph->CreatePass("Generate Sky Lut");
    pass->Use(currLut, RG::COMPUTE_SHADER_RWTEXTURE_WRITEONLY);
    pass->Use(prevLut, RG::COMPUTE_SHADER_TEXTURE);
    pass->SetCallback(
        [this, currLut, prevLut, parameters, &transmittanceLut, &multiScatterLut](RG::PassContext &passCtx)
    {
        float lerpFactor = 1.0f - std::pow(0.03f, 0.4f * parameters->dt);
        lerpFactor = std::clamp(lerpFactor, 0.001f, 0.05f);

        SkyLutPass passGroupData;
        passGroupData.SkyLutTextureRW         = currLut->Get(passCtx);
        passGroupData.SkyHistoryLutTexture    = prevLut->Get(passCtx);
        passGroupData.TransmittanceLutTexture = transmittanceLut.GetLut();
        passGroupData.MultiScatterLutTexture  = multiScatterLut.GetLut();
        passGroupData.atmosphere              = parameters->atmosphere;
        passGroupData.outputResolution        = lutRes_;
        passGroupData.rayMarchStepCount       = stepCount_;
        passGroupData.frameRandom01           = std::uniform_real_distribution<float>(0, 1)(randomEngine_);
        passGroupData.eyePosY                 = parameters->eyePosition.y;
        passGroupData.lerpFactor              = lerpFactor;
        passGroupData.sunDirection            = parameters->sunDirection;
        passGroupData.sunIntensity            = parameters->sunIntensity * parameters->sunColor;
        auto passGroup = device_->CreateBindingGroup(passGroupData);

        auto &cmd = passCtx.GetCommandBuffer();
        cmd.BindComputePipeline(shader_->GetComputePipeline());
        cmd.BindComputeGroup(0, passGroup);
        cmd.Dispatch(shader_->ComputeThreadGroupCount(Vector3i(lutRes_, 1)));
    });

    RenderGraphInterface ret;
    ret.skyLut = currLut;
    return ret;
}

AtmosphereDetail::SkyLut::PrepareLutRGInterface AtmosphereDetail::SkyLut::PrepareLut(
    RG::RenderGraph *renderGraph, RC<StatefulTexture> &lut)
{
    PrepareLutRGInterface ret;
    if(lut && lut->GetWidth() == lutRes_.x && lut->GetHeight() == lutRes_.y)
    {
        ret.clearPass = renderGraph->CreateDummyPass("Clear Sky Lut");
        ret.lut = renderGraph->RegisterTexture(lut);
        return ret;
    }

    lut = device_->CreatePooledTexture(RHI::TextureDesc
    {
        .dim                  = RHI::TextureDimension::Tex2D,
        .format               = RHI::Format::R32G32B32A32_Float,
        .width                = static_cast<uint32_t>(lutRes_.x),
        .height               = static_cast<uint32_t>(lutRes_.y),
        .arraySize            = 1,
        .mipLevels            = 1,
        .sampleCount          = 1,
        .usage                = RHI::TextureUsage::UnorderAccess |
                                RHI::TextureUsage::ShaderResource |
                                RHI::TextureUsage::TransferDst,
        .initialLayout        = RHI::TextureLayout::Undefined,
        .concurrentAccessMode = RHI::QueueConcurrentAccessMode::Exclusive
    });

    ret.lut = renderGraph->RegisterTexture(lut);
    ret.clearPass = renderGraph->CreateClearTexture2DPass("Clear Sky Lut", ret.lut, { 0, 0, 0, 0 });
    return ret;
}

AtmosphereRenderer::AtmosphereRenderer(const BuiltinResourceManager &builtinResources)
    : device_(builtinResources.GetDevice()), builtinResources_(builtinResources)
{
    SetSunDirection(Vector3f(0, -1, 0));
    SetSunIntensity(10);
    SetSunColor({ 1, 1, 1 });
    yOffset_ = 0;

    transLutRes_ = { 256, 256 };
    msLutRes_ = { 256, 256 };
    
    skyLut_ = MakeBox<SkyLut>(builtinResources_);
    skyLut_->SetRayMarchingStepCount(24);
    skyLut_->SetOutputResolution({ 256, 128 });
}

void AtmosphereDetail::AtmosphereRenderer::SetSunDirection(const Vector3f &normalizedSunToScene)
{
    frameParameters_.sunDirection = normalizedSunToScene;
}

void AtmosphereDetail::AtmosphereRenderer::SetSunIntensity(float intensity)
{
    frameParameters_.sunIntensity = intensity;
}

void AtmosphereDetail::AtmosphereRenderer::SetSunColor(const Vector3f &color)
{
    frameParameters_.sunColor = color;
}

void AtmosphereDetail::AtmosphereRenderer::SetProperties(const AtmosphereProperties &properties)
{
    if(frameParameters_.atmosphere != properties)
    {
        frameParameters_.atmosphere = properties;
        transLut_ = {};
        msLut_ = {};
    }
}

void AtmosphereRenderer::SetYOffset(float offset)
{
    yOffset_ = offset;
}

void AtmosphereDetail::AtmosphereRenderer::SetTransmittanceLutResolution(const Vector2i &res)
{
    if(res != transLutRes_)
    {
        transLutRes_ = res;
        transLut_ = {};
    }
}

void AtmosphereDetail::AtmosphereRenderer::SetMultiScatterLutResolution(const Vector2i &res)
{
    if(res != msLutRes_)
    {
        msLutRes_ = res;
        msLut_ = {};
    }
}

void AtmosphereRenderer::SetSkyLutResolution(const Vector2i &res)
{
    skyLut_->SetOutputResolution(res);
}

AtmosphereRenderer::RenderGraphInterface AtmosphereDetail::AtmosphereRenderer::AddToRenderGraph(
    RG::RenderGraph &graph, const Camera &camera, float dt)
{
    if(!transLut_)
    {
        transLut_ = MakeBox<TransmittanceLut>(builtinResources_, frameParameters_.atmosphere, transLutRes_);
    }
    if(!msLut_)
    {
        msLut_ = MakeBox<MultiScatterLut>(builtinResources_, *transLut_, frameParameters_.atmosphere, msLutRes_);
    }

    frameParameters_.eyePosition = camera.GetPosition();
    frameParameters_.eyePosition.y += yOffset_;
    frameParameters_.dt = dt;

    assert(skyLut_);
    auto skyRGData = skyLut_->AddToRenderGraph(&frameParameters_, &graph, *transLut_, *msLut_);

    RenderGraphInterface ret;
    ret.skyLut = skyRGData.skyLut;
    return ret;
}

RTRC_RENDERER_END
