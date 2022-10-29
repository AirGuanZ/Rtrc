#include <cassert>
#include <map>

#include <Rtrc/Graphics/Shader/ShaderBindingParser.h>
#include <Rtrc/Graphics/Shader/ShaderTokenStream.h>

RTRC_BEGIN

bool ShaderPreprocess::ParseEntryPragma(std::string &source, const std::string &entryName, std::string &result)
{
    assert(ShaderTokenStream::IsIdentifier(entryName));
    size_t beginPos = 0;
    while(true)
    {
        const size_t pos = source.find("#" + entryName, beginPos);
        if(pos == std::string::npos)
        {
            return false;
        }

        const size_t findLen = 1 + entryName.size();
        if(!ShaderTokenStream::IsNonIdentifierChar(source[pos + findLen]))
        {
            beginPos = pos + findLen;
            continue;
        }

        std::string s = source;
        ShaderTokenStream tokens(s, pos + findLen);
        if(ShaderTokenStream::IsIdentifier(tokens.GetCurrentToken()))
        {
            result = tokens.GetCurrentToken();
            for(size_t i = pos; i < tokens.GetCurrentPosition(); ++i)
            {
                if(source[i] != '\n')
                {
                    source[i] = ' ';
                }
            }
            return true;
        }
        tokens.Throw(fmt::format("expect identifier after '#{}'", entryName));
    }
}

void ShaderPreprocess::ParseKeywords(std::string &source, std::vector<std::string> &keywords)
{
    assert(keywords.empty());
    size_t beginPos = 0;
    while(true)
    {
        constexpr std::string_view KEYWORD = "#keyword";
        const size_t pos = source.find(KEYWORD, beginPos);
        if(pos == std::string::npos)
        {
            return;
        }

        if(!ShaderTokenStream::IsNonIdentifierChar(source[pos + KEYWORD.size()]))
        {
            beginPos = pos + KEYWORD.size();
            continue;
        }

        std::string s = source;
        ShaderTokenStream tokens(s, pos + KEYWORD.size());
        if(ShaderTokenStream::IsIdentifier(tokens.GetCurrentToken()))
        {
            keywords.push_back(std::string(tokens.GetCurrentToken()));
            for(size_t i = 0; i < tokens.GetCurrentPosition(); ++i)
            {
                if(source[i] != '\n')
                {
                    source[i] = ' ';
                }
            }
            beginPos = tokens.GetCurrentPosition();
        }
        else
        {
            tokens.Throw("identifier expected after '#keyword'");
        }
    }
}

void ShaderPreprocess::RemoveComments(std::string &source)
{
    for(char &c : source)
    {
        if(c == '\r')
        {
            c = ' ';
        }
    }

    size_t pos = 0;
    while(pos < source.size())
    {
        if(source[pos] == '/' && source[pos + 1] == '/')
        {
            // find '\n'
            size_t endPos = pos;
            while(endPos < source.size() && source[endPos] != '\n')
            {
                source[endPos] = ' ';
                ++endPos;
            }
            pos = endPos + 1;
        }
        else if(source[pos] == '/' && source[pos + 1] == '*')
        {
            // find "*/"
            const size_t newPos = source.find("*/", pos);
            if(newPos == std::string::npos)
            {
                throw Exception("\"*/\" expected");
            }
            for(size_t i = pos; i < newPos + 2; ++i)
            {
                if(!std::isspace(source[i]))
                {
                    source[i] = ' ';
                }
            }
            pos = newPos + 2;
        }
        else if(source[pos] == '"')
        {
            // skip string

            size_t endPos = pos + 1;
            while(true)
            {
                if(endPos >= source.size())
                {
                    throw Exception("enclosing '\"' expected");
                }
                if(source[endPos] == '\\')
                {
                    endPos += 2;
                }
                else if(source[endPos] == '"')
                {
                    break;
                }
                else
                {
                    ++endPos;
                }
            }
            pos = endPos + 1;
        }
        else
        {
            ++pos;
        }
    }
}

RTRC_END
