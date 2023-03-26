#include <Rtrc/Renderer/RenderLoopScene/CachedScene.h>

RTRC_RENDERER_BEGIN

CachedScene::CachedScene(ObserverPtr<Device> device)
    : device_(device)
    , materialDataUploadBufferPool_(device, RHI::BufferUsage::TransferSrc, 1)
    , instanceDataUploadBufferPool_(
        device, RHI::BufferUsage::AccelerationStructureBuildInput | RHI::BufferUsage::DeviceAddress, 1)
{
    
}

CachedScene::RenderGraphInterface CachedScene::Update(
    const RenderCommand_RenderStandaloneFrame &frame,
    const CachedMeshManager                   &meshManager,
    const CachedMaterialManager               &materialManager,
    RG::RenderGraph                           &renderGraph)
{
    objectAllocator_.Destroy();
    objects_.clear();
    tlasObjects_.clear();

    RenderGraphInterface ret;

    // Collect objects

    for(const StaticMeshRenderProxy *staticMeshProxy : frame.scene->GetStaticMeshRenderObjects())
    {
        const CachedMaterialManager::CachedMaterial *material =
            materialManager.FindCachedMaterial(staticMeshProxy->materialRenderingData->GetUniqueID());
        assert(material);

        const CachedMeshManager::CachedMesh *mesh =
            meshManager.FindCachedMesh(staticMeshProxy->meshRenderingData->GetUniqueID());
        assert(mesh);

        auto record = objectAllocator_.Create<StaticMeshRecord>();
        record->proxy          = staticMeshProxy;
        record->cachedMesh     = mesh;
        record->cachedMaterial = material;
        objects_.push_back(record);

        const bool inOpaqueTlas =
            staticMeshProxy->rayTracingFlags.contains(StaticMeshRendererRayTracingFlagBit::InOpaqueTlas);
        if(inOpaqueTlas && mesh->blas && material->albedoTextureEntry)
        {
            tlasObjects_.push_back(record);
        }
    }

    // Prepare instance data & material data

    std::vector<RHI::RayTracingInstanceData> instanceData;
    instanceData.reserve(tlasObjects_.size());

    std::vector<TlasMaterial> materialData;
    materialData.reserve(tlasObjects_.size());

    for(const StaticMeshRecord *object : tlasObjects_)
    {
        RHI::RayTracingInstanceData &instance = instanceData.emplace_back();
        std::memcpy(instance.transform3x4, &object->proxy->localToWorld[0][0], sizeof(instance.transform3x4));
        instance.instanceCustomIndex          = object->cachedMesh->geometryBufferEntry.GetOffset();
        instance.instanceMask                 = 0xff;
        instance.instanceSbtOffset            = 0;
        instance.flags                        = 0;
        instance.accelerationStructureAddress = object->cachedMesh->blas->GetRHIObject()->GetDeviceAddress().address;

        TlasMaterial &material = materialData.emplace_back();
        material.albedoTextureIndex = object->cachedMaterial->albedoTextureEntry->GetOffset();
    }

    // Upload material data

    const size_t materialDataSize = sizeof(TlasMaterial) * materialData.size();
    const size_t materialDataBufferSize = std::max<size_t>(materialDataSize, 1024);
    RC<Buffer> materialDataUploadBuffer = materialDataUploadBufferPool_.Acquire(materialDataBufferSize);
    materialDataUploadBuffer->Upload(materialData.data(), 0, materialDataSize);

    if(!tlasMaterials_ || tlasMaterials_->GetSize() < materialDataBufferSize)
    {
        auto buffer = device_->CreateBuffer(RHI::BufferDesc
        {
            .size           = materialDataBufferSize,
            .usage          = RHI::BufferUsage::ShaderStructuredBuffer | RHI::BufferUsage::TransferDst,
            .hostAccessType = RHI::BufferHostAccessType::None
        });
        tlasMaterials_ = StatefulBuffer::FromBuffer(std::move(buffer));
    }
    ret.tlasMaterialDataBuffer = renderGraph.RegisterBuffer(tlasMaterials_);

    ret.prepareTlasMaterialDataPass = renderGraph.CreatePass("Prepare tlas material data buffer");
    if(!materialData.empty())
    {
        ret.prepareTlasMaterialDataPass->Use(ret.tlasMaterialDataBuffer, RG::COPY_DST);
        ret.prepareTlasMaterialDataPass->SetCallback([
            src = materialDataUploadBuffer,
            dst = ret.tlasMaterialDataBuffer,
            size = materialDataBufferSize]
            (RG::PassContext &context)
        {
            context.GetCommandBuffer().CopyBuffer(*dst->Get(), 0, *src, 0, size);
        });
    }

    // Build tlas

    const size_t instanceDataSize = sizeof(RHI::RayTracingInstanceData) * instanceData.size();
    const size_t instanceDataBufferSize = std::max<size_t>(instanceDataSize, 1024);
    RC<Buffer> instanceDataUploadBuffer = instanceDataUploadBufferPool_.Acquire(instanceDataBufferSize);
    instanceDataUploadBuffer->Upload(instanceData.data(), 0, instanceDataSize);

    const RHI::RayTracingInstanceArrayDesc instanceArrayDesc =
    {
        .instanceCount = static_cast<uint32_t>(instanceData.size()),
        .instanceData  = instanceDataUploadBuffer->GetDeviceAddress(),
        .opaque        = true
    };
    TlasPrebuildInfo prebuildInfo = device_->CreateTlasPrebuildInfo(
        instanceArrayDesc, RHI::RayTracingAccelerationStructureBuildFlagBit::PreferFastBuild);

    const size_t tlasScratchBufferSize = prebuildInfo.GetBuildScratchBufferSize();
    if(!opaqueTlasScratchBuffer_ || opaqueTlasScratchBuffer_->GetSize() < tlasScratchBufferSize)
    {
        auto buffer = device_->CreateBuffer(RHI::BufferDesc
        {
            .size           = tlasScratchBufferSize + device_->GetAccelerationStructureScratchBufferAlignment(),
            .usage          = RHI::BufferUsage::AccelerationStructureScratch,
            .hostAccessType = RHI::BufferHostAccessType::None
        });
        opaqueTlasScratchBuffer_ = StatefulBuffer::FromBuffer(std::move(buffer));
    }

    const size_t tlasBufferSize = prebuildInfo.GetAccelerationStructureBufferSize();
    if(!opaqueTlasBuffer_ || opaqueTlasBuffer_->GetSize() < tlasBufferSize)
    {
        auto buffer = device_->CreateBuffer(RHI::BufferDesc
        {
            .size           = tlasBufferSize,
            .usage          = RHI::BufferUsage::AccelerationStructure,
            .hostAccessType = RHI::BufferHostAccessType::None
        });
        opaqueTlasBuffer_ = StatefulBuffer::FromBuffer(std::move(buffer));
        opaqueTlas_ = device_->CreateTlas(opaqueTlasBuffer_);
    }
    ret.tlasBuffer = renderGraph.RegisterBuffer(opaqueTlasBuffer_);

    auto rgTlasScratchBuffer = renderGraph.RegisterBuffer(opaqueTlasScratchBuffer_);
    
    ret.buildTlasPass = renderGraph.CreatePass("Build tlas");
    ret.buildTlasPass->Use(ret.tlasBuffer, RG::BUILD_ACCELERATION_STRUCTURE_OUTPUT);
    ret.buildTlasPass->Use(rgTlasScratchBuffer, RG::BUUILD_ACCELERATION_STRUCTURE_SCRATCH);
    ret.buildTlasPass->SetCallback([
        info = std::move(prebuildInfo), instanceArrayDesc, tlas = opaqueTlas_, scratch = opaqueTlasScratchBuffer_]
        (RG::PassContext &context)
    {
        context.GetCommandBuffer().BuildTlas(tlas, instanceArrayDesc, info, scratch);
    });

    return ret;
}

RTRC_RENDERER_END
