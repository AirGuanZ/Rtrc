#pragma once

#include <Rtrc/Graphics/Object/GeneralGPUObject.h>
#include <Rtrc/Graphics/Shader/Shader.h>
#include <Rtrc/Utils/SharedObjectPool.h>

RTRC_BEGIN

class BindingLayout;
class MeshLayout;
class Shader;

class GraphicsPipeline : public GeneralGPUObject<RHI::GraphicsPipelinePtr>
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

private:

    friend class PipelineManager;

    Desc desc_;
};

class ComputePipeline : public GeneralGPUObject<RHI::ComputePipelinePtr>
{
    friend class PipelineManager;
};

class PipelineManager
{
public:

    explicit PipelineManager(GeneralGPUObjectManager &objectManager);

    RC<GraphicsPipeline> CreateGraphicsPipeline(const GraphicsPipeline::Desc &desc);

    RC<ComputePipeline> CreateComputePipeline(const RC<Shader> &shader);

    void GC();

private:

    GeneralGPUObjectManager &objectManager_;
    SharedObjectPool<GraphicsPipeline::DescKey, GraphicsPipeline, true, true> graphicsPipelinePool_;
    SharedObjectPool<Shader::UniqueID, ComputePipeline, true, true> computePipelinePool_;
};

RTRC_END

namespace std
{

    template<>
    struct hash<Rtrc::GraphicsPipeline::DescKey>
    {
        size_t operator()(const Rtrc::GraphicsPipeline::DescKey &key) const noexcept
        {
            return key.Hash();
        }
    };

} // namespace std