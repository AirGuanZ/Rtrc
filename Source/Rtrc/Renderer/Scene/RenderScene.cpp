#include <Rtrc/Renderer/Common.h>
#include <Rtrc/Renderer/Scene/RenderCamera.h>
#include <Rtrc/Core/Enumerate.h>

RTRC_RENDERER_BEGIN

RenderScene::RenderScene(
    const Config                       &config,
    ObserverPtr<Device>                 device,
    ObserverPtr<ResourceManager>        resources,
    ObserverPtr<BindlessTextureManager> bindlessTextures)
    : config_(config)
    , device_(device)
    , scene_(nullptr)
    , bindlessTextures_(bindlessTextures)
    , renderMeshes_(nullptr)
    , renderMaterials_(nullptr)
    , instanceStagingBufferPool_(device, RHI::BufferUsage::TransferSrc)
    , rhiInstanceDataBufferPool_(device, RHI::BufferUsage::AccelerationStructureBuildInput)
{
    renderLights_ = MakeBox<RenderLights>(device);
    renderAtmosphere_ = MakeBox<RenderAtmosphere>(device, resources);
}

void RenderScene::Update(
    const RenderCommand_RenderStandaloneFrame &frame,
    TransientConstantBufferAllocator          &transientConstantBufferAllocator,
    const RenderMeshes                        &renderMeshes,
    const RenderMaterials                     &renderMaterials,
    RG::RenderGraph                           &renderGraph,
    LinearAllocator                           &linearAllocator)
{
    scene_ = frame.scene.get();
    renderMeshes_ = &renderMeshes;
    renderMaterials_ = &renderMaterials;

    CollectObjects(*frame.scene, renderMeshes, renderMaterials, linearAllocator);
    
    // Lights
    
    renderLights_->Update(*scene_, renderGraph);

    // Per-camera scenes

    std::vector<Box<RenderCamera>> newCachedScenesPerCamera;
    for(Box<RenderCamera> &camera : renderCameras_)
    {
        if(camera->GetCameraRenderData().originalId == frame.camera.originalId)
        {
            newCachedScenesPerCamera.push_back(std::move(camera));
            break;
        }
    }
    if(newCachedScenesPerCamera.empty())
    {
        newCachedScenesPerCamera.push_back(MakeBox<RenderCamera>(device_, *this, frame.camera.originalId));
    }
    newCachedScenesPerCamera.back()->Update(frame.camera, transientConstantBufferAllocator, linearAllocator);
    renderCameras_.swap(newCachedScenesPerCamera);

    // Tlas

    if(config_.rayTracing)
    {
        UpdateTlasScene(renderGraph);
    }

    // Atmosphere

    renderAtmosphere_->Update(renderGraph, frame.scene->GetAtmosphere());
}

void RenderScene::ClearFrameData()
{
    objects_.clear();
    tlasObjects_.clear();

    renderMeshes_ = nullptr;
    renderMaterials_ = nullptr;

    buildTlasPass_  = nullptr;
    instanceBuffer_ = nullptr;
    opaqueTlas_     = nullptr;

    renderLights_->ClearFrameData();
    renderAtmosphere_->ClearFrameData();
}

RenderCamera *RenderScene::GetRenderCamera(UniqueId cameraId) const
{
    size_t beg = 0, end = renderCameras_.size();
    while(beg < end)
    {
        const size_t mid = (beg + end) / 2;
        if(renderCameras_[mid]->GetCameraRenderData().originalId == cameraId)
        {
            return renderCameras_[mid].get();
        }
        if(renderCameras_[mid]->GetCameraRenderData().originalId < cameraId)
        {
            beg = mid;
        }
        else
        {
            end = mid;
        }
    }
    return nullptr;
}

void RenderScene::CollectObjects(
    const SceneProxy      &scene,
    const RenderMeshes    &meshManager,
    const RenderMaterials &materialManager,
    LinearAllocator       &linearAllocator)
{
    objects_.clear();
    tlasObjects_.clear();

    for(const StaticMeshRenderProxy *staticMeshProxy : scene.GetStaticMeshRenderObjects())
    {
        const UniqueId materialID = staticMeshProxy->materialRenderingData->GetUniqueID();
        const RenderMaterials::RenderMaterial *material = materialManager.FindCachedMaterial(materialID);
        assert(material);

        const UniqueId staticMeshID = staticMeshProxy->meshRenderingData->GetUniqueID();
        const RenderMeshes::RenderMesh *mesh = meshManager.FindCachedMesh(staticMeshID);
        assert(mesh);

        auto record = linearAllocator.Create<StaticMeshRecord>();
        record->proxy          = staticMeshProxy;
        record->renderMesh     = mesh;
        record->renderMaterial = material;
        objects_.push_back(record);

        if(config_.rayTracing)
        {
            using enum StaticMeshRendererRayTracingFlags::Bits;
            const bool inOpaqueTlas = staticMeshProxy->rayTracingFlags.Contains(InOpaqueTlas);
            if(inOpaqueTlas && mesh->blas && material->albedoTextureEntry)
            {
                tlasObjects_.push_back(record);
            }
        }
    }
}

void RenderScene::UpdateTlasScene(RG::RenderGraph &renderGraph)
{
    // Prepare instance data & material data
    
    std::vector<RHI::RayTracingInstanceData> rhiInstanceData;
    rhiInstanceData.reserve(tlasObjects_.size());
    
    std::vector<TlasInstance> instanceData;
    instanceData.reserve(tlasObjects_.size());
    
    for(auto &&[instanceIndex, object] : Enumerate(tlasObjects_))
    {
        RHI::RayTracingInstanceData &rhiInstance = rhiInstanceData.emplace_back();
        std::memcpy(rhiInstance.transform3x4, &object->proxy->localToWorld[0][0], sizeof(rhiInstance.transform3x4));
        rhiInstance.instanceCustomIndex          = 0;
        rhiInstance.instanceMask                 = 0xff;
        rhiInstance.instanceSbtOffset            = 0;
        rhiInstance.flags                        = 0;
        rhiInstance.accelerationStructureAddress = object->renderMesh->blas->GetRHIObject()->GetDeviceAddress().address;
    
        TlasInstance &instance = instanceData.emplace_back();
        instance.albedoTextureIndex        = object->renderMaterial->albedoTextureEntry->GetOffset();
        instance.albedoScale               = object->renderMaterial->albedoScale;

        const uint32_t geometryBufferOffset = object->renderMesh->geometryBufferEntry.GetOffset();
        const bool isIndex16Bit = object->renderMesh->meshRenderingData->GetIndexFormat() == RHI::IndexFormat::UInt16;
        const bool hasIndexBuffer = object->renderMesh->hasIndexBuffer;
        instance.encodedGeometryBufferInfo = geometryBufferOffset |
                                            ((isIndex16Bit ? 1 : 0) << 14) |
                                            ((hasIndexBuffer ? 1 : 0) << 15);
    }

    // Upload instance data

    {
        const size_t instanceDataSize = sizeof(TlasInstance) * instanceData.size();
        const size_t instanceDataBufferSize = std::max<size_t>(instanceDataSize, 1024);
        assert(!instanceBuffer_);
        instanceBuffer_ = renderGraph.CreateBuffer(RHI::BufferDesc
        {
            .size  = instanceDataBufferSize,
            .usage = RHI::BufferUsage::ShaderStructuredBuffer | RHI::BufferUsage::TransferDst
        }, "Instance data buffer");

        if(!instanceData.empty())
        {
            auto stagingBuffer = instanceStagingBufferPool_.Acquire(instanceDataBufferSize);
            stagingBuffer->SetName("Instance data staging buffer");
            stagingBuffer->Upload(instanceData.data(), 0, instanceDataSize);

            auto uploadInstanceDataPass = renderGraph.CreatePass("Upload instance data buffer");
            uploadInstanceDataPass->Use(instanceBuffer_, RG::CopyDst);
            uploadInstanceDataPass->SetCallback([
                src = stagingBuffer,
                dst = instanceBuffer_,
                size = instanceDataBufferSize]
            {
                RG::GetCurrentCommandBuffer().CopyBuffer(*dst->Get(), 0, *src, 0, size);
            });
        }
    }
    
    // Build tlas
    
    const size_t rhiInstanceDataSize = sizeof(RHI::RayTracingInstanceData) * rhiInstanceData.size();
    const size_t rhiInstanceDataBufferSize = std::max<size_t>(rhiInstanceDataSize, 1024);
    RC<Buffer> rhiInstanceDataBuffer = rhiInstanceDataBufferPool_.Acquire(rhiInstanceDataBufferSize);
    rhiInstanceDataBuffer->SetName("Tlas Instance Data Buffer");
    rhiInstanceDataBuffer->Upload(rhiInstanceData.data(), 0, rhiInstanceDataSize);
    
    const RHI::RayTracingInstanceArrayDesc instanceArrayDesc =
    {
        .instanceCount = static_cast<uint32_t>(rhiInstanceData.size()),
        .instanceData  = rhiInstanceDataBuffer->GetDeviceAddress()
    };
    TlasPrebuildInfo prebuildInfo = device_->CreateTlasPrebuildInfo(
        instanceArrayDesc, RHI::RayTracingAccelerationStructureBuildFlagBit::PreferFastBuild);
    
    const size_t tlasScratchBufferSize = prebuildInfo.GetBuildScratchBufferSize();
    auto opaqueTlasScratchBuffer = renderGraph.CreateBuffer(RHI::BufferDesc
    {
        .size           = tlasScratchBufferSize + device_->GetAccelerationStructureScratchBufferAlignment(),
        .usage          = RHI::BufferUsage::AccelerationStructureScratch,
        .hostAccessType = RHI::BufferHostAccessType::None
    }, "Opaque tlas scratch buffer");
    
    const size_t tlasBufferSize = prebuildInfo.GetAccelerationStructureBufferSize();
    if(!cachedOpaqueTlasBuffer_ || cachedOpaqueTlasBuffer_->GetSize() < tlasBufferSize)
    {
        auto buffer = device_->CreateBuffer(RHI::BufferDesc
        {
            .size           = tlasBufferSize,
            .usage          = RHI::BufferUsage::AccelerationStructure,
            .hostAccessType = RHI::BufferHostAccessType::None
        });
        buffer->SetName("Tlas Buffer");
        cachedOpaqueTlasBuffer_ = StatefulBuffer::FromBuffer(std::move(buffer));
        cachedOpaqueTlas_ = device_->CreateTlas(cachedOpaqueTlasBuffer_);
    }
    assert(!opaqueTlas_);
    opaqueTlas_ = renderGraph.RegisterTlas(cachedOpaqueTlas_, renderGraph.RegisterBuffer(cachedOpaqueTlasBuffer_));

    buildTlasPass_ = renderGraph.CreatePass("Build Tlas");
    buildTlasPass_->Build(opaqueTlas_);
    buildTlasPass_->Use(opaqueTlasScratchBuffer, RG::BuildAS_Scratch);
    buildTlasPass_->SetCallback([
        info = std::move(prebuildInfo), instanceArrayDesc, tlas = cachedOpaqueTlas_, scratch = opaqueTlasScratchBuffer]
    {
        RG::GetCurrentCommandBuffer().BuildTlas(tlas, instanceArrayDesc, info, scratch->Get());
    });
}

RTRC_RENDERER_END
