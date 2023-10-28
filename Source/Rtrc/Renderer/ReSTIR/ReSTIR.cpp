#include <Rtrc/Renderer/GPUScene/RenderCamera.h>
#include <Rtrc/Renderer/Utility/PcgStateTexture.h>
#include <Rtrc/Scene/Light/Light.h>

#include <Rtrc/Renderer/ReSTIR/Shader/ReSTIR.shader.outh>

RTRC_RENDERER_BEGIN

namespace ReSTIRDetail
{

    rtrc_struct(LightShadingData)
    {
        float3 position;      // Valid for point light and spot light
        uint   type;          // See Light::Type
        float3 direction;     // Valid for spot light and directional light
        float  softness = 0;  // Radius for point light; direction radius for directional light

        float3 color;
        float  intensity = 1;

        float distFadeBias  = 0; // Valid for point light and spot light
        float distFadeScale = 0;
        float coneFadeBias  = 0; // Valid for spot light
        float coneFadeScale = 0;
    };

    LightShadingData ExtractLightShadingData(const Light *light)
    {
        const Light::Type type = light->GetType();
        assert(type == Light::Type::Point || type == Light::Type::Spot || type == Light::Type::Directional);
        LightShadingData ret;
        ret.type      = std::to_underlying(type);
        ret.color     = light->GetColor();
        ret.intensity = light->GetIntensity();
        if(type == Light::Type::Point || type == Light::Type::Spot)
        {
            ret.position = light->GetPosition();
            // distFadeFactor = saturate(scale * dist + bias)
            //                = saturate(-dist/(end-beg) + end/(end-beg))
            const float deltaDist = std::max(light->GetDistFadeEnd() - light->GetDistFadeBegin(), 1e-5f);
            ret.distFadeScale = -1 / deltaDist;
            ret.distFadeBias = light->GetDistFadeEnd() / deltaDist;
        }
        if(type == Light::Type::Directional || type == Light::Type::Spot)
        {
            ret.direction = light->GetDirection();
        }
        if(type == Light::Type::Point || type == Light::Type::Spot)
        {
            ret.softness = light->GetSoftness();
        }
        if(type == Light::Type::Directional)
        {
            ret.softness = std::min(light->GetSoftness(), 0.999f);
        }
        if(type == Light::Type::Spot)
        {
            // coneFadeFactor = saturate(scale * dot + bias)
            const float dot0 = std::clamp(std::cos(Deg2Rad(light->GetConeFadeBegin())), 0.0f, 1.0f);
            const float dot1 = std::clamp(std::cos(Deg2Rad(light->GetConeFadeEnd())), 0.0f, 1.0f);
            ret.coneFadeScale = 1.0 / (dot0 - dot1);
            ret.coneFadeBias = -ret.coneFadeScale * dot1;
        }
        return ret;
    }

} // namespace ReSTIRDetail

ReSTIR::ReSTIR(ObserverPtr<ResourceManager> resources)
    : RenderAlgorithm(resources), svgf_(resources)
{
    
}

void ReSTIR::Render(RenderCamera &renderCamera, RG::RenderGraph &renderGraph)
{
    RTRC_RG_SCOPED_PASS_GROUP(renderGraph, "ReSTIR");

    PerCameraData &data = renderCamera.GetReSTIRData();
    const RenderScene &renderScene = renderCamera.GetScene();
    const Scene &scene = renderScene.GetScene();

    auto sky = renderCamera.GetAtmosphereData().S;

    // Setup rng states && reservoirs

    auto gbuffers = renderCamera.GetGBuffers();
    const Vector2u framebufferSize = gbuffers.currDepth->GetSize();
    auto pcgState = Prepare2DPcgStateTexture(renderGraph, resources_, data.pcgState, framebufferSize);

    std::swap(data.prevReservoirs, data.currReservoirs);
    if(data.prevReservoirs && data.prevReservoirs->GetSize() != framebufferSize)
    {
        data.prevReservoirs.reset();
    }

    bool shouldClearPrevReservoirs = false;
    if(!data.prevReservoirs)
    {
        shouldClearPrevReservoirs = true;
        data.prevReservoirs = device_->CreatePooledTexture(RHI::TextureDesc
        {
            .dim    = RHI::TextureDimension::Tex2D,
            .format = RHI::Format::R32G32B32A32_UInt,
            .width  = framebufferSize.x,
            .height = framebufferSize.y,
            .usage  = RHI::TextureUsage::ShaderResource | RHI::TextureUsage::UnorderAccess
        });
        data.prevReservoirs->SetName("PersistentReservoirsA");
        data.currReservoirs = device_->CreatePooledTexture(data.prevReservoirs->GetDesc());
        data.currReservoirs->SetName("PersistentReservoirsB");
    }
    auto prevReservoirs = renderGraph.RegisterTexture(data.prevReservoirs);
    auto currReservoirs = renderGraph.RegisterTexture(data.currReservoirs);
    if(shouldClearPrevReservoirs)
    {
        renderGraph.CreateClearRWTexture2DPass(
            "ClearPreviousReservoirs", prevReservoirs, Vector4u(0, 0, 0, 0));
    }
    
    // Setup light buffers

    std::vector<ReSTIRDetail::LightShadingData> lightShadingData;
    lightShadingData.reserve(scene.GetLightCount());
    scene.ForEachLight([&](const Light *light)
    {
        lightShadingData.push_back(ReSTIRDetail::ExtractLightShadingData(light));
    });
    const int lightCount = scene.GetLightCount();

    RC<Buffer> lightBuffer;
    if(lightShadingData.empty())
    {
        if(!dummyLightBuffer_)
        {
            ReSTIRDetail::LightShadingData dummyLightShadingData = {};
            dummyLightBuffer_ = device_->CreateAndUploadBuffer(RHI::BufferDesc
            {
                .size = sizeof(dummyLightShadingData),
                .usage = RHI::BufferUsage::ShaderStructuredBuffer
            }, &dummyLightShadingData);
            dummyLightBuffer_->SetDefaultStructStride(sizeof(ReSTIRDetail::LightShadingData));
        }
        lightBuffer = dummyLightBuffer_;
    }
    else
    {
        lightBuffer = device_->CreateAndUploadBuffer(RHI::BufferDesc
        {
            .size = lightShadingData.size() * sizeof(ReSTIRDetail::LightShadingData),
            .usage = RHI::BufferUsage::ShaderStructuredBuffer
        }, lightShadingData.data());
        lightBuffer->SetDefaultStructStride(sizeof(ReSTIRDetail::LightShadingData));
    }

    // Create new reservoirs

    auto newReservoirs = renderGraph.CreateTexture(prevReservoirs->GetDesc(), "NewReservoirs");
    {
        StaticShaderInfo<"ReSTIR/CreateNewReservoirs">::Pass passData;
        renderCamera.FillGBuffersIntoBindingGroup(passData);
        passData.Camera                    = renderCamera.GetCameraCBuffer();
        passData.Sky                       = sky;
        passData.Scene                     = renderScene.GetTlas();
        passData.LightShadingDataBuffer    = lightBuffer;
        passData.lightCount                = lightCount;
        passData.PcgStateTextureRW         = pcgState;
        passData.OutputTextureRW           = newReservoirs;
        passData.OutputTextureRW.writeOnly = true;
        passData.outputResolution          = newReservoirs->GetSize();
        passData.M                         = settings_.M;
        renderGraph.CreateComputePassWithThreadCount(
            "CreateNewReservoirs",
            GetStaticShader<"ReSTIR/CreateNewReservoirs">(),
            newReservoirs->GetSize(),
            passData);
    }

    // Temporal reuse

    auto temporalReuseOutput = newReservoirs;
    if(settings_.enableTemporalReuse)
    {
        temporalReuseOutput = currReservoirs;

        StaticShaderInfo<"ReSTIR/TemporalReuse">::Pass passData;
        renderCamera.FillGBuffersIntoBindingGroup(passData);
        passData.Sky                      = sky;
        passData.PrevDepth                = gbuffers.prevDepth ? gbuffers.prevDepth : gbuffers.currDepth;
        passData.CurrDepth                = gbuffers.currDepth;
        passData.PrevNormal               = gbuffers.prevNormal ? gbuffers.prevNormal : gbuffers.currNormal;
        passData.CurrNormal               = gbuffers.currNormal;
        passData.PrevCamera               = renderCamera.GetPreviousCameraCBuffer();
        passData.CurrCamera               = renderCamera.GetCameraCBuffer();
        passData.LightShadingDataBuffer   = lightBuffer;
        passData.PrevReservoirs           = prevReservoirs;
        passData.CurrReservoirs           = currReservoirs;
        passData.CurrReservoirs.writeOnly = true;
        passData.NewReservoirs            = newReservoirs;
        passData.PcgState                 = pcgState;
        passData.maxM                     = settings_.maxM;
        passData.resolution               = newReservoirs->GetSize();
        passData.lightCount               = lightCount;
        renderGraph.CreateComputePassWithThreadCount(
            "TemporalReuse",
            GetStaticShader<"ReSTIR/TemporalReuse">(),
            newReservoirs->GetSize(),
            passData);
    }

    // Spatial reuse

    auto finalReservoirs = temporalReuseOutput;
    if(settings_.N > 0 && settings_.radius > 0)
    {
        auto reservoirsA = temporalReuseOutput;
        auto reservoirsB = renderGraph.CreateTexture(temporalReuseOutput->GetDesc(), "TempReservoirs");

        StaticShaderInfo<"ReSTIR/SpatialReuse">::Pass passData;
        passData.Camera                     = renderCamera.GetCameraCBuffer();
        passData.Sky                        = sky;
        passData.Depth                      = gbuffers.currDepth;
        passData.Normal                     = gbuffers.currNormal;
        passData.LightShadingDataBuffer     = lightBuffer;
        passData.InputReservoirs            = reservoirsA;
        passData.OutputReservoirs           = reservoirsB;
        passData.OutputReservoirs.writeOnly = true;
        passData.PcgState                   = pcgState;
        passData.resolution                 = reservoirsA->GetSize();
        passData.N                          = settings_.N;
        passData.maxM                       = settings_.maxM;
        passData.radius                     = settings_.radius;
        passData.lightCount                 = lightCount;
        renderGraph.CreateComputePassWithThreadCount(
            "SpatialReuse",
            GetStaticShader<"ReSTIR/SpatialReuse">(),
            reservoirsA->GetSize(),
            passData);

        finalReservoirs = reservoirsB;
    }

    // Resolve final result

    auto unfilteredIllum = renderGraph.CreateTexture(RHI::TextureDesc
    {
        .dim    = RHI::TextureDimension::Tex2D,
        .format = RHI::Format::R32G32B32A32_Float,
        .width  = framebufferSize.x,
        .height = framebufferSize.y,
        .usage  = RHI::TextureUsage::ShaderResource | RHI::TextureUsage::UnorderAccess
    });

    {
        StaticShaderInfo<"ReSTIR/Resolve">::Pass passData;
        renderCamera.FillGBuffersIntoBindingGroup(passData);
        passData.Camera                 = renderCamera.GetCameraCBuffer();
        passData.Sky                    = sky;
        passData.Scene                  = renderScene.GetTlas();
        passData.LightShadingDataBuffer = lightBuffer;
        passData.Reservoirs             = finalReservoirs;
        passData.Output                 = unfilteredIllum;
        passData.Output.writeOnly       = true;
        passData.outputResolution       = unfilteredIllum->GetSize();
        passData.lightCount             = lightCount;
        renderGraph.CreateComputePassWithThreadCount(
            "Resolve",
            GetStaticShader<"ReSTIR/Resolve">(),
            unfilteredIllum->GetSize(),
            passData);
    }

    // SVGF

    svgf_.Render(renderGraph, renderCamera, data.svgf, unfilteredIllum);

    assert(!data.directIllum);
    data.directIllum = data.svgf.finalColor;
}

void ReSTIR::ClearFrameData(PerCameraData &data) const
{
    data.directIllum = nullptr;
    svgf_.ClearFrameData(data.svgf);
}

RTRC_RENDERER_END
