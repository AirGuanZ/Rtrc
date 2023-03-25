#pragma once

#include <list>
#include <semaphore>
#include <thread>

#include <Rtrc/Graphics/ImGui/Instance.h>
#include <Rtrc/Graphics/RenderGraph/Executable.h>
#include <Rtrc/Renderer/RenderLoopScene/BlasBuilder.h>
#include <Rtrc/Renderer/RenderCommand.h>
#include <Rtrc/Renderer/RenderLoopScene/RenderLoopMaterialManager.h>
#include <Rtrc/Renderer/RenderLoopScene/RenderLoopMeshManager.h>
#include <Rtrc/Renderer/TlasPool.h>
#include <Rtrc/Scene/Camera/Camera.h>
#include <Rtrc/Scene/Scene.h>
#include <Rtrc/Utility/Timer.h>

RTRC_RENDERER_BEGIN

class RenderLoop : public Uncopyable
{
public:
    
    RenderLoop(
        ObserverPtr<Device>                       device,
        ObserverPtr<const BuiltinResourceManager> builtinResources,
        ObserverPtr<BindlessTextureManager>       bindlessTextures);
    ~RenderLoop();

    void AddCommand(RenderCommand command);

private:

    static constexpr int MAX_COUNT_BINDLESS_STRUCTURE_BUFFER_FOR_BLAS = 4096;

    struct MaterialDataPerInstanceInOpaqueTlas
    {
        uint32_t albedoTextureIndex = 0;
    };
    
    struct TransientScene
    {
        struct StaticMeshRecord
        {
            const StaticMeshRenderProxy                     *renderProxy = nullptr;
            const RenderLoopMeshManager::CachedMesh         *cachedMesh = nullptr;
            const RenderLoopMaterialManager::CachedMaterial *cachedMaterial = nullptr;
        };

        std::vector<StaticMeshRecord> objectsInOpaqueTlas;
        RC<Tlas>                      tlas;

        RC<Buffer> materialDataForOpaqueTlas;
    };
    
    static float ComputeBuildBlasSortKey(const Vector3f &eye, const StaticMeshRenderProxy *renderer);

    static const BindlessTextureEntry *ExtractAlbedoTextureEntry(const MaterialInstance::SharedRenderingData *material);
    
    void RenderThreadEntry();

    void RenderSingleFrame(const RenderCommand_RenderStandaloneFrame &frame);
    
    RC<Tlas> BuildOpaqueTlasForScene(
        const SceneProxy &scene, TransientScene &transientScene, CommandBuffer &commandBuffer);
    
    ObserverPtr<Device>                       device_;
    ObserverPtr<const BuiltinResourceManager> builtinResources_;
    ObserverPtr<BindlessTextureManager>       bindlessTextures_;

    Box<BindlessBufferManager> bindlessStructuredBuffersForBlas_;

    Box<ImGuiRenderer> imguiRenderer_;
    Box<RG::Executer>  renderGraphExecuter_;

    std::jthread                         renderThread_;
    tbb::concurrent_queue<RenderCommand> renderCommandQueue_;

    RenderLoopMeshManager     meshManager_;
    RenderLoopMaterialManager materialManager_;

    RC<Tlas> opaqueTlas_;
    TlasPool tlasPool_;

    int32_t frameIndex_;
    Timer   frameTimer_;

    // Transient context

    Box<TransientScene> transientScene_;
};

RTRC_RENDERER_END

RTRC_BEGIN

using Renderer::RenderLoop;

RTRC_END
