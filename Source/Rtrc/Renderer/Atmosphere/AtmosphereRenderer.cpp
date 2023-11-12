#include <Core/Math/LowDiscrepancy.h>
#include <Rtrc/Renderer/Atmosphere/AtmosphereRenderer.h>

#include <Rtrc/Renderer/Atmosphere/Shader/Atmosphere.shader.outh>

RTRC_RENDERER_BEGIN

void RenderAtmosphere::SetLutTResolution(const Vector2u &res)
{
    assert(!T_);
    if(resT_ != res)
    {
        resT_ = res;
        persistentT_ = nullptr;
    }
}

void RenderAtmosphere::SetLutMResolution(const Vector2u &res)
{
    assert(!M_);
    if(resM_ != res)
    {
        resM_ = res;
        persistentM_ = nullptr;
    }
}

void RenderAtmosphere::SetLutSResolution(const Vector2u &res)
{
    resS_ = res;
}

void RenderAtmosphere::SetRayMarchingStepCount(int stepCount)
{
    rayMarchingStepCount_ = stepCount;
}

void RenderAtmosphere::Update(RG::RenderGraph &renderGraph, const AtmosphereProperties &properties)
{
    RTRC_RG_SCOPED_PASS_GROUP(renderGraph, "UpdateAtmosphere");

    assert(!T_ && !M_);
    if(properties_ != properties)
    {
        persistentT_ = nullptr;
        persistentM_ = nullptr;
        properties_ = properties;
    }

    T_ = persistentT_ ? renderGraph.RegisterTexture(persistentT_) : GenerateT(renderGraph, properties);
    M_ = persistentM_ ? renderGraph.RegisterTexture(persistentM_) : GenerateM(renderGraph, properties);
    assert(persistentT_ && persistentT_->GetSize() == resT_);
    assert(persistentM_ && persistentM_->GetSize() == resM_);
}

void RenderAtmosphere::ClearFrameData()
{
    T_ = nullptr;
    M_ = nullptr;
}

void RenderAtmosphere::Render(
    RG::RenderGraph &renderGraph,
    const Vector3f  &sunDir,
    const Vector3f  &sunColor,
    float            eyeAltitude,
    float            dt,
    PerCameraData   &perCameraData) const
{
    RTRC_RG_SCOPED_PASS_GROUP(renderGraph, "RenderAtmosphere");

    // Prepare sky texture

    bool needClearSkyLut = false;
    if(!perCameraData.persistentS || perCameraData.persistentS->GetSize() != resS_)
    {
        perCameraData.persistentS = device_->CreatePooledTexture(RHI::TextureDesc
        {
            .dim    = RHI::TextureDimension::Tex2D,
            .format = RHI::Format::R32G32B32A32_Float,
            .width  = resS_.x,
            .height = resS_.y,
            .usage  = RHI::TextureUsage::UnorderAccess | RHI::TextureUsage::ShaderResource
        });
        perCameraData.persistentS->SetName("AtmosphereSkyLut");
        needClearSkyLut = true;
    }
    assert(T_ && M_);
    assert(!perCameraData.S);
    perCameraData.S = renderGraph.RegisterTexture(perCameraData.persistentS);
    if(needClearSkyLut)
    {
        renderGraph.CreateClearRWTexture2DPass("ClearSkyLut", perCameraData.S, Vector4f(0));
    }
    auto S = perCameraData.S;

    // Render

    const float lerpFactor = std::clamp(1.0f - std::pow(0.03f, 0.4f * dt), 0.001f, 0.05f);

    StaticShaderInfo<"Atmosphere/GenerateSkyLut">::Pass skyPassData;
    skyPassData.SkyLutTextureRW           = S;
    skyPassData.SkyLutTextureRW.writeOnly = true;
    skyPassData.TransmittanceLutTexture   = T_;
    skyPassData.MultiScatterLutTexture    = M_;
    skyPassData.atmosphere                = properties_;
    skyPassData.outputResolution          = resS_;
    skyPassData.rayMarchStepCount         = rayMarchingStepCount_;
    skyPassData.frameRandom01             = std::uniform_real_distribution<float>(0, 1)(perCameraData.rng);
    skyPassData.eyePosY                   = eyeAltitude;
    skyPassData.lerpFactor                = lerpFactor;
    skyPassData.sunDirection              = sunDir;
    skyPassData.sunIntensity              = sunColor;

    renderGraph.CreateComputePassWithThreadCount(
        "GenerateSkyLut",
        resources_->GetMaterialManager()->GetCachedShader<"Atmosphere/GenerateSkyLut">(),
        resS_,
        skyPassData);
}

void RenderAtmosphere::ClearPerCameraFrameData(PerCameraData &perCameraData) const
{
    perCameraData.S = nullptr;
}

RG::TextureResource *RenderAtmosphere::GenerateT(RG::RenderGraph &renderGraph, const AtmosphereProperties &props)
{
    assert(!persistentT_);
    persistentT_ = device_->CreatePooledTexture(RHI::TextureDesc
    {
        .dim    = RHI::TextureDimension::Tex2D,
        .format = RHI::Format::R32G32B32A32_Float,
        .width  = resT_.x,
        .height = resT_.y,
        .usage  = RHI::TextureUsage::UnorderAccess | RHI::TextureUsage::ShaderResource,
    });
    persistentT_->SetName("RenderAtmosphere_T");
    auto T = renderGraph.RegisterTexture(persistentT_);

    StaticShaderInfo<"Atmosphere/GenerateTransmittanceLut">::Pass passData;
    passData.outputResolution                 = resT_;
    passData.TransmittanceTextureRW           = T;
    passData.TransmittanceTextureRW.writeOnly = true;
    passData.atmosphere                       = props;
    renderGraph.CreateComputePassWithThreadCount(
        "GenerateTransmittanceLut",
        resources_->GetMaterialManager()->GetCachedShader<"Atmosphere/GenerateTransmittanceLut">(),
        resT_,
        passData);

    return T;
}

RG::TextureResource *RenderAtmosphere::GenerateM(RG::RenderGraph &renderGraph, const AtmosphereProperties &props)
{
    if(!multiScatterDirSamples_)
    {
        auto data = GeneratePoissonDiskSamples<Vector2f>(MULTI_SCATTER_DIR_SAMPLE_COUNT, 42);
        multiScatterDirSamples_ = device_->CreateAndUploadBuffer(RHI::BufferDesc
        {
            .size           = sizeof(Vector2f) * MULTI_SCATTER_DIR_SAMPLE_COUNT,
            .usage          = RHI::BufferUsage::ShaderStructuredBuffer,
            .hostAccessType = RHI::BufferHostAccessType::Upload
        }, data.data());
        multiScatterDirSamples_->SetName("MultiScatterDirSamples");
        multiScatterDirSamples_->SetDefaultStructStride(sizeof(Vector2f));
    }

    assert(!persistentM_);
    persistentM_ = device_->CreatePooledTexture(RHI::TextureDesc
    {
        .dim    = RHI::TextureDimension::Tex2D,
        .format = RHI::Format::R32G32B32A32_Float,
        .width  = resM_.x,
        .height = resM_.y,
        .usage  = RHI::TextureUsage::ShaderResource | RHI::TextureUsage::UnorderAccess
    });
    persistentM_->SetName("RenderAtmosphere_M");
    auto M = renderGraph.RegisterTexture(persistentM_);

    assert(T_);
    StaticShaderInfo<"Atmosphere/GenerateMultiScatterLut">::Pass passData;
    passData.MultiScatterTextureRW           = M;
    passData.MultiScatterTextureRW.writeOnly = true;
    passData.RawDirSamples                   = multiScatterDirSamples_;
    passData.TransmittanceLutTexture         = T_;
    passData.outputResolution                = resM_;
    passData.dirSampleCount                  = MULTI_SCATTER_DIR_SAMPLE_COUNT;
    passData.rayMarchStepCount               = rayMarchingStepCount_;
    passData.atmosphere                      = props;

    renderGraph.CreateComputePassWithThreadCount(
        "GenerateMultiScatteringLut",
        resources_->GetMaterialManager()->GetCachedShader<"Atmosphere/GenerateMultiScatterLut">(),
        resM_,
        passData);

    return M;
}

RTRC_RENDERER_END
