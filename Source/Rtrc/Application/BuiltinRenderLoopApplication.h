#pragma once

#include <Rtrc/Application/Application.h>
#include <Rtrc/Renderer/RenderLoop/RealTimeRenderLoop.h>

RTRC_BEGIN

class BuiltinRenderLoopApplication : public Application
{
public:

    using Application::Application;

protected:
    
    Renderer::RealTimeRenderLoop::RenderSettings       &GetRenderSettings()       { return activeRenderSettings_; }
    const Renderer::RealTimeRenderLoop::RenderSettings &GetRenderSettings() const { return activeRenderSettings_; }

    virtual void UpdateLogic() { }
    virtual void InitializeLogic() { }
    virtual void DestroyLogic() { }

    ObserverPtr<Scene> GetActiveScene() { return activeScene_; }
    ObserverPtr<Camera> GetActiveCamera() { return activeCamera_; }

private:

    void Initialize() final;
    void Update() final;
    void Destroy() final;
    void ResizeFrameBuffer(uint32_t width, uint32_t height) final;

    Box<Scene>                                   activeScene_;
    Camera                                       activeCamera_;
    Box<Renderer::RealTimeRenderLoop>            renderLoop_;
    Renderer::RealTimeRenderLoop::RenderSettings activeRenderSettings_;
};

RTRC_END
