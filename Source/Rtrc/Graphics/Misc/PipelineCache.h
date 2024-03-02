#pragma once

#include <ankerl/unordered_dense.h>

#include <Rtrc/Core/Hash.h>
#include <Rtrc/Graphics/Device/Device.h>
#include <Rtrc/Graphics/Misc/PipelineStateCacheCommon.h>
#include <Rtrc/RHI/RHIDeclaration.h>

RTRC_BEGIN

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
    RHI::StencilOps frontStencil;
    RHI::StencilOps backStencil;

    auto operator<=>(const DepthStencilStateDesc &) const = default;
};

struct RasterizerStateDesc
{
    RHI::PrimitiveTopology primitiveTopology = RHI::PrimitiveTopology::TriangleList;
    RHI::FillMode          fillMode          = RHI::FillMode::Fill;
    RHI::CullMode          cullMode          = RHI::CullMode::DontCull;
    RHI::FrontFaceMode     frontFaceMode     = RHI::FrontFaceMode::Clockwise;

    auto operator<=>(const RasterizerStateDesc &) const = default;
};

struct BlendStateDesc
{
    bool             enableBlending = false;
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

struct PipelineMiscStateDesc
{
    float depthBiasConstFactor = 0.0f;
    float depthBiasSlopeFactor = 0.0f;
    float depthBiasClampValue = 0.0f;

    auto operator<=>(const PipelineMiscStateDesc &) const = default;

    size_t Hash() const { return ::Rtrc::Hash(depthBiasConstFactor, depthBiasSlopeFactor, depthBiasClampValue); }
};

#define DEFINE_PIPELINE_STATE(STATE)                            \
    using STATE        = PipelineStateCacheKey<STATE##Desc>;    \
    using STATE##Cache = PipelineStateCache<STATE##Desc>;       \
    class STATE##Builder                                        \
    {                                                           \
    public:                                                     \
        STATE operator+(const STATE##Desc &desc) const          \
        {                                                       \
            return STATE##Cache::GetInstance().Register(desc);  \
        }                                                       \
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
    mutable Shader::UniqueId shaderId; // Reserved for caching. Don't use it.

    RHI::Viewports viewports = 1;
    RHI::Scissors  scissors = 1;

    const MeshLayout *meshLayout = nullptr;

    DepthStencilState depthStencilState;
    PipelineMiscStateDesc miscStateDesc;

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
            miscStateDesc == rhs.miscStateDesc &&
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
            miscStateDesc,
            rasterizerState,
            blendState,
            attachmentState);
    }
};

class GraphicsPipelineCache
{
public:

    using Key = GraphicsPipelineDesc;

    void SetDevice(Ref<Device> device)
    {
        this->SetPipelineManager(device->GetPipelineManager());
    }

    void SetPipelineManager(Ref<PipelineManager> pipelineManager)
    {
        pipelineManager_ = pipelineManager;
        pipelines_.clear();
    }

    RC<GraphicsPipeline> Get(const Key &key)
    {
        assert(pipelineManager_);

        {
            std::shared_lock lock(mutex_);
            if(auto it = pipelines_.find(key); it != pipelines_.end())
            {
                return it->second;
            }
        }

        std::unique_lock lock(mutex_);
        if(auto it = pipelines_.find(key); it != pipelines_.end())
        {
            return it->second;
        }

        auto &rasterizerStateDesc   = key.rasterizerState.GetDesc();
        auto &depthStencilStateDesc = key.depthStencilState.GetDesc();
        auto &blendStateDesc        = key.blendState.GetDesc();
        auto &attachmentStateDesc   = key.attachmentState.GetDesc();

        auto pipeline = pipelineManager_->CreateGraphicsPipeline(GraphicsPipeline::Desc
        {
            .shader                 = key.shader,
            .viewports              = key.viewports,
            .scissors               = key.scissors,
            .meshLayout             = key.meshLayout,
            .primitiveTopology      = rasterizerStateDesc.primitiveTopology,
            .fillMode               = rasterizerStateDesc.fillMode,
            .cullMode               = rasterizerStateDesc.cullMode,
            .frontFaceMode          = rasterizerStateDesc.frontFaceMode,
            .enableDepthBias        = depthStencilStateDesc.enableDepthBias,
            .depthBiasConstFactor   = key.miscStateDesc.depthBiasConstFactor,
            .depthBiasSlopeFactor   = key.miscStateDesc.depthBiasSlopeFactor,
            .depthBiasClampValue    = key.miscStateDesc.depthBiasClampValue,
            .enableDepthClip        = depthStencilStateDesc.enableDepthClip,
            .multisampleCount       = attachmentStateDesc.multisampleCount,
            .enableDepthTest        = depthStencilStateDesc.enableDepthTest,
            .enableDepthWrite       = depthStencilStateDesc.enableDepthWrite,
            .depthCompareOp         = depthStencilStateDesc.depthCompareOp,
            .enableStencilTest      = depthStencilStateDesc.enableStencilTest,
            .stencilReadMask        = depthStencilStateDesc.stencilReadMask,
            .stencilWriteMask       = depthStencilStateDesc.stencilWriteMask,
            .frontStencil           = depthStencilStateDesc.frontStencil,
            .backStencil            = depthStencilStateDesc.backStencil,
            .enableBlending         = blendStateDesc.enableBlending,
            .blendingSrcColorFactor = blendStateDesc.blendingSrcColorFactor,
            .blendingDstColorFactor = blendStateDesc.blendingDstColorFactor,
            .blendingColorOp        = blendStateDesc.blendingColorOp,
            .blendingSrcAlphaFactor = blendStateDesc.blendingSrcAlphaFactor,
            .blendingDstAlphaFactor = blendStateDesc.blendingDstAlphaFactor,
            .blendingAlphaOp        = blendStateDesc.blendingAlphaOp,
            .colorAttachmentFormats = attachmentStateDesc.colorAttachmentFormats,
            .depthStencilFormat     = attachmentStateDesc.depthStencilFormat,
        });

        auto it = pipelines_.insert({ key, pipeline }).first;
        it->first.shader.reset();
        it->first.shaderId = key.shader->GetUniqueID();
        return it->second;
    }

private:

    using HashMap = ankerl::unordered_dense::map<Key, RC<GraphicsPipeline>, HashOperator<Key>>;

    Ref<PipelineManager> pipelineManager_;
    std::shared_mutex    mutex_;
    HashMap              pipelines_;
};

RTRC_END
