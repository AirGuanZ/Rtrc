#include <Rtrc/Renderer/GBufferBinding.h>
#include <Rtrc/Renderer/CriusGI/CriusGI.h>
#include <Rtrc/Renderer/GPUScene/RenderCamera.h>
#include <Rtrc/Renderer/Utility/PcgStateTexture.h>

RTRC_RENDERER_BEGIN

void CriusGI::Render(RG::RenderGraph &renderGraph, RenderCamera &camera, const GBuffers &gbuffers) const
{
    const Vector2u framebufferSize = gbuffers.normal->GetSize();
    auto &data = camera.GetCriusGIGIData();
    InitializeData(renderGraph, framebufferSize, data);

    auto &scene = camera.GetScene();
    auto material = resources_->GetMaterialManager()->GetCachedMaterial<"Builtin/CriusGI">().get();

    {
        rtrc_group(Pass)
        {
            rtrc_inline(GBufferBindings_NormalDepth, gbuffers);

            rtrc_define(ConstantBuffer<CameraData>, Camera);
            rtrc_define(Texture2D, SkyLut);

            rtrc_define(RaytracingAccelerationStructure, Scene);
            rtrc_define(StructuredBuffer, Instances);

            rtrc_define(RWTexture2D, RngState);

            rtrc_define(RWTexture2D, OutputA);
            rtrc_define(RWTexture2D, OutputB);

            rtrc_uniform(uint2, resolution);
        };

        auto pass = renderGraph.CreatePass("Trace");
        DeclareGBufferUses<Pass>(pass, gbuffers, RHI::PipelineStage::ComputeShader);
        pass->Use(camera.GetAtmosphereData().S, RG::CS_Texture);
        pass->Use(scene.GetTlas(),              RG::CS_ReadAS);
        pass->Use(scene.GetInstanceBuffer(),    RG::CS_StructuredBuffer);
        pass->Use(data.rngState,                RG::CS_RWStructuredBuffer);
        pass->Use(data.rawTracedA,              RG::CS_RWTexture_WriteOnly);
        pass->Use(data.rawTracedB,              RG::CS_RWTexture_WriteOnly);
        pass->SetCallback([&camera, &scene, &gbuffers, &data, material, this]
        {
            Pass passData;
            FillBindingGroupGBuffers(passData, gbuffers);
            passData.Camera     = camera.GetCameraCBuffer();
            passData.SkyLut     = camera.GetAtmosphereData().S;
            passData.Scene      = scene.GetTlas();
            passData.Instances  = scene.GetInstanceBuffer();
            passData.RngState   = data.rngState;
            passData.OutputA    = data.rawTracedA;
            passData.OutputB    = data.rawTracedB;
            passData.resolution = data.rawTracedA->GetSize();

            auto passGroup = device_->CreateBindingGroupWithCachedLayout(passData);
            auto geometryGroup = scene.GetBindlessGeometryBuffers();
            auto shader = material->GetPassByIndex(PassIndex_Trace)->GetShader().get();

            auto &commandBuffer = RG::GetCurrentCommandBuffer();
            commandBuffer.BindComputePipeline(shader->GetComputePipeline());
            commandBuffer.BindComputeGroups({ passGroup, geometryGroup });
            commandBuffer.DispatchWithThreadCount(data.rawTracedA->GetSize());
        });
    }

    {
        
    }
}

void CriusGI::ClearFrameData(PerCameraData &data) const
{
    data.rngState        = nullptr;
    data.rawTracedA      = nullptr;
    data.rawTracedB      = nullptr;
    data.reservoirs      = nullptr;
    data.indirectDiffuse = nullptr;
}

void CriusGI::InitializeData(RG::RenderGraph &renderGraph, const Vector2u &framebufferSize, PerCameraData &data) const
{
    assert(framebufferSize.x % 2 == 0 && framebufferSize.y % 2 == 0);
    const Vector2u halfSize = framebufferSize / 2u;

    assert(!data.rngState);
    data.rngState = Prepare2DPcgStateTexture(renderGraph, resources_, data.persistentRngState, halfSize);

    assert(!data.rawTracedA && !data.rawTracedB);
    data.rawTracedA = renderGraph.CreateTexture(RHI::TextureDesc
    {
        .format = RHI::Format::R32G32B32A32_Float,
        .width  = halfSize.x,
        .height = halfSize.y,
        .usage  = RHI::TextureUsage::ShaderResource | RHI::TextureUsage::UnorderAccess
    }, "CriusGI-RawTraceA");
    data.rawTracedB = renderGraph.CreateTexture(data.rawTracedA->GetDesc(), "CriusGI-RawTraceB");

    assert(!data.reservoirs);
    if(!data.persistentReservoirs || data.persistentReservoirs->GetSize() != halfSize)
    {
        data.persistentReservoirs = device_->CreatePooledTexture(RHI::TextureDesc
        {
            .format = RHI::Format::R32G32B32A32_Float,
            .width  = halfSize.x,
            .height = halfSize.y,
            .usage  = RHI::TextureUsage::ShaderResource | RHI::TextureUsage::UnorderAccess
        });
    }
    data.reservoirs = renderGraph.RegisterTexture(data.persistentReservoirs);

    assert(!data.indirectDiffuse);
    data.indirectDiffuse = renderGraph.CreateTexture(RHI::TextureDesc
    {
        .format = RHI::Format::R32G32B32A32_Float,
        .width  = halfSize.x,
        .height = halfSize.y,
        .usage  = RHI::TextureUsage::ShaderResource | RHI::TextureUsage::UnorderAccess
    });
}

RTRC_RENDERER_END
