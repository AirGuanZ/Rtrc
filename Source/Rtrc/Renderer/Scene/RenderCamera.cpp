#include <Rtrc/Renderer/Common.h>
#include <Rtrc/Renderer/Scene/RenderCamera.h>

RTRC_RENDERER_BEGIN

RenderCamera::RenderCamera(ObserverPtr<Device> device, const RenderScene &scene, UniqueId cameraId)
    : scene_(scene), perObjectDataBufferPool_(device, RHI::BufferUsage::ShaderStructuredBuffer)
{
    renderCamera_.originalId = cameraId;
}

void RenderCamera::Update(
    const CameraRenderData           &camera,
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
    for(const RenderScene::StaticMeshRecord *src : scene_.GetStaticMeshes())
    {
        StaticMeshCBuffer cbuffer;
        cbuffer.localToWorld  = src->proxy->localToWorld;
        cbuffer.worldToLocal  = src->proxy->worldToLocal;
        cbuffer.localToClip   = camera.worldToClip * cbuffer.localToWorld;
        cbuffer.localToCamera = camera.worldToCamera * cbuffer.localToWorld;
        auto cbufferBindingGroup = transientConstantBufferAllocator.CreateConstantBufferBindingGroup(cbuffer);
        
        auto record = linearAllocator.Create<StaticMeshRecord>();
        record->renderMesh            = src->renderMesh;
        record->renderMaterial        = src->renderMaterial;
        record->proxy                 = src->proxy;
        record->perObjectBindingGroup = std::move(cbufferBindingGroup);
        objects_.push_back(record);
    }
    std::ranges::sort(objects_, [](const StaticMeshRecord *lhs, const StaticMeshRecord *rhs)
    {
        return std::make_tuple(
                    lhs->renderMaterial->materialId,
                    lhs->renderMaterial,
                    lhs->renderMesh->meshRenderingData->GetLayout(),
                    lhs->renderMesh) <
               std::make_tuple(
                    rhs->renderMaterial->materialId,
                    rhs->renderMaterial,
                    rhs->renderMesh->meshRenderingData->GetLayout(),
                    rhs->renderMesh);
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
