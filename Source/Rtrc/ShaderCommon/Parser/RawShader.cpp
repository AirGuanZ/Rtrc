#include <map>

#include <Rtrc/Core/Filesystem/File.h>
#include <Rtrc/Core/Parser/ShaderTokenStream.h>
#include <Rtrc/Core/String.h>
#include <Rtrc/ShaderCommon/Parser/RawShader.h>
#include <Rtrc/ShaderCommon/Parser/ParserHelper.h>

RTRC_BEGIN

namespace ShaderDatabaseDetail
{

    std::vector<std::string_view> FindDirectDependencies(
        std::string_view source, std::string_view sourceForFindingKeyword)
    {
        std::vector<std::string_view> ret;

        size_t keywordBeginPos = 0;
        while(true)
        {
            constexpr std::string_view KEYWORD = "rtrc_refcode";
            const size_t p = FindKeyword(sourceForFindingKeyword, KEYWORD, keywordBeginPos);
            if(p == std::string_view::npos)
            {
                return ret;
            }
            keywordBeginPos = p + KEYWORD.size();

            ShaderTokenStream tokens(source, keywordBeginPos, ShaderTokenStream::ErrorMode::Material);
            tokens.ConsumeOrThrow("(");
            std::string name = tokens.GetCurrentToken();
            if(!ShaderTokenStream::IsNonEmptyStringLiterial(name))
            {
                tokens.Throw(std::format("Invalid refcode name: {}", name));
            }
            name = name.substr(1, name.size() - 2); // Remove quotation marks
            tokens.Next();
            tokens.ConsumeOrThrow(")");
        }
    }

    // rtrc_code("CodeName")
    // {...}
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

        struct CodeSegment
        {
            std::string name;
            std::vector<int> directDependencies;
            int charBegin;
            int charEnd;
        };
        std::vector<CodeSegment> codeSegments;

        size_t keywordBeginPos = 0;
        while(true)
        {
            std::string_view keyword;
            const size_t p = FindFirstKeyword(
                sourceForFindingKeyword, { "rtrc_code", "rtrc_shader" }, keywordBeginPos, keyword);
            if(p == std::string_view::npos)
            {
                break;
            }
            keywordBeginPos = p + keyword.size();
            ShaderTokenStream tokens(source, keywordBeginPos, ShaderTokenStream::ErrorMode::Material);

            tokens.ConsumeOrThrow("(");
            std::string segmentName = tokens.GetCurrentToken();
            if(!ShaderTokenStream::IsNonEmptyStringLiterial(segmentName))
            {
                tokens.Throw(std::format("Invalid shader/code name: {}", segmentName));
            }
            segmentName = segmentName.substr(1, segmentName.size() - 2); // Remove quotation marks
            tokens.Next();
            tokens.ConsumeOrThrow(")");

            if(tokens.GetCurrentToken() != "{")
            {
                tokens.Throw("'{' expected");
            }
            const size_t leftBracketPos = tokens.GetCurrentPosition();
            const size_t rightBracketPos = FindMatchedRightBracket(source, leftBracketPos);
            if(rightBracketPos == std::string::npos)
            {
                tokens.Throw("Matched '}' is not found");
            }

            // Find direct dependencies

            const std::string_view segmentContent = std::string_view(source).substr(
                leftBracketPos + 1, rightBracketPos - (leftBracketPos + 1));
            const std::string_view segmentContentForFindingKeyword = std::string_view(sourceForFindingKeyword).substr(
                leftBracketPos + 1, rightBracketPos - (leftBracketPos + 1));
            const auto stringDependencies = FindDirectDependencies(segmentContent, segmentContentForFindingKeyword);

            std::vector<int> directDependencies;
            for(auto &strDep : stringDependencies)
            {
                bool found = false;
                for(size_t i = 0; i < codeSegments.size(); ++i)
                {
                    if(strDep == codeSegments[i].name)
                    {
                        directDependencies.push_back(static_cast<int>(i));
                        found = true;
                        break;
                    }
                }
                if(!found)
                {
                    tokens.Throw(std::format("Unknown code dependency {}", strDep));
                }
            }

            // Finalize

            if(keyword == "rtrc_shader")
            {
                std::set<int> allDependencies;
                std::set<int> pendingDependencies = { directDependencies.begin(), directDependencies.end() };
                while(!pendingDependencies.empty())
                {
                    const auto begin = pendingDependencies.begin();
                    const int directDependency = *begin;
                    pendingDependencies.erase(begin);

                    allDependencies.insert(directDependency);
                    for(int indirectDependencies : codeSegments[directDependency].directDependencies)
                    {
                        if(!allDependencies.contains(indirectDependencies))
                        {
                            pendingDependencies.insert(indirectDependencies);
                        }
                    }
                }

                auto &shader = database.rawShaders.emplace_back();
                shader.shaderName = std::move(segmentName);
                shader.filename = filename;
                for(int segmentIndex : allDependencies)
                {
                    auto &range = shader.ranges.emplace_back();
                    range.begin = codeSegments[segmentIndex].charBegin;
                    range.end = codeSegments[segmentIndex].charEnd;
                }
                shader.ranges.push_back(
                {
                    .begin = static_cast<int>(leftBracketPos) + 1,
                    .end = static_cast<int>(rightBracketPos)
                });
            }
            else
            {
                auto &segment = codeSegments.emplace_back();
                segment.name = std::move(segmentName);
                segment.directDependencies = std::move(directDependencies);
                segment.charBegin = static_cast<int>(leftBracketPos) + 1;
                segment.charEnd = static_cast<int>(rightBracketPos);
            }
        }
    }
    
} // namespace ShaderDatabaseDetail

RawShaderDatabase CreateRawShaderDatabase(const std::filesystem::path &filename)
{
    assert(std::filesystem::is_regular_file(filename));
    assert(filename.extension() == ".shader");

    std::map<std::string, int> filenameToIndex;
    RawShaderDatabase database;
    ShaderDatabaseDetail::AddShaderFile(database, filename.string());
    
    return database;
}

RTRC_END
