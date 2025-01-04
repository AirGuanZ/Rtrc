#pragma once

#include <Rtrc/Core/Timer.h>
#include <Rtrc/RHI/Capture/GPUCapturer.h>
#include <Rtrc/ToolKit/ImGui/ImGuiInstance.h>
#include <Rtrc/ToolKit/Resource/BindlessResourceManager.h>
#include <Rtrc/ToolKit/Resource/ResourceManager.h>

RTRC_BEGIN

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

    Application() = default;
    Application(int argc, char *argv[]);

    virtual ~Application() = default;
    
    void Run(const Config &config);

protected:

    virtual void Initialize() { }
    virtual void Update() { }
    virtual void Destroy() { }
    virtual void ResizeFrameBuffer(uint32_t width, uint32_t height) = 0;

    bool GetExitFlag() const;
    void SetExitFlag(bool shouldExit);

    Ref<Device>                 GetDevice()                 const { return device_; }
    const RHI::SwapchainUPtr   &GetSwapchain()              const { return GetDevice()->GetSwapchain(); }
    Ref<ResourceManager>        GetResourceManager()        const { return resourceManager_; }
    Ref<BindlessTextureManager> GetBindlessTextureManager() const { return bindlessTextureManager_; }
    Ref<ImGuiInstance>          GetImGuiInstance()          const { return imgui_; }
    
    Window      &GetWindow();
    WindowInput &GetWindowInput();

          Timer &GetFrameTimer()       { return frameTimer_; }
    const Timer &GetFrameTimer() const { return frameTimer_; }

    void SetGPUCaptureOutput(std::string prefix) { gpuCaptureOutputPrefix_ = std::move(prefix); }
    void AddPendingCaptureFrames(int frames) { pendingGPUCaptureFrames_ += frames; }
    bool IsGPUCapturerAvailable() const { return gpuCapturer_ != nullptr; }

    Box<Window>        window_;
    Ref<WindowInput>   input_;
    Box<Device>        device_;
    Box<ImGuiInstance> imgui_;

    std::vector<std::string> programArguments_;

private:
    
    void UpdateLoop();
    
    Box<ResourceManager>        resourceManager_;
    Box<BindlessTextureManager> bindlessTextureManager_;

    Timer frameTimer_;

    Box<RHI::GPUCapturer> gpuCapturer_;
    std::string           gpuCaptureOutputPrefix_ = "GPUCapture";
    int                   pendingGPUCaptureFrames_ = 0;
    bool                  isCapturing_ = false;
};

class RunWithExceptionHandlingHelper
{
public:

    template<typename F>
    void operator+(F &&f) const
    {
        try
        {
            std::invoke(std::forward<F>(f));
        }
#if RTRC_ENABLE_EXCEPTION_STACKTRACE
        catch(const Exception &e)
        {
            LogError("{}\n{}", e.what(), e.stacktrace());
        }
#endif
        catch(const std::exception &e)
        {
            LogError(e.what());
        }
    }
};

#define RTRC_RUN_WITH_EXCEPTION_HANDLING ::Rtrc::RunWithExceptionHandlingHelper{}+[&]

#define RTRC_APPLICATION_MAIN(APP, ...)                          \
    int main(int argc, char *argv[])                             \
    {                                                            \
        Rtrc::EnableMemoryLeakReporter();                        \
        RTRC_RUN_WITH_EXCEPTION_HANDLING                         \
        {                                                        \
            if constexpr(requires { APP(argc, argv); })          \
            {                                                    \
                APP(argc, argv).Run(APP::Config{ __VA_ARGS__ }); \
            }                                                    \
            else                                                 \
            {                                                    \
                APP().Run(APP::Config{ __VA_ARGS__ });           \
            }                                                    \
        };                                                       \
    }

RTRC_END
