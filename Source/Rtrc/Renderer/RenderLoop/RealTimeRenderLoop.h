#pragma once

#include <Graphics/ImGui/Instance.h>
#include <Graphics/RenderGraph/Executable.h>
#include <Rtrc/Renderer/Debug/GBufferVisualizer.h>
#include <Rtrc/Renderer/DeferredLighting/DeferredLighting.h>
#include <Rtrc/Renderer/GBuffer/GBufferPass.h>
#include <Rtrc/Renderer/GPUScene/RenderCamera.h>
#include <Rtrc/Renderer/PathTracer/PathTracer.h>
#include <Rtrc/Renderer/ReSTIR/ReSTIR.h>
#include <Rtrc/Renderer/RenderLoop/RenderLoop.h>
#include <Core/Timer.h>

RTRC_RENDERER_BEGIN

class RealTimeRenderLoop : public RenderLoop, public Uncopyable
{
public:

    enum class VisualizationMode
    {
        None,
        IndirectDiffuse,
        Normal,
        ReSTIRDirectIllumination,
        Count
    };

    static const char *GetVisualizationModeName(VisualizationMode mode)
    {
        constexpr const char *ret[] = { "None", "IndirectDiffuse", "Normal", "ReSTIR", "Count" };
        return ret[std::to_underlying(mode)];
    }

    struct RenderSettings
    {
        bool              enableIndirectDiffuse = false;
        VisualizationMode visualizationMode = VisualizationMode::None;

        unsigned int ReSTIR_M = 4;
        unsigned int ReSTIR_MaxM = 64;

        unsigned int ReSTIR_N = 8;
        float        ReSTIR_Radius = 25.0f;
        bool         ReSTIR_EnableTemporalReuse = true;

        float        ReSTIR_SVGFTemporalFilterAlpha = 0.05f;
        unsigned int ReSTIR_SVGFSpatialFilterIterations = 2;
    };

    RealTimeRenderLoop(
        ObserverPtr<ResourceManager>        resources,
        ObserverPtr<BindlessTextureManager> bindlessTextures);
    
    void BeginRenderLoop() override;
    void EndRenderLoop() override;

    void SetRenderSettings(const RenderSettings &settings);

    void ResizeFramebuffer(uint32_t width, uint32_t height) override;
    void RenderFrame(const FrameInput &frame) override;

private:

    void RenderFrameImpl(const FrameInput &frame);
    
    RenderSettings renderSettings_;

    ObserverPtr<Device>                 device_;
    ObserverPtr<ResourceManager>        resources_;
    ObserverPtr<BindlessTextureManager> bindlessTextures_;

    int32_t frameIndex_;
    Timer   frameTimer_;

    Box<ImGuiRenderer>                    imguiRenderer_;
    Box<TransientConstantBufferAllocator> transientCBufferAllocator_;
    Box<RG::Executer>                     renderGraphExecuter_;

    Box<MeshRenderingCacheManager>     cachedMeshes_;
    Box<MaterialRenderingCacheManager> cachedMaterials_;
    Box<RenderSceneManager>            renderScenes_;
    Box<RenderCameraManager>           renderCameras_;

    Box<GBufferPass>       gbufferPass_;
    Box<PathTracer>        pathTracer_;
    Box<GBufferVisualizer> gbufferVisualizer_;
    Box<ReSTIR>            restir_;
    Box<DeferredLighting>  deferredLighting_;

    bool isSwapchainInvalid_;
};

RTRC_RENDERER_END
