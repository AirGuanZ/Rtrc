#include <Rtrc/Rtrc.h>

using namespace Rtrc;

int main()
{
    try
    {
        Application().Run(Application::Config{ .maximized = true });
    }
    catch(const std::exception &e)
    {
        LogError(e.what());
    }
}
