#pragma once

#include <Rtrc/Core/Container/Span.h>
#include <Rtrc/Core/EnumFlags.h>
#include <Rtrc/Core/Event.h>
#include <Rtrc/Core/SmartPointer/ReferenceCounted.h>
#include <Rtrc/Core/Uncopyable.h>
#include <Rtrc/RHI/Window.h>

RTRC_RHI_BEGIN
#define RTRC_RHI_DECLARE_TYPE(NAME) \
    class NAME;                     \
    using NAME##Ref = ReferenceCountedPtr<NAME>

RTRC_RHI_DECLARE_TYPE(Texture);
RTRC_RHI_DECLARE_TYPE(TextureSRV);
RTRC_RHI_DECLARE_TYPE(TextureUAV);
RTRC_RHI_DECLARE_TYPE(TextureRTV);
RTRC_RHI_DECLARE_TYPE(TextureDSV);
RTRC_RHI_DECLARE_TYPE(Buffer);
RTRC_RHI_DECLARE_TYPE(BufferSRV);
RTRC_RHI_DECLARE_TYPE(BufferUAV);
RTRC_RHI_DECLARE_TYPE(Shader);
RTRC_RHI_DECLARE_TYPE(GraphicsPipeline);
RTRC_RHI_DECLARE_TYPE(ComputePipeline);
RTRC_RHI_DECLARE_TYPE(Fence);
RTRC_RHI_DECLARE_TYPE(Semaphore);
RTRC_RHI_DECLARE_TYPE(CommandBuffer);
RTRC_RHI_DECLARE_TYPE(Queue);
RTRC_RHI_DECLARE_TYPE(Device);

#undef RTRC_RHI_DECLARE_TYPE

#define RTRC_RHI_API virtual
#define RTRC_RHI_PURE = 0
#define RTRC_RHI_OVERRIDE override

enum class QueueType
{
    Graphics,
    Compute,
    Copy
};

enum class TextureDimension : uint8_t
{
    Tex1D,
    Tex2D,
    Tex3D
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

enum class QueueAccessMode : uint8_t
{
    Exclusive,
    Concurrent
};

enum class BufferHostAccessMode : uint8_t
{
    Default,
    Upload,
    Readback
};

enum class TextureUsageBit : uint32_t
{
    None            = 0u,
    CopySrc         = 1u << 0,
    CopyDst         = 1u << 1,
    ShaderResource  = 1u << 2,
    UnorderedAccess   = 1u << 3,
    RenderTarget    = 1u << 4,
    DepthStencil    = 1u << 5,
};
RTRC_DEFINE_ENUM_FLAGS(TextureUsageBit)
using TextureUsage = EnumFlagsTextureUsageBit;

enum class BufferUsageBit : uint32_t
{
    None              = 0u,
    CopySrc           = 1u << 0,
    CopyDst           = 1u << 1,
    ShaderResource    = 1u << 2,
    UnorderedAccess   = 1u << 3,
    VertexIndexBuffer = 1u << 4,
};
RTRC_DEFINE_ENUM_FLAGS(BufferUsageBit)
using BufferUsage = EnumFlagsBufferUsageBit;

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
    TextureDimension          dimension = TextureDimension::Tex2D;
    Format                    format;
    uint32_t                  width;
    uint32_t                  height      = 1;
    uint32_t                  depth       = 1;
    uint32_t                  mipLevels   = 1;
    uint32_t                  arrayLayers = 1;
    TextureUsage              usage;
    uint32_t                  MSAASampleCount = 1;
    uint32_t                  MSAASampleQuality = 0;
    QueueAccessMode           queueAccessMode = QueueAccessMode::Exclusive;
    std::optional<ClearValue> fastClearValue;

    auto operator<=>(const TextureDesc &) const = default;
};

struct TextureSRVDesc
{
    Format format = Format::Unknown; // unknown means auto

    uint32_t firstMipLevel = 0;
    uint32_t mipLevels = 0; // 0 means all levels from firstMipLevel

    bool isArray = false;
    uint32_t firstArrayLayer = 0;
    uint32_t arrayLayers = 0; // 0 means all layers from firstArrayLayer. Used only when isArray is true

    auto operator<=>(const TextureSRVDesc &) const = default;
};

struct TextureUAVDesc
{
    Format format = Format::Unknown; // unknown means auto

    uint32_t mipLevel = 0;

    bool isArray = false;
    uint32_t firstArrayLayer = 0;
    uint32_t arrayLayers = 0; // 0 means all layers from firstArrayLayer. Used only when isArray is true.

    uint32_t firstDepthLayer = 0;
    uint32_t depthLayers = 0; // 0 means all layers from firstDepthLayer. Used only for 3d texture.

    auto operator<=>(const TextureUAVDesc &) const = default;
};

struct TextureRTVDesc
{
    Format format = Format::Unknown; // unknown means auto
    uint32_t mipLevel = 0;

    auto operator<=>(const TextureRTVDesc &) const = default;
};

struct TextureDSVDesc
{
    Format format = Format::Unknown; // unknown means auto
    uint32_t mipLevel = 0;
    uint32_t arrayLayer = 0;
    bool isDepthReadOnly = false;
    bool isStencilReadOnly = false;

    auto operator<=>(const TextureDSVDesc &) const = default;
};

struct BufferDesc
{
    size_t               size;
    BufferUsage          usage;
    BufferHostAccessMode hostAccessMode = BufferHostAccessMode::Default;
    QueueAccessMode      queueAccessMode = QueueAccessMode::Exclusive;

    auto operator<=>(const BufferDesc &) const = default;
};

// Typed buffer       : fill offset, element and count
// Structured buffer  : fill offset, stride and count
// Byte address buffer: fill offset and count
// For byte address buffer, element size is treated as 4.
struct BufferSRVDesc
{
    size_t offset = 0;               // In size of element
    Format format = Format::Unknown; // Typed buffer only
    size_t stride = 0;               // Structured buffer only
    size_t count  = 0;               // Element count

    auto operator<=>(const BufferSRVDesc &) const = default;
};

struct BufferUAVDesc
{
    size_t offset = 0;               // In size of element
    Format format = Format::Unknown; // Typed buffer only
    size_t stride = 0;               // Structured buffer only
    size_t count  = 0;               // Element count

    auto operator<=>(const BufferUAVDesc &) const = default;
};

struct DeviceDesc
{
    bool debugMode     = RTRC_DEBUG;
    bool gpuValidation = false;

    bool graphicsQueue = true;
    bool presentQueue  = true;
    bool computeQueue  = true;
    bool copyQueue     = true;

    WindowRef swapchainWindow;
    Format    swapchainFormat         = Format::B8G8R8A8_UNorm;
    bool      enableSwapchainImageUAV = false;
    uint32_t  swapchainImageCount     = 2;

    bool vsync = true;
};

class RHIObject : public ReferenceCounted, public Uncopyable
{
public:

    virtual ~RHIObject() = default;

    RTRC_RHI_API void SetName(std::string name) { }
};

class Texture : public RHIObject
{
public:

    RTRC_RHI_API const TextureDesc &GetDesc() const RTRC_RHI_PURE;

    TextureDimension GetDimension() const { return GetDesc().dimension; }
    uint32_t GetWidth()  const { return GetDesc().width; }
    uint32_t GetHeight() const { return GetDesc().height; }
    uint32_t GetDepth()  const { return GetDesc().depth; }

    uint32_t GetMSAASampleCount() const { return GetDesc().MSAASampleCount; }
    uint32_t GetMSAASampleQuality() const { return GetDesc().MSAASampleQuality; }
    bool IsMSAAEnabled() const { return GetDesc().MSAASampleCount > 1; }

    RTRC_RHI_API TextureSRVRef GetSRV(const TextureSRVDesc &desc) RTRC_RHI_PURE;
    RTRC_RHI_API TextureUAVRef GetUAV(const TextureUAVDesc &desc) RTRC_RHI_PURE;
    RTRC_RHI_API TextureRTVRef GetRTV(const TextureRTVDesc &desc) RTRC_RHI_PURE;
    RTRC_RHI_API TextureDSVRef GetDSV(const TextureDSVDesc &desc) RTRC_RHI_PURE;
};

class TextureSRV : public RHIObject
{

};

class TextureUAV : public RHIObject
{

};

class TextureRTV : public RHIObject
{

};

class TextureDSV : public RHIObject
{

};

class Buffer : public RHIObject
{
public:

    RTRC_RHI_API const BufferDesc &GetDesc() const RTRC_RHI_PURE;

    RTRC_RHI_API BufferSRVRef GetSRV(const BufferSRVDesc &desc) RTRC_RHI_PURE;
    RTRC_RHI_API BufferUAVRef GetUAV(const BufferUAVDesc &desc) RTRC_RHI_PURE;
};

class BufferSRV : public RHIObject
{

};

class BufferUAV : public RHIObject
{

};

class Shader : public RHIObject
{
    
};

class GraphicsPipeline : public RHIObject
{
    
};

class ComputePipeline : public RHIObject
{
    
};

class Fence : public RHIObject
{
public:

    RTRC_RHI_API void Wait() RTRC_RHI_PURE;
};

class Semaphore : public RHIObject
{

};

class CommandBuffer : public RHIObject
{

};

class Queue : public RHIObject
{
public:

    RTRC_RHI_API CommandBufferRef NewCommandBuffer() RTRC_RHI_PURE;
    RTRC_RHI_API void Submit(Span<CommandBufferRef> commandBuffer) RTRC_RHI_PURE;

    RTRC_RHI_API void SignalFence(FenceRef fence) RTRC_RHI_PURE;
    RTRC_RHI_API void SignalSemaphore(SemaphoreRef semaphore) RTRC_RHI_PURE;
    RTRC_RHI_API void WaitSemaphore(SemaphoreRef semaphore) RTRC_RHI_PURE;

    RTRC_RHI_API void WaitIdle() RTRC_RHI_PURE;
};

class Device : public RHIObject
{
public:

    static DeviceRef Create(const DeviceDesc &desc);

    RTRC_RHI_API void BeginThreadedMode() RTRC_RHI_PURE;
    RTRC_RHI_API void EndThreadedMode() RTRC_RHI_PURE;

    RTRC_RHI_API QueueRef GetQueue(QueueType queue) RTRC_RHI_PURE;
    RTRC_RHI_API TextureRef GetSwapchainImage() RTRC_RHI_PURE;
    RTRC_RHI_API bool Present() RTRC_RHI_PURE;

    RTRC_RHI_API TextureRef CreateTexture(const TextureDesc &desc) RTRC_RHI_PURE;
    RTRC_RHI_API BufferRef CreateBuffer(const BufferDesc &desc) RTRC_RHI_PURE;

    RTRC_RHI_API FenceRef CreateQueueFence() RTRC_RHI_PURE;
    RTRC_RHI_API SemaphoreRef CreateQueueSemaphore() RTRC_RHI_PURE;
};

RTRC_RHI_END
