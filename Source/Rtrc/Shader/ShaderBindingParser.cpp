#include <cassert>
#include <map>

#include <fmt/format.h>

#include <Rtrc/Shader/ShaderBindingParser.h>
#include <Rtrc/Shader/ShaderTokenStream.h>

RTRC_BEGIN

namespace
{

    size_t FindFirstBindingGroupKeyword(const std::string &source)
    {
        size_t initPos = 0;
        for(;;)
        {
            const size_t pos = source.find("group", initPos);
            if(pos == std::string::npos)
            {
                return std::string::npos;
            }
            if((pos == 0 || ShaderTokenStream::IsNonIdentifierChar(source[pos - 1])) &&
                ShaderTokenStream::IsNonIdentifierChar(source[pos + 5]))
            {
                return pos;
            }
            initPos = pos + 6;
        }
    }

    const std::map<std::string, RHI::BindingType, std::less<>> NAME_TO_BINDING_TYPE =
    {
        { "Texture2D",          RHI::BindingType::Texture2D },
        { "RWTexture2D",        RHI::BindingType::RWTexture2D },
        { "Buffer",             RHI::BindingType::Buffer },
        { "RWBuffer",           RHI::BindingType::RWBuffer },
        { "StructuredBuffer",   RHI::BindingType::StructuredBuffer },
        { "RWStructuredBuffer", RHI::BindingType::RWStructuredBuffer },
        { "ConstantBuffer",     RHI::BindingType::ConstantBuffer },
        { "SamplerState",       RHI::BindingType::Sampler }
    };

} // namespace anonymous

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

bool ShaderBindingGroupRewrite::RewriteNextBindingGroup(std::string &source, ParsedBindingGroup &bindings)
{
    const size_t groupKeywordPos = FindFirstBindingGroupKeyword(source);
    if(groupKeywordPos == std::string::npos)
    {
        return false;
    }
    ShaderTokenStream tokens(source, groupKeywordPos);
    assert(!tokens.IsFinished() && tokens.GetCurrentToken() == "group");
    tokens.RemoveAndNext();

    // binding name

    if(!ShaderTokenStream::IsIdentifier(tokens.GetCurrentToken()))
    {
        tokens.Throw("group name expected");
    }
    bindings.name = tokens.GetCurrentToken();
    tokens.RemoveAndNext();

    // {

    if(tokens.GetCurrentToken() != "{")
    {
        tokens.Throw("'{' expected");
    }
    tokens.RemoveAndNext();

    // bindings

    auto ParseStages = [&]
    {
        if(tokens.GetCurrentToken() != ":")
        {
            return RHI::ShaderStage::VertexShader
                 | RHI::ShaderStage::FragmentShader
                 | RHI::ShaderStage::ComputeShader;
        }
        tokens.RemoveAndNext();

        RHI::ShaderStageFlag result = 0;
        while(true)
        {
            std::string_view curr = tokens.GetCurrentToken();
            if(curr == "VS")
            {
                result |= RHI::ShaderStage::VertexShader;
            }
            else if(curr == "FS" || curr == "PS")
            {
                result |= RHI::ShaderStage::FragmentShader;
            }
            else if(curr == "CS")
            {
                result |= RHI::ShaderStage::ComputeShader;
            }
            else
            {
                tokens.Throw(fmt::format("unknown shader stage: {}", curr));
            }
            tokens.RemoveAndNext();

            if(tokens.GetCurrentToken() == "|")
            {
                tokens.RemoveAndNext();
            }
            else
            {
                return result;
            }
        }
    };

    bool parsingAliasedBindings = false;

    std::vector<Binding> aliasedNames;

    assert(bindings.bindings.empty());
    while(true)
    {
        if(tokens.GetCurrentToken() == "}")
        {
            if(parsingAliasedBindings)
            {
                tokens.Throw("unexpected token: '}'");
            }
            tokens.RemoveCurrentToken();
            break;
        }

        if(tokens.GetCurrentToken() == "]")
        {
            if(!parsingAliasedBindings)
            {
                tokens.Throw("unexpected token: ']'");
            }
            tokens.RemoveAndNext();
            parsingAliasedBindings = false;
            bindings.bindings.push_back(std::move(aliasedNames));
            aliasedNames.clear();
            continue;
        }

        if(tokens.GetCurrentToken() == "[")
        {
            if(parsingAliasedBindings)
            {
                tokens.Throw("aliased bindings cannot be nested");
            }
            tokens.RemoveAndNext();
            assert(aliasedNames.empty());
            parsingAliasedBindings = true;
            continue;
        }

        if(ShaderTokenStream::IsIdentifier(tokens.GetCurrentToken()))
        {
            std::string_view id = tokens.GetCurrentToken();
            if(auto it = NAME_TO_BINDING_TYPE.find(id); it == NAME_TO_BINDING_TYPE.end())
            {
                std::string name(tokens.GetCurrentToken());
                tokens.RemoveAndNext();

                const RHI::ShaderStageFlag stages = ParseStages();
                if(parsingAliasedBindings)
                {
                    aliasedNames.push_back({ std::move(name), stages });
                }
                else
                {
                    bindings.bindings.push_back({ { std::move(name), stages } });
                }

                if(tokens.GetCurrentToken() == "," || tokens.GetCurrentToken() == ";")
                {
                    tokens.RemoveAndNext();
                }
                else if(tokens.GetCurrentToken() != "}" && tokens.GetCurrentToken() != "]")
                {
                    tokens.Throw("',' or ';' expected");
                }
            }
            else
            {
                tokens.Next();
                const RHI::BindingType bindingType = it->second;

                const bool hasTypeParam = bindingType != RHI::BindingType::Sampler;
                if(hasTypeParam)
                {
                    if(tokens.GetCurrentToken() != "<")
                    {
                        tokens.Throw("'<' expected");
                    }
                    tokens.Next();

                    if(!ShaderTokenStream::IsIdentifier(tokens.GetCurrentToken()))
                    {
                        tokens.Throw("type parameter expected");
                    }
                    tokens.Next();

                    if(tokens.GetCurrentToken() != ">")
                    {
                        tokens.Throw("'>' expected");
                    }
                    tokens.Next();
                }

                if(!ShaderTokenStream::IsIdentifier(tokens.GetCurrentToken()))
                {
                    tokens.Throw("binding name expected");
                }
                std::string name(tokens.GetCurrentToken());
                tokens.Next();

                if(tokens.GetCurrentToken() == "[")
                {
                    tokens.Next();
                    tokens.Next(); // skip array size
                    if(tokens.GetCurrentToken() != "]")
                    {
                        tokens.Throw("']' expected");
                    }
                    tokens.Next();
                }

                const RHI::ShaderStageFlag stages = ParseStages();
                if(parsingAliasedBindings)
                {
                    aliasedNames.push_back({ std::move(name), stages });
                }
                else
                {
                    bindings.bindings.push_back({ { std::move(name), stages } });
                }

                if(tokens.GetCurrentToken() == ",")
                {
                    tokens.ReplaceCurrentSingleCharToken(';');
                }
                else if(tokens.GetCurrentToken() != ";" &&
                        tokens.GetCurrentToken() != "}" &&
                        tokens.GetCurrentToken() != "]")
                {
                    tokens.Throw("';' expected after resource definition");
                }
                tokens.Next();
            }
            continue;
        }

        tokens.Throw(fmt::format("unexpected token: {}", tokens.GetCurrentToken()));
    }

    return true;
}

void ShaderBindingParse::ParseBindings(std::string &source, std::vector<ParsedBinding> &bindings)
{
    assert(bindings.empty());
    while(true)
    {
        const size_t searchBegPos = bindings.empty() ? 0 : bindings.back().endPosInSource;

        // find keyword

        size_t keywordBegPos; RHI::BindingType bindingType;
        {
            size_t initPos = searchBegPos;
            while(true)
            {
                size_t candidatePos = std::string::npos, candidateLength = 0;
                RHI::BindingType candidateType = RHI::BindingType::Sampler;
                for(auto &[name, type] : NAME_TO_BINDING_TYPE)
                {
                    const size_t pos = source.find(name, initPos);
                    if(pos < candidatePos)
                    {
                        candidatePos = pos;
                        candidateType = type;
                        candidateLength = name.size();
                    }
                }
                if(candidatePos == std::string::npos)
                {
                    return;
                }
                if((candidatePos == 0 || ShaderTokenStream::IsNonIdentifierChar(source[candidatePos - 1])) &&
                    ShaderTokenStream::IsNonIdentifierChar(source[candidatePos + candidateLength]))
                {
                    keywordBegPos = candidatePos;
                    bindingType = candidateType;
                    break;
                }
                initPos = candidatePos + candidateLength;
            }
        }

        // token stream

        ShaderTokenStream tokens(source, keywordBegPos);
        tokens.Next();

        // skip type parameter

        const bool hasTypeParam = bindingType != RHI::BindingType::Sampler;
        if(hasTypeParam)
        {
            if(tokens.GetCurrentToken() != "<")
            {
                tokens.Throw("'<' expected");
            }
            tokens.Next();

            if(!ShaderTokenStream::IsIdentifier(tokens.GetCurrentToken()))
            {
                tokens.Throw("type parameter expected");
            }
            tokens.Next();

            if(tokens.GetCurrentToken() != ">")
            {
                tokens.Throw("'>' expected");
            }
            tokens.Next();
        }

        // name

        if(!ShaderTokenStream::IsIdentifier(tokens.GetCurrentToken()))
        {
            tokens.Throw("binding name expected");
        }
        std::string name(tokens.GetCurrentToken());
        tokens.Next();

        // array size

        std::optional<uint32_t> arraySize;
        if(tokens.GetCurrentToken() == "[")
        {
            tokens.Next();
            try
            {
                arraySize = std::stoul(std::string(tokens.GetCurrentToken()));
            }
            catch(...)
            {
                tokens.Throw("invalid array size");
            }
            tokens.Next();
            if(tokens.GetCurrentToken() != "]")
            {
                tokens.Throw("']' expected");
            }
            tokens.Next();
        }

        // eventually...

        const size_t endPos = tokens.GetCurrentPosition();
        if(tokens.GetCurrentToken() != ";")
        {
            tokens.Throw("';' expected");
        }

        bindings.push_back(ParsedBinding
        {
            .begPosInSource = keywordBegPos,
            .endPosInSource = endPos,
            .type           = bindingType,
            .name           = std::move(name),
            .arraySize      = arraySize
        });
    }
}

RTRC_END
