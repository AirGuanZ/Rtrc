#pragma once

#include <Graphics/Device/GeneralGPUObject.h>
#include <Graphics/Shader/Shader.h>
/*#include <Core/ObjectCache.h>*/

RTRC_BEGIN

class MeshLayout;

class GraphicsPipeline : public GeneralGPUObject<RHI::GraphicsPipelineUPtr>
{
public:
    
    using StencilOp  = RHI::StencilOp;
    using StencilOps = RHI::GraphicsPipelineDesc::StencilOps;

    using Viewport  = RHI::Viewport;
    using Viewports = RHI::Viewports;

    using Scissor  = RHI::Scissor;
    using Scissors = RHI::Scissors;

    using PrimitiveTopology = RHI::PrimitiveTopology;
    using FillMode          = RHI::FillMode;
    using CullMode          = RHI::CullMode;
    using FrontFaceMode     = RHI::FrontFaceMode;

    using CompareOp = RHI::CompareOp;

    using BlendFactor = RHI::BlendFactor;
    using BlendOp     = RHI::BlendOp;

    using Format = RHI::Format;
    
    struct Desc
    {
        RC<Shader>       shader;
        Shader::UniqueId shaderId; // Reserved for caching. Don't use it.

        Viewports viewports = 1;
        Scissors  scissors  = 1;

        const MeshLayout *meshLayout = nullptr;

        PrimitiveTopology primitiveTopology = PrimitiveTopology::TriangleList;
        FillMode          fillMode          = FillMode::Fill;
        CullMode          cullMode          = CullMode::DontCull;
        FrontFaceMode     frontFaceMode     = FrontFaceMode::Clockwise;

        bool  enableDepthBias      = false;
        float depthBiasConstFactor = 0.0f;
        float depthBiasSlopeFactor = 0.0f;
        float depthBiasClampValue  = 0.0f;

        int multisampleCount = 1;

        bool      enableDepthTest  = false;
        bool      enableDepthWrite = false;
        CompareOp depthCompareOp   = CompareOp::Always;

        bool       enableStencilTest = false;
        uint8_t    stencilReadMask = 0;
        uint8_t    stencilWriteMask = 0;
        StencilOps frontStencil;
        StencilOps backStencil;

        bool        enableBlending = false;
        BlendFactor blendingSrcColorFactor = BlendFactor::One;
        BlendFactor blendingDstColorFactor = BlendFactor::Zero;
        BlendOp     blendingColorOp        = BlendOp::Add;
        BlendFactor blendingSrcAlphaFactor = BlendFactor::One;
        BlendFactor blendingDstAlphaFactor = BlendFactor::Zero;
        BlendOp     blendingAlphaOp        = BlendOp::Add;

        StaticVector<Format, 8> colorAttachmentFormats;
        Format                  depthStencilFormat = RHI::Format::Unknown;

        void Validate() const;

        size_t Hash() const;

        bool operator==(const Desc &) const = default;
    };

    const RC<const ShaderInfo> &GetShaderInfo() const;

private:

    friend class PipelineManager;

    Desc desc_;
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
