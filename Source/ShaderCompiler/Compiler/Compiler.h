#pragma once

#include <Core/Serialization/Serialize.h>
#include <Core/SmartPointer/ObserverPtr.h>
#include <Graphics/Device/Device.h>
#include <ShaderCompiler/DXC/DXC.h>
#include <ShaderCompiler/Parser/ShaderParser.h>

RTRC_SHADER_COMPILER_BEGIN

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

class Compiler : public Uncopyable
{
public:
    
    void SetDevice(ObserverPtr<Device> device);
    
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
    ObserverPtr<Device> device_;
};

RTRC_SHADER_COMPILER_END
