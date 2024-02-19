#pragma once

#include <array>
#include <optional>
#include <vector>

#include <Rtrc/Core/Container/Span.h>
#include <Rtrc/Core/EnumFlags.h>
#include <Rtrc/Core/Hash.h>
#include <Rtrc/Core/Math/Rect.h>
#include <Rtrc/Core/Math/Vector4.h>
#include <Rtrc/Core/SmartPointer/ObserverPtr.h>
#include <Rtrc/Core/SmartPointer/ReferenceCounted.h>
#include <Rtrc/Core/SmartPointer/UniquePointer.h>
#include <Rtrc/Core/StringPool.h>
#include <Rtrc/Core/Uncopyable.h>
#include <Rtrc/Core/Variant.h>
#include <Rtrc/RHI/Window/Window.h>

RTRC_RHI_BEGIN
    
template<typename T>
using RPtr = ReferenceCountedPtr<T>;

template<typename T>
using OPtr = Ref<T>;

template<typename T, typename...Args>
RPtr <T> MakeRPtr(Args&&...args)
{
    T *object = new T(std::forward<Args>(args)...);
    return RPtr<T>(object);
}

class RHIObject;
using RHIObjectPtr = RPtr<RHIObject>;

#if !RTRC_STATIC_RHI

#define RTRC_RHI_API virtual
#define RTRC_RHI_API_PURE = 0
#define RTRC_RHI_OVERRIDE override
#define RTRC_RHI_IMPLEMENT(DERIVED, BASE) class DERIVED final : public BASE
#define RTRC_RHI_FORWARD_DECL(CLASS) \
    class CLASS;                     \
    using CLASS##RPtr = RPtr<CLASS>; \
    using CLASS##UPtr = UPtr<CLASS>; \
    using CLASS##OPtr = OPtr<CLASS>;
#define RTRC_RHI_IMPL_PREFIX(X) X

#else // #if !RTRC_STATC_RHI

#define RTRC_RHI_API
#define RTRC_RHI_API_PURE
#define RTRC_RHI_OVERRIDE
#define RTRC_RHI_IMPLEMENT(DERIVED, BASE) class DERIVED final : public RHIObject
#if RTRC_RHI_VULKAN
#define RTRC_RHI_FORWARD_DECL(CLASS)      \
    namespace Vk { class Vulkan##CLASS; } \
    using CLASS = Vk::Vulkan##CLASS;      \
    using CLASS##RPtr = RPtr<CLASS>;      \
    using CLASS##UPtr = UPtr<CLASS>;      \
    using CLASS##OPtr = OPtr<CLASS>;
#define RTRC_RHI_IMPL_PREFIX(X) Vulkan##X
#elif RTRC_RHI_DIRECTX12
#define RTRC_RHI_FORWARD_DECL(CLASS)            \
    namespace D3D12 { class DirectX12##CLASS; } \
    using CLASS = D3D12::DirectX12##CLASS;      \
    using CLASS##RPtr = RPtr<CLASS>;            \
    using CLASS##UPtr = UPtr<CLASS>;            \
    using CLASS##OPtr = OPtr<CLASS>;
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
RTRC_RHI_FORWARD_DECL(RayTracingPipeline)
RTRC_RHI_FORWARD_DECL(Texture)
RTRC_RHI_FORWARD_DECL(TextureRtv)
RTRC_RHI_FORWARD_DECL(TextureSrv)
RTRC_RHI_FORWARD_DECL(TextureUav)
RTRC_RHI_FORWARD_DECL(TextureDsv)
RTRC_RHI_FORWARD_DECL(Buffer)
RTRC_RHI_FORWARD_DECL(BufferSrv)
RTRC_RHI_FORWARD_DECL(BufferUav)
RTRC_RHI_FORWARD_DECL(RayTracingLibrary)
RTRC_RHI_FORWARD_DECL(Sampler)
RTRC_RHI_FORWARD_DECL(BlasPrebuildInfo)
RTRC_RHI_FORWARD_DECL(TlasPrebuildInfo)
RTRC_RHI_FORWARD_DECL(Blas)
RTRC_RHI_FORWARD_DECL(Tlas)
RTRC_RHI_FORWARD_DECL(TransientResourcePool)

#undef RTRC_RHI_FORWARD_DECL

struct GraphicsPipelineDesc;
struct ComputePipelineDesc;
class BindingGroupUpdateBatch;

// =============================== rhi enums ===============================

enum class BackendType
{
    DirectX12,
    Vulkan,
#if RTRC_RHI_DIRECTX12
    Default = DirectX12,
#else
    Default = Vulkan,
#endif
};

enum class BarrierMemoryModel
{
    Undefined,           // For directx12
    AvailableAndVisible, // For vulkan
};

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
    R32_Float,
    R32G32_Float,
    R32G32B32A32_Float,
    R32G32B32_Float,
    R32G32B32A32_UInt,
    A2R10G10B10_UNorm,
    R16_UInt,
    R32_UInt,
    R8_UNorm,
    R16_UNorm,
    R16G16_UNorm,
    R16G16_Float,

    D24S8,
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

bool CanBeAccessedAsFloatInShader(Format format);
bool CanBeAccessedAsIntInShader(Format format);
bool CanBeAccessedAsUIntInShader(Format format);
bool NeedUNormAsTypedUAV(Format format);
bool NeedSNormAsTypedUAV(Format format);

bool HasDepthAspect(Format format);
bool HasStencilAspect(Format format);

enum class IndexFormat
{
    UInt16,
    UInt32
};

enum class ShaderType : uint8_t
{
    VertexShader     = 1u << 0,
    FragmentShader   = 1u << 1,
    ComputeShader    = 1u << 2,
    RayTracingShader = 1u << 3,
};

enum class ShaderStage : uint32_t
{
    None = 0,

    VertexShader          = 1u << 0,
    FragmentShader        = 1u << 1,
    ComputeShader         = 1u << 2,
    RT_RayGenShader       = 1u << 3,
    RT_MissShader         = 1u << 4,
    RT_ClosestHitShader   = 1u << 5,
    RT_IntersectionShader = 1u << 6,
    RT_AnyHitShader       = 1u << 7,
    CallableShader        = 1u << 8,

    VS = VertexShader,
    FS = FragmentShader,
    CS = ComputeShader,

    RT_RGS = RT_RayGenShader,
    RT_CHS = RT_ClosestHitShader,
    RT_MS  = RT_MissShader,
    RT_IS  = RT_IntersectionShader,
    RT_AHS = RT_AnyHitShader,

    AllGraphics = VS | FS,
    AllRTCommon = RT_RGS | RT_MS,
    AllRTHit    = RT_CHS | RT_IS | RT_AHS,
    AllRT       = AllRTCommon | AllRTHit | CallableShader,
    All         = AllGraphics | AllRT | CS
};
RTRC_DEFINE_ENUM_FLAGS(ShaderStage)
using ShaderStageFlags = EnumFlagsShaderStage;

std::string GetShaderStageFlagsName(ShaderStageFlags flags);

enum class PrimitiveTopology
{
    TriangleList,
    LineList,
};

enum class CullMode
{
    DontCull,
    CullFront,
    CullBack
};

enum class FrontFaceMode
{
    Clockwise,
    CounterClockwise
};

enum class FillMode
{
    Fill,
    Line
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
    AccelerationStructure,
};

inline const char *GetBindingTypeName(BindingType type)
{
    static const char *NAMES[] =
    {
        "Texture",
        "RWTexture",
        "Buffer",
        "StructuredBuffer",
        "RWBuffer",
        "RWStructuredBuffer",
        "ConstantBuffer",
        "Sampler",
        "AccelerationStructure"
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
    None           = 0,
    TransferDst    = 1 << 0,
    TransferSrc    = 1 << 1,
    ShaderResource = 1 << 2,
    UnorderAccess  = 1 << 3,
    RenderTarget   = 1 << 4,
    DepthStencil   = 1 << 5,
    ClearDst     = 1 << 6, // vulkan -> TransferDst; directx12 -> RenderTarget
};
RTRC_DEFINE_ENUM_FLAGS(TextureUsage)
using TextureUsageFlags = EnumFlagsTextureUsage;

enum class BufferUsage : uint32_t
{
    TransferDst                     = 1 << 0,
    TransferSrc                     = 1 << 1,
    ShaderBuffer                    = 1 << 2,
    ShaderRWBuffer                  = 1 << 3,
    ShaderStructuredBuffer          = 1 << 4,
    ShaderRWStructuredBuffer        = 1 << 5,
    ShaderConstantBuffer            = 1 << 6,
    IndexBuffer                     = 1 << 7,
    VertexBuffer                    = 1 << 8,
    IndirectBuffer                  = 1 << 9,
    DeviceAddress                   = 1 << 10,
    AccelerationStructureBuildInput = 1 << 11,
    AccelerationStructure           = 1 << 12,
    ShaderBindingTable              = 1 << 13,

    ComputeBuffer = ShaderStructuredBuffer | ShaderRWStructuredBuffer,
    AccelerationStructureScratch = ShaderRWStructuredBuffer | DeviceAddress,
};
RTRC_DEFINE_ENUM_FLAGS(BufferUsage)
using BufferUsageFlag = EnumFlagsBufferUsage;

enum class BufferHostAccessType
{
    None,
    Upload,
    Readback
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
    DepthStencilAttachment,         // Depth stencil attachment
    DepthStencilReadOnlyAttachment, // Depth stencil readonly attachment
    ShaderTexture,                  // Read in shader (Texture<...>)
    ShaderRWTexture,                // Read in shader (RWTexture<...>)
    CopySrc,
    CopyDst,
    ResolveSrc,
    ResolveDst,
    ClearDst,
    Present
};

enum class PipelineStage : uint32_t
{
    None             = 0,
    VertexInput      = 1 << 0,
    IndexInput       = 1 << 1,
    VertexShader     = 1 << 2,
    FragmentShader   = 1 << 3,
    ComputeShader    = 1 << 4,
    RayTracingShader = 1 << 5,
    DepthStencil     = 1 << 6,
    RenderTarget     = 1 << 7,
    Copy             = 1 << 8,
    Clear            = 1 << 9,
    Resolve          = 1 << 10,
    BuildAS          = 1 << 11,
    CopyAS           = 1 << 12,
    IndirectCommand  = 1 << 13,
    All              = 1 << 14
};
RTRC_DEFINE_ENUM_FLAGS(PipelineStage)
using PipelineStageFlag = EnumFlagsPipelineStage;

inline PipelineStageFlag ShaderStagesToPipelineStages(ShaderStageFlags stages)
{
    PipelineStageFlag ret = PipelineStage::None;
    if(stages.Contains(ShaderStage::VertexShader))   { ret |= PipelineStage::VertexShader; }
    if(stages.Contains(ShaderStage::FragmentShader)) { ret |= PipelineStage::FragmentShader; }
    if(stages.Contains(ShaderStage::ComputeShader))  { ret |= PipelineStage::ComputeShader; }
    if(stages | ShaderStage::AllRT)                  { ret |= PipelineStage::RayTracingShader; }
    return ret;
}

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
    StructuredBufferRead    = 1 << 11,
    RWBufferRead            = 1 << 12,
    RWBufferWrite           = 1 << 13,
    RWStructuredBufferRead  = 1 << 14,
    RWStructuredBufferWrite = 1 << 15,
    CopyRead                = 1 << 16,
    CopyWrite               = 1 << 17,
    ResolveRead             = 1 << 18,
    ResolveWrite            = 1 << 19,
    ClearColorWrite         = 1 << 20,
    ClearDepthStencilWrite  = 1 << 21,
    ReadAS                  = 1 << 22,
    WriteAS                 = 1 << 23,
    BuildASScratch          = 1 << 24,
    ReadSBT                 = 1 << 25,
    ReadForBuildAS          = 1 << 26,
    IndirectCommandRead     = 1 << 27,
    All                     = 1 << 28
};
RTRC_DEFINE_ENUM_FLAGS(ResourceAccess)
using ResourceAccessFlag = EnumFlagsResourceAccess;

bool IsReadOnly(ResourceAccessFlag access);
bool IsWriteOnly(ResourceAccessFlag access);
bool IsUAVOnly(ResourceAccessFlag access);
bool HasUAVAccess(ResourceAccessFlag access);
ResourceAccessFlag RemoveUAVAccesses(ResourceAccessFlag access);

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
    Exclusive,  // Exclusively owned by one queue
    Shared,     // Exclusively accessed by one queue
    Concurrent, // Concurrently accessed by graphics/compute queues
};

enum class RayTracingGeometryType
{
    Triangles,
    Precodural
};

enum class RayTracingVertexFormat
{
    Float3,
};

enum class RayTracingIndexFormat
{
    None,
    UInt16,
    UInt32,
};

enum class RayTracingAccelerationStructureBuildFlagBit : uint32_t
{
    AllowUpdate     = 1 << 0,
    AllowCompaction = 1 << 1,
    PreferFastBuild = 1 << 2,
    PreferFastTrace = 1 << 3,
    PreferLowMemory = 1 << 4,
};
RTRC_DEFINE_ENUM_FLAGS(RayTracingAccelerationStructureBuildFlagBit)
using RayTracingAccelerationStructureBuildFlags = EnumFlagsRayTracingAccelerationStructureBuildFlagBit;

struct BufferDeviceAddress
{
    uint64_t address = 0;
};

inline BufferDeviceAddress operator+(const BufferDeviceAddress &lhs, std::ptrdiff_t rhs)
{
    return { lhs.address + rhs };
}

inline BufferDeviceAddress operator+(std::ptrdiff_t lhs, const BufferDeviceAddress &rhs)
{
    return rhs + lhs;
}

// =============================== rhi descriptions ===============================

using QueueSessionID = uint64_t;

constexpr QueueSessionID INITIAL_QUEUE_SESSION_ID = 0;

class VertexSemantic
{

    static constexpr uint32_t INVALID_KEY = (std::numeric_limits<uint32_t>::max)();

    uint32_t key_;

public:

    struct VertexSemanticTag { };
    using PooledSemanticName = PooledString<VertexSemanticTag, uint32_t>;

    static VertexSemantic FromPooledName(PooledSemanticName pooledSemanticName, uint32_t semanticIndex)
    {
        VertexSemantic ret;
        ret.key_ = pooledSemanticName.GetIndex() | (semanticIndex << 24);
        return ret;
    }

    VertexSemantic() : key_(INVALID_KEY) { }
    VertexSemantic(std::string_view semanticName, uint32_t semanticIndex)
    {
        const auto pooledString = PooledSemanticName(semanticName);
        assert(pooledString.GetIndex() < 0xffffff);
        assert(semanticIndex < 0xff);
        key_ = (semanticIndex << 24) | pooledString.GetIndex();
    }
    VertexSemantic(std::string_view semanticString)
    {
        assert(!semanticString.empty());
        size_t i = semanticString.size();
        while(i > 0 && std::isdigit(semanticString[i - 1]))
        {
            --i;
        }
        assert(i > 0);
        const std::string_view semanticName = semanticString.substr(0, i);
        int semanticIndex = 0;
        if(i != semanticString.size())
        {
            const char *end = semanticString.data() + semanticString.size();
            const auto parseResult = std::from_chars(
                semanticString.data() + i, end, semanticIndex);
            if(parseResult.ec != std::errc() || parseResult.ptr != end)
            {
                throw Exception(fmt::format("Fail to parse semantic index in {}", semanticString));
            }
        }
        key_ = (semanticIndex << 24) | PooledSemanticName(semanticName).GetIndex();
    }

    bool IsValid() const { return key_ != INVALID_KEY; }
    const std::string &GetName() const { return PooledSemanticName::FromIndex(key_ & 0xffffff).GetString(); }
    uint32_t GetIndex() const { return key_ >> 24; }
    std::string ToString() const { return fmt::format("{}{}", GetName(), GetIndex()); }

    auto operator<=>(const VertexSemantic &) const = default;
};

#define RTRC_VERTEX_SEMANTIC(X, INDEX)            \
    (::Rtrc::RHI::VertexSemantic::FromPooledName( \
        RTRC_POOLED_STRING(::Rtrc::RHI::VertexSemantic::PooledSemanticName, X), INDEX))

struct IndirectDrawIndexedArgument
{
    uint32_t indexCountPerInstance;
    uint32_t instanceCount;
    uint32_t startIndexLocation;
    int32_t  baseVertexLocation;
    uint32_t startInstanceLocation;
};

struct IndirectDispatchArgument
{
    uint32_t threadGroupCountX;
    uint32_t threadGroupCountY;
    uint32_t threadGroupCountZ;
};

struct DeviceDesc
{
    bool graphicsQueue    = true;
    bool computeQueue     = true;
    bool transferQueue    = true;
    bool supportSwapchain = true;
    bool enableRayTracing = false;
};

struct SwapchainDesc
{
    Format format = Format::B8G8R8A8_UNorm;
    uint32_t imageCount = 2;
    bool vsync = true;
    bool allowUav = false;
};

struct RawShaderEntry
{
    ShaderStage stage;
    std::string name;
};

struct BindingDesc
{
    BindingType              type;
    ShaderStageFlags         stages = ShaderStage::All;
    std::optional<uint32_t>  arraySize;
    std::vector<SamplerRPtr> immutableSamplers;
    bool                     bindless = false;

    auto operator<=>(const BindingDesc &other) const = default;
    bool operator==(const BindingDesc &) const = default;
};

struct BindingGroupLayoutDesc
{
    std::vector<BindingDesc> bindings;
    bool variableArraySize = false; // When present, the last binding item must be bindless

    auto operator<=>(const BindingGroupLayoutDesc &other) const = default;
    bool operator==(const BindingGroupLayoutDesc &) const = default;
};

constexpr uint32_t STANDARD_PUSH_CONSTANT_BLOCK_SIZE = 256;

struct PushConstantRange
{
    uint32_t         offset = 0;
    uint32_t         size   = 0;
    ShaderStageFlags stages = {};

    auto operator<=>(const PushConstantRange &) const = default;
    bool operator==(const PushConstantRange &) const = default;
};

struct UnboundedBindingArrayAliasing
{
    int srcGroup; // assert(groups[srcGroup].variableArraySize)

    auto operator<=>(const UnboundedBindingArrayAliasing &) const = default;
};

struct BindingLayoutDesc
{
    std::vector<OPtr<BindingGroupLayout>>      groups;
    std::vector<PushConstantRange>             pushConstantRanges;
    std::vector<UnboundedBindingArrayAliasing> unboundedAliases;

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

    size_t Hash() const
    {
        return Rtrc::Hash(
            magFilter, minFilter, mipFilter,
            addressModeU, addressModeV, addressModeW,
            mipLODBias, minLOD, maxLOD,
            enableAnisotropy, maxAnisotropy,
            enableComparision, compareOp,
            borderColor[0], borderColor[1], borderColor[2], borderColor[3]);
    }
};

struct TexSubrsc
{
    uint32_t mipLevel;
    uint32_t arrayLayer;

    auto operator<=>(const TexSubrsc &) const = default;
    bool operator==(const TexSubrsc &) const = default;
};

struct TexSubrscs
{
    uint32_t mipLevel   = 0;
    uint32_t levelCount = 1;
    uint32_t arrayLayer = 0;
    uint32_t layerCount = 1;
};

struct ColorClearValue
{
    float r = 0, g = 0, b = 0, a = 0;

    auto operator<=>(const ColorClearValue &) const = default;
    bool operator==(const ColorClearValue &) const = default;
};

struct DepthStencilClearValue
{
    float   depth = 1;
    uint8_t stencil = 0;

    auto operator<=>(const DepthStencilClearValue &) const = default;
    bool operator==(const DepthStencilClearValue &) const = default;
};

using ClearValue = Variant<ColorClearValue, DepthStencilClearValue>;

struct TextureDesc
{
    using OptionalClearValue = Variant<std::monostate, ColorClearValue, DepthStencilClearValue>;

    TextureDimension dim = TextureDimension::Tex2D;
    Format           format;
    uint32_t         width;
    uint32_t         height;
    union
    {
        uint32_t arraySize = 1;
        uint32_t depth;
    };
    uint32_t                  mipLevels = 1;
    uint32_t                  sampleCount = 1;
    TextureUsageFlags         usage;
    TextureLayout             initialLayout = TextureLayout::Undefined;
    QueueConcurrentAccessMode concurrentAccessMode = QueueConcurrentAccessMode::Exclusive;
    OptionalClearValue        clearValue;
    bool                      linearHint = false;

    auto operator<=>(const TextureDesc &rhs) const
    {
        return std::make_tuple(
                    dim, format, width, height, arraySize, mipLevels,
                    sampleCount, usage, initialLayout, concurrentAccessMode, clearValue, linearHint)
           <=> std::make_tuple(
                    rhs.dim, rhs.format, rhs.width, rhs.height, rhs.arraySize, rhs.mipLevels,
                    rhs.sampleCount, rhs.usage, rhs.initialLayout, rhs.concurrentAccessMode, rhs.clearValue, linearHint);
    }

    bool operator==(const TextureDesc &rhs) const
    {
        return std::make_tuple(
                    dim, format, width, height, arraySize, mipLevels,
                    sampleCount, usage, initialLayout, concurrentAccessMode, clearValue, linearHint)
            == std::make_tuple(
                     rhs.dim, rhs.format, rhs.width, rhs.height, rhs.arraySize,
                     rhs.mipLevels, rhs.sampleCount, rhs.usage, rhs.initialLayout,
                     rhs.concurrentAccessMode, rhs.clearValue, linearHint);
    }

    size_t Hash() const
    {
        return Rtrc::Hash(
            dim, format, width, height, depth, mipLevels,
            sampleCount, usage, initialLayout, concurrentAccessMode, linearHint);
    }
};

struct TextureRtvDesc
{
    Format   format     = Format::Unknown;
    uint32_t mipLevel   = 0;
    uint32_t arrayLayer = 0;

    auto operator<=>(const TextureRtvDesc &) const = default;
};

struct TextureSrvDesc
{
    bool             isArray        = false;
    Format           format         = Format::Unknown;
    uint32_t         baseMipLevel   = 0;
    uint32_t         levelCount     = 0; // 0 means all levels
    uint32_t         baseArrayLayer = 0;
    uint32_t         layerCount     = 0; // 0 means all layers. only used when isArray == true
    
    auto operator<=>(const TextureSrvDesc &) const = default;
};

struct TextureUavDesc
{
    bool     isArray        = false;
    Format   format         = Format::Unknown;
    uint32_t mipLevel       = 0;
    uint32_t baseArrayLayer = 0;
    uint32_t layerCount     = 0; // 0 means all layers. only used when isArray == true

    auto operator<=>(const TextureUavDesc &) const = default;
};

struct TextureDsvDesc
{
    Format           format       = Format::Unknown;
    uint32_t         mipLevel     = 0;
    uint32_t         arrayLayer   = 0;
    
    auto operator<=>(const TextureDsvDesc &) const = default;
};

struct BufferDesc
{
    size_t               size;
    BufferUsageFlag      usage;
    BufferHostAccessType hostAccessType = BufferHostAccessType::None;

    auto operator<=>(const BufferDesc &) const = default;
    auto Hash() const { return Rtrc::Hash(size, usage, hostAccessType); }
};

struct BufferSrvDesc
{
    Format   format; // For texel buffer
    uint32_t offset;
    uint32_t range;
    uint32_t stride; // For structured buffer

    auto operator<=>(const BufferSrvDesc &) const = default;
};

using BufferUavDesc = BufferSrvDesc;

struct GlobalMemoryBarrier
{
    PipelineStageFlag  beforeStages;
    ResourceAccessFlag beforeAccesses;
    PipelineStageFlag  afterStages;
    ResourceAccessFlag afterAccesses;
};

struct TextureTransitionBarrier
{
    Texture            *texture;
    TexSubrscs          subresources;
    PipelineStageFlag   beforeStages;
    ResourceAccessFlag  beforeAccesses;
    TextureLayout       beforeLayout;
    PipelineStageFlag   afterStages;
    ResourceAccessFlag  afterAccesses;
    TextureLayout       afterLayout;
};

// For non-sharing resource, every cross-queue sync needs a release/acquire pair
struct TextureReleaseBarrier
{
    Texture            *texture;
    TexSubrscs          subresources;
    PipelineStageFlag   beforeStages;
    ResourceAccessFlag  beforeAccesses;
    TextureLayout       beforeLayout;
    TextureLayout       afterLayout;
    QueueType           beforeQueue;
    QueueType           afterQueue;
};

struct TextureAcquireBarrier
{
    Texture            *texture;
    TexSubrscs          subresources;
    TextureLayout       beforeLayout;
    PipelineStageFlag   afterStages;
    ResourceAccessFlag  afterAccesses;
    TextureLayout       afterLayout;
    QueueType           beforeQueue;
    QueueType           afterQueue;
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
    Vector2f topLeftCorner;
    Vector2f size;
    float    minDepth;
    float    maxDepth;

    auto operator<=>(const Viewport &) const = default;

    size_t Hash() const { return ::Rtrc::Hash(topLeftCorner, size, minDepth, maxDepth); }

    static Viewport Create(const TextureDesc &desc, float minDepth = 0, float maxDepth = 1)
    {
        return Viewport{
            .topLeftCorner   = { 0, 0 },
            .size            = { static_cast<float>(desc.width), static_cast<float>(desc.height) },
            .minDepth        = minDepth,
            .maxDepth        = maxDepth
        };
    }

    static Viewport Create(const TextureOPtr &tex, float minDepth = 0, float maxDepth = 1);

    template<typename T>
    static Viewport Create(const Rect<T> &viewportRect, float minDepth = 0, float maxDepth = 1);
};

struct Scissor
{
    Vector2i topLeftCorner;
    Vector2i size;

    auto operator<=>(const Scissor &) const = default;

    size_t Hash() const { return ::Rtrc::Hash(topLeftCorner, size); }

    static Scissor Create(const TextureDesc &desc)
    {
        return Scissor{ { 0, 0 }, { static_cast<int>(desc.width), static_cast<int>(desc.height) } };
    }

    static Scissor Create(const TextureOPtr &tex);
};

struct ColorAttachment
{
    TextureRtvRPtr    renderTargetView;
    AttachmentLoadOp  loadOp = AttachmentLoadOp::Load;
    AttachmentStoreOp storeOp = AttachmentStoreOp::Store;
    ColorClearValue   clearValue;
};

struct DepthStencilAttachment
{
    TextureDsvRPtr         depthStencilView;
    AttachmentLoadOp       loadOp = AttachmentLoadOp::Load;
    AttachmentStoreOp      storeOp = AttachmentStoreOp::Store;
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

// Fixed viewports; dynamic viewports with fixed count; dynamic count
using Viewports = Variant<std::monostate, StaticVector<Viewport, 4>, int, DynamicViewportCount>;

// Fixed viewports; dynamic viewports with fixed count; dynamic count
using Scissors = Variant<std::monostate, StaticVector<Scissor, 4>, int, DynamicScissorCount>;

struct VertexInputBuffer
{
    uint32_t elementSize;
    bool     perInstance;
};

struct VertexInputAttribute
{
    uint32_t            location;
    uint32_t            inputBufferIndex;
    uint32_t            byteOffsetInBuffer;
    VertexAttributeType type;
    std::string         semanticName;
    uint32_t            semanticIndex;
};

struct DebugLabel
{
    std::string             name;
    std::optional<Vector4f> color;
};

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

struct GraphicsPipelineDesc
{

    OPtr<RawShader> vertexShader;
    OPtr<RawShader> fragmentShader;

    OPtr<BindingLayout> bindingLayout;

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

    bool enableDepthClip = true;

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
    OPtr<RawShader>     computeShader;
    OPtr<BindingLayout> bindingLayout;
};

static constexpr uint32_t RAY_TRACING_UNUSED_SHADER = (std::numeric_limits<uint32_t>::max)();

struct RayTracingHitShaderGroup
{
    uint32_t closestHitShaderIndex   = RAY_TRACING_UNUSED_SHADER;
    uint32_t anyHitShaderIndex       = RAY_TRACING_UNUSED_SHADER;
    uint32_t intersectionShaderIndex = RAY_TRACING_UNUSED_SHADER;
};

struct RayTracingRayGenShaderGroup   { uint32_t rayGenShaderIndex = RAY_TRACING_UNUSED_SHADER;   };
struct RayTracingMissShaderGroup     { uint32_t missShaderIndex = RAY_TRACING_UNUSED_SHADER;     };
struct RayTracingCallableShaderGroup { uint32_t callableShaderIndex = RAY_TRACING_UNUSED_SHADER; };

using RayTracingShaderGroup = Variant<
    RayTracingHitShaderGroup,
    RayTracingRayGenShaderGroup,
    RayTracingMissShaderGroup,
    RayTracingCallableShaderGroup>;

struct RayTracingLibraryDesc
{
    RawShaderOPtr                      rawShader;
    std::vector<RayTracingShaderGroup> shaderGroups;
    uint32_t                           maxRayPayloadSize;
    uint32_t                           maxRayHitAttributeSize;
    uint32_t                           maxRecursiveDepth;
};

struct RayTracingPipelineDesc
{
    std::vector<RawShaderOPtr>         rawShaders;
    std::vector<RayTracingShaderGroup> shaderGroups;
    std::vector<RayTracingLibraryOPtr> libraries;
    OPtr<BindingLayout>                bindingLayout;
    uint32_t                           maxRayPayloadSize;
    uint32_t                           maxRayHitAttributeSize;
    uint32_t                           maxRecursiveDepth;
    bool                               useCustomStackSize = false;
};

struct RayTracingTrianglesGeometryData
{
    RayTracingVertexFormat vertexFormat;
    uint32_t               vertexStride;
    uint32_t               vertexCount;
    RayTracingIndexFormat  indexFormat;
    bool                   hasTransform;

    BufferDeviceAddress vertexData    = {};
    BufferDeviceAddress indexData     = {};
    BufferDeviceAddress transformData = {};
};

struct RayTracingProceduralGeometryData
{
    uint32_t            aabbStride;
    BufferDeviceAddress aabbData = {};
};

struct RayTracingGeometryDesc
{
    RayTracingGeometryType type;
    uint32_t               primitiveCount;
    bool                   opaque = false;
    bool                   noDuplicateAnyHitInvocation = false;
    union
    {
        RayTracingTrianglesGeometryData  trianglesData;
        RayTracingProceduralGeometryData proceduralData;
    };
};

struct RayTracingAccelerationStructurePrebuildInfo
{
    uint64_t accelerationStructureSize;
    uint64_t updateScratchSize;
    uint64_t buildScratchSize;
};

struct RayTracingInstanceData
{
    float        transform3x4[3][4];
    unsigned int instanceCustomIndex : 24;
    unsigned int instanceMask        : 8;
    unsigned int instanceSbtOffset   : 24;
    unsigned int flags               : 8;
    uint64_t     accelerationStructureAddress;
};

struct RayTracingInstanceArrayDesc
{
    uint32_t            instanceCount;
    BufferDeviceAddress instanceData;
};

struct ShaderGroupRecordRequirements
{
    uint32_t shaderGroupHandleSize;      // Size of each single shader group handle
    uint32_t shaderGroupHandleAlignment; // Alignment of each single shader group handle
    uint32_t shaderGroupBaseAlignment;   // Alignment of the table start address
    uint32_t maxShaderGroupStride;
};

struct ShaderBindingTableRegion
{
    BufferDeviceAddress deviceAddress = { 0 };
    uint32_t            stride = 0; // In bytes
    uint32_t            size   = 0; // In bytes
};

struct BufferReadRange
{
    size_t offset = 0;
    size_t size = 0;

    bool operator==(const BufferReadRange &) const = default;
};

static constexpr BufferReadRange READ_WHOLE_BUFFER = { 0, (std::numeric_limits<size_t>::max)() };

struct WarpSizeInfo
{
    uint32_t minSize;
    uint32_t maxSize;
};

struct TransientResourcePoolDesc
{
    size_t chunkSizeHint;
};

struct TransientBufferDeclaration
{
    BufferDesc desc;
    int beginPass;
    int endPass;

    BufferRPtr buffer;
};

struct TransientTextureDeclaration
{
    TextureDesc desc;
    int beginPass;
    int endPass;

    TextureRPtr texture;
};

using TransientResourceDeclaration = Variant<TransientBufferDeclaration, TransientTextureDeclaration>;

struct AliasedTransientResourcePair
{
    int first;
    int second;
};

// =============================== rhi interfaces ===============================

class RHIObject : public ReferenceCounted, public Uncopyable
{
    std::string RHIObjectName_;

public:

    virtual ~RHIObject() = default;

    virtual void SetName(std::string name) { RHIObjectName_ = std::move(name); }

    virtual const std::string &GetName() const { return RHIObjectName_; }
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
        RPtr<BufferSrv>,
        RPtr<BufferUav>,
        RPtr<TextureSrv>,
        RPtr<TextureUav>,
        OPtr<Sampler>,
        ConstantBufferUpdate,
        OPtr<Tlas>>;

    struct Record
    {
        BindingGroup *group;
        int           index;
        int           arrayElem;
        UpdateData    data;
    };

    void Append(BindingGroup &group, int index, RPtr<BufferSrv>             bufferSrv);
    void Append(BindingGroup &group, int index, RPtr<BufferUav>             bufferUav);
    void Append(BindingGroup &group, int index, RPtr<TextureSrv>            textureSrv);
    void Append(BindingGroup &group, int index, RPtr<TextureUav>            textureUav);
    void Append(BindingGroup &group, int index, OPtr<Sampler>               sampler);
    void Append(BindingGroup &group, int index, const ConstantBufferUpdate &cbuffer);
    void Append(BindingGroup &group, int index, OPtr<Tlas>                  tlas);

    void Append(BindingGroup &group, int index, int arrayElem, RPtr<BufferSrv>             bufferSrv);
    void Append(BindingGroup &group, int index, int arrayElem, RPtr<BufferUav>             bufferUav);
    void Append(BindingGroup &group, int index, int arrayElem, RPtr<TextureSrv>            textureSrv);
    void Append(BindingGroup &group, int index, int arrayElem, RPtr<TextureUav>            textureUav);
    void Append(BindingGroup &group, int index, int arrayElem, OPtr<Sampler>               sampler);
    void Append(BindingGroup &group, int index, int arrayElem, const ConstantBufferUpdate &cbuffer);
    void Append(BindingGroup &group, int index, int arrayElem, OPtr<Tlas>                  tlas);

    Span<Record> GetRecords() const { return records_; }

private:

    std::vector<Record> records_;
};

class QueueSyncQuery
{
public:

    void Set(QueueOPtr queue);

    bool IsSynchronized() const;

private:

    QueueOPtr      queue_;
    QueueSessionID sessionID_ = 0;
};

#define RTRC_RHI_QUEUE_COMMON               \
    struct BackBufferSemaphoreDependency    \
    {                                       \
        OPtr<BackBufferSemaphore> semaphore;\
        PipelineStageFlag        stages;    \
    };                                      \
    struct SemaphoreDependency              \
    {                                       \
        OPtr<Semaphore>    semaphore;       \
        PipelineStageFlag stages;           \
        uint64_t          value;            \
    };

#define RTRC_RHI_COMMAND_BUFFER_COMMON_METHODS                                              \
    template <typename...Ts>                                                                \
    void ExecuteBarriers(const Ts&...ts)                                                    \
    {                                                                                       \
        /* TODO: avoid unnecessary memory allocations */                                    \
                                                                                            \
        std::vector<GlobalMemoryBarrier>      globals;                                      \
        std::vector<TextureTransitionBarrier> textureTs;                                    \
        std::vector<TextureAcquireBarrier>    textureAs;                                    \
        std::vector<TextureReleaseBarrier>    textureRs;                                    \
        std::vector<BufferTransitionBarrier>  bufferTs;                                     \
                                                                                            \
        auto process = [&]<typename T>(const T & t)                                         \
        {                                                                                   \
            using ET = typename std::remove_reference_t<decltype(Span(t))>::ElementType;    \
            auto span = Span<ET>(t);                                                        \
            for(auto &s : span)                                                             \
            {                                                                               \
                if constexpr(std::is_same_v<ET, GlobalMemoryBarrier>)                       \
                {                                                                           \
                    globals.push_back(s);                                                   \
                }                                                                           \
                else if constexpr(std::is_same_v<ET, TextureTransitionBarrier>)             \
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
        this->ExecuteBarriersInternal(globals, textureTs, bufferTs, textureRs, textureAs);  \
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
    void ModifyMember(int index, const ConstantBufferUpdate &cbuffer) { this->ModifyMember(index, 0, cbuffer);    } \
    void ModifyMember(int index, Tlas *tlas)                          { this->ModifyMember(index, 0, tlas);       }

#if !RTRC_STATIC_RHI

class Surface : public RHIObject
{
    
};

class Instance : public RHIObject
{
public:

    RTRC_RHI_API UPtr<Device> CreateDevice(const DeviceDesc &desc = {}) RTRC_RHI_API_PURE;
};

class Device : public RHIObject
{
public:

    RTRC_RHI_API BackendType GetBackendType() const RTRC_RHI_API_PURE;
    RTRC_RHI_API bool IsGlobalBarrierWellSupported() const RTRC_RHI_API_PURE;
    RTRC_RHI_API BarrierMemoryModel GetBarrierMemoryModel() const RTRC_RHI_API_PURE;
    
    RTRC_RHI_API RPtr<Queue> GetQueue(QueueType type) RTRC_RHI_API_PURE;

    RTRC_RHI_API UPtr<CommandPool> CreateCommandPool(const RPtr<Queue> &queue) RTRC_RHI_API_PURE;
    RTRC_RHI_API UPtr<Swapchain>   CreateSwapchain(const SwapchainDesc &desc, Window &window) RTRC_RHI_API_PURE;

    RTRC_RHI_API UPtr<Fence>     CreateFence(bool signaled) RTRC_RHI_API_PURE;
    RTRC_RHI_API UPtr<Semaphore> CreateTimelineSemaphore(uint64_t initialValue) RTRC_RHI_API_PURE;

    RTRC_RHI_API UPtr<RawShader> CreateShader(
        const void *data, size_t size, std::vector<RawShaderEntry> entries) RTRC_RHI_API_PURE;

    RTRC_RHI_API UPtr<GraphicsPipeline>   CreateGraphicsPipeline  (const GraphicsPipelineDesc &desc) RTRC_RHI_API_PURE;
    RTRC_RHI_API UPtr<ComputePipeline>    CreateComputePipeline   (const ComputePipelineDesc &desc) RTRC_RHI_API_PURE;
    RTRC_RHI_API UPtr<RayTracingPipeline> CreateRayTracingPipeline(const RayTracingPipelineDesc &desc) RTRC_RHI_API_PURE;

    RTRC_RHI_API UPtr<RayTracingLibrary> CreateRayTracingLibrary(const RayTracingLibraryDesc &desc) RTRC_RHI_API_PURE;

    RTRC_RHI_API UPtr<BindingGroupLayout> CreateBindingGroupLayout(const BindingGroupLayoutDesc &desc) RTRC_RHI_API_PURE;
    RTRC_RHI_API UPtr<BindingGroup> CreateBindingGroup(
        const OPtr<BindingGroupLayout> &bindingGroupLayout, uint32_t variableArraySize = 0) RTRC_RHI_API_PURE;
    RTRC_RHI_API UPtr<BindingLayout> CreateBindingLayout(const BindingLayoutDesc &desc) RTRC_RHI_API_PURE;

    RTRC_RHI_API void UpdateBindingGroups(const BindingGroupUpdateBatch &batch) RTRC_RHI_API_PURE;

    RTRC_RHI_API void CopyBindingGroup(
        const BindingGroupOPtr &dstGroup, uint32_t dstIndex, uint32_t dstArrayOffset,
        const BindingGroupOPtr &srcGroup, uint32_t srcIndex, uint32_t srcArrayOffset,
        uint32_t count) RTRC_RHI_API_PURE;

    RTRC_RHI_API UPtr<Texture> CreateTexture(const TextureDesc &desc) RTRC_RHI_API_PURE;
    RTRC_RHI_API UPtr<Buffer>  CreateBuffer(const BufferDesc &desc) RTRC_RHI_API_PURE;
    RTRC_RHI_API UPtr<Sampler> CreateSampler(const SamplerDesc &desc) RTRC_RHI_API_PURE;

    RTRC_RHI_API size_t GetConstantBufferAlignment() const RTRC_RHI_API_PURE;
    RTRC_RHI_API size_t GetConstantBufferSizeAlignment() const RTRC_RHI_API_PURE;
    RTRC_RHI_API size_t GetAccelerationStructureScratchBufferAlignment() const RTRC_RHI_API_PURE;
    RTRC_RHI_API size_t GetTextureBufferCopyRowPitchAlignment(Format texelFormat) const RTRC_RHI_API_PURE;

    RTRC_RHI_API void WaitIdle() RTRC_RHI_API_PURE;

    RTRC_RHI_API BlasUPtr CreateBlas(const BufferRPtr &buffer, size_t offset, size_t size) RTRC_RHI_API_PURE;
    RTRC_RHI_API TlasUPtr CreateTlas(const BufferRPtr &buffer, size_t offset, size_t size) RTRC_RHI_API_PURE;

    RTRC_RHI_API BlasPrebuildInfoUPtr CreateBlasPrebuildInfo(
        Span<RayTracingGeometryDesc>              geometries,
        RayTracingAccelerationStructureBuildFlags flags) RTRC_RHI_API_PURE;
    RTRC_RHI_API TlasPrebuildInfoUPtr CreateTlasPrebuildInfo(
        const RayTracingInstanceArrayDesc        &instances,
        RayTracingAccelerationStructureBuildFlags flags) RTRC_RHI_API_PURE;

    RTRC_RHI_API const ShaderGroupRecordRequirements &GetShaderGroupRecordRequirements() const RTRC_RHI_API_PURE;
    RTRC_RHI_API const WarpSizeInfo &GetWarpSizeInfo() const RTRC_RHI_API_PURE;

    RTRC_RHI_API UPtr<TransientResourcePool> CreateTransientResourcePool(
        const TransientResourcePoolDesc &desc) RTRC_RHI_API_PURE;
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
    RTRC_RHI_API bool Present() RTRC_RHI_API_PURE;

    RTRC_RHI_API OPtr<BackBufferSemaphore> GetAcquireSemaphore() RTRC_RHI_API_PURE;
    RTRC_RHI_API OPtr<BackBufferSemaphore> GetPresentSemaphore() RTRC_RHI_API_PURE;

    RTRC_RHI_API int                GetRenderTargetCount() const RTRC_RHI_API_PURE;
    RTRC_RHI_API const TextureDesc &GetRenderTargetDesc() const RTRC_RHI_API_PURE;
    RTRC_RHI_API RPtr<Texture>      GetRenderTarget() const RTRC_RHI_API_PURE;
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
    RTRC_RHI_API uint32_t GetVariableArraySize() const RTRC_RHI_API_PURE;
    
    RTRC_RHI_API void ModifyMember(int index, int arrayElem, const BufferSrv            *bufferSrv)  RTRC_RHI_API_PURE;
    RTRC_RHI_API void ModifyMember(int index, int arrayElem, const BufferUav            *bufferUav)  RTRC_RHI_API_PURE;
    RTRC_RHI_API void ModifyMember(int index, int arrayElem, const TextureSrv           *textureSrv) RTRC_RHI_API_PURE;
    RTRC_RHI_API void ModifyMember(int index, int arrayElem, const TextureUav           *textureUav) RTRC_RHI_API_PURE;
    RTRC_RHI_API void ModifyMember(int index, int arrayElem, const Sampler              *sampler)    RTRC_RHI_API_PURE;
    RTRC_RHI_API void ModifyMember(int index, int arrayElem, const ConstantBufferUpdate &cbuffer)    RTRC_RHI_API_PURE;
    RTRC_RHI_API void ModifyMember(int index, int arrayElem, const Tlas                 *tlas)       RTRC_RHI_API_PURE;
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

    // Fence (if not null) must be in unsignaled state.
    RTRC_RHI_API void Submit(
        BackBufferSemaphoreDependency waitBackBufferSemaphore,
        Span<SemaphoreDependency>     waitSemaphores,
        Span<RPtr<CommandBuffer>>     commandBuffers,
        BackBufferSemaphoreDependency signalBackBufferSemaphore,
        Span<SemaphoreDependency>     signalSemaphores,
        OPtr<Fence>                   signalFence) RTRC_RHI_API_PURE;

    RTRC_RHI_API QueueSessionID GetCurrentSessionID() RTRC_RHI_API_PURE;
    RTRC_RHI_API QueueSessionID GetSynchronizedSessionID() RTRC_RHI_API_PURE;
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

    RTRC_RHI_API RPtr<CommandBuffer> NewCommandBuffer() RTRC_RHI_API_PURE;
};

class CommandBuffer : public RHIObject
{
public:

    RTRC_RHI_COMMAND_BUFFER_COMMON_METHODS

    RTRC_RHI_API void Begin() RTRC_RHI_API_PURE;
    RTRC_RHI_API void End()   RTRC_RHI_API_PURE;

    // Pipeline states
    
    RTRC_RHI_API void BeginRenderPass(
        Span<ColorAttachment>         colorAttachments,
        const DepthStencilAttachment &depthStencilAttachment) RTRC_RHI_API_PURE;
    RTRC_RHI_API void EndRenderPass() RTRC_RHI_API_PURE;

    RTRC_RHI_API void BindPipeline(const OPtr<GraphicsPipeline>   &pipeline) RTRC_RHI_API_PURE;
    RTRC_RHI_API void BindPipeline(const OPtr<ComputePipeline>    &pipeline) RTRC_RHI_API_PURE;
    RTRC_RHI_API void BindPipeline(const OPtr<RayTracingPipeline> &pipeline) RTRC_RHI_API_PURE;

    RTRC_RHI_API void BindGroupsToGraphicsPipeline  (int startIndex, Span<OPtr<BindingGroup>> groups) RTRC_RHI_API_PURE;
    RTRC_RHI_API void BindGroupsToComputePipeline   (int startIndex, Span<OPtr<BindingGroup>> groups) RTRC_RHI_API_PURE;
    RTRC_RHI_API void BindGroupsToRayTracingPipeline(int startIndex, Span<OPtr<BindingGroup>> groups) RTRC_RHI_API_PURE;

    RTRC_RHI_API void BindGroupToGraphicsPipeline  (int index, const OPtr<BindingGroup> &group) RTRC_RHI_API_PURE;
    RTRC_RHI_API void BindGroupToComputePipeline   (int index, const OPtr<BindingGroup> &group) RTRC_RHI_API_PURE;
    RTRC_RHI_API void BindGroupToRayTracingPipeline(int index, const OPtr<BindingGroup> &group) RTRC_RHI_API_PURE;

    RTRC_RHI_API void SetViewports(Span<Viewport> viewports) RTRC_RHI_API_PURE;
    RTRC_RHI_API void SetScissors (Span<Scissor> scissor)    RTRC_RHI_API_PURE;

    RTRC_RHI_API void SetViewportsWithCount(Span<Viewport> viewports) RTRC_RHI_API_PURE;
    RTRC_RHI_API void SetScissorsWithCount (Span<Scissor> scissors)   RTRC_RHI_API_PURE;

    RTRC_RHI_API void SetVertexBuffer(
        int              slot,
        Span<BufferRPtr> buffers,
        Span<size_t>     byteOffsets,
        Span<size_t>     byteStrides) RTRC_RHI_API_PURE;
    RTRC_RHI_API void SetIndexBuffer(
        const BufferRPtr  &buffer,
        size_t            byteOffset,
        IndexFormat       format) RTRC_RHI_API_PURE;

    RTRC_RHI_API void SetStencilReferenceValue(uint8_t value) RTRC_RHI_API_PURE;
    
    RTRC_RHI_API void SetGraphicsPushConstants(
        uint32_t    rangeIndex,
        uint32_t    offsetInRange,
        uint32_t    size,
        const void *data) RTRC_RHI_API_PURE;
    RTRC_RHI_API void SetComputePushConstants(
        uint32_t    rangeIndex,
        uint32_t    offsetInRange,
        uint32_t    size,
        const void *data) RTRC_RHI_API_PURE;

    // Draw & dispatch & trace

    RTRC_RHI_API void Draw(int vertexCount, int instanceCount, int firstVertex, int firstInstance) RTRC_RHI_API_PURE;
    RTRC_RHI_API void DrawIndexed(
        int indexCount,
        int instanceCount,
        int firstIndex,
        int firstVertex,
        int firstInstance) RTRC_RHI_API_PURE;

    RTRC_RHI_API void Dispatch(int groupCountX, int groupCountY, int groupCountZ) RTRC_RHI_API_PURE;

    RTRC_RHI_API void TraceRays(
        int                             rayCountX,
        int                             rayCountY,
        int                             rayCountZ,
        const ShaderBindingTableRegion &rayGenSbt,
        const ShaderBindingTableRegion &missSbt,
        const ShaderBindingTableRegion &hitSbt,
        const ShaderBindingTableRegion &callableSbt) RTRC_RHI_API_PURE;

    // Indirect draw

    RTRC_RHI_API void DispatchIndirect(const BufferRPtr &buffer, size_t byteOffset) RTRC_RHI_API_PURE;

    RTRC_RHI_API void DrawIndexedIndirect(
        const BufferRPtr &buffer, uint32_t drawCount, size_t byteOffset, size_t byteStride) RTRC_RHI_API_PURE;

    // Copy

    RTRC_RHI_API void CopyBuffer(
        Buffer *dst,
        size_t  dstOffset,
        Buffer *src,
        size_t  srcOffset,
        size_t  range) RTRC_RHI_API_PURE;
    RTRC_RHI_API void CopyColorTexture(
        Texture* dst,
        uint32_t dstMipLevel,
        uint32_t dstArrayLayer,
        Texture* src,
        uint32_t srcMipLevel,
        uint32_t srcArrayLayer) RTRC_RHI_API_PURE;

    RTRC_RHI_API void CopyBufferToColorTexture2D(
        Texture *dst,
        uint32_t mipLevel,
        uint32_t arrayLayer,
        Buffer  *src,
        size_t   srcOffset,
        size_t   srcRowBytes) RTRC_RHI_API_PURE;
    RTRC_RHI_API void CopyColorTexture2DToBuffer(
        Buffer  *dst,
        size_t   dstOffset,
        size_t   dstRowBytes,
        Texture *src,
        uint32_t mipLevel,
        uint32_t arrayLayer) RTRC_RHI_API_PURE;

    // Clear

    RTRC_RHI_API void ClearColorTexture2D(Texture *dst, const ColorClearValue &clearValue) RTRC_RHI_API_PURE;
    RTRC_RHI_API void ClearDepthStencilTexture(
        Texture *dst, const DepthStencilClearValue &clearValue, bool depth, bool stencil) RTRC_RHI_API_PURE;
    
    // Debug

    RTRC_RHI_API void BeginDebugEvent(const DebugLabel &label) RTRC_RHI_API_PURE;
    RTRC_RHI_API void EndDebugEvent()                          RTRC_RHI_API_PURE;

    // Acceleration structure

    // VertexData/inputData/transformData/aabbData will be accessed with [stage = BuildAS, access = ReadForBuildAS].
    // Scratch buffer will be accessed with [stage = BuildAS, access = ReadAS | WriteAS].
    // Tlas/blas will be accessed with [stage = BuildAS, access = WriteAS].

    RTRC_RHI_API void BuildBlas(
        const BlasPrebuildInfoOPtr  &buildInfo,
        Span<RayTracingGeometryDesc> geometries,
        const BlasOPtr              &blas,
        BufferDeviceAddress          scratchBufferAddress) RTRC_RHI_API_PURE;
    RTRC_RHI_API void BuildTlas(
        const TlasPrebuildInfoOPtr        &buildInfo,
        const RayTracingInstanceArrayDesc &instances,
        const TlasOPtr                    &tlas,
        BufferDeviceAddress                scratchBufferAddress) RTRC_RHI_API_PURE;

protected:

    // Release/acquire barriers for buffers are not needed since all buffers are assumed to be simultaneous-accessible
    RTRC_RHI_API void ExecuteBarriersInternal(
        Span<GlobalMemoryBarrier>      globalMemoryBarriers,
        Span<TextureTransitionBarrier> textureTransitions,
        Span<BufferTransitionBarrier>  bufferTransitions,
        Span<TextureReleaseBarrier>    textureReleaseBarriers,
        Span<TextureAcquireBarrier>    textureAcquireBarriers) RTRC_RHI_API_PURE;
};

class RawShader : public RHIObject
{

};

class RayTracingLibrary : public RHIObject
{

};

class GraphicsPipeline : public RHIObject
{
public:

    RTRC_RHI_API const OPtr<BindingLayout> &GetBindingLayout() const RTRC_RHI_API_PURE;
};

class ComputePipeline : public RHIObject
{
public:

    RTRC_RHI_API const OPtr<BindingLayout> &GetBindingLayout() const RTRC_RHI_API_PURE;
};

class RayTracingPipeline : public RHIObject
{
public:

    RTRC_RHI_API const OPtr<BindingLayout> &GetBindingLayout() const RTRC_RHI_API_PURE;

    RTRC_RHI_API void GetShaderGroupHandles(
        uint32_t               startGroupIndex,
        uint32_t               groupCount,
        MutSpan<unsigned char> outputData) const RTRC_RHI_API_PURE;
};

class Texture : public RHIObject
{
public:

    RTRC_RHI_TEXTURE_COMMON

    RTRC_RHI_API const TextureDesc &GetDesc() const RTRC_RHI_API_PURE;

    RTRC_RHI_API RPtr<TextureRtv> CreateRtv(const TextureRtvDesc &desc = {}) const RTRC_RHI_API_PURE;
    RTRC_RHI_API RPtr<TextureSrv> CreateSrv(const TextureSrvDesc &desc = {}) const RTRC_RHI_API_PURE;
    RTRC_RHI_API RPtr<TextureUav> CreateUav(const TextureUavDesc &desc = {}) const RTRC_RHI_API_PURE;
    RTRC_RHI_API RPtr<TextureDsv> CreateDsv(const TextureDsvDesc &desc = {}) const RTRC_RHI_API_PURE;
};

class TextureRtv : public RHIObject
{
public:

    RTRC_RHI_API const Vector2u GetSize() const RTRC_RHI_API_PURE;
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

    RTRC_RHI_API const Vector2u GetSize() const RTRC_RHI_API_PURE;
    RTRC_RHI_API const TextureDsvDesc &GetDesc() const RTRC_RHI_API_PURE;
};

class Buffer : public RHIObject
{
public:

    RTRC_RHI_API const BufferDesc &GetDesc() const RTRC_RHI_API_PURE;

    RTRC_RHI_API RPtr<BufferSrv> CreateSrv(const BufferSrvDesc &desc) const RTRC_RHI_API_PURE;
    RTRC_RHI_API RPtr<BufferUav> CreateUav(const BufferUavDesc &desc) const RTRC_RHI_API_PURE;

    RTRC_RHI_API void *Map(size_t offset, size_t size, const BufferReadRange &readRange, bool invalidate = false) RTRC_RHI_API_PURE;
    RTRC_RHI_API void Unmap(size_t offset, size_t size, bool flush = false)     RTRC_RHI_API_PURE;
    RTRC_RHI_API void InvalidateBeforeRead(size_t offset, size_t size)          RTRC_RHI_API_PURE;
    RTRC_RHI_API void FlushAfterWrite(size_t offset, size_t size)               RTRC_RHI_API_PURE;

    RTRC_RHI_API BufferDeviceAddress GetDeviceAddress() const RTRC_RHI_API_PURE;
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

class BlasPrebuildInfo : public RHIObject
{
public:

    RTRC_RHI_API const RayTracingAccelerationStructurePrebuildInfo &GetPrebuildInfo() const RTRC_RHI_API_PURE;
};

class TlasPrebuildInfo : public RHIObject
{
public:

    RTRC_RHI_API const RayTracingAccelerationStructurePrebuildInfo &GetPrebuildInfo() const RTRC_RHI_API_PURE;
};

class Blas : public RHIObject
{
public:

    RTRC_RHI_API BufferDeviceAddress GetDeviceAddress() const RTRC_RHI_API_PURE;
};

class Tlas : public RHIObject
{
public:

    RTRC_RHI_API BufferDeviceAddress GetDeviceAddress() const RTRC_RHI_API_PURE;
};

class TransientResourcePool : public RHIObject
{
public:

    virtual void Recycle() RTRC_RHI_API_PURE;

    virtual RC<QueueSyncQuery> Allocate(
        QueueOPtr                                  queue,
        MutSpan<TransientResourceDeclaration>      resources,
        std::vector<AliasedTransientResourcePair> &aliasRelation) RTRC_RHI_API_PURE;
};

#endif // #if !RTRC_STATIC_RHI

// =============================== vulkan backend ===============================

#if RTRC_RHI_VULKAN

// Can be called multiple times
void InitializeVulkanBackend();

struct VulkanInstanceDesc
{
    std::vector<std::string> extensions;
    bool debugMode = RTRC_DEBUG;
};

UPtr<Instance> CreateVulkanInstance(const VulkanInstanceDesc &desc);

#endif

// =============================== directx12 backend ===============================

#if RTRC_RHI_DIRECTX12

void InitializeDirectX12Backend();

struct DirectX12InstanceDesc
{
    bool debugMode = RTRC_DEBUG;
    bool gpuValidation = RTRC_DEBUG;
};

UPtr<Instance> CreateDirectX12Instance(const DirectX12InstanceDesc &desc);

#endif

RTRC_RHI_END
