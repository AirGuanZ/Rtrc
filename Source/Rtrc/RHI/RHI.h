#pragma once

#include <vector>

#include <Rtrc/Utils/EnumFlags.h>
#include <Rtrc/Utils/Uncopyable.h>
#include <Rtrc/Utils/Variant.h>
#include <Rtrc/Window/Window.h>

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
class BindingGroupInstance;
class BindingLayout;
class Pipeline;
class PipelineBuilder;
class Texture;
class Texture2DRTV;
class Texture2DSRV;
class Buffer;

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
    B8G8R8A8_UNorm
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
    constexpr auto PS  = ShaderStageFlag(ShaderStage::FragmentShader);
    constexpr auto All = VS | PS;

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
    Texture,
    RWTexture,
    Buffer,
    StructuredBuffer,
    RWBuffer,
    RWStructuredBuffer,
    ConstantBuffer,
    Sampler,
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
    ShaderUniformBuffer,
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

enum class TextureLayout
{
    Undefined,
    RenderTarget,
    ShaderResource,
    DepthStencilReadWrite,
    DepthStencilReadOnly,
    DepthReadOnlyStencilReadWrite,
    DepthReadWriteStencilReadOnly,
    DepthReadWrite,
    DepthReadOnly,
    StencilReadWrite,
    StencilReadOnly,
    TransferDst,
    TransferSrc,
    Present
};

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

enum class AccessType : uint32_t
{
    None                      = 1 << 0,
    IndirectCommandRead       = 1 << 1,
    IndexRead                 = 1 << 2,
    VertexAttributeRead       = 1 << 3,
    UniformBufferRead         = 1 << 4,
    ImageRead                 = 1 << 5,
    BufferRead                = 1 << 6,
    StructuredBufferRead      = 1 << 7,
    RWImageRead               = 1 << 8,
    RWBufferRead              = 1 << 9,
    RWStructuredBufferRead    = 1 << 10,
    RWImageWrite              = 1 << 11,
    RWBufferWrite             = 1 << 12,
    RWStructuredBufferWrite   = 1 << 13,
    ColorAttachmentRead       = 1 << 14,
    ColorAttachmentWrite      = 1 << 15,
    DepthStencilRead          = 1 << 16,
    DepthStencilWrite         = 1 << 17,
    TransferRead              = 1 << 18,
    TransferWrite             = 1 << 19,
    HostRead                  = 1 << 20,
    HostWrite                 = 1 << 21,
};

RTRC_DEFINE_ENUM_FLAGS(AccessType)

using AccessTypeFlag = EnumFlags<AccessType>;

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
    BindingType           type;
    EnumFlags<ShaderStage> shaderStages;
    bool                  isArray;
    uint32_t              arraySize;
};

using AliasedBindingsDesc = std::vector<BindingDesc>;

struct BindingGroupLayoutDesc
{
    std::vector<AliasedBindingsDesc> bindings;
};

struct BindingLayoutDesc
{
    std::vector<RC<BindingGroupLayout>> groups;
};

struct Viewport
{
    Vector2i upperLeftCorner;
    Vector2i size;
    float minDepth;
    float maxDepth;
};

struct Texture2DDesc
{
    Format         format;
    uint32_t       width;
    uint32_t       height;
    uint32_t       mipLevels;
    uint32_t       arraySize;
    uint32_t       sampleCount;
    TextureUsageFlag usage;
    TextureLayout    initialLayout;
};

struct Texture2DRTVDesc
{
    Format format;
    uint32_t mipLevel;
    uint32_t arrayLayer;
};

struct Texture2DSRVDesc
{
    Format format;
    uint32_t baseMipLevel;
    uint32_t levelCount;
    uint32_t baseArrayLayer;
    uint32_t layerCount;
};

struct BufferDesc
{
    size_t size;
    BufferUsageFlag usage;
    BufferHostAccessType hostAccessType;
};

struct TextureTransitionBarrier
{
    RC<Texture>       texture;
    AspectTypeFlag    aspectTypeFlag;
    PipelineStageFlag beforeStages;
    PipelineStageFlag afterStages;
    AccessTypeFlag    beforeAccess;
    AccessTypeFlag    afterAccess;
    TextureLayout       beforeLayout;
    TextureLayout       afterLayout;
    uint32_t          mipLevel;
    uint32_t          arrayLayer;
};

struct BufferTransitionBarrier
{
    RC<Buffer>        buffer;
    PipelineStageFlag beforeStages;
    PipelineStageFlag afterStages;
    AccessTypeFlag    beforeAccess;
    AccessTypeFlag    afterAccess;
};

struct ColorClearValue
{
    float r, g, b, a;
};

struct DepthStencilClearValue
{
    float depth;
    uint32_t stencil;
};

using ClearValue = Variant<ColorClearValue, DepthStencilClearValue>;

struct RenderPassColorAttachment
{
    RC<Texture2DRTV>  rtv;
    TextureLayout     layout;
    AttachmentLoadOp  loadOp;
    AttachmentStoreOp storeOp;
    ClearValue        clearValue;
};

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

    RC<RawShader> CreateVertexShader(const void *data, size_t size, std::string entryPoint)
    {
        return this->CreateShader(data, size, std::move(entryPoint), ShaderStage::VertexShader);
    }

    RC<RawShader> CreateFragmentShader(const void *data, size_t size, std::string entryPoint)
    {
        return this->CreateShader(data, size, std::move(entryPoint), ShaderStage::FragmentShader);
    }

    virtual RC<PipelineBuilder> CreatePipelineBuilder() = 0;

    virtual RC<BindingGroupLayout> CreateBindingGroupLayout(const BindingGroupLayoutDesc &desc) = 0;

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

    virtual RC<Texture2DRTV> GetRenderTargetView() const = 0;
};

class BindingGroupLayout : public RHIObject
{
public:

    virtual RC<BindingGroupInstance> CreateBindingGroup(bool updateAfterBind) = 0;
};

class BindingGroupInstance : public RHIObject
{
    
};

class BindingLayout : public RHIObject
{
    
};

class Queue : public RHIObject
{
public:

    virtual RC<CommandPool> CreateCommandPool() = 0;

    virtual void Submit(
        const RC<BackBufferSemaphore>        &waitBackBufferSemaphore,
        PipelineStage                         waitBackBufferStages,
        const std::vector<RC<CommandBuffer>> &commandBuffers,
        const RC<BackBufferSemaphore>        &signalBackBufferSemaphore,
        PipelineStage                         signalBackBufferStages,
        const RC<Fence>                      &signalFence) = 0;
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
        const std::vector<TextureTransitionBarrier> &textureTransitions,
        const std::vector<BufferTransitionBarrier> &bufferTransitions) = 0;

    virtual void BeginRenderPass(const std::vector<RenderPassColorAttachment> &colorAttachments) = 0;

    virtual void EndRenderPass() = 0;
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
