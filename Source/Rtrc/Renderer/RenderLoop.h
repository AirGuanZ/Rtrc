#pragma once

#include <Graphics/ImGui/Instance.h>
#include <Graphics/RenderGraph/Executable.h>
#include <Rtrc/Renderer/Debug/GBufferVisualizer.h>
#include <Rtrc/Renderer/GBuffer/GBufferPass.h>
#include <Rtrc/Renderer/GPUScene/RenderCamera.h>
#include <Rtrc/Renderer/RenderSettings.h>
#include <Core/Timer.h>

RTRC_RENDERER_BEGIN

class RenderLoop : public Uncopyable
{
public:
    
    struct FrameInput
    {
        const Scene         *scene;
        const Camera        *camera;
        const ImGuiDrawData *imguiDrawData;
    };

    RenderLoop(ObserverPtr<ResourceManager> resources, ObserverPtr<BindlessTextureManager> bindlessTextures);
    
    void BeginRenderLoop();
    void EndRenderLoop();

    void SetRenderSettings(const RenderSettings &settings);

    void ResizeFramebuffer(uint32_t width, uint32_t height);

    void RenderFrame(const FrameInput &frame);

private:

    void RenderFrameImpl(const FrameInput &frame);
    
    RenderSettings renderSettings_;

    ObserverPtr<Device>                 device_;
    ObserverPtr<ResourceManager>        resources_;
    ObserverPtr<BindlessTextureManager> bindlessTextures_;

    int32_t frameIndex_;
    Timer   frameTimer_;

    Box<ImGuiRenderer>                    imguiRenderer_;
    Box<RG::Executer>                     renderGraphExecuter_;
    Box<TransientConstantBufferAllocator> transientCBufferAllocator_;

    Box<MeshRenderingCacheManager>     cachedMeshes_;
    Box<MaterialRenderingCacheManager> cachedMaterials_;
    Box<RenderSceneManager>            renderScenes_;
    Box<RenderCameraManager>           renderCameras_;

    Box<GBufferPass>       gbufferPass_;
    Box<PathTracer>        pathTracer_;
    Box<GBufferVisualizer> gbufferVisualizer_;

    bool isSwapchainInvalid_;
};

RTRC_RENDERER_END
