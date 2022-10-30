#pragma once

#include <map>

#include <Rtrc/Graphics/Object/Buffer.h>
#include <Rtrc/Graphics/Object/Queue.h>
#include <Rtrc/Graphics/Object/Texture.h>

RTRC_BEGIN

class CommandBuffer;
class RenderContext;

RTRC_END

RTRC_RG_BEGIN

struct ExecutableResources;

class RenderGraph;
class Executer;
class Pass;
class Compiler;

struct UseInfo
{
    RHI::TextureLayout      layout   = RHI::TextureLayout::Undefined;
    RHI::PipelineStageFlag  stages   = RHI::PipelineStage::None;
    RHI::ResourceAccessFlag accesses = RHI::ResourceAccess::None;
};

inline constexpr UseInfo RENDER_TARGET =
{
    .layout   = RHI::TextureLayout::RenderTarget,
    .stages   = RHI::PipelineStage::RenderTarget,
    .accesses = RHI::ResourceAccess::RenderTargetWrite | RHI::ResourceAccess::RenderTargetRead
};

inline constexpr UseInfo RENDER_TARGET_READ =
{
    .layout   = RHI::TextureLayout::RenderTarget,
    .stages   = RHI::PipelineStage::RenderTarget,
    .accesses = RHI::ResourceAccess::RenderTargetRead
};

inline constexpr UseInfo RENDER_TARGET_WRITE =
{
    .layout   = RHI::TextureLayout::RenderTarget,
    .stages   = RHI::PipelineStage::RenderTarget,
    .accesses = RHI::ResourceAccess::RenderTargetWrite
};

inline constexpr UseInfo DEPTH_STENCIL =
{
    .layout   = RHI::TextureLayout::DepthStencil,
    .stages   = RHI::PipelineStage::DepthStencil,
    .accesses = RHI::ResourceAccess::DepthStencilRead | RHI::ResourceAccess::DepthStencilWrite
};

inline constexpr UseInfo DEPTH_STENCIL_WRITE =
{
    .layout   = RHI::TextureLayout::DepthStencil,
    .stages   = RHI::PipelineStage::DepthStencil,
    .accesses = RHI::ResourceAccess::DepthStencilWrite
};

inline constexpr UseInfo DEPTH_STENCIL_READ =
{
    .layout   = RHI::TextureLayout::DepthStencil,
    .stages   = RHI::PipelineStage::DepthStencil,
    .accesses = RHI::ResourceAccess::DepthStencilRead
};

inline constexpr UseInfo CLEAR_DST =
{
    .layout   = RHI::TextureLayout::ClearDst,
    .stages   = RHI::PipelineStage::Clear,
    .accesses = RHI::ResourceAccess::ClearWrite
};

inline constexpr UseInfo PIXEL_SHADER_TEXTURE_READ =
{
    .layout = RHI::TextureLayout::ShaderTexture,
    .stages = RHI::PipelineStage::FragmentShader,
    .accesses = RHI::ResourceAccess::TextureRead
};

class Resource : public Uncopyable
{
    int index_;

public:

    Resource(const RenderGraph *graph, int resourceIndex) : index_(resourceIndex), graph_(graph) { }

    virtual ~Resource() = default;

    int GetResourceIndex() const { return index_; }

protected:

    const RenderGraph *graph_;
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
};

class TextureResource : public Resource
{
public:

    using Resource::Resource;

    virtual const RHI::TextureDesc &GetDesc() const = 0;

    RHI::TextureDimension GetDimension() const { return GetDesc().dim; }
    uint32_t              GetWidth()     const { return GetDesc().width; }
    uint32_t              GetHeight()    const { return GetDesc().height; }
    uint32_t              GetDepth()     const { return GetDesc().depth; }
    uint32_t              GetMipLevels() const { return GetDesc().mipLevels; }
    uint32_t              GetArraySize() const { return GetDesc().arraySize; }

    RC<Texture> Get() const;
};

class PassContext : public Uncopyable
{
public:

    CommandBuffer &GetCommandBuffer();

    RC<Buffer> GetBuffer(const BufferResource *resource);
    RC<Texture> GetTexture2D(const TextureResource *resource);

private:

    friend class Executer;

    PassContext(const ExecutableResources &resources, CommandBuffer &commandBuffer);

    const ExecutableResources &resources_;
    CommandBuffer &commandBuffer_;
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

    using SubTexUsage = UnsynchronizedTextureAccess;
    using BufferUsage = UnsynchronizedBufferAccess;

    using TextureUsage = TextureSubresourceMap<std::optional<SubTexUsage>>;

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

    explicit RenderGraph(Queue queue = Queue(nullptr));

    void SetQueue(Queue queue);

    BufferResource  *CreateBuffer(const RHI::BufferDesc &desc);
    TextureResource *CreateTexture2D(const RHI::TextureDesc &desc);

    BufferResource  *RegisterBuffer(RC<Buffer> buffer);
    TextureResource *RegisterTexture(RC<Texture> texture);

    TextureResource *RegisterSwapchainTexture(
        RHI::TexturePtr             rhiTexture,
        RHI::BackBufferSemaphorePtr acquireSemaphore,
        RHI::BackBufferSemaphorePtr presentSemaphore);

    TextureResource *RegisterSwapchainTexture(const RHI::SwapchainPtr &swapchain);

    Pass *CreatePass(std::string name = {});

private:

    friend class Compiler;
    friend class Executer;
    friend class BufferResource;
    friend class TextureResource;

    class ExternalBufferResource : public BufferResource
    {
    public:

        using BufferResource::BufferResource;

        RC<Buffer> buffer;
    };

    class ExternalTextureResource : public TextureResource
    {
    public:

        using TextureResource::TextureResource;

        const RHI::TextureDesc &GetDesc() const override;

        RC<Texture> texture;
    };

    class InternalBufferResource : public BufferResource
    {
    public:

        using BufferResource::BufferResource;

        RHI::BufferDesc rhiDesc;
    };

    class InternalTextureResource : public TextureResource
    {
    public:

        using TextureResource::TextureResource;

        const RHI::TextureDesc &GetDesc() const override;

        RHI::TextureDesc rhiDesc;
    };

    class SwapchainTexture : public ExternalTextureResource
    {
    public:

        using ExternalTextureResource::ExternalTextureResource;

        RHI::BackBufferSemaphorePtr acquireSemaphore;
        RHI::BackBufferSemaphorePtr presentSemaphore;
    };

    Queue queue_;

    std::vector<Box<BufferResource>>  buffers_;
    std::vector<Box<TextureResource>> textures_;
    SwapchainTexture                 *swapchainTexture_;
    mutable ExecutableResources      *executableResource_;

    std::vector<Box<Pass>> passes_;
};

RTRC_RG_END
