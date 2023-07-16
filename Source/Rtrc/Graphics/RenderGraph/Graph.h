#pragma once

#include <map>
#include <stack>

#include <Rtrc/Graphics/Device/Buffer.h>
#include <Rtrc/Graphics/Device/Queue.h>
#include <Rtrc/Graphics/Device/Texture.h>
#include <Rtrc/Graphics/RenderGraph/Label.h>
#include <Rtrc/Utility/SmartPointer/ObserverPtr.h>

#define RTRC_RG_DEBUG RTRC_DEBUG

RTRC_BEGIN

class Device;

RTRC_END

RTRC_RG_BEGIN

struct ExecutableResources;

class RenderGraph;
class Executer;
class Pass;
class PassContext;
class Compiler;

struct UseInfo
{
    RHI::TextureLayout      layout   = RHI::TextureLayout::Undefined;
    RHI::PipelineStageFlag  stages   = RHI::PipelineStage::None;
    RHI::ResourceAccessFlag accesses = RHI::ResourceAccess::None;
};

constexpr UseInfo operator|(const UseInfo &lhs, const UseInfo &rhs)
{
    assert(lhs.layout == rhs.layout);
    return UseInfo
    {
        .layout   = lhs.layout,
        .stages   = lhs.stages   | rhs.stages,
        .accesses = lhs.accesses | rhs.accesses
    };
}

inline constexpr UseInfo ColorAttachment =
{
    .layout   = RHI::TextureLayout::ColorAttachment,
    .stages   = RHI::PipelineStage::RenderTarget,
    .accesses = RHI::ResourceAccess::RenderTargetWrite | RHI::ResourceAccess::RenderTargetRead
};

inline constexpr UseInfo ColorAttachmentReadOnly =
{
    .layout   = RHI::TextureLayout::ColorAttachment,
    .stages   = RHI::PipelineStage::RenderTarget,
    .accesses = RHI::ResourceAccess::RenderTargetRead
};

inline constexpr UseInfo ColorAttachmentWriteOnly =
{
    .layout   = RHI::TextureLayout::ColorAttachment,
    .stages   = RHI::PipelineStage::RenderTarget,
    .accesses = RHI::ResourceAccess::RenderTargetWrite
};

inline constexpr UseInfo DepthStencilAttachment =
{
    .layout   = RHI::TextureLayout::DepthStencilAttachment,
    .stages   = RHI::PipelineStage::DepthStencil,
    .accesses = RHI::ResourceAccess::DepthStencilRead | RHI::ResourceAccess::DepthStencilWrite
};

inline constexpr UseInfo DepthStencilAttachmentWriteOnly =
{
    .layout   = RHI::TextureLayout::DepthStencilAttachment,
    .stages   = RHI::PipelineStage::DepthStencil,
    .accesses = RHI::ResourceAccess::DepthStencilWrite
};

inline constexpr UseInfo DepthStencilAttachmentReadOnly =
{
    .layout   = RHI::TextureLayout::DepthStencilReadOnlyAttachment,
    .stages   = RHI::PipelineStage::DepthStencil,
    .accesses = RHI::ResourceAccess::DepthStencilRead
};

inline constexpr UseInfo ClearDst =
{
    .layout   = RHI::TextureLayout::ClearDst,
    .stages   = RHI::PipelineStage::Clear,
    .accesses = RHI::ResourceAccess::ClearWrite
};

inline constexpr UseInfo PS_Texture =
{
    .layout   = RHI::TextureLayout::ShaderTexture,
    .stages   = RHI::PipelineStage::FragmentShader,
    .accesses = RHI::ResourceAccess::TextureRead
};

inline constexpr UseInfo CS_Texture =
{
    .layout   = RHI::TextureLayout::ShaderTexture,
    .stages   = RHI::PipelineStage::ComputeShader,
    .accesses = RHI::ResourceAccess::TextureRead
};

inline constexpr UseInfo CS_RWTexture =
{
    .layout   = RHI::TextureLayout::ShaderRWTexture,
    .stages   = RHI::PipelineStage::ComputeShader,
    .accesses = RHI::ResourceAccess::RWTextureRead | RHI::ResourceAccess::RWTextureWrite
};

inline constexpr UseInfo CS_RWTexture_WriteOnly =
{
    .layout   = RHI::TextureLayout::ShaderRWTexture,
    .stages   = RHI::PipelineStage::ComputeShader,
    .accesses = RHI::ResourceAccess::RWTextureWrite
};

inline constexpr UseInfo CS_Buffer =
{
    .layout   = RHI::TextureLayout::Undefined,
    .stages   = RHI::PipelineStage::ComputeShader,
    .accesses = RHI::ResourceAccess::BufferRead
};

inline constexpr UseInfo CS_RWBuffer =
{
    .layout   = RHI::TextureLayout::Undefined,
    .stages   = RHI::PipelineStage::ComputeShader,
    .accesses = RHI::ResourceAccess::RWBufferRead | RHI::ResourceAccess::RWBufferWrite
};

inline constexpr UseInfo CS_RWBuffer_WriteOnly =
{
    .layout   = RHI::TextureLayout::Undefined,
    .stages   = RHI::PipelineStage::ComputeShader,
    .accesses = RHI::ResourceAccess::RWBufferWrite
};

inline constexpr UseInfo CS_StructuredBuffer =
{
    .layout   = RHI::TextureLayout::Undefined,
    .stages   = RHI::PipelineStage::ComputeShader,
    .accesses = RHI::ResourceAccess::StructuredBufferRead
};

inline constexpr UseInfo CS_RWStructuredBuffer =
{
    .layout   = RHI::TextureLayout::Undefined,
    .stages   = RHI::PipelineStage::ComputeShader,
    .accesses = RHI::ResourceAccess::RWStructuredBufferRead | RHI::ResourceAccess::RWStructuredBufferWrite
};

inline constexpr UseInfo CS_RWStructuredBuffer_WriteOnly =
{
    .layout   = RHI::TextureLayout::Undefined,
    .stages   = RHI::PipelineStage::ComputeShader,
    .accesses = RHI::ResourceAccess::RWStructuredBufferWrite
};

inline constexpr UseInfo CopyDst =
{
    .layout   = RHI::TextureLayout::CopyDst,
    .stages   = RHI::PipelineStage::Copy,
    .accesses = RHI::ResourceAccess::CopyWrite
};

inline constexpr UseInfo CopySrc =
{
    .layout   = RHI::TextureLayout::CopySrc,
    .stages   = RHI::PipelineStage::Copy,
    .accesses = RHI::ResourceAccess::CopyRead
};

inline constexpr UseInfo BuildAS_Output =
{
    .layout   = RHI::TextureLayout::Undefined,
    .stages   = RHI::PipelineStage::BuildAS,
    .accesses = RHI::ResourceAccess::WriteAS
};

inline constexpr UseInfo BuildAS_Scratch =
{
    .layout   = RHI::TextureLayout::Undefined,
    .stages   = RHI::PipelineStage::BuildAS,
    .accesses = RHI::ResourceAccess::BuildASScratch
};

inline constexpr UseInfo CS_ReadAS =
{
    .layout   = RHI::TextureLayout::Undefined,
    .stages   = RHI::PipelineStage::ComputeShader,
    .accesses = RHI::ResourceAccess::ReadAS
};

inline constexpr UseInfo IndirectDispatchRead =
{
    .layout   = RHI::TextureLayout::Undefined,
    .stages   = RHI::PipelineStage::IndirectCommand,
    .accesses = RHI::ResourceAccess::IndirectCommandRead
};

class Resource : public Uncopyable
{
    int index_;

public:

    explicit Resource(int resourceIndex) : index_(resourceIndex) { }

    virtual ~Resource() = default;

    int GetResourceIndex() const { return index_; }
};

template<typename T>
const T *TryCastResource(const Resource *rsc) { return dynamic_cast<const T *>(rsc); }

template<typename T>
T *TryCastResource(Resource *rsc) { return dynamic_cast<T *>(rsc); }

class BufferResource : public Resource
{
public:

    using Resource::Resource;

    RC<Buffer> Get() const;

    // Helper methods available when executing pass callback function:

    BufferSrv GetStructuredSrv();
    BufferSrv GetStructuredSrv(size_t structStride);
    BufferSrv GetStructuredSrv(size_t byteOffset, size_t structStride);

    BufferSrv GetTexelSrv();
    BufferSrv GetTexelSrv(RHI::Format texelFormat);
    BufferSrv GetTexelSrv(size_t byteOffset, RHI::Format texelFormat);

    BufferUav GetStructuredUav();
    BufferUav GetStructuredUav(size_t structStride);
    BufferUav GetStructuredUav(size_t byteOffset, size_t structStride);

    BufferUav GetTexelUav();
    BufferUav GetTexelUav(RHI::Format texelFormat);
    BufferUav GetTexelUav(size_t byteOffset, RHI::Format texelFormat);
};

class TlasResource
{
    friend class RenderGraph;

    RC<Tlas>        tlas_;
    BufferResource *tlasBuffer_;

    TlasResource(RC<Tlas> tlas, BufferResource *tlasBuffer): tlas_(std::move(tlas)), tlasBuffer_(tlasBuffer) { }

public:

    BufferResource *GetInternalBuffer() const { return tlasBuffer_; }

    const RC<Tlas> &Get() const;
};

class TextureResource : public Resource
{
public:

    using Resource::Resource;

    virtual const RHI::TextureDesc &GetDesc() const = 0;

    RHI::TextureDimension GetDimension() const { return GetDesc().dim; }
    RHI::Format           GetFormat()    const { return GetDesc().format; }
    uint32_t              GetWidth()     const { return GetDesc().width; }
    uint32_t              GetHeight()    const { return GetDesc().height; }
    Vector2u              GetSize()      const { return { GetWidth(), GetHeight() }; }
    uint32_t              GetDepth()     const { return GetDesc().depth; }
    uint32_t              GetMipLevels() const { return GetDesc().mipLevels; }
    uint32_t              GetArraySize() const { return GetDesc().arraySize; }

    RHI::Viewport GetViewport(float minDepth = 0, float maxDepth = 1) const;
    RHI::Scissor GetScissor() const;

    RC<Texture> Get() const;

    // Helper methods available when executing pass callback function:

    // non-array view for single-layer texture, array view for multi-layer texture
    TextureSrv CreateSrv(RHI::TextureViewFlags flags = 0);
    // non-array view
    TextureSrv CreateSrv(uint32_t mipLevel, uint32_t levelCount, uint32_t arrayLayer, RHI::TextureViewFlags flags = 0);
    // array view
    TextureSrv CreateSrv(
        uint32_t mipLevel, uint32_t levelCount, uint32_t arrayLayer, uint32_t layerCount, RHI::TextureViewFlags flags = 0);
    // non-array view for single-layer texture, array view for multi-layer texture
    TextureUav CreateUav();
    // non-array view
    TextureUav CreateUav(uint32_t mipLevel, uint32_t arrayLayer);
    // array view
    TextureUav CreateUav(uint32_t mipLevel, uint32_t arrayLayer, uint32_t layerCount);
    TextureRtv CreateRtv(uint32_t mipLevel = 0, uint32_t arrayLayer = 0);
    TextureDsv CreateDsv(RHI::TextureViewFlags flags);
    TextureDsv CreateDsv(uint32_t mipLevel = 0, uint32_t arrayLayer = 0, RHI::TextureViewFlags flags = 0);
};

class PassContext : public Uncopyable
{
public:

    CommandBuffer &GetCommandBuffer();

    RC<Buffer>      Get(const BufferResource *resource);
    const RC<Tlas> &Get(const TlasResource *resource);
    RC<Texture>     Get(const TextureResource *resource);

private:

    friend class Executer;

    PassContext(const ExecutableResources &resources, CommandBuffer &commandBuffer);
    ~PassContext();

    const ExecutableResources &resources_;
    CommandBuffer &commandBuffer_;

#if RTRC_RG_DEBUG
    const std::set<const Resource *> *declaredResources_ = nullptr;
#endif
};

PassContext   &GetCurrentPassContext();
CommandBuffer &GetCurrentCommandBuffer();

void Connect(Pass *head, Pass *tail);

class Pass
{
public:

    using Callback = std::function<void()>;
    using LegacyCallback = std::function<void(PassContext &)>;

    friend void Connect(Pass *head, Pass *tail);

    Pass *Use(BufferResource *buffer, const UseInfo &info);

    Pass *Use(TextureResource *texture, const UseInfo &info);
    Pass *Use(TextureResource *texture, const RHI::TextureSubresource &subrsc, const UseInfo &info);

    Pass *Use(TlasResource *tlas, const UseInfo &info);
    Pass *Build(TlasResource *tlas);
    Pass *Read(TlasResource *tlas, RHI::PipelineStageFlag stages);

    Pass *SetCallback(Callback callback);
    Pass *SetCallback(LegacyCallback callback);

    Pass *SetSignalFence(RHI::FencePtr fence);

private:

    using SubTexUsage = TextureSubrscState;
    using BufferUsage = BufferState;

    using TextureUsage = TextureSubrscMap<std::optional<SubTexUsage>>;

    friend class RenderGraph;
    friend class Compiler;

    Pass(int index, const LabelStack::Node *node);
    
    int           index_;
    Callback      callback_;
    RHI::FencePtr signalFence_;

    const LabelStack::Node *nameNode_;

    std::map<BufferResource *, BufferUsage> bufferUsages_;
    std::map<TextureResource *, TextureUsage> textureUsages_;

    std::set<Pass *> prevs_;
    std::set<Pass *> succs_;
};

class RenderGraph : public Uncopyable
{
public:

    explicit RenderGraph(ObserverPtr<Device> device, Queue queue = Queue(nullptr));

    ObserverPtr<Device> GetDevice() const { return device_; }

    const Queue &GetQueue() const { return queue_; }
    void SetQueue(Queue queue) { queue_ = std::move(queue); }

    BufferResource  *CreateBuffer(const RHI::BufferDesc &desc, std::string name = {});
    TextureResource *CreateTexture(const RHI::TextureDesc &desc, std::string name = {});

    BufferResource  *RegisterBuffer(RC<StatefulBuffer> buffer);
    TextureResource *RegisterTexture(RC<StatefulTexture> texture);
    TextureResource *RegisterReadOnlyTexture(RC<Texture> texture);
    TlasResource    *RegisterTlas(RC<Tlas> tlas, BufferResource *internalBuffer);

    TextureResource *RegisterSwapchainTexture(
        RHI::TexturePtr             rhiTexture,
        RHI::BackBufferSemaphorePtr acquireSemaphore,
        RHI::BackBufferSemaphorePtr presentSemaphore);

    TextureResource *RegisterSwapchainTexture(const RHI::SwapchainPtr &swapchain);

    void PushPassGroup(std::string name);
    void PopPassGroup();

    Pass *CreatePass(std::string name);
    Pass *CreateClearTexture2DPass(std::string name, TextureResource *tex2D, const Vector4f &clearValue);
    Pass *CreateDummyPass(std::string name);

    Pass *CreateClearRWBufferPass(std::string name, BufferResource *buffer, uint32_t value);
    Pass *CreateClearRWStructuredBufferPass(std::string name, BufferResource *buffer, uint32_t value);

    Pass* CreateCopyBufferPass(
        std::string name,
        BufferResource *src, size_t srcOffset,
        BufferResource *dst, size_t dstOffset,
        size_t size);
    Pass *CreateCopyBufferPass(
        std::string name,
        BufferResource *src,
        BufferResource *dst,
        size_t size);

    Pass *CreateBlitTexture2DPass(
        std::string name,
        TextureResource *src, uint32_t srcArrayLayer, uint32_t srcMipLevel,
        TextureResource *dst, uint32_t dstArrayLayer, uint32_t dstMipLevel,
        bool usePointSampling = true, float gamma = 1.0f);
    Pass *CreateBlitTexture2DPass(
        std::string name, TextureResource *src, TextureResource *dst, bool usePointSampling = true, float gamma = 1.0f);
    
    void SetCompleteFence(RHI::FencePtr fence);

private:

    friend class Compiler;
    friend class Executer;
    friend class BufferResource;
    friend class TextureResource;

    class ExternalBufferResource : public BufferResource
    {
    public:

        using BufferResource::BufferResource;

        RC<StatefulBuffer> buffer;
    };

    class ExternalTextureResource : public TextureResource
    {
    public:

        using TextureResource::TextureResource;

        const RHI::TextureDesc &GetDesc() const override;

        bool isReadOnlySampledTexture = false;
        RC<StatefulTexture> texture;
    };

    class InternalBufferResource : public BufferResource
    {
    public:

        using BufferResource::BufferResource;

        RHI::BufferDesc rhiDesc;
        std::string name;
    };

    class InternalTextureResource : public TextureResource
    {
    public:

        using TextureResource::TextureResource;

        const RHI::TextureDesc &GetDesc() const override;

        RHI::TextureDesc rhiDesc;
        std::string name;
    };

    class SwapchainTexture : public ExternalTextureResource
    {
    public:

        using ExternalTextureResource::ExternalTextureResource;

        RHI::BackBufferSemaphorePtr acquireSemaphore;
        RHI::BackBufferSemaphorePtr presentSemaphore;
    };

    ObserverPtr<Device> device_;
    Queue               queue_;

    LabelStack labelStack_;
    
    std::map<const void *, int>       externalResourceMap_; // RHI object pointer to resource index
    std::vector<Box<BufferResource>>  buffers_;
    std::vector<Box<TextureResource>> textures_;
    SwapchainTexture                 *swapchainTexture_;
    mutable ExecutableResources      *executableResource_;

    std::map<const BufferResource *, Box<TlasResource>> tlasResources_;

    std::vector<Box<Pass>> passes_;

    RHI::FencePtr completeFence_;
};

#define RTRC_RG_SCOPED_PASS_GROUP(RENDERGRAPH, NAME)       \
    ::Rtrc::ObserverPtr(RENDERGRAPH)->PushPassGroup(NAME); \
    RTRC_SCOPE_EXIT{ ::Rtrc::ObserverPtr(RENDERGRAPH)->PopPassGroup(); };

inline BufferSrv BufferResource::GetStructuredSrv()
{
    return Get()->GetStructuredSrv();
}

inline BufferSrv BufferResource::GetStructuredSrv(size_t structStride)
{
    return Get()->GetStructuredSrv(structStride);
}

inline BufferSrv BufferResource::GetStructuredSrv(size_t byteOffset, size_t structStride)
{
    return Get()->GetStructuredSrv(byteOffset, structStride);
}

inline BufferSrv BufferResource::GetTexelSrv()
{
    return Get()->GetTexelSrv();
}

inline BufferSrv BufferResource::GetTexelSrv(RHI::Format texelFormat)
{
    return Get()->GetTexelSrv(texelFormat);
}

inline BufferSrv BufferResource::GetTexelSrv(size_t byteOffset, RHI::Format texelFormat)
{
    return Get()->GetTexelSrv(byteOffset, texelFormat);
}

inline BufferUav BufferResource::GetStructuredUav()
{
    return Get()->GetStructuredUav();
}

inline BufferUav BufferResource::GetStructuredUav(size_t structStride)
{
    return Get()->GetStructuredUav(structStride);
}

inline BufferUav BufferResource::GetStructuredUav(size_t byteOffset, size_t structStride)
{
    return Get()->GetStructuredUav(byteOffset, structStride);
}

inline BufferUav BufferResource::GetTexelUav()
{
    return Get()->GetTexelUav();
}

inline BufferUav BufferResource::GetTexelUav(RHI::Format texelFormat)
{
    return Get()->GetTexelUav(texelFormat);
}

inline BufferUav BufferResource::GetTexelUav(size_t byteOffset, RHI::Format texelFormat)
{
    return Get()->GetTexelUav(byteOffset, texelFormat);
}

inline TextureSrv TextureResource::CreateSrv(RHI::TextureViewFlags flags)
{
    return Get()->CreateSrv(flags);
}

inline TextureSrv TextureResource::CreateSrv(
    uint32_t mipLevel, uint32_t levelCount, uint32_t arrayLayer, RHI::TextureViewFlags flags)
{
    return Get()->CreateSrv(mipLevel, levelCount, arrayLayer, flags);
}

inline TextureSrv TextureResource::CreateSrv(
    uint32_t mipLevel, uint32_t levelCount, uint32_t arrayLayer, uint32_t layerCount, RHI::TextureViewFlags flags)
{
    return Get()->CreateSrv(mipLevel, levelCount, arrayLayer, layerCount, flags);
}

inline TextureUav TextureResource::CreateUav()
{
    return Get()->CreateUav();
}

inline TextureUav TextureResource::CreateUav(uint32_t mipLevel, uint32_t arrayLayer)
{
    return Get()->CreateUav(mipLevel, arrayLayer);
}

inline TextureUav TextureResource::CreateUav(uint32_t mipLevel, uint32_t arrayLayer, uint32_t layerCount)
{
    return Get()->CreateUav(mipLevel, arrayLayer, layerCount);
}

inline TextureDsv TextureResource::CreateDsv(RHI::TextureViewFlags flags)
{
    return Get()->CreateDsv(flags);
}

inline TextureDsv TextureResource::CreateDsv(uint32_t mipLevel, uint32_t arrayLayer, RHI::TextureViewFlags flags)
{
    return Get()->CreateDsv(mipLevel, arrayLayer, flags);
}

inline TextureRtv TextureResource::CreateRtv(uint32_t mipLevel, uint32_t arrayLayer)
{
    return Get()->CreateRtv(mipLevel, arrayLayer);
}

RTRC_RG_END
