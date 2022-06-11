#pragma once

#include <vector>

#include <Rtrc/Utils/EnumFlags.h>
#include <Rtrc/Utils/Span.h>
#include <Rtrc/Utils/Uncopyable.h>
#include <Rtrc/Utils/Variant.h>
#include <Rtrc/Window/Window.h>

#include "Rtrc/Shader/Struct.h"

RTRC_BEGIN

struct StructInfo;

RTRC_END

RTRC_RHI_BEGIN

class Instance;
class Device;
class BackBufferSemaphore;
class Swapchain;
class Queue;
class Fence;
class CommandPool;
class CommandBuffer;
class Semaphore;
class RawShader;
class BindingGroupLayout;
class BindingGroup;
class BindingLayout;
class Pipeline;
class PipelineBuilder;
class Texture;
class Texture2DRTV;
class Texture2DSRV;
class Buffer;
class BufferSRV;

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
};

const char *GetFormatName(Format format);

enum class ShaderStage : uint8_t
{
    VertexShader   = 0b0001,
    FragmentShader = 0b0010
};

RTRC_DEFINE_ENUM_FLAGS(ShaderStage)

using ShaderStageFlag = EnumFlags<ShaderStage>;

namespace ShaderStageFlags
{

    constexpr auto VS  = ShaderStageFlag(ShaderStage::VertexShader);
    constexpr auto FS  = ShaderStageFlag(ShaderStage::FragmentShader);
    constexpr auto All = VS | FS;

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

constexpr uint32_t _rtrcUninitializedBase         = 1 << 0;
constexpr uint32_t _rtrcRenderTargetBase          = 1 << 1;
constexpr uint32_t _rtrcDepthBase                 = 1 << 2;
constexpr uint32_t _rtrcStencilBase               = 1 << 3;
constexpr uint32_t _rtrcConstantBufferBase        = 1 << 4;
constexpr uint32_t _rtrcTextureBase               = 1 << 5;
constexpr uint32_t _rtrcRWTextureBase             = 1 << 6;
constexpr uint32_t _rtrcBufferBase                = 1 << 7;
constexpr uint32_t _rtrcRWBufferBase              = 1 << 8;
constexpr uint32_t _rtrcStructuredBufferBase      = 1 << 9;
constexpr uint32_t _rtrcRWStructuredBufferBase    = 1 << 10;
constexpr uint32_t _rtrcCopyBase                  = 1 << 11;
constexpr uint32_t _rtrcResolveBase               = 1 << 12;
constexpr uint32_t _rtrcPresentBase               = 1 << 13;
constexpr uint32_t _rtrcDepthReadStencilWriteBase = 1 << 14;
constexpr uint32_t _rtrcDepthWriteStencilReadBase = 1 << 15;
constexpr uint32_t _rtrcVSBase                    = 1u << 20;
constexpr uint32_t _rtrcFSBase                    = 1u << 21;
constexpr uint32_t _rtrcReadBase                  = 1u << 30;
constexpr uint32_t _rtrcWriteBase                 = 1u << 31;

enum class ResourceState : uint32_t
{
    Uninitialized               = _rtrcUninitializedBase,
    RenderTargetReadWrite       = _rtrcRenderTargetBase | _rtrcReadBase | _rtrcWriteBase,
    RenderTargetWrite           = _rtrcRenderTargetBase | _rtrcWriteBase,
    DepthStencilReadWrite       = _rtrcDepthBase | _rtrcStencilBase | _rtrcReadBase | _rtrcWriteBase,
    DepthStencilReadOnly        = _rtrcDepthBase | _rtrcStencilBase | _rtrcReadBase,
    DepthStencilWriteOnly       = _rtrcDepthBase | _rtrcStencilBase | _rtrcWriteBase,
    DepthReadWrite              = _rtrcDepthBase | _rtrcReadBase | _rtrcWriteBase,
    DepthRead                   = _rtrcDepthBase | _rtrcReadBase,
    DepthWrite                  = _rtrcDepthBase | _rtrcWriteBase,
    StencilReadWrite            = _rtrcStencilBase | _rtrcReadBase | _rtrcWriteBase,
    StencilRead                 = _rtrcStencilBase | _rtrcReadBase,
    StencilWrite                = _rtrcStencilBase | _rtrcWriteBase,
    DepthReadStencilWrite       = _rtrcDepthReadStencilWriteBase | _rtrcReadBase | _rtrcWriteBase,
    DepthWriteStencilRead       = _rtrcDepthWriteStencilReadBase | _rtrcReadBase | _rtrcWriteBase,
    ConstantBufferRead          = _rtrcConstantBufferBase | _rtrcReadBase,                       // must be used together with stage mask
    TextureRead                 = _rtrcTextureBase | _rtrcReadBase,                             // must be used together with stage mask
    RWTextureRead               = _rtrcRWTextureBase | _rtrcReadBase,                           // must be used together with stage mask
    RWTextureWrite              = _rtrcRWTextureBase | _rtrcWriteBase,                          // must be used together with stage mask
    RWTextureReadWrite          = _rtrcRWTextureBase | _rtrcReadBase | _rtrcWriteBase,          // must be used together with stage mask
    BufferRead                  = _rtrcBufferBase | _rtrcReadBase,                              // must be used together with stage mask
    RWBufferRead                = _rtrcRWBufferBase | _rtrcReadBase,                            // must be used together with stage mask
    RWBufferWrite               = _rtrcRWBufferBase | _rtrcWriteBase,                           // must be used together with stage mask
    RWBufferReadWrite           = _rtrcRWBufferBase | _rtrcReadBase | _rtrcWriteBase,           // must be used together with stage mask
    StructuredBufferRead        = _rtrcStructuredBufferBase | _rtrcReadBase,                    // must be used together with stage mask
    RWStructuredBufferRead      = _rtrcRWStructuredBufferBase | _rtrcReadBase,                  // must be used together with stage mask
    RWStructuredBufferWrite     = _rtrcRWStructuredBufferBase | _rtrcWriteBase,                 // must be used together with stage mask
    RWStructuredBufferReadWrite = _rtrcRWStructuredBufferBase | _rtrcReadBase | _rtrcWriteBase, // must be used together with stage mask
    CopySrc                     = _rtrcCopyBase | _rtrcReadBase,
    CopyDst                     = _rtrcCopyBase | _rtrcWriteBase,
    ResolveSrc                  = _rtrcResolveBase | _rtrcReadBase,
    ResolveDst                  = _rtrcResolveBase | _rtrcWriteBase,
    Present                     = _rtrcPresentBase | _rtrcReadBase,

    VS = _rtrcVSBase,
    FS = _rtrcFSBase
};

RTRC_DEFINE_ENUM_FLAGS(ResourceState)

using ResourceStateFlag = EnumFlags<ResourceState>;

enum class AspectType : uint8_t
{
    Color   = 1 << 0,
    Depth   = 1 << 1,
    Stencil = 1 << 2
};

RTRC_DEFINE_ENUM_FLAGS(AspectType)

using AspectTypeFlag = EnumFlags<AspectType>;

enum class PipelineStage : uint32_t
{
    All                   = 1 << 0,
    None                  = 1 << 1,
    DrawIndirect          = 1 << 2,
    VertexInput           = 1 << 3,
    IndexInput            = 1 << 4,
    VertexShader          = 1 << 5,
    FragmentShader        = 1 << 6,
    EarlyFragmentTests    = 1 << 7,
    LateFragmentTests     = 1 << 8,
    ColorAttachmentOutput = 1 << 9,
    ComputeShader         = 1 << 10,
    Copy                  = 1 << 11,
    Resolve               = 1 << 12,
    Blit                  = 1 << 13,
    Clear                 = 1 << 14,
    Host                  = 1 << 15,
};

RTRC_DEFINE_ENUM_FLAGS(PipelineStage)

using PipelineStageFlag = EnumFlags<PipelineStage>;

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
    std::string                  name;
    BindingType                  type;
    ShaderStageFlag              shaderStages      = ShaderStageFlags::All;
    bool                         isArray           = false;
    uint32_t                     arraySize         = 1;
    const StructInfo            *structInfo        = nullptr;
    BindingTemplateParameterType templateParameter = BindingTemplateParameterType::Struct;

    std::strong_ordering operator<=>(const BindingDesc &other) const
    {
        const uint32_t thisArraySize = isArray ? arraySize : 0;
        const uint32_t otherArraySize = other.isArray ? other.arraySize : 0;
        return std::tie(name, type, shaderStages, isArray, thisArraySize) <=>
               std::tie(other.name, other.type, other.shaderStages, other.isArray, otherArraySize);
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
    std::vector<RC<BindingGroupLayout>> groups;

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
    ResourceState             initialState;
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

struct TextureTransitionBarrier
{
    RC<Texture>       texture;
    AspectTypeFlag    aspectTypeFlag;
    uint32_t          mipLevel;
    uint32_t          arrayLayer;
    ResourceStateFlag beforeState;
    ResourceStateFlag afterState;
};

struct TextureReleaseBarrier
{
    RC<Texture>       texture;
    AspectTypeFlag    aspectTypeFlag;
    uint32_t          mipLevel;
    uint32_t          arrayLayer;
    ResourceStateFlag beforeState;
    ResourceStateFlag afterState;
    RC<Queue>         beforeQueue;
    RC<Queue>         afterQueue;
};

using TextureAcquireBarrier = TextureReleaseBarrier;

struct BufferTransitionBarrier
{
    RC<Buffer>        buffer;
    ResourceStateFlag beforeState;
    ResourceStateFlag afterState;
};

struct BufferReleaseBarrier
{
    RC<Buffer>        buffer;
    ResourceStateFlag beforeState;
    ResourceStateFlag afterState;
    RC<Queue>         beforeQueue;
    RC<Queue>         afterQueue;
};

using BufferAcquireBarrier = BufferReleaseBarrier;

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
    RC<Texture2DRTV>  rtv;
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

// =============================== rhi interfaces ===============================

class RHIObject : public Uncopyable
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

    virtual RC<Device> CreateDevice(const DeviceDesc &desc = {}) = 0;
};

class Device : public RHIObject
{
public:

    virtual RC<Queue> GetQueue(QueueType type) = 0;

    virtual RC<Fence> CreateFence(bool signaled) = 0;

    virtual RC<Swapchain> CreateSwapchain(const SwapchainDesc &desc, Window &window) = 0;

    virtual RC<RawShader> CreateShader(const void *data, size_t size, std::string entryPoint, ShaderStage type) = 0;

    virtual RC<PipelineBuilder> CreatePipelineBuilder() = 0;

    virtual RC<BindingGroupLayout> CreateBindingGroupLayout(const BindingGroupLayoutDesc *desc) = 0;

    template<typename T>
    RC<BindingGroupLayout> CreateBindingGroupLayout();

    virtual RC<BindingLayout> CreateBindingLayout(const BindingLayoutDesc &desc) = 0;

    virtual RC<Texture> CreateTexture2D(const Texture2DDesc &desc) = 0;

    virtual RC<Buffer> CreateBuffer(const BufferDesc &desc) = 0;

    virtual void WaitIdle() = 0;
};

class BackBufferSemaphore : public RHIObject
{

};

class Swapchain : public RHIObject
{
public:

    virtual bool Acquire() = 0;

    virtual RC<BackBufferSemaphore> GetAcquireSemaphore() = 0;

    virtual RC<BackBufferSemaphore> GetPresentSemaphore() = 0;

    virtual void Present() = 0;

    virtual int GetRenderTargetCount() const = 0;

    virtual const Texture2DDesc &GetRenderTargetDesc() const = 0;

    virtual RC<Texture> GetRenderTarget() const = 0;
};

class BindingGroupLayout : public RHIObject
{
public:

    virtual RC<BindingGroup> CreateBindingGroup(bool updateAfterBind) = 0;
};

class BindingGroup : public RHIObject
{
public:

    virtual void ModifyMember(int index, const RC<BufferSRV> &bufferSRV) = 0;

    virtual void ModifyMember(int index, const RC<Texture2DSRV> &textureSRV) = 0;

    template<typename Struct, typename Member>
    void ModifyMember(Member Struct::*member, const RC<BufferSRV> &bufferSRV);

    template<typename Struct, typename Member>
    void ModifyMember(Member Struct::*member, const RC<Texture2DSRV> &textureSRV);
};

class BindingLayout : public RHIObject
{
    
};

class Queue : public RHIObject
{
public:

    virtual QueueType GetType() const = 0;

    virtual RC<CommandPool> CreateCommandPool() = 0;

    virtual void WaitIdle() = 0;

    virtual void Submit(
        const RC<BackBufferSemaphore> &waitBackBufferSemaphore,
        PipelineStage                  waitBackBufferStages,
        Span<RC<CommandBuffer>>        commandBuffers,
        const RC<BackBufferSemaphore> &signalBackBufferSemaphore,
        PipelineStage                  signalBackBufferStages,
        const RC<Fence>               &signalFence) = 0;
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

    virtual RC<CommandBuffer> NewCommandBuffer() = 0;
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

    virtual void BindPipeline(const RC<Pipeline> &pipeline) = 0;

    virtual void BindGroups(int startIndex, Span<RC<BindingGroup>> groups) = 0;

    virtual void SetViewports(Span<Viewport> viewports) = 0;

    virtual void SetScissors(Span<Scissor> scissor) = 0;

    virtual void SetViewportsWithCount(Span<Viewport> viewports) = 0;

    virtual void SetScissorsWithCount(Span<Scissor> scissors) = 0;

    virtual void Draw(int vertexCount, int instanceCount, int firstVertex, int firstInstance) = 0;
};

class RawShader : public RHIObject
{
public:

    virtual ShaderStage GetType() const = 0;
};

class Pipeline : public RHIObject
{
public:

};

class PipelineBuilder : public RHIObject
{
public:

    virtual PipelineBuilder &SetVertexShader(RC<RawShader> vertexShader) = 0;

    virtual PipelineBuilder &SetFragmentShader(RC<RawShader> fragmentShader) = 0;

    virtual PipelineBuilder &SetBindingLayout(RC<BindingLayout> layout) = 0;

    virtual PipelineBuilder &SetViewports(const Viewports &viewports) = 0;

    virtual PipelineBuilder &SetScissors(const Scissors &scissors) = 0;

    // default is trianglelist
    virtual PipelineBuilder &SetPrimitiveTopology(PrimitiveTopology topology) = 0;

    // default is fill
    virtual PipelineBuilder &SetFillMode(FillMode mode) = 0;

    // default is cullnone
    virtual PipelineBuilder &SetCullMode(CullMode mode) = 0;

    // default is cw
    virtual PipelineBuilder &SetFrontFace(FrontFaceMode mode) = 0;

    // default is 0
    virtual PipelineBuilder &SetDepthBias(float constFactor, float slopeFactor, float clamp) = 0;

    // default is 1
    virtual PipelineBuilder &SetMultisample(int sampleCount) = 0;

    // default is disabled
    virtual PipelineBuilder &SetDepthTest(bool enableTest, bool enableWrite, CompareOp compareOp) = 0;

    // default is disabled
    virtual PipelineBuilder &SetStencilTest(bool enableTest) = 0;

    // default is keep, keep, keep, always, 0xff, 0xff
    virtual PipelineBuilder &SetStencilFrontOp(
        StencilOp depthFailOp,
        StencilOp failOp,
        StencilOp passOp,
        CompareOp compareOp,
        uint32_t  compareMask,
        uint32_t  writeMask) = 0;

    // default is keep, keep, keep, always, 0xff, 0xff
    virtual PipelineBuilder &SetStencilBackOp(
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
    virtual PipelineBuilder &SetBlending(
        bool        enableBlending,
        BlendFactor srcColorFactor,
        BlendFactor dstColorFactor,
        BlendOp     colorOp,
        BlendFactor srcAlphaFactor,
        BlendFactor dstAlphaFactor,
        BlendOp     alphaOp) = 0;

    // default is empty
    virtual PipelineBuilder &AddColorAttachment(Format format) = 0;

    // default is undefined
    virtual PipelineBuilder &SetDepthStencilAttachment(Format format) = 0;

    virtual RC<Pipeline> CreatePipeline() const = 0;
};

class Texture : public RHIObject
{
public:

    virtual TextureDimension GetDimension() const = 0;

    virtual const Texture2DDesc &Get2DDesc() const = 0;

    virtual RC<Texture2DRTV> Create2DRTV(const Texture2DRTVDesc &desc) const = 0;

    virtual RC<Texture2DSRV> Create2DSRV(const Texture2DSRVDesc &desc) const = 0;
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

class Buffer : public RHIObject
{
public:

    virtual const BufferDesc &GetDesc() const = 0;

    virtual RC<BufferSRV> CreateSRV(const BufferSRVDesc &desc) const = 0;

    virtual void *Map(size_t offset, size_t size) const = 0;

    virtual void Unmap() = 0;
};

class BufferSRV : public RHIObject
{
public:

    virtual const BufferSRVDesc &GetDesc() const = 0;
};

// =============================== vulkan backend ===============================

// can be called multiple times
void InitializeVulkanBackend();

#ifdef RTRC_RHI_VULKAN

struct VulkanInstanceDesc
{
    std::vector<std::string> extensions;
    bool debugMode = false;
};

Unique<Instance> CreateVulkanInstance(const VulkanInstanceDesc &desc);

#endif

RTRC_RHI_END
