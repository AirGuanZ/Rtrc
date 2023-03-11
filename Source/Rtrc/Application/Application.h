#pragma once

#include <Rtrc/Application/RenderCommand.h>
#include <Rtrc/Renderer/DeferredRenderer/DeferredRenderer.h>
#include <Rtrc/Utility/Timer.h>

RTRC_BEGIN

class Application : public Uncopyable
{
public:

    struct Config
    {
        std::string title  = "Rtrc Application";
        uint32_t    width  = 640;
        uint32_t    height = 480;
        bool        vsync  = true;
        bool        debug  = RTRC_DEBUG;
    };
    
    void Run(const Config &config);
    
private:
    
    void RenderLoop();
    void UpdateLoop();

    void Render(const Camera *camera, const SceneProxy *scene);

    Window      window_;
    Box<Device> device_;

    Box<RG::Executer> renderGraphExecutor_;
    
    std::jthread renderThread_;
    tbb::concurrent_queue<RenderCommand> renderCommandQueue_;

    Timer frameTimer_;
};

RTRC_END
