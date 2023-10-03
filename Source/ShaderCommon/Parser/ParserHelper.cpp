#include <Core/Parser/ShaderTokenStream.h>
#include <ShaderCommon/Parser/ParserHelper.h>

RTRC_BEGIN

namespace ParserHelperDetail
{

    template<bool Comments, bool StringContent>
    void RemoveCommentsAndStringsImpl(std::string &source)
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
                // Find '\n'
                size_t endPos = pos;
                while(endPos < source.size() && source[endPos] != '\n')
                {
                    if constexpr(Comments)
                    {
                        source[endPos] = ' ';
                    }
                    ++endPos;
                }
                pos = endPos + 1;
            }
            else if(source[pos] == '/' && source[pos + 1] == '*')
            {
                // Find "*/"
                const size_t newPos = source.find("*/", pos);
                if(newPos == std::string::npos)
                {
                    throw Exception("\"*/\" expected");
                }
                for(size_t i = pos; i < newPos + 2; ++i)
                {
                    if constexpr(Comments)
                    {
                        if(!std::isspace(source[i]))
                        {
                            source[i] = ' ';
                        }
                    }
                }
                pos = newPos + 2;
            }
            else if(source[pos] == '"')
            {
                // Find another '"'
                size_t endPos = pos + 1;
                while(true)
                {
                    if(endPos >= source.size())
                    {
                        throw Exception("Enclosing '\"' expected");
                    }
                    if(source[endPos] == '\\')
                    {
                        if constexpr(StringContent)
                        {
                            source[endPos + 0] = ' ';
                            source[endPos + 1] = ' ';
                        }
                        endPos += 2;
                    }
                    else if(source[endPos] == '"')
                    {
                        break;
                    }
                    else
                    {
                        if constexpr(StringContent)
                        {
                            source[endPos] = ' ';
                        }
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

} // namespace ParserHelperDetail

size_t FindKeyword(std::string_view source, std::string_view keyword, size_t beginPos)
{
    while(true)
    {
        const size_t p = source.find(keyword, beginPos);
        if(p == std::string::npos)
        {
            return std::string::npos;
        }
        const bool isPrevCharOk = p == 0 || ShaderTokenStream::IsNonIdentifierChar(source[p - 1]);
        const bool isNextCharOk = ShaderTokenStream::IsNonIdentifierChar(source[p + keyword.size()]);
        if(isPrevCharOk && isNextCharOk)
        {
            return p;
        }
        beginPos = p + keyword.size();
    }
}

size_t FindFirstKeyword(
    std::string_view       source,
    Span<std::string_view> keywords,
    size_t                 beginPos,
    std::string_view      &outKeyword)
{
    size_t ret = std::string::npos;
    for(auto keyword : keywords)
    {
        const size_t p = FindKeyword(source, keyword, beginPos);
        if(p < ret)
        {
            outKeyword = keyword;
            ret = p;
        }
    }
    return ret;
}

size_t FindMatchedRightBracket(std::string_view source, size_t begin)
{
    assert(source[begin] == '{');
    int depth = 1;
    size_t result = begin + 1;
    while(true)
    {
        if(result >= source.size())
        {
            return std::string::npos;
        }
        if(source[result] == '{')
        {
            ++depth;
            ++result;
        }
        else if(source[result] == '}')
        {
            if(!--depth)
            {
                return result;
            }
            ++result;
        }
        else
        {
            ++result;
        }
    }
}

void RemoveComments(std::string &source)
{
    return ParserHelperDetail::RemoveCommentsAndStringsImpl<true, false>(source);
}

void RemoveCommentsAndStrings(std::string &source)
{
    return ParserHelperDetail::RemoveCommentsAndStringsImpl<true, true>(source);
}

void RemoveStrings(std::string &source)
{
    return ParserHelperDetail::RemoveCommentsAndStringsImpl<false, true>(source);
}

RTRC_END
