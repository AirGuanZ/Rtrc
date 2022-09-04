#include <fmt/format.h>

#include <Rtrc/Material/ShaderManager.h>
#include <Rtrc/Utils/File.h>

RTRC_BEGIN

RC<Shader> ShaderTemplate::GetShader(const KeywordValues &keywordMap)
{
    const auto values = FilterValuesByKeywords(keywords_, keywordMap);
#if RTRC_DEBUG
    if(values.size() != keywords_.size())
    {
        for(const Keyword &keyword : keywords_)
        {
            bool found = false;
            for(auto &[first, _] : values)
            {
                if(first == keyword)
                {
                    found = true;
                    break;
                }
            }
            if(!found)
            {
                throw Exception(fmt::format(
                    "keyword '{}' not found in given keyword values", keyword.GetString()));
            }
        }
    }
#endif
    return compiledShaders_.GetOrCreate(values, [&]
    {
        ShaderCompiler::Macros macros;
        for(auto &[keyword, value] : values)
        {
            macros[keyword.GetString()] = value;
        }
        if(debug_)
        {
            macros["ENABLE_SHADER_DEBUG"] = "1";
        }
        return shaderCompiler_->Compile(source_, debug_, macros);
    });
}

void ShaderTemplate::GC()
{
    compiledShaders_.GC();
}

void ShaderManager::SetDevice(RHI::DevicePtr device)
{
    shaderCompiler_.SetDevice(std::move(device));
}

void ShaderManager::SetRootDirectory(std::string_view rootDir)
{
    shaderCompiler_.SetRootDirectory(rootDir);
}

RC<ShaderTemplate> ShaderManager::GetShaderTemplate(std::string_view name) const
{
    std::lock_guard lock(mutex_);
    auto it = nameToTemplate_.find(name);
    return it != nameToTemplate_.end() ? it->second : nullptr;
}

void ShaderManager::AddShaderTemplate(
    const std::string &name, const std::string &source, const std::string &filename, bool debug)
{
    std::string preprocessed = source;
    ShaderPreprocess::RemoveComments(preprocessed);

    std::string VSEntry;
    for(auto s : { "vertex", "vert" })
    {
        if(ShaderPreprocess::ParseEntryPragma(preprocessed, s, VSEntry))
        {
            break;
        }
    }

    std::string FSEntry;
    for(auto &s : { "fragment", "frag", "pixel" })
    {
        if(ShaderPreprocess::ParseEntryPragma(preprocessed, s, FSEntry))
        {
            break;
        }
    }

    std::string CSEntry;
    for(auto &s : { "compute", "comp" })
    {
        if(ShaderPreprocess::ParseEntryPragma(preprocessed, s, CSEntry))
        {
            break;
        }
    }

    std::vector<std::string> keywordStrings;
    ShaderPreprocess::ParseKeywords(preprocessed, keywordStrings);
    KeywordsBuilder keywordsBuilder;
    for(auto &s : keywordStrings)
    {
        keywordsBuilder(s);
    }

    auto shaderTemplate = MakeRC<ShaderTemplate>();
    shaderTemplate->debug_           = debug;
    shaderTemplate->keywords_        = keywordsBuilder.Build();
    shaderTemplate->source_.source   = preprocessed;
    shaderTemplate->source_.filename = filename;
    shaderTemplate->source_.VSEntry  = std::move(VSEntry);
    shaderTemplate->source_.FSEntry  = std::move(FSEntry);
    shaderTemplate->source_.CSEntry  = std::move(CSEntry);
    shaderTemplate->shaderCompiler_  = &shaderCompiler_;

    std::lock_guard lock(mutex_);
    nameToTemplate_.insert({ name, shaderTemplate });
}

void ShaderManager::AddShaderFile(const std::string &filename, bool debug)
{
    const std::string mappedFilename = shaderCompiler_.GetMappedFilename(filename);

    std::string source = File::ReadTextFile(mappedFilename);
    ShaderPreprocess::RemoveComments(source);

    size_t begPos = 0;
    while(true)
    {
        constexpr std::string_view SHADER = "Shader";
        const size_t shaderKeywordPos = source.find(SHADER, begPos);
        if(shaderKeywordPos == std::string::npos)
        {
            break;
        }

        // check for non-identifier char

        auto IsIdentifierChar = [](char c)
        {
            return std::isalnum(c) || c == '_';
        };

        if((shaderKeywordPos > 0 && IsIdentifierChar(source[shaderKeywordPos - 1])) ||
            IsIdentifierChar(source[shaderKeywordPos + SHADER.size()]))
        {
            begPos = shaderKeywordPos + SHADER.size();
            continue;
        }

        // skip whitespaces

        size_t nameBegPos = shaderKeywordPos + SHADER.size();
        while(std::isspace(source[nameBegPos]))
        {
            ++nameBegPos;
        }

        // get name

        if(source[nameBegPos] != '"' && source[nameBegPos != '\''])
        {
            throw Exception("shader name expected after 'Shader'");
        }
        const char delimiter = source[nameBegPos++];
        size_t nameEndPos = nameBegPos;

        while(nameEndPos < source.size() && source[nameEndPos] != '\n' && source[nameEndPos] != delimiter)
        {
            ++nameEndPos;
        }
        if(source[nameEndPos] != delimiter)
        {
            throw Exception("unclosed shader name string");
        }
        const std::string name = source.substr(nameBegPos, nameEndPos - nameBegPos);

        // skip whitespaces

        size_t leftBracePos = nameEndPos + 1;
        while(std::isspace(source[leftBracePos]))
        {
            ++leftBracePos;
        }

        // check '{'

        if(source[leftBracePos] != '{')
        {
            throw Exception("'{' expected");
        }

        // find enclosing '}'

        size_t nestedCount = 1, rightBracePos = leftBracePos;
        while(true)
        {
            ++rightBracePos;
            if(rightBracePos >= source.size())
            {
                throw Exception("enclosing '}' not found");
            }
            if(source[rightBracePos] == '{')
            {
                ++nestedCount;
            }
            else if(source[rightBracePos] == '}' && !--nestedCount)
            {
                break;
            }
        }
        assert(source[rightBracePos] == '}');

        // extract shader source

        std::string shaderSource = source;
        for(size_t i = 0; i <= leftBracePos; ++i)
        {
            if(shaderSource[i] != '\n')
            {
                shaderSource[i] = ' ';
            }
        }
        for(size_t i = rightBracePos; i < shaderSource.size(); ++i)
        {
            if(shaderSource[i] != '\n')
            {
                shaderSource[i] = ' ';
            }
        }
        AddShaderTemplate(name, shaderSource, filename, debug);

        begPos = rightBracePos;
    }
}

RTRC_END
