#pragma once

#include <Rtrc/Renderer/RenderLoop.h>
#include <Rtrc/Graphics/ImGui/Instance.h>
#include <Rtrc/Graphics/Resource/ResourceManager.h>
#include <Rtrc/Renderer/DeferredRenderer/DeferredRenderer.h>
#include <Rtrc/Utility/Timer.h>

RTRC_BEGIN

class Application : public Uncopyable
{
public:

    struct Config
    {
        std::string title     = "Rtrc Application";
        uint32_t    width     = 640;
        uint32_t    height    = 480;
        bool        maximized = false;
        bool        vsync     = true;
        bool        debug     = RTRC_DEBUG;
    };
    
    void Run(const Config &config);
    
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
