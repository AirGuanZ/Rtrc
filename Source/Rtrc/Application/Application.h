#pragma once

#include <Rtrc/Renderer/RenderLoop.h>
#include <Rtrc/Graphics/ImGui/Instance.h>
#include <Rtrc/Graphics/Resource/ResourceManager.h>
#include <Rtrc/Utility/Timer.h>

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
        std::string title                      = "Rtrc Application";
        uint32_t    width                      = 640;
        uint32_t    height                     = 480;
        bool        maximized                  = false;
        bool        vsync                      = true;
        bool        debug                      = RTRC_DEBUG;
        bool        rayTracing                 = false;
        bool        handleCrossThreadException = true;
    };

    virtual ~Application() = default;
    
    void Run(const Config &config);

protected:

    virtual void Initialize(const ApplicationInitializeContext &context);
    virtual void Update(const ApplicationUpdateContext &context);
    virtual void Destroy();

    bool GetExitFlag() const;
    void SetExitFlag(bool shouldExit);

    Window      &GetWindow();
    WindowInput &GetWindowInput();

private:
    
    void UpdateLoop();
    
    Window                      window_;
    Box<Device>                 device_;
    Box<ImGuiInstance>          imgui_;
    Box<ResourceManager>        resourceManager_;
    Box<BindlessTextureManager> bindlessTextureManager;
    Box<RenderLoop>             renderLoop_;

    Box<Scene> activeScene_;
    Camera     activeCamera_;
};

RTRC_END
