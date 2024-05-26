#include <Rtrc/Rtrc.h>

using namespace Rtrc;

template<typename Func>
struct FunctionTrait
{
    using T = typename FunctionTrait<decltype(std::function{ std::declval<Func>() })>::T;
};

template<typename...Args>
struct FunctionTrait<std::function<void(Args...)>>
{
    using T = std::function<void(Args...)>;
};

class ShaderDSLDemo : public SimpleApplication
{
    void InitializeSimpleApplication(GraphRef graph) override
    {
        auto entry = eDSL::BuildComputeEntry(
            GetDevice(),
            [](const eDSL::RWBuffer<eDSL::u32> &buffer, const eDSL::u32 &threadCount, const eDSL::u32 &value)
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
