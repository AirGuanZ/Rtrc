#include <spdlog/spdlog.h>

#include <Rtrc/Common.h>

RTRC_BEGIN

void LogDebugUnformatted(std::string_view msg)
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
