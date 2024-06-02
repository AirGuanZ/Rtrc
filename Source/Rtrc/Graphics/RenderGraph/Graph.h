#pragma once

#include <map>

#include <Rtrc/Core/SmartPointer/ObserverPtr.h>
#include <Rtrc/Graphics/Device/Buffer.h>
#include <Rtrc/Graphics/Device/Device.h>
#include <Rtrc/Graphics/Device/Queue.h>
#include <Rtrc/Graphics/Device/Texture.h>
#include <Rtrc/Graphics/RenderGraph/Label.h>
#include <Rtrc/Graphics/RenderGraph/Pass.h>
#include <Rtrc/Graphics/Shader/DSL/BindingGroup.h>

RTRC_BEGIN

class Device;
struct RGExecutableResources;
class RenderGraph;
class RGExecuter;
class RGCompiler;

template<typename T>
concept RGBindingGroupInput = RtrcGroupStruct<T> || std::is_same_v<T, RC<BindingGroup>>;

template<typename T>
concept RGStaticShader = requires { std::string(T::Name.GetString()); };

class RenderGraph : public Uncopyable
{
public:

    explicit RenderGraph(Ref<Device> device, Queue queue = Queue(nullptr));

    Ref<Device> GetDevice() const { return device_; }

    const Queue &GetQueue() const { return queue_; }
    void SetQueue(Queue queue) { assert(recording_); queue_ = std::move(queue); }

    RGBuffer CreateBuffer(const RHI::BufferDesc &desc, std::string name = {});
    RGTexture CreateTexture(const RHI::TextureDesc &desc, std::string name = {});

    RGBuffer CreateTexelBuffer(size_t count, RHI::Format format, RHI::BufferUsageFlag usages, std::string name = {});
    RGBuffer CreateStructuredBuffer(size_t count, size_t stride, RHI::BufferUsageFlag usages, std::string name = {});

    RGBuffer  RegisterBuffer(RC<StatefulBuffer> buffer);
    RGTexture RegisterTexture(RC<StatefulTexture> texture);
    RGBuffer  RegisterReadOnlyBuffer(RC<Buffer> buffer);
    RGTexture RegisterReadOnlyTexture(RC<Texture> texture);
    RGTlas    RegisterTlas(RC<Tlas> tlas, RGBufImpl *internalBuffer);

    // This code assumes that the swapchain texture is not accessed externally prior to this graph.
    // As a result, the swapchain texture can only be registered in a single render graph per frame.
    RGTexture RegisterSwapchainTexture(RHI::SwapchainOPtr swapchain);
    RGTexture RegisterSwapchainTexture(
        RHI::TextureRPtr             rhiTexture,
        RHI::BackBufferSemaphoreOPtr acquireSemaphore,
        RHI::BackBufferSemaphoreOPtr presentSemaphore);

    void SetCompleteFence(RHI::FenceOPtr fence);

    void PushPassGroup(std::string name);
    void PopPassGroup();

    void BeginUAVOverlap();
    void EndUAVOverlap();

    RGPass CreatePass(std::string name);

private:

    friend class RGCompiler;
    friend class RGExecuter;
    friend class RGBufImpl;
    friend class RGTexImpl;

    class ExternalBufferResource : public RGBufImpl
    {
    public:

        using RGBufImpl::RGBufImpl;

        const RHI::BufferDesc& GetDesc() const override { return buffer->GetDesc(); }

        size_t GetDefaultStructStride() const override;
        RHI::Format GetDefaultTexelFormat() const override;

        void SetDefaultTexelFormat(RHI::Format format) override;
        void SetDefaultStructStride(size_t stride) override;

        bool isReadOnlyBuffer = false;
        RC<StatefulBuffer> buffer;
    };

    class ExternalTextureResource : public RGTexImpl
    {
    public:

        using RGTexImpl::RGTexImpl;

        bool SkipDeclarationCheck() const override { return isReadOnlySampledTexture; }

        const RHI::TextureDesc &GetDesc() const override;

        bool isReadOnlySampledTexture = false;
        RC<StatefulTexture> texture;
    };

    class InternalTextureResource : public RGTexImpl
    {
    public:

        using RGTexImpl::RGTexImpl;

        const RHI::TextureDesc &GetDesc() const override;

        RHI::TextureDesc    rhiDesc;
        std::string         name;
        RC<StatefulTexture> crossExecutionResource; // Internal resource must preserve its content across executions.
                                                    // See comments of RGExecuter::ExecutePartially.
    };

    class InternalBufferResource : public RGBufImpl
    {
    public:

        using RGBufImpl::RGBufImpl;

        const RHI::BufferDesc& GetDesc() const override { return rhiDesc; }

        void SetDefaultStructStride(size_t stride) override;
        void SetDefaultTexelFormat(RHI::Format format) override;

        size_t GetDefaultStructStride() const override;
        RHI::Format GetDefaultTexelFormat() const override;

        size_t             defaultStructStride = 0;
        RHI::Format        defaultTexelFormat = RHI::Format::Unknown;
        RHI::BufferDesc    rhiDesc;
        std::string        name;
        RC<StatefulBuffer> crossExecutionResource;
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
    bool        recording_;

    RGLabelStack labelStack_;
    
    std::map<const void *, int>       externalResourceMap_; // RHI object pointer to resource index
    std::vector<Box<RGBufImpl>>  buffers_;
    std::vector<Box<RGTexImpl>> textures_;
    SwapchainTexture                 *swapchainTexture_;
    mutable RGExecutableResources      *executableResource_;

    std::map<const RGBufImpl *, Box<RGTlasImpl>> tlasResources_;

    std::vector<Box<RGPassImpl>> passes_;
    std::vector<Box<RGPassImpl>> executedPasses_;

    RHI::FenceOPtr completeFence_;

    size_t            currentUAVOverlapGroupDepth_ = 0;
    uint32_t          nextUAVOverlapGroupIndex_ = 0;
    RGUavOverlapGroup currentUAVOverlapGroup_;
};

#define RTRC_RG_SCOPED_PASS_GROUP(RENDERGRAPH, NAME) \
    ::Rtrc::Ref(RENDERGRAPH)->PushPassGroup(NAME);   \
    RTRC_SCOPE_EXIT{ ::Rtrc::Ref(RENDERGRAPH)->PopPassGroup(); }

RTRC_END

#include <Rtrc/Graphics/RenderGraph/GraphUtility.inl>
