#pragma once

#include <array>
#include <optional>
#include <vector>

#include <Rtrc/Utils/EnumFlags.h>
#include <Rtrc/Utils/ReferenceCounted.h>
#include <Rtrc/Utils/Span.h>
#include <Rtrc/Utils/Uncopyable.h>
#include <Rtrc/Utils/Variant.h>
#include <Rtrc/Window/Window.h>

RTRC_BEGIN

struct StructInfo;

RTRC_END

RTRC_RHI_BEGIN

template<typename T>
using Ptr = ReferenceCountedPtr<T>;

template<typename T, typename...Args>
Ptr<T> MakePtr(Args&&...args)
{
    T *object = new T(std::forward<Args>(args)...);
    return Ptr<T>(object);
}

#define RTRC_RHI_FORWARD_DECL(CLASS) class CLASS; using CLASS##Ptr = Ptr<CLASS>;

RTRC_RHI_FORWARD_DECL(Instance)
RTRC_RHI_FORWARD_DECL(Device)
RTRC_RHI_FORWARD_DECL(BackBufferSemaphore)
RTRC_RHI_FORWARD_DECL(Swapchain)
RTRC_RHI_FORWARD_DECL(Queue)
RTRC_RHI_FORWARD_DECL(Fence)
RTRC_RHI_FORWARD_DECL(CommandPool)
RTRC_RHI_FORWARD_DECL(CommandBuffer)
RTRC_RHI_FORWARD_DECL(RawShader)
RTRC_RHI_FORWARD_DECL(BindingGroupLayout)
RTRC_RHI_FORWARD_DECL(BindingGroup)
RTRC_RHI_FORWARD_DECL(BindingLayout)
RTRC_RHI_FORWARD_DECL(GraphicsPipeline)
RTRC_RHI_FORWARD_DECL(GraphicsPipelineBuilder)
RTRC_RHI_FORWARD_DECL(ComputePipeline)
RTRC_RHI_FORWARD_DECL(ComputePipelineBuilder)
RTRC_RHI_FORWARD_DECL(Texture)
RTRC_RHI_FORWARD_DECL(Texture2DRTV)
RTRC_RHI_FORWARD_DECL(Texture2DSRV)
RTRC_RHI_FORWARD_DECL(Texture2DUAV)
RTRC_RHI_FORWARD_DECL(Buffer)
RTRC_RHI_FORWARD_DECL(BufferSRV)
RTRC_RHI_FORWARD_DECL(BufferUAV)
RTRC_RHI_FORWARD_DECL(Sampler)
RTRC_RHI_FORWARD_DECL(MemoryPropertyRequirements)
RTRC_RHI_FORWARD_DECL(MemoryBlock)

#undef RTRC_RHI_FORWARD_DECL

// =============================== rhi enums ===============================

enum class QueueType : uint8_t
{
    Graphics,
    Compute,
    Transfer
};

enum class Format : uint32_t
{
    Unknown,
    B8G8R8A8_UNorm,
    R32G32_Float,
    R32G32B32A32_Float,
};

const char *GetFormatName(Format format);

size_t GetTexelSize(Format format);

enum class ShaderStage : uint8_t
{
    VertexShader   = 0b0001,
    FragmentShader = 0b0010,
    ComputeShader  = 0b0100
};

RTRC_DEFINE_ENUM_FLAGS(ShaderStage)

using ShaderStageFlag = EnumFlags<ShaderStage>;

namespace ShaderStageFlags
{

    constexpr auto VS  = ShaderStageFlag(ShaderStage::VertexShader);
    constexpr auto FS  = ShaderStageFlag(ShaderStage::FragmentShader);
    constexpr auto CS  = ShaderStageFlag(ShaderStage::ComputeShader);
    constexpr auto All = VS | FS | CS;

} // namespace ShaderStageFlags

enum class PrimitiveTopology
{
    TriangleList
};

enum class CullMode
{
    DontCull,
    CullFront,
    CullBack,
    CullAll
};

enum class FrontFaceMode
{
    Clockwise,
    CounterClockwise
};

enum class FillMode
{
    Fill,
    Line,
    Point
};

enum class CompareOp
{
    Less,
    LessEqual,
    Equal,
    NotEqual,
    GreaterEqual,
    Greater,
    Always,
    Never
};

enum class StencilOp
{
    Keep,
    Zero,
    Replace,
    IncreaseClamp,
    DecreaseClamp,
    IncreaseWrap,
    DecreaseWrap,
    Invert
};

enum class BlendFactor
{
    Zero,
    One,
    SrcAlpha,
    DstAlpha,
    OneMinusSrcAlpha,
    OneMinusDstAlpha
};

enum class BlendOp
{
    Add,
    Sub,
    SubReverse,
    Min,
    Max
};

enum class BindingType
{
    Texture2D,
    RWTexture2D,
    Buffer,
    StructuredBuffer,
    RWBuffer,
    RWStructuredBuffer,
    ConstantBuffer,
    Sampler,
};

enum class BindingTemplateParameterType
{
    Undefined,
    Int,   Int2,   Int3,   Int4,
    UInt,  UInt2,  UInt3,  UInt4,
    Float, Float2, Float3, Float4,
    Struct,
};

enum class TextureDimension
{
    Tex2D
};

enum class TextureUsage : uint32_t
{
    TransferDst    = 1 << 0,
    TransferSrc    = 1 << 1,
    ShaderResource = 1 << 2,
    UnorderAccess  = 1 << 3,
    RenderTarget   = 1 << 4,
    DepthStencil   = 1 << 5
};

RTRC_DEFINE_ENUM_FLAGS(TextureUsage)

using TextureUsageFlag = EnumFlags<TextureUsage>;

enum class BufferUsage : uint32_t
{
    TransferDst,
    TransferSrc,
    ShaderBuffer,
    ShaderRWBuffer,
    ShaderStructuredBuffer,
    ShaderRWStructuredBuffer,
    ShaderConstantBuffer,
    IndexBuffer,
    VertexBuffer,
    IndirectBuffer
};

RTRC_DEFINE_ENUM_FLAGS(BufferUsage)

using BufferUsageFlag = EnumFlags<BufferUsage>;

enum class BufferHostAccessType
{
    None,
    SequentialWrite,
    Random
};

enum class FilterMode
{
    Point,
    Linear
};

enum class AddressMode
{
    Repeat,
    Mirror,
    Clamp,
    Border
};

enum class TextureLayout
{
    Undefined,
    RenderTarget,
    DepthStencil,
    DepthStencilReadOnly,
    ShaderTexture,
    ShaderRWTexture,
    CopySrc,
    CopyDst,
    ResolveSrc,
    ResolveDst,
    Present
};

enum class PipelineStage : uint32_t
{
    None           = 0,
    InputAssembler = 1 << 0,
    VertexShader   = 1 << 1,
    FragmentShader = 1 << 2,
    ComputeShader  = 1 << 3,
    DepthStencil   = 1 << 4,
    RenderTarget   = 1 << 5,
    Copy           = 1 << 6,
    Resolve        = 1 << 7
};

RTRC_DEFINE_ENUM_FLAGS(PipelineStage)

using PipelineStageFlag = EnumFlags<PipelineStage>;

enum class ResourceAccess : uint32_t
{
    None                    = 0,
    VertexBufferRead        = 1 << 0,
    IndexBufferRead         = 1 << 1,
    ConstantBufferRead      = 1 << 2,
    RenderTargetRead        = 1 << 3,
    RenderTargetWrite       = 1 << 4,
    DepthStencilRead        = 1 << 5,
    DepthStencilWrite       = 1 << 6,
    TextureRead             = 1 << 7,
    RWTextureRead           = 1 << 8,
    RWTextureWrite          = 1 << 9,
    BufferRead              = 1 << 10,
    RWBufferRead            = 1 << 11,
    RWBufferWrite           = 1 << 12,
    RWStructuredBufferRead  = 1 << 13,
    RWStructuredBufferWrite = 1 << 14,
    CopyRead                = 1 << 15,
    CopyWrite               = 1 << 16,
    ResolveRead             = 1 << 17,
    ResolveWrite            = 1 << 18,
};

RTRC_DEFINE_ENUM_FLAGS(ResourceAccess)

using ResourceAccessFlag = EnumFlags<ResourceAccess>;

enum class AspectType : uint8_t
{
    Color   = 1 << 0,
    Depth   = 1 << 1,
    Stencil = 1 << 2
};

RTRC_DEFINE_ENUM_FLAGS(AspectType)

using AspectTypeFlag = EnumFlags<AspectType>;

enum class AttachmentLoadOp
{
    Load,
    Clear,
    DontCare
};

enum class AttachmentStoreOp
{
    Store,
    DontCare,
};

enum class QueueConcurrentAccessMode
{
    Exclusive, // exclusively accessed by one queue
    Concurrent // concurrently accessed by graphics/compute queues
};

// =============================== rhi descriptions ===============================

struct DeviceDesc
{
    bool graphicsQueue    = true;
    bool computeQueue     = true;
    bool transferQueue    = true;
    bool supportSwapchain = true;
};

struct SwapchainDesc
{
    Format format;
    uint32_t imageCount;
};

struct BindingDesc
{
    std::string             name;
    BindingType             type;
    ShaderStageFlag         shaderStages = ShaderStageFlags::All;
    std::optional<uint32_t> arraySize;

    std::strong_ordering operator<=>(const BindingDesc &other) const
    {
        return std::tie(name, type, shaderStages, arraySize) <=>
               std::tie(other.name, other.type, other.shaderStages, other.arraySize);
    }
};

using AliasedBindingsDesc = std::vector<BindingDesc>;

struct BindingGroupLayoutDesc
{
    std::vector<AliasedBindingsDesc> bindings;

    auto operator<=>(const BindingGroupLayoutDesc &other) const = default;
};

struct BindingLayoutDesc
{
    std::vector<Ptr<BindingGroupLayout>> groups;

    auto operator<=>(const BindingLayoutDesc &) const = default;
};

struct Viewport
{
    Vector2f lowerLeftCorner;
    Vector2f size;
    float minDepth;
    float maxDepth;
};

struct Scissor
{
    Vector2i lowerLeftCorner;
    Vector2i size;
};

struct Texture2DDesc
{
    Format                    format;
    uint32_t                  width;
    uint32_t                  height;
    uint32_t                  mipLevels;
    uint32_t                  arraySize;
    uint32_t                  sampleCount;
    TextureUsageFlag          usage;
    TextureLayout             initialLayout;
    QueueConcurrentAccessMode concurrentAccessMode;
};

struct Texture2DRTVDesc
{
    Format   format;
    uint32_t mipLevel;
    uint32_t arrayLayer;
};

struct Texture2DSRVDesc
{
    Format   format;
    uint32_t baseMipLevel;
    uint32_t levelCount;
    uint32_t baseArrayLayer;
    uint32_t layerCount;
};

struct Texture2DUAVDesc
{
    Format   format;
    uint32_t baseMipLevel;
    uint32_t levelCount;
    uint32_t baseArrayLayer;
    uint32_t layerCount;
};

struct BufferDesc
{
    size_t                    size;
    BufferUsageFlag           usage;
    BufferHostAccessType      hostAccessType;
    QueueConcurrentAccessMode concurrentAccessMode;
};

struct BufferSRVDesc
{
    Format   format; // for texel buffer
    uint32_t offset;
    uint32_t range;
    uint32_t stride; // for structured buffer
};

using BufferUAVDesc = BufferSRVDesc;

struct SamplerDesc
{
    FilterMode magFilter;
    FilterMode minFilter;
    FilterMode mipFilter;

    AddressMode addressModeU;
    AddressMode addressModeV;
    AddressMode addressModeW;

    float mipLODBias;
    float minLOD;
    float maxLOD;

    bool enableAnisotropy;
    int  maxAnisotropy;

    bool      enableComparision;
    CompareOp compareOp;

    std::array<float, 4> borderColor;
};

struct TextureSubresourceRange
{
    uint32_t       mipLevel;
    uint32_t       levelCount;
    uint32_t       arrayLayer;
    uint32_t       layerCount;
    AspectTypeFlag aspects;
};

struct TextureTransitionBarrier
{
    Texture                *texture;
    AspectTypeFlag          aspectTypeFlag;
    uint32_t                mipLevel;
    uint32_t                arrayLayer;
    PipelineStageFlag       beforeStages;
    ResourceAccessFlag      beforeAccesses;
    TextureLayout           beforeLayout;
    PipelineStageFlag       afterStages;
    ResourceAccessFlag      afterAccesses;
    TextureLayout           afterLayout;
    TextureSubresourceRange range;
};

// for non-sharing resource, every cross-queue sync needs a release/acquire pair
struct TextureReleaseBarrier
{
    Texture                *texture;
    AspectTypeFlag          aspectTypeFlag;
    uint32_t                mipLevel;
    uint32_t                arrayLayer;
    PipelineStageFlag       beforeStages;
    ResourceAccessFlag      beforeAccesses;
    TextureLayout           beforeLayout;
    TextureLayout           afterLayout;
    Queue                  *beforeQueue;
    Queue                  *afterQueue;
    TextureSubresourceRange range;
};

struct TextureAcquireBarrier
{
    Texture                *texture;
    AspectTypeFlag          aspectTypeFlag;
    uint32_t                mipLevel;
    uint32_t                arrayLayer;
    TextureLayout           beforeLayout;
    PipelineStageFlag       afterStages;
    ResourceAccessFlag      afterAccesses;
    TextureLayout           afterLayout;
    Queue                  *beforeQueue;
    Queue                  *afterQueue;
    TextureSubresourceRange range;
};

struct BufferTransitionBarrier
{
    Buffer            *buffer;
    PipelineStageFlag  beforeStages;
    PipelineStageFlag  afterStages;
    ResourceAccessFlag beforeAccesses;
    ResourceAccessFlag afterAccesses;
};

struct BufferReleaseBarrier
{
    Buffer            *buffer;
    PipelineStageFlag  beforeStages;
    ResourceAccessFlag beforeAccesses;
    Queue             *beforeQueue;
    Queue             *afterQueue;
};

struct BufferAcquireBarrier
{
    Buffer            *buffer;
    PipelineStageFlag  afterStages;
    ResourceAccessFlag afterAccesses;
    Queue             *beforeQueue;
    Queue             *afterQueue;
};

struct ColorClearValue
{
    float r, g, b, a;
};

struct DepthStencilClearValue
{
    float    depth;
    uint32_t stencil;
};

using ClearValue = Variant<ColorClearValue, DepthStencilClearValue>;

struct RenderPassColorAttachment
{
    Texture2DRTV     *rtv;
    AttachmentLoadOp  loadOp;
    AttachmentStoreOp storeOp;
    ClearValue        clearValue;
};

struct DynamicViewportCount
{

};

struct DynamicScissorCount
{

};

// fixed viewports; dynamic viewports with fixed count; dynamic count
using Viewports = Variant<std::monostate, std::vector<Viewport>, int, DynamicViewportCount>;

// fixed viewports; dynamic viewports with fixed count; dynamic count
using Scissors = Variant<std::monostate, std::vector<Scissor>, int, DynamicScissorCount>;

struct MemoryBlockDesc
{
    size_t size;
    size_t alignment;
    Ptr<MemoryPropertyRequirements> properties;
};

// =============================== rhi interfaces ===============================

class RHIObject : public ReferenceCounted, public Uncopyable
{
protected:

    std::string RHIObjectName_;

public:

    virtual ~RHIObject() = default;

    virtual void SetName(std::string name) { RHIObjectName_ = std::move(name); }

    virtual const std::string &GetName() { return RHIObjectName_; }
};

class Surface : public RHIObject
{
    
};

class Instance : public RHIObject
{
public:

    virtual Ptr<Device> CreateDevice(const DeviceDesc &desc = {}) = 0;
};

class Device : public RHIObject
{
public:

    virtual Ptr<Queue> GetQueue(QueueType type) = 0;

    virtual Ptr<Fence> CreateFence(bool signaled) = 0;

    virtual Ptr<Swapchain> CreateSwapchain(const SwapchainDesc &desc, Window &window) = 0;

    virtual Ptr<RawShader> CreateShader(const void *data, size_t size, std::string entryPoint, ShaderStage type) = 0;

    virtual Ptr<GraphicsPipelineBuilder> CreateGraphicsPipelineBuilder() = 0;

    virtual Ptr<ComputePipelineBuilder> CreateComputePipelineBuilder() = 0;

    virtual Ptr<BindingGroupLayout> CreateBindingGroupLayout(const BindingGroupLayoutDesc *desc) = 0;

    virtual Ptr<BindingLayout> CreateBindingLayout(const BindingLayoutDesc &desc) = 0;

    virtual Ptr<Texture> CreateTexture2D(const Texture2DDesc &desc) = 0;

    virtual Ptr<Buffer> CreateBuffer(const BufferDesc &desc) = 0;

    virtual Ptr<Sampler> CreateSampler(const SamplerDesc &desc) = 0;

    virtual Ptr<MemoryPropertyRequirements> GetMemoryRequirements(
        const Texture2DDesc &desc, size_t *size, size_t *alignment) const = 0;

    virtual Ptr<MemoryPropertyRequirements> GetMemoryRequirements(
        const BufferDesc &desc, size_t *size, size_t *alignment) const = 0;

    virtual Ptr<MemoryBlock> CreateMemoryBlock(const MemoryBlockDesc &desc) = 0;

    virtual Ptr<Texture> CreatePlacedTexture2D(
        const Texture2DDesc &desc, const Ptr<MemoryBlock> &memoryBlock, size_t offsetInMemoryBlock) = 0;

    virtual Ptr<Buffer> CreatePlacedBuffer(
        const BufferDesc &desc, const Ptr<MemoryBlock> &memoryBlock, size_t offsetInMemoryBlock) = 0;

    virtual void WaitIdle() = 0;
};

class BackBufferSemaphore : public RHIObject
{

};

class Swapchain : public RHIObject
{
public:

    virtual bool Acquire() = 0;

    virtual Ptr<BackBufferSemaphore> GetAcquireSemaphore() = 0;

    virtual Ptr<BackBufferSemaphore> GetPresentSemaphore() = 0;

    virtual void Present() = 0;

    virtual int GetRenderTargetCount() const = 0;

    virtual const Texture2DDesc &GetRenderTargetDesc() const = 0;

    virtual Ptr<Texture> GetRenderTarget() const = 0;
};

class BindingGroupLayout : public RHIObject
{
public:

    virtual const BindingGroupLayoutDesc *GetDesc() const = 0;

    virtual Ptr<BindingGroup> CreateBindingGroup() = 0;
};

class BindingGroup : public RHIObject
{
public:

    virtual const BindingGroupLayout *GetLayout() const = 0;

    virtual void ModifyMember(int index, const Ptr<BufferSRV> &bufferSRV) = 0;

    virtual void ModifyMember(int index, const Ptr<BufferUAV> &bufferUAV) = 0;

    virtual void ModifyMember(int index, const Ptr<Texture2DSRV> &textureSRV) = 0;

    virtual void ModifyMember(int index, const Ptr<Texture2DUAV> &textureUAV) = 0;

    virtual void ModifyMember(int index, const Ptr<Sampler> &sampler) = 0;

    virtual void ModifyMember(int index, const Ptr<Buffer> &uniformBuffer, size_t offset, size_t range) = 0;
};

class BindingLayout : public RHIObject
{

};

class Queue : public RHIObject
{
public:

    virtual QueueType GetType() const = 0;

    virtual Ptr<CommandPool> CreateCommandPool() = 0;

    virtual void WaitIdle() = 0;

    virtual void Submit(
        const Ptr<BackBufferSemaphore>  &waitBackBufferSemaphore,
        PipelineStage                    waitBackBufferStages,
        Span<Ptr<CommandBuffer>>         commandBuffers,
        const Ptr<BackBufferSemaphore>  &signalBackBufferSemaphore,
        PipelineStage                    signalBackBufferStages,
        const Ptr<Fence>               &signalFence) = 0;
};

class Fence : public RHIObject
{
public:

    virtual void Reset() = 0;

    virtual void Wait() = 0;
};

class CommandPool : public RHIObject
{
public:

    virtual void Reset() = 0;

    virtual Ptr<CommandBuffer> NewCommandBuffer() = 0;
};

class CommandBuffer : public RHIObject
{
public:

    virtual void Begin() = 0;

    virtual void End() = 0;

    virtual void ExecuteBarriers(
        Span<TextureTransitionBarrier> textureTransitions,
        Span<BufferTransitionBarrier>  bufferTransitions,
        Span<TextureReleaseBarrier>    textureReleaseBarriers,
        Span<TextureAcquireBarrier>    textureAcquireBarriers,
        Span<BufferReleaseBarrier>     bufferReleaseBarriers,
        Span<BufferAcquireBarrier>     bufferAcquireBarriers) = 0;

    virtual void BeginRenderPass(Span<RenderPassColorAttachment> colorAttachments) = 0;

    virtual void EndRenderPass() = 0;

    virtual void BindPipeline(const Ptr<GraphicsPipeline> &pipeline) = 0;

    virtual void BindPipeline(const Ptr<ComputePipeline> &pipeline) = 0;

    virtual void BindGroupsToGraphicsPipeline(int startIndex, Span<RC<BindingGroup>> groups) = 0;

    virtual void BindGroupsToComputePipeline(int startIndex, Span<RC<BindingGroup>> groups) = 0;

    virtual void BindGroupToGraphicsPipeline(int index, const Ptr<BindingGroup> &group) = 0;

    virtual void BindGroupToComputePipeline(int index, const Ptr<BindingGroup> &group) = 0;

    virtual void SetViewports(Span<Viewport> viewports) = 0;

    virtual void SetScissors(Span<Scissor> scissor) = 0;

    virtual void SetViewportsWithCount(Span<Viewport> viewports) = 0;

    virtual void SetScissorsWithCount(Span<Scissor> scissors) = 0;

    virtual void Draw(int vertexCount, int instanceCount, int firstVertex, int firstInstance) = 0;

    virtual void Dispatch(int groupCountX, int groupCountY, int groupCountZ) = 0;

    virtual void CopyBuffer(
        Buffer *dst, size_t dstOffset,
        Buffer *src, size_t srcOffset, size_t range) = 0;

    virtual void CopyBufferToTexture(
        Texture *dst, AspectTypeFlag aspect, uint32_t mipLevel, uint32_t arrayLayer,
        Buffer *src, size_t srcOffset) = 0;

    virtual void CopyTextureToBuffer(
        Buffer *dst, size_t dstOffset,
        Texture *src, AspectTypeFlag aspect, uint32_t mipLevel, uint32_t arrayLayer) = 0;

protected:

    virtual const Ptr<GraphicsPipeline> &GetCurrentPipeline() const = 0;
};

class RawShader : public RHIObject
{
public:

    virtual ShaderStage GetType() const = 0;
};

class GraphicsPipeline : public RHIObject
{
public:

    virtual const Ptr<BindingLayout> &GetBindingLayout() const = 0;
};

class GraphicsPipelineBuilder : public RHIObject
{
public:

    virtual GraphicsPipelineBuilder &SetVertexShader(Ptr<RawShader> vertexShader) = 0;

    virtual GraphicsPipelineBuilder &SetFragmentShader(Ptr<RawShader> fragmentShader) = 0;

    virtual GraphicsPipelineBuilder &SetBindingLayout(Ptr<BindingLayout> layout) = 0;

    virtual GraphicsPipelineBuilder &SetViewports(const Viewports &viewports) = 0;

    virtual GraphicsPipelineBuilder &SetScissors(const Scissors &scissors) = 0;

    // default is trianglelist
    virtual GraphicsPipelineBuilder &SetPrimitiveTopology(PrimitiveTopology topology) = 0;

    // default is fill
    virtual GraphicsPipelineBuilder &SetFillMode(FillMode mode) = 0;

    // default is cullnone
    virtual GraphicsPipelineBuilder &SetCullMode(CullMode mode) = 0;

    // default is cw
    virtual GraphicsPipelineBuilder &SetFrontFace(FrontFaceMode mode) = 0;

    // default is 0
    virtual GraphicsPipelineBuilder &SetDepthBias(float constFactor, float slopeFactor, float clamp) = 0;

    // default is 1
    virtual GraphicsPipelineBuilder &SetMultisample(int sampleCount) = 0;

    // default is disabled
    virtual GraphicsPipelineBuilder &SetDepthTest(bool enableTest, bool enableWrite, CompareOp compareOp) = 0;

    // default is disabled
    virtual GraphicsPipelineBuilder &SetStencilTest(bool enableTest) = 0;

    // default is keep, keep, keep, always, 0xff, 0xff
    virtual GraphicsPipelineBuilder &SetStencilFrontOp(
        StencilOp depthFailOp,
        StencilOp failOp,
        StencilOp passOp,
        CompareOp compareOp,
        uint32_t  compareMask,
        uint32_t  writeMask) = 0;

    // default is keep, keep, keep, always, 0xff, 0xff
    virtual GraphicsPipelineBuilder &SetStencilBackOp(
        StencilOp depthFailOp,
        StencilOp failOp,
        StencilOp passOp,
        CompareOp compareOp,
        uint32_t  compareMask,
        uint32_t  writeMask) = 0;

    // default is disabled
    // src: from fragment shader
    // dst: from render target
    // out = srcFactor * src op dstFactor * dst
    virtual GraphicsPipelineBuilder &SetBlending(
        bool        enableBlending,
        BlendFactor srcColorFactor,
        BlendFactor dstColorFactor,
        BlendOp     colorOp,
        BlendFactor srcAlphaFactor,
        BlendFactor dstAlphaFactor,
        BlendOp     alphaOp) = 0;

    // default is empty
    virtual GraphicsPipelineBuilder &AddColorAttachment(Format format) = 0;

    // default is undefined
    virtual GraphicsPipelineBuilder &SetDepthStencilAttachment(Format format) = 0;

    virtual Ptr<GraphicsPipeline> CreatePipeline() const = 0;
};

class ComputePipeline : public RHIObject
{
public:

    virtual const Ptr<BindingLayout> &GetBindingLayout() const = 0;
};

class ComputePipelineBuilder : public RHIObject
{
public:

    virtual ComputePipelineBuilder &SetComputeShader(Ptr<RawShader> shader) = 0;

    virtual ComputePipelineBuilder &SetBindingLayout(Ptr<BindingLayout> layout) = 0;

    virtual Ptr<ComputePipeline> CreatePipeline() const = 0;
};

class Texture : public RHIObject
{
public:

    virtual TextureDimension GetDimension() const = 0;

    virtual const Texture2DDesc &Get2DDesc() const = 0;

    virtual Ptr<Texture2DRTV> Create2DRTV(const Texture2DRTVDesc &desc) const = 0;

    virtual Ptr<Texture2DSRV> Create2DSRV(const Texture2DSRVDesc &desc) const = 0;

    virtual Ptr<Texture2DUAV> Create2DUAV(const Texture2DUAVDesc &desc) const = 0;
};

class Texture2DRTV : public RHIObject
{
public:

    virtual const Texture2DRTVDesc &GetDesc() const = 0;
};

class Texture2DSRV : public RHIObject
{
public:

    virtual const Texture2DSRVDesc &GetDesc() const = 0;
};

class Texture2DUAV : public RHIObject
{
public:

    virtual const Texture2DUAVDesc &GetDesc() const = 0;
};

class Buffer : public RHIObject
{
public:

    virtual const BufferDesc &GetDesc() const = 0;

    virtual Ptr<BufferSRV> CreateSRV(const BufferSRVDesc &desc) const = 0;

    virtual Ptr<BufferUAV> CreateUAV(const BufferUAVDesc &desc) const = 0;

    virtual void *Map(size_t offset, size_t size) const = 0;

    virtual void Unmap(size_t offset, size_t size) = 0;
};

class BufferSRV : public RHIObject
{
public:

    virtual const BufferSRVDesc &GetDesc() const = 0;
};

class BufferUAV : public RHIObject
{
public:

    virtual const BufferUAVDesc &GetDesc() const = 0;
};

class Sampler : public RHIObject
{
public:

    virtual const SamplerDesc &GetDesc() const = 0;
};

// all memory requirements other than size & alignment
class MemoryPropertyRequirements : public RHIObject
{
public:

    virtual bool IsValid() const = 0;

    virtual bool Merge(const MemoryPropertyRequirements &other) = 0;

    virtual Ptr<MemoryPropertyRequirements> Clone() const = 0;
};

class MemoryBlock : public RHIObject
{
public:

    virtual const MemoryBlockDesc &GetDesc() const = 0;
};

// =============================== vulkan backend ===============================

#ifdef RTRC_RHI_VULKAN

// can be called multiple times
void InitializeVulkanBackend();

struct VulkanInstanceDesc
{
    std::vector<std::string> extensions;
    bool debugMode = false;
};

Ptr<Instance> CreateVulkanInstance(const VulkanInstanceDesc &desc);

#endif

// =============================== directx12 backend ===============================

#ifdef RTRC_RHI_DIRECTX12

void InitializeDirectX12Backend();

struct DirectX12InstanceDesc
{
    bool debugMode = false;
    bool gpuValidation = false;
};

Ptr<Instance> CreateDirectX12Instance(const DirectX12InstanceDesc &desc);

#endif

RTRC_RHI_END
