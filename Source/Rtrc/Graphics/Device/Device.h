#pragma once

#include <Rtrc/Graphics/Device/AccelerationStructure.h>
#include <Rtrc/Graphics/Device/BindingGroup.h>
#include <Rtrc/Graphics/Device/BindingGroupDSL.h>
#include <Rtrc/Graphics/Device/Buffer.h>
#include <Rtrc/Graphics/Device/CopyContext.h>
#include <Rtrc/Graphics/Device/Queue.h>
#include <Rtrc/Graphics/Device/Pipeline.h>
#include <Rtrc/Graphics/Device/Sampler.h>
#include <Rtrc/Graphics/Device/Texture.h>
#include <Rtrc/Graphics/RenderGraph/Graph.h>

RTRC_BEGIN

namespace DeviceDetail
{

    enum class FlagBit : uint32_t
    {
        EnableRayTracing   = 1 << 0,
        EnableSwapchainUav = 1 << 1,
    };

    RTRC_DEFINE_ENUM_FLAGS(FlagBit)

    using Flags = EnumFlags<FlagBit>;

} // namespace DeviceDetail

class Device : public Uncopyable
{
public:

    using Flags = DeviceDetail::Flags;

    using enum DeviceDetail::FlagBit;

    // Creation & destructor

    static Box<Device> CreateComputeDevice(RHI::DevicePtr rhiDevice);
    static Box<Device> CreateComputeDevice(bool debugMode = RTRC_DEBUG);

    static Box<Device> CreateGraphicsDevice(
        RHI::DevicePtr rhiDevice,
        Window        &window,
        RHI::Format    swapchainFormat     = RHI::Format::B8G8R8A8_UNorm,
        int            swapchainImageCount = 3,
        bool           vsync               = false,
        Flags          flags               = {});
    static Box<Device> CreateGraphicsDevice(
        Window     &window,
        RHI::Format swapchainFormat     = RHI::Format::B8G8R8A8_UNorm,
        int         swapchainImageCount = 3,
        bool        debugMode           = RTRC_DEBUG,
        bool        vsync               = false,
        Flags       flags               = {});

    ~Device();

    // Query

    const RHI::ShaderGroupRecordRequirements &GetShaderGroupRecordRequirements() const;

    // Context objects

    Queue &GetQueue();
    DeviceSynchronizer &GetSynchronizer();

    const RHI::SwapchainPtr &GetSwapchain() const;
    const RHI::TextureDesc &GetSwapchainImageDesc() const;

    // Frame synchronization

    void WaitIdle();

    void BeginRenderLoop();
    void EndRenderLoop();
    void Present();

    bool BeginFrame();
    const RHI::FencePtr &GetFrameFence();

    // Resource creation & upload

    RC<Buffer>         CreateBuffer(const RHI::BufferDesc &desc);
    RC<StatefulBuffer> CreatePooledBuffer(const RHI::BufferDesc &desc);

    RC<Texture>         CreateTexture(const RHI::TextureDesc &desc);
    RC<StatefulTexture> CreatePooledTexture(const RHI::TextureDesc &desc);
    RC<Texture>         CreateColorTexture2D(uint8_t r, uint8_t g, uint8_t b, uint8_t a, const std::string &name = {});

    RC<DynamicBuffer> CreateDynamicBuffer();
    RC<SubBuffer>     CreateConstantBuffer(const void *data, size_t bytes);
    template<RtrcStruct T>
    RC<SubBuffer>     CreateConstantBuffer(const T &data);
    
    void UploadBuffer(
        const RC<Buffer> &buffer,
        const void       *initData,
        size_t            offset = 0,
        size_t            size   = 0);
    void UploadTexture2D(
        const RC<Texture> &texture,
        uint32_t           arrayLayer,
        uint32_t           mipLevel,
        const void        *data,
        RHI::TextureLayout postLayout = RHI::TextureLayout::CopyDst);
    void UploadTexture2D(
        const RC<Texture> &texture,
        uint32_t             arrayLayer,
        uint32_t             mipLevel,
        const ImageDynamic  &image,
        RHI::TextureLayout   postLayout = RHI::TextureLayout::CopyDst);

    RC<Buffer> CreateAndUploadBuffer(
        const RHI::BufferDesc &desc,
        const void            *initData,
        size_t                 initDataOffset = 0,
        size_t                 initDataSize = 0);
    RC<Texture> CreateAndUploadTexture2D(
        const RHI::TextureDesc &desc,
        Span<const void *>      imageData,
        RHI::TextureLayout      postLayout = RHI::TextureLayout::CopyDst);
    RC<Texture> CreateAndUploadTexture2D(
        const RHI::TextureDesc &desc,
        Span<ImageDynamic>      images,
        RHI::TextureLayout      postLayout = RHI::TextureLayout::CopyDst);
    RC<Texture> LoadTexture2D(
        const std::string    &filename,
        RHI::Format           format,
        RHI::TextureUsageFlag usages,
        bool                  generateMipLevels,
        RHI::TextureLayout    postLayout = RHI::TextureLayout::CopyDst);

    // Pipeline object creation

    Box<RG::RenderGraph> CreateRenderGraph();

    template<BindingGroupDSL::RtrcGroupStruct T>
    RC<BindingGroupLayout> CreateBindingGroupLayout();
    RC<BindingGroupLayout> CreateBindingGroupLayout(const BindingGroupLayout::Desc &desc);

    template<BindingGroupDSL::RtrcGroupStruct T>
    RC<BindingGroup> CreateBindingGroup(int variableBindingCount = 0);
    template<BindingGroupDSL::RtrcGroupStruct T>
    RC<BindingGroup> CreateBindingGroup(const RC<BindingGroupLayout> &layoutHint, int variableBindingCount = 0);
    template<BindingGroupDSL::RtrcGroupStruct T>
    RC<BindingGroup> CreateBindingGroup(const T &value, int variableBindingCount = 0);
    template<BindingGroupDSL::RtrcGroupStruct T>
    RC<BindingGroup> CreateBindingGroup(const T &value, const RC<BindingGroupLayout> &layoutHint, int variableBindingCount = 0);

    RC<BindingLayout> CreateBindingLayout(const BindingLayout::Desc &desc);

    CommandBuffer CreateCommandBuffer();

    RC<GraphicsPipeline> CreateGraphicsPipeline(const GraphicsPipeline::Desc &desc);
    RC<ComputePipeline> CreateComputePipeline(const RC<Shader> &shader);

    RC<RayTracingLibrary> CreateRayTracingLibrary(const RayTracingLibrary::Desc &desc);
    RC<RayTracingPipeline> CreateRayTracingPipeline(const RayTracingPipeline::Desc &desc);

    RC<Sampler> CreateSampler(const RHI::SamplerDesc &desc);

    // Acceleration structure
    
    BlasPrebuildInfo CreateBlasPrebuildinfo(
        Span<RHI::RayTracingGeometryDesc>             geometries,
        RHI::RayTracingAccelerationStructureBuildFlag flags);
    TlasPrebuildInfo CreateTlasPrebuildInfo(
        Span<RHI::RayTracingInstanceArrayDesc>        instanceArrays,
        RHI::RayTracingAccelerationStructureBuildFlag flags);

    RC<Blas> CreateBlas(RC<SubBuffer> buffer = nullptr);
    RC<Tlas> CreateTlas(RC<SubBuffer> buffer = nullptr);

    // Immediate execution

    void ExecuteAndWait(CommandBuffer commandBuffer);
    template<typename F> requires !std::is_same_v<std::remove_cvref_t<F>, CommandBuffer>
    void ExecuteAndWait(F &&f);

    void ExecuteBarrier(
        const RC<StatefulBuffer> &buffer,
        RHI::PipelineStageFlag    stages,
        RHI::ResourceAccessFlag   accesses);
    void ExecuteBarrier(
        const RC<StatefulTexture> &texture,
        RHI::TextureLayout         layout,
        RHI::PipelineStageFlag     stages,
        RHI::ResourceAccessFlag    accesses);
    void ExecuteBarrier(
        const RC<StatefulTexture> &texture,
        uint32_t                   arrayLayer,
        uint32_t                   mipLevel,
        RHI::TextureLayout         layout,
        RHI::PipelineStageFlag     stages,
        RHI::ResourceAccessFlag    accesses);
    void ExecuteBarrier(
        const RC<Buffer>       &buffer,
        RHI::PipelineStageFlag  prevStages,
        RHI::ResourceAccessFlag prevAccesses,
        RHI::PipelineStageFlag  succStages,
        RHI::ResourceAccessFlag succAccesses);
    void ExecuteBarrier(
        const RC<Texture>      &texture,
        RHI::TextureLayout      prevLayout,
        RHI::PipelineStageFlag  prevStages,
        RHI::ResourceAccessFlag prevAccesses,
        RHI::TextureLayout      succLayout,
        RHI::PipelineStageFlag  succStages,
        RHI::ResourceAccessFlag succAccesses);
    void ExecuteBarrier(
        const RC<Texture>      &texture,
        uint32_t                arrayLayer,
        uint32_t                mipLevel,
        RHI::TextureLayout      prevLayout,
        RHI::PipelineStageFlag  prevStages,
        RHI::ResourceAccessFlag prevAccesses,
        RHI::TextureLayout      succLayout,
        RHI::PipelineStageFlag  succStages,
        RHI::ResourceAccessFlag succAccesses);
    void ExecuteBarrier(
        const RC<Texture> &texture,
        RHI::TextureLayout prevLayout,
        RHI::TextureLayout succLayout);
    void ExecuteBarrier(
        const RC<StatefulTexture> &texture,
        RHI::TextureLayout         prevLayout,
        RHI::TextureLayout         succLayout);

    // Internal helper classes

    CopyContext          &GetCopyContext();
    BufferManager        &GetBufferManager();
    BindingGroupManager  &GetBindingGroupManager();
    const RHI::DevicePtr &GetRawDevice() const;

    operator DynamicBufferManager &();

private:

    Device() = default;
    
    void InitializeInternal(RHI::DevicePtr device, bool isComputeOnly);

    void RecreateSwapchain();

    template<typename...Args>
    void ExecuteBarrierImpl(Args&&...args);

    RHI::InstancePtr instance_;
    RHI::DevicePtr device_;
    Queue mainQueue_;

    Window           *window_              = nullptr;
    RHI::Format       swapchainFormat_     = RHI::Format::Unknown;
    int               swapchainImageCount_ = 0;
    bool              vsync_               = true;
    bool              swapchainUav_        = false;
    RHI::SwapchainPtr swapchain_;

    Box<DeviceSynchronizer> sync_;

    Box<BindingGroupManager>          bindingLayoutManager_;
    Box<BufferManager>                bufferManager_;
    Box<PooledBufferManager>          pooledBufferManager_;
    Box<CommandBufferManager>         commandBufferManager_;
    Box<DynamicBufferManager>         dynamicBufferManager_;
    Box<PipelineManager>              pipelineManager_;
    Box<SamplerManager>               samplerManager_;
    Box<TextureManager>               textureManager_;
    Box<PooledTextureManager>         pooledTextureManager_;
    Box<CopyContext>                  copyContext_;
    Box<AccelerationStructureManager> accelerationManager_;
};

inline const RHI::ShaderGroupRecordRequirements &Device::GetShaderGroupRecordRequirements() const
{
    return GetRawDevice()->GetShaderGroupRecordRequirements();
}

inline Queue &Device::GetQueue()
{
    return mainQueue_;
}

inline DeviceSynchronizer &Device::GetSynchronizer()
{
    return *sync_;
}

inline const RHI::SwapchainPtr &Device::GetSwapchain() const
{
    return swapchain_;
}

inline const RHI::TextureDesc &Device::GetSwapchainImageDesc() const
{
    return swapchain_->GetRenderTargetDesc();
}

inline void Device::WaitIdle()
{
    sync_->WaitIdle();
}

inline void Device::BeginRenderLoop()
{
    sync_->BeginRenderLoop(swapchainImageCount_);
}

inline void Device::EndRenderLoop()
{
    sync_->EndRenderLoop();
}

inline void Device::Present()
{
    swapchain_->Present();
}

inline bool Device::BeginFrame()
{
    if(window_)
    {
        Window::DoEvents();
        if(!window_->HasFocus() || window_->GetFramebufferSize().x <= 0 || window_->GetFramebufferSize().y <= 0)
        {
            return false;
        }
    }

    // Note: commandBufferManager->_internalEndFrame() should be called before sync_->WaitForOldFrame().
    // Imagine that binding group B is bound to command buffer C in frame F. After F ends, B can be reused.
    // If we put commandBufferManager->_internalEndFrame() after sync_->WaitForOldFrame(), C will be reset to initial
    // state one frame later than B is reused, which means that B can be updated when still bound to C.
    commandBufferManager_->_internalEndFrame();
    sync_->WaitForOldFrame();

    if(swapchain_ && !swapchain_->Acquire())
    {
        return false;
    }
    sync_->BeginNewFrame();
    return true;
}

inline const RHI::FencePtr &Device::GetFrameFence()
{
    return sync_->GetFrameFence();
}

inline RC<Buffer> Device::CreateBuffer(const RHI::BufferDesc &desc)
{
    return bufferManager_->Create(desc);
}

inline RC<StatefulBuffer> Device::CreatePooledBuffer(const RHI::BufferDesc &desc)
{
    return pooledBufferManager_->Create(desc);
}

inline RC<Texture> Device::CreateTexture(const RHI::TextureDesc &desc)
{
    return textureManager_->Create(desc);
}

inline RC<StatefulTexture> Device::CreatePooledTexture(const RHI::TextureDesc &desc)
{
    return pooledTextureManager_->Create(desc);
}

inline RC<DynamicBuffer> Device::CreateDynamicBuffer()
{
    return dynamicBufferManager_->Create();
}

inline RC<SubBuffer> Device::CreateConstantBuffer(const void *data, size_t bytes)
{
    return dynamicBufferManager_->CreateConstantBuffer(data, bytes);
}

template<RtrcStruct T>
RC<SubBuffer> Device::CreateConstantBuffer(const T &data)
{
    auto ret = this->CreateDynamicBuffer();
    ret->SetData(data);
    return ret;
}

inline Box<RG::RenderGraph> Device::CreateRenderGraph()
{
    auto rg = MakeBox<RG::RenderGraph>();
    rg->SetQueue(mainQueue_);
    return rg;
}

template<BindingGroupDSL::RtrcGroupStruct T>
RC<BindingGroupLayout> Device::CreateBindingGroupLayout()
{
    // TODO: fast cache for these group layouts
    return this->CreateBindingGroupLayout(GetBindingGroupLayoutDesc<T>());
}

inline RC<BindingGroupLayout> Device::CreateBindingGroupLayout(const BindingGroupLayout::Desc &desc)
{
    return bindingLayoutManager_->CreateBindingGroupLayout(desc);
}

inline RC<BindingLayout> Device::CreateBindingLayout(const BindingLayout::Desc &desc)
{
    return bindingLayoutManager_->CreateBindingLayout(desc);
}

template<BindingGroupDSL::RtrcGroupStruct T>
RC<BindingGroup> Device::CreateBindingGroup(int variableBindingCount)
{
    return this->CreateBindingGroupLayout<T>()->CreateBindingGroup(variableBindingCount);
}

template<BindingGroupDSL::RtrcGroupStruct T>
RC<BindingGroup> Device::CreateBindingGroup(const RC<BindingGroupLayout> &layoutHint, int variableBindingCount)
{
    assert(layoutHint == this->CreateBindingGroupLayout<T>());
    return layoutHint->CreateBindingGroup(variableBindingCount);
}

template<BindingGroupDSL::RtrcGroupStruct T>
RC<BindingGroup> Device::CreateBindingGroup(const T &value, int variableBindingCount)
{
    auto group = this->CreateBindingGroup<T>(variableBindingCount);
    Rtrc::ApplyBindingGroup(device_.Get(), dynamicBufferManager_.get(), group, value);
    return group;
}

template<BindingGroupDSL::RtrcGroupStruct T>
RC<BindingGroup> Device::CreateBindingGroup(
    const T &value, const RC<BindingGroupLayout> &layoutHint, int variableBindingCount)
{
    auto group = this->CreateBindingGroup<T>(layoutHint, variableBindingCount);
    Rtrc::ApplyBindingGroup(device_.Get(), dynamicBufferManager_.get(), group, value);
    return group;
}

inline CommandBuffer Device::CreateCommandBuffer()
{
    return commandBufferManager_->Create();
}

inline RC<GraphicsPipeline> Device::CreateGraphicsPipeline(const GraphicsPipeline::Desc &desc)
{
    return pipelineManager_->CreateGraphicsPipeline(desc);
}

inline RC<ComputePipeline> Device::CreateComputePipeline(const RC<Shader> &shader)
{
    return pipelineManager_->CreateComputePipeline(shader);
}

inline RC<RayTracingLibrary> Device::CreateRayTracingLibrary(const RayTracingLibrary::Desc &desc)
{
    return pipelineManager_->CreateRayTracingLibrary(desc);
}

inline RC<RayTracingPipeline> Device::CreateRayTracingPipeline(const RayTracingPipeline::Desc &desc)
{
    return pipelineManager_->CreateRayTracingPipeline(desc);
}

inline RC<Sampler> Device::CreateSampler(const RHI::SamplerDesc &desc)
{
    return samplerManager_->CreateSampler(desc);
}

inline BlasPrebuildInfo Device::CreateBlasPrebuildinfo(
    Span<RHI::RayTracingGeometryDesc>             geometries,
    RHI::RayTracingAccelerationStructureBuildFlag flags)
{
    return accelerationManager_->CreateBlasPrebuildinfo(geometries, flags);
}

inline TlasPrebuildInfo Device::CreateTlasPrebuildInfo(
    Span<RHI::RayTracingInstanceArrayDesc>        instanceArrays,
    RHI::RayTracingAccelerationStructureBuildFlag flags)
{
    return accelerationManager_->CreateTlasPrebuildInfo(instanceArrays, flags);
}

inline RC<Blas> Device::CreateBlas(RC<SubBuffer> buffer)
{
    return accelerationManager_->CreateBlas(std::move(buffer));
}

inline RC<Tlas> Device::CreateTlas(RC<SubBuffer> buffer)
{
    return accelerationManager_->CreateTlas(std::move(buffer));
}

inline void Device::ExecuteAndWait(CommandBuffer commandBuffer)
{
    {
        auto c = std::move(commandBuffer);
        mainQueue_.Submit(c);
    }
    WaitIdle();
}

template<typename F> requires !std::is_same_v<std::remove_cvref_t<F>, CommandBuffer>
void Device::ExecuteAndWait(F &&f)
{
    auto c = commandBufferManager_->Create();
    c.Begin();
    std::invoke(std::forward<F>(f), c);
    c.End();
    ExecuteAndWait(std::move(c));
}

inline void Device::ExecuteBarrier(
    const RC<StatefulBuffer> &buffer,
    RHI::PipelineStageFlag    stages,
    RHI::ResourceAccessFlag   accesses)
{
    ExecuteBarrierImpl(buffer, stages, accesses);
}

inline void Device::ExecuteBarrier(
    const RC<StatefulTexture> &texture,
    RHI::TextureLayout         layout,
    RHI::PipelineStageFlag     stages,
    RHI::ResourceAccessFlag    accesses)
{
    ExecuteBarrierImpl(texture, layout, stages, accesses);
}

inline void Device::ExecuteBarrier(
    const RC<StatefulTexture> &texture,
    uint32_t                   arrayLayer,
    uint32_t                   mipLevel,
    RHI::TextureLayout         layout,
    RHI::PipelineStageFlag     stages,
    RHI::ResourceAccessFlag    accesses)
{
    ExecuteBarrierImpl(texture, arrayLayer, mipLevel, layout, stages, accesses);
}

inline void Device::ExecuteBarrier(
    const RC<Buffer>       &buffer,
    RHI::PipelineStageFlag  prevStages,
    RHI::ResourceAccessFlag prevAccesses,
    RHI::PipelineStageFlag  succStages,
    RHI::ResourceAccessFlag succAccesses)
{
    ExecuteBarrierImpl(buffer, prevStages, prevAccesses, succStages, succAccesses);
}

inline void Device::ExecuteBarrier(
    const RC<Texture>      &texture,
    RHI::TextureLayout      prevLayout,
    RHI::PipelineStageFlag  prevStages,
    RHI::ResourceAccessFlag prevAccesses,
    RHI::TextureLayout      succLayout,
    RHI::PipelineStageFlag  succStages,
    RHI::ResourceAccessFlag succAccesses)
{
    ExecuteBarrierImpl(
        texture, prevLayout, prevStages, prevAccesses, succLayout, succStages, succAccesses);
}

inline void Device::ExecuteBarrier(
    const RC<Texture>      &texture,
    uint32_t                arrayLayer,
    uint32_t                mipLevel,
    RHI::TextureLayout      prevLayout,
    RHI::PipelineStageFlag  prevStages,
    RHI::ResourceAccessFlag prevAccesses,
    RHI::TextureLayout      succLayout,
    RHI::PipelineStageFlag  succStages,
    RHI::ResourceAccessFlag succAccesses)
{
    ExecuteBarrierImpl(
        texture, arrayLayer, mipLevel,
        prevLayout, prevStages, prevAccesses, succLayout, succStages, succAccesses);
}

inline void Device::ExecuteBarrier(
    const RC<Texture> &texture,
    RHI::TextureLayout prevLayout,
    RHI::TextureLayout succLayout)
{
    ExecuteBarrierImpl(texture, prevLayout, succLayout);
}

inline void Device::ExecuteBarrier(
    const RC<StatefulTexture> &texture,
    RHI::TextureLayout         prevLayout,
    RHI::TextureLayout         succLayout)
{
    ExecuteBarrierImpl(texture, prevLayout, succLayout);
}

inline CopyContext &Device::GetCopyContext()
{
    return *copyContext_;
}

inline BufferManager &Device::GetBufferManager()
{
    return *bufferManager_;
}

inline BindingGroupManager &Device::GetBindingGroupManager()
{
    return *bindingLayoutManager_;
}

inline const RHI::DevicePtr &Device::GetRawDevice() const
{
    return device_;
}

inline Device::operator DynamicBufferManager&()
{
    return *dynamicBufferManager_;
}

template<typename... Args>
void Device::ExecuteBarrierImpl(Args &&... args)
{
    this->ExecuteAndWait([&](CommandBuffer &cmd)
    {
        cmd.ExecuteBarriers(BarrierBatch{}(std::forward<Args>(args)...));
    });
}

RTRC_END
