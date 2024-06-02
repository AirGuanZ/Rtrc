#include <Rtrc/Rtrc.h>
#include <Rtrc/eDSL.h>

using namespace Rtrc;
using namespace eDSL;

class ShaderDSLDemo : public SimpleApplication
{
    void InitializeSimpleApplication(GraphRef graph) override
    {
        using namespace eDSL;

        entry_ = BuildComputeEntry(
            GetDevice(),
            [](eRWBuffer<euint> buffer, euint threadCount, euint value)
            {
                $numthreads(64, 1, 1);
                $if($SV_DispatchThreadID.x < threadCount)
                {
                    buffer[$SV_DispatchThreadID.x] = value;
                };
            });
    }

    void UpdateSimpleApplication(GraphRef graph) override
    {
        if(GetWindowInput().IsKeyDown(KeyCode::Escape))
        {
            SetExitFlag(true);
        }

        RGClearColor(
            graph, "ClearFramebuffer",
            graph->RegisterSwapchainTexture(GetDevice()->GetSwapchain()),
            Vector4f(0, 1, 1, 0));

        auto buffer = graph->CreateBuffer(RHI::BufferDesc
        {
            .size = 16 * sizeof(uint16_t),
            .usage = RHI::BufferUsage::ShaderRWBuffer
        });
        buffer->SetDefaultTexelFormat(RHI::Format::R16_UInt);

        RGDispatchWithThreadCount(graph, "Pass1", entry_, 16, buffer, 16, 1);
        RGDispatchWithThreadCount(graph, "Pass2", entry_, 16, buffer, 16, 2);
    }

    ComputeEntry<eRWBuffer<euint>, euint, euint> entry_;
};

RTRC_APPLICATION_MAIN(
    ShaderDSLDemo,
    .title             = "Rtrc Sample: Shader DSL",
    .width             = 640,
    .height            = 480,
    .maximized         = true,
    .vsync             = true,
    .debug             = RTRC_DEBUG,
    .rayTracing        = false,
    .backendType       = RHI::BackendType::Default,
    .enableGPUCapturer = false)
