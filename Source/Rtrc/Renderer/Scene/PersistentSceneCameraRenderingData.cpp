#include <Rtrc/Renderer/Common.h>
#include <Rtrc/Renderer/Scene/PersistentScenecameraRenderingData.h>

RTRC_RENDERER_BEGIN

PersistentSceneCameraRenderingData::PersistentSceneCameraRenderingData(ObserverPtr<Device> device, const PersistentSceneRenderingData &scene, UniqueId cameraId)
    : scene_(scene), perObjectDataBufferPool_(device, RHI::BufferUsage::ShaderStructuredBuffer)
{
    renderCamera_.originalId = cameraId;
}

void PersistentSceneCameraRenderingData::Update(
    const RenderCamera               &camera,
    TransientConstantBufferAllocator &transientConstantBufferAllocator,
    LinearAllocator                  &linearAllocator)
{
    assert(renderCamera_.originalId == camera.originalId);
    renderCamera_ = camera;

    {
        const CameraConstantBuffer cbufferData = CameraConstantBuffer::FromCamera(renderCamera_);
        cameraCBuffer_ = transientConstantBufferAllocator.CreateConstantBuffer(cbufferData);
    }

    objects_.clear();
    for(const PersistentSceneRenderingData::StaticMeshRecord *src : scene_.GetStaticMeshes())
    {
        StaticMeshCBuffer cbuffer;
        cbuffer.localToWorld  = src->proxy->localToWorld;
        cbuffer.worldToLocal  = src->proxy->worldToLocal;
        cbuffer.localToClip   = camera.worldToClip * cbuffer.localToWorld;
        cbuffer.localToCamera = camera.worldToCamera * cbuffer.localToWorld;
        auto cbufferBindingGroup = transientConstantBufferAllocator.CreateConstantBufferBindingGroup(cbuffer);
        
        auto record = linearAllocator.Create<StaticMeshRecord>();
        record->cachedMesh            = src->cachedMesh;
        record->cachedMaterial        = src->cachedMaterial;
        record->proxy                 = src->proxy;
        record->perObjectBindingGroup = std::move(cbufferBindingGroup);
        objects_.push_back(record);
    }
    std::ranges::sort(objects_, [](const StaticMeshRecord *lhs, const StaticMeshRecord *rhs)
    {
        return std::make_tuple(
                    lhs->cachedMaterial->materialId,
                    lhs->cachedMaterial,
                    lhs->cachedMesh->meshRenderingData->GetLayout(),
                    lhs->cachedMesh) <
               std::make_tuple(
                    rhs->cachedMaterial->materialId,
                    rhs->cachedMaterial,
                    rhs->cachedMesh->meshRenderingData->GetLayout(),
                    rhs->cachedMesh);
    });

    std::vector<StaticMeshCBuffer> perObjectData(objects_.size());
    for(size_t i = 0; i < objects_.size(); ++i)
    {
        const StaticMeshRecord *src = objects_[i];
        StaticMeshCBuffer &dst = perObjectData[i];
        dst.localToWorld  = src->proxy->localToWorld;
        dst.worldToLocal  = src->proxy->worldToLocal;
        dst.localToCamera = renderCamera_.worldToCamera * dst.localToWorld;
        dst.localToClip   = renderCamera_.worldToClip * dst.localToWorld;
    }
    perObjectDataBuffer_ = perObjectDataBufferPool_.Acquire(
        sizeof(StaticMeshCBuffer) * std::max<size_t>(perObjectData.size(), 1));
    perObjectDataBuffer_->SetDefaultStructStride(sizeof(StaticMeshCBuffer));
    perObjectDataBuffer_->SetName("PerObjectDataBuffer");
    if(!perObjectData.empty())
    {
        perObjectDataBuffer_->Upload(perObjectData.data(), 0, sizeof(StaticMeshCBuffer) * perObjectData.size());
    }
}

RTRC_RENDERER_END
