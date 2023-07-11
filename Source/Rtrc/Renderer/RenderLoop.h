#pragma once

#include <list>
#include <semaphore>
#include <thread>

#include <Rtrc/Graphics/ImGui/Instance.h>
#include <Rtrc/Graphics/RenderGraph/Executable.h>
#include <Rtrc/Renderer/Passes/DeferredLightingPass.h>
#include <Rtrc/Renderer/Passes/GBufferPass.h>
#include <Rtrc/Renderer/Passes/ShadowMaskPass.h>
#include <Rtrc/Renderer/RenderCommand.h>
#include <Rtrc/Renderer/Scene/RenderCamera.h>
#include <Rtrc/Utility/Timer.h>

RTRC_RENDERER_BEGIN

class RenderLoop : public Uncopyable
{
public:

    struct Config
    {
        bool rayTracing                 = false;
        bool handleCrossThreadException = true;
    };
    
    RenderLoop(
        const Config                       &config,
        ObserverPtr<Device>                 device,
        ObserverPtr<BuiltinResourceManager> builtinResources,
        ObserverPtr<BindlessTextureManager> bindlessTextures);
    ~RenderLoop();

    void AddCommand(RenderCommand command);

    bool HasException() const { return hasException_; }

    const std::exception_ptr &GetExceptionPointer() const { return exceptionPtr_; }

private:

    static constexpr int MAX_BLAS_BINDLESS_BUFFER_COUNT = 4096;

    struct MaterialDataPerInstanceInOpaqueTlas
    {
        uint32_t albedoTextureIndex = 0;
    };
    
    static const BindlessTextureEntry *ExtractAlbedoTextureEntry(const MaterialInstance::SharedRenderingData *material);
    
    void RenderThreadEntry();

    void RenderStandaloneFrame(const RenderCommand_RenderStandaloneFrame &frame);

    Config config_;

    std::atomic<bool>  hasException_;
    std::exception_ptr exceptionPtr_;

    ObserverPtr<Device>                 device_;
    ObserverPtr<BuiltinResourceManager> builtinResources_;
    ObserverPtr<BindlessTextureManager> bindlessTextures_;

    int32_t frameIndex_;
    Timer   frameTimer_;

    Box<ImGuiRenderer>                    imguiRenderer_;
    Box<RG::Executer>                     renderGraphExecuter_;
    Box<TransientConstantBufferAllocator> transientConstantBufferAllocator_;

    std::jthread                         renderThread_;
    tbb::concurrent_queue<RenderCommand> renderCommandQueue_;

    RenderMeshes    renderMeshes_;
    RenderMaterials renderMaterials_;
    
    RenderScene renderScene_;
    
    Box<GBufferPass>            gbufferPass_;
    Box<DeferredLightingPass>   deferredLightingPass_;
    Box<ShadowMaskPass>         shadowMaskPass_;
};

RTRC_RENDERER_END

RTRC_BEGIN

using Renderer::RenderLoop;

RTRC_END
