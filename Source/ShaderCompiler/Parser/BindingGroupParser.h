#pragma once

#include <optional>

#include <ShaderCompiler/Common.h>

RTRC_SHADER_COMPILER_BEGIN

struct ParsedRayGenShaderGroup
{
    std::string entry;
};

struct ParsedMissShaderGroup
{
    std::string entry;
};

struct ParsedHitShaderGroup
{
    std::string closestHitEntry;
    std::string anyHitEntry;
    std::string intersectionEntry;
};

struct ParsedShaderEntry
{
    std::string vertexEntry;
    std::string fragmentEntry;
    std::string computeEntry;

    bool isRayTracingShader = false;
    std::vector<std::vector<std::string>> shaderGroups;
};

struct ParsedBindingAlias
{
    std::string      name;
    std::string      aliasedName;
    RHI::BindingType bindingType;
    std::string      rawTypename;
    std::string      templateParam;
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

    auto operator<=>(const ParsedBinding &) const = default;
};

struct ParsedUniformDefinition
{
    std::string type;
    std::string name;

    auto operator<=>(const ParsedUniformDefinition &) const = default;
};

struct ParsedBindingGroup
{
    std::string                          name;
    std::vector<ParsedUniformDefinition> uniformPropertyDefinitions;
    std::vector<ParsedBinding>           bindings;
    std::vector<bool>                    isRef;
    RHI::ShaderStageFlags                uniformStages;

    auto operator<=>(const ParsedBindingGroup &) const = default;
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
};

struct ParsedPushConstantRange
{
    RHI::PushConstantRange                  range;
    std::vector<ParsedPushConstantVariable> variables;
    std::string                             name;
};

// rtrc_define          (Texture2D<float3>[101], Tex1, FS)
// rtrc_bindless        (Texture2D<float3>[512], Tex2)
// rtrc_bindless_varsize(Texture2D<float3>[512], Tex3, RT_CHS)
void ParseBindings(
    const std::string          &source,
    std::vector<ParsedBinding> &bindings);

void ParseBindingGroups(
    const std::string                &source,
    const std::vector<ParsedBinding> &allBindings,
    std::vector<ParsedBinding>       &ungroupedBindings, 
    std::vector<ParsedBindingGroup>  &groups);

// rtrc_alias(Texture2D<float4>, AliasTex, SourceTex)
void ParseBindingAliases(
    const std::string               &source,
    std::vector<ParsedBindingAlias> &aliases);

void ParseInlineSamplers(
    const std::string             &source,
    std::vector<RHI::SamplerDesc> &descs,
    std::map<std::string, int>     nameToDescIndex);

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
    std::vector<ParsedPushConstantRange> &ranges);

RTRC_SHADER_COMPILER_END
