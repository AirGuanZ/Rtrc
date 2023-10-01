#pragma once

#include <cassert>

#include <Core/Common.h>

RTRC_BEGIN

class ShaderTokenStream
{
public:

    enum class ErrorMode
    {
        PreprocessedShader,
        Material
    };

    static bool IsIdentifier(std::string_view str)
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
    }

    static bool IsNonEmptyStringLiterial(std::string_view s)
    {
        return s.size() >= 3 && s[0] == '"' && s[s.size() - 1] == '"';
    }

    static bool IsNonIdentifierChar(char c)
    {
        return !std::isalnum(c) && c != '_';
    }

    ShaderTokenStream(std::string_view source, size_t startPos, ErrorMode errMode = ErrorMode::PreprocessedShader)
        : nextPos_(startPos), errMode_(errMode), source_(source)
    {
        Next();
    }

    void SetStartPosition(size_t pos)
    {
        assert(pos <= source_.size());
        nextPos_ = pos;
        Next();
    }

    bool IsFinished() const
    {
        return currentToken_.empty() && nextPos_ >= source_.size();
    }

    const std::string &GetCurrentToken() const
    {
        return currentToken_;
    }

    void ConsumeOrThrow(std::string_view token, std::string_view msg = "")
    {
        if(GetCurrentToken() == token)
        {
            Next();
        }
        else if(!msg.size())
        {
            Throw(fmt::format("'{}' expected", token));
        }
        else
        {
            Throw(msg);
        }
    }

    std::string ConsumeIdentifierOrThrow(std::string_view msg = "")
    {
        if(IsIdentifier(GetCurrentToken()))
        {
            auto ret = GetCurrentToken();
            Next();
            return ret;
        }
        if(msg.empty())
        {
            Throw("Identifier expected");
        }
        Throw(msg);
    }

    size_t GetCurrentPosition() const
    {
        return nextPos_;
    }

    void Next()
    {
        // Skip empty chars

        while(nextPos_ < source_.size() && std::isspace(source_[nextPos_]))
        {
            ++nextPos_;
        }

        // Skip '#line ...'

        if(nextPos_ < source_.size() && source_.substr(nextPos_).starts_with("#line"))
        {
            while(nextPos_ < source_.size() && source_[nextPos_] != '\n')
            {
                ++nextPos_;
            }
            Next();
            return;
        }

        // Early exit

        if(nextPos_ >= source_.size())
        {
            currentToken_ = {};
            return;
        }

        // Special tokens

        const char ch = source_[nextPos_];

        if(ch == ':' && source_[nextPos_ + 1] == ':') // ::
        {
            currentToken_ = "::";
            nextPos_ += 2;
            return;
        }

        if(ch == '{' || ch == '}' || ch == ',' || ch == ';' || ch == ':' ||
           ch == '[' || ch == ']' || ch == '<' || ch == '>' || ch == '|' ||
           ch == '(' || ch == ')' || ch == '=' || ch == '#')
        {
            currentToken_ = source_.substr(nextPos_, 1);
            ++nextPos_;
            return;
        }

        // Number

        if(std::isdigit(ch))
        {
            const size_t endPos = FindEndOfNumber(nextPos_);
            currentToken_ = source_.substr(nextPos_, endPos - nextPos_);
            nextPos_ = endPos;
            return;
        }

        if(ch == '"' || ch == '\'')
        {
            size_t strEndPos = nextPos_ + 1;
            while(strEndPos < source_.size() && source_[strEndPos] != '\n' && source_[strEndPos] != ch)
            {
                ++strEndPos;
            }
            if(source_[strEndPos] != ch)
            {
                Throw("Unclosed string");
            }
            currentToken_ = source_.substr(nextPos_, strEndPos + 1 - nextPos_);
            nextPos_ = strEndPos + 1;
            return;
        }

        // Identifier

        if(!std::isalpha(ch) && ch != '_')
        {
            Throw(fmt::format("Unknown token starting with '{}'", ch));
        }

        size_t endPos = nextPos_ + 1;
        while(endPos < source_.size() && (std::isalnum(source_[endPos]) || source_[endPos] == '_'))
        {
            ++endPos;
        }
        currentToken_ = source_.substr(nextPos_, endPos - nextPos_);
        nextPos_ = endPos;
    }

    [[noreturn]] void Throw(std::string_view msg) const
    {
        if(errMode_ == ErrorMode::PreprocessedShader)
        {
            ThrowInPreprocessedShaderMode(msg);
        }
        ThrowInMaterialMode(msg);
    }

private:

    [[noreturn]] void ThrowInPreprocessedShaderMode(std::string_view msg) const
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

                throw Exception(fmt::format("Parsing error at {}, line {}. {}", filename, line, msg));
            }

            ++line;
        }
        throw Exception(fmt::format(
            "Invalid shader source file: '#line FILENAME' not found before the {}th character", nextPos_ + 1));
    }

    [[noreturn]] void ThrowInMaterialMode(std::string_view msg) const
    {
        assert(nextPos_ > 0);
        int line = 1;
        for(int i = static_cast<int>(nextPos_ - 1); i >= 0; --i)
        {
            if(source_[i] == '\n')
            {
                ++line;
            }
        }
        throw Exception(fmt::format("Parsing error at line {}. {}", line, msg));
    }

    size_t FindEndOfNumber(size_t start) const
    {
        assert(std::isdigit(source_[start]) || source_[start] == '.');
        bool meetDot = false;
        if(source_[start] == '.')
        {
            meetDot = true;
            ++start;
        }
        if(!meetDot)
        {
            while(std::isdigit(source_[start]))
            {
                ++start;
            }
            if(source_[start] != '.')
            {
                return start;
            }
            ++start;
        }
        while(std::isdigit(source_[start]))
        {
            ++start;
        }
        return start;
    }

    size_t nextPos_;
    ErrorMode errMode_;
    std::string currentToken_;
    std::string_view source_;
};

RTRC_END
