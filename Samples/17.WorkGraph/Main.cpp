#include <Rtrc/Rtrc.h>

#include "WorkGraph.shader.outh"

using namespace Rtrc;

class WorkGraphDemo : public SimpleApplication
{
    void InitializeSimpleApplication(GraphRef graph) override
    {
        pipeline_ = device_->CreateWorkGraphPipeline(WorkGraphPipeline::Desc
        {
            .shaders = { device_->GetShader<RtrcShader::WorkGraph::Name>() },
            .entryNodes = { { "EntryNode", 0 } }
        });

        // TODO
    }

    void UpdateSimpleApplication(GraphRef graph) override
    {
        if(input_->IsKeyDown(KeyCode::Escape))
        {
            SetExitFlag(true);
        }

        auto swapchainTexture = graph->RegisterSwapchainTexture(device_->GetSwapchain());
        RGClearColor(graph, "ClearSwapchainTexture", swapchainTexture, { 0, 1, 1, 0 });
    }

    RC<WorkGraphPipeline> pipeline_;
};

RTRC_APPLICATION_MAIN(
    WorkGraphDemo,
    .title             = "Rtrc Sample: Work Graph",
    .width             = 640,
    .height            = 480,
    .maximized         = true,
    .vsync             = true,
    .debug             = RTRC_DEBUG,
    .rayTracing        = false,
    .backendType       = RHI::BackendType::Default,
    .enableGPUCapturer = false)
