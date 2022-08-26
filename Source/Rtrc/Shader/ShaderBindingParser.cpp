#include <cassert>
#include <map>

#include <fmt/format.h>

#include <Rtrc/Shader/ShaderBindingParser.h>

RTRC_BEGIN

namespace
{
    
    class ShaderTokenStream
    {
    public:

        ShaderTokenStream(std::string &source, size_t startPos)
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

        void ReplaceCurrentSingleCharToken(char c)
        {
            assert(nextPos_ > 0);
            assert(currentToken_.size() == 1);
            currentToken_ = c;
            source_[nextPos_ - 1] = c;
        }

        size_t GetCurrentPosition() const
        {
            return nextPos_;
        }

        void RemoveCurrentToken()
        {
            assert(!currentToken_.empty());
            for(size_t i = nextPos_ - currentToken_.size(); i < nextPos_; ++i)
            {
                assert(!std::isspace(source_[i]));
                source_[i] = ' ';
            }
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

            if(ch == '{' || ch == '}' || ch == ',' || ch == ';' || ch == ':' ||
               ch == '[' || ch == ']' || ch == '<' || ch == '>' || ch == '|')
            {
                currentToken_ = source_.substr(nextPos_, 1);
                ++nextPos_;
                return;
            }

            // integer

            if(std::isdigit(ch))
            {
                size_t endPos = nextPos_ + 1;
                while(endPos < source_.size() && std::isdigit(source_[endPos]))
                {
                    ++endPos;
                }
                currentToken_ = source_.substr(nextPos_, endPos - nextPos_);
                nextPos_ = endPos;
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

        void RemoveAndNext()
        {
            RemoveCurrentToken();
            Next();
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
                    const std::string filename = source_.substr(filenameStart, filenameEnd - filenameStart);

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
        std::string &source_;
    };

    bool IsIdentifier(std::string_view str)
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

    bool IsNonIdentifierChar(char c)
    {
        return !std::isalnum(c) && c != '_';
    }

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
            if((pos == 0 || IsNonIdentifierChar(source[pos - 1])) && IsNonIdentifierChar(source[pos + 5]))
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

    if(!IsIdentifier(tokens.GetCurrentToken()))
    {
        tokens.ThrowWithLineAndFilename("group name expected");
    }
    bindings.name = tokens.GetCurrentToken();
    tokens.RemoveAndNext();

    // {

    if(tokens.GetCurrentToken() != "{")
    {
        tokens.ThrowWithLineAndFilename("'{' expected");
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
                tokens.ThrowWithLineAndFilename(fmt::format("unknown shader stage: {}", curr));
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
                tokens.ThrowWithLineAndFilename("unexpected token: '}'");
            }
            tokens.RemoveAndNext();
            break;
        }

        if(tokens.GetCurrentToken() == "]")
        {
            if(!parsingAliasedBindings)
            {
                tokens.ThrowWithLineAndFilename("unexpected token: ']'");
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
                tokens.ThrowWithLineAndFilename("aliased bindings cannot be nested");
            }
            tokens.RemoveAndNext();
            assert(aliasedNames.empty());
            parsingAliasedBindings = true;
            continue;
        }

        if(IsIdentifier(tokens.GetCurrentToken()))
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
                    tokens.ThrowWithLineAndFilename("',' or ';' expected");
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
                        tokens.ThrowWithLineAndFilename("'<' expected");
                    }
                    tokens.Next();

                    if(!IsIdentifier(tokens.GetCurrentToken()))
                    {
                        tokens.ThrowWithLineAndFilename("type parameter expected");
                    }
                    tokens.Next();

                    if(tokens.GetCurrentToken() != ">")
                    {
                        tokens.ThrowWithLineAndFilename("'>' expected");
                    }
                    tokens.Next();
                }

                if(!IsIdentifier(tokens.GetCurrentToken()))
                {
                    tokens.ThrowWithLineAndFilename("binding name expected");
                }
                std::string name(tokens.GetCurrentToken());
                tokens.Next();

                if(tokens.GetCurrentToken() == "[")
                {
                    tokens.Next();
                    tokens.Next(); // skip array size
                    if(tokens.GetCurrentToken() != "]")
                    {
                        tokens.ThrowWithLineAndFilename("']' expected");
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
                    tokens.ThrowWithLineAndFilename("';' expected after resource definition");
                }
                tokens.Next();
            }
            continue;
        }

        tokens.ThrowWithLineAndFilename(fmt::format("unexpected token: {}", tokens.GetCurrentToken()));
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
                if((candidatePos == 0 || IsNonIdentifierChar(source[candidatePos - 1])) &&
                    IsNonIdentifierChar(source[candidatePos + candidateLength]))
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
                tokens.ThrowWithLineAndFilename("'<' expected");
            }
            tokens.Next();

            if(!IsIdentifier(tokens.GetCurrentToken()))
            {
                tokens.ThrowWithLineAndFilename("type parameter expected");
            }
            tokens.Next();

            if(tokens.GetCurrentToken() != ">")
            {
                tokens.ThrowWithLineAndFilename("'>' expected");
            }
            tokens.Next();
        }

        // name

        if(!IsIdentifier(tokens.GetCurrentToken()))
        {
            tokens.ThrowWithLineAndFilename("binding name expected");
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
                tokens.ThrowWithLineAndFilename("invalid array size");
            }
            tokens.Next();
            if(tokens.GetCurrentToken() != "]")
            {
                tokens.ThrowWithLineAndFilename("']' expected");
            }
            tokens.Next();
        }

        // eventually...

        const size_t endPos = tokens.GetCurrentPosition();
        if(tokens.GetCurrentToken() != ";")
        {
            tokens.ThrowWithLineAndFilename("';' expected");
        }
        tokens.Next();

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
