#pragma once

#include <Rtrc/Graphics/Device/GeneralGPUObject.h>
#include <Rtrc/Graphics/Device/PipelineDesc.h>
#include <Rtrc/Graphics/Shader/Shader.h>

RTRC_BEGIN

class MeshLayout;

class GraphicsPipeline : public GeneralGPUObject<RHI::GraphicsPipelineUPtr>
{
public:
    
    const RC<const ShaderInfo> &GetShaderInfo() const;

private:

    friend class PipelineManager;

    RC<const ShaderInfo> shaderInfo_;
};

class ComputePipeline : public GeneralGPUObject<RHI::ComputePipelineUPtr>
{
public:

    const RC<const ShaderInfo> &GetShaderInfo() const;

    friend class PipelineManager;

private:

    RC<const ShaderInfo> shaderInfo_;
};

class RayTracingLibrary : public Uncopyable
{
public:

    struct Desc
    {
        RC<Shader>                              shader;
        std::vector<RHI::RayTracingShaderGroup> shaderGroups;
        uint32_t                                maxRayPayloadSize;
        uint32_t                                maxRayHitAttributeSize;
        uint32_t                                maxRecursiveDepth;
    };

    uint32_t GetShaderGroupCount() const;

private:

    friend class PipelineManager;

    RC<ShaderBindingLayoutInfo> bindingLayoutInfo_;
    uint32_t                    shaderGroupCount_ = 0;
    RHI::RayTracingLibraryUPtr  library_;
};

class RayTracingPipeline : public GeneralGPUObject<RHI::RayTracingPipelineUPtr>
{
public:

    struct Desc
    {
        std::vector<RC<Shader>>                 shaders;
        std::vector<RHI::RayTracingShaderGroup> shaderGroups;
        std::vector<RC<RayTracingLibrary>>      libraries;

        RC<BindingLayout> bindingLayout;

        uint32_t maxRayPayloadSize;
        uint32_t maxRayHitAttributeSize;
        uint32_t maxRecursiveDepth;
        bool     useCustomStackSize = false;
    };
    
    void GetShaderGroupHandles(
        uint32_t               startGroupIndex,
        uint32_t               groupCount,
        MutSpan<unsigned char> outputData) const;

    uint32_t GetShaderGroupCount() const;
    uint32_t GetShaderGroupHandleSize() const;
    
    const ShaderBindingLayoutInfo &GetBindingLayoutInfo() const;

private:

    friend class PipelineManager;

    RC<ShaderBindingLayoutInfo> bindingLayoutInfo_;
    uint32_t                    shaderGroupCount_ = 0;
};

class WorkGraphPipeline : public GeneralGPUObject<RHI::WorkGraphPipelineUPtr>
{
public:

    struct Desc
    {
        std::vector<RC<Shader>> shaders;
        std::vector<WorkGraphEntryPoint> entryNodes;
    };

    const ShaderBindingLayoutInfo &GetBindingLayoutInfo() const;

    const RHI::WorkGraphMemoryRequirements &GetMemoryRequirements() const;

private:

    friend class PipelineManager;

    RC<const ShaderBindingLayoutInfo> bindingLayoutInfo_;
};

class PipelineManager : public GeneralGPUObjectManager
{
public:

    PipelineManager(RHI::DeviceOPtr device, DeviceSynchronizer &sync);

    RC<RayTracingLibrary> CreateRayTracingLibrary(const RayTracingLibrary::Desc &desc);

    RC<GraphicsPipeline>   CreateGraphicsPipeline  (const GraphicsPipelineDesc &desc);
    RC<ComputePipeline>    CreateComputePipeline   (const RC<Shader> &shader);
    RC<RayTracingPipeline> CreateRayTracingPipeline(const RayTracingPipeline::Desc &desc);
    RC<WorkGraphPipeline>  CreateWorkGraphPipeline (const WorkGraphPipeline::Desc &desc);

    const RHI::DeviceOPtr &_internalGetRHIDevice() const;

private:

    RHI::DeviceOPtr device_;
};

inline const RC<const ShaderInfo> &GraphicsPipeline::GetShaderInfo() const
{
    return shaderInfo_;
}

inline const RC<const ShaderInfo> &ComputePipeline::GetShaderInfo() const
{
    return shaderInfo_;
}

inline uint32_t RayTracingLibrary::GetShaderGroupCount() const
{
    return shaderGroupCount_;
}

inline const ShaderBindingLayoutInfo &WorkGraphPipeline::GetBindingLayoutInfo() const
{
    return *bindingLayoutInfo_;
}

inline const RHI::WorkGraphMemoryRequirements &WorkGraphPipeline::GetMemoryRequirements() const
{
    return rhiObject_->GetMemoryRequirements();
}

inline const ShaderBindingLayoutInfo &RayTracingPipeline::GetBindingLayoutInfo() const
{
    return *bindingLayoutInfo_;
}

RTRC_END
