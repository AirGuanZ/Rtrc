#pragma once

#include <list>
#include <semaphore>
#include <thread>

#include <Rtrc/Graphics/ImGui/Instance.h>
#include <Rtrc/Graphics/RenderGraph/Executable.h>
#include <Rtrc/Renderer/Passes/GBufferPass.h>
#include <Rtrc/Renderer/RenderCommand.h>
#include <Rtrc/Renderer/Scene/CachedScene.h>
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
    
    static float ComputeBuildBlasSortKey(const Vector3f &eye, const StaticMeshRenderProxy *renderer);

    static const BindlessTextureEntry *ExtractAlbedoTextureEntry(const MaterialInstance::SharedRenderingData *material);
    
    void RenderThreadEntry();

    void RenderStandaloneFrame(const RenderCommand_RenderStandaloneFrame &frame);
    
    ObserverPtr<Device>                       device_;
    ObserverPtr<const BuiltinResourceManager> builtinResources_;
    ObserverPtr<BindlessTextureManager>       bindlessTextures_;

    int32_t frameIndex_;
    Timer   frameTimer_;

    Box<ImGuiRenderer>                    imguiRenderer_;
    Box<RG::Executer>                     renderGraphExecuter_;
    Box<BindlessBufferManager>            bindlessStructuredBuffersForBlas_;
    Box<TransientConstantBufferAllocator> transientConstantBufferAllocator_;

    std::jthread                         renderThread_;
    tbb::concurrent_queue<RenderCommand> renderCommandQueue_;

    CachedMeshManager     meshManager_;
    CachedMaterialManager materialManager_;
    
    CachedScene cachedScene_;

    Box<GBufferPass> gbufferPass_;
};

RTRC_RENDERER_END

RTRC_BEGIN

using Renderer::RenderLoop;

RTRC_END
