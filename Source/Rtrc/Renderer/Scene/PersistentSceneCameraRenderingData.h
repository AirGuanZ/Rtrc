#pragma once

#include <Rtrc/Renderer/Passes/AtmospherePass.h>
#include <Rtrc/Renderer/Scene/PersistentSceneRenderingData.h>
#include <Rtrc/Renderer/Scene/CachedMeshManager.h>
#include <Rtrc/Renderer/Utility/TransientConstantBufferAllocator.h>
#include <Rtrc/Renderer/Utility/UploadBufferPool.h>
#include <Rtrc/Utility/Memory/Arena.h>

RTRC_RENDERER_BEGIN

class PersistentSceneCameraRenderingData
{
public:

    struct StaticMeshRecord : PersistentSceneRenderingData::StaticMeshRecord
    {
        RC<BindingGroup> perObjectBindingGroup;
    };

    PersistentSceneCameraRenderingData(ObserverPtr<Device> device, const PersistentSceneRenderingData &scene, UniqueId cameraId);
    
    const RenderCamera  &GetCamera() const { return renderCamera_; }
    const RC<SubBuffer> &GetCameraCBuffer() const { return cameraCBuffer_; }

    const PersistentSceneRenderingData &GetCachedScene() const { return scene_; }

    Span<StaticMeshRecord *> GetStaticMeshes() const { return objects_; }
    const RC<Buffer>        &GetStaticMeshPerObjectData() const { return perObjectDataBuffer_; }

    Span<const Light::SharedRenderingData*> GetLights() const { return scene_.GetLights(); }

    PhysicalAtmospherePass::CachedData &GetCachedAtmosphereData() { return atmosphereData_; }

    void Update(
        const RenderCamera               &camera,
        TransientConstantBufferAllocator &transientConstantBufferAllocator,
        LinearAllocator                  &linearAllocator);

private:
    
    const PersistentSceneRenderingData  &scene_;
    RenderCamera        renderCamera_;

    RC<SubBuffer> cameraCBuffer_;

    std::vector<StaticMeshRecord *> objects_;
    RC<Buffer>                      perObjectDataBuffer_;
    UploadBufferPool<>              perObjectDataBufferPool_;

    PhysicalAtmospherePass::CachedData atmosphereData_;
};

RTRC_RENDERER_END
