#pragma once

#include <Rtrc/ToolKit/Application/Application.h>

RTRC_BEGIN

class SimpleApplication : public Application
{
public:

    using Application::Application;

protected:

    virtual void InitializeSimpleApplication() { }
    virtual void UpdateSimpleApplication(Ref<RG::RenderGraph> renderGraph) { }
    virtual void DestroySimpleApplication() { }

    void ExecuteStandaloneRenderGraph(Ref<RG::RenderGraph> graph, bool enableTransientResourcePool = false);

private:

    void Initialize() final;
    void Update() final;
    void Destroy() final;
    void ResizeFrameBuffer(uint32_t width, uint32_t height) final;

    Box<RG::Executer>  renderGraphExecuter_;
    Box<ImGuiRenderer> imguiRenderer_;
    bool               isSwapchainInvalid = false;
};

RTRC_END
