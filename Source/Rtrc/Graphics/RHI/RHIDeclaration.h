#pragma once

#include <array>
#include <optional>
#include <vector>

#include <Rtrc/Math/Vector4.h>
#include <Rtrc/Utility/EnumFlags.h>
#include <Rtrc/Utility/ReferenceCounted.h>
#include <Rtrc/Utility/Span.h>
#include <Rtrc/Utility/Uncopyable.h>
#include <Rtrc/Utility/Variant.h>
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

class RHIObject;
using RHIObjectPtr = Ptr<RHIObject>;

#ifndef RTRC_STATIC_RHI

#define RTRC_RHI_API virtual
#define RTRC_RHI_API_PURE = 0
#define RTRC_RHI_OVERRIDE override
#define RTRC_RHI_IMPLEMENT(DERIVED, BASE) class DERIVED final : public BASE
#define RTRC_RHI_FORWARD_DECL(CLASS) class CLASS; using CLASS##Ptr = Ptr<CLASS>;
#define RTRC_RHI_IMPL_PREFIX(X) X

#else // #ifndef RTRC_STATC_RHI

#define RTRC_RHI_API
#define RTRC_RHI_API_PURE
#define RTRC_RHI_OVERRIDE
#define RTRC_RHI_IMPLEMENT(DERIVED, BASE) class DERIVED final : public RHIObject
#ifdef RTRC_RHI_VULKAN
#define RTRC_RHI_FORWARD_DECL(CLASS) \
    namespace Vk { class Vulkan##CLASS; } using CLASS = Vk::Vulkan##CLASS; using CLASS##Ptr = Ptr<CLASS>;
#define RTRC_RHI_IMPL_PREFIX(X) Vulkan##X
#else
#error "No RHI backend is enabled"
#endif

#endif // #ifndef RTRC_STATC_RHI

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
RTRC_RHI_FORWARD_DECL(ComputePipeline)
RTRC_RHI_FORWARD_DECL(Texture)
RTRC_RHI_FORWARD_DECL(TextureRtv)
RTRC_RHI_FORWARD_DECL(TextureSrv)
RTRC_RHI_FORWARD_DECL(TextureUav)
RTRC_RHI_FORWARD_DECL(TextureDsv)
RTRC_RHI_FORWARD_DECL(Buffer)
RTRC_RHI_FORWARD_DECL(BufferSrv)
RTRC_RHI_FORWARD_DECL(BufferUav)
RTRC_RHI_FORWARD_DECL(Sampler)
RTRC_RHI_FORWARD_DECL(MemoryPropertyRequirements)
RTRC_RHI_FORWARD_DECL(MemoryBlock)

#undef RTRC_RHI_FORWARD_DECL

struct GraphicsPipelineDesc;
struct ComputePipelineDesc;
class BindingGroupUpdateBatch;

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
    R8G8B8A8_UNorm,
    R32G32_Float,
    R32G32B32A32_Float,
    A2R10G10B10_UNorm,
    A2B10G10R10_UNorm,
    R11G11B10_UFloat,

    D24S8,
    D32S8,
    D32
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
    Float4,
    UChar4Norm,
};

const char *GetFormatName(Format format);

size_t GetTexelSize(Format format);

inline bool HasDepthAspect(Format format)
{
    return format == Format::D24S8 || format == Format::D32 || format == Format::D32S8;
}

inline bool HasStencilAspect(Format format)
{
    return format == Format::D24S8 || format == Format::D32S8;
}

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
    ColorAttachment,
    DepthStencilAttachment,                       // Depth stencil attachment
    DepthStencilReadOnlyAttachment,               // Depth stencil readonly attachment
    DepthShaderTexture_StencilAttachment,         // Read depth in shader, read/write stencil as attachment
    DepthShaderTexture_StencilReadOnlyAttachment, // Read depth in shader, read stencil as attachment
    ShaderTexture,                                // Read in shader (Texture<...>)
    ShaderRWTexture,                              // Read in shader (RWTexture<...>)
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
    Resolve        = 1 << 8,
    All            = 1 << 9
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
    ClearWrite              = 1 << 19,
    All                     = 1 << 20
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

enum class TextureSrvFlagBit
{
    SpecialLayout_DepthSrv_StencilAttachment         = 1 << 0,
    SpecialLayout_DepthSrv_StencilAttachmentReadOnly = 1 << 1,
};

using TextureSrvFlag = EnumFlags<TextureSrvFlagBit>;

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
    Format format = Format::B8G8R8A8_UNorm;
    uint32_t imageCount = 2;
    bool vsync = true;
};

struct BindingDesc
{
    BindingType              type;
    ShaderStageFlag          shaderStages = ShaderStageFlags::All;
    std::optional<uint32_t>  arraySize;
    std::vector<SamplerPtr>  immutableSamplers;
    bool                     bindless = false;

    auto operator<=>(const BindingDesc &other) const = default;
    bool operator==(const BindingDesc &) const = default;
};

struct BindingGroupLayoutDesc
{
    std::vector<BindingDesc> bindings;

    auto operator<=>(const BindingGroupLayoutDesc &other) const = default;
    bool operator==(const BindingGroupLayoutDesc &) const = default;
};

struct BindingLayoutDesc
{
    std::vector<Ptr<BindingGroupLayout>> groups;

    auto operator<=>(const BindingLayoutDesc &) const = default;
    bool operator==(const BindingLayoutDesc &) const = default;
};

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

    std::strong_ordering operator<=>(const TextureDesc &rhs) const
    {
        return std::make_tuple(
                    dim, format, width, height, arraySize, mipLevels,
                    sampleCount, usage, initialLayout, concurrentAccessMode)
           <=> std::make_tuple(
                    rhs.dim, rhs.format, rhs.width, rhs.height, rhs.arraySize, rhs.mipLevels,
                    rhs.sampleCount, rhs.usage, rhs.initialLayout, rhs.concurrentAccessMode);
    }

    bool operator==(const TextureDesc &rhs) const
    {
        return (*this <=> rhs) == std::strong_ordering::equal;
    }

    size_t Hash() const
    {
        return Rtrc::Hash(
            dim, format, width, height, depth, mipLevels, sampleCount, usage, initialLayout, concurrentAccessMode);
    }
};

struct TextureRtvDesc
{
    Format   format     = Format::Unknown;
    uint32_t mipLevel   = 0;
    uint32_t arrayLayer = 0;
};

struct TextureSrvDesc
{
    bool                 isArray        = false;
    Format               format         = Format::Unknown;
    uint32_t             baseMipLevel   = 0;
    uint32_t             levelCount     = 0; // all levels
    uint32_t             baseArrayLayer = 0;
    uint32_t             layerCount     = 0; // 0 means all layers. only used when isArray == true
    TextureSrvFlag       flags          = 0;
};

struct TextureUavDesc
{
    bool                 isArray        = false;
    Format               format         = Format::Unknown;
    uint32_t             mipLevel       = 0;
    uint32_t             baseArrayLayer = 0;
    uint32_t             layerCount     = 0; // 0 means all layers. only used when isArray == true
};

struct TextureDsvDesc
{
    Format   format     = Format::Unknown;
    uint32_t mipLevel   = 0;
    uint32_t arrayLayer = 0;
};

struct BufferDesc
{
    size_t               size;
    BufferUsageFlag      usage;
    BufferHostAccessType hostAccessType;

    auto operator<=>(const BufferDesc &) const = default;
    auto Hash() const { return Rtrc::Hash(size, usage, hostAccessType); }
};

struct BufferSrvDesc
{
    Format   format; // for texel buffer
    uint32_t offset;
    uint32_t range;
    uint32_t stride; // for structured buffer
};

using BufferUavDesc = BufferSrvDesc;

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
    float r = 0, g = 0, b = 0, a = 0;
};

struct DepthStencilClearValue
{
    float    depth   = 1;
    uint32_t stencil = 0;
};

using ClearValue = Variant<ColorClearValue, DepthStencilClearValue>;

struct RenderPassColorAttachment
{
    TextureRtv       *renderTargetView = nullptr;
    AttachmentLoadOp  loadOp           = AttachmentLoadOp::Load;
    AttachmentStoreOp storeOp          = AttachmentStoreOp::Store;
    ColorClearValue   clearValue;
};

struct RenderPassDepthStencilAttachment
{
    TextureDsv            *depthStencilView = nullptr;
    AttachmentLoadOp       loadOp           = AttachmentLoadOp::Load;
    AttachmentStoreOp      storeOp          = AttachmentStoreOp::Store;
    DepthStencilClearValue clearValue;
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

        auto operator<=>(const StencilOps &) const = default;

        size_t Hash() const
        {
            return ::Rtrc::Hash(depthFailOp, failOp, passOp, compareOp);
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
    uint8_t    stencilReadMask  = 0;
    uint8_t    stencilWriteMask = 0;
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

struct ConstantBufferUpdate
{
    const Buffer *buffer;
    size_t offset;
    size_t range;
};

class BindingGroupUpdateBatch
{
public:

    using UpdateData = Variant<
        const BufferSrv *,
        const BufferUav *,
        const TextureSrv *,
        const TextureUav *,
        const Sampler *,
        ConstantBufferUpdate>;

    struct Record
    {
        BindingGroup *group;
        int index;
        int arrayElem;
        UpdateData data;
    };

    void Append(BindingGroup &group, int index, const BufferSrv *bufferSrv);
    void Append(BindingGroup &group, int index, const BufferUav *bufferUav);
    void Append(BindingGroup &group, int index, const TextureSrv *textureSrv);
    void Append(BindingGroup &group, int index, const TextureUav *textureUav);
    void Append(BindingGroup &group, int index, const Sampler *sampler);
    void Append(BindingGroup &group, int index, const ConstantBufferUpdate &cbuffer);

    void Append(BindingGroup &group, int index, int arrayElem, const BufferSrv *bufferSrv);
    void Append(BindingGroup &group, int index, int arrayElem, const BufferUav *bufferUav);
    void Append(BindingGroup &group, int index, int arrayElem, const TextureSrv *textureSrv);
    void Append(BindingGroup &group, int index, int arrayElem, const TextureUav *textureUav);
    void Append(BindingGroup &group, int index, int arrayElem, const Sampler *sampler);
    void Append(BindingGroup &group, int index, int arrayElem, const ConstantBufferUpdate &cbuffer);

    Span<Record> GetRecords() const { return records_; }

private:

    std::vector<Record> records_;
};

class GraphicsPipelineBuilder : GraphicsPipelineDesc
{
    friend class RTRC_RHI_IMPL_PREFIX(Device);

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

    // default is 0
    GraphicsPipelineBuilder &SetStencilMask(uint8_t readMask, uint8_t writeMask);

    // default is keep, keep, keep, always, 0xff, 0xff
    GraphicsPipelineBuilder &SetStencilFrontOp(
        StencilOp depthFailOp,
        StencilOp failOp,
        StencilOp passOp,
        CompareOp compareOp);

    // default is keep, keep, keep, always, 0xff, 0xff
    GraphicsPipelineBuilder &SetStencilBackOp(
        StencilOp depthFailOp,
        StencilOp failOp,
        StencilOp passOp,
        CompareOp compareOp);

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

class ComputePipelineBuilder : public RHIObject, ComputePipelineDesc
{
public:

    ComputePipelineBuilder &SetComputeShader(Ptr<RawShader> shader);

    ComputePipelineBuilder &SetBindingLayout(Ptr<BindingLayout> layout);

    Ptr<ComputePipeline> CreatePipeline(const DevicePtr &device) const;
};

#define RTRC_RHI_QUEUE_COMMON               \
    struct BackBufferSemaphoreDependency    \
    {                                       \
        Ptr<BackBufferSemaphore> semaphore; \
        PipelineStageFlag        stages;    \
    };                                      \
    struct SemaphoreDependency              \
    {                                       \
        Ptr<Semaphore>    semaphore;        \
        PipelineStageFlag stages;           \
        uint64_t          value;            \
    };

#define RTRC_RHI_COMMAND_BUFFER_COMMON_METHODS                                              \
    template <typename...Ts>                                                                \
    void ExecuteBarriers(const Ts&...ts)                                                    \
    {                                                                                       \
        /* TODO: avoid unnecessary memory allocations */                                    \
                                                                                            \
        std::vector<TextureTransitionBarrier> textureTs;                                    \
        std::vector<TextureAcquireBarrier>    textureAs;                                    \
        std::vector<TextureReleaseBarrier>    textureRs;                                    \
                                                                                            \
        std::vector<BufferTransitionBarrier> bufferTs;                                      \
                                                                                            \
        auto process = [&]<typename T>(const T & t)                                         \
        {                                                                                   \
            using ET = typename std::remove_reference_t<decltype(Span(t))>::ElementType;    \
            auto span = Span<ET>(t);                                                        \
            for(auto &s : span)                                                             \
            {                                                                               \
                if constexpr(std::is_same_v<ET, TextureTransitionBarrier>)                  \
                {                                                                           \
                    if(s.texture)                                                           \
                    {                                                                       \
                        textureTs.push_back(s);                                             \
                    }                                                                       \
                }                                                                           \
                else if constexpr(std::is_same_v<ET, TextureAcquireBarrier>)                \
                {                                                                           \
                    if(s.texture)                                                           \
                    {                                                                       \
                        textureAs.push_back(s);                                             \
                    }                                                                       \
                }                                                                           \
                else if constexpr(std::is_same_v<ET, TextureReleaseBarrier>)                \
                {                                                                           \
                    if(s.texture)                                                           \
                    {                                                                       \
                        textureRs.push_back(s);                                             \
                    }                                                                       \
                }                                                                           \
                else                                                                        \
                {                                                                           \
                    static_assert(std::is_same_v<ET, BufferTransitionBarrier>);             \
                    if(s.buffer)                                                            \
                    {                                                                       \
                        bufferTs.push_back(s);                                              \
                    }                                                                       \
                }                                                                           \
            }                                                                               \
        };                                                                                  \
        (process(ts), ...);                                                                 \
        this->ExecuteBarriersInternal(textureTs, bufferTs, textureRs, textureAs);           \
    }

#define RTRC_RHI_TEXTURE_COMMON                                                                          \
    QueueConcurrentAccessMode GetConcurrentAccessMode() const { return GetDesc().concurrentAccessMode; } \
    TextureDimension GetDimension() const { return GetDesc().dim; }                                      \
    Format           GetFormat()    const { return GetDesc().format; }                                   \
    uint32_t         GetMipLevels() const { return GetDesc().mipLevels; }                                \
    uint32_t         GetArraySize() const { return GetDesc().arraySize; }                                \
    uint32_t         GetWidth()     const { return GetDesc().width; }                                    \
    uint32_t         GetHeight()    const { return GetDesc().height; }                                   \
    uint32_t         GetDepth()     const { return GetDesc().depth; }

#define RTRC_RHI_BINDING_GROUP_COMMON                                                                               \
    void ModifyMember(int index, BufferSrv *bufferSrv)                { this->ModifyMember(index, 0, bufferSrv);  } \
    void ModifyMember(int index, BufferUav *bufferUav)                { this->ModifyMember(index, 0, bufferUav);  } \
    void ModifyMember(int index, TextureSrv *textureSrv)              { this->ModifyMember(index, 0, textureSrv); } \
    void ModifyMember(int index, TextureUav *textureUav)              { this->ModifyMember(index, 0, textureUav); } \
    void ModifyMember(int index, Sampler *sampler)                    { this->ModifyMember(index, 0, sampler);    } \
    void ModifyMember(int index, const ConstantBufferUpdate &cbuffer) { this->ModifyMember(index, 0, cbuffer);    }

#ifndef RTRC_STATIC_RHI

class Surface : public RHIObject
{
    
};

class Instance : public RHIObject
{
public:

    RTRC_RHI_API Ptr<Device> CreateDevice(const DeviceDesc &desc = {}) RTRC_RHI_API_PURE;
};

class Device : public RHIObject
{
public:
    
    RTRC_RHI_API Ptr<Queue> GetQueue(QueueType type) RTRC_RHI_API_PURE;

    RTRC_RHI_API Ptr<CommandPool> CreateCommandPool(const Ptr<Queue> &queue) RTRC_RHI_API_PURE;

    RTRC_RHI_API Ptr<Swapchain> CreateSwapchain(const SwapchainDesc &desc, Window &window) RTRC_RHI_API_PURE;

    RTRC_RHI_API Ptr<Fence>     CreateFence(bool signaled) RTRC_RHI_API_PURE;
    RTRC_RHI_API Ptr<Semaphore> CreateSemaphore(uint64_t initialValue) RTRC_RHI_API_PURE;

    RTRC_RHI_API Ptr<RawShader> CreateShader(const void *data, size_t size, std::string entryPoint, ShaderStage type) RTRC_RHI_API_PURE;

    RTRC_RHI_API Ptr<GraphicsPipeline> CreateGraphicsPipeline(const GraphicsPipelineDesc &desc) RTRC_RHI_API_PURE;
    RTRC_RHI_API Ptr<ComputePipeline>  CreateComputePipeline (const ComputePipelineDesc &desc) RTRC_RHI_API_PURE;

    RTRC_RHI_API Ptr<BindingGroupLayout> CreateBindingGroupLayout(const BindingGroupLayoutDesc &desc) RTRC_RHI_API_PURE;
    RTRC_RHI_API Ptr<BindingGroup>       CreateBindingGroup(const Ptr<BindingGroupLayout> &bindingGroupLayout) RTRC_RHI_API_PURE;
    RTRC_RHI_API Ptr<BindingLayout>      CreateBindingLayout(const BindingLayoutDesc &desc) RTRC_RHI_API_PURE;

    RTRC_RHI_API void UpdateBindingGroups(const BindingGroupUpdateBatch &batch) RTRC_RHI_API_PURE;

    RTRC_RHI_API Ptr<Texture> CreateTexture(const TextureDesc &desc) RTRC_RHI_API_PURE;
    RTRC_RHI_API Ptr<Buffer>  CreateBuffer(const BufferDesc &desc) RTRC_RHI_API_PURE;
    RTRC_RHI_API Ptr<Sampler> CreateSampler(const SamplerDesc &desc) RTRC_RHI_API_PURE;

    RTRC_RHI_API Ptr<MemoryPropertyRequirements> GetMemoryRequirements(
        const TextureDesc &desc, size_t *size, size_t *alignment) const RTRC_RHI_API_PURE;
    RTRC_RHI_API Ptr<MemoryPropertyRequirements> GetMemoryRequirements(
        const BufferDesc &desc, size_t *size, size_t *alignment) const RTRC_RHI_API_PURE;
    RTRC_RHI_API Ptr<MemoryBlock> CreateMemoryBlock(const MemoryBlockDesc &desc) RTRC_RHI_API_PURE;

    RTRC_RHI_API Ptr<Texture> CreatePlacedTexture(
        const TextureDesc &desc, const Ptr<MemoryBlock> &memoryBlock, size_t offsetInMemoryBlock) RTRC_RHI_API_PURE;
    RTRC_RHI_API Ptr<Buffer> CreatePlacedBuffer(
        const BufferDesc &desc, const Ptr<MemoryBlock> &memoryBlock, size_t offsetInMemoryBlock) RTRC_RHI_API_PURE;

    RTRC_RHI_API size_t GetConstantBufferAlignment() const RTRC_RHI_API_PURE;

    RTRC_RHI_API void WaitIdle() RTRC_RHI_API_PURE;
};

class BackBufferSemaphore : public RHIObject
{

};

class Semaphore : public RHIObject
{
public:

    RTRC_RHI_API uint64_t GetValue() const RTRC_RHI_API_PURE;
};

class Swapchain : public RHIObject
{
public:

    RTRC_RHI_API bool Acquire() RTRC_RHI_API_PURE;
    RTRC_RHI_API void Present() RTRC_RHI_API_PURE;

    RTRC_RHI_API Ptr<BackBufferSemaphore> GetAcquireSemaphore() RTRC_RHI_API_PURE;
    RTRC_RHI_API Ptr<BackBufferSemaphore> GetPresentSemaphore() RTRC_RHI_API_PURE;

    RTRC_RHI_API int                GetRenderTargetCount() const RTRC_RHI_API_PURE;
    RTRC_RHI_API const TextureDesc &GetRenderTargetDesc() const RTRC_RHI_API_PURE;
    RTRC_RHI_API Ptr<Texture>       GetRenderTarget() const RTRC_RHI_API_PURE;
};

class BindingGroupLayout : public RHIObject
{
public:

    RTRC_RHI_API const BindingGroupLayoutDesc &GetDesc() const RTRC_RHI_API_PURE;
};

class BindingGroup : public RHIObject
{
public:

    RTRC_RHI_BINDING_GROUP_COMMON

    RTRC_RHI_API const BindingGroupLayout *GetLayout() const RTRC_RHI_API_PURE;
    
    RTRC_RHI_API void ModifyMember(int index, int arrayElem, BufferSrv                  *bufferSrv)  RTRC_RHI_API_PURE;
    RTRC_RHI_API void ModifyMember(int index, int arrayElem, BufferUav                  *bufferUav)  RTRC_RHI_API_PURE;
    RTRC_RHI_API void ModifyMember(int index, int arrayElem, TextureSrv                 *textureSrv) RTRC_RHI_API_PURE;
    RTRC_RHI_API void ModifyMember(int index, int arrayElem, TextureUav                 *textureUav) RTRC_RHI_API_PURE;
    RTRC_RHI_API void ModifyMember(int index, int arrayElem, Sampler                    *sampler)    RTRC_RHI_API_PURE;
    RTRC_RHI_API void ModifyMember(int index, int arrayElem, const ConstantBufferUpdate &cbuffer)    RTRC_RHI_API_PURE;
};

class BindingLayout : public RHIObject
{

};

class Queue : public RHIObject
{
public:

    RTRC_RHI_QUEUE_COMMON

    RTRC_RHI_API QueueType GetType() const RTRC_RHI_API_PURE;

    RTRC_RHI_API void WaitIdle() RTRC_RHI_API_PURE;

    RTRC_RHI_API void Submit(
        BackBufferSemaphoreDependency waitBackBufferSemaphore,
        Span<SemaphoreDependency>     waitSemaphores,
        Span<Ptr<CommandBuffer>>      commandBuffers,
        BackBufferSemaphoreDependency signalBackBufferSemaphore,
        Span<SemaphoreDependency>     signalSemaphores,
        const Ptr<Fence>             &signalFence) RTRC_RHI_API_PURE;
};

class Fence : public RHIObject
{
public:

    RTRC_RHI_API void Reset() RTRC_RHI_API_PURE;

    RTRC_RHI_API void Wait() RTRC_RHI_API_PURE;
};

class CommandPool : public RHIObject
{
public:

    RTRC_RHI_API void Reset() RTRC_RHI_API_PURE;

    RTRC_RHI_API QueueType GetType() const RTRC_RHI_API_PURE;

    RTRC_RHI_API Ptr<CommandBuffer> NewCommandBuffer() RTRC_RHI_API_PURE;
};

class CommandBuffer : public RHIObject
{
public:

    RTRC_RHI_API void Begin() RTRC_RHI_API_PURE;

    RTRC_RHI_API void End() RTRC_RHI_API_PURE;
    
    RTRC_RHI_API void BeginRenderPass(
        Span<RenderPassColorAttachment>         colorAttachments,
        const RenderPassDepthStencilAttachment &depthStencilAttachment) RTRC_RHI_API_PURE;
    RTRC_RHI_API void EndRenderPass() RTRC_RHI_API_PURE;

    RTRC_RHI_API void BindPipeline(const Ptr<GraphicsPipeline> &pipeline) RTRC_RHI_API_PURE;
    RTRC_RHI_API void BindPipeline(const Ptr<ComputePipeline>  &pipeline) RTRC_RHI_API_PURE;

    RTRC_RHI_API void BindGroupsToGraphicsPipeline(int startIndex, Span<RC<BindingGroup>> groups) RTRC_RHI_API_PURE;
    RTRC_RHI_API void BindGroupsToComputePipeline (int startIndex, Span<RC<BindingGroup>> groups) RTRC_RHI_API_PURE;

    RTRC_RHI_API void BindGroupToGraphicsPipeline(int index, const Ptr<BindingGroup> &group) RTRC_RHI_API_PURE;
    RTRC_RHI_API void BindGroupToComputePipeline (int index, const Ptr<BindingGroup> &group) RTRC_RHI_API_PURE;

    RTRC_RHI_API void SetViewports(Span<Viewport> viewports) RTRC_RHI_API_PURE;
    RTRC_RHI_API void SetScissors (Span<Scissor> scissor) RTRC_RHI_API_PURE;

    RTRC_RHI_API void SetViewportsWithCount(Span<Viewport> viewports) RTRC_RHI_API_PURE;
    RTRC_RHI_API void SetScissorsWithCount (Span<Scissor> scissors) RTRC_RHI_API_PURE;

    RTRC_RHI_API void SetVertexBuffer(int slot, Span<BufferPtr> buffers, Span<size_t> byteOffsets) RTRC_RHI_API_PURE;
    RTRC_RHI_API void SetIndexBuffer(const BufferPtr &buffer, size_t byteOffset, IndexBufferFormat format) RTRC_RHI_API_PURE;

    RTRC_RHI_API void SetStencilReferenceValue(uint8_t value) RTRC_RHI_API_PURE;

    RTRC_RHI_API void Draw(int vertexCount, int instanceCount, int firstVertex, int firstInstance) RTRC_RHI_API_PURE;
    RTRC_RHI_API void DrawIndexed(int indexCount, int instanceCount, int firstIndex, int firstVertex, int firstInstance) RTRC_RHI_API_PURE;

    RTRC_RHI_API void Dispatch(int groupCountX, int groupCountY, int groupCountZ) RTRC_RHI_API_PURE;

    RTRC_RHI_API void CopyBuffer(Buffer *dst, size_t dstOffset, Buffer *src, size_t srcOffset, size_t range) RTRC_RHI_API_PURE;
    RTRC_RHI_API void CopyBufferToColorTexture2D(
        Texture *dst, uint32_t mipLevel, uint32_t arrayLayer, Buffer *src, size_t srcOffset) RTRC_RHI_API_PURE;
    RTRC_RHI_API void CopyColorTexture2DToBuffer(
        Buffer *dst, size_t dstOffset, Texture *src, uint32_t mipLevel, uint32_t arrayLayer) RTRC_RHI_API_PURE;

    RTRC_RHI_API void ClearColorTexture2D(Texture *dst, const ColorClearValue &clearValue) RTRC_RHI_API_PURE;

    RTRC_RHI_API void BeginDebugEvent(const DebugLabel &label) RTRC_RHI_API_PURE;
    RTRC_RHI_API void EndDebugEvent() RTRC_RHI_API_PURE;

    RTRC_RHI_COMMAND_BUFFER_COMMON_METHODS

protected:

    RTRC_RHI_API void ExecuteBarriersInternal(
        Span<TextureTransitionBarrier> textureTransitions,
        Span<BufferTransitionBarrier>  bufferTransitions,
        Span<TextureReleaseBarrier>    textureReleaseBarriers,
        Span<TextureAcquireBarrier>    textureAcquireBarriers) RTRC_RHI_API_PURE;
};

class RawShader : public RHIObject
{
public:

    RTRC_RHI_API ShaderStage GetType() const RTRC_RHI_API_PURE;
};

class GraphicsPipeline : public RHIObject
{
public:

    RTRC_RHI_API const Ptr<BindingLayout> &GetBindingLayout() const RTRC_RHI_API_PURE;
};

class ComputePipeline : public RHIObject
{
public:

    RTRC_RHI_API const Ptr<BindingLayout> &GetBindingLayout() const RTRC_RHI_API_PURE;
};

class Texture : public RHIObject
{
public:

    RTRC_RHI_TEXTURE_COMMON

    RTRC_RHI_API const TextureDesc &GetDesc() const RTRC_RHI_API_PURE;

    RTRC_RHI_API Ptr<TextureRtv> CreateRtv(const TextureRtvDesc &desc = {}) const RTRC_RHI_API_PURE;
    RTRC_RHI_API Ptr<TextureSrv> CreateSrv(const TextureSrvDesc &desc = {}) const RTRC_RHI_API_PURE;
    RTRC_RHI_API Ptr<TextureUav> CreateUav(const TextureUavDesc &desc = {}) const RTRC_RHI_API_PURE;
    RTRC_RHI_API Ptr<TextureDsv> CreateDsv(const TextureDsvDesc &desc = {}) const RTRC_RHI_API_PURE;
};

class TextureRtv : public RHIObject
{
public:

    RTRC_RHI_API const TextureRtvDesc &GetDesc() const RTRC_RHI_API_PURE;
};

class TextureSrv : public RHIObject
{
public:

    RTRC_RHI_API const TextureSrvDesc &GetDesc() const RTRC_RHI_API_PURE;
};

class TextureUav : public RHIObject
{
public:

    RTRC_RHI_API const TextureUavDesc &GetDesc() const RTRC_RHI_API_PURE;
};

class TextureDsv : public RHIObject
{
public:

    RTRC_RHI_API const TextureDsvDesc &GetDesc() const RTRC_RHI_API_PURE;
};

class Buffer : public RHIObject
{
public:

    RTRC_RHI_API const BufferDesc &GetDesc() const RTRC_RHI_API_PURE;

    RTRC_RHI_API Ptr<BufferSrv> CreateSrv(const BufferSrvDesc &desc) const RTRC_RHI_API_PURE;
    RTRC_RHI_API Ptr<BufferUav> CreateUav(const BufferUavDesc &desc) const RTRC_RHI_API_PURE;

    RTRC_RHI_API void *Map(size_t offset, size_t size, bool invalidate = false) RTRC_RHI_API_PURE;
    RTRC_RHI_API void Unmap(size_t offset, size_t size, bool flush = false) RTRC_RHI_API_PURE;
    RTRC_RHI_API void InvalidateBeforeRead(size_t offset, size_t size) RTRC_RHI_API_PURE;
    RTRC_RHI_API void FlushAfterWrite(size_t offset, size_t size) RTRC_RHI_API_PURE;
};

class BufferSrv : public RHIObject
{
public:

    RTRC_RHI_API const BufferSrvDesc &GetDesc() const RTRC_RHI_API_PURE;
};

class BufferUav : public RHIObject
{
public:

    RTRC_RHI_API const BufferUavDesc &GetDesc() const RTRC_RHI_API_PURE;
};

class Sampler : public RHIObject
{
public:

    RTRC_RHI_API const SamplerDesc &GetDesc() const RTRC_RHI_API_PURE;
};

// all memory requirements other than size & alignment
class MemoryPropertyRequirements : public RHIObject
{
public:

    RTRC_RHI_API bool IsValid() const RTRC_RHI_API_PURE;

    RTRC_RHI_API bool Merge(const MemoryPropertyRequirements &other) RTRC_RHI_API_PURE;

    RTRC_RHI_API Ptr<MemoryPropertyRequirements> Clone() const RTRC_RHI_API_PURE;
};

class MemoryBlock : public RHIObject
{
public:

    RTRC_RHI_API const MemoryBlockDesc &GetDesc() const RTRC_RHI_API_PURE;
};

#endif // #ifndef RTRC_STATIC_RHI

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

RTRC_RHI_END
