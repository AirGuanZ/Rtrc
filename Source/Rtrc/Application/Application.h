#pragma once

#include <Graphics/ImGui/Instance.h>
#include <RHI/Capture/GPUCapturer.h>
#include <Rtrc/Renderer/RenderLoop.h>
#include <Rtrc/Resource/ResourceManager.h>
#include <Core/Timer.h>

RTRC_BEGIN

struct ApplicationInitializeContext
{
    Device                 *device;
    ResourceManager        *resourceManager;
    BindlessTextureManager *bindlessTextureManager;

    Scene  *activeScene;
    Camera *activeCamera;
};

struct ApplicationUpdateContext
{
    Scene         *activeScene;
    Camera        *activeCamera;
    ImGuiInstance *imgui;
    Timer         *frameTimer;
};

class Application : public Uncopyable
{
public:

    struct Config
    {
        std::string      title             = "Rtrc Application";
        uint32_t         width             = 640;
        uint32_t         height            = 480;
        bool             maximized         = false;
        bool             vsync             = true;
        bool             debug             = RTRC_DEBUG;
        bool             rayTracing        = false;
        RHI::BackendType backendType       = RHI::BackendType::Default;
        bool             enableGPUCapturer = false;
    };

    static Application &GetInstance();
    
    Application();
    virtual ~Application();
    
    void Run(const Config &config);

protected:

    virtual void Initialize(const ApplicationInitializeContext &context);
    virtual void Update(const ApplicationUpdateContext &context);
    virtual void Destroy();

    bool GetExitFlag() const;
    void SetExitFlag(bool shouldExit);

    BindlessTextureManager &GetBindlessTextureManager();

    Window      &GetWindow();
    WindowInput &GetWindowInput();

          Renderer::RenderSettings &GetRenderSettings()       { return activeRenderSettings_; }
    const Renderer::RenderSettings &GetRenderSettings() const { return activeRenderSettings_; }

    void SetGPUCaptureOutput(std::string prefix) { gpuCaptureOutputPrefix_ = std::move(prefix); }
    void AddPendingCaptureFrames(int frames) { pendingGPUCaptureFrames_ += frames; }
    bool IsGPUCapturerAvailable() const { return gpuCapturer_ != nullptr; }

private:
    
    void UpdateLoop();
    
    Window                      window_;
    Box<Device>                 device_;
    Box<ImGuiInstance>          imgui_;
    Box<ResourceManager>        resourceManager_;
    Box<BindlessTextureManager> bindlessTextureManager_;
    Box<Renderer::RenderLoop>   renderLoop_;

    Box<RHI::GPUCapturer> gpuCapturer_;
    std::string           gpuCaptureOutputPrefix_ = "GPUCapture";
    int                   pendingGPUCaptureFrames_ = 0;
    bool                  isCapturing_ = false;

    Box<Scene>               activeScene_;
    Camera                   activeCamera_;
    Renderer::RenderSettings activeRenderSettings_;

    Renderer::VisualizationMode visualizationMode_ = Renderer::VisualizationMode::None;
};

RTRC_END
