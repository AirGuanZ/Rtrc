#pragma once

#include <map>

#include <Rtrc/RHI/Helper/StatefulResource.h>

RTRC_RG_BEGIN

struct ExecutableResources;

class RenderGraph;
class Executer;
class Pass;
class Compiler;

class Resource : public Uncopyable
{
    int index_;

public:

    explicit Resource(int resourceIndex) : index_(resourceIndex) { }

    virtual ~Resource() = default;

    int GetResourceIndex() const { return index_; }
};

class BufferResource : public Resource
{
public:

    using Resource::Resource;
};

class TextureResource : public Resource
{
public:

    using Resource::Resource;

    virtual uint32_t GetMipLevels() const = 0;
    virtual uint32_t GetArraySize() const = 0;
};

class PassContext : public Uncopyable
{
public:

    const RHI::CommandBufferPtr &GetRHICommandBuffer();
    RHI::BufferPtr GetRHIBuffer(const BufferResource *resource);
    RHI::TexturePtr GetRHITexture(const TextureResource *resource);

private:

    friend class Executer;

    PassContext(const ExecutableResources &resources, RHI::CommandBufferPtr commandBuffer);

    const ExecutableResources &resources_;
    RHI::CommandBufferPtr commandBuffer_;
};

void Connect(Pass *head, Pass *tail);

class Pass
{
public:

    using Callback = std::function<void(PassContext &)>;

    friend void Connect(Pass *head, Pass *tail);

    void Use(
        BufferResource         *buffer,
        RHI::PipelineStageFlag  stages,
        RHI::ResourceAccessFlag accesses);

    void Use(
        TextureResource        *texture,
        RHI::TextureLayout      layout,
        RHI::PipelineStageFlag  stages,
        RHI::ResourceAccessFlag accesses);

    void Use(
        TextureResource               *texture,
        const RHI::TextureSubresource &subresource,
        RHI::TextureLayout             layout,
        RHI::PipelineStageFlag         stages,
        RHI::ResourceAccessFlag        accesses);

    void SetCallback(Callback callback);

    void SetSignalFence(RHI::FencePtr fence);

private:

    using SubTexUsage = RHI::StatefulTexture::State;
    using BufferUsage = RHI::StatefulBuffer::State;

    using TextureUsage = RHI::TextureSubresourceMap<std::optional<SubTexUsage>>;

    friend class RenderGraph;
    friend class Compiler;

    Pass(int index, std::string name, RHI::QueuePtr queue);

    int           index_;
    std::string   name_;
    RHI::QueuePtr queue_;
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

    RenderGraph();

    BufferResource  *CreateBuffer(const RHI::BufferDesc &desc);
    TextureResource *CreateTexture2D(const RHI::Texture2DDesc &desc);

    BufferResource  *RegisterBuffer(RC<RHI::StatefulBuffer> buffer);
    TextureResource *RegisterTexture(RC<RHI::StatefulTexture> texture);

    TextureResource *RegisterSwapchainTexture(
        RC<RHI::StatefulTexture>    texture,
        RHI::BackBufferSemaphorePtr acquireSemaphore,
        RHI::BackBufferSemaphorePtr presentSemaphore);

    Pass *CreatePass(std::string name, RHI::QueuePtr queue);

private:

    friend class Compiler;

    class ExternalBufferResource : public BufferResource
    {
    public:

        using BufferResource::BufferResource;

        RC<RHI::StatefulBuffer> buffer;
    };

    class ExternalTextureResource : public TextureResource
    {
    public:

        using TextureResource::TextureResource;

        uint32_t GetMipLevels() const override;
        uint32_t GetArraySize() const override;

        RC<RHI::StatefulTexture> texture;
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
    };

    class InternalTexture2DResource : public InternalTextureResource
    {
    public:

        using InternalTextureResource::InternalTextureResource;

        uint32_t GetMipLevels() const override;
        uint32_t GetArraySize() const override;

        RHI::Texture2DDesc rhiDesc;
    };

    class SwapchainTexture : public ExternalTextureResource
    {
    public:

        using ExternalTextureResource::ExternalTextureResource;

        RHI::BackBufferSemaphorePtr acquireSemaphore;
        RHI::BackBufferSemaphorePtr presentSemaphore;
    };

    std::vector<Box<BufferResource>>  buffers_;
    std::vector<Box<TextureResource>> textures_;
    SwapchainTexture                 *swapchainTexture_;

    std::vector<Box<Pass>> passes_;
};

RTRC_RG_END
