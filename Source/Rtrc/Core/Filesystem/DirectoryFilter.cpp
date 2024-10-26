#include <Rtrc/Core/Filesystem/DirectoryFilter.h>
#include <Rtrc/Core/String.h>
#include <Rtrc/Core/Unreachable.h>

RTRC_BEGIN

namespace DirectoryFilter
{

    struct SubNamePattern
    {
        enum Type
        {
            Empty,
            Star,
            Str1,
            Str1Star,
            StarStr1,
            Str1StarStr2,
            StarStr1Star
        };

        Type type = Empty;
        std::string str1;
        std::string str2;
    };

    struct NamePattern
    {
        SubNamePattern namePattern;
        SubNamePattern extensionPattern;
    };

    struct DirectoryPattern
    {
        std::string directory;
        bool recursive = false;
    };

    // returns end
    size_t ParseNameStr(std::string_view str, size_t begin)
    {
        while(begin < str.size() && str[begin] != '*')
        {
            ++begin;
        }
        return begin;
    }

    SubNamePattern ParseSubNamePattern(std::string_view str)
    {
        auto InvalidPattern = [str]
        {
            throw Exception(std::format("Invalid pattern: '{}'", str));
        };

        SubNamePattern ret;

        if(str.empty())
        {
            return ret;
        }
        
        if(str[0] == '*')
        {
            if(str.size() == 1)
            {
                ret.type = SubNamePattern::Star;
                return ret;
            }

            const size_t str1Beg = 1;
            const size_t str1End = ParseNameStr(str, str1Beg);
            if(str1End == str1Beg)
            {
                InvalidPattern();
            }

            if(str1End == str.size())
            {
                ret.type = SubNamePattern::StarStr1;
                ret.str1 = str.substr(str1Beg, str1End - str1Beg);
                return ret;
            }

            if(str1End + 1 == str.size() && str.back() == '*')
            {
                ret.type = SubNamePattern::StarStr1Star;
                ret.str1 = str.substr(str1Beg, str1End - str1Beg);
                return ret;
            }

            InvalidPattern();
        }

        const size_t str1Beg = 0;
        const size_t str1End = ParseNameStr(str, str1Beg);
        assert(str1End > str1Beg);
        ret.str1 = str.substr(str1Beg, str1End - str1Beg);

        if(str1End == str.size())
        {
            ret.type = SubNamePattern::Str1;
            return ret;
        }

        assert(str[str1End] == '*');
        if(str1End + 1 == str.size())
        {
            ret.type = SubNamePattern::Str1Star;
            return ret;
        }

        const size_t str2Beg = str1End + 1;
        const size_t str2End = ParseNameStr(str, str2Beg);
        if(str2End == str2Beg)
        {
            InvalidPattern();
        }
        ret.str2 = str.substr(str2Beg, str2End - str2Beg);

        if(str2End == str.size())
        {
            ret.type = SubNamePattern::Str1StarStr2;
            return ret;
        }

        InvalidPattern();
        Unreachable();
    }

    using Pos = std::string_view::size_type;
    constexpr Pos npos = std::string_view::npos;

    NamePattern ParseNamePattern(std::string_view str)
    {
        // Transform AAA/BBB/CCC/DDD -> DDD

        const Pos slash = str.rfind('/');
        if(slash != npos)
        {
            str = str.substr(slash + 1);
        }

        // Split name & extension

        std::string_view name, ext;
        if(auto dot = str.rfind('.'); dot != npos)
        {
            name = str.substr(0, dot);
            ext = str.substr(dot + 1);
        }
        else
        {
            name = str;
        }

        NamePattern ret;
        ret.namePattern = ParseSubNamePattern(name);
        ret.extensionPattern = ParseSubNamePattern(ext);
        return ret;
    }

    DirectoryPattern ParseDirectoryPattern(std::string_view str)
    {
        DirectoryPattern ret;

        const size_t slash = str.rfind('/');
        if(slash == npos)
        {
            return ret;
        }

        if(slash == 0) // Root...?
        {
            throw Exception(std::format("Invalid directory pattern: {}", str));
        }

        str = str.substr(0, slash);

        if(str.back() == '*')
        {
            ret.directory = str.substr(0, str.size() - 1);
            ret.recursive = true;
            return ret;
        }

        ret.directory = str;
        return ret;
    }

    bool Match(std::string_view str, const SubNamePattern &pattern)
    {
        switch(pattern.type)
        {
        case SubNamePattern::Empty:
            return str.empty();
        case SubNamePattern::Star:
            return true;
        case SubNamePattern::Str1:
            return str == pattern.str1;
        case SubNamePattern::Str1Star:
            return StartsWith(str, pattern.str1);
        case SubNamePattern::StarStr1:
            return EndsWith(str, pattern.str1);
        case SubNamePattern::Str1StarStr2:
            return str.size() >= pattern.str1.size() + pattern.str2.size() &&
                   StartsWith(str, pattern.str1) && EndsWith(str, pattern.str2);
        case SubNamePattern::StarStr1Star:
            return str.find(pattern.str1) != npos;
        }
        Unreachable();
    }

    bool Match(std::string_view name, std::string_view extension, const NamePattern &pattern)
    {
        return Match(name, pattern.namePattern) && Match(extension, pattern.extensionPattern);
    }

} // namespace DirectoryFilter

std::set<std::filesystem::path> DirectoryFilter::FilterFiles(Span<CommandVariant> commands)
{
    std::set<std::filesystem::path> ret;
    for(const CommandVariant &command : commands)
    {
        command.Match([&](auto &c) { DirectoryFilter::Apply(ret, c); });
    }
    return ret;
}

void DirectoryFilter::Apply(std::set<std::filesystem::path> &set, const IncludeCommand &command)
{
    const DirectoryPattern directoryPattern = ParseDirectoryPattern(command.range);
    const NamePattern namePattern = ParseNamePattern(command.range);

    auto ProcessFile = [&](const std::filesystem::path &path)
    {
        const std::string filename = path.filename().string();
        std::string_view name, ext;
        if(const size_t dotPos = filename.rfind('.'); dotPos != npos)
        {
            name = std::string_view(filename).substr(0, dotPos);
            ext = std::string_view(filename).substr(dotPos + 1);
        }
        else
        {
            name = filename;
        }
        if(Match(name, ext, namePattern))
        {
            set.insert(absolute(path).lexically_normal());
        }
    };

    if(directoryPattern.recursive)
    {
        for(auto &p : std::filesystem::recursive_directory_iterator(directoryPattern.directory))
        {
            if(p.is_regular_file())
            {
                ProcessFile(p.path());
            }
        }
    }
    else
    {
        for(auto &p : std::filesystem::directory_iterator(directoryPattern.directory))
        {
            if(p.is_regular_file())
            {
                ProcessFile(p.path());
            }
        }
    }
}

void DirectoryFilter::Apply(std::set<std::filesystem::path> &set, const ExcludeCommand &command)
{
    const DirectoryPattern directoryPattern = ParseDirectoryPattern(command.range);
    const NamePattern namePattern = ParseNamePattern(command.range);

    const std::filesystem::path directory = std::filesystem::absolute(directoryPattern.directory).lexically_normal();

    auto MatchPath = [&](const std::filesystem::path &path)
    {
        const std::filesystem::path parent = path.parent_path();
        if(directoryPattern.recursive)
        {
            for(auto it = directory.begin(), it2 = parent.begin(); it != directory.end(); ++it, ++it2)
            {
                if(it2 == parent.end())
                {
                    return false;
                }
                if(*it != *it2)
                {
                    return false;
                }
            }
        }
        else if(directory != parent)
        {
            return false;
        }

        const std::string filename = path.filename().string();
        std::string_view name, ext;
        if(const size_t dotPos = filename.rfind('.'); dotPos != npos)
        {
            name = std::string_view(filename).substr(0, dotPos);
            ext = std::string_view(filename).substr(dotPos + 1);
        }
        else
        {
            name = filename;
        }

        return Match(name, ext, namePattern);
    };

    for(auto it = set.begin(); it != set.end();)
    {
        if(MatchPath(*it))
        {
            it = set.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void DirectoryFilter::Apply(std::set<std::filesystem::path> &set, const EmptyCommand &command)
{
    // do nothing
}

RTRC_END
