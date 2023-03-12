#include <Rtrc/Rtrc.h>

using namespace Rtrc;

int main()
{
    try
    {
        Application().Run(Application::Config{ .maximized = true });
    }
    catch(const Exception &e)
    {
        LogError("{}\n{}", e.what(), e.stacktrace());
    }
    catch(const std::exception &e)
    {
        LogError(e.what());
    }
}
