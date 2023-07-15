#include <Standalone/StandaloneApplication.h>

int main()
{
    Rtrc::EnableMemoryLeakReporter();
    try
    {
        StandaloneApplication().Run(Rtrc::Application::Config
        {
            .title                      = "Rtrc Standalone Renderer",
            .width                      = 640,
            .height                     = 480,
            .maximized                  = true,
            .vsync                      = true,
            .debug                      = RTRC_DEBUG,
            .rayTracing                 = true,
            .renderThread               = false,
            .handleCrossThreadException = true,
            .backendType                = Rtrc::RHI::BackendType::Default
        });
    }
    catch(const Rtrc::Exception &e)
    {
        Rtrc::LogError("{}\n{}", e.what(), e.stacktrace());
    }
    catch(const std::exception &e)
    {
        Rtrc::LogError(e.what());
    }
}
