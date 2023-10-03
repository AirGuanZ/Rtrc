#pragma once

#include <optional>

#include <RHI/RHI.h>

RTRC_BEGIN

struct ParsedRayGenShaderGroup
{
    std::string entry;

    RTRC_AUTO_SERIALIZE_WITH_MEMBER_COUNT_CHECK(entry);
};

struct ParsedMissShaderGroup
{
    std::string entry;

    RTRC_AUTO_SERIALIZE_WITH_MEMBER_COUNT_CHECK(entry);
};

struct ParsedHitShaderGroup
{
    std::string closestHitEntry;
    std::string anyHitEntry;
    std::string intersectionEntry;

    RTRC_AUTO_SERIALIZE_WITH_MEMBER_COUNT_CHECK(closestHitEntry, anyHitEntry, intersectionEntry);
};

struct ParsedShaderEntry
{
    std::string vertexEntry;
    std::string fragmentEntry;
    std::string computeEntry;

    bool isRayTracingShader = false;
    std::vector<std::vector<std::string>> shaderGroups;

    RTRC_AUTO_SERIALIZE_WITH_MEMBER_COUNT_CHECK(
        vertexEntry, fragmentEntry, computeEntry, isRayTracingShader, shaderGroups);
};

struct ParsedBindingAlias
{
    std::string      name;
    std::string      aliasedName;
    RHI::BindingType bindingType;
    std::string      rawTypename;
    std::string      templateParam;

    RTRC_AUTO_SERIALIZE_WITH_MEMBER_COUNT_CHECK(name, aliasedName, bindingType, rawTypename, templateParam);
};

struct ParsedBinding
{
    std::string             name;
    RHI::BindingType        type = {};
    std::string             rawTypeName;
    std::optional<uint32_t> arraySize;
    RHI::ShaderStageFlags   stages = {};
    std::string             templateParam;
    bool                    bindless = false;
    bool                    variableArraySize = false;
    std::string             definedGroupName; // Which group/groupStruct this binding is defined in
                                              // (if it is not referenced by rtrc_ref)

    auto operator<=>(const ParsedBinding &) const = default;

    RTRC_AUTO_SERIALIZE_WITH_MEMBER_COUNT_CHECK(
        name, type, rawTypeName, arraySize, stages, templateParam, bindless, variableArraySize, definedGroupName);
};

struct ParsedUniformDefinition
{
    std::string type;
    std::string name;

    auto operator<=>(const ParsedUniformDefinition &) const = default;

    RTRC_AUTO_SERIALIZE_WITH_MEMBER_COUNT_CHECK(type, name);
};

struct ParsedBindingGroup
{
    std::string                          name;
    std::vector<ParsedUniformDefinition> uniformPropertyDefinitions;
    std::vector<ParsedBinding>           bindings;
    std::vector<bool>                    isRef;
    RHI::ShaderStageFlags                stages;

    auto operator<=>(const ParsedBindingGroup &) const = default;

    RTRC_AUTO_SERIALIZE_WITH_MEMBER_COUNT_CHECK(name, uniformPropertyDefinitions, bindings, isRef, stages);
};

struct ParsedPushConstantVariable
{
    enum Type
    {
        Float, Float2, Float3, Float4,
        Int,   Int2,   Int3,   Int4,
        UInt,  UInt2,  UInt3,  UInt4
    };

    static const char *TypeToName(Type type);
    static size_t      TypeToSize(Type type);

    uint32_t    offset;
    Type        type;
    std::string name;

    RTRC_AUTO_SERIALIZE_WITH_MEMBER_COUNT_CHECK(offset, type, name);
};

struct ParsedPushConstantRange
{
    RHI::PushConstantRange                  range;
    std::vector<ParsedPushConstantVariable> variables;
    std::string                             name;

    RTRC_AUTO_SERIALIZE_WITH_MEMBER_COUNT_CHECK(range, variables, name);
};

// rtrc_define          (Texture2D<float3>[101], Tex1, FS)
// rtrc_bindless        (Texture2D<float3>[512], Tex2)
// rtrc_bindless_varsize(Texture2D<float3>[512], Tex3, RT_CHS)
void ParseBindings(
    const std::string          &source,
    const std::string          &sourceWithoutString,
    std::vector<ParsedBinding> &bindings);

void ParseBindingGroups(
    const std::string                &source,
    const std::string                &sourceWithoutString,
    const std::vector<ParsedBinding> &allBindings,
    std::vector<ParsedBinding>       &ungroupedBindings, 
    std::vector<ParsedBindingGroup>  &groups);

// rtrc_alias(Texture2D<float4>, AliasTex, SourceTex)
void ParseBindingAliases(
    const std::string               &source,
    const std::string               &sourceWithoutString,
    std::vector<ParsedBindingAlias> &aliases);

void ParseInlineSamplers(
    const std::string             &source,
    const std::string             &sourceWithoutString,
    std::vector<RHI::SamplerDesc> &descs,
    std::map<std::string, int>    &nameToDescIndex);

// rtrc_push_constant(Range0, VS)
// {
//     float2 xy;
//     float  z;
//     uint   w;
// };
// rtrc_push_constant(Range2)
// { ... };
void ParsePushConstantRanges(
    const std::string                    &source,
    const std::string                    &sourceWithoutString,
    std::vector<ParsedPushConstantRange> &ranges);

RTRC_END
