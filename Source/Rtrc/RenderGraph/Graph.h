#pragma once

#include <map>

#include <Rtrc/RenderGraph/PersistentResource.h>

RTRC_RG_BEGIN

class Resource
{
public:

    virtual ~Resource() = default;
};

class BufferResource : public Resource
{
    
};

class TextureResource : public Resource
{
    
};

class PassContext
{
public:

    const RHI::CommandBufferPtr &GetRHICommandBuffer();
    const RHI::BufferPtr        &GetRHIBuffer(const BufferResource *resource);
    const RHI::TexturePtr       &GetRHITexture(const TextureResource *resource);

    int GetSubPassCount() const;
    int GetSubPassIndex() const;

private:

    RHI::CommandBufferPtr commandBuffer_;
};

class Pass
{
public:

    using Callback = std::function<void(PassContext &)>;

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

    void SetSubPassCount(int count);

    void SetCallback(Callback callback);

private:

    struct TextureSubresourceUsage
    {
        RHI::TextureLayout      layout;
        RHI::PipelineStageFlag  stages;
        RHI::ResourceAccessFlag accesses;
    };

    struct BufferUsage
    {
        RHI::PipelineStageFlag  stages;
        RHI::ResourceAccessFlag accesses;
    };

    using TextureUsage = TextureSubresourceMap<std::optional<TextureSubresourceUsage>>;

    friend class RenderGraph;

    std::string   name_;
    RHI::QueuePtr queue_;
    Callback      callback_;

    std::map<BufferResource *, BufferUsage>   bufferUsages_;
    std::map<TextureResource *, TextureUsage> textureUsages_;

    std::set<Pass *> prevs_;
    std::set<Pass *> succs_;
};

class RenderGraph : public Uncopyable
{
public:

    RenderGraph();

    BufferResource  *RegisterBuffer(RC<PersistentBuffer> buffer);
    TextureResource *RegisterTexture(RC<PersistentTexture> texture);

    BufferResource  *CreateBuffer(const RHI::BufferDesc &desc);
    TextureResource *CreateTexture2D(const RHI::Texture2DDesc &desc);

    TextureResource *RegisterSwapchainTexture(
        RC<PersistentTexture>       texture,
        RHI::BackBufferSemaphorePtr acquireSemaphore,
        RHI::BackBufferSemaphorePtr presentSemaphore);

    Pass *CreatePass(std::string name, RHI::QueuePtr queue);
    
    RenderGraph &AddDependency(Pass *head, Pass *tail);

    void Execute(RHI::DevicePtr device);

private:

    class ExternalBufferResource : public BufferResource
    {
    public:

        RC<PersistentBuffer> buffer;
    };

    class ExternalTextureResource : public TextureResource
    {
    public:

        RC<PersistentTexture> texture;
    };

    class InternalBufferResource : public BufferResource
    {
    public:

        RHI::BufferDesc rhiDesc;
    };

    class InternalTextureResource : public TextureResource
    {
    public:

        RHI::Texture2DDesc rhiDesc;
    };

    class SwapchainTexture : public TextureResource
    {
        RC<PersistentTexture>       rhiTexture;
        RHI::BackBufferSemaphorePtr acquireSemaphore;
        RHI::BackBufferSemaphorePtr presentSemaphore;
    };

    std::vector<Box<ExternalBufferResource>>  externalBuffers_;
    std::vector<Box<ExternalTextureResource>> externalTextures_;
    std::vector<Box<InternalBufferResource>>  internalBuffers_;
    std::vector<Box<InternalTextureResource>> internalTextures_;
    Box<SwapchainTexture>                     swapchainTexture_;

    std::vector<Box<Pass>> passes_;
};

RTRC_RG_END
