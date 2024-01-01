#pragma once

#include <Rtrc/Core/Serialization/Serialize.h>
#include <Rtrc/ShaderCommon/Parser/RawShader.h>

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
    std::string source;

    std::string vertexEntry;
    std::string fragmentEntry;
    std::string computeEntry;
    std::vector<std::vector<std::string>> entryGroups;

    std::vector<ParsedBindingGroup> bindingGroups;
    std::vector<ParsedBinding>      ungroupedBindings;
    std::vector<ParsedBindingAlias> aliases;

    std::vector<RHI::SamplerDesc> inlineSamplerDescs;
    std::map<std::string, int>    inlineSamplerNameToDesc;

    std::vector<ParsedPushConstantRange> pushConstantRanges;

    RTRC_AUTO_SERIALIZE_WITH_MEMBER_COUNT_CHECK(
        source,
        vertexEntry,
        fragmentEntry,
        computeEntry,
        entryGroups,
        bindingGroups,
        ungroupedBindings,
        aliases,
        inlineSamplerDescs,
        inlineSamplerNameToDesc,
        pushConstantRanges);
};

struct ParsedShader
{
    std::string                      name;
    std::string                      sourceFilename;
    std::vector<ShaderKeyword>       keywords;
    std::vector<ParsedShaderVariant> variants;

    RTRC_AUTO_SERIALIZE_WITH_MEMBER_COUNT_CHECK(name, sourceFilename, keywords, variants);
};

std::vector<ShaderKeyword> CollectKeywords(const std::string &source);

size_t ComputeVariantIndex(Span<ShaderKeyword> keywords, Span<ShaderKeywordValue> values);

std::vector<ShaderKeywordValue> ExtractKeywordValues(Span<ShaderKeyword> keywords, size_t variantIndex);

// dxc is optional
ParsedShader ParseShader(
    const ShaderCompileEnvironment &envir,
    const DXC                      *dxc,
    std::string                     source,
    std::string                     shaderName,
    std::string                     filename);

ParsedShader ParseShader(
    const ShaderCompileEnvironment &envir,
    DXC                            *dxc,
    const RawShader                &rawShader);

RTRC_END
