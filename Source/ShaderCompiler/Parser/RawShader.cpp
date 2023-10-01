#include <Core/Filesystem/File.h>
#include <Core/Parser/ShaderTokenStream.h>
#include <Core/String.h>
#include <ShaderCompiler/Parser/RawShader.h>
#include <ShaderCompiler/Parser/ParserHelper.h>

RTRC_SHADER_COMPILER_BEGIN

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
                if(keywordBeginPos > 0)
                {
                    database.rawShaders.back().charEnd = source.size();
                }
                break;
            }
            if(keywordBeginPos > 0)
            {
                database.rawShaders.back().charEnd = p;
            }

            keywordBeginPos = p + KEYWORD.size();
            ShaderTokenStream tokens(source, keywordBeginPos);

            tokens.ConsumeOrThrow("(");

            std::string shaderName = tokens.GetCurrentToken();
            if(!ShaderTokenStream::IsNonEmptyStringLiterial(shaderName))
            {
                tokens.Throw(fmt::format("Invalid shader name: {}", shaderName));
            }
            shaderName = shaderName.substr(1, shaderName.size() - 2);
            tokens.Next();

            if(tokens.GetCurrentToken() != ")")
            {
                tokens.Throw("')' expected");
            }

            auto &shader = database.rawShaders.emplace_back();
            shader.shaderName = std::move(shaderName);
            shader.filename   = filename;
            shader.charBegin  = tokens.GetCurrentPosition();
            shader.charEnd    = shader.charBegin;
        }
    }
    
    void AddMaterialFile(RawShaderDatabase &database, const std::string &filename)
    {
        std::string materialSource = File::ReadTextFile(filename);
        const auto materialRecords = ParseRawMaterials(materialSource);
        RemoveComments(materialSource);

        std::string materialSourceForFindingKeyword = materialSource;
        RemoveCommentsAndStrings(materialSourceForFindingKeyword);

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

                ShaderTokenStream tokens(passSource, shaderBeginPos + SHADER.size());
                tokens.ConsumeOrThrow("(");

                std::string shaderName;
                if(tokens.GetCurrentToken() == ")")
                {
                    shaderName = pass.name;
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

                if(tokens.GetCurrentToken() != ")")
                {
                    tokens.Throw("')' expected");
                }

                const size_t shaderBegin = tokens.GetCurrentPosition();
                const size_t shaderEnd = passSource.size();

                auto &shader = database.rawShaders.emplace_back();
                shader.filename   = filename;
                shader.shaderName = std::move(shaderName);
                shader.charBegin  = absPassBegin + shaderBegin;
                shader.charEnd    = absPassBegin + shaderEnd;
            }
        }
    }
    
} // namespace ShaderDatabaseDetail

std::vector<RawMaterialRecord> ParseRawMaterials(const std::string &rawSource)
{
    std::string source = rawSource;
    RemoveComments(source);

    std::string sourceForFindingKeyword = source;
    RemoveCommentsAndStrings(sourceForFindingKeyword);

    // Collect material records
    
    std::vector<RawMaterialRecord> materials;

    size_t materialBeginPos = 0;
    while(true)
    {
        constexpr std::string_view MATERIAL = "rtrc_material";
        const size_t materialPos = FindKeyword(sourceForFindingKeyword, MATERIAL, materialBeginPos);
        if(materialPos == std::string::npos)
        {
            if(!materials.empty())
            {
                materials.back().charEnd = sourceForFindingKeyword.size();
            }
            break;
        }
        if(!materials.empty())
        {
            materials.back().charEnd = materialPos;
        }

        materialBeginPos = materialPos + MATERIAL.size();
        ShaderTokenStream tokens(source, materialBeginPos);
        tokens.ConsumeOrThrow("(");

        std::string materialName = tokens.GetCurrentToken();
        if(!ShaderTokenStream::IsNonEmptyStringLiterial(materialName))
        {
            tokens.Throw(fmt::format("Invalid material name: {}", materialName));
        }
        materialName = materialName.substr(1, materialName.size() - 2);
        tokens.Next();

        if(tokens.GetCurrentToken() != ")")
        {
            tokens.Throw("')' expected");
        }

        auto &record = materials.emplace_back();
        record.name = std::move(materialName);
        record.charBegin = tokens.GetCurrentPosition();
        record.charEnd = record.charBegin;
    }

    // Collect property & pass records

    for(auto &material : materials)
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
            ShaderTokenStream tokens(materialSource, propertyBeginPos);
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
                if(!material.passes.empty())
                {
                    material.passes.back().charEnd = materialSourceForFindingKeyword.size();
                }
                break;
            }
            if(!material.passes.empty())
            {
                materials.back().charEnd = passPos;
            }

            passBeginPos = passPos + PASS.size();
            ShaderTokenStream tokens(materialSource, passBeginPos);
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

            if(tokens.GetCurrentToken() != ")")
            {
                tokens.Throw("')' expected");
            }

            auto &record = material.passes.emplace_back();
            record.name = std::move(passName);
            record.charBegin = tokens.GetCurrentPosition();
            record.charEnd = record.charBegin;
        }
    }

    return materials;
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

RTRC_SHADER_COMPILER_END
