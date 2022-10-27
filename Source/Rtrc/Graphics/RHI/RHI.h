#pragma once

#include <array>
#include <optional>
#include <vector>

#include <Rtrc/Math/Vector4.h>
#include <Rtrc/Utils/EnumFlags.h>
#include <Rtrc/Utils/ReferenceCounted.h>
#include <Rtrc/Utils/Span.h>
#include <Rtrc/Utils/Uncopyable.h>
#include <Rtrc/Utils/Variant.h>
#include <Rtrc/Window/Window.h>

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

RTRC_RHI_FORWARD_DECL(RHIObject)
RTRC_RHI_FORWARD_DECL(Instance)
RTRC_RHI_FORWARD_DECL(Device)
RTRC_RHI_FORWARD_DECL(BackBufferSemaphore)
RTRC_RHI_FORWARD_DECL(Semaphore)
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
RTRC_RHI_FORWARD_DECL(Resource)
RTRC_RHI_FORWARD_DECL(Texture)
RTRC_RHI_FORWARD_DECL(TextureRTV)
RTRC_RHI_FORWARD_DECL(TextureSRV)
RTRC_RHI_FORWARD_DECL(TextureUAV)
RTRC_RHI_FORWARD_DECL(Buffer)
RTRC_RHI_FORWARD_DECL(BufferSRV)
RTRC_RHI_FORWARD_DECL(BufferUAV)
RTRC_RHI_FORWARD_DECL(Sampler)
RTRC_RHI_FORWARD_DECL(MemoryPropertyRequirements)
RTRC_RHI_FORWARD_DECL(MemoryBlock)

#undef RTRC_RHI_FORWARD_DECL

struct GraphicsPipelineDesc;
struct ComputePipelineDesc;

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
    R10G10B10A2_UNorm,
};

enum class VertexAttributeType : uint32_t
{
    UInt,
    UInt2,
    UInt3,
    UInt4,
    Int,
    Int2,
    Int3,
    Int4,
    Float,
    Float2,
    Float3,
    Float4
};

const char *GetFormatName(Format format);

size_t GetTexelSize(Format format);

enum class IndexBufferFormat
{
    UInt16,
    UInt32
};

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
    Texture3D,
    RWTexture3D,
    Texture2DArray,
    RWTexture2DArray,
    Texture3DArray,
    RWTexture3DArray,
    Buffer,
    StructuredBuffer,
    RWBuffer,
    RWStructuredBuffer,
    ConstantBuffer,
    Sampler,
};

inline const char *GetBindingTypeName(BindingType type)
{
    static const char *NAMES[] =
    {
        "Texture2D",
        "RWTexture2D",
        "Texture3D",
        "RWTexture3D",
        "Texture2DArray",
        "RWTexture2DArray",
        "Texture3DArray",
        "RWTexture3DArray",
        "Buffer",
        "StructuredBuffer",
        "RWBuffer",
        "RWStructuredBuffer",
        "ConstantBuffer",
        "Sampler",
    };
    assert(static_cast<int>(type) < GetArraySize<int>(NAMES));
    return NAMES[static_cast<int>(type)];
}

enum class TextureDimension
{
    Tex2D,
    Tex3D
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
    ClearDst,
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
    Clear          = 1 << 7,
    Resolve        = 1 << 8
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
    ClearWrite              = 1 << 19
};

RTRC_DEFINE_ENUM_FLAGS(ResourceAccess)

using ResourceAccessFlag = EnumFlags<ResourceAccess>;

bool IsReadOnly(ResourceAccessFlag access);
bool IsWriteOnly(ResourceAccessFlag access);

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

    auto operator<=>(const BindingDesc &other) const = default;
    bool operator==(const BindingDesc &) const = default;
};

using AliasedBindingsDesc = std::vector<BindingDesc>;

struct BindingGroupLayoutDesc
{
    std::string name;
    std::vector<AliasedBindingsDesc> bindings;

    auto operator<=>(const BindingGroupLayoutDesc &other) const = default;
    bool operator==(const BindingGroupLayoutDesc &) const = default;
};

struct BindingLayoutDesc
{
    std::vector<Ptr<BindingGroupLayout>> groups;

    auto operator<=>(const BindingLayoutDesc &) const = default;
    bool operator==(const BindingLayoutDesc &) const = default;
};

struct TextureSubresource
{
    uint32_t mipLevel;
    uint32_t arrayLayer;

    auto operator<=>(const TextureSubresource &) const = default;
    bool operator==(const TextureSubresource &) const = default;
};

struct TextureSubresources
{
    uint32_t mipLevel   = 0;
    uint32_t levelCount = 1;
    uint32_t arrayLayer = 0;
    uint32_t layerCount = 1;
};

struct TextureDesc
{
    TextureDimension dim;
    Format           format;
    uint32_t         width;
    uint32_t         height;
    union
    {
        uint32_t arraySize;
        uint32_t depth;
    };
    uint32_t                  mipLevels;
    uint32_t                  sampleCount;
    TextureUsageFlag          usage;
    TextureLayout             initialLayout;
    QueueConcurrentAccessMode concurrentAccessMode;

    auto operator<=>(const TextureDesc &rhs) const
    {
        return std::make_tuple(
                    dim, format, width, height, arraySize, mipLevels,
                    sampleCount, usage, initialLayout, concurrentAccessMode)
           <=> std::make_tuple(
                    rhs.dim, rhs.format, rhs.width, rhs.height, rhs.arraySize, rhs.mipLevels,
                    rhs.sampleCount, rhs.usage, rhs.initialLayout, rhs.concurrentAccessMode);
    }
};

struct TextureRTVDesc
{
    Format   format     = Format::Unknown;
    uint32_t mipLevel   = 0;
    uint32_t arrayLayer = 0;
};

struct TextureSRVDesc
{
    bool                 isArray        = false;
    Format               format         = Format::Unknown;
    uint32_t             baseMipLevel   = 0;
    uint32_t             levelCount     = 0; // all levels
    uint32_t             baseArrayLayer = 0;
    uint32_t             layerCount     = 0; // 0 means all layers. only used when isArray == true
};

struct TextureUAVDesc
{
    bool                 isArray        = false;
    Format               format         = Format::Unknown;
    uint32_t             mipLevel       = 0;
    uint32_t             baseArrayLayer = 0;
    uint32_t             layerCount     = 0; // 0 means all layers. only used when isArray == true
};

struct BufferDesc
{
    size_t               size;
    BufferUsageFlag      usage;
    BufferHostAccessType hostAccessType;

    auto operator<=>(const BufferDesc &) const = default;
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
    FilterMode magFilter = FilterMode::Point;
    FilterMode minFilter = FilterMode::Point;
    FilterMode mipFilter = FilterMode::Point;

    AddressMode addressModeU = AddressMode::Clamp;
    AddressMode addressModeV = AddressMode::Clamp;
    AddressMode addressModeW = AddressMode::Clamp;

    float mipLODBias = 0.0f;
    float minLOD     = 0.0f;
    float maxLOD     = (std::numeric_limits<float>::max)();

    bool enableAnisotropy = false;
    int  maxAnisotropy    = 0;

    bool      enableComparision = false;
    CompareOp compareOp         = CompareOp::Less;

    std::array<float, 4> borderColor = { 0, 0, 0, 0 };

    auto operator<=>(const SamplerDesc &) const = default;
    bool operator==(const SamplerDesc &) const = default;
};

struct TextureTransitionBarrier
{
    Texture                *texture;
    TextureSubresources     subresources;
    PipelineStageFlag       beforeStages;
    ResourceAccessFlag      beforeAccesses;
    TextureLayout           beforeLayout;
    PipelineStageFlag       afterStages;
    ResourceAccessFlag      afterAccesses;
    TextureLayout           afterLayout;
};

// for non-sharing resource, every cross-queue sync needs a release/acquire pair
struct TextureReleaseBarrier
{
    Texture                *texture;
    TextureSubresources     subresources;
    PipelineStageFlag       beforeStages;
    ResourceAccessFlag      beforeAccesses;
    TextureLayout           beforeLayout;
    TextureLayout           afterLayout;
    QueueType               beforeQueue;
    QueueType               afterQueue;
};

struct TextureAcquireBarrier
{
    Texture                *texture;
    TextureSubresources     subresources;
    TextureLayout           beforeLayout;
    PipelineStageFlag       afterStages;
    ResourceAccessFlag      afterAccesses;
    TextureLayout           afterLayout;
    QueueType               beforeQueue;
    QueueType               afterQueue;
};

struct BufferTransitionBarrier
{
    Buffer            *buffer;
    PipelineStageFlag  beforeStages;
    ResourceAccessFlag beforeAccesses;
    PipelineStageFlag  afterStages;
    ResourceAccessFlag afterAccesses;
};

struct Viewport
{
    Vector2f lowerLeftCorner;
    Vector2f size;
    float minDepth;
    float maxDepth;

    auto operator<=>(const Viewport &) const = default;

    size_t Hash() const { return ::Rtrc::Hash(lowerLeftCorner, size, minDepth, maxDepth); }

    static Viewport Create(const TextureDesc &desc, float minDepth = 0, float maxDepth = 1)
    {
        return Viewport{
            .lowerLeftCorner = { 0, 0 },
            .size            = { static_cast<float>(desc.width), static_cast<float>(desc.height) },
            .minDepth        = minDepth,
            .maxDepth        = maxDepth
        };
    }

    static Viewport Create(const TexturePtr &tex, float minDepth = 0, float maxDepth = 1);
};

struct Scissor
{
    Vector2i lowerLeftCorner;
    Vector2i size;

    auto operator<=>(const Scissor &) const = default;

    size_t Hash() const { return ::Rtrc::Hash(lowerLeftCorner, size); }

    static Scissor Create(const TextureDesc &desc)
    {
        return Scissor{ { 0, 0 }, { static_cast<int>(desc.width), static_cast<int>(desc.height) } };
    }

    static Scissor Create(const TexturePtr &tex);
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
    TextureRTV       *renderTargetView;
    AttachmentLoadOp  loadOp;
    AttachmentStoreOp storeOp;
    ClearValue        clearValue;
};

struct DynamicViewportCount
{
    auto operator<=>(const DynamicViewportCount &) const = default;
    size_t Hash() const { return 1; }
};

struct DynamicScissorCount
{
    auto operator<=>(const DynamicScissorCount &) const = default;
    size_t Hash() const { return 2; }
};

// fixed viewports; dynamic viewports with fixed count; dynamic count
using Viewports = Variant<std::monostate, std::vector<Viewport>, int, DynamicViewportCount>;

// fixed viewports; dynamic viewports with fixed count; dynamic count
using Scissors = Variant<std::monostate, std::vector<Scissor>, int, DynamicScissorCount>;

struct VertexInputBuffer
{
    uint32_t elementSize;
    bool     perInstance;
};

struct VertexInputAttribute
{
    uint32_t location;
    uint32_t inputBufferIndex;
    uint32_t byteOffsetInBuffer;
    VertexAttributeType type;
};

struct MemoryBlockDesc
{
    size_t size;
    size_t alignment;
    Ptr<MemoryPropertyRequirements> properties;
};

struct DebugLabel
{
    std::string name;
    std::optional<Vector4f> color;
};

struct GraphicsPipelineDesc
{
    struct StencilOps
    {
        StencilOp depthFailOp = StencilOp::Keep;
        StencilOp failOp      = StencilOp::Keep;
        StencilOp passOp      = StencilOp::Keep;
        CompareOp compareOp   = CompareOp::Always;
        uint32_t  compareMask = 0xff;
        uint32_t  writeMask   = 0xff;

        auto operator<=>(const StencilOps &) const = default;

        size_t Hash() const
        {
            return ::Rtrc::Hash(depthFailOp, failOp, passOp, compareOp, compareMask, writeMask);
        }
    };

    Ptr<RawShader> vertexShader;
    Ptr<RawShader> fragmentShader;

    Ptr<BindingLayout> bindingLayout;

    Viewports viewports;
    Scissors  scissors;

    std::vector<VertexInputBuffer>    vertexBuffers;
    std::vector<VertexInputAttribute> vertexAttributs;

    PrimitiveTopology primitiveTopology = PrimitiveTopology::TriangleList;
    FillMode          fillMode          = FillMode::Fill;
    CullMode          cullMode          = CullMode::DontCull;
    FrontFaceMode     frontFaceMode     = FrontFaceMode::Clockwise;

    bool  enableDepthBias      = false;
    float depthBiasConstFactor = 0;
    float depthBiasSlopeFactor = 0;
    float depthBiasClampValue  = 0;

    int multisampleCount = 1;

    bool      enableDepthTest  = false;
    bool      enableDepthWrite = false;
    CompareOp depthCompareOp   = CompareOp::Always;

    bool       enableStencilTest = false;
    StencilOps frontStencilOp;
    StencilOps backStencilOp;

    bool        enableBlending         = false;
    BlendFactor blendingSrcColorFactor = BlendFactor::One;
    BlendFactor blendingDstColorFactor = BlendFactor::Zero;
    BlendFactor blendingSrcAlphaFactor = BlendFactor::One;
    BlendFactor blendingDstAlphaFactor = BlendFactor::Zero;
    BlendOp     blendingColorOp        = BlendOp::Add;
    BlendOp     blendingAlphaOp        = BlendOp::Add;

    StaticVector<Format, 8> colorAttachments;
    Format                  depthStencilFormat = Format::Unknown;
};

struct ComputePipelineDesc
{
    Ptr<RawShader> computeShader;
    Ptr<BindingLayout> bindingLayout;
};

// =============================== rhi interfaces ===============================

class RHIObject : public ReferenceCounted, public Uncopyable
{
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

    virtual Ptr<CommandPool> CreateCommandPool(const Ptr<Queue> &queue) = 0;

    virtual Ptr<Swapchain> CreateSwapchain(const SwapchainDesc &desc, Window &window) = 0;

    virtual Ptr<Fence>     CreateFence(bool signaled) = 0;
    virtual Ptr<Semaphore> CreateSemaphore(uint64_t initialValue) = 0;

    virtual Ptr<RawShader> CreateShader(const void *data, size_t size, std::string entryPoint, ShaderStage type) = 0;

    virtual Ptr<GraphicsPipeline> CreateGraphicsPipeline(const GraphicsPipelineDesc &desc) = 0;
    virtual Ptr<ComputePipeline>  CreateComputePipeline (const ComputePipelineDesc &desc) = 0;

    virtual Ptr<BindingGroupLayout> CreateBindingGroupLayout(const BindingGroupLayoutDesc &desc) = 0;
    virtual Ptr<BindingGroup>       CreateBindingGroup(const Ptr<BindingGroupLayout> &bindingGroupLayout) = 0;
    virtual Ptr<BindingLayout>      CreateBindingLayout(const BindingLayoutDesc &desc) = 0;

    virtual Ptr<Texture> CreateTexture(const TextureDesc &desc) = 0;
    virtual Ptr<Buffer>  CreateBuffer(const BufferDesc &desc) = 0;
    virtual Ptr<Sampler> CreateSampler(const SamplerDesc &desc) = 0;

    virtual Ptr<MemoryPropertyRequirements> GetMemoryRequirements(
        const TextureDesc &desc, size_t *size, size_t *alignment) const = 0;
    virtual Ptr<MemoryPropertyRequirements> GetMemoryRequirements(
        const BufferDesc &desc, size_t *size, size_t *alignment) const = 0;
    virtual Ptr<MemoryBlock> CreateMemoryBlock(const MemoryBlockDesc &desc) = 0;

    virtual Ptr<Texture> CreatePlacedTexture(
        const TextureDesc &desc, const Ptr<MemoryBlock> &memoryBlock, size_t offsetInMemoryBlock) = 0;
    virtual Ptr<Buffer> CreatePlacedBuffer(
        const BufferDesc &desc, const Ptr<MemoryBlock> &memoryBlock, size_t offsetInMemoryBlock) = 0;

    virtual size_t GetConstantBufferAlignment() const = 0;

    virtual void WaitIdle() = 0;
};

class BackBufferSemaphore : public RHIObject
{

};

class Semaphore : public RHIObject
{
public:

    virtual uint64_t GetValue() const = 0;
};

class Swapchain : public RHIObject
{
public:

    virtual bool Acquire() = 0;
    virtual void Present() = 0;

    virtual Ptr<BackBufferSemaphore> GetAcquireSemaphore() = 0;
    virtual Ptr<BackBufferSemaphore> GetPresentSemaphore() = 0;

    virtual int                GetRenderTargetCount() const = 0;
    virtual const TextureDesc &GetRenderTargetDesc() const = 0;
    virtual Ptr<Texture>       GetRenderTarget() const = 0;
};

class BindingGroupLayout : public RHIObject
{
public:

    virtual const BindingGroupLayoutDesc &GetDesc() const = 0;
};

class BindingGroup : public RHIObject
{
public:

    struct UniformBufferModifyParameters
    {
        Ptr<Buffer> buffer;
        size_t      offset;
        size_t      range;
    };

    virtual const BindingGroupLayout *GetLayout() const = 0;

    virtual void ModifyMember(int index, const Ptr<BufferSRV>  &bufferSRV) = 0;
    virtual void ModifyMember(int index, const Ptr<BufferUAV>  &bufferUAV) = 0;
    virtual void ModifyMember(int index, const Ptr<TextureSRV> &textureSRV) = 0;
    virtual void ModifyMember(int index, const Ptr<TextureUAV> &textureUAV) = 0;
    virtual void ModifyMember(int index, const Ptr<Sampler>    &sampler) = 0;
    virtual void ModifyMember(int index, const Ptr<Buffer>     &uniformBuffer, size_t offset, size_t range) = 0;
};

class BindingLayout : public RHIObject
{

};

class Queue : public RHIObject
{
public:

    struct BackBufferSemaphoreDependency
    {
        Ptr<BackBufferSemaphore> semaphore;
        PipelineStageFlag        stages;
    };

    struct SemaphoreDependency
    {
        Ptr<Semaphore>    semaphore;
        PipelineStageFlag stages;
        uint64_t          value;
    };

    virtual QueueType GetType() const = 0;

    virtual void WaitIdle() = 0;

    virtual void Submit(
        BackBufferSemaphoreDependency waitBackBufferSemaphore,
        Span<SemaphoreDependency>     waitSemaphores,
        Span<Ptr<CommandBuffer>>      commandBuffers,
        BackBufferSemaphoreDependency signalBackBufferSemaphore,
        Span<SemaphoreDependency>     signalSemaphores,
        const Ptr<Fence>             &signalFence) = 0;
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

    virtual QueueType GetType() const = 0;

    virtual Ptr<CommandBuffer> NewCommandBuffer() = 0;
};

class CommandBuffer : public RHIObject
{
public:

    virtual void Begin() = 0;

    virtual void End() = 0;

    template<typename...Ts>
    void ExecuteBarriers(const Ts&...ts);

    virtual void BeginRenderPass(Span<RenderPassColorAttachment> colorAttachments) = 0;
    virtual void EndRenderPass() = 0;

    virtual void BindPipeline(const Ptr<GraphicsPipeline> &pipeline) = 0;
    virtual void BindPipeline(const Ptr<ComputePipeline>  &pipeline) = 0;

    virtual void BindGroupsToGraphicsPipeline(int startIndex, Span<RC<BindingGroup>> groups) = 0;
    virtual void BindGroupsToComputePipeline (int startIndex, Span<RC<BindingGroup>> groups) = 0;

    virtual void BindGroupToGraphicsPipeline(int index, const Ptr<BindingGroup> &group) = 0;
    virtual void BindGroupToComputePipeline (int index, const Ptr<BindingGroup> &group) = 0;

    virtual void SetViewports(Span<Viewport> viewports) = 0;
    virtual void SetScissors (Span<Scissor> scissor) = 0;

    virtual void SetViewportsWithCount(Span<Viewport> viewports) = 0;
    virtual void SetScissorsWithCount (Span<Scissor> scissors) = 0;

    virtual void SetVertexBuffer(int slot, Span<BufferPtr> buffers, Span<size_t> byteOffsets) = 0;
    virtual void SetIndexBuffer(const BufferPtr &buffer, size_t byteOffset, IndexBufferFormat format) = 0;

    virtual void Draw(int vertexCount, int instanceCount, int firstVertex, int firstInstance) = 0;
    virtual void DrawIndexed(int indexCount, int instanceCount, int firstIndex, int firstVertex, int firstInstance) = 0;

    virtual void Dispatch(int groupCountX, int groupCountY, int groupCountZ) = 0;

    virtual void CopyBuffer(Buffer *dst, size_t dstOffset, Buffer *src, size_t srcOffset, size_t range) = 0;
    virtual void CopyBufferToColorTexture2D(
        Texture *dst, uint32_t mipLevel, uint32_t arrayLayer, Buffer *src, size_t srcOffset) = 0;
    virtual void CopyColorTexture2DToBuffer(
        Buffer *dst, size_t dstOffset, Texture *src, uint32_t mipLevel, uint32_t arrayLayer) = 0;

    virtual void ClearColorTexture2D(Texture *dst, const ColorClearValue &clearValue) = 0;

    virtual void BeginDebugEvent(const DebugLabel &label) = 0;
    virtual void EndDebugEvent() = 0;

protected:

    virtual void ExecuteBarriersInternal(
        Span<TextureTransitionBarrier> textureTransitions,
        Span<BufferTransitionBarrier>  bufferTransitions,
        Span<TextureReleaseBarrier>    textureReleaseBarriers,
        Span<TextureAcquireBarrier>    textureAcquireBarriers) = 0;
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

class GraphicsPipelineBuilder : GraphicsPipelineDesc
{
    friend class Device;

public:

    GraphicsPipelineBuilder &SetVertexShader(Ptr<RawShader> vertexShader);
    GraphicsPipelineBuilder &SetFragmentShader(Ptr<RawShader> fragmentShader);

    GraphicsPipelineBuilder &SetBindingLayout(Ptr<BindingLayout> layout);

    GraphicsPipelineBuilder &SetViewports(const Viewports &viewports);
    GraphicsPipelineBuilder &SetScissors (const Scissors &scissors);

    GraphicsPipelineBuilder &AddVertexInputBuffers(Span<VertexInputBuffer> buffers);
    GraphicsPipelineBuilder &AddVertexInputAttributes(Span<VertexInputAttribute> attributes);

    // default is trianglelist
    GraphicsPipelineBuilder &SetPrimitiveTopology(PrimitiveTopology topology);

    // default is fill
    GraphicsPipelineBuilder &SetFillMode(FillMode mode);

    // default is cullnone
    GraphicsPipelineBuilder &SetCullMode(CullMode mode);

    // default is cw
    GraphicsPipelineBuilder &SetFrontFace(FrontFaceMode mode);

    // default is 0
    GraphicsPipelineBuilder &SetDepthBias(float constFactor, float slopeFactor, float clamp);

    // default is 1
    GraphicsPipelineBuilder &SetMultisample(int sampleCount);

    // default is disabled
    GraphicsPipelineBuilder &SetDepthTest(bool enableTest, bool enableWrite, CompareOp compareOp);

    // default is disabled
    GraphicsPipelineBuilder &SetStencilTest(bool enableTest);

    // default is keep, keep, keep, always, 0xff, 0xff
    GraphicsPipelineBuilder &SetStencilFrontOp(
        StencilOp depthFailOp,
        StencilOp failOp,
        StencilOp passOp,
        CompareOp compareOp,
        uint32_t  compareMask,
        uint32_t  writeMask);

    // default is keep, keep, keep, always, 0xff, 0xff
    GraphicsPipelineBuilder &SetStencilBackOp(
        StencilOp depthFailOp,
        StencilOp failOp,
        StencilOp passOp,
        CompareOp compareOp,
        uint32_t  compareMask,
        uint32_t  writeMask);

    // default is disabled
    // src: from fragment shader
    // dst: from render target
    // out = srcFactor * src op dstFactor * dst
    GraphicsPipelineBuilder &SetBlending(
        bool        enableBlending,
        BlendFactor srcColorFactor,
        BlendFactor dstColorFactor,
        BlendOp     colorOp,
        BlendFactor srcAlphaFactor,
        BlendFactor dstAlphaFactor,
        BlendOp     alphaOp);

    // default is empty
    GraphicsPipelineBuilder &AddColorAttachment(Format format);

    // default is undefined
    GraphicsPipelineBuilder &SetDepthStencilAttachment(Format format);

    Ptr<GraphicsPipeline> CreatePipeline(const DevicePtr &device) const;
};

class ComputePipeline : public RHIObject
{
public:

    virtual const Ptr<BindingLayout> &GetBindingLayout() const = 0;
};

class ComputePipelineBuilder : public RHIObject, ComputePipelineDesc
{
public:

    ComputePipelineBuilder &SetComputeShader(Ptr<RawShader> shader);

    ComputePipelineBuilder &SetBindingLayout(Ptr<BindingLayout> layout);

    Ptr<ComputePipeline> CreatePipeline(const DevicePtr &device) const;
};

class Resource : public RHIObject
{
public:

    virtual bool IsBuffer() const { return false; }
    virtual BufferPtr AsBuffer() { return nullptr; }

    virtual bool IsTexture() const { return false; }
    virtual TexturePtr AsTexture() { return nullptr; }
};

class Texture : public Resource
{
public:

    bool IsTexture() const final { return true; }
    TexturePtr AsTexture() final { return TexturePtr(this); }

    QueueConcurrentAccessMode GetConcurrentAccessMode() const { return GetDesc().concurrentAccessMode; }

    TextureDimension GetDimension() const { return GetDesc().dim; }
    Format           GetFormat()    const { return GetDesc().format; }
    uint32_t         GetMipLevels() const { return GetDesc().mipLevels; }
    uint32_t         GetArraySize() const { return GetDesc().arraySize; }
    uint32_t         GetWidth()     const { return GetDesc().width; }
    uint32_t         GetHeight()    const { return GetDesc().height; }
    uint32_t         GetDepth()     const { return GetDesc().depth; }

    virtual const TextureDesc &GetDesc() const = 0;

    virtual Ptr<TextureRTV> CreateRTV(const TextureRTVDesc &desc = {}) const = 0;
    virtual Ptr<TextureSRV> CreateSRV(const TextureSRVDesc &desc = {}) const = 0;
    virtual Ptr<TextureUAV> CreateUAV(const TextureUAVDesc &desc = {}) const = 0;
};

class TextureRTV : public RHIObject
{
public:

    virtual const TextureRTVDesc &GetDesc() const = 0;
};

class TextureSRV : public RHIObject
{
public:

    virtual const TextureSRVDesc &GetDesc() const = 0;
};

class TextureUAV : public RHIObject
{
public:

    virtual const TextureUAVDesc &GetDesc() const = 0;
};

class Buffer : public Resource
{
public:

    bool IsBuffer() const final { return true; }
    BufferPtr AsBuffer() override { return BufferPtr(this); }

    virtual const BufferDesc &GetDesc() const = 0;

    virtual Ptr<BufferSRV> CreateSRV(const BufferSRVDesc &desc) const = 0;
    virtual Ptr<BufferUAV> CreateUAV(const BufferUAVDesc &desc) const = 0;

    virtual void *Map(size_t offset, size_t size, bool invalidate = false) = 0;
    virtual void Unmap(size_t offset, size_t size, bool flush = false) = 0;
    virtual void InvalidateBeforeRead(size_t offset, size_t size) = 0;
    virtual void FlushAfterWrite(size_t offset, size_t size) = 0;
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
    bool debugMode = RTRC_DEBUG;
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

// =============================== inlined implementation ===============================

inline Viewport Viewport::Create(const TexturePtr &tex, float minDepth, float maxDepth)
{
    return Create(tex->GetDesc(), minDepth, maxDepth);
}

inline Scissor Scissor::Create(const TexturePtr &tex)
{
    return Create(tex->GetDesc());
}

template <typename...Ts>
void CommandBuffer::ExecuteBarriers(const Ts&...ts)
{
    // TODO: avoid unnecessary memory allocations

    std::vector<TextureTransitionBarrier> textureTs;
    std::vector<TextureAcquireBarrier>    textureAs;
    std::vector<TextureReleaseBarrier>    textureRs;

    std::vector<BufferTransitionBarrier> bufferTs;

    auto process = [&]<typename T>(const T &t)
    {
        using ET = typename std::remove_reference_t<decltype(Span(t))>::ElementType;
        auto span = Span<ET>(t);
        for(auto &s : span)
        {
            if constexpr(std::is_same_v<ET, TextureTransitionBarrier>)
            {
                if(s.texture)
                {
                    textureTs.push_back(s);
                }
            }
            else if constexpr(std::is_same_v<ET, TextureAcquireBarrier>)
            {
                if(s.texture)
                {
                    textureAs.push_back(s);
                }
            }
            else if constexpr(std::is_same_v<ET, TextureReleaseBarrier>)
            {
                if(s.texture)
                {
                    textureRs.push_back(s);
                }
            }
            else
            {
                static_assert(std::is_same_v<ET, BufferTransitionBarrier>);
                if(s.buffer)
                {
                    bufferTs.push_back(s);
                }
            }
        }
    };
    (process(ts), ...);
    this->ExecuteBarriersInternal(textureTs, bufferTs, textureRs, textureAs);
}

RTRC_RHI_END
