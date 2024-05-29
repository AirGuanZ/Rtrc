#include <Rtrc/Rtrc.h>

using namespace Rtrc;

class ShaderDSLDemo : public SimpleApplication
{
    void InitializeSimpleApplication(GraphRef graph) override
    {
        using namespace eDSL;

        auto entry = BuildComputeEntry(
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
        SetExitFlag(true);
    }
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
