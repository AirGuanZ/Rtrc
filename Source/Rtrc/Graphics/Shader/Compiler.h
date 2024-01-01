#pragma once

#include <Rtrc/Core/Serialization/Serialize.h>
#include <Rtrc/Core/SmartPointer/ObserverPtr.h>
#include <Rtrc/Graphics/Device/Device.h>
#include <Rtrc/ShaderCommon/DXC/DXC.h>
#include <Rtrc/ShaderCommon/Parser/ShaderParser.h>

RTRC_BEGIN

struct CompilableShader
{
    std::string name;
    std::string source;
    std::string sourceFilename;

    std::vector<ShaderKeyword>      keywords;
    std::vector<ShaderKeywordValue> keywordValues;

    std::string vertexEntry;
    std::string fragmentEntry;
    std::string computeEntry;

    bool isRayTracingShader = false;
    std::vector<std::vector<std::string>> entryGroups;

    std::vector<ParsedBindingGroup> bindingGroups;
    std::vector<ParsedBinding>      ungroupedBindings;
    std::vector<ParsedBindingAlias> aliases;

    std::vector<RHI::SamplerDesc> inlineSamplerDescs;
    std::map<std::string, int>    inlineSamplerNameToDesc;

    std::vector<ParsedPushConstantRange> pushConstantRanges;

    RTRC_AUTO_SERIALIZE_WITH_MEMBER_COUNT_CHECK(
        name,
        source,
        sourceFilename,
        keywords,
        keywordValues,
        vertexEntry,
        fragmentEntry,
        computeEntry,
        isRayTracingShader,
        entryGroups,
        bindingGroups,
        ungroupedBindings,
        aliases,
        inlineSamplerDescs,
        inlineSamplerNameToDesc,
        pushConstantRanges);
};

// TODO: support rtrc_group_struct
class ShaderCompiler : public Uncopyable
{
public:
    
    void SetDevice(Ref<Device> device);
    
    RC<Shader> Compile(
        const ShaderCompileEnvironment &envir,
        const CompilableShader         &shader,
        bool                            debug = RTRC_DEBUG) const;

    DXC &GetDXC() { return dxc_; }

private:

    enum class D3D12RegisterType
    {
        B, // Constant buffer
        S, // Sampler
        T, // SRV
        U, // UAV
    };

    enum class CompileStage
    {
        VS,
        FS,
        CS,
        RT
    };

    static RHI::ShaderStageFlags GetFullShaderStages(ShaderInfo::Category category);
    
    static D3D12RegisterType BindingTypeToD3D12RegisterType(RHI::BindingType type);

    static std::string GetArraySpecifier(std::optional<uint32_t> arraySize);

    void DoCompilation(
        const DXC::ShaderInfo      &shaderInfo,
        CompileStage                stage,
        bool                        debug,
        std::vector<unsigned char> &outData,
        Box<ShaderReflection>      &outRefl) const;

    void EnsureAllUsedBindingsAreGrouped(
        const CompilableShader      &shader,
        const Box<ShaderReflection> &refl,
        std::string_view             stage) const;

    DXC                 dxc_;
    Ref<Device> device_;
};

RTRC_END
