#pragma once

#include <Rtrc/Graphics/Device/AccelerationStructure.h>
#include <Rtrc/Graphics/Device/BindingGroup.h>
#include <Rtrc/Graphics/Device/Buffer.h>
#include <Rtrc/Graphics/Device/CopyContext/Downloader.h>
#include <Rtrc/Graphics/Device/CopyContext/Uploader.h>
#include <Rtrc/Graphics/Device/LocalBindingGroupLayoutCache.h>
#include <Rtrc/Graphics/Device/Pipeline.h>
#include <Rtrc/Graphics/Device/Queue.h>
#include <Rtrc/Graphics/Device/Sampler.h>
#include <Rtrc/Graphics/Device/Texture.h>
#include <Rtrc/Graphics/Device/Utility/ClearBufferUtils.h>
#include <Rtrc/Graphics/Device/Utility/ClearTextureUtils.h>
#include <Rtrc/Graphics/Device/Utility/CopyTextureUtils.h>
#include <Rtrc/Graphics/Shader/DSL/BindingGroup/BindingGroupHelpers.h>
#include <Rtrc/Graphics/Shader/ShaderManager.h>

RTRC_BEGIN

class RenderGraph;
using GraphRef = Ref<RenderGraph>;

namespace DeviceDetail
{

    enum class FlagBit : uint32_t
    {
        None                         = 0,
        EnableRayTracing             = 1 << 0,
        DisableAutoSwapchainRecreate = 1 << 1, // Don't recreate swapchain when window size changes.
                                               // In that case, swapchain must be manually recreated by calling Device::RecreateSwapchain().
    };
    RTRC_DEFINE_ENUM_FLAGS(FlagBit)
    using Flags = EnumFlagsFlagBit;

} // namespace DeviceDetail

class Device;
using DeviceRef = Ref<Device>;

class Device : public Uncopyable, public WithUniqueObjectID
{
public:
    
    using Flags = DeviceDetail::Flags;
    using enum Flags::Bits;

    struct ComputeDeviceDesc
    {
        RHI::BackendType rhiType   = RHI::BackendType::Default;
        bool             debugMode = RTRC_DEBUG;
    };

    struct GraphicsDeviceDesc
    {
        Ref<Window>      window; // optional. only required if want to create a swapchain with the device.
        RHI::BackendType rhiType             = RHI::BackendType::Default;
        RHI::Format      swapchainFormat     = RHI::Format::B8G8R8A8_UNorm;
        int              swapchainImageCount = 3;
        bool             debugMode           = RTRC_DEBUG;
        bool             vsync               = false;
        Flags            flags               = {};
    };

    // Creation & destructor

    static Box<Device> CreateComputeDevice(const ComputeDeviceDesc &desc);
    static Box<Device> CreateGraphicsDevice(const GraphicsDeviceDesc &desc);

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
    Queue &GetCopyQueue();
    DeviceSynchronizer &GetSynchronizer();

    const RHI::SwapchainUPtr &GetSwapchain() const;
    const RHI::TextureDesc   &GetSwapchainImageDesc() const;

    // Frame synchronization

    void WaitIdle();

    /** Simplified BeginFrame with neither handling window events nor switching swapchain image */
    void AddSynchronizationPoint();

    void BeginRenderLoop();
    void EndRenderLoop();
    bool Present();

    bool BeginFrame(bool processWindowEvents = true);
    void EndFrame();

    const RHI::FenceUPtr& GetFrameFence();

    // Shader

    ShaderManager &GetShaderManager();

    RC<Shader>         GetShader(std::string_view name, bool persistent);
    RC<ShaderTemplate> GetShaderTemplate(std::string_view name, bool persistent);

    template<TemplateStringParameter Name> RC<Shader>         GetShader();
    template<TemplateStringParameter Name> RC<ShaderTemplate> GetShaderTemplate();

    // Resource creation & upload

    RC<Buffer>         CreateBuffer(const RHI::BufferDesc &desc, std::string name = {});
    RC<StatefulBuffer> CreateStatefulBuffer(const RHI::BufferDesc &desc, std::string name = {});

    RC<Buffer> CreateTexelBuffer(size_t count, RHI::Format format, RHI::BufferUsageFlag usages, std::string name = {});
    RC<Buffer> CreateStructuredBuffer(size_t count, size_t stride, RHI::BufferUsageFlag usages, std::string name = {});

    RC<StatefulBuffer> CreateStatefulTexelBuffer(size_t count, RHI::Format format, RHI::BufferUsageFlag usages, std::string name = {});
    RC<StatefulBuffer> CreateStatefulStructuredBuffer(size_t count, size_t stride, RHI::BufferUsageFlag usages, std::string name = {});

    RC<Texture>         CreateTexture(const RHI::TextureDesc &desc, std::string name = {});
    RC<StatefulTexture> CreateStatefulTexture(const RHI::TextureDesc &desc, std::string name = {});
    RC<Texture>         CreateColorTexture2D(uint8_t r, uint8_t g, uint8_t b, uint8_t a, std::string name = {});

    RC<DynamicBuffer> CreateDynamicBuffer();

    RC<SubBuffer> CreateConstantBuffer(const void *data, size_t bytes);
    template<RtrcStruct T>
    RC<SubBuffer> CreateConstantBuffer(const T &data);

    Uploader &GetUploader();
    Downloader &GetDownloader();
    
    void Upload(
        const RC<Buffer> &buffer,
        const void       *data,
        size_t            offset = 0,
        size_t            size = 0);
    void Upload(
        const RC<Texture> &texture,
        TexSubrsc          subrsc,
        const void        *data,
        size_t             dataRowBytes,
        RHI::TextureLayout afterLayout);
    void Upload(
        const RC<StatefulTexture> &texture,
        TexSubrsc                  subrsc,
        const void                *data,
        size_t                     dataRowBytes,
        RHI::TextureLayout         afterLayout);
    void Upload(
        const RC<Texture>  &texture,
        TexSubrsc           subrsc,
        const ImageDynamic &image,
        RHI::TextureLayout  afterLayout);
    void Upload(
        const RC<StatefulTexture> &texture,
        TexSubrsc                  subrsc,
        const ImageDynamic        &image,
        RHI::TextureLayout         afterLayout);
    
    void Download(const RC<StatefulBuffer> &buffer,
                  void                     *data,
                  size_t                    offset = 0,
                  size_t                    size = 0);

    void Download(const RC<StatefulTexture> &texture,
                  TexSubrsc         subrsc,
                  void             *outputData,
                  size_t            dataRowBytes = 0);

    template<typename T>
    RC<Buffer> CreateAndUploadTexelBuffer(RHI::BufferUsageFlag usages, Span<T> data, RHI::Format format);
    template<typename T>
    RC<Buffer> CreateAndUploadTexelBuffer(Span<T> data, RHI::Format format); // buffer usages: ShaderBuffer
    template<typename T>
    RC<Buffer> CreateAndUploadStructuredBuffer(RHI::BufferUsageFlag usages, Span<T> data);
    template<typename T>
    RC<Buffer> CreateAndUploadStructuredBuffer(Span<T> data); // buffer usages: ShaderStructuredBuffer
    RC<Buffer> CreateAndUploadBuffer(
        const RHI::BufferDesc &desc,
        const void            *initData,
        size_t                 initDataOffset = 0,
        size_t                 initDataSize = 0);
    RC<Texture> CreateAndUploadTexture(
        const RHI::TextureDesc &desc,
        Span<const void *>      imageData, // 'data of layer a, mip m' is in imageData[a * M + m]
        RHI::TextureLayout      afterLayout);
    RC<Texture> CreateAndUploadTexture2D(
        const RHI::TextureDesc &desc,
        Span<ImageDynamic>      images,
        RHI::TextureLayout      afterLayout);

    RC<Texture> LoadTexture2D(
        const std::string     &filename,
        RHI::Format            format,
        RHI::TextureUsageFlags usages,
        bool                   generateMipLevels,
        RHI::TextureLayout     afterLayout);

    // Pipeline object creation

    Box<RenderGraph> CreateRenderGraph();

    template<RtrcGroupStruct T>
    RC<BindingGroupLayout> CreateBindingGroupLayout();
    RC<BindingGroupLayout> CreateBindingGroupLayout(const BindingGroupLayout::Desc &desc);

    template<RtrcGroupStruct T>
    RC<BindingGroup> CreateBindingGroup(int variableBindingCount = 0);
    template<RtrcGroupStruct T>
    RC<BindingGroup> CreateBindingGroup(const RC<BindingGroupLayout> &layoutHint, int variableBindingCount = 0);
    template<RtrcGroupStruct T>
    RC<BindingGroup> CreateBindingGroup(const T &value, int variableBindingCount = 0);
    template<RtrcGroupStruct T>
    RC<BindingGroup> CreateBindingGroup(const T &value, const RC<BindingGroupLayout> &layoutHint, int variableBindingCount = 0);

    template<RtrcGroupStruct T>
    RC<BindingGroup> CreateBindingGroupWithCachedLayout(const T &value);

    void CopyBindings(
        const RC<BindingGroup> &dst, uint32_t dstSlot, uint32_t dstArrElem,
        const RC<BindingGroup> &src, uint32_t srcSlot, uint32_t srcArrElem,
        uint32_t count);

    RC<BindingLayout> CreateBindingLayout(const BindingLayout::Desc &desc);

    CommandBuffer CreateCommandBuffer();

    RC<GraphicsPipeline> CreateGraphicsPipeline(const GraphicsPipelineDesc &desc);
    RC<ComputePipeline> CreateComputePipeline(const RC<Shader> &shader);
    RC<WorkGraphPipeline> CreateWorkGraphPipeline(const WorkGraphPipeline::Desc &desc);

    RC<RayTracingLibrary> CreateRayTracingLibrary(const RayTracingLibrary::Desc &desc);
    RC<RayTracingPipeline> CreateRayTracingPipeline(const RayTracingPipeline::Desc &desc);

    RC<Sampler> CreateSampler(const RHI::SamplerDesc &desc);

    // Acceleration structure
    
    Box<BlasPrebuildInfo> CreateBlasPrebuildinfo(
        Span<RHI::RayTracingGeometryDesc>              geometries,
        RHI::RayTracingAccelerationStructureBuildFlags flags);
    Box<TlasPrebuildInfo> CreateTlasPrebuildInfo(
        const RHI::RayTracingInstanceArrayDesc        &instances,
        RHI::RayTracingAccelerationStructureBuildFlags flags);

    RC<Blas> CreateBlas(RC<SubBuffer> buffer = nullptr);
    RC<Tlas> CreateTlas(RC<SubBuffer> buffer = nullptr);

    // Immediate execution. Don't use these in render loop.

    void ExecuteAndWait(CommandBuffer commandBuffer);
    template<typename F> requires !std::is_same_v<std::remove_cvref_t<F>, CommandBuffer>
    void ExecuteAndWait(F &&f); // f(commandBuffer)

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

    // Internal helper classes

    BufferManager           &GetBufferManager();
    BindingGroupManager     &GetBindingGroupManager();
    BindingGroupLayoutCache &GetBindingGroupCache();
    PipelineManager         &GetPipelineManager();
    const RHI::DeviceUPtr   &GetRawDevice() const;

    ClearBufferUtils  &GetClearBufferUtils()  { return *clearBufferUtils_; }
    ClearTextureUtils &GetClearTextureUtils() { return *clearTextureUtils_; }
    CopyTextureUtils  &GetCopyTextureUtils()  { return *copyTextureUtils_; }

    operator DynamicBufferManager &();

private:

    static RHI::BackendType DebugOverrideBackendType(RHI::BackendType type);

    Device() = default;
    
    void InitializeInternal(Flags flags, RHI::DeviceUPtr device, bool isComputeOnly, bool debugMode);

    template<typename...Args>
    void ExecuteBarrierImpl(Args&&...args);

    Flags flags_;

    RHI::InstanceUPtr instance_;
    RHI::DeviceUPtr   device_;
    Queue             mainQueue_;
    Queue             copyQueue_;

    Window            *window_              = nullptr;
    RHI::Format        swapchainFormat_     = RHI::Format::Unknown;
    int                swapchainImageCount_ = 0;
    bool               vsync_               = true;
    bool               swapchainUav_        = false;
    RHI::SwapchainUPtr swapchain_;

    RC<Receiver<WindowResizeEvent>> windowResizeReceiver_;

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
    Box<Downloader>                   downloader_;
    Box<Uploader>                     uploader_;
    Box<AccelerationStructureManager> accelerationManager_;
    Box<ClearBufferUtils>             clearBufferUtils_;
    Box<ClearTextureUtils>            clearTextureUtils_;
    Box<CopyTextureUtils>             copyTextureUtils_;
    Box<BindingGroupLayoutCache>      bindingGroupLayoutCache_;
    Box<ShaderManager>                shaderManager_;
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

inline Queue &Device::GetCopyQueue()
{
    return copyQueue_;
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
    commandBufferManager_->_internalEndFrame();
    sync_->WaitIdle();
}

inline void Device::AddSynchronizationPoint()
{
    commandBufferManager_->_internalEndFrame();
    if(sync_->IsInRenderLoop())
    {
        sync_->WaitForOldFrame();
        sync_->BeginNewFrame();
    }
    else
    {
        sync_->WaitIdle();
    }
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
    assert(sync_->IsInRenderLoop());
    if(processWindowEvents && window_)
    {
        Window::DoEvents();
        if(!window_->HasFocus() || window_->GetFramebufferSize().x <= 0 || window_->GetFramebufferSize().y <= 0)
        {
            return false;
        }
    }

    sync_->WaitForOldFrame();

    if(swapchain_ && !swapchain_->Acquire())
    {
        return false;
    }
    sync_->BeginNewFrame();
    return true;
}

inline void Device::EndFrame()
{
    assert(sync_->IsInRenderLoop());
    commandBufferManager_->_internalEndFrame();
}

inline const RHI::FenceUPtr &Device::GetFrameFence()
{
    return sync_->GetFrameFence();
}

inline ShaderManager &Device::GetShaderManager()
{
    return *shaderManager_;
}

inline RC<Shader> Device::GetShader(std::string_view name, bool persistent)
{
    return shaderManager_->GetShader(name, persistent);
}

inline RC<ShaderTemplate> Device::GetShaderTemplate(std::string_view name, bool persistent)
{
    return shaderManager_->GetShaderTemplate(name, persistent);
}

template<TemplateStringParameter Name>
RC<Shader> Device::GetShader()
{
    return shaderManager_->GetShader<Name>();
}

template<TemplateStringParameter Name>
RC<ShaderTemplate> Device::GetShaderTemplate()
{
    return shaderManager_->GetShaderTemplate<Name>();
}

inline RC<Buffer> Device::CreateBuffer(const RHI::BufferDesc &desc, std::string name)
{
    auto ret = bufferManager_->Create(desc);
    if(!name.empty())
    {
        ret->SetName(std::move(name));
    }
    return ret;
}

inline RC<StatefulBuffer> Device::CreateStatefulBuffer(const RHI::BufferDesc &desc, std::string name)
{
    auto ret = pooledBufferManager_->Create(desc);
    if(!name.empty())
    {
        ret->SetName(std::move(name));
    }
    return ret;
}

inline RC<Buffer> Device::CreateTexelBuffer(
    size_t count, RHI::Format format, RHI::BufferUsageFlag usages, std::string name)
{
    auto buffer = this->CreateBuffer(RHI::BufferDesc
    {
        .size = count * RHI::GetTexelSize(format),
        .usage = usages
    }, std::move(name));
    buffer->SetDefaultTexelFormat(format);
    return buffer;
}

inline RC<Buffer> Device::CreateStructuredBuffer(
    size_t count, size_t stride, RHI::BufferUsageFlag usages, std::string name)
{
    auto buffer = this->CreateBuffer(RHI::BufferDesc
    {
        .size = count * stride,
        .usage = usages
    }, std::move(name));
    buffer->SetDefaultStructStride(stride);
    return buffer;
}

inline RC<StatefulBuffer> Device::CreateStatefulTexelBuffer(
    size_t count, RHI::Format format, RHI::BufferUsageFlag usages, std::string name)
{
    auto buffer = this->CreateStatefulBuffer(RHI::BufferDesc
    {
        .size = count * RHI::GetTexelSize(format),
        .usage = usages
    }, std::move(name));
    buffer->SetDefaultTexelFormat(format);
    return buffer;
}

inline RC<StatefulBuffer> Device::CreateStatefulStructuredBuffer(
    size_t count, size_t stride, RHI::BufferUsageFlag usages, std::string name)
{
    auto buffer = this->CreateStatefulBuffer(RHI::BufferDesc
    {
        .size = count * stride,
        .usage = usages
    }, std::move(name));
    buffer->SetDefaultStructStride(stride);
    return buffer;
}

inline RC<Texture> Device::CreateTexture(const RHI::TextureDesc &desc, std::string name)
{
    auto ret = textureManager_->Create(desc);
    if(!name.empty())
    {
        ret->SetName(std::move(name));
    }
    return ret;
}

inline RC<StatefulTexture> Device::CreateStatefulTexture(const RHI::TextureDesc &desc, std::string name)
{
    auto ret = pooledTextureManager_->Create(desc);
    if(!name.empty())
    {
        ret->SetName(std::move(name));
    }
    return ret;
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

inline Uploader &Device::GetUploader()
{
    return *uploader_;
}

inline Downloader &Device::GetDownloader()
{
    return *downloader_;
}

inline void Device::Upload(
    const RC<Buffer> &buffer,
    const void       *data,
    size_t            offset,
    size_t            size)
{
    uploader_->Upload(buffer, data, offset, size);
}

inline void Device::Upload(
    const RC<Texture> &texture,
    TexSubrsc          subrsc,
    const void        *data,
    size_t             dataRowBytes,
    RHI::TextureLayout afterLayout)
{
    uploader_->Upload(texture, subrsc, data, dataRowBytes, afterLayout);
}

inline void Device::Upload(
    const RC<StatefulTexture> &texture,
    TexSubrsc                  subrsc,
    const void                *data,
    size_t                     dataRowBytes,
    RHI::TextureLayout         afterLayout)
{
    uploader_->Upload(texture, subrsc, data, dataRowBytes, afterLayout);
}

inline void Device::Upload(
    const RC<Texture>  &texture,
    TexSubrsc           subrsc,
    const ImageDynamic &image,
    RHI::TextureLayout  afterLayout)
{
    uploader_->Upload(texture, subrsc, image, afterLayout);
}

inline void Device::Upload(
    const RC<StatefulTexture> &texture,
    TexSubrsc                  subrsc,
    const ImageDynamic        &image,
    RHI::TextureLayout         afterLayout)
{
    uploader_->Upload(texture, subrsc, image, afterLayout);
}

inline void Device::Download(const RC<StatefulBuffer> &buffer, void *data, size_t offset, size_t size)
{
    downloader_->Download(buffer, data, offset, size);
}

inline void Device::Download(
    const RC<StatefulTexture> &texture, TexSubrsc subrsc, void *outputData, size_t dataRowBytes)
{
    downloader_->Download(texture, subrsc, outputData, dataRowBytes);
}

template<typename T>
RC<Buffer> Device::CreateAndUploadTexelBuffer(RHI::BufferUsageFlag usages, Span<T> data, RHI::Format format)
{
    assert(RHI::GetTexelSize(format) == sizeof(T));
    auto buffer = this->CreateAndUploadBuffer(RHI::BufferDesc
        {
            .size = data.GetSize() * sizeof(T),
            .usage = usages
        });
    buffer->SetDefaultTexelFormat(format);
    return buffer;
}

template <typename T>
RC<Buffer> Device::CreateAndUploadTexelBuffer(Span<T> data, RHI::Format format)
{
    return this->CreateAndUploadTexelBuffer(RHI::BufferUsage::ShaderBuffer, data, format);
}

template<typename T>
RC<Buffer> Device::CreateAndUploadStructuredBuffer(RHI::BufferUsageFlag usages, Span<T> data)
{
    auto buffer = this->CreateAndUploadBuffer(RHI::BufferDesc
        {
            .size = data.GetSize() * sizeof(T),
            .usage = usages
        }, data.GetData());
    buffer->SetDefaultStructStride(sizeof(T));
    return buffer;
}

template <typename T>
RC<Buffer> Device::CreateAndUploadStructuredBuffer(Span<T> data)
{
    return this->CreateAndUploadStructuredBuffer(RHI::BufferUsage::ShaderStructuredBuffer, data);
}

template<RtrcGroupStruct T>
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

template<RtrcGroupStruct T>
RC<BindingGroup> Device::CreateBindingGroup(int variableBindingCount)
{
    return this->CreateBindingGroupLayout<T>()->CreateBindingGroup(variableBindingCount);
}

template<RtrcGroupStruct T>
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

template<RtrcGroupStruct T>
RC<BindingGroup> Device::CreateBindingGroup(const T &value, int variableBindingCount)
{
    auto group = this->CreateBindingGroup<T>(variableBindingCount);
    Rtrc::ApplyBindingGroup(device_.Get(), dynamicBufferManager_.get(), group, value);
    return group;
}

template<RtrcGroupStruct T>
RC<BindingGroup> Device::CreateBindingGroup(
    const T &value, const RC<BindingGroupLayout> &layoutHint, int variableBindingCount)
{
    auto group = this->CreateBindingGroup<T>(layoutHint, variableBindingCount);
    Rtrc::ApplyBindingGroup(device_.Get(), dynamicBufferManager_.get(), group, value);
    return group;
}

template<RtrcGroupStruct T>
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

inline RC<GraphicsPipeline> Device::CreateGraphicsPipeline(const GraphicsPipelineDesc &desc)
{
    return pipelineManager_->CreateGraphicsPipeline(desc);
}

inline RC<ComputePipeline> Device::CreateComputePipeline(const RC<Shader> &shader)
{
    return pipelineManager_->CreateComputePipeline(shader);
}

inline RC<WorkGraphPipeline> Device::CreateWorkGraphPipeline(const WorkGraphPipeline::Desc &desc)
{
    return pipelineManager_->CreateWorkGraphPipeline(desc);
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

inline Box<BlasPrebuildInfo> Device::CreateBlasPrebuildinfo(
    Span<RHI::RayTracingGeometryDesc>              geometries,
    RHI::RayTracingAccelerationStructureBuildFlags flags)
{
    return accelerationManager_->CreateBlasPrebuildinfo(geometries, flags);
}

inline Box<TlasPrebuildInfo> Device::CreateTlasPrebuildInfo(
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

inline PipelineManager &Device::GetPipelineManager()
{
    return *pipelineManager_;
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

template<RtrcGroupStruct T>
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
