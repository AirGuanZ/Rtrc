#include <Rtrc/Application/Application.h>
#include <Rtrc/Renderer/Atmosphere/AtmosphereRenderer.h>
#include <Rtrc/Math/LowDiscrepancy.h>

RTRC_RENDERER_BEGIN

namespace PhysicalAtmospherePassDetail
{
    
    rtrc_group(TransmittanceLutPass)
    {
        rtrc_define(RWTexture2D,                   TransmittanceTextureRW);
        rtrc_uniform(uint2,                        outputResolution);
        rtrc_uniform(AtmosphereProperties, atmosphere);
    };

    rtrc_group(MultiScatterLutPass)
    {
        rtrc_define(RWTexture2D,                   MultiScatterTextureRW);
        rtrc_define(StructuredBuffer,              RawDirSamples);
        rtrc_define(Texture2D,                     TransmittanceLut);
        rtrc_uniform(uint2,                        outputResolution);
        rtrc_uniform(int,                          dirSampleCount);
        rtrc_uniform(int,                          rayMarchStepCount);
        rtrc_uniform(AtmosphereProperties, atmosphere);
        rtrc_uniform(float3,                       terrainAlbedo);
    };

    rtrc_group(SkyLutPass)
    {
        rtrc_define(RWTexture2D,           SkyLutTextureRW);
        rtrc_define(Texture2D,             TransmittanceLutTexture);
        rtrc_define(Texture2D,             MultiScatterLutTexture);
        rtrc_uniform(AtmosphereProperties, atmosphere);
        rtrc_uniform(uint2,                outputResolution);
        rtrc_uniform(int,                  rayMarchStepCount);
        rtrc_uniform(float,                frameRandom01);
        rtrc_uniform(float,                eyePosY);
        rtrc_uniform(float3,               sunDirection);
        rtrc_uniform(float,                lerpFactor);
        rtrc_uniform(float3,               sunIntensity);
    };

} // namespace PhysicalAtmospherePassDetail

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

    auto pass = renderGraph.CreatePass("GenerateSkyLut");
    pass->Use(S, RG::CS_RWTexture);
    pass->Use(T_, RG::CS_Texture);
    pass->Use(M_, RG::CS_Texture);
    pass->SetCallback([&perCameraData, dt, S, eyeAltitude, sunDir, sunColor, this]
    {
        const float lerpFactor = std::clamp(
            1.0f - std::pow(0.03f, 0.4f * dt), 0.001f, 0.05f);

        PhysicalAtmospherePassDetail::SkyLutPass passData;
        passData.SkyLutTextureRW         = S;
        passData.TransmittanceLutTexture = T_;
        passData.MultiScatterLutTexture  = M_;
        passData.atmosphere              = properties_;
        passData.outputResolution        = resS_;
        passData.rayMarchStepCount       = rayMarchingStepCount_;
        passData.frameRandom01           = std::uniform_real_distribution<float>(0, 1)(perCameraData.rng);
        passData.eyePosY                 = eyeAltitude;
        passData.lerpFactor              = lerpFactor;
        passData.sunDirection            = sunDir;
        passData.sunIntensity            = sunColor;
        auto passGroup = device_->CreateBindingGroupWithCachedLayout(passData);

        auto material = resources_->GetBuiltinMaterial(BuiltinMaterial::Atmosphere).get();
        auto shader = material->GetPassByIndex(Pass_GenerateS)->GetShader().get();

        CommandBuffer &commandBuffer = RG::GetCurrentCommandBuffer();
        commandBuffer.BindComputePipeline(shader->GetComputePipeline());
        commandBuffer.BindComputeGroup(0, passGroup);
        commandBuffer.DispatchWithThreadCount(resS_);
    });
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

    auto passT = renderGraph.CreatePass("GenerateTransmittanceLut");
    passT->Use(T, RG::CS_RWTexture_WriteOnly);
    passT->SetCallback([this, T, props]
    {
        PhysicalAtmospherePassDetail::TransmittanceLutPass passData;
        passData.outputResolution       = resT_;
        passData.TransmittanceTextureRW = T;
        passData.atmosphere             = props;
        auto passGroup = device_->CreateBindingGroupWithCachedLayout(passData);

        auto material = resources_->GetBuiltinMaterial(BuiltinMaterial::Atmosphere).get();
        auto shader = material->GetPassByIndex(Pass_GenerateT)->GetShader().get();

        CommandBuffer &commandBuffer = RG::GetCurrentCommandBuffer();
        commandBuffer.BindComputePipeline(shader->GetComputePipeline());
        commandBuffer.BindComputeGroup(0, passGroup);
        commandBuffer.DispatchWithThreadCount(resT_);
    });

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
    auto passM = renderGraph.CreatePass("GenerateMultiScatteringLut");
    passM->Use(T_, RG::CS_Texture);
    passM->Use(M, RG::CS_RWTexture_WriteOnly);
    passM->SetCallback([this, M, props]
    {
        PhysicalAtmospherePassDetail::MultiScatterLutPass passData;
        passData.MultiScatterTextureRW = M;
        passData.RawDirSamples         = multiScatterDirSamples_;
        passData.TransmittanceLut      = T_;
        passData.outputResolution      = resM_;
        passData.dirSampleCount        = MULTI_SCATTER_DIR_SAMPLE_COUNT;
        passData.rayMarchStepCount     = rayMarchingStepCount_;
        passData.atmosphere            = props;
        auto passGroup = device_->CreateBindingGroupWithCachedLayout(passData);

        auto material = resources_->GetBuiltinMaterial(BuiltinMaterial::Atmosphere).get();
        auto shader = material->GetPassByIndex(Pass_GenerateM)->GetShader().get();

        CommandBuffer &commandBuffer = RG::GetCurrentCommandBuffer();
        commandBuffer.BindComputePipeline(shader->GetComputePipeline());
        commandBuffer.BindComputeGroup(0, passGroup);
        commandBuffer.DispatchWithThreadCount(resM_);
    });

    return M;
}

RTRC_RENDERER_END
