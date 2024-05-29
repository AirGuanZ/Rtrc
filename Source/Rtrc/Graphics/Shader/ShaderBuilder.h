#pragma once

#include <Rtrc/Graphics/Shader/Shader.h>

RTRC_BEGIN

class Device;

class ShaderBuilder
{
public:

    struct Desc
    {
        ShaderCategory category;

        // ==================== Raw shader ====================

        RHI::RawShaderUPtr vertexShader;
        RHI::RawShaderUPtr fragmentShader;
        RHI::RawShaderUPtr computeShader;
        RHI::RawShaderUPtr rayTracingShader;
        RHI::RawShaderUPtr meshShader;
        RHI::RawShaderUPtr taskShader;

        // ==================== Input attributes ====================

        std::vector<ShaderIOVar> VSInput;

        // ==================== Bindings ====================

        std::map<std::string, int, std::less<>> nameToBindingGroupLayoutIndex;
        std::vector<RC<BindingGroupLayout>>     bindingGroupLayouts;
        std::vector<std::string>                bindingGroupNames;
        RC<BindingLayout>                       bindingLayout;
        ShaderBindingNameMap                    bindingNameMap;
        int                                     builtinBindingGroupIndices[EnumCount<Shader::BuiltinBindingGroup>];
        RC<BindingGroup>                        bindingGroupForInlineSamplers; // Should be the last group, if present

        // ==================== Push constant ====================

        std::vector<Shader::PushConstantRange> pushConstantRanges;

        // ==================== Compute shader ====================

        Vector3u computeShaderThreadGroupSize;

        // ==================== Ray tracing shader group ====================

        std::vector<RHI::RayTracingRayGenShaderGroup> rayGenShaderGroups;
        std::vector<RHI::RayTracingMissShaderGroup>   missShaderGroups;
        std::vector<RHI::RayTracingHitShaderGroup>    hitShaderGroups;
    };

    static RC<Shader> BuildShader(Ref<Device> device, Desc desc);
};

RTRC_END
