#pragma once

#include <Rtrc/RHI/Vulkan/Pipeline/BindingLayout.h>
#include <Rtrc/RHI/Vulkan/Pipeline/Shader.h>

RTRC_RHI_VK_BEGIN

class VulkanPipeline : public Pipeline
{
public:

    VulkanPipeline(RC<VulkanBindingLayout> layout, VkDevice device, VkPipeline pipeline);

    ~VulkanPipeline() override;

    VkPipeline GetNativePipeline() const;

    const VulkanBindingLayout *GetLayout() const;

private:

    RC<VulkanBindingLayout> layout_;

    VkDevice   device_;
    VkPipeline pipeline_;
};

class VulkanPipelineBuilder : public PipelineBuilder
{
public:

    explicit VulkanPipelineBuilder(VkDevice device);

    PipelineBuilder &SetVertexShader(RC<RawShader> vertexShader) override;

    PipelineBuilder &SetFragmentShader(RC<RawShader> fragmentShader) override;

    PipelineBuilder &SetBindingLayout(RC<BindingLayout> layout) override;

    PipelineBuilder &SetViewports(const Viewports &viewports) override;

    PipelineBuilder &SetScissors(const Scissors &scissors) override;

    PipelineBuilder &SetPrimitiveTopology(PrimitiveTopology topology) override;

    PipelineBuilder &SetFillMode(FillMode mode) override;

    PipelineBuilder &SetCullMode(CullMode mode) override;

    PipelineBuilder &SetFrontFace(FrontFaceMode mode) override;

    PipelineBuilder &SetDepthBias(float constFactor, float slopeFactor, float clamp) override;

    PipelineBuilder &SetMultisample(int sampleCount) override;

    PipelineBuilder &SetDepthTest(bool enableTest, bool enableWrite, CompareOp compareOp) override;

    PipelineBuilder &SetStencilTest(bool enableTest) override;

    PipelineBuilder &SetStencilFrontOp(
        StencilOp depthFailOp,
        StencilOp failOp,
        StencilOp passOp,
        CompareOp compareOp,
        uint32_t  compareMask,
        uint32_t  writeMask) override;

    PipelineBuilder &SetStencilBackOp(
        StencilOp depthFailOp,
        StencilOp failOp,
        StencilOp passOp,
        CompareOp compareOp,
        uint32_t  compareMask,
        uint32_t  writeMask) override;

    PipelineBuilder &SetBlending(
        bool        enableBlending,
        BlendFactor srcColorFactor,
        BlendFactor dstColorFactor,
        BlendOp     colorOp,
        BlendFactor srcAlphaFactor,
        BlendFactor dstAlphaFactor,
        BlendOp     alphaOp) override;

    PipelineBuilder &AddColorAttachment(Format format) override;

    PipelineBuilder &SetDepthStencilAttachment(Format format) override;

    RC<Pipeline> CreatePipeline() const override;

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

    RC<VulkanShader> vertexShader_;
    RC<VulkanShader> fragmentShader_;

    RC<VulkanBindingLayout> bindingLayout_;

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
