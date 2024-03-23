#pragma once

#include <shared_mutex>

#include <Rtrc/Core/Serialization/Serialize.h>
#include <Rtrc/ShaderCommon/Parser/RawShader.h>
#include <Rtrc/ShaderCommon/Parser/RtrcSyntaxParser.h>

RTRC_BEGIN

class DXC;

struct BoolShaderKeyword
{
    std::string name;

    RTRC_AUTO_SERIALIZE_WITH_MEMBER_COUNT_CHECK(name);
};

struct EnumShaderKeyword
{
    std::string              name;
    std::vector<std::string> values;

    RTRC_AUTO_SERIALIZE_WITH_MEMBER_COUNT_CHECK(name, values);
};

using ShaderKeyword = Variant<BoolShaderKeyword, EnumShaderKeyword>;

struct ShaderKeywordValue
{
    int value;
};

struct ShaderCompileEnvironment
{
    std::vector<std::string> includeDirs;
    std::map<std::string, std::string> macros;

    RTRC_AUTO_SERIALIZE_WITH_MEMBER_COUNT_CHECK(includeDirs, macros);
};

struct ParsedShaderVariant
{
    std::string vertexEntry;
    std::string fragmentEntry;
    std::string computeEntry;
    std::string taskEntry;
    std::string meshEntry;
    std::vector<std::vector<std::string>> entryGroups;

    std::vector<ParsedBindingGroup> bindingGroups;
    std::vector<ParsedBinding>      ungroupedBindings;
    std::vector<ParsedBindingAlias> aliases;

    std::vector<RHI::SamplerDesc> inlineSamplerDescs;
    std::map<std::string, int>    inlineSamplerNameToDesc;

    std::vector<ParsedPushConstantRange> pushConstantRanges;
    std::vector<ParsedStructDefinition> specialStructs;

    RTRC_AUTO_SERIALIZE_WITH_MEMBER_COUNT_CHECK(
        vertexEntry,
        fragmentEntry,
        computeEntry,
        taskEntry,
        meshEntry,
        entryGroups,
        bindingGroups,
        ungroupedBindings,
        aliases,
        inlineSamplerDescs,
        inlineSamplerNameToDesc,
        pushConstantRanges,
        specialStructs);
};

struct ParsedShader
{
    std::string                      name;
    std::vector<std::string>         cppSymbolName;
    std::string                      sourceFilename;
    std::vector<ShaderKeyword>       keywords;
    std::vector<ParsedShaderVariant> variants;

    ParsedShader();
    ParsedShader(ParsedShader &&other) noexcept = default;
    ParsedShader &operator=(ParsedShader &&other) noexcept = default;

    const std::string &GetCachedSource() const;

    RTRC_AUTO_SERIALIZE(name, cppSymbolName, sourceFilename, keywords, variants);

private:

    mutable std::string    cachedSource;
    Box<std::shared_mutex> cachedSourceMutex;
};

std::vector<ShaderKeyword> CollectKeywords(const std::string &source);

std::vector<std::string> GetCppSymbolName(const std::string &source);

int ComputeVariantIndex(Span<ShaderKeyword> keywords, Span<ShaderKeywordValue> values);

std::vector<ShaderKeywordValue> ExtractKeywordValues(Span<ShaderKeyword> keywords, size_t variantIndex);

std::string ReadShaderSource(const std::string &filename, std::string_view shaderName);

// dxc is optional
ParsedShader ParseShader(
    const ShaderCompileEnvironment &envir,
    const DXC                      *dxc,
    std::string                     source,
    std::string                     shaderName,
    const std::string              &filename);

ParsedShader ParseShader(
    const ShaderCompileEnvironment &envir,
    DXC                            *dxc,
    const RawShader                &rawShader);

RTRC_END
