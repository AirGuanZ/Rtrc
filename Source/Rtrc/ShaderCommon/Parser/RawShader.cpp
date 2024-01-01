#include <Rtrc/Core/Filesystem/File.h>
#include <Rtrc/Core/Parser/ShaderTokenStream.h>
#include <Rtrc/Core/String.h>
#include <Rtrc/ShaderCommon/Parser/RawShader.h>
#include <Rtrc/ShaderCommon/Parser/ParserHelper.h>

RTRC_BEGIN

namespace ShaderDatabaseDetail
{
    
    // rtrc_shader("ShaderName")
    // ...
    // rtrc_shader("AnotherShaderName")
    // ...
    void AddShaderFile(RawShaderDatabase &database, const std::string &filename)
    {
        std::string source = File::ReadTextFile(filename);
        RemoveComments(source);
        
        std::string sourceForFindingKeyword = source;
        RemoveCommentsAndStrings(sourceForFindingKeyword);

        size_t keywordBeginPos = 0;
        while(true)
        {
            constexpr std::string_view KEYWORD = "rtrc_shader";
            const size_t p = FindKeyword(sourceForFindingKeyword, KEYWORD, keywordBeginPos);
            if(p == std::string::npos)
            {
                break;
            }

            keywordBeginPos = p + KEYWORD.size();
            ShaderTokenStream tokens(source, keywordBeginPos, ShaderTokenStream::ErrorMode::Material);

            tokens.ConsumeOrThrow("(");
            std::string shaderName = tokens.GetCurrentToken();
            if(!ShaderTokenStream::IsNonEmptyStringLiterial(shaderName))
            {
                tokens.Throw(fmt::format("Invalid shader name: {}", shaderName));
            }
            shaderName = shaderName.substr(1, shaderName.size() - 2);
            tokens.Next();
            tokens.ConsumeOrThrow(")");

            if(tokens.GetCurrentToken() != "{")
            {
                tokens.Throw("'{' expected");
            }
            const size_t charBegin = tokens.GetCurrentPosition();
            const size_t charEnd = FindMatchedRightBracket(source, charBegin);
            if(charEnd == std::string::npos)
            {
                tokens.Throw("Matched '}' is not found");
            }

            auto &shader = database.rawShaders.emplace_back();
            shader.shaderName = std::move(shaderName);
            shader.filename   = filename;
            shader.charBegin  = charBegin + 1;
            shader.charEnd    = charEnd;
        }
    }
    
    void AddMaterialFile(RawShaderDatabase &database, const std::string &filename)
    {
        std::string materialSource = File::ReadTextFile(filename);
        std::vector<RawMaterialRecord> materialRecords; std::vector<RawShader> shaderRecords;
        ParseRawMaterials(materialSource, materialRecords, shaderRecords);
        RemoveComments(materialSource);

        std::string materialSourceForFindingKeyword = materialSource;
        RemoveCommentsAndStrings(materialSourceForFindingKeyword);

        for(auto &s : shaderRecords)
        {
            s.filename = filename;
            database.rawShaders.push_back(std::move(s));
        }
        
        for(auto &material : materialRecords)
        {
            for(auto &pass : material.passes)
            {
                const size_t absPassBegin = material.charBegin + pass.charBegin;
                const std::string_view passSource =
                    std::string_view(materialSource)
                   .substr(absPassBegin, pass.charEnd - pass.charBegin);
                const std::string_view passSourceForFindingKeyword =
                    std::string_view(materialSourceForFindingKeyword)
                   .substr(absPassBegin, pass.charEnd - pass.charBegin);

                constexpr std::string_view SHADER = "rtrc_shader";
                const size_t shaderBeginPos = FindKeyword(passSourceForFindingKeyword, SHADER, 0);
                if(shaderBeginPos == std::string::npos)
                {
                    continue;
                }

                ShaderTokenStream tokens(
                    passSource, shaderBeginPos + SHADER.size(), ShaderTokenStream::ErrorMode::Material);

                tokens.ConsumeOrThrow("(");
                std::string shaderName;
                if(tokens.GetCurrentToken() == ")")
                {
                    shaderName = fmt::format("{}/{}/shader", material.name, pass.name);
                }
                else
                {
                    shaderName = tokens.GetCurrentToken();
                    if(!ShaderTokenStream::IsNonEmptyStringLiterial(shaderName))
                    {
                        tokens.Throw("Invalid shader name: " + shaderName);
                    }
                    shaderName = shaderName.substr(1, shaderName.size() - 2);
                    tokens.Next();
                }
                tokens.ConsumeOrThrow(")");

                if(tokens.GetCurrentToken() != "{")
                {
                    tokens.Throw("'{' expected");
                }
                const size_t charBegin = tokens.GetCurrentPosition();
                const size_t charEnd = FindMatchedRightBracket(passSource, charBegin);
                if(charEnd == std::string::npos)
                {
                    tokens.Throw("Matched '}' not found");
                }

                auto &shader = database.rawShaders.emplace_back();
                shader.filename   = filename;
                shader.shaderName = std::move(shaderName);
                shader.charBegin  = absPassBegin + charBegin + 1;
                shader.charEnd    = absPassBegin + charEnd;
            }
        }
    }
    
} // namespace ShaderDatabaseDetail

void ParseRawMaterials(
    const std::string              &rawSource,
    std::vector<RawMaterialRecord> &rawMaterials,
    std::vector<RawShader>         &rawShaders)
{
    std::string source = rawSource;
    RemoveComments(source);

    std::string sourceForFindingKeyword = source;
    RemoveCommentsAndStrings(sourceForFindingKeyword);

    // Collect material records
    
    rawMaterials.clear();
    rawShaders.clear();

    size_t materialBeginPos = 0;
    while(true)
    {
        constexpr std::string_view MATERIAL = "rtrc_material";
        const size_t materialPos = FindKeyword(sourceForFindingKeyword, MATERIAL, materialBeginPos);
        if(materialPos == std::string::npos)
        {
            break;
        }

        materialBeginPos = materialPos + MATERIAL.size();
        ShaderTokenStream tokens(source, materialBeginPos, ShaderTokenStream::ErrorMode::Material);
        tokens.ConsumeOrThrow("(");
        std::string materialName = tokens.GetCurrentToken();
        if(!ShaderTokenStream::IsNonEmptyStringLiterial(materialName))
        {
            tokens.Throw(fmt::format("Invalid material name: {}", materialName));
        }
        materialName = materialName.substr(1, materialName.size() - 2);
        tokens.Next();
        tokens.ConsumeOrThrow(")");
        
        if(tokens.GetCurrentToken() != "{")
        {
            tokens.Throw("'{' expected");
        }
        const size_t charBegin = tokens.GetCurrentPosition();
        const size_t charEnd = FindMatchedRightBracket(source, charBegin);
        if(charEnd == std::string::npos)
        {
            tokens.Throw("Matched '}' is not found");
        }

        auto &record = rawMaterials.emplace_back();
        record.name      = std::move(materialName);
        record.charBegin = charBegin;
        record.charEnd   = charEnd;
    }

    // Collect property & pass records

    for(auto &material : rawMaterials)
    {
        const std::string_view materialSource = std::string_view(source).substr(
            material.charBegin, material.charEnd - material.charBegin);
        const std::string_view materialSourceForFindingKeyword = std::string_view(sourceForFindingKeyword).substr(
            material.charBegin, material.charEnd - material.charBegin);

        size_t propertyBeginPos = 0;
        while(true)
        {
            constexpr std::string_view PROPERTY = "rtrc_property";
            const size_t propertyPos = FindKeyword(materialSourceForFindingKeyword, PROPERTY, propertyBeginPos);
            if(propertyPos == std::string::npos)
            {
                break;
            }
            propertyBeginPos = propertyPos + PROPERTY.size();
            ShaderTokenStream tokens(materialSource, propertyBeginPos, ShaderTokenStream::ErrorMode::Material);
            tokens.ConsumeOrThrow("(");
            std::string type = tokens.GetCurrentToken();
            if(!ShaderTokenStream::IsIdentifier(type))
            {
                tokens.Throw(fmt::format("Invalid material property type: {}", type));
            }
            tokens.Next();
            tokens.ConsumeOrThrow(",");
            std::string name = tokens.GetCurrentToken();
            if(!ShaderTokenStream::IsIdentifier(name))
            {
                tokens.Throw(fmt::format("Invalid material property name: {}", name));
            }
            tokens.Next();
            if(tokens.GetCurrentToken() != ")")
            {
                tokens.Throw("')' expected");
            }

            auto &prop = material.properties.emplace_back();
            prop.type = std::move(type);
            prop.name = std::move(name);
        }

        size_t passBeginPos = 0;
        while(true)
        {
            constexpr std::string_view PASS = "rtrc_pass";
            const size_t passPos = FindKeyword(materialSourceForFindingKeyword, PASS, passBeginPos);
            if(passPos == std::string::npos)
            {
                break;
            }

            passBeginPos = passPos + PASS.size();
            ShaderTokenStream tokens(materialSource, passBeginPos, ShaderTokenStream::ErrorMode::Material);
            tokens.ConsumeOrThrow("(");
            std::string passName;
            if(tokens.GetCurrentToken() != ")")
            {
                passName = tokens.GetCurrentToken();
                if(!ShaderTokenStream::IsNonEmptyStringLiterial(passName))
                {
                    tokens.Throw(fmt::format("Invalid pass name: {}", passName));
                }
                passName = passName.substr(1, passName.size() - 2);
                tokens.Next();
            }
            else
            {
                passName = fmt::format("pass{}", material.passes.size());
            }
            tokens.ConsumeOrThrow(")");

            if(tokens.GetCurrentToken() != "{")
            {
                tokens.Throw("'{' expected");
            }
            const size_t passCharBegin = tokens.GetCurrentPosition();
            const size_t passCharEnd = FindMatchedRightBracket(materialSource, passCharBegin);
            if(passCharEnd == std::string::npos)
            {
                tokens.Throw("Matched '}' is not found");
            }

            auto &record = material.passes.emplace_back();
            record.name      = std::move(passName);
            record.charBegin = passCharBegin;
            record.charEnd   = passCharEnd;
        }

        for(auto &pass : material.passes)
        {
            const std::string_view passSource = std::string_view(source).substr(
                material.charBegin + pass.charBegin, pass.charEnd - pass.charBegin);
            const std::string_view passSourceForFindingKeyword = std::string_view(sourceForFindingKeyword).substr(
                material.charBegin + pass.charBegin, pass.charEnd - pass.charBegin);

            // Tags

            size_t tagBeginPos = 0;
            while(true)
            {
                constexpr std::string_view TAG = "rtrc_tag";
                const size_t tagPos = FindKeyword(passSourceForFindingKeyword, TAG, tagBeginPos);
                if(tagPos == std::string::npos)
                {
                    break;
                }
                tagBeginPos = tagPos + TAG.size();
                ShaderTokenStream tokens(passSource, tagBeginPos, ShaderTokenStream::ErrorMode::Material);
                tokens.ConsumeOrThrow("(");
                while(true)
                {
                    if(tokens.IsFinished())
                    {
                        tokens.Throw("pass tag expected");
                    }
                    std::string tag = tokens.GetCurrentToken();
                    if(!ShaderTokenStream::IsNonEmptyStringLiterial(tag))
                    {
                        tokens.Throw("Invalid pass tag: " + tag);
                    }
                    tag = tag.substr(1, tag.size() - 2);
                    pass.tags.push_back(std::move(tag));
                    tokens.Next();
                    if(tokens.GetCurrentToken() == ")")
                    {
                        break;
                    }
                    tokens.ConsumeOrThrow(",");
                }
            }

            // Shader

            constexpr std::string_view KEYWORDS[] = { "rtrc_shader", "rtrc_shader_ref" };
            std::string_view keyword;
            if(const size_t shaderPos = FindFirstKeyword(passSourceForFindingKeyword, KEYWORDS, 0, keyword);
               shaderPos != std::string::npos)
            {
                ShaderTokenStream tokens(
                    passSource, shaderPos + keyword.size(), ShaderTokenStream::ErrorMode::Material);
                tokens.ConsumeOrThrow("(");
                std::string shaderName;
                if(keyword == "rtrc_shader" && tokens.GetCurrentToken() == ")")
                {
                    shaderName = fmt::format("{}/{}/shader", material.name, pass.name);
                }
                else
                {
                    shaderName = tokens.GetCurrentToken();
                    if(!ShaderTokenStream::IsNonEmptyStringLiterial(shaderName))
                    {
                        tokens.Throw("Invalid shader name: " + shaderName);
                    }
                    shaderName = shaderName.substr(1, shaderName.size() - 2);
                    tokens.Next();
                }
                pass.shaderName = std::move(shaderName);
                tokens.ConsumeOrThrow(")");
            }
            else
            {
                throw Exception(fmt::format("Shader not found in {}/{}", material.name, pass.name));
            }
        }
    }

    // Standalone shaders

    for(auto &m : rawMaterials)
    {
        for(size_t i = m.charBegin; i < m.charEnd; ++i)
        {
            if(!std::isspace(source[i]))
            {
                source[i] = ' ';
            }
            if(!std::isspace(sourceForFindingKeyword[i]))
            {
                sourceForFindingKeyword[i] = ' ';
            }
        }
    }
    
    size_t shaderBeginPos = 0;
    while(true)
    {
        constexpr std::string_view KEYWORD = "rtrc_shader";
        const size_t p = FindKeyword(sourceForFindingKeyword, KEYWORD, shaderBeginPos);
        if(p == std::string::npos)
        {
            break;
        }

        shaderBeginPos = p + KEYWORD.size();
        ShaderTokenStream tokens(source, shaderBeginPos, ShaderTokenStream::ErrorMode::Material);

        tokens.ConsumeOrThrow("(");
        std::string shaderName = tokens.GetCurrentToken();
        if(!ShaderTokenStream::IsNonEmptyStringLiterial(shaderName))
        {
            tokens.Throw(fmt::format("Invalid shader name: {}", shaderName));
        }
        shaderName = shaderName.substr(1, shaderName.size() - 2);
        tokens.Next();
        tokens.ConsumeOrThrow(")");

        if(tokens.GetCurrentToken() != "{")
        {
            tokens.Throw("'{' expected");
        }
        const size_t charBegin = tokens.GetCurrentPosition();
        const size_t charEnd = FindMatchedRightBracket(source, charBegin);
        if(charEnd == std::string::npos)
        {
            tokens.Throw("Matched '}' is not found");
        }

        auto &shader = rawShaders.emplace_back();
        shader.shaderName = std::move(shaderName);
        shader.charBegin  = charBegin + 1;
        shader.charEnd    = charEnd;
    }
}

RawShaderDatabase CreateRawShaderDatabase(const std::set<std::filesystem::path> &filenames)
{
    using path = std::filesystem::path;

    std::set<std::string> shaderFilenames;
    std::set<std::string> materialFilenames;
    for(const path &p : filenames)
    {
        if(!is_regular_file(p))
        {
            continue;
        }
        const path absp = absolute(p).lexically_normal();
        const std::string ext = ToLower(absp.extension().string());
        if(ext == ".shader")
        {
            shaderFilenames.insert(absp.string());
        }
        else if(ext == ".material")
        {
            materialFilenames.insert(absp.string());
        }
    }

    std::map<std::string, int> filenameToIndex;
    RawShaderDatabase database;
    for(auto &filename : shaderFilenames)
    {
        ShaderDatabaseDetail::AddShaderFile(database, filename);
    }
    for(auto &filename : materialFilenames)
    {
        ShaderDatabaseDetail::AddMaterialFile(database, filename);
    }
    
    return database;
}

RTRC_END
