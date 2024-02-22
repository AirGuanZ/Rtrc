#include <Rtrc/Core/Filesystem/File.h>
#include <Rtrc/Core/String.h>
#include <Rtrc/Core/Parser/ShaderTokenStream.h>
#include <Rtrc/ShaderCommon/DXC/DXC.h>
#include <Rtrc/ShaderCommon/Parser/ParserHelper.h>
#include <Rtrc/ShaderCommon/Parser/ShaderParser.h>

RTRC_BEGIN

namespace ShaderParserDetail
{

    int GetValueCount(const ShaderKeyword &keyword)
    {
        return keyword.Match(
            [&](const BoolShaderKeyword &) { return 2; },
            [&](const EnumShaderKeyword &ke) { return static_cast<int>(ke.values.size()); });
    }

    template<typename F>
    void ForEachKeywordCombinationHelper(
        Span<ShaderKeyword>        allKeywords,
        std::vector<ShaderKeywordValue> &processedValues,
        const F                         &func)
    {
        const size_t processedCount = processedValues.size();
        if(processedCount == allKeywords.size())
        {
            func(processedValues);
            return;
        }

        const int nextKeywordValueCount = GetValueCount(allKeywords[processedCount]);
        processedValues.emplace_back();
        for(int i = 0; i < nextKeywordValueCount; ++i)
        {
            processedValues.back().value = i;
            ShaderParserDetail::ForEachKeywordCombinationHelper(allKeywords, processedValues, func);
        }
        processedValues.pop_back();
    }

    template<typename F>
    void ForEachKeywordCombination(Span<ShaderKeyword> allKeywords, const F &func)
    {
        std::vector<ShaderKeywordValue> values;
        ShaderParserDetail::ForEachKeywordCombinationHelper(allKeywords, values, func);
    }

    void ParseEntries(const std::string &source, const std::string &sourceWithoutString, ParsedShaderVariant &variant)
    {
        size_t beginPos = 0;
        while(true)
        {
            constexpr std::string_view KEYWORDS[] =
            {
                "rtrc_vertex",
                "rtrc_vert",
                "rtrc_pixel",
                "rtrc_fragment",
                "rtrc_frag",
                "rtrc_compute",
                "rtrc_comp",
                "rtrc_raytracing",
                "rtrc_rt",
                "rtrc_rt_group",
                "rtrc_task",
                "rtrc_mesh",
            };
            std::string_view keyword;
            const size_t pos = FindFirstKeyword(sourceWithoutString, KEYWORDS, beginPos, keyword);
            if(pos == std::string::npos)
            {
                break;
            }

            beginPos = pos + keyword.size();
            ShaderTokenStream tokens(source, beginPos);
            tokens.ConsumeOrThrow("(");

            if(keyword == "rtrc_vertex" || keyword == "rtrc_vert")
            {
                std::string entryName = tokens.GetCurrentToken();
                if(!ShaderTokenStream::IsIdentifier(entryName))
                {
                    tokens.Throw("Invalid vertex shader entry name: " + entryName);
                }
                tokens.Next();
                variant.vertexEntry = std::move(entryName);
            }
            else if(keyword == "rtrc_pixel" || keyword == "rtrc_fragment" || keyword == "rtrc_frag")
            {
                std::string entryName = tokens.GetCurrentToken();
                if(!ShaderTokenStream::IsIdentifier(entryName))
                {
                    tokens.Throw("Invalid fragment shader entry name: " + entryName);
                }
                tokens.Next();
                variant.fragmentEntry = std::move(entryName);
            }
            else if(keyword == "rtrc_compute" || keyword == "rtrc_comp")
            {
                std::string entryName = tokens.GetCurrentToken();
                if(!ShaderTokenStream::IsIdentifier(entryName))
                {
                    tokens.Throw("Invalid compute shader entry name: " + entryName);
                }
                tokens.Next();
                variant.computeEntry = std::move(entryName);
            }
            else if(keyword == "rtrc_task")
            {
                std::string entryName = tokens.GetCurrentToken();
                if(!ShaderTokenStream::IsIdentifier(entryName))
                {
                    tokens.Throw("Invalid task shader entry name: " + entryName);
                }
                tokens.Next();
                variant.taskEntry = std::move(entryName);
            }
            else if(keyword == "rtrc_mesh")
            {
                std::string entryName = tokens.GetCurrentToken();
                if(!ShaderTokenStream::IsIdentifier(entryName))
                {
                    tokens.Throw("Invalid mesh shader entry name: " + entryName);
                }
                tokens.Next();
                variant.meshEntry = std::move(entryName);
            }
            else if(keyword == "rtrc_raytracing" || keyword == "rtrc_rt")
            {
                // Do nothing
            }
            else
            {
                assert(keyword == "rtrc_rt_group");
                std::vector<std::string> group;
                while(true)
                {
                    std::string name = tokens.GetCurrentToken();
                    if(!ShaderTokenStream::IsIdentifier(name))
                    {
                        tokens.Throw("Invalid entry name in rt shader group: " + name);
                    }
                    group.push_back(std::move(name));
                    tokens.Next();

                    if(tokens.GetCurrentToken() != ",")
                    {
                        break;
                    }
                    tokens.Next();
                }
                variant.entryGroups.push_back(std::move(group));
            }

            if(tokens.GetCurrentToken() != ")")
            {
                tokens.Throw("')' expected");
            }
        }
    }

    void FillParsedShaderVariant(
        const ShaderCompileEnvironment &envir,
        const DXC                      *dxc,
        ParsedShader                   &shader,
        Span<ShaderKeywordValue>        keywordValues,
        const std::string              &source)
    {
        const int variantIndex = ComputeVariantIndex(shader.keywords, keywordValues);
        ParsedShaderVariant &variant = shader.variants[variantIndex];

        // Macros

        std::map<std::string, std::string> macros = envir.macros;
        macros["_rtrc_generated_shader_prefix"] = {};

        assert(shader.keywords.size() == keywordValues.size());
        for(size_t i = 0; i < shader.keywords.size(); ++i)
        {
            const int value = keywordValues[i].value;
            shader.keywords[i].Match(
                [&](const BoolShaderKeyword &keyword)
                {
                    macros[keyword.name] = std::to_string(value);
                },
                [&](const EnumShaderKeyword &keyword)
                {
                    for(size_t j = 0; j < keyword.values.size(); ++j)
                    {
                        macros[fmt::format("{}_{}", keyword.name, keyword.values[j])] = std::to_string(j);
                    }
                    macros[keyword.name] = fmt::format("{}_{}", keyword.name, keyword.values[value]);
                });
        }

        // Preprocess

        DXC::ShaderInfo shaderInfo;
        shaderInfo.source         = source;
        shaderInfo.sourceFilename = shader.sourceFilename;
        shaderInfo.includeDirs    = envir.includeDirs;
        shaderInfo.macros         = std::move(macros);

        Box<DXC> tempDXC;
        if(!dxc)
        {
            tempDXC = MakeBox<DXC>();
            dxc = tempDXC.get();
        }

        std::string preprocessedSource;
        dxc->Compile(shaderInfo, DXC::Target::DirectX12_CS_6_6, true, &preprocessedSource, nullptr, nullptr);

        std::string preprocessedSouceWithoutString = preprocessedSource;
        RemoveComments(preprocessedSource);
        RemoveCommentsAndStrings(preprocessedSouceWithoutString);

        // Parse entries

        ParseEntries(preprocessedSource, preprocessedSouceWithoutString, variant);

        // Parse bindings

        std::vector<ParsedBinding> allBindings;
        ParseBindings(preprocessedSource, preprocessedSouceWithoutString, allBindings);
        ParseBindingGroups(preprocessedSource, preprocessedSouceWithoutString, allBindings, variant.ungroupedBindings, variant.bindingGroups);
        ParseBindingAliases(preprocessedSource, preprocessedSouceWithoutString, variant.aliases);
        ParseInlineSamplers(preprocessedSource, preprocessedSouceWithoutString, variant.inlineSamplerDescs, variant.inlineSamplerNameToDesc);
        ParsePushConstantRanges(preprocessedSource, preprocessedSouceWithoutString, variant.pushConstantRanges);
    }

} // namespace ShaderParserDetail

ParsedShader::ParsedShader()
{
    cachedSourceMutex = MakeBox<std::shared_mutex>();
}

const std::string &ParsedShader::GetCachedSource() const
{
    {
        std::shared_lock lock(*cachedSourceMutex);
        if(!cachedSource.empty())
        {
            return cachedSource;
        }
    }

    std::unique_lock lock(*cachedSourceMutex);
    if(!cachedSource.empty())
    {
        return cachedSource;
    }

    std::string fileContent = File::ReadTextFile(sourceFilename);
    std::string fileContentWithoutComments = fileContent;
    RemoveComments(fileContentWithoutComments);

    cachedSource = ReadShaderSource(fileContent, fileContentWithoutComments, name);
    cachedSource = "_rtrc_generated_shader_prefix " + cachedSource;

    return cachedSource;
}

std::vector<ShaderKeyword> CollectKeywords(const std::string &source)
{
    std::vector<ShaderKeyword> ret;
    size_t beginPos = 0;
    while(true)
    {
        constexpr std::string_view KEYWORD = "rtrc_keyword";
        const size_t pos = FindKeyword(source, KEYWORD, beginPos);
        if(pos == std::string::npos)
        {
            break;
        }
        beginPos = pos + KEYWORD.size();
        ShaderTokenStream tokens(source, beginPos);
        tokens.ConsumeOrThrow("(");

        std::string name = tokens.ConsumeIdentifierOrThrow("Keyword name expected");

        if(tokens.GetCurrentToken() == ")") // bool
        {
            auto &keyword = ret.emplace_back();
            keyword = BoolShaderKeyword{ std::move(name) };
        }
        else // enum
        {
            std::vector<std::string> values;
            while(tokens.GetCurrentToken() == ",")
            {
                tokens.Next();
                values.push_back(tokens.ConsumeIdentifierOrThrow("Keyword value expected"));
            }
            auto &keyword = ret.emplace_back();
            keyword = EnumShaderKeyword{ std::move(name), std::move(values) };
        }

        if(tokens.GetCurrentToken() != ")")
        {
            tokens.Throw("')' expected");
        }
    }
    return ret;
}

std::vector<std::string> GetCppSymbolName(const std::string &source)
{
    constexpr std::string_view keyword = "rtrc_symbol_name";
    const size_t p = FindKeyword(source, keyword, 0);
    if(p == std::string::npos)
    {
        return {};
    }
    ShaderTokenStream tokens(source, p + keyword.size());
    tokens.ConsumeOrThrow("(");
    std::vector<std::string> ret;
    while(true)
    {
        std::string name = tokens.GetCurrentToken();
        if(!ShaderTokenStream::IsIdentifier(name))
        {
            tokens.Throw("Symbol name identifier expected");
        }
        ret.push_back(std::move(name));
        tokens.Next();

        if(tokens.GetCurrentToken() != "::")
        {
            break;
        }
        tokens.Next();
    }
    if(tokens.GetCurrentToken() != ")")
    {
        tokens.Throw("')' expected");
    }
    return ret;
}

int ComputeVariantIndex(Span<ShaderKeyword> keywords, Span<ShaderKeywordValue> values)
{
    assert(keywords.size() == values.size());
    int ret = 0;
    for(size_t i = 0; i < keywords.size(); ++i)
    {
        const int valueCount = ShaderParserDetail::GetValueCount(keywords[i]);
        ret = valueCount * ret + values[i].value;
    }
    return ret;
}

std::vector<ShaderKeywordValue> ExtractKeywordValues(Span<ShaderKeyword> keywords, size_t variantIndex)
{
    std::vector<ShaderKeywordValue> values;
    values.resize(keywords.size());
    for(int i = static_cast<int>(keywords.size()) - 1; i >= 0; --i)
    {
        const int valueCount = ShaderParserDetail::GetValueCount(keywords[i]);
        values[i].value = static_cast<int>(variantIndex) % valueCount;
        variantIndex /= valueCount;
    }
    return values;
}

std::string ReadShaderSource(
    std::string_view   fileContent,
    const std::string &fileContentWithoutComments,
    std::string_view   shaderName)
{
    const std::string key = fmt::format("rtrc_shader(\"{}\")", shaderName);
    const size_t p = fileContentWithoutComments.find(key);
    if(p == std::string_view::npos)
    {
        throw Exception(fmt::format("Cannot find shader \"{}\" in given file", shaderName));
    }

    ShaderTokenStream tokens(
        fileContentWithoutComments, p + key.size(), ShaderTokenStream::ErrorMode::Material);

    if(tokens.GetCurrentToken() != "{")
    {
        tokens.Throw("{ expected");
    }

    const size_t charBegin = tokens.GetCurrentPosition();
    const size_t charEnd = FindMatchedRightBracket(fileContentWithoutComments, charBegin);
    if(charEnd == std::string_view::npos)
    {
        tokens.Throw("Matched '}' is not found");
    }

    std::string result(fileContent);
    for(size_t i = 0; i < charBegin + 1; ++i)
    {
        if(result[i] != '\n' && result[i] != '\r')
        {
            result[i] = ' ';
        }
    }
    for(size_t i = charEnd; i < fileContent.size(); ++i)
    {
        if(result[i] != '\n' && result[i] != '\r')
        {
            result[i] = ' ';
        }
    }
    return result;
}

ParsedShader ParseShader(
    const ShaderCompileEnvironment &envir,
    const DXC                      *dxc,
    std::string                     source,
    std::string                     shaderName,
    const std::string              &filename)
{
    source = "_rtrc_generated_shader_prefix " + source;
    
    // Collect keywords

    auto keywords = CollectKeywords(source);

    size_t totalVariantCount = 1;
    for(auto &kw : keywords)
    {
        totalVariantCount *= ShaderParserDetail::GetValueCount(kw);
    }

    // Parse variants

    ParsedShader ret;
    ret.name           = std::move(shaderName);
    ret.cppSymbolName  = GetCppSymbolName(source);
    ret.sourceFilename = std::filesystem::relative(filename).string();
    ret.keywords       = std::move(keywords);

    if(ret.cppSymbolName.empty())
    {
        std::string name = ret.name;
        Replace_(name, "\\", "/");
        Replace_(name, ":", "/");
        ret.cppSymbolName = Split(name, "/");
    }

    ret.variants.resize(totalVariantCount);
    ShaderParserDetail::ForEachKeywordCombination(ret.keywords, [&](Span<ShaderKeywordValue> values)
    {
        ShaderParserDetail::FillParsedShaderVariant(envir, dxc, ret, values, source);
    });

    return ret;
}

ParsedShader ParseShader(const ShaderCompileEnvironment &envir, DXC *dxc, const RawShader &rawShader)
{
    std::string source = File::ReadTextFile(rawShader.filename);
    for(int i = 0; i < rawShader.charBegin; ++i)
    {
        if(!std::isspace(source[i]))
        {
            source[i] = ' ';
        }
    }
    for(int i = rawShader.charEnd; i < static_cast<int>(source.size()); ++i)
    {
        if(!std::isspace(source[i]))
        {
            source[i] = ' ';
        }
    }
    return ParseShader(envir, dxc, source, rawShader.shaderName, rawShader.filename);
}

RTRC_END
