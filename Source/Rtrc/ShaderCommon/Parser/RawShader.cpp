#include <Rtrc/Core/Filesystem/File.h>
#include <Rtrc/Core/Parser/ShaderTokenStream.h>
#include <Rtrc/Core/String.h>
#include <Rtrc/ShaderCommon/Parser/RawShader.h>
#include <Rtrc/ShaderCommon/Parser/ParserHelper.h>

RTRC_BEGIN

namespace ShaderDatabaseDetail
{
    
    // rtrc_shader("ShaderName")
    // {...}
    // rtrc_shader("AnotherShaderName")
    // {...}
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
            shader.charBegin  = static_cast<int>(charBegin) + 1;
            shader.charEnd    = static_cast<int>(charEnd);
        }
    }
    
} // namespace ShaderDatabaseDetail

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
    }

    std::map<std::string, int> filenameToIndex;
    RawShaderDatabase database;
    for(auto &filename : shaderFilenames)
    {
        ShaderDatabaseDetail::AddShaderFile(database, filename);
    }
    
    return database;
}

RTRC_END
