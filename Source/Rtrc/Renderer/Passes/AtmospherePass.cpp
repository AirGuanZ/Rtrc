#include <Rtrc/Renderer/Passes/AtmospherePass.h>
#include <Rtrc/Math/LowDiscrepancy.h>

RTRC_RENDERER_BEGIN

namespace PhysicalAtmospherePassDetail
{
    
    rtrc_group(TransmittanceLutPass)
    {
        rtrc_define(RWTexture2D,                   TransmittanceTextureRW);
        rtrc_uniform(uint2,                        outputResolution);
        rtrc_uniform(PhysicalAtmosphereProperties, atmosphere);
    };

    rtrc_group(MultiScatterLutPass)
    {
        rtrc_define(RWTexture2D,                   MultiScatterTextureRW);
        rtrc_define(StructuredBuffer,              RawDirSamples);
        rtrc_define(Texture2D,                     TransmittanceLut);
        rtrc_uniform(uint2,                        outputResolution);
        rtrc_uniform(int,                          dirSampleCount);
        rtrc_uniform(int,                          rayMarchStepCount);
        rtrc_uniform(PhysicalAtmosphereProperties, atmosphere);
        rtrc_uniform(float3,                       terrainAlbedo);
    };

    rtrc_group(SkyLutPass)
    {
        rtrc_define(RWTexture2D,                   SkyLutTextureRW);
        rtrc_define(Texture2D,                     TransmittanceLutTexture);
        rtrc_define(Texture2D,                     MultiScatterLutTexture);
        rtrc_uniform(PhysicalAtmosphereProperties, atmosphere);
        rtrc_uniform(uint2,                        outputResolution);
        rtrc_uniform(int,                          rayMarchStepCount);
        rtrc_uniform(float,                        frameRandom01);
        rtrc_uniform(float,                        eyePosY);
        rtrc_uniform(float3,                       sunDirection);
        rtrc_uniform(float,                        lerpFactor);
        rtrc_uniform(float3,                       sunIntensity);
    };

} // namespace PhysicalAtmospherePassDetail

PhysicalAtmospherePass::PhysicalAtmospherePass(
    ObserverPtr<Device>                       device,
    ObserverPtr<const BuiltinResourceManager> builtinResources)
    : rng_(42)
    , device_(device)
    , builtinResources_(builtinResources)
    , transmittanceLutRes_(256, 256)
    , multiScatteringLutRes_(256, 256)
    , skyLutRes_(256, 128)
    , rayMarchingStepCount_(24)
{
    using namespace PhysicalAtmospherePassDetail;

    Material *material = builtinResources_->GetBuiltinMaterial(BuiltinMaterial::Atmosphere).get();
    transmittanceLutShader_   = material->GetPassByIndex(AMP_GenerateTransmittanceLut)  ->GetShader();
    multiScatteringLutShader_ = material->GetPassByIndex(AMP_GenerateMultiScatteringLut)->GetShader();
    skyLutShader_             = material->GetPassByIndex(AMP_GenerateSkyLut)            ->GetShader();

    skyPassBindingGroupLayout_             = device_->CreateBindingGroupLayout<SkyLutPass>();
    transmittancePassBindingGroupLayout_   = device_->CreateBindingGroupLayout<TransmittanceLutPass>();
    multiScatteringPassBindingGroupLayout_ = device_->CreateBindingGroupLayout<MultiScatterLutPass>();
}

const PhysicalAtmosphereProperties &PhysicalAtmospherePass::GetProperties() const
{
    return properties_;
}

void PhysicalAtmospherePass::SetProperties(const PhysicalAtmosphereProperties &properties)
{
    if(properties_ != properties)
    {
        properties_ = properties;
        transmittanceLut_ = {};
        multiScatteringLut_ = {};
    }
}

void PhysicalAtmospherePass::SetTransmittanceLutResolution(const Vector2u &res)
{
    if(transmittanceLutRes_ != res)
    {
        transmittanceLutRes_ = res;
        transmittanceLut_ = {};
    }
}

void PhysicalAtmospherePass::SetMultiScatteringLutResolution(const Vector2u &res)
{
    if(multiScatteringLutRes_ != res)
    {
        multiScatteringLutRes_ = res;
        multiScatteringLut_ = {};
    }
}

void PhysicalAtmospherePass::SetSkyLutResolution(const Vector2u &res)
{
    skyLutRes_ = res;
}

void PhysicalAtmospherePass::SetRayMarchingStepCount(int stepCount)
{
    if(rayMarchingStepCount_ != stepCount)
    {
        rayMarchingStepCount_ = stepCount;
        multiScatteringLut_ = {};
    }
}

PhysicalAtmospherePass::RenderGraphOutput PhysicalAtmospherePass::RenderAtmosphere(
    RG::RenderGraph    &graph,
    const Vector3f     &sunDirection,
    const Vector3f     &sunColor,
    float               eyeAltitude,
    float               dt,
    CachedData &perSceneData)
{
    RG::TextureResource *T = GetTransmittanceLut(graph);
    RG::TextureResource *M = GetMultiScatteringLut(graph, T);
    
    RG::TextureResource *S = InitializeSkyLut(graph, perSceneData.skyLut, true);

    RG::Pass *passS = graph.CreatePass("GenerateSkyLut");
    passS->Use(S, RG::COMPUTE_SHADER_RWTEXTURE);
    passS->Use(T, RG::COMPUTE_SHADER_TEXTURE);
    passS->Use(M, RG::COMPUTE_SHADER_TEXTURE);
    passS->SetCallback(
        [this, S, T, M, dt, eyeAltitude, sunDirection, sunColor]
    (RG::PassContext &context)
    {
        const float lerpFactor = std::clamp(
            1.0f - std::pow(0.03f, 0.4f * dt), 0.001f, 0.05f);

        PhysicalAtmospherePassDetail::SkyLutPass passData;
        passData.SkyLutTextureRW         = S->Get(context);
        passData.TransmittanceLutTexture = T->Get(context);
        passData.MultiScatterLutTexture  = M->Get(context);
        passData.atmosphere              = properties_;
        passData.outputResolution        = skyLutRes_;
        passData.rayMarchStepCount       = rayMarchingStepCount_;
        passData.frameRandom01           = std::uniform_real_distribution<float>(0, 1)(rng_);
        passData.eyePosY                 = eyeAltitude;
        passData.lerpFactor              = lerpFactor;
        passData.sunDirection            = sunDirection;
        passData.sunIntensity            = sunColor;
        auto bindingGroup = device_->CreateBindingGroup(passData, skyPassBindingGroupLayout_);

        CommandBuffer &commandBuffer = context.GetCommandBuffer();
        commandBuffer.BindComputePipeline(skyLutShader_->GetComputePipeline());
        commandBuffer.BindComputeGroup(0, bindingGroup);
        commandBuffer.Dispatch(skyLutShader_->ComputeThreadGroupCount(Vector3i(
            static_cast<int>(skyLutRes_.x), static_cast<int>(skyLutRes_.y), 1)));
    });

    RenderGraphOutput ret;
    ret.passS = passS;
    ret.skyLut = S;
    return ret;
}

RG::TextureResource *PhysicalAtmospherePass::GetTransmittanceLut(RG::RenderGraph &graph)
{
    if(transmittanceLut_)
    {
        assert(transmittanceLut_->GetSize() == transmittanceLutRes_);
        return graph.RegisterTexture(transmittanceLut_);
    }
    
    transmittanceLut_ = device_->CreatePooledTexture(RHI::TextureDesc
    {
        .dim                  = RHI::TextureDimension::Tex2D,
        .format               = RHI::Format::R32G32B32A32_Float,
        .width                = transmittanceLutRes_.x,
        .height               = transmittanceLutRes_.y,
        .arraySize            = 1,
        .mipLevels            = 1,
        .sampleCount          = 1,
        .usage                = RHI::TextureUsage::UnorderAccess | RHI::TextureUsage::ShaderResource,
        .initialLayout        = RHI::TextureLayout::Undefined,
        .concurrentAccessMode = RHI::QueueConcurrentAccessMode::Exclusive
    });
    transmittanceLut_->SetName("TransmittanceLut");
    RG::TextureResource *T = graph.RegisterTexture(transmittanceLut_);

    auto passT = graph.CreatePass("GenerateTransmittanceLut");
    passT->Use(T, RG::COMPUTE_SHADER_RWTEXTURE_WRITEONLY);
    passT->SetCallback([this, T](RG::PassContext &context)
    {
        PhysicalAtmospherePassDetail::TransmittanceLutPass passData;
        passData.outputResolution = transmittanceLutRes_;
        passData.TransmittanceTextureRW = T->Get(context);
        passData.atmosphere = properties_;
        auto passGroup = device_->CreateBindingGroup(passData, transmittancePassBindingGroupLayout_);

        CommandBuffer &commandBuffer = context.GetCommandBuffer();
        commandBuffer.BindComputePipeline(transmittanceLutShader_->GetComputePipeline());
        commandBuffer.BindComputeGroup(0, passGroup);
        commandBuffer.DispatchWithThreadCount(
            static_cast<int>(transmittanceLutRes_.x), static_cast<int>(transmittanceLutRes_.y), 1);
    });
    return T;
}

RG::TextureResource *PhysicalAtmospherePass::GetMultiScatteringLut(RG::RenderGraph &graph, RG::TextureResource *T)
{
    if(multiScatteringLut_)
    {
        assert(multiScatteringLut_->GetSize() == multiScatteringLutRes_);
        return graph.RegisterTexture(multiScatteringLut_);
    }

    RC<Buffer> poissonDiskSamples;
    {
        auto data = GeneratePoissonDiskSamples(MS_DIR_SAMPLE_COUNT, 42);
        poissonDiskSamples = device_->CreateAndUploadBuffer(RHI::BufferDesc
        {
            .size           = sizeof(Vector2f) * MS_DIR_SAMPLE_COUNT,
            .usage          = RHI::BufferUsage::ShaderStructuredBuffer,
            .hostAccessType = RHI::BufferHostAccessType::SequentialWrite
        }, data.data());
    }
    poissonDiskSamples->SetName("PoissonDiskSamples");
    poissonDiskSamples->SetDefaultStructStride(sizeof(Vector2f));

    multiScatteringLut_ = device_->CreatePooledTexture(RHI::TextureDesc
    {
        .dim                  = RHI::TextureDimension::Tex2D,
        .format               = RHI::Format::R32G32B32A32_Float,
        .width                = multiScatteringLutRes_.x,
        .height               = multiScatteringLutRes_.y,
        .arraySize            = 1,
        .mipLevels            = 1,
        .sampleCount          = 1,
        .usage                = RHI::TextureUsage::ShaderResource | RHI::TextureUsage::UnorderAccess,
        .initialLayout        = RHI::TextureLayout::Undefined,
        .concurrentAccessMode = RHI::QueueConcurrentAccessMode::Exclusive
    });
    multiScatteringLut_->SetName("MultiScatteringLut");
    RG::TextureResource *M = graph.RegisterTexture(multiScatteringLut_);

    auto passM = graph.CreatePass("GenerateMultiScatteringLut");
    passM->Use(T, RG::COMPUTE_SHADER_TEXTURE);
    passM->Use(M, RG::COMPUTE_SHADER_RWTEXTURE_WRITEONLY);
    passM->SetCallback([this, M, T, poissonDiskSamples](RG::PassContext &context)
    {
        PhysicalAtmospherePassDetail::MultiScatterLutPass passData;
        passData.MultiScatterTextureRW = M->Get(context);
        passData.RawDirSamples         = poissonDiskSamples;
        passData.TransmittanceLut      = T->Get(context);
        passData.outputResolution      = multiScatteringLutRes_;
        passData.dirSampleCount        = MS_DIR_SAMPLE_COUNT;
        passData.rayMarchStepCount     = rayMarchingStepCount_;
        passData.atmosphere            = properties_;
        auto passGroup = device_->CreateBindingGroup(passData, multiScatteringPassBindingGroupLayout_);

        CommandBuffer &commandBuffer = context.GetCommandBuffer();
        commandBuffer.BindComputePipeline(multiScatteringLutShader_->GetComputePipeline());
        commandBuffer.BindComputeGroup(0, passGroup);
        commandBuffer.DispatchWithThreadCount(
            static_cast<int>(multiScatteringLutRes_.x), static_cast<int>(multiScatteringLutRes_.y), 1);
    });

    return M;
}

RG::TextureResource *PhysicalAtmospherePass::InitializeSkyLut(
    RG::RenderGraph     &graph,
    RC<StatefulTexture> &tex,
    bool                 clearWhenCreated) const
{
    bool needClear = clearWhenCreated;
    if(!tex || tex->GetSize() != skyLutRes_)
    {
        tex = device_->CreatePooledTexture(RHI::TextureDesc
        {
            .dim                  = RHI::TextureDimension::Tex2D,
            .format               = RHI::Format::R32G32B32A32_Float,
            .width                = skyLutRes_.x,
            .height               = skyLutRes_.y,
            .arraySize            = 1,
            .mipLevels            = 1,
            .sampleCount          = 1,
            .usage                = RHI::TextureUsage::UnorderAccess |
                                    RHI::TextureUsage::ShaderResource |
                                    RHI::TextureUsage::TransferDst,
            .initialLayout        = RHI::TextureLayout::Undefined,
            .concurrentAccessMode = RHI::QueueConcurrentAccessMode::Exclusive
        });
        tex->SetName("SkyLut");
    }
    else
    {
        needClear = false;
    }

    RG::TextureResource *ret = graph.RegisterTexture(tex);
    if(needClear)
    {
        graph.CreateClearTexture2DPass("ClearSkyLut", ret, { 0, 0, 0, 0 });
    }
    return ret;
}

RTRC_RENDERER_END
