#pragma once

#include <filesystem>
#include <set>

#include <Rtrc/Core/Container/Span.h>
#include <Rtrc/Core/Variant.h>

RTRC_BEGIN

namespace DirectoryFilter
{

    struct EmptyCommand   { };
    struct IncludeCommand { std::string range; };
    struct ExcludeCommand { std::string range; };

    inline IncludeCommand operator+(IncludeCommand cmd) { return cmd; }
    inline ExcludeCommand operator-(IncludeCommand cmd) { return ExcludeCommand{ std::move(cmd.range) }; }

    template<typename T>
    concept Command = std::is_same_v<T, IncludeCommand>
                   || std::is_same_v<T, ExcludeCommand>
                   || std::is_same_v<T, EmptyCommand>
                   || requires (T x) { std::string(x); };

    using CommandVariant = Variant<IncludeCommand, ExcludeCommand>;

    inline IncludeCommand Include(std::string range) { return { std::move(range) }; }
    inline ExcludeCommand Exclude(std::string range) { return { std::move(range) }; }

    std::set<std::filesystem::path> FilterFiles(Span<CommandVariant> commands);

    template<Command...Args>
    std::set<std::filesystem::path> FilterFiles(const Args&...commands);

    void Apply(std::set<std::filesystem::path> &set, const IncludeCommand &command);
    void Apply(std::set<std::filesystem::path> &set, const ExcludeCommand &command);
    void Apply(std::set<std::filesystem::path> &set, const EmptyCommand   &command);

    template<typename T> requires requires (T x) { std::string(x); }
    void Apply(std::set<std::filesystem::path> &set, T &&command)
    {
        Apply(set, Include(std::string(std::forward<T>(command))));
    }

} // namespace DirectoryFilter

#define $rtrc_get_files(...)                                      \
    ([&]                                                          \
    {                                                             \
        auto add    = ::Rtrc::DirectoryFilter::Include;           \
        auto remove = ::Rtrc::DirectoryFilter::Exclude;           \
        return ::Rtrc::DirectoryFilter::FilterFiles(__VA_ARGS__); \
    }())

template<DirectoryFilter::Command ... Args>
std::set<std::filesystem::path> DirectoryFilter::FilterFiles(const Args &... commands)
{
    std::set<std::filesystem::path> ret;
    (DirectoryFilter::Apply(ret, commands), ...);
    return ret;
}

RTRC_END
