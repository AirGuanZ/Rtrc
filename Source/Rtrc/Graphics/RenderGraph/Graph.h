#pragma once

#include <map>

#include <Rtrc/Core/SmartPointer/ObserverPtr.h>
#include <Rtrc/Graphics/Device/BindingGroupDSL.h>
#include <Rtrc/Graphics/Device/Buffer.h>
#include <Rtrc/Graphics/Device/Device.h>
#include <Rtrc/Graphics/Device/Queue.h>
#include <Rtrc/Graphics/Device/Texture.h>
#include <Rtrc/Graphics/RenderGraph/Label.h>
#include <Rtrc/Graphics/RenderGraph/Pass.h>

RTRC_RG_BEGIN

class Executer;
class RenderGraph;

RTRC_RG_END

RTRC_BEGIN

class Device;

using RenderGraph = RG::RenderGraph;
using RenderGraphExecutor = RG::Executer;

RTRC_END

RTRC_RG_BEGIN

struct ExecutableResources;

class RenderGraph;
class Executer;
class Compiler;

template<typename T>
concept RenderGraphBindingGroupInput = BindingGroupDSL::RtrcGroupStruct<T> ||
                                       std::is_same_v<T, RC<BindingGroup>>;

template<typename T>
concept RenderGraphStaticShader = requires { std::string(T::Name.GetString()); };

class RenderGraph : public Uncopyable
{
public:

    class InternalBufferResource : public BufferResource
    {
    public:

        using BufferResource::BufferResource;

        const RHI::BufferDesc& GetDesc() const override { return rhiDesc_; }

        void SetDefaultStructStride(size_t stride) override;
        void SetDefaultTexelFormat(RHI::Format format) override;

        size_t GetDefaultStructStride() const override;
        RHI::Format GetDefaultTexelFormat() const override;

    private:

        friend class RenderGraph;
        friend class Compiler;

        size_t          defaultStructStride_ = 0;
        RHI::Format     defaultTexelFormat_ = RHI::Format::Unknown;
        RHI::BufferDesc rhiDesc_;
        std::string     name_;
    };

    explicit RenderGraph(Ref<Device> device, Queue queue = Queue(nullptr));

    Ref<Device> GetDevice() const { return device_; }

    const Queue &GetQueue() const { return queue_; }
    void SetQueue(Queue queue) { queue_ = std::move(queue); }

    BufferResource  *CreateBuffer(const RHI::BufferDesc &desc, std::string name = {});
    TextureResource *CreateTexture(const RHI::TextureDesc &desc, std::string name = {});

    BufferResource *CreateTexelBuffer(
        size_t count, RHI::Format format, RHI::BufferUsageFlag usages, std::string name = {});
    BufferResource *CreateStructuredBuffer(
        size_t count, size_t stride, RHI::BufferUsageFlag usages, std::string name = {});

    BufferResource  *RegisterBuffer(RC<StatefulBuffer> buffer);
    TextureResource *RegisterTexture(RC<StatefulTexture> texture);
    TextureResource *RegisterReadOnlyTexture(RC<Texture> texture);
    TlasResource    *RegisterTlas(RC<Tlas> tlas, BufferResource *internalBuffer);

    TextureResource *RegisterSwapchainTexture(
        RHI::TextureRPtr             rhiTexture,
        RHI::BackBufferSemaphoreOPtr acquireSemaphore,
        RHI::BackBufferSemaphoreOPtr presentSemaphore);
    TextureResource *RegisterSwapchainTexture(RHI::SwapchainOPtr swapchain);

    void SetCompleteFence(RHI::FenceOPtr fence);

    void PushPassGroup(std::string name);
    void PopPassGroup();

    void BeginUAVOverlap();
    void EndUAVOverlap();

    Pass *CreatePass(std::string name);

    // ====================== Common functional passes ======================

    Pass *CreateClearTexture2DPass(
        std::string      name,
        TextureResource *tex2D,
        const Vector4f  &clearValue);
    Pass *CreateDummyPass(std::string name);

    Pass *CreateClearRWBufferPass(
        std::string     name,
        BufferResource *buffer,
        uint32_t        value);
    Pass *CreateClearRWStructuredBufferPass(
        std::string     name,
        BufferResource *buffer,
        uint32_t        value);

    Pass* CreateCopyBufferPass(
        std::string     name,
        BufferResource *src,
        size_t          srcOffset,
        BufferResource *dst,
        size_t          dstOffset,
        size_t          size);
    Pass *CreateCopyBufferPass(
        std::string     name,
        BufferResource *src,
        BufferResource *dst,
        size_t          size);
    Pass *CreateCopyColorTexturePass(
        std::string      name,
        TextureResource *src,
        TextureResource *dst);
    Pass *CreateCopyColorTexturePass(
        std::string                name,
        TextureResource           *src,
        TextureResource           *dst,
        const TextureSubresources &subrscs);
    Pass *CreateCopyColorTexturePass(
        std::string                name,
        TextureResource           *src,
        const TextureSubresources &srcSubrscs,
        TextureResource           *dst,
        const TextureSubresources &dstSubrscs);

    Pass *CreateClearRWTexture2DPass(std::string name, TextureResource *tex2D, const Vector4f &value);
    Pass *CreateClearRWTexture2DPass(std::string name, TextureResource *tex2D, const Vector4u &value);
    Pass *CreateClearRWTexture2DPass(std::string name, TextureResource *tex2D, const Vector4i &value);

    Pass *CreateBlitTexture2DPass(
        std::string      name,
        TextureResource *src,
        uint32_t         srcArrayLayer,
        uint32_t         srcMipLevel,
        TextureResource *dst,
        uint32_t         dstArrayLayer,
        uint32_t         dstMipLevel,
        bool             usePointSampling = true,
        float            gamma = 1.0f);
    Pass *CreateBlitTexture2DPass(
        std::string      name,
        TextureResource *src,
        TextureResource *dst,
        bool             usePointSampling = true,
        float            gamma = 1.0f);
    
#define DECLARE_CREATE_COMPUTE_PASS_WITH_THREAD_COUNT(...)                 \
    template<RenderGraphBindingGroupInput...Ts>                            \
    Pass *CreateComputePass(                                               \
        std::string     name,                                              \
        RC<Shader>      shader,                                            \
        __VA_ARGS__,                                                       \
        const Ts&...    bindingGroups);                                    \
    template<RenderGraphBindingGroupInput...Ts>                            \
    Pass *CreateComputePassWithThreadCount(                                \
        std::string     name,                                              \
        RC<Shader>      shader,                                            \
        __VA_ARGS__,                                                       \
        const Ts&...    bindingGroups);                                    \
    template<RenderGraphStaticShader S, RenderGraphBindingGroupInput...Ts> \
    Pass *CreateComputePass(                                               \
        __VA_ARGS__,                                                       \
        const Ts&...bindingGroups);                                        \
    template<RenderGraphStaticShader S, RenderGraphBindingGroupInput...Ts> \
    Pass *CreateComputePassWithThreadCount(                                \
        __VA_ARGS__,                                                       \
        const Ts&...bindingGroups);

    DECLARE_CREATE_COMPUTE_PASS_WITH_THREAD_COUNT(const Vector3i &threadCount)
    DECLARE_CREATE_COMPUTE_PASS_WITH_THREAD_COUNT(const Vector3u &threadCount)
    DECLARE_CREATE_COMPUTE_PASS_WITH_THREAD_COUNT(const Vector2i &threadCount)
    DECLARE_CREATE_COMPUTE_PASS_WITH_THREAD_COUNT(const Vector2u &threadCount)
    DECLARE_CREATE_COMPUTE_PASS_WITH_THREAD_COUNT(unsigned int threadCountX)
    DECLARE_CREATE_COMPUTE_PASS_WITH_THREAD_COUNT(unsigned int threadCountX, unsigned int threadCountY)
    DECLARE_CREATE_COMPUTE_PASS_WITH_THREAD_COUNT(unsigned int threadCountX, unsigned int threadCountY, unsigned int threadCountZ)

#undef DECLARE_CREATE_COMPUTE_PASS_WITH_THREAD_COUNT

    template<RenderGraphBindingGroupInput...Ts>
    Pass *CreateIndirectComputePass(
        std::string     name,
        RC<Shader>      shader,
        BufferResource *indirectBuffer,
        size_t          indirectBufferOffset,
        const Ts&...    bindnigGroups);

    template<RenderGraphStaticShader S, RenderGraphBindingGroupInput...Ts>
    Pass *CreateIndirectComputePass(
        BufferResource *indirectBuffer,
        size_t          indirectBufferOffset,
        const Ts&...    bindnigGroups);

private:

    friend class Compiler;
    friend class Executer;
    friend class BufferResource;
    friend class TextureResource;

    class ExternalBufferResource : public BufferResource
    {
    public:

        using BufferResource::BufferResource;

        const RHI::BufferDesc& GetDesc() const override { return buffer->GetDesc(); }

        size_t GetDefaultStructStride() const override;
        RHI::Format GetDefaultTexelFormat() const override;

        void SetDefaultTexelFormat(RHI::Format format) override;
        void SetDefaultStructStride(size_t stride) override;

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

        RHI::BackBufferSemaphoreOPtr acquireSemaphore;
        RHI::BackBufferSemaphoreOPtr presentSemaphore;
    };

    Ref<Device> device_;
    Queue       queue_;

    LabelStack labelStack_;
    
    std::map<const void *, int>       externalResourceMap_; // RHI object pointer to resource index
    std::vector<Box<BufferResource>>  buffers_;
    std::vector<Box<TextureResource>> textures_;
    SwapchainTexture                 *swapchainTexture_;
    mutable ExecutableResources      *executableResource_;

    std::map<const BufferResource *, Box<TlasResource>> tlasResources_;

    std::vector<Box<Pass>> passes_;

    RHI::FenceOPtr  completeFence_;

    size_t currentUAVOverlapGroupDepth_ = 0;
    uint32_t nextUAVOverlapGroupIndex_ = 0;
    UAVOverlapGroup currentUAVOverlapGroup_;
};

#define RTRC_RG_SCOPED_PASS_GROUP(RENDERGRAPH, NAME)       \
    ::Rtrc::Ref(RENDERGRAPH)->PushPassGroup(NAME); \
    RTRC_SCOPE_EXIT{ ::Rtrc::Ref(RENDERGRAPH)->PopPassGroup(); }

template<RenderGraphBindingGroupInput ... Ts>
Pass *RenderGraph::CreateComputePass(
    std::string     name,
    RC<Shader>      shader,
    const Vector3i &threadGroupCount,
    const Ts &...   bindingGroups)
{
    auto pass = CreatePass(std::move(name));
    auto DeclareUse = [&]<RenderGraphBindingGroupInput T>(const T &group)
    {
        if constexpr(BindingGroupDSL::RtrcGroupStruct<T>)
        {
            DeclareRenderGraphResourceUses(pass, group, RHI::PipelineStageFlag::ComputeShader);
        }
    };
    (DeclareUse(bindingGroups), ...);
    pass->SetCallback([d = device_, s = std::move(shader), threadGroupCount, ...bindingGroups = bindingGroups]
    {
        std::vector<RC<BindingGroup>> finalGroups;
        auto BindGroup = [&]<RenderGraphBindingGroupInput T>(const T &group)
        {
            if constexpr(BindingGroupDSL::RtrcGroupStruct<T>)
            {
                finalGroups.push_back(d->CreateBindingGroupWithCachedLayout(group));
            }
            else
            {
                finalGroups.push_back(group);
            }
        };
        (BindGroup(bindingGroups), ...);

        auto &commandBuffer = GetCurrentCommandBuffer();
        commandBuffer.BindComputePipeline(s->GetComputePipeline());
        commandBuffer.BindComputeGroups(finalGroups);
        commandBuffer.Dispatch(threadGroupCount);
    });
    return pass;
}

template<RenderGraphBindingGroupInput ... Ts>
Pass *RenderGraph::CreateComputePassWithThreadCount(
    std::string     name,
    RC<Shader>      shader,
    const Vector3i &threadCount,
    const Ts &...   bindingGroups)
{
    const Vector3i groupCount = shader->ComputeThreadGroupCount(threadCount);
    return this->CreateComputePass(std::move(name), std::move(shader), groupCount, bindingGroups...);
}

template<RenderGraphStaticShader S, RenderGraphBindingGroupInput ... Ts>
Pass *RenderGraph::CreateComputePass(const Vector3i &threadGroupCount, const Ts &... bindingGroups)
{
    return this->CreateComputePass(S::Name, device_->GetShader<S::Name>(), threadGroupCount, bindingGroups...);
}

template<RenderGraphStaticShader S, RenderGraphBindingGroupInput ... Ts>
Pass *RenderGraph::CreateComputePassWithThreadCount(const Vector3i &threadCount, const Ts &... bindingGroups)
{
    return this->CreateComputePassWithThreadCount(
        S::Name, device_->GetShader<S::Name>(), threadCount, bindingGroups...);
}

#define DEFINE_CREATE_COMPUTE_PASS_WITH_THREAD_COUNT(EXPR, ...)                                                \
template<RenderGraphBindingGroupInput ... Ts>                                                                  \
Pass *RenderGraph::CreateComputePass(                                                                          \
    std::string     name,                                                                                      \
    RC<Shader>      shader,                                                                                    \
    __VA_ARGS__,                                                                                               \
    const Ts &...   bindingGroups)                                                                             \
{                                                                                                              \
    return this->CreateComputePass(std::move(name), std::move(shader), EXPR, bindingGroups...);                \
}                                                                                                              \
template<RenderGraphBindingGroupInput ... Ts>                                                                  \
Pass *RenderGraph::CreateComputePassWithThreadCount(                                                           \
    std::string     name,                                                                                      \
    RC<Shader>      shader,                                                                                    \
    __VA_ARGS__,                                                                                               \
    const Ts &...   bindingGroups)                                                                             \
{                                                                                                              \
    return this->CreateComputePassWithThreadCount(std::move(name), std::move(shader), EXPR, bindingGroups...); \
}                                                                                                              \
template<RenderGraphStaticShader S, RenderGraphBindingGroupInput...Ts>                                         \
Pass *RenderGraph::CreateComputePass(                                                                          \
    __VA_ARGS__,                                                                                               \
    const Ts&...bindingGroups)                                                                                 \
{                                                                                                              \
    return this->CreateComputePass<S>(EXPR, bindingGroups...);                                                 \
}                                                                                                              \
template<RenderGraphStaticShader S, RenderGraphBindingGroupInput...Ts>                                         \
Pass *RenderGraph::CreateComputePassWithThreadCount(                                                           \
    __VA_ARGS__,                                                                                               \
    const Ts&...bindingGroups)                                                                                 \
{                                                                                                              \
    return this->CreateComputePassWithThreadCount<S>(EXPR, bindingGroups...);                                  \
}

DEFINE_CREATE_COMPUTE_PASS_WITH_THREAD_COUNT(Vector3i(t.x, t.y, t.z), const Vector3u &t)
DEFINE_CREATE_COMPUTE_PASS_WITH_THREAD_COUNT(Vector3i(t.x, t.y, 1), const Vector2i &t)
DEFINE_CREATE_COMPUTE_PASS_WITH_THREAD_COUNT(Vector3i(t.x, t.y, 1), const Vector2u &t)
DEFINE_CREATE_COMPUTE_PASS_WITH_THREAD_COUNT(Vector3i(x, 1, 1), unsigned int x)
DEFINE_CREATE_COMPUTE_PASS_WITH_THREAD_COUNT(Vector3i(x, y, 1), unsigned int x, unsigned int y)
DEFINE_CREATE_COMPUTE_PASS_WITH_THREAD_COUNT(Vector3i(x, y, z), unsigned int x, unsigned int y, unsigned int z)

#undef DEFINE_CREATE_COMPUTE_PASS_WITH_THREAD_COUNT

template<RenderGraphBindingGroupInput ... Ts>
Pass *RenderGraph::CreateIndirectComputePass(
    std::string     name,
    RC<Shader>      shader,
    BufferResource *indirectBuffer,
    size_t          indirectBufferOffset,
    const Ts &...   bindingGroups)
{
    auto pass = CreatePass(std::move(name));
    auto DeclareUse = [&]<RenderGraphBindingGroupInput T>(const T & group)
    {
        if constexpr(BindingGroupDSL::RtrcGroupStruct<T>)
        {
            DeclareRenderGraphResourceUses(pass, group, RHI::PipelineStageFlag::ComputeShader);
        }
    };
    (DeclareUse(bindingGroups), ...);
    pass->Use(indirectBuffer, IndirectDispatchRead);
    pass->SetCallback(
        [d = device_, s = std::move(shader), indirectBuffer, indirectBufferOffset, ...bindingGroups = bindingGroups]
    {
        std::vector<RC<BindingGroup>> finalGroups;
        auto BindGroup = [&]<RenderGraphBindingGroupInput T>(const T &group)
        {
            if constexpr(BindingGroupDSL::RtrcGroupStruct<T>)
            {
                finalGroups.push_back(d->CreateBindingGroupWithCachedLayout(group));
            }
            else
            {
                finalGroups.push_back(group);
            }
        };
        (BindGroup(bindingGroups), ...);

        auto &commandBuffer = GetCurrentCommandBuffer();
        commandBuffer.BindComputePipeline(s->GetComputePipeline());
        commandBuffer.BindComputeGroups(finalGroups);
        commandBuffer.DispatchIndirect(indirectBuffer->Get(), indirectBufferOffset);
    });
    return pass;
}

template<RenderGraphStaticShader S, RenderGraphBindingGroupInput ... Ts>
Pass *RenderGraph::CreateIndirectComputePass(
    BufferResource *indirectBuffer,
    size_t          indirectBufferOffset,
    const Ts &...   bindnigGroups)
{
    return this->CreateIndirectComputePass(
        S::Name, device_->GetShader<S::Name>(), indirectBuffer, indirectBufferOffset, bindnigGroups...);
}

RTRC_RG_END
