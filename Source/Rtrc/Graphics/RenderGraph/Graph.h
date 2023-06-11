#pragma once

#include <map>
#include <stack>

#include <Rtrc/Graphics/Device/Buffer.h>
#include <Rtrc/Graphics/Device/Queue.h>
#include <Rtrc/Graphics/Device/Texture.h>

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
    .layout   = RHI::TextureLayout::DepthStencilAttachment,
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

    RC<Buffer> Get(PassContext &passContext) const;
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

    RC<Texture> Get(PassContext &passContext) const;
};

class PassContext : public Uncopyable
{
public:

    CommandBuffer &GetCommandBuffer();

    RC<Buffer> Get(const BufferResource *resource);
    RC<Texture> Get(const TextureResource *resource);

private:

    friend class Executer;

    PassContext(const ExecutableResources &resources, CommandBuffer &commandBuffer);

    const ExecutableResources &resources_;
    CommandBuffer &commandBuffer_;

#if RTRC_RG_DEBUG
    const std::set<const Resource *> *declaredResources_ = nullptr;
#endif
};

void Connect(Pass *head, Pass *tail);

class Pass
{
public:

    using Callback = std::function<void(PassContext &)>;

    friend void Connect(Pass *head, Pass *tail);

    Pass *Use(
        BufferResource         *buffer,
        RHI::PipelineStageFlag  stages,
        RHI::ResourceAccessFlag accesses);

    Pass *Use(
        TextureResource        *texture,
        RHI::TextureLayout      layout,
        RHI::PipelineStageFlag  stages,
        RHI::ResourceAccessFlag accesses);

    Pass *Use(
        TextureResource               *texture,
        const RHI::TextureSubresource &subresource,
        RHI::TextureLayout             layout,
        RHI::PipelineStageFlag         stages,
        RHI::ResourceAccessFlag        accesses);

    Pass *Use(BufferResource *buffer, const UseInfo &info);
    Pass *Use(TextureResource *texture, const UseInfo &info);
    Pass *Use(TextureResource *texture, const RHI::TextureSubresource &subrsc, const UseInfo &info);

    Pass *SetCallback(Callback callback);

    Pass *SetSignalFence(RHI::FencePtr fence);

private:

    using SubTexUsage = TextureSubrscState;
    using BufferUsage = BufferState;

    using TextureUsage = TextureSubrscMap<std::optional<SubTexUsage>>;

    friend class RenderGraph;
    friend class Compiler;

    Pass(int index, std::string name);
    
    int           index_;
    std::string   name_;
    Callback      callback_;
    RHI::FencePtr signalFence_;

    std::map<BufferResource *, BufferUsage> bufferUsages_;
    std::map<TextureResource *, TextureUsage> textureUsages_;

    std::set<Pass *> prevs_;
    std::set<Pass *> succs_;
};

class RenderGraph : public Uncopyable
{
public:

    explicit RenderGraph(ObserverPtr<Device> device, Queue queue = Queue(nullptr));

    void SetQueue(Queue queue);

    BufferResource  *CreateBuffer(const RHI::BufferDesc &desc, std::string name = {});
    TextureResource *CreateTexture(const RHI::TextureDesc &desc, std::string name = {});

    BufferResource  *RegisterBuffer(RC<StatefulBuffer> buffer);
    TextureResource *RegisterTexture(RC<StatefulTexture> texture);
    TextureResource *RegisterReadOnlyTexture(RC<Texture> texture);

    TextureResource *RegisterSwapchainTexture(
        RHI::TexturePtr             rhiTexture,
        RHI::BackBufferSemaphorePtr acquireSemaphore,
        RHI::BackBufferSemaphorePtr presentSemaphore);

    TextureResource *RegisterSwapchainTexture(const RHI::SwapchainPtr &swapchain);

    void PushPassGroup(std::string_view name);
    void PopPassGroup();

    Pass *CreatePass(std::string_view name);
    Pass *CreateClearTexture2DPass(std::string_view name, TextureResource *tex2D, const Vector4f &clearValue);
    Pass *CreateDummyPass(std::string_view name);

    Pass *CreateClearRWBufferPass(std::string_view name, BufferResource *buffer, uint32_t value);
    Pass *CreateClearRWStructuredBufferPass(std::string_view name, BufferResource *buffer, uint32_t value);
    
    void MakeDummyPassIfNull(Pass *&pass, std::string_view name);

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

    std::string        passNamePrefix_;
    std::stack<size_t> passNamePrefixLengths_;

    std::map<const void *, int>       externalResourceMap_; // RHI object pointer to resource index
    std::vector<Box<BufferResource>>  buffers_;
    std::vector<Box<TextureResource>> textures_;
    SwapchainTexture                 *swapchainTexture_;
    mutable ExecutableResources      *executableResource_;

    std::vector<Box<Pass>> passes_;

    RHI::FencePtr completeFence_;
};

#define RTRC_RG_SCOPED_PASS_GROUP(RENDERGRAPH, NAME) \
    (RENDERGRAPH).PushPassGroup(NAME);               \
    RTRC_SCOPE_EXIT{ (RENDERGRAPH).PopPassGroup(); };

RTRC_RG_END
