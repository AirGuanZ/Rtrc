#pragma once

#include <Rtrc/Graphics/Device/MeshLayout.h>
#include <Rtrc/Graphics/Device/PipelineDescKey.h>
#include <Rtrc/Graphics/Shader/Shader.h>

RTRC_BEGIN

/*
    Full description of a graphics pipeline object is too large for efficient comparing and hashing, thus segmented into
    several components:
    - DepthStencilStateDesc: Captures depth-stencil state, excluding depth bias constants.
    - RasterizerStateDesc: Defines the rasterization state.
    - BlendStateDesc: Contains blend state configurations.
    - AttachmentStateDesc: Describes state related to framebuffer attachments.
    - Others: shader, viewports, scissors, mesh layout, and depth bias constants.
    The first three components (DepthStencilStateDesc, RasterizerStateDesc, BlendStateDesc) are often pre-hashed to
    enhance cache performance.
*/

struct DepthStencilStateDesc
{
    // depth bias const/slope/clamp factors are not included here

    bool enableDepthClip = true;
    bool enableDepthBias = false;

    bool           enableDepthTest  = false;
    bool           enableDepthWrite = false;
    RHI::CompareOp depthCompareOp   = RHI::CompareOp::Always;

    bool            enableStencilTest = false;
    uint8_t         stencilReadMask   = 0;
    uint8_t         stencilWriteMask  = 0;
    RHI::StencilOps frontStencil      = {};
    RHI::StencilOps backStencil       = {};

    auto operator<=>(const DepthStencilStateDesc &) const = default;
};

struct RasterizerStateDesc
{
    RHI::PrimitiveTopology primitiveTopology         = RHI::PrimitiveTopology::TriangleList;
    RHI::FillMode          fillMode                  = RHI::FillMode::Fill;
    RHI::CullMode          cullMode                  = RHI::CullMode::DontCull;
    RHI::FrontFaceMode     frontFaceMode             = RHI::FrontFaceMode::Clockwise;
    bool                   conservativeRasterization = false;

    auto operator<=>(const RasterizerStateDesc &) const = default;
};

struct BlendStateDesc
{
    bool             enableBlending         = false;
    RHI::BlendFactor blendingSrcColorFactor = RHI::BlendFactor::One;
    RHI::BlendFactor blendingDstColorFactor = RHI::BlendFactor::Zero;
    RHI::BlendOp     blendingColorOp        = RHI::BlendOp::Add;
    RHI::BlendFactor blendingSrcAlphaFactor = RHI::BlendFactor::One;
    RHI::BlendFactor blendingDstAlphaFactor = RHI::BlendFactor::Zero;
    RHI::BlendOp     blendingAlphaOp        = RHI::BlendOp::Add;

    auto operator<=>(const BlendStateDesc &) const = default;
};

struct AttachmentStateDesc
{
    int                          multisampleCount = 1;
    StaticVector<RHI::Format, 8> colorAttachmentFormats;
    RHI::Format                  depthStencilFormat = RHI::Format::Unknown;

    auto operator<=>(const AttachmentStateDesc &) const = default;
};

DEFINE_PIPELINE_STATE(DepthStencilState)
DEFINE_PIPELINE_STATE(RasterizerState)
DEFINE_PIPELINE_STATE(BlendState)
DEFINE_PIPELINE_STATE(AttachmentState)

#define RTRC_DEPTH_STENCIL_STATE ::Rtrc::DepthStencilStateBuilder()+Rtrc::DepthStencilStateDesc
#define RTRC_RASTERIZER_STATE    ::Rtrc::RasterizerStateBuilder()+Rtrc::RasterizerStateDesc
#define RTRC_BLEND_STATE         ::Rtrc::BlendStateBuilder()+Rtrc::BlendStateDesc
#define RTRC_ATTACHMENT_STATE    ::Rtrc::AttachmentStateBuilder()+Rtrc::AttachmentStateDesc

#define RTRC_DEFINE_STATIC_DEPTH_STENCIL_STATE(NAME) static const auto NAME = RTRC_DEPTH_STENCIL_STATE
#define RTRC_DEFINE_STATIC_RASTERIZER_STATE(NAME)    static const auto NAME = RTRC_RASTERIZER_STATE
#define RTRC_DEFINE_STATIC_BLEND_STATE(NAME)         static const auto NAME = RTRC_BLEND_STATE
#define RTRC_DEFINE_STATIC_ATTACHMENT_STATE(NAME)    static const auto NAME = RTRC_ATTACHMENT_STATE

#define RTRC_STATIC_DEPTH_STENCIL_STATE(...) \
    ([](){ RTRC_DEFINE_STATIC_DEPTH_STENCIL_STATE(state) __VA_ARGS__ ; return state; }())
#define RTRC_STATIC_RASTERIZER_STATE(...) \
    ([](){ RTRC_DEFINE_STATIC_RASTERIZER_STATE(state) __VA_ARGS__ ; return state; }())
#define RTRC_STATIC_BLEND_STATE(...) \
    ([](){ RTRC_DEFINE_STATIC_BLEND_STATE(state) __VA_ARGS__ ; return state; }())
#define RTRC_STATIC_ATTACHMENT_STATE(...) \
    ([](){ RTRC_DEFINE_STATIC_ATTACHMENT_STATE(state) __VA_ARGS__ ; return state; }())

struct GraphicsPipelineDesc
{
    mutable RC<Shader>       shader;
    mutable Shader::UniqueId shaderId = {}; // Reserved for caching

    RHI::Viewports viewports = 1;
    RHI::Scissors  scissors = 1;

    const MeshLayout *meshLayout = nullptr;

    DepthStencilState depthStencilState;
    float depthBiasConstFactor = 0.0f;
    float depthBiasSlopeFactor = 0.0f;
    float depthBiasClampValue = 0.0f;

    RasterizerState rasterizerState;
    BlendState      blendState;
    AttachmentState attachmentState;

    Shader::UniqueId GetShaderUniqueId() const
    {
        return shader ? shader->GetUniqueID() : shaderId;
    }

    bool operator==(const GraphicsPipelineDesc &rhs) const
    {
        return
            GetShaderUniqueId() == rhs.GetShaderUniqueId() &&
            viewports == rhs.viewports &&
            scissors == rhs.scissors &&
            meshLayout == rhs.meshLayout &&
            depthStencilState == rhs.depthStencilState &&
            depthBiasConstFactor == rhs.depthBiasConstFactor &&
            depthBiasSlopeFactor == rhs.depthBiasSlopeFactor &&
            depthBiasClampValue == rhs.depthBiasClampValue &&
            rasterizerState == rhs.rasterizerState &&
            blendState == rhs.blendState &&
            attachmentState == rhs.attachmentState;
    }

    size_t Hash() const
    {
        return ::Rtrc::Hash(
            GetShaderUniqueId(),
            viewports,
            scissors,
            meshLayout,
            depthStencilState,
            depthBiasConstFactor,
            depthBiasSlopeFactor,
            depthBiasClampValue,
            rasterizerState,
            blendState,
            attachmentState);
    }
};

RTRC_END
