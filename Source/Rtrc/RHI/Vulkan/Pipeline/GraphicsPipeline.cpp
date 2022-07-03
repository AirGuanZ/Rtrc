#include <Rtrc/RHI/Vulkan/Pipeline/BindingLayout.h>
#include <Rtrc/RHI/Vulkan/Pipeline/GraphicsPipeline.h>
#include <Rtrc/RHI/Vulkan/Pipeline/Shader.h>
#include <Rtrc/Utils/Enumerate.h>
#include <Rtrc/Utils/ScopeGuard.h>

RTRC_RHI_VK_BEGIN

VulkanGraphicsPipeline::VulkanGraphicsPipeline(RC<BindingLayout> layout, VkDevice device, VkPipeline pipeline)
    : layout_(std::move(layout)), device_(device), pipeline_(pipeline)
{
    
}

VulkanGraphicsPipeline::~VulkanGraphicsPipeline()
{
    assert(pipeline_);
    vkDestroyPipeline(device_, pipeline_, VK_ALLOC);
}

const RC<BindingLayout> &VulkanGraphicsPipeline::GetBindingLayout() const
{
    return layout_;
}

VkPipeline VulkanGraphicsPipeline::GetNativePipeline() const
{
    return pipeline_;
}

VulkanGraphicsPipelineBuilder::VulkanGraphicsPipelineBuilder(VkDevice device)
    : device_(device)
{
    
}

GraphicsPipelineBuilder &VulkanGraphicsPipelineBuilder::SetVertexShader(RC<RawShader> vertexShader)
{
    vertexShader_ = std::move(vertexShader);
    return *this;
}

GraphicsPipelineBuilder &VulkanGraphicsPipelineBuilder::SetFragmentShader(RC<RawShader> fragmentShader)
{
    fragmentShader_ = std::move(fragmentShader);
    return *this;
}

GraphicsPipelineBuilder &VulkanGraphicsPipelineBuilder::SetBindingLayout(RC<BindingLayout> layout)
{
    bindingLayout_ = std::move(layout);
    return *this;
}

GraphicsPipelineBuilder &VulkanGraphicsPipelineBuilder::SetViewports(const Viewports &viewports)
{
    viewports_ = viewports;
    return *this;
}

GraphicsPipelineBuilder &VulkanGraphicsPipelineBuilder::SetScissors(const Scissors &scissors)
{
    scissors_ = scissors;
    return *this;
}

GraphicsPipelineBuilder &VulkanGraphicsPipelineBuilder::SetPrimitiveTopology(PrimitiveTopology topology)
{
    primitiveTopology_ = topology;
    return *this;
}

GraphicsPipelineBuilder &VulkanGraphicsPipelineBuilder::SetFillMode(FillMode mode)
{
    fillMode_ = mode;
    return *this;
}

GraphicsPipelineBuilder &VulkanGraphicsPipelineBuilder::SetCullMode(CullMode mode)
{
    cullMode_ = mode;
    return *this;
}

GraphicsPipelineBuilder &VulkanGraphicsPipelineBuilder::SetFrontFace(FrontFaceMode mode)
{
    frontFaceMode_ = mode;
    return *this;
}

GraphicsPipelineBuilder &VulkanGraphicsPipelineBuilder::SetDepthBias(float constFactor, float slopeFactor, float clamp)
{
    enableDepthBias_ = true;
    depthBiasConstFactor_ = constFactor;
    depthBiasSlopeFactor_ = slopeFactor;
    depthBiasClampValue_ = clamp;
    return *this;
}

GraphicsPipelineBuilder &VulkanGraphicsPipelineBuilder::SetMultisample(int sampleCount)
{
    multisampleCount_ = sampleCount;
    return *this;
}

GraphicsPipelineBuilder &VulkanGraphicsPipelineBuilder::SetDepthTest(bool enableTest, bool enableWrite, CompareOp compareOp)
{
    enableDepthTest_ = enableTest;
    enableDepthWrite_ = enableWrite;
    depthCompareOp_ = compareOp;
    return *this;
}

GraphicsPipelineBuilder &VulkanGraphicsPipelineBuilder::SetStencilTest(bool enableTest)
{
    enableStencilTest_ = enableTest;
    return *this;
}

GraphicsPipelineBuilder &VulkanGraphicsPipelineBuilder::SetStencilFrontOp(
    StencilOp depthFailOp,
    StencilOp failOp,
    StencilOp passOp,
    CompareOp compareOp,
    uint32_t  compareMask,
    uint32_t  writeMask)
{
    frontStencilOp_ = StencilOps{
        .depthFailOp = depthFailOp,
        .failOp      = failOp,
        .passOp      = passOp,
        .compareOp   = compareOp,
        .compareMask = compareMask,
        .writeMask   = writeMask
    };
    return *this;
}

GraphicsPipelineBuilder &VulkanGraphicsPipelineBuilder::SetStencilBackOp(
    StencilOp depthFailOp,
    StencilOp failOp,
    StencilOp passOp,
    CompareOp compareOp,
    uint32_t  compareMask,
    uint32_t  writeMask)
{
    backStencilOp_ = StencilOps{
        .depthFailOp = depthFailOp,
        .failOp      = failOp,
        .passOp      = passOp,
        .compareOp   = compareOp,
        .compareMask = compareMask,
        .writeMask   = writeMask
    };
    return *this;
}

GraphicsPipelineBuilder &VulkanGraphicsPipelineBuilder::SetBlending(
    bool        enableBlending,
    BlendFactor srcColorFactor,
    BlendFactor dstColorFactor,
    BlendOp     colorOp,
    BlendFactor srcAlphaFactor,
    BlendFactor dstAlphaFactor,
    BlendOp     alphaOp)
{
    enableBlending_         = enableBlending;
    blendingSrcColorFactor_ = srcColorFactor;
    blendingDstColorFactor_ = dstColorFactor;
    blendingColorOp_        = colorOp;
    blendingSrcAlphaFactor_ = srcAlphaFactor;
    blendingDstAlphaFactor_ = dstAlphaFactor;
    blendingAlphaOp_        = alphaOp;
    return *this;
}

GraphicsPipelineBuilder &VulkanGraphicsPipelineBuilder::AddColorAttachment(Format format)
{
    colorAttachments_.push_back(format);
    return *this;
}

GraphicsPipelineBuilder &VulkanGraphicsPipelineBuilder::SetDepthStencilAttachment(Format format)
{
    depthStencilFormat_ = format;
    return *this;
}

RC<GraphicsPipeline> VulkanGraphicsPipelineBuilder::CreatePipeline() const
{
    assert(!viewports_.Is<std::monostate>());
    assert(!scissors_.Is<std::monostate>());

    std::vector<VkFormat> colorAttachments;
    for(auto f : colorAttachments_)
    {
        colorAttachments.push_back(TranslateTexelFormat(f));
    }

    const VkPipelineRenderingCreateInfo renderingCreateInfo = {
        .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .viewMask                = 0,
        .colorAttachmentCount    = static_cast<uint32_t>(colorAttachments.size()),
        .pColorAttachmentFormats = colorAttachments.data(),
        .depthAttachmentFormat   = TranslateTexelFormat(depthStencilFormat_),
        .stencilAttachmentFormat = TranslateTexelFormat(depthStencilFormat_)
    };

    const VkPipelineShaderStageCreateInfo stages[] = {
        static_cast<VulkanShader*>(vertexShader_.get())->GetStageCreateInfo(),
        static_cast<VulkanShader *>(fragmentShader_.get())->GetStageCreateInfo()
    };

    const VkPipelineVertexInputStateCreateInfo vertexInputState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO
    };

    const VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = TranslatePrimitiveTopology(primitiveTopology_)
    };

    VkPipelineViewportStateCreateInfo viewportState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    };

    std::vector<VkViewport> vkViewports;
    if(auto vs = viewports_.AsIf<std::vector<Viewport>>())
    {
        vkViewports.resize(vs->size());
        for(auto &&[i, v] : Enumerate(*vs))
        {
            vkViewports[i] = TranslateViewport(v);
        }
        viewportState.viewportCount = static_cast<uint32_t>(vkViewports.size());
        viewportState.pViewports = vkViewports.data();
    }
    else if(auto c = viewports_.AsIf<int>())
    {
        viewportState.viewportCount = static_cast<uint32_t>(*c);
    }

    std::vector<VkRect2D> vkScissors;
    if(auto ss = scissors_.AsIf<std::vector<Scissor>>())
    {
        vkScissors.resize(ss->size());
        for(auto &&[i, s] : Enumerate(*ss))
        {
            vkScissors[i] = TranslateScissor(s);
        }
        viewportState.scissorCount = static_cast<uint32_t>(vkScissors.size());
        viewportState.pScissors = vkScissors.data();
    }
    else if(auto c = scissors_.AsIf<int>())
    {
        viewportState.scissorCount = static_cast<uint32_t>(*c);
    }

    const VkPipelineRasterizationStateCreateInfo rasterizationState = {
        .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode             = TranslateFillMode(fillMode_),
        .cullMode                = TranslateCullMode(cullMode_),
        .frontFace               = TranslateFrontFaceMode(frontFaceMode_),
        .depthBiasEnable         = enableDepthBias_,
        .depthBiasConstantFactor = depthBiasConstFactor_,
        .depthBiasClamp          = depthBiasClampValue_,
        .depthBiasSlopeFactor    = depthBiasSlopeFactor_,
        .lineWidth               = 1
    };

    const VkPipelineMultisampleStateCreateInfo multisampleState = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = TranslateSampleCount(multisampleCount_)
    };

    const VkStencilOpState frontStencilOp = {
        .failOp      = TranslateStencilOp(frontStencilOp_.failOp),
        .passOp      = TranslateStencilOp(frontStencilOp_.passOp),
        .depthFailOp = TranslateStencilOp(frontStencilOp_.depthFailOp),
        .compareOp   = TranslateCompareOp(frontStencilOp_.compareOp),
        .compareMask = frontStencilOp_.compareMask,
        .writeMask   = frontStencilOp_.writeMask,
        .reference   = 0
    };
    
    const VkStencilOpState backStencilOp = {
        .failOp      = TranslateStencilOp(backStencilOp_.failOp),
        .passOp      = TranslateStencilOp(backStencilOp_.passOp),
        .depthFailOp = TranslateStencilOp(backStencilOp_.depthFailOp),
        .compareOp   = TranslateCompareOp(backStencilOp_.compareOp),
        .compareMask = backStencilOp_.compareMask,
        .writeMask   = backStencilOp_.writeMask,
        .reference   = 0
    };

    const VkPipelineDepthStencilStateCreateInfo depthStencilState = {
        .sType             = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable   = enableDepthTest_,
        .depthWriteEnable  = enableDepthWrite_,
        .depthCompareOp    = TranslateCompareOp(depthCompareOp_),
        .stencilTestEnable = enableStencilTest_,
        .front             = frontStencilOp,
        .back              = backStencilOp
    };

    const auto blendAttachment = VkPipelineColorBlendAttachmentState{
        .blendEnable         = enableBlending_,
        .srcColorBlendFactor = TranslateBlendFactor(blendingSrcColorFactor_),
        .dstColorBlendFactor = TranslateBlendFactor(blendingDstColorFactor_),
        .colorBlendOp        = TranslateBlendOp(blendingColorOp_),
        .srcAlphaBlendFactor = TranslateBlendFactor(blendingSrcAlphaFactor_),
        .dstAlphaBlendFactor = TranslateBlendFactor(blendingDstAlphaFactor_),
        .alphaBlendOp        = TranslateBlendOp(blendingAlphaOp_),
        .colorWriteMask      = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                             | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };
    std::vector blendAttachments(colorAttachments.size(), blendAttachment);
    const VkPipelineColorBlendStateCreateInfo colorBlendState = {
        .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = static_cast<uint32_t>(colorAttachments.size()),
        .pAttachments    = blendAttachments.data()
    };

    uint32_t dynamicStateCount = 0;
    VkDynamicState dynamicStates[2];

    if(viewports_.Is<int>())
    {
        dynamicStates[dynamicStateCount++] = VK_DYNAMIC_STATE_VIEWPORT;
    }
    else if(viewports_.Is<DynamicViewportCount>())
    {
        dynamicStates[dynamicStateCount++] = VK_DYNAMIC_STATE_VIEWPORT_WITH_COUNT;
    }

    if(scissors_.Is<int>())
    {
        dynamicStates[dynamicStateCount++] = VK_DYNAMIC_STATE_SCISSOR;
    }
    else if(scissors_.Is<DynamicScissorCount>())
    {
        dynamicStates[dynamicStateCount++] = VK_DYNAMIC_STATE_SCISSOR_WITH_COUNT;
    }

    const VkPipelineDynamicStateCreateInfo dynamicState = {
        .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = dynamicStateCount,
        .pDynamicStates    = dynamicStates
    };

    auto vkBindingLayout = static_cast<VulkanBindingLayout *>(bindingLayout_.get());
    const VkGraphicsPipelineCreateInfo pipelineCreateInfo = {
        .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext               = &renderingCreateInfo,
        .stageCount          = 2,
        .pStages             = stages,
        .pVertexInputState   = &vertexInputState,
        .pInputAssemblyState = &inputAssemblyState,
        .pViewportState      = &viewportState,
        .pRasterizationState = &rasterizationState,
        .pMultisampleState   = &multisampleState,
        .pDepthStencilState  = &depthStencilState,
        .pColorBlendState    = &colorBlendState,
        .pDynamicState       = &dynamicState,
        .layout              = vkBindingLayout->GetNativeLayout()
    };

    VkPipeline pipeline;
    VK_FAIL_MSG(
        vkCreateGraphicsPipelines(device_, nullptr, 1, &pipelineCreateInfo, VK_ALLOC, &pipeline),
        "failed to create vulkan graphics pipeline");
    RTRC_SCOPE_FAIL{ vkDestroyPipeline(device_, pipeline, VK_ALLOC); };

    return MakeRC<VulkanGraphicsPipeline>(bindingLayout_, device_, pipeline);
}

RTRC_RHI_VK_END
