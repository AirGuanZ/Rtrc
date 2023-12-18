#include <Rtrc/Renderer/GPUScene/RenderCamera.h>

RTRC_RENDERER_BEGIN

RenderCamera::RenderCamera(Ref<Device> device, const RenderScene &scene, const Camera &camera)
    : device_(device)
    , scene_(&scene)
    , camera_(&camera)
    , perObjectDataBufferPool_(device, RHI::BufferUsage::ShaderStructuredBuffer)
{

}

void RenderCamera::CreateGBuffers(Ref<RG::RenderGraph> renderGraph, const Vector2u &framebufferSize)
{
    // Normal

    assert(!gbuffers_.prevNormal && !gbuffers_.currNormal);
    std::swap(prevNormal_, currNormal_);
    if(prevNormal_ && prevNormal_->GetSize() != framebufferSize)
    {
        prevNormal_.reset();
    }
    if(prevNormal_)
    {
        gbuffers_.prevNormal = renderGraph->RegisterTexture(prevNormal_);
    }
    if(!currNormal_ || currNormal_->GetSize() != framebufferSize)
    {
        currNormal_ = device_->CreatePooledTexture(RHI::TextureDesc
        {
            .dim        = RHI::TextureDimension::Tex2D,
            .format     = RHI::Format::A2R10G10B10_UNorm,
            .width      = framebufferSize.x,
            .height     = framebufferSize.y,
            .usage      = RHI::TextureUsage::ShaderResource | RHI::TextureUsage::RenderTarget,
            .clearValue = RHI::ColorClearValue{ 0, 0, 0, 0 }
        });
        currNormal_->SetName("GBuffer normal");
    }
    gbuffers_.currNormal = renderGraph->RegisterTexture(currNormal_);

    // Others

    gbuffers_.albedoMetallic = renderGraph->CreateTexture(RHI::TextureDesc
    {
        .dim                  = RHI::TextureDimension::Tex2D,
        .format               = RHI::Format::R8G8B8A8_UNorm,
        .width                = framebufferSize.x,
        .height               = framebufferSize.y,
        .arraySize            = 1,
        .mipLevels            = 1,
        .sampleCount          = 1,
        .usage                = RHI::TextureUsage::RenderTarget | RHI::TextureUsage::ShaderResource,
        .initialLayout        = RHI::TextureLayout::Undefined,
        .concurrentAccessMode = RHI::QueueConcurrentAccessMode::Exclusive,
        .clearValue           = RHI::ColorClearValue{ 0, 0, 0, 0 }
    }, "GBufferAlbedoMetallic");
    gbuffers_.roughness = renderGraph->CreateTexture(RHI::TextureDesc
    {
        .dim                  = RHI::TextureDimension::Tex2D,
        .format               = RHI::Format::R8G8B8A8_UNorm,
        .width                = framebufferSize.x,
        .height               = framebufferSize.y,
        .arraySize            = 1,
        .mipLevels            = 1,
        .sampleCount          = 1,
        .usage                = RHI::TextureUsage::RenderTarget | RHI::TextureUsage::ShaderResource,
        .initialLayout        = RHI::TextureLayout::Undefined,
        .concurrentAccessMode = RHI::QueueConcurrentAccessMode::Exclusive,
        .clearValue           = RHI::ColorClearValue{ 0, 0, 0, 0 }
    }, "GBufferRoughness");
    gbuffers_.depthStencil = renderGraph->CreateTexture(RHI::TextureDesc
    {
        .dim                  = RHI::TextureDimension::Tex2D,
        .format               = RHI::Format::D24S8,
        .width                = framebufferSize.x,
        .height               = framebufferSize.y,
        .arraySize            = 1,
        .mipLevels            = 1,
        .sampleCount          = 1,
        .usage                = RHI::TextureUsage::DepthStencil | RHI::TextureUsage::ShaderResource,
        .initialLayout        = RHI::TextureLayout::Undefined,
        .concurrentAccessMode = RHI::QueueConcurrentAccessMode::Exclusive,
        .clearValue           = RHI::DepthStencilClearValue{ 1, 0 }
    }, "GBufferDepth");
}

void RenderCamera::ClearGBuffers()
{
    gbuffers_ = {};
}

void RenderCamera::CreateNormalTexture(RG::RenderGraph &renderGraph, const Vector2u &framebufferSize, GBuffers &gbuffers)
{
    assert(!gbuffers.prevNormal && !gbuffers.currNormal);
    std::swap(prevNormal_, currNormal_);

    if(prevNormal_ && prevNormal_->GetSize() != framebufferSize)
    {
        prevNormal_.reset();
    }
    if(prevNormal_)
    {
        gbuffers.prevNormal = renderGraph.RegisterTexture(prevNormal_);
    }

    if(!currNormal_ || currNormal_->GetSize() != framebufferSize)
    {
        currNormal_ = device_->CreatePooledTexture(RHI::TextureDesc
        {
            .dim        = RHI::TextureDimension::Tex2D,
            .format     = RHI::Format::A2R10G10B10_UNorm,
            .width      = framebufferSize.x,
            .height     = framebufferSize.y,
            .usage      = RHI::TextureUsage::ShaderResource | RHI::TextureUsage::RenderTarget,
            .clearValue = RHI::ColorClearValue{ 0, 0, 0, 0 }
        });
        currNormal_->SetName("GBuffer normal");
    }
    gbuffers.currNormal = renderGraph.RegisterTexture(currNormal_);
}

void RenderCamera::Update(
    const MeshRenderingCacheManager     &cachedMeshes,
    const MaterialRenderingCacheManager &cachedMaterials,
    LinearAllocator                     &linearAllocator)
{
    prevRenderCamera_ = currRenderCamera_;
    currRenderCamera_ = camera_->GetData();

    {
        prevCameraCBuffer_ = currCameraCBuffer_;
        currCameraCBuffer_ = device_->CreateConstantBuffer(currRenderCamera_);
    }

    objects_.clear();
    scene_->GetScene().ForEachMeshRenderer([&](const MeshRenderer *renderer)
    {
        PerObjectData perObjectData;
        perObjectData.localToWorld  = renderer->GetLocalToWorld();
        perObjectData.worldToLocal  = renderer->GetWorldToLocal();
        perObjectData.localToClip   = currRenderCamera_.worldToClip * perObjectData.localToWorld;
        perObjectData.localToCamera = currRenderCamera_.worldToCamera * perObjectData.localToWorld;

        auto record = linearAllocator.Create<MeshRendererRecord>();
        record->meshRenderer  = renderer;
        record->meshCache     = cachedMeshes.FindMeshRenderingCache(renderer->GetMesh().get());
        record->materialCache = cachedMaterials.FindMaterialRenderingCache(renderer->GetMaterial().get());
        record->perObjectData = perObjectData;

        objects_.push_back(record);
    });

    std::vector<PerObjectData> perObjectData(objects_.size());
    for(size_t i = 0; i < objects_.size(); ++i)
    {
        perObjectData[i] = objects_[i]->perObjectData;
        objects_[i]->indexInPerObjectDataBuffer = i;
    }

    const size_t perObjectDataBufferSize = sizeof(PerObjectData) * std::max<size_t>(perObjectData.size(), 1);
    perObjectDataBuffer_ = perObjectDataBufferPool_.Acquire(perObjectDataBufferSize);
    perObjectDataBuffer_->SetDefaultStructStride(sizeof(PerObjectData));
    perObjectDataBuffer_->SetName("PerObjectDataBuffer");
    if(!perObjectData.empty())
    {
        perObjectDataBuffer_->Upload(perObjectData.data(), 0, sizeof(PerObjectData) * perObjectData.size());
    }
}

void RenderCamera::ResolveDepthTexture(RG::RenderGraph &renderGraph)
{
    assert(!gbuffers_.prevDepth && !gbuffers_.currDepth);
    std::swap(prevDepth_, currDepth_);

    auto depthStencil = gbuffers_.depthStencil;
    if(prevDepth_ && prevDepth_->GetSize() != depthStencil->GetSize())
    {
        prevDepth_ = nullptr;
    }
    if(prevDepth_)
    {
        gbuffers_.prevDepth = renderGraph.RegisterTexture(prevDepth_);
    }

    if(!currDepth_ || currDepth_->GetSize() != depthStencil->GetSize())
    {
        currDepth_ = device_->CreatePooledTexture(RHI::TextureDesc
        {
            .dim    = RHI::TextureDimension::Tex2D,
            .format = RHI::Format::R32_Float,
            .width  = depthStencil->GetWidth(),
            .height = depthStencil->GetHeight(),
            .usage  = RHI::TextureUsage::ShaderResource | RHI::TextureUsage::RenderTarget
        });
        currDepth_->SetName("Readonly depth");
    }
    gbuffers_.currDepth = renderGraph.RegisterTexture(currDepth_);
    renderGraph.CreateBlitTexture2DPass("ResolveDepth", depthStencil, gbuffers_.currDepth);
}

RenderCameraManager::RenderCameraManager(Ref<Device> device, Ref<RenderSceneManager> scenes)
    : device_(device), scenes_(scenes)
{
    
}

RenderCameraManager::~RenderCameraManager()
{
    for(auto &record : std::ranges::views::values(cameras_))
    {
        record.cameraConnection.Disconnect();
        record.sceneConnection.Disconnect();
    }
}

RenderCamera &RenderCameraManager::GetRenderCamera(const Scene &scene, const Camera &camera)
{
    if(auto it = cameras_.find(&camera); it != cameras_.end())
    {
        auto &record = it->second;
        if(record.scene == &scene)
        {
            return *record.camera;
        }
        record.cameraConnection.Disconnect();
        record.sceneConnection.Disconnect();
        cameras_.erase(it);
    }

    auto &record = cameras_[&camera];
    record.scene = &scene;
    record.camera = MakeBox<RenderCamera>(device_, scenes_->GetRenderScene(scene), camera);
    record.sceneConnection = scene.OnDestroy([c = &camera, s = &scene, this]
    {
        if(auto it = cameras_.find(c); it != cameras_.end() && it->second.scene == s)
        {
            it->second.cameraConnection.Disconnect();
            cameras_.erase(it);
        }
    });
    record.cameraConnection = camera.OnDestroy([c = &camera, this]
    {
        if(auto it = cameras_.find(c); it != cameras_.end())
        {
            it->second.sceneConnection.Disconnect();
            cameras_.erase(it);
        }
    });
    return *record.camera;
}

RTRC_RENDERER_END
