#include <cassert>

#include <fmt/format.h>

#include <Rtrc/Shader/ShaderBindingParser.h>

RTRC_BEGIN

namespace
{

    class ShaderTokenStream
    {
    public:

        ShaderTokenStream(std::string_view source, size_t startPos)
            : nextPos_(startPos), source_(source)
        {
            Next();
        }

        bool IsFinished() const
        {
            return currentToken_.empty() && nextPos_ >= source_.size();
        }

        std::string_view GetCurrentToken() const
        {
            return currentToken_;
        }

        size_t GetCurrentPosition() const
        {
            return nextPos_;
        }

        void Next()
        {
            // skip empty chars

            while(nextPos_ < source_.size() && std::isspace(source_[nextPos_]))
            {
                ++nextPos_;
            }

            // early exit

            if(nextPos_ >= source_.size())
            {
                currentToken_ = {};
                return;
            }

            // special tokens

            const char ch = source_[nextPos_];

            if(ch == '{' || ch == '}' || ch == ',' || ch == ';' || ch == '[' || ch == ']')
            {
                currentToken_ = source_.substr(nextPos_, 1);
                ++nextPos_;
                return;
            }

            // identifier

            if(!std::isalpha(ch) && ch != '_')
            {
                ThrowWithLineAndFilename(fmt::format("unknown token starting with '{}'", ch));
            }

            size_t endPos = nextPos_ + 1;
            while(endPos < source_.size() && (std::isalnum(source_[endPos]) || source_[endPos] == '_'))
            {
                ++endPos;
            }

            currentToken_ = source_.substr(nextPos_, endPos - nextPos_);
            nextPos_ = endPos;
        }

        [[noreturn]] void ThrowWithLineAndFilename(std::string_view msg) const
        {
            assert(nextPos_ > 0);
            int line = 0;
            for(int i = static_cast<int>(nextPos_ - 1); i >= 0; --i)
            {
                if(source_[i] != '\n' && i > 0)
                {
                    continue;
                }

                if(i == 0 && source_[i] == '#')
                {
                    --i;
                }

                // #line LINE "Filename"
                if(source_[i + 1] == '#')
                {
                    const size_t lineStart = i + 7;
                    assert(lineStart < source_.size());
                    size_t lineEnd = lineStart + 1;
                    while(lineEnd < source_.size() && std::isdigit(source_[lineEnd]))
                    {
                        ++lineEnd;
                    }
                    const int initialLine = std::stoi(std::string(source_.substr(lineStart, lineEnd - lineStart)));
                    line += initialLine - 1;

                    assert(source_[lineEnd + 1] == '\"');
                    const size_t filenameStart = lineEnd + 2;
                    assert(filenameStart < source_.size());
                    size_t filenameEnd = filenameStart + 2;
                    while(filenameEnd < source_.size() && source_[filenameEnd] != '\"')
                    {
                        ++filenameEnd;
                    }
                    const std::string_view filename = source_.substr(filenameStart, filenameEnd - filenameStart);

                    throw Exception(fmt::format("parsing error at {}, line {}. {}", filename, line, msg));
                }

                ++line;
            }
            throw Exception(fmt::format(
                "invalid shader source file: '#line FILENAME' not found before the {}th character", nextPos_ + 1));
        }

    private:

        size_t nextPos_;
        std::string currentToken_;
        std::string_view source_;
    };

} // namespace anonymous

ShaderBindingParser::ShaderBindingParser(std::string source)
    : pos_(0), source_(std::move(source))
{

}

bool ShaderBindingParser::ProcessNextBindingGroup(ParsedBindingGroup &bindings)
{
    const size_t keywordPos = FindNextBindingGroupKeyword();
    if(keywordPos == std::string::npos)
    {
        return false;
    }

    ShaderTokenStream tokenStream(source_, keywordPos);
    assert(!tokenStream.IsFinished() && tokenStream.GetCurrentToken() == "group");

    // extract group name

    tokenStream.Next();
    if(tokenStream.IsFinished())
    {
        tokenStream.ThrowWithLineAndFilename("group name expected");
    }
    bindings.name = tokenStream.GetCurrentToken();

    tokenStream.Next();
    if(tokenStream.GetCurrentToken() != "{")
    {
        tokenStream.ThrowWithLineAndFilename("'{' expected");
    }
    tokenStream.Next();

    // extract binding names

    bindings.bindings.clear();

    auto isIdentifier = [&](std::string_view str)
    {
        if(str.empty())
        {
            return false;
        }
        if(!std::isalpha(str[0]) && str[0] != '_')
        {
            return false;
        }
        for(char c : str)
        {
            if(!std::isalnum(c) && c != '_')
            {
                return false;
            }
        }
        return true;
    };

    for(;;)
    {
        if(tokenStream.IsFinished())
        {
            tokenStream.ThrowWithLineAndFilename("binding name or '}' is expected");
        }

        if(tokenStream.GetCurrentToken() == "}")
        {
            break;
        }
        if(isIdentifier(tokenStream.GetCurrentToken()))
        {
            bindings.bindings.push_back(AliasedBindingNames{ std::string(tokenStream.GetCurrentToken()) });
            tokenStream.Next();
        }
        else if(tokenStream.GetCurrentToken() == "[")
        {
            tokenStream.Next();
            bindings.bindings.emplace_back();
            auto &aliasedNames = bindings.bindings.back();
            for(;;)
            {
                if(!isIdentifier(tokenStream.GetCurrentToken()))
                {
                    tokenStream.ThrowWithLineAndFilename("binding name expected");
                }
                aliasedNames.push_back(std::string(tokenStream.GetCurrentToken()));
                tokenStream.Next();
                if(tokenStream.GetCurrentToken() == ",")
                {
                    tokenStream.Next();
                    if(tokenStream.GetCurrentToken() == "]")
                    {
                        tokenStream.Next();
                        break;
                    }
                    continue;
                }
                if(tokenStream.GetCurrentToken() == "]")
                {
                    tokenStream.Next();
                    break;
                }
                tokenStream.ThrowWithLineAndFilename("',' or ']' expected");
            }
        }
        else
        {
            tokenStream.ThrowWithLineAndFilename("(aliased) binding name(s) expected");
        }

        if(tokenStream.GetCurrentToken() == ",")
        {
            tokenStream.Next();
        }
        else if(tokenStream.GetCurrentToken() != "}")
        {
            tokenStream.ThrowWithLineAndFilename("'}' or ',' expected");
        }
    }

    for(size_t i = keywordPos; i < tokenStream.GetCurrentPosition(); ++i)
    {
        char &c = source_[i];
        if(c != '\n')
            c = ' ';
    }

    return true;
}

const std::string &ShaderBindingParser::GetFinalSource() const
{
    return source_;
}

size_t ShaderBindingParser::FindNextBindingGroupKeyword()
{
    auto isOtherChar = [](char c)
    {
        return !std::isalnum(c) && c != '_';
    };

    for(;;)
    {
        const size_t p = source_.find("group", pos_);
        if(p == std::string::npos)
        {
            return std::string::npos;
        }
        if((p == 0 || isOtherChar(source_[p - 1])) && (p == source_.size() - 1 || isOtherChar(source_[p + 5])))
        {
            return p;
        }
        pos_ = p + 6;
    }
}

RTRC_END
