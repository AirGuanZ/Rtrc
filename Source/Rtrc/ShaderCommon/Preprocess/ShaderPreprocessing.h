#pragma once

#include <Rtrc/Core/Serialization/Serialize.h>
#include <Rtrc/ShaderCommon/DXC/DXC.h>
#include <Rtrc/ShaderCommon/Parser/ShaderParser.h>

RTRC_BEGIN

struct ShaderPreprocessingInput;
struct ShaderPreprocessingOutput;

class ShaderBindingNameMap
{
public:

    int GetContainedBindingGroupIndex(std::string_view bindingName) const;

    int GetIndexInBindingGroup(std::string_view bindingName) const;

    const std::string &GetBindingName(int groupIndex, int indexInGroup) const;
    const GeneralPooledString &GetPooledBindingName(int groupIndex, int indexInGroup) const;

private:

    friend ShaderPreprocessingOutput PreprocessShader(const ShaderPreprocessingInput &input, RHI::BackendType backend);

    struct BindingIndexInfo
    {
        int bindingGroupIndex;
        int indexInBindingGroup;
    };

    // binding name -> (binding group index, index in group)
    std::map<std::string, BindingIndexInfo, std::less<>> nameToSetAndSlot_;

    std::vector<std::vector<std::string>>         allBindingNames_;
    std::vector<std::vector<GeneralPooledString>> allPooledBindingNames_;
};

enum class ShaderCategory
{
    ClassicalGraphics,
    MeshGraphics,
    Compute,
    RayTracing,
};

using ShaderPropertyName = GeneralPooledString;
#define RTRC_SHADER_PROPERTY_NAME(X) RTRC_GENERAL_POOLED_STRING(X)

struct ShaderUniformBlock
{
    enum UniformType
    {
        Int,   Int2,   Int3,   Int4,
        UInt,  UInt2,  UInt3,  UInt4,
        Float, Float2, Float3, Float4,
        Float4x4,
        Unknown,
    };

    struct UniformVariable
    {
        UniformType type;
        std::string name;
        ShaderPropertyName pooledName;
    };

    static UniformType GetTypeFromTypeName(std::string_view name);
    static size_t GetTypeSize(UniformType type);

    std::string                  name;
    uint32_t                     group;
    uint32_t                     indexInGroup;
    std::vector<UniformVariable> variables;
};

struct ShaderPreprocessingInput
{
    ShaderCompileEnvironment envir;

    std::string name;
    std::string source;
    std::string sourceFilename;

    std::vector<ShaderKeyword>      keywords;
    std::vector<ShaderKeywordValue> keywordValues;

    std::string vertexEntry;
    std::string fragmentEntry;
    std::string computeEntry;
    std::string taskEntry;
    std::string meshEntry;

    bool isRayTracingShader = false;
    std::vector<std::vector<std::string>> entryGroups;

    std::vector<ParsedBindingGroup> bindingGroups;
    std::vector<ParsedBinding>      ungroupedBindings;
    std::vector<ParsedBindingAlias> aliases;

    std::vector<RHI::SamplerDesc> inlineSamplerDescs;
    std::map<std::string, int>    inlineSamplerNameToDesc;

    std::vector<ParsedPushConstantRange> pushConstantRanges;

    RTRC_AUTO_SERIALIZE_WITH_MEMBER_COUNT_CHECK(
        envir,
        name,
        source,
        sourceFilename,
        keywords,
        keywordValues,
        vertexEntry,
        fragmentEntry,
        computeEntry,
        taskEntry,
        meshEntry,
        isRayTracingShader,
        entryGroups,
        bindingGroups,
        ungroupedBindings,
        aliases,
        inlineSamplerDescs,
        inlineSamplerNameToDesc,
        pushConstantRanges);
};

struct ShaderPreprocessingOutput
{
    std::string name;
    std::string source;
    std::string sourceFilename;

    std::string vertexEntry;
    std::string fragmentEntry;
    std::string computeEntry;
    std::string taskEntry;
    std::string meshEntry;

    ShaderCategory category;

    // Valid only when category is RayTracing
    std::vector<std::vector<std::string>> rayTracingEntryGroups;

    std::vector<std::string> includeDirs;
    std::map<std::string, std::string> macros;

    // If present, groups[inlineSamplerBindingGroupIndex] should be filled with corresponding
    // sampler array in the first binding slot
    int inlineSamplerBindingGroupIndex = -1;
    std::vector<RHI::SamplerDesc> inlineSamplerDescs;
    
    std::vector<ShaderUniformBlock>                 uniformBlocks;
    std::vector<RHI::BindingGroupLayoutDesc>        groups;
    std::vector<RHI::PushConstantRange>             pushConstantRanges;
    std::vector<RHI::UnboundedBindingArrayAliasing> unboundedAliases;

    std::vector<std::string>                bindingGroupNames;
    std::map<std::string, int, std::less<>> nameToBindingGroupLayoutIndex;

    ShaderBindingNameMap bindingNameMap;

    bool hasBindless = false;
};

ShaderPreprocessingOutput PreprocessShader(const ShaderPreprocessingInput &input, RHI::BackendType backend);

std::set<std::string> GetDependentFiles(
    DXC                             *dxc,
    const ShaderPreprocessingOutput &shader,
    RHI::BackendType                 backend);

inline int ShaderBindingNameMap::GetContainedBindingGroupIndex(std::string_view bindingName) const
{
    auto it = nameToSetAndSlot_.find(bindingName);
    return it != nameToSetAndSlot_.end() ? it->second.bindingGroupIndex : -1;
}

inline int ShaderBindingNameMap::GetIndexInBindingGroup(std::string_view bindingName) const
{
    auto it = nameToSetAndSlot_.find(bindingName);
    return it != nameToSetAndSlot_.end() ? it->second.indexInBindingGroup : -1;
}

inline const std::string &ShaderBindingNameMap::GetBindingName(int groupIndex, int indexInGroup) const
{
    return allBindingNames_[groupIndex][indexInGroup];
}

inline const GeneralPooledString &ShaderBindingNameMap::GetPooledBindingName(int groupIndex, int indexInGroup) const
{
    return allPooledBindingNames_[groupIndex][indexInGroup];
}

inline ShaderUniformBlock::UniformType ShaderUniformBlock::GetTypeFromTypeName(std::string_view name)
{
    using enum UniformType;
    static const std::map<std::string, UniformType, std::less<>> m =
    {
        { "int",      Int    },
        { "int2",     Int2   },
        { "int3",     Int3   },
        { "int4",     Int4   },
        { "uint",     UInt   },
        { "uint2",    UInt2  },
        { "uint3",    UInt3  },
        { "uint4",    UInt4  },
        { "float",    Float  },
        { "float2",   Float2 },
        { "float3",   Float3 },
        { "float4",   Float4 },
        { "float4x4", Float4x4 }
    };
    const auto it = m.find(name);
    return it != m.end() ? it->second : Unknown;
}

inline size_t ShaderUniformBlock::GetTypeSize(UniformType type)
{
    static constexpr size_t ret[] =
    {
        4, 8, 12, 16,
        4, 8, 12, 16,
        4, 8, 12, 16,
        64,
        0
    };
    return ret[std::to_underlying(type)];
}

RTRC_END
