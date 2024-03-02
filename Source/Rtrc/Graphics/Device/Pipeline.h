#pragma once

#include <Rtrc/Graphics/Device/GeneralGPUObject.h>
#include <Rtrc/Graphics/Shader/Shader.h>

RTRC_BEGIN

class MeshLayout;

class GraphicsPipeline : public GeneralGPUObject<RHI::GraphicsPipelineUPtr>
{
public:
    
    struct Desc
    {
        RC<Shader> shader;

        RHI::Viewports viewports = 1;
        RHI::Scissors  scissors  = 1;

        const MeshLayout *meshLayout = nullptr;

        RHI::PrimitiveTopology primitiveTopology = RHI::PrimitiveTopology::TriangleList;
        RHI::FillMode          fillMode          = RHI::FillMode::Fill;
        RHI::CullMode          cullMode          = RHI::CullMode::DontCull;
        RHI::FrontFaceMode     frontFaceMode     = RHI::FrontFaceMode::Clockwise;

        bool  enableDepthBias      = false;
        float depthBiasConstFactor = 0.0f;
        float depthBiasSlopeFactor = 0.0f;
        float depthBiasClampValue  = 0.0f;

        bool enableDepthClip = true;

        int multisampleCount = 1;

        bool           enableDepthTest  = false;
        bool           enableDepthWrite = false;
        RHI::CompareOp depthCompareOp   = RHI::CompareOp::Always;

        bool            enableStencilTest = false;
        uint8_t         stencilReadMask = 0;
        uint8_t         stencilWriteMask = 0;
        RHI::StencilOps frontStencil;
        RHI::StencilOps backStencil;

        bool             enableBlending = false;
        RHI::BlendFactor blendingSrcColorFactor = RHI::BlendFactor::One;
        RHI::BlendFactor blendingDstColorFactor = RHI::BlendFactor::Zero;
        RHI::BlendOp     blendingColorOp        = RHI::BlendOp::Add;
        RHI::BlendFactor blendingSrcAlphaFactor = RHI::BlendFactor::One;
        RHI::BlendFactor blendingDstAlphaFactor = RHI::BlendFactor::Zero;
        RHI::BlendOp     blendingAlphaOp        = RHI::BlendOp::Add;

        StaticVector<RHI::Format, 8> colorAttachmentFormats;
        RHI::Format                  depthStencilFormat = RHI::Format::Unknown;

        void Validate() const;

        bool operator==(const Desc &) const = default;
    };

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

class PipelineManager : public GeneralGPUObjectManager
{
public:

    PipelineManager(RHI::DeviceOPtr device, DeviceSynchronizer &sync);

    RC<RayTracingLibrary> CreateRayTracingLibrary(const RayTracingLibrary::Desc &desc);

    RC<GraphicsPipeline>   CreateGraphicsPipeline  (const GraphicsPipeline::Desc &desc);
    RC<ComputePipeline>    CreateComputePipeline   (const RC<Shader> &shader);
    RC<RayTracingPipeline> CreateRayTracingPipeline(const RayTracingPipeline::Desc &desc);

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

RTRC_END
