#pragma once

#include <Rtrc/Shader/ShaderCompiler.h>
#include <Rtrc/Utils/StringPool.h>

/*
ShaderTemplate:
    Path path
    KeywordSet keywords
ShaderManager:
    Map<Path, (KeywordSet, Map<KeywordMap, Shader>)> shaders
    map: keywords + path -> shader
Material:
    Map<Tag, Pass> passes
Pass:
    Optional<PipelineState> overridePipelineStates
    KeywordMap overrideKeywords
    Shader GetShader(KeywordMap keywordContext)
*/

RTRC_BEGIN

struct KeywordStringTag { };

using Keyword    = PooledString<KeywordStringTag>;
using KeywordSet = std::set<Keyword>;
using KeywordMap = std::map<Keyword, std::string>;

class ShaderTemplate
{
public:

    RC<Shader> GetShader(const KeywordMap &keywordContext) const;

private:

    KeywordSet keywords_;
    std::function<RC<Shader>(const KeywordMap &)> keywordsToShader_;
};

class ShaderManager
{
public:

    // `keywordMap` must only contain necessary keywords
    // This function should never be called directly in user code. use it indirectly by ShaderTemplate
    RC<Shader> GetShader(const std::string &path, const KeywordMap &keywordMap);

private:

    struct ShaderKey
    {
        std::string path;
        std::vector<std::string> keywordValues;
    };

    std::map<ShaderKey, std::weak_ptr<Shader>> keyToShader_;
};

RTRC_END
