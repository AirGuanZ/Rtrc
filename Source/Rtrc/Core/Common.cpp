#include <spdlog/spdlog.h>

#include <Rtrc/Core/Common.h>

RTRC_BEGIN

namespace CommonDetail
{

    class SetSpdLogLevel
    {
    public:

        SetSpdLogLevel()
        {
            SetLogLevel(LogLevel::Default);
        }
    };

    SetSpdLogLevel gSetSpdLogLevel;

} // namespace CommonDetail

void SetLogLevel(LogLevel level)
{
    switch(level)
    {
    case LogLevel::Debug:
        spdlog::set_level(spdlog::level::debug);
        break;
    case LogLevel::Release:
        spdlog::set_level(spdlog::level::warn);
        break;
    }
}

void LogVerboseUnformatted(std::string_view msg)
{
    spdlog::debug(msg);
}

void LogInfoUnformatted(std::string_view msg)
{
    spdlog::info(msg);
}

void LogWarningUnformatted(std::string_view msg)
{
    spdlog::warn(msg);
}

void LogErrorUnformatted(std::string_view msg)
{
    spdlog::error(msg);
}

RTRC_END
