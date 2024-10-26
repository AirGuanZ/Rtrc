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

    thread_local unsigned gLogIndentSize = 2;
    thread_local unsigned gLogIndentLevel = 0;

    constexpr char gIndentChars[128] =
    {
        ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
        ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
        ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
        ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
        ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
        ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
        ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
        ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
        ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
        ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
        ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
        ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
        ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
        ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
        ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
        ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    };

    void _rtrcInternalLogVerboseUnformatted(std::string_view msg)
    {
        spdlog::debug(msg);
    }

    void _rtrcInternalLogInfoUnformatted(std::string_view msg)
    {
        spdlog::info(msg);
    }

    void _rtrcInternalLogWarningUnformatted(std::string_view msg)
    {
        spdlog::warn(msg);
    }

    void _rtrcInternalLogErrorUnformatted(std::string_view msg)
    {
        spdlog::error(msg);
    }

} // namespace CommonDetail

void SetLogLevel(LogLevel level)
{
    switch(level)
    {
    case LogLevel::Debug:
        spdlog::set_level(spdlog::level::debug);
        break;
    case LogLevel::Release:
        spdlog::set_level(spdlog::level::info);
        break;
    }
}

void LogVerboseUnformatted(std::string_view msg)
{
    CommonDetail::_rtrcInternalLogVerboseUnformatted(std::format("{}{}", GetLogIndentString(), msg));
}

void LogInfoUnformatted(std::string_view msg)
{
    CommonDetail::_rtrcInternalLogInfoUnformatted(std::format("{}{}", GetLogIndentString(), msg));
}

void LogWarningUnformatted(std::string_view msg)
{
    CommonDetail::_rtrcInternalLogWarningUnformatted(std::format("{}{}", GetLogIndentString(), msg));
}

void LogErrorUnformatted(std::string_view msg)
{
    CommonDetail::_rtrcInternalLogErrorUnformatted(std::format("{}{}", GetLogIndentString(), msg));
}

void SetLogIndentSize(unsigned size)
{
    assert(size * CommonDetail::gLogIndentLevel < GetArraySize(CommonDetail::gIndentChars));
    CommonDetail::gLogIndentSize = size;
}

unsigned GetLogIndentSize()
{
    return CommonDetail::gLogIndentSize;
}

void SetLogIndentLevel(unsigned level)
{
    assert(CommonDetail::gLogIndentSize * level < GetArraySize(CommonDetail::gIndentChars));
    CommonDetail::gLogIndentLevel = level;
}

unsigned GetLogIndentLevel()
{
    return CommonDetail::gLogIndentLevel;
}

void PushLogIndent()
{
    SetLogIndentLevel(GetLogIndentLevel() + 1);
}

void PopLogIndent()
{
    const unsigned level = GetLogIndentLevel();
    assert(level > 0);
    SetLogIndentLevel(level - 1);
}

std::string_view GetLogIndentString()
{
    return { CommonDetail::gIndentChars, CommonDetail::gLogIndentLevel * CommonDetail::gLogIndentSize };
}

RTRC_END
