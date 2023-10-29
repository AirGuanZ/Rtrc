#include <Standalone/StandaloneApplication.h>

int main()
{
    Rtrc::EnableMemoryLeakReporter();
    try
    {
        StandaloneApplication().Run(
        {
            .title             = "Rtrc Standalone Renderer",
            .width             = 640,
            .height            = 480,
            .maximized         = true,
            .vsync             = true,
            .debug             = RTRC_DEBUG,
            .backendType       = Rtrc::RHI::BackendType::DirectX12,
            .enableGPUCapturer = false
        });
    }
    catch(const Rtrc::Exception &e)
    {
#if RTRC_ENABLE_EXCEPTION_STACKTRACE
        Rtrc::LogError("{}\n{}", e.what(), e.stacktrace());
#else
        Rtrc::LogErrorUnformatted(e.what());
#endif
    }
    catch(const std::exception &e)
    {
        Rtrc::LogErrorUnformatted(e.what());
    }
}
