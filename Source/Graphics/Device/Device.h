#pragma once

#include <Graphics/Device/AccelerationStructure.h>
#include <Graphics/Device/BindingGroup.h>
#include <Graphics/Device/BindingGroupDSL.h>
#include <Graphics/Device/Buffer.h>
#include <Graphics/Device/CopyContext.h>
#include <Graphics/Device/LocalBindingGroupLayoutCache.h>
#include <Graphics/Device/Pipeline.h>
#include <Graphics/Device/Queue.h>
#include <Graphics/Device/Sampler.h>
#include <Graphics/Device/Texture.h>
#include <Graphics/Device/Utility/ClearBufferUtils.h>
#include <Graphics/Device/Utility/ClearTextureUtils.h>
#include <Graphics/Device/Utility/CopyTextureUtils.h>

RTRC_RG_BEGIN

class RenderGraph;

RTRC_RG_END

RTRC_BEGIN

namespace DeviceDetail
{

    enum class FlagBit : uint32_t
    {
        None                         = 0,
        EnableRayTracing             = 1 << 0,
        DisableAutoSwapchainRecreate = 1 << 1, // Don't recreate swapchain when window size changes
                                               // Usually required when render thread differs from window event thread
    };
    RTRC_DEFINE_ENUM_FLAGS(FlagBit)
    using Flags = EnumFlagsFlagBit;

} // namespace DeviceDetail

class Device : public Uncopyable, public WithUniqueObjectID
{
public:
    
    using Flags = DeviceDetail::Flags;
    using enum Flags::Bits;

    // Creation & destructor

    static Box<Device> CreateComputeDevice(RHI::DeviceUPtr rhiDevice);
    static Box<Device> CreateComputeDevice(RHI::BackendType rhiType, bool debugMode = RTRC_DEBUG);
    static Box<Device> CreateComputeDevice(bool debugMode = RTRC_DEBUG);

    static Box<Device> CreateGraphicsDevice(
        RHI::DeviceUPtr rhiDevice,
        Window         &window,
        RHI::Format     swapchainFormat     = RHI::Format::B8G8R8A8_UNorm,
        int             swapchainImageCount = 3,
        bool            vsync               = false,
        Flags           flags               = {});
    static Box<Device> CreateGraphicsDevice(
        Window          &window,
        RHI::BackendType rhiType,
        RHI::Format      swapchainFormat     = RHI::Format::B8G8R8A8_UNorm,
        int              swapchainImageCount = 3,
        bool             debugMode           = RTRC_DEBUG,
        bool             vsync               = false,
        Flags            flags               = {});
    static Box<Device> CreateGraphicsDevice(
        Window     &window,
        RHI::Format swapchainFormat     = RHI::Format::B8G8R8A8_UNorm,
        int         swapchainImageCount = 3,
        bool        debugMode           = RTRC_DEBUG,
        bool        vsync               = false,
        Flags       flags               = {});

    ~Device();

    // Query

    RHI::BackendType GetBackendType() const { return GetRawDevice()->GetBackendType(); }

    bool IsRayTracingEnabled() const { return flags_.Contains(EnableRayTracing); }

    const RHI::ShaderGroupRecordRequirements &GetShaderGroupRecordRequirements() const;

    size_t GetAccelerationStructureScratchBufferAlignment() const;

    size_t GetTextureBufferCopyRowPitchAlignment(RHI::Format format) const;

    const RHI::WarpSizeInfo &GetWarpSizeInfo() const;

    // Context objects

    void RecreateSwapchain();

    Queue &GetQueue();
    DeviceSynchronizer &GetSynchronizer();

    const RHI::SwapchainUPtr &GetSwapchain() const;
    const RHI::TextureDesc   &GetSwapchainImageDesc() const;

    // Frame synchronization

    void WaitIdle();

    void BeginRenderLoop();
    void EndRenderLoop();
    bool Present();

    bool BeginFrame(bool processWindowEvents = true);
    const RHI::FenceUPtr &GetFrameFence();

    // Resource creation & upload

    RC<Buffer>         CreateBuffer(const RHI::BufferDesc &desc);
    RC<StatefulBuffer> CreatePooledBuffer(const RHI::BufferDesc &desc);

    RC<Texture>         CreateTexture(const RHI::TextureDesc &desc);
    RC<StatefulTexture> CreatePooledTexture(const RHI::TextureDesc &desc);
    RC<Texture>         CreateColorTexture2D(uint8_t r, uint8_t g, uint8_t b, uint8_t a, const std::string &name = {});

    RC<DynamicBuffer> CreateDynamicBuffer();

    RC<SubBuffer> CreateConstantBuffer(const void *data, size_t bytes);
    template<RtrcReflShaderStruct T>
    RC<SubBuffer> CreateConstantBuffer(const T &data);
    
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
        RHI::TextureUsageFlags usages,
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

    template<BindingGroupDSL::RtrcGroupStruct T>
    RC<BindingGroup> CreateBindingGroupWithCachedLayout(const T &value);

    void CopyBindings(
        const RC<BindingGroup> &dst, uint32_t dstSlot, uint32_t dstArrElem,
        const RC<BindingGroup> &src, uint32_t srcSlot, uint32_t srcArrElem,
        uint32_t count);

    RC<BindingLayout> CreateBindingLayout(const BindingLayout::Desc &desc);

    CommandBuffer CreateCommandBuffer();

    RC<GraphicsPipeline> CreateGraphicsPipeline(const GraphicsPipeline::Desc &desc);
    RC<ComputePipeline> CreateComputePipeline(const RC<Shader> &shader);

    RC<RayTracingLibrary> CreateRayTracingLibrary(const RayTracingLibrary::Desc &desc);
    RC<RayTracingPipeline> CreateRayTracingPipeline(const RayTracingPipeline::Desc &desc);

    RC<Sampler> CreateSampler(const RHI::SamplerDesc &desc);

    // Acceleration structure
    
    BlasPrebuildInfo CreateBlasPrebuildinfo(
        Span<RHI::RayTracingGeometryDesc>              geometries,
        RHI::RayTracingAccelerationStructureBuildFlags flags);
    TlasPrebuildInfo CreateTlasPrebuildInfo(
        const RHI::RayTracingInstanceArrayDesc        &instances,
        RHI::RayTracingAccelerationStructureBuildFlags flags);

    RC<Blas> CreateBlas(RC<SubBuffer> buffer = nullptr);
    RC<Tlas> CreateTlas(RC<SubBuffer> buffer = nullptr);

    // Immediate execution
    // Don't use them in render loop

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

    CopyContext             &GetCopyContext();
    BufferManager           &GetBufferManager();
    BindingGroupManager     &GetBindingGroupManager();
    BindingGroupLayoutCache &GetBindingGroupCache();
    const RHI::DeviceUPtr   &GetRawDevice() const;

    ClearBufferUtils  &GetClearBufferUtils()  { return *clearBufferUtils_; }
    ClearTextureUtils &GetClearTextureUtils() { return *clearTextureUtils_; }
    CopyTextureUtils  &GetCopyTextureUtils()  { return *copyTextureUtils_; }

    operator DynamicBufferManager &();

private:

    Device() = default;
    
    void InitializeInternal(Flags flags, RHI::DeviceUPtr device, bool isComputeOnly);

    template<typename...Args>
    void ExecuteBarrierImpl(Args&&...args);

    Flags flags_;

    RHI::InstanceUPtr instance_;
    RHI::DeviceUPtr   device_;
    Queue             mainQueue_;

    Window            *window_              = nullptr;
    RHI::Format        swapchainFormat_     = RHI::Format::Unknown;
    int                swapchainImageCount_ = 0;
    bool               vsync_               = true;
    bool               swapchainUav_        = false;
    RHI::SwapchainUPtr swapchain_;

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
    Box<ClearBufferUtils>             clearBufferUtils_;
    Box<ClearTextureUtils>            clearTextureUtils_;
    Box<CopyTextureUtils>             copyTextureUtils_;
    Box<BindingGroupLayoutCache>      bindingGroupLayoutCache_;
};

inline const RHI::ShaderGroupRecordRequirements &Device::GetShaderGroupRecordRequirements() const
{
    return GetRawDevice()->GetShaderGroupRecordRequirements();
}

inline size_t Device::GetAccelerationStructureScratchBufferAlignment() const
{
    return GetRawDevice()->GetAccelerationStructureScratchBufferAlignment();
}

inline size_t Device::GetTextureBufferCopyRowPitchAlignment(RHI::Format format) const
{
    return GetRawDevice()->GetTextureBufferCopyRowPitchAlignment(format);
}

inline const RHI::WarpSizeInfo &Device::GetWarpSizeInfo() const
{
    return GetRawDevice()->GetWarpSizeInfo();
}

inline void Device::RecreateSwapchain()
{
    assert(window_);
    swapchain_.Reset();
    swapchain_ = device_->CreateSwapchain(RHI::SwapchainDesc
    {
        .format     = swapchainFormat_,
        .imageCount = static_cast<uint32_t>(swapchainImageCount_),
        .vsync      = vsync_,
        .allowUav   = swapchainUav_
    }, *window_);
}

inline Queue &Device::GetQueue()
{
    return mainQueue_;
}

inline DeviceSynchronizer &Device::GetSynchronizer()
{
    return *sync_;
}

inline const RHI::SwapchainUPtr &Device::GetSwapchain() const
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

inline bool Device::Present()
{
    return swapchain_->Present();
}

inline bool Device::BeginFrame(bool processWindowEvents)
{
    if(processWindowEvents && window_)
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

inline const RHI::FenceUPtr &Device::GetFrameFence()
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

template<RtrcReflShaderStruct T>
RC<SubBuffer> Device::CreateConstantBuffer(const T &data)
{
    auto ret = this->CreateDynamicBuffer();
    ret->SetData(data);
    return ret;
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
#if RTRC_DEBUG
    auto layout = this->CreateBindingGroupLayout<T>();
    if(layout != layoutHint)
    {
        LogError("Device::CreateBindingGroup: Unmatched binding group layout");
        LogError("Layout hint is");
        Rtrc::DumpBindingGroupLayoutDesc(layoutHint->GetRHIObject()->GetDesc());
        LogError("Layout created from type is");
        Rtrc::DumpBindingGroupLayoutDesc(layout->GetRHIObject()->GetDesc());
        std::terminate();
    }
#endif
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

template<BindingGroupDSL::RtrcGroupStruct T>
RC<BindingGroup> Device::CreateBindingGroupWithCachedLayout(const T &value)
{
    RTRC_STATIC_BINDING_GROUP_LAYOUT(this, std::remove_cvref_t<decltype(value)>, _tempLayout);
    return this->CreateBindingGroup(value, _tempLayout);
}

inline void Device::CopyBindings(
    const RC<BindingGroup> &dst, uint32_t dstSlot, uint32_t dstArrElem,
    const RC<BindingGroup> &src, uint32_t srcSlot, uint32_t srcArrElem,
    uint32_t count)
{
    bindingLayoutManager_->CopyBindings(dst, dstSlot, dstArrElem, src, srcSlot, srcArrElem, count);
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
    Span<RHI::RayTracingGeometryDesc>              geometries,
    RHI::RayTracingAccelerationStructureBuildFlags flags)
{
    return accelerationManager_->CreateBlasPrebuildinfo(geometries, flags);
}

inline TlasPrebuildInfo Device::CreateTlasPrebuildInfo(
    const RHI::RayTracingInstanceArrayDesc        &instances,
    RHI::RayTracingAccelerationStructureBuildFlags flags)
{
    return accelerationManager_->CreateTlasPrebuildInfo(instances, flags);
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

inline BindingGroupLayoutCache &Device::GetBindingGroupCache()
{
    return *bindingGroupLayoutCache_;
}

inline const RHI::DeviceUPtr &Device::GetRawDevice() const
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

template<BindingGroupDSL::RtrcGroupStruct T>
RC<BindingGroupLayout> BindingGroupLayoutCache::Get(CachedBindingGroupLayoutStorage *storage)
{
    const uint32_t index = storage->index_;

    {
        std::shared_lock lock(mutex_);
        if(index < layouts_.size() && layouts_[index])
        {
            return layouts_[index];
        }
    }

    std::lock_guard lock(mutex_);
    if(index < layouts_.size() && layouts_[index])
    {
        return layouts_[index];
    }

    if(index >= layouts_.size())
    {
        layouts_.resize(index + 1);
    }
    if(!layouts_[index])
    {
        layouts_[index] = device_->CreateBindingGroupLayout<T>();
    }
    return layouts_[index];
}

template<typename T>
RC<BindingGroupLayout> CachedBindingGroupLayoutHandle<T>::Get() const
{
    return device_->GetBindingGroupCache().Get<T>(storage_);
}

RTRC_END
