#include <Rtrc/Renderer/GPUScene/RenderCamera.h>
#include <Rtrc/Renderer/GBufferBinding.h>
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

    LightShadingData ExtractLight(const Light *light)
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
        if(type == Light::Type::Point || type == Light::Type::Directional)
        {
            ret.softness = light->GetSoftness();
        }
        return ret;
    }

} // namespace ReSTIRDetail

void ReSTIR::Render(RenderCamera &renderCamera, RG::RenderGraph &renderGraph, const GBuffers &gbuffers)
{
    RTRC_RG_SCOPED_PASS_GROUP(renderGraph, "ReSTIR");

    PerCameraData &data = renderCamera.GetReSTIRData();
    const RenderScene &renderScene = renderCamera.GetScene();
    const Scene &scene = renderScene.GetScene();

    // Setup RTs

    const Vector2u framebufferSize = gbuffers.currDepth->GetSize();
    if(data.reservoirs && data.reservoirs->GetSize() != framebufferSize)
    {
        data.reservoirs.reset();
    }
    
    if(!data.reservoirs)
    {
        data.reservoirs = device_->CreatePooledTexture(RHI::TextureDesc
        {
            .dim    = RHI::TextureDimension::Tex2D,
            .format = RHI::Format::R32G32B32A32_UInt,
            .width  = framebufferSize.x,
            .height = framebufferSize.y,
            .usage  = RHI::TextureUsage::ShaderResource | RHI::TextureUsage::UnorderAccess
        });
    }
    auto historyReservoirs = renderGraph.RegisterTexture(data.reservoirs);

    auto reservoirs0 = renderGraph.CreateTexture(historyReservoirs->GetDesc(), "TempReservoirs0");
    auto reservoirs1 = renderGraph.CreateTexture(historyReservoirs->GetDesc(), "TempReservoirs1");

    // Setup light buffers

    std::vector<ReSTIRDetail::LightShadingData> lightShadingData;
    lightShadingData.reserve(scene.GetLightCount());
    scene.ForEachLight([&](const Light *light)
    {
        lightShadingData.push_back(ReSTIRDetail::ExtractLight(light));
    });

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
            }, &dummyLightShadingData, sizeof(dummyLightShadingData));
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
    }

    // Initial sample pass

    if(lightShadingData.empty())
    {
        // TODO
    }
    else
    {
        StaticShaderInfo<"ReSTIR/CreateNewReservoirs">::Variant::Pass passData;
        FillBindingGroupGBuffers(passData, gbuffers);
        passData.Camera                    = renderCamera.GetCameraCBuffer();
        passData.LightShadingDataBuffer    = lightBuffer;
        passData.lightCount                = renderScene.GetScene().GetLightCount();
        passData.OutputTextureRW           = reservoirs0;
        passData.OutputTextureRW.writeOnly = true;
        passData.outputResolution          = reservoirs0->GetSize();
        passData.M                         = M_;
        renderGraph.CreateComputePassWithThreadCount(
            "InitialSamplePass",
            resources_->GetMaterialManager()->GetCachedShader<"Builtin/ReSTIR/InitialSamplePass">(),
            reservoirs0->GetSize(),
            passData);
    }
}

void ReSTIR::ClearFrameData(PerCameraData &data) const
{
    data.directIllum = nullptr;
}

RTRC_RENDERER_END
