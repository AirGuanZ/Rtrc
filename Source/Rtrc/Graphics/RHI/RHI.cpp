#include <Rtrc/Graphics/RHI/RHI.h>
#include <Rtrc/Utils/Unreachable.h>

RTRC_RHI_BEGIN

const char *GetFormatName(Format format)
{
#define ADD_CASE(NAME) case Format::NAME: return #NAME;
    switch(format)
    {
    ADD_CASE(Unknown)
    ADD_CASE(B8G8R8A8_UNorm)
    ADD_CASE(R32G32_Float)
    ADD_CASE(R32G32B32A32_Float)
    }
    throw Exception("unknown format: " + std::to_string(static_cast<int>(format)));
#undef ADD_CASE
}

size_t GetTexelSize(Format format)
{
    switch(format)
    {
    case Format::Unknown:            return 0;
    case Format::B8G8R8A8_UNorm:     return 4;
    case Format::R32G32_Float:       return 8;
    case Format::R32G32B32A32_Float: return 16;
    }
    throw Exception("unknown format: " + std::to_string(static_cast<int>(format)));
}

bool IsReadOnly(ResourceAccessFlag access)
{
    using enum ResourceAccess;
    constexpr ResourceAccessFlag READONLY_MASK =
        VertexBufferRead
      | IndexBufferRead
      | ConstantBufferRead
      | RenderTargetRead
      | DepthStencilRead
      | TextureRead
      | RWTextureRead
      | BufferRead
      | RWBufferRead
      | RWStructuredBufferRead
      | CopyRead
      | ResolveRead;
    return (access & READONLY_MASK) == access;
}

bool IsWriteOnly(ResourceAccessFlag access)
{
    using enum ResourceAccess;
    constexpr ResourceAccessFlag WRITEONLY_MASK =
        RenderTargetWrite
      | DepthStencilWrite
      | RWTextureWrite
      | RWBufferWrite
      | RWStructuredBufferWrite
      | CopyWrite
      | ResolveWrite
      | ClearWrite;
    return (access & WRITEONLY_MASK) == access;
}

GraphicsPipelineBuilder &GraphicsPipelineBuilder::SetVertexShader(Ptr<RawShader> _vertexShader)
{
    vertexShader = std::move(_vertexShader);
    return *this;
}

GraphicsPipelineBuilder &GraphicsPipelineBuilder::SetFragmentShader(Ptr<RawShader> _fragmentShader)
{
    fragmentShader = std::move(_fragmentShader);
    return *this;
}

GraphicsPipelineBuilder &GraphicsPipelineBuilder::SetBindingLayout(Ptr<BindingLayout> layout)
{
    bindingLayout = std::move(layout);
    return *this;
}

GraphicsPipelineBuilder &GraphicsPipelineBuilder::SetViewports(const Viewports &_viewports)
{
    viewports = _viewports;
    return *this;
}

GraphicsPipelineBuilder &GraphicsPipelineBuilder::SetScissors(const Scissors &_scissors)
{
    scissors = _scissors;
    return *this;
}

GraphicsPipelineBuilder &GraphicsPipelineBuilder::AddVertexInputBuffers(Span<VertexInputBuffer> buffers)
{
    assert(!buffers.IsEmpty());
    const size_t initSize = vertexBuffers.size();
    vertexBuffers.resize(initSize + buffers.size());
    std::copy(buffers.begin(), buffers.end(), &vertexBuffers[initSize]);
    return *this;
}

GraphicsPipelineBuilder &GraphicsPipelineBuilder::AddVertexInputAttributes(Span<VertexInputAttribute> attributes)
{
    assert(!attributes.IsEmpty());
    const size_t initSize = vertexBuffers.size();
    vertexAttributs.resize(initSize + attributes.size());
    std::copy(attributes.begin(), attributes.end(), &vertexAttributs[initSize]);
    return *this;
}

GraphicsPipelineBuilder &GraphicsPipelineBuilder::SetPrimitiveTopology(PrimitiveTopology topology)
{
    primitiveTopology = topology;
    return *this;
}

GraphicsPipelineBuilder &GraphicsPipelineBuilder::SetFillMode(FillMode mode)
{
    fillMode = mode;
    return *this;
}

GraphicsPipelineBuilder &GraphicsPipelineBuilder::SetCullMode(CullMode mode)
{
    cullMode = mode;
    return *this;
}

GraphicsPipelineBuilder &GraphicsPipelineBuilder::SetFrontFace(FrontFaceMode mode)
{
    frontFaceMode = mode;
    return *this;
}

GraphicsPipelineBuilder &GraphicsPipelineBuilder::SetDepthBias(float constFactor, float slopeFactor, float clamp)
{
    enableDepthBias = true;
    depthBiasConstFactor = constFactor;
    depthBiasSlopeFactor = slopeFactor;
    depthBiasClampValue = clamp;
    return *this;
}

GraphicsPipelineBuilder &GraphicsPipelineBuilder::SetMultisample(int sampleCount)
{
    multisampleCount = sampleCount;
    return *this;
}

GraphicsPipelineBuilder &GraphicsPipelineBuilder::SetDepthTest(bool enableTest, bool enableWrite, CompareOp compareOp)
{
    enableDepthTest = enableTest;
    enableDepthWrite = enableWrite;
    depthCompareOp = compareOp;
    return *this;
}

GraphicsPipelineBuilder &GraphicsPipelineBuilder::SetStencilTest(bool enableTest)
{
    enableStencilTest = enableTest;
    return *this;
}

GraphicsPipelineBuilder &GraphicsPipelineBuilder::SetStencilFrontOp(
    StencilOp depthFailOp,
    StencilOp failOp,
    StencilOp passOp,
    CompareOp compareOp,
    uint32_t  compareMask,
    uint32_t  writeMask)
{
    frontStencilOp = StencilOps{
        .depthFailOp = depthFailOp,
        .failOp = failOp,
        .passOp = passOp,
        .compareOp = compareOp,
        .compareMask = compareMask,
        .writeMask = writeMask
    };
    return *this;
}

GraphicsPipelineBuilder &GraphicsPipelineBuilder::SetStencilBackOp(
    StencilOp depthFailOp,
    StencilOp failOp,
    StencilOp passOp,
    CompareOp compareOp,
    uint32_t  compareMask,
    uint32_t  writeMask)
{
    backStencilOp = StencilOps{
        .depthFailOp = depthFailOp,
        .failOp = failOp,
        .passOp = passOp,
        .compareOp = compareOp,
        .compareMask = compareMask,
        .writeMask = writeMask
    };
    return *this;
}

GraphicsPipelineBuilder &GraphicsPipelineBuilder::SetBlending(
    bool        _enableBlending,
    BlendFactor srcColorFactor,
    BlendFactor dstColorFactor,
    BlendOp     colorOp,
    BlendFactor srcAlphaFactor,
    BlendFactor dstAlphaFactor,
    BlendOp     alphaOp)
{
    enableBlending = _enableBlending;
    blendingSrcColorFactor = srcColorFactor;
    blendingDstColorFactor = dstColorFactor;
    blendingColorOp = colorOp;
    blendingSrcAlphaFactor = srcAlphaFactor;
    blendingDstAlphaFactor = dstAlphaFactor;
    blendingAlphaOp = alphaOp;
    return *this;
}

GraphicsPipelineBuilder &GraphicsPipelineBuilder::AddColorAttachment(Format format)
{
    colorAttachments.push_back(format);
    return *this;
}

GraphicsPipelineBuilder &GraphicsPipelineBuilder::SetDepthStencilAttachment(Format format)
{
    depthStencilFormat = format;
    return *this;
}

Ptr<GraphicsPipeline> GraphicsPipelineBuilder::CreatePipeline(const DevicePtr &device) const
{
    return device->CreateGraphicsPipeline(*this);
}

ComputePipelineBuilder &ComputePipelineBuilder::SetComputeShader(Ptr<RawShader> shader)
{
    computeShader = std::move(shader);
    return *this;
}

ComputePipelineBuilder &ComputePipelineBuilder::SetBindingLayout(Ptr<BindingLayout> layout)
{
    bindingLayout = std::move(layout);
    return *this;
}

Ptr<ComputePipeline> ComputePipelineBuilder::CreatePipeline(const DevicePtr &device) const
{
    return device->CreateComputePipeline(*this);
}

RTRC_RHI_END
