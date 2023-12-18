#include <Core/Enumerate.h>
#include <Rtrc/Renderer/GPUScene/RenderScene.h>

RTRC_RENDERER_BEGIN

RenderScene::RenderScene(
    Ref<ResourceManager>               resources,
    Ref<MaterialRenderingCacheManager> cachedMaterials,
    Ref<MeshRenderingCacheManager>     cachedMeshes,
    Ref<BindlessTextureManager>        bindlessTextures,
    const Scene                               &scene)
    : scene_(scene)
    , device_(resources->GetDevice())
    , resources_(resources)
    , cachedMaterials_(cachedMaterials)
    , cachedMeshes_(cachedMeshes)
    , bindlessTextures_(bindlessTextures)
    , instanceStagingBufferPool_(device_, RHI::BufferUsageFlag::TransferSrc)
    , rhiInstanceDataBufferPool_(device_, RHI::BufferUsageFlag::AccelerationStructureBuildInput)
{
    atmosphere_ = MakeBox<RenderAtmosphere>(resources);
}

void RenderScene::FrameUpdate(const Config &config, RG::RenderGraph &renderGraph)
{
    struct TlasObjectRecord
    {
        const MeshRenderer           *meshRenderer;
        const MeshRenderingCache     *meshCache;
        const MaterialRenderingCache *materialCache;
    };
    std::vector<TlasObjectRecord> tlasObjects;
    scene_.ForEachMeshRenderer([&](const MeshRenderer *meshRenderer)
    {
        if(!meshRenderer->GetFlags().Contains(MeshRendererFlagBit::OpaqueTlas))
            return;

        auto meshCache = cachedMeshes_->FindMeshRenderingCache(meshRenderer->GetMesh().get());
        assert(meshCache);
        if(!meshCache->blas)
            return;

        auto materialCache = cachedMaterials_->FindMaterialRenderingCache(meshRenderer->GetMaterial().get());
        assert(materialCache);
        if(!materialCache->albedoTextureEntry)
            return;

        auto &record = tlasObjects.emplace_back();
        record.meshRenderer  = meshRenderer;
        record.meshCache     = meshCache;
        record.materialCache = materialCache;
    });

    if(config.opaqueTlas)
    {
        // Collect instance data

        std::vector<RHI::RayTracingInstanceData> rhiInstanceData;
        rhiInstanceData.reserve(tlasObjects.size());

        std::vector<TlasInstance> instanceData;
        instanceData.reserve(tlasObjects.size());

        for(auto &&[instanceIndex, object] : Enumerate(tlasObjects))
        {
            auto &rhiInstance = rhiInstanceData.emplace_back();
            rhiInstance.instanceCustomIndex = 0;
            rhiInstance.instanceMask        = 0xff;
            rhiInstance.instanceSbtOffset   = 0;
            rhiInstance.flags               = 0;
            rhiInstance.accelerationStructureAddress = object.meshCache->blas->GetDeviceAddress().address;

            const Matrix4x4f &localToWorld = object.meshRenderer->GetLocalToWorld();
            std::memcpy(&rhiInstance.transform3x4, &localToWorld, sizeof(rhiInstance.transform3x4));

            auto &instance = instanceData.emplace_back();
            instance.albedoTextureIndex = static_cast<uint16_t>(object.materialCache->albedoTextureEntry.GetOffset());
            instance.albedoScale = object.materialCache->albedoScale;

            const uint32_t geometryBufferOffset = object.meshCache->geometryBufferEntry.GetOffset();
            const bool hasIndexBuffer = object.meshCache->hasIndexBuffer;
            instance.encodedGeometryBufferInfo = static_cast<uint16_t>(geometryBufferOffset | (hasIndexBuffer << 15));
        }

        // Upload instance data

        const size_t instanceDataSize = instanceData.size() * sizeof(TlasInstance);
        const size_t instanceDataBufferSize = std::max<size_t>(instanceDataSize, 1024);
        assert(!tlasInstanceBuffer_);
        tlasInstanceBuffer_ = renderGraph.CreateBuffer(RHI::BufferDesc
        {
            .size = instanceDataBufferSize,
            .usage = RHI::BufferUsage::ShaderStructuredBuffer | RHI::BufferUsage::TransferDst
        }, "Opaque tlas instance data buffer");

        if(!instanceData.empty())
        {
            auto stagingBuffer = instanceStagingBufferPool_.Acquire(instanceDataSize);
            stagingBuffer->SetName("Instance data staging buffer");
            stagingBuffer->Upload(instanceData.data(), 0, instanceDataSize);

            auto uploadPass = renderGraph.CreatePass("Upload opaque tlas instance data");
            uploadPass->Use(tlasInstanceBuffer_, RG::CopyDst);
            uploadPass->SetCallback([
                src = stagingBuffer,
                dst = tlasInstanceBuffer_,
                size = instanceDataSize]
            {
                RG::GetCurrentCommandBuffer().CopyBuffer(*dst->Get(), 0, *src, 0, size);
            });
        }

        // Build tlas

        const size_t rhiInstanceDataSize = sizeof(RHI::RayTracingInstanceData) * rhiInstanceData.size();
        const size_t rhiInstanceDataBufferSize = std::max<size_t>(rhiInstanceDataSize, 1024);
        auto rhiInstanceDataBuffer = rhiInstanceDataBufferPool_.Acquire(rhiInstanceDataBufferSize);
        rhiInstanceDataBuffer->SetName("Opaque tlas instance data buffer");
        rhiInstanceDataBuffer->Upload(rhiInstanceData.data(), 0, rhiInstanceDataSize);

        const RHI::RayTracingInstanceArrayDesc instanceArrayDesc =
        {
            .instanceCount = static_cast<uint32_t>(rhiInstanceData.size()),
            .instanceData = rhiInstanceDataBuffer->GetDeviceAddress()
        };
        Box<TlasPrebuildInfo> prebuildInfo = device_->CreateTlasPrebuildInfo(
            instanceArrayDesc, RHI::RayTracingAccelerationStructureBuildFlags::PreferFastBuild);

        const size_t scratchBufferSize = prebuildInfo->GetBuildScratchBufferSize();
        auto scratchBuffer = renderGraph.CreateBuffer(RHI::BufferDesc
        {
            .size  = scratchBufferSize + device_->GetAccelerationStructureScratchBufferAlignment(),
            .usage = RHI::BufferUsage::AccelerationStructureScratch
        });

        const size_t tlasBufferSize = prebuildInfo->GetAccelerationStructureBufferSize();
        if(!cachedOpaqueTlasBuffer_ || cachedOpaqueTlasBuffer_->GetSize() < tlasBufferSize)
        {
            auto newBuffer = device_->CreateBuffer(RHI::BufferDesc
            {
                .size = tlasBufferSize,
                .usage = RHI::BufferUsage::AccelerationStructure
            });
            newBuffer->SetName("Opaque tlas buffer");
            cachedOpaqueTlasBuffer_ = StatefulBuffer::FromBuffer(std::move(newBuffer));
            cachedOpaqueTlas_ = device_->CreateTlas(cachedOpaqueTlasBuffer_);
        }
        assert(!opaqueTlas_);
        opaqueTlas_ = renderGraph.RegisterTlas(cachedOpaqueTlas_, renderGraph.RegisterBuffer(cachedOpaqueTlasBuffer_));

        buildTlasPass_ = renderGraph.CreatePass("Build opaque tlas");
        buildTlasPass_->Build(opaqueTlas_);
        buildTlasPass_->Use(scratchBuffer, RG::BuildAS_Scratch);
        buildTlasPass_->SetCallback([
            info = std::move(prebuildInfo), instanceArrayDesc, tlas = cachedOpaqueTlas_, scratchBuffer]
        {
            RG::GetCurrentCommandBuffer().BuildTlas(tlas, instanceArrayDesc, *info, scratchBuffer->Get());
        });
    }
    else
    {
        assert(!buildTlasPass_ && !tlasInstanceBuffer_ && !opaqueTlas_);
        cachedOpaqueTlasBuffer_ = {};
        cachedOpaqueTlas_ = {};
    }

    // Sky

    atmosphere_->Update(renderGraph, scene_.GetSky().GetAtmosphere());
}

void RenderScene::ClearFrameData()
{
    buildTlasPass_      = nullptr;
    tlasInstanceBuffer_ = nullptr;
    opaqueTlas_         = nullptr;

    atmosphere_->ClearFrameData();
}

RenderSceneManager::RenderSceneManager(
    Ref<ResourceManager>               resources,
    Ref<MaterialRenderingCacheManager> cachedMaterials,
    Ref<MeshRenderingCacheManager>     cachedMeshes,
    Ref<BindlessTextureManager>        bindlessTextures)
    : resources_(resources)
    , cachedMaterials_(cachedMaterials)
    , cachedMeshes_(cachedMeshes)
    , bindlessTextures_(bindlessTextures)
{
    
}

RenderSceneManager::~RenderSceneManager()
{
    for(auto &record : std::ranges::views::values(renderScenes_))
    {
        record.connection.Disconnect();
    }
}

RenderScene &RenderSceneManager::GetRenderScene(const Scene &scene)
{
    if(auto it = renderScenes_.find(&scene); it != renderScenes_.end())
    {
        return *it->second.renderScene;
    }

    auto &record = renderScenes_[&scene];
    record.renderScene = MakeBox<RenderScene>(
        resources_, cachedMaterials_, cachedMeshes_, bindlessTextures_, scene);
    record.connection = scene.OnDestroy([s = &scene, this]
    {
        renderScenes_.erase(s);
    });
    return *record.renderScene;
}

RTRC_RENDERER_END
