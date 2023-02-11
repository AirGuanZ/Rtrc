#pragma once

#include <Rtrc/Graphics/Device/GeneralGPUObject.h>
#include <Rtrc/Graphics/Shader/Shader.h>
/*#include <Rtrc/Utility/ObjectCache.h>*/

RTRC_BEGIN

class MeshLayout;

class GraphicsPipeline : public GeneralGPUObject<RHI::GraphicsPipelinePtr>/*, public InObjectCache*/
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

    struct DescKey
    {
        Shader::UniqueID shader;
        
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
        uint8_t    stencilReadMask   = 0xff;
        uint8_t    stencilWriteMask  = 0xff;
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

        size_t Hash() const;

        bool operator==(const DescKey &other) const noexcept = default;
    };

    struct Desc
    {
        RC<Shader> shader;

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

        DescKey AsKey() const;
    };

    const RC<const ShaderInfo> &GetShaderInfo() const;

private:

    friend class PipelineManager;

    Desc desc_;
    RC<const ShaderInfo> shaderInfo_;
};

class ComputePipeline : public GeneralGPUObject<RHI::ComputePipelinePtr>/*, public InObjectCache*/
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

private:

    friend class PipelineManager;

    RHI::RayTracingLibraryPtr library_;
};

class RayTracingPipeline : public GeneralGPUObject<RHI::RayTracingPipelinePtr>
{
public:

    struct Desc
    {
        std::vector<RC<Shader>>                 shaders;
        std::vector<RHI::RayTracingShaderGroup> shaderGroups;
        std::vector<RC<RayTracingLibrary>>      libraries;

        RC<BindingLayout> bindingLayout;

        uint32_t maxRayPayloadSize;      // Required when libraries is not empty
        uint32_t maxRayHitAttributeSize; // Required when libraries is not empty
        uint32_t maxRecursiveDepth;
        bool     useCustomStackSize = false;
    };

private:

    friend class PipelineManager;
};

class PipelineManager : public GeneralGPUObjectManager
{
public:

    PipelineManager(RHI::DevicePtr device, DeviceSynchronizer &sync);

    RC<RayTracingLibrary> CreateRayTracingLibrary(const RayTracingLibrary::Desc &desc);

    RC<GraphicsPipeline>   CreateGraphicsPipeline  (const GraphicsPipeline::Desc &desc);
    RC<ComputePipeline>    CreateComputePipeline   (const RC<Shader> &shader);
    RC<RayTracingPipeline> CreateRayTracingPipeline(const RayTracingPipeline::Desc &desc);

private:

    RHI::DevicePtr device_;
    //ObjectCache<GraphicsPipeline::DescKey, GraphicsPipeline, true, true> graphicsPipelineCache_;
    //ObjectCache<Shader::UniqueID, ComputePipeline, true, true> computePipelineCache_;
};

inline const RC<const ShaderInfo> &GraphicsPipeline::GetShaderInfo() const
{
    return shaderInfo_;
}

inline const RC<const ShaderInfo> &ComputePipeline::GetShaderInfo() const
{
    return shaderInfo_;
}

RTRC_END
