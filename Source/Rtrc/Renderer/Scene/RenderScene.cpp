#include <Rtrc/Renderer/Common.h>
#include <Rtrc/Renderer/Scene/RenderCamera.h>

RTRC_RENDERER_BEGIN

RenderScene::RenderScene(
    const Config &config, ObserverPtr<Device> device, ObserverPtr<const BuiltinResourceManager> builtinResources)
    : config_(config)
    , device_(device)
    , scene_(nullptr)
    , materialDataUploadBufferPool_(device, RHI::BufferUsage::TransferSrc)
    , instanceDataUploadBufferPool_(device, RHI::BufferUsage::AccelerationStructureBuildInput)
{
    renderLights_ = MakeBox<RenderLights>(device);
    renderAtmosphere_ = MakeBox<RenderAtmosphere>(device, builtinResources);
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

    buildTlasPass_        = nullptr;
    tlasMaterialBuffer_   = nullptr;
    opaqueTlas_           = nullptr;

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
    
    std::vector<RHI::RayTracingInstanceData> instanceData;
    instanceData.reserve(tlasObjects_.size());
    
    std::vector<TlasMaterial> materialData;
    materialData.reserve(tlasObjects_.size());
    
    for(const StaticMeshRecord *object : tlasObjects_)
    {
        RHI::RayTracingInstanceData &instance = instanceData.emplace_back();
        std::memcpy(instance.transform3x4, &object->proxy->localToWorld[0][0], sizeof(instance.transform3x4));
        instance.instanceCustomIndex          = object->renderMesh->geometryBufferEntry.GetOffset();
        instance.instanceMask                 = 0xff;
        instance.instanceSbtOffset            = 0;
        instance.flags                        = 0;
        instance.accelerationStructureAddress = object->renderMesh->blas->GetRHIObject()->GetDeviceAddress().address;
    
        TlasMaterial &material = materialData.emplace_back();
        material.albedoTextureIndex = object->renderMaterial->albedoTextureEntry->GetOffset();
        material.albedoScale        = object->renderMaterial->albedoScale;
    }
    
    // Upload material data
    
    const size_t materialDataSize = sizeof(TlasMaterial) * materialData.size();
    const size_t materialDataBufferSize = std::max<size_t>(materialDataSize, 1024);
    assert(!tlasMaterialBuffer_);
    tlasMaterialBuffer_ = renderGraph.CreateBuffer(RHI::BufferDesc
    {
        .size           = materialDataBufferSize,
        .usage          = RHI::BufferUsage::ShaderStructuredBuffer | RHI::BufferUsage::TransferDst,
        .hostAccessType = RHI::BufferHostAccessType::None
    }, "Tlas material data buffer");
    
    auto prepareTlasMaterialDataPass = renderGraph.CreatePass("Prepare Tlas Material Data Buffer");
    if(!materialData.empty())
    {
        RC<Buffer> materialDataUploadBuffer = materialDataUploadBufferPool_.Acquire(materialDataBufferSize);
        materialDataUploadBuffer->SetName("Tlas Material Data Upload Buffer");
        materialDataUploadBuffer->Upload(materialData.data(), 0, materialDataSize);
    
        prepareTlasMaterialDataPass->Use(tlasMaterialBuffer_, RG::CopyDst);
        prepareTlasMaterialDataPass->SetCallback([
            src = materialDataUploadBuffer,
            dst = tlasMaterialBuffer_,
            size = materialDataBufferSize]
            (RG::PassContext &context)
        {
            context.GetCommandBuffer().CopyBuffer(*dst->Get(context), 0, *src, 0, size);
        });
    }
    
    // Build tlas
    
    const size_t instanceDataSize = sizeof(RHI::RayTracingInstanceData) * instanceData.size();
    const size_t instanceDataBufferSize = std::max<size_t>(instanceDataSize, 1024);
    RC<Buffer> instanceDataBuffer = instanceDataUploadBufferPool_.Acquire(instanceDataBufferSize);
    instanceDataBuffer->SetName("Tlas Instance Data Buffer");
    instanceDataBuffer->Upload(instanceData.data(), 0, instanceDataSize);
    
    const RHI::RayTracingInstanceArrayDesc instanceArrayDesc =
    {
        .instanceCount = static_cast<uint32_t>(instanceData.size()),
        .instanceData  = instanceDataBuffer->GetDeviceAddress()
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
        (RG::PassContext &context)
    {
        context.GetCommandBuffer().BuildTlas(tlas, instanceArrayDesc, info, scratch->Get(context));
    });
}

RTRC_RENDERER_END
