#pragma once

#include <Rtrc/RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

class VulkanGraphicsPipeline : public GraphicsPipeline
{
public:

    VulkanGraphicsPipeline(Ptr<BindingLayout> layout, VkDevice device, VkPipeline pipeline);

    ~VulkanGraphicsPipeline() override;

    const Ptr<BindingLayout> &GetBindingLayout() const override;

    VkPipeline GetNativePipeline() const;

private:

    Ptr<BindingLayout> layout_;

    VkDevice   device_;
    VkPipeline pipeline_;
};

class VulkanGraphicsPipelineBuilder : public GraphicsPipelineBuilder
{
public:

    explicit VulkanGraphicsPipelineBuilder(VkDevice device);

    GraphicsPipelineBuilder &SetVertexShader(Ptr<RawShader> vertexShader) override;

    GraphicsPipelineBuilder &SetFragmentShader(Ptr<RawShader> fragmentShader) override;

    GraphicsPipelineBuilder &SetBindingLayout(Ptr<BindingLayout> layout) override;

    GraphicsPipelineBuilder &SetViewports(const Viewports &viewports) override;

    GraphicsPipelineBuilder &SetScissors(const Scissors &scissors) override;

    GraphicsPipelineBuilder &SetPrimitiveTopology(PrimitiveTopology topology) override;

    GraphicsPipelineBuilder &SetFillMode(FillMode mode) override;

    GraphicsPipelineBuilder &SetCullMode(CullMode mode) override;

    GraphicsPipelineBuilder &SetFrontFace(FrontFaceMode mode) override;

    GraphicsPipelineBuilder &SetDepthBias(float constFactor, float slopeFactor, float clamp) override;

    GraphicsPipelineBuilder &SetMultisample(int sampleCount) override;

    GraphicsPipelineBuilder &SetDepthTest(bool enableTest, bool enableWrite, CompareOp compareOp) override;

    GraphicsPipelineBuilder &SetStencilTest(bool enableTest) override;

    GraphicsPipelineBuilder &SetStencilFrontOp(
        StencilOp depthFailOp,
        StencilOp failOp,
        StencilOp passOp,
        CompareOp compareOp,
        uint32_t  compareMask,
        uint32_t  writeMask) override;

    GraphicsPipelineBuilder &SetStencilBackOp(
        StencilOp depthFailOp,
        StencilOp failOp,
        StencilOp passOp,
        CompareOp compareOp,
        uint32_t  compareMask,
        uint32_t  writeMask) override;

    GraphicsPipelineBuilder &SetBlending(
        bool        enableBlending,
        BlendFactor srcColorFactor,
        BlendFactor dstColorFactor,
        BlendOp     colorOp,
        BlendFactor srcAlphaFactor,
        BlendFactor dstAlphaFactor,
        BlendOp     alphaOp) override;

    GraphicsPipelineBuilder &AddColorAttachment(Format format) override;

    GraphicsPipelineBuilder &SetDepthStencilAttachment(Format format) override;

    Ptr<GraphicsPipeline> CreatePipeline() const override;

private:

    struct StencilOps
    {
        StencilOp depthFailOp = StencilOp::Keep;
        StencilOp failOp      = StencilOp::Keep;
        StencilOp passOp      = StencilOp::Keep;
        CompareOp compareOp   = CompareOp::Always;
        uint32_t  compareMask = 0xff;
        uint32_t  writeMask   = 0xff;
    };

    VkDevice device_;

    Ptr<RawShader> vertexShader_;
    Ptr<RawShader> fragmentShader_;

    Ptr<BindingLayout> bindingLayout_;

    Viewports viewports_;
    Scissors scissors_;

    PrimitiveTopology primitiveTopology_ = PrimitiveTopology::TriangleList;
    FillMode          fillMode_          = FillMode::Fill;
    CullMode          cullMode_          = CullMode::DontCull;
    FrontFaceMode     frontFaceMode_     = FrontFaceMode::Clockwise;

    bool enableDepthBias_       = false;
    float depthBiasConstFactor_ = 0;
    float depthBiasSlopeFactor_ = 0;
    float depthBiasClampValue_  = 0;

    int multisampleCount_ = 1;

    bool      enableDepthTest_  = false;
    bool      enableDepthWrite_ = false;
    CompareOp depthCompareOp_   = CompareOp::Always;

    bool       enableStencilTest_ = false;
    StencilOps frontStencilOp_;
    StencilOps backStencilOp_;

    bool        enableBlending_         = false;
    BlendFactor blendingSrcColorFactor_ = BlendFactor::One;
    BlendFactor blendingDstColorFactor_ = BlendFactor::Zero;
    BlendFactor blendingSrcAlphaFactor_ = BlendFactor::One;
    BlendFactor blendingDstAlphaFactor_ = BlendFactor::Zero;
    BlendOp     blendingColorOp_        = BlendOp::Add;
    BlendOp     blendingAlphaOp_        = BlendOp::Add;

    std::vector<Format> colorAttachments_;
    Format              depthStencilFormat_ = Format::Unknown;
};

RTRC_RHI_VK_END
