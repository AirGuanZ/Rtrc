#include <Rtrc/Graphics/Device/Device.h>
#include <Rtrc/Graphics/RenderGraph/Graph.h>
#include <Rtrc/Core/Resource/GenerateMipmap.h>

RTRC_BEGIN

Box<Device> Device::CreateComputeDevice(RHI::DeviceUPtr rhiDevice)
{
    Box<Device> ret{ new Device };
    ret->InitializeInternal(None, std::move(rhiDevice), true);
    return ret;
}

Box<Device> Device::CreateComputeDevice(RHI::BackendType rhiType, bool debugMode)
{
    rhiType = DebugOverrideBackendType(rhiType);
    Box<Device> ret{ new Device };
#if RTRC_RHI_VULKAN
    if(rhiType == RHI::BackendType::Vulkan)
    {
        ret->instance_ = RHI::CreateVulkanInstance({ .debugMode = debugMode });
    }
#endif
#if RTRC_RHI_DIRECTX12
    if(rhiType == RHI::BackendType::DirectX12)
    {
        ret->instance_ = RHI::CreateDirectX12Instance({ .debugMode = debugMode, .gpuValidation = false });
    }
#endif
    if(!ret->instance_)
    {
        throw Exception("No valid rhi instance is created");
    }
    const RHI::DeviceDesc deviceDesc =
    {
        .graphicsQueue    = false,
        .computeQueue     = true,
        .transferQueue    = true,
        .supportSwapchain = false
    };
    auto rhiDevice = ret->instance_->CreateDevice(deviceDesc);
    ret->InitializeInternal(None, std::move(rhiDevice), true);
    return ret;
}

Box<Device> Device::CreateComputeDevice(bool debugMode)
{
    return CreateComputeDevice(RHI::BackendType::Default, debugMode);
}

Box<Device> Device::CreateGraphicsDevice(
    RHI::DeviceUPtr rhiDevice,
    Window         &window,
    RHI::Format     swapchainFormat,
    int             swapchainImageCount,
    bool            vsync,
    Flags           flags)
{
    Box<Device> ret{ new Device };
    ret->InitializeInternal(flags, std::move(rhiDevice), false);
    ret->window_              = &window;
    ret->swapchainFormat_     = swapchainFormat;
    ret->swapchainImageCount_ = swapchainImageCount;
    ret->vsync_               = vsync;
    ret->swapchainUav_        = false; // d3d12 doesn't support swapchain uav
    ret->RecreateSwapchain();
    if(!flags.Contains(DisableAutoSwapchainRecreate))
    {
        ret->window_->Attach([d = ret.get()](const WindowResizeEvent &e)
        {
            if(e.width > 0 && e.height > 0)
            {
                d->device_->WaitIdle();
                d->RecreateSwapchain();
            }
        });
    }
    return ret;
}

Box<Device> Device::CreateGraphicsDevice(
    Window          &window,
    RHI::BackendType rhiType,
    RHI::Format      swapchainFormat,
    int              swapchainImageCount,
    bool             debugMode,
    bool             vsync,
    Flags            flags)
{
    Box<Device> ret{ new Device };

    rhiType = DebugOverrideBackendType(rhiType);
#if RTRC_RHI_VULKAN
    if(rhiType == RHI::BackendType::Vulkan)
    {
        ret->instance_ = CreateVulkanInstance(RHI::VulkanInstanceDesc
            {
                .extensions = Window::GetRequiredVulkanInstanceExtensions(),
                .debugMode = debugMode
            });
    }
#endif
#if RTRC_RHI_DIRECTX12
    if(rhiType == RHI::BackendType::DirectX12)
    {
        ret->instance_ = CreateDirectX12Instance(RHI::DirectX12InstanceDesc
        {
            .debugMode = debugMode,
#if RTRC_DEBUG
            .gpuValidation = false /*debugMode*/ // gpu validation cause serious ram leak. disable it for now.
#else
            .gpuValidation = false
#endif
        });
    }
#endif
    if(!ret->instance_)
    {
        throw Exception("No valid rhi instance is created");
    }

    const RHI::DeviceDesc deviceDesc =
    {
        .graphicsQueue    = true,
        .computeQueue     = true,
        .transferQueue    = true,
        .supportSwapchain = true,
        .enableRayTracing = flags.Contains(EnableRayTracing)
    };
    auto rhiDevice = ret->instance_->CreateDevice(deviceDesc);
    ret->InitializeInternal(flags, std::move(rhiDevice), false);
    ret->window_              = &window;
    ret->swapchainFormat_     = swapchainFormat;
    ret->swapchainImageCount_ = swapchainImageCount;
    ret->vsync_               = vsync;
    ret->swapchainUav_        = false;
    ret->RecreateSwapchain();
    if(!flags.Contains(DisableAutoSwapchainRecreate))
    {
        ret->window_->Attach([d = ret.get()](const WindowResizeEvent &e)
        {
            if(e.width > 0 && e.height > 0)
            {
                d->device_->WaitIdle();
                d->RecreateSwapchain();
            }
        });
    }
    return ret;
}

Box<Device> Device::CreateGraphicsDevice(
    Window     &window,
    RHI::Format swapchainFormat,
    int         swapchainImageCount,
    bool        debugMode,
    bool        vsync,
    Flags       flags)
{
    return CreateGraphicsDevice(
        window, RHI::BackendType::Default, swapchainFormat, swapchainImageCount, debugMode, vsync, flags);
}

Device::~Device()
{
    if(mainQueue_.GetRHIObject())
    {
        if(mainQueue_.GetRHIObject()->GetType() == RHI::QueueType::Graphics && window_)
        {
            // Present queue must be properly synchronized
            device_->WaitIdle();
        }
        else
        {
            mainQueue_.GetRHIObject()->WaitIdle();
        }
    }

    if(sync_)
    {
        sync_->PrepareDestruction();
    }

    bindingGroupLayoutCache_.reset();
    copyTextureUtils_.reset();
    clearTextureUtils_.reset();
    clearBufferUtils_.reset();
    accelerationManager_.reset();
    bindingLayoutManager_.reset();
    downloader_.reset();
    uploader_.reset();
    bufferManager_.reset();
    pooledBufferManager_.reset();
    commandBufferManager_.reset();
    dynamicBufferManager_.reset();
    pipelineManager_.reset();
    samplerManager_.reset();
    textureManager_.reset();
    pooledTextureManager_.reset();
    sync_.reset();
}

Box<RG::RenderGraph> Device::CreateRenderGraph()
{
    return MakeBox<RG::RenderGraph>(this, mainQueue_);
}

RC<Texture> Device::CreateColorTexture2D(uint8_t r, uint8_t g, uint8_t b, uint8_t a, std::string name)
{
    auto tex = textureManager_->Create(RHI::TextureDesc
    {
        .dim                  = RHI::TextureDimension::Tex2D,
        .format               = RHI::Format::B8G8R8A8_UNorm,
        .width                = 1,
        .height               = 1,
        .arraySize            = 1,
        .mipLevels            = 1,
        .sampleCount          = 1,
        .usage                = RHI::TextureUsage::TransferDst | RHI::TextureUsage::ShaderResource,
        .initialLayout        = RHI::TextureLayout::Undefined,
        .concurrentAccessMode = RHI::QueueConcurrentAccessMode::Shared
    });
    if(!name.empty())
    {
        tex->SetName(std::move(name));
    }
    const Vector4b color(b, g, r, a);
    Upload(tex, { 0, 0 }, &color, 0, RHI::TextureLayout::ShaderTexture);
    return tex;
}

RHI::BackendType Device::DebugOverrideBackendType(RHI::BackendType type)
{
    //return RHI::BackendType::DirectX12;
    //return RHI::BackendType::Vulkan;
    return type;
}

void Device::InitializeInternal(Flags flags, RHI::DeviceUPtr device, bool isComputeOnly)
{
    flags_ = flags;

    device_ = std::move(device);
    mainQueue_ = Queue(device_->GetQueue(isComputeOnly ? RHI::QueueType::Compute : RHI::QueueType::Graphics));
    sync_ = MakeBox<DeviceSynchronizer>(device_, mainQueue_.GetRHIObject());

    bufferManager_ = MakeBox<BufferManager>(device_, *sync_);
    pooledBufferManager_ = MakeBox<PooledBufferManager>(device_, *sync_);
    dynamicBufferManager_ = MakeBox<DynamicBufferManager>(device_, *sync_);

    textureManager_ = MakeBox<TextureManager>(device_, *sync_);
    pooledTextureManager_ = MakeBox<PooledTextureManager>(device_, *sync_);

    bindingLayoutManager_ = MakeBox<BindingGroupManager>(device_, *sync_, dynamicBufferManager_.get());
    commandBufferManager_ = MakeBox<CommandBufferManager>(this, *sync_);
    pipelineManager_ = MakeBox<PipelineManager>(device_, *sync_);
    samplerManager_ = MakeBox<SamplerManager>(device_, *sync_);

    downloader_ = MakeBox<Downloader>(device_, mainQueue_.GetRHIObject());
    uploader_ = MakeBox<Uploader>(device_, mainQueue_.GetRHIObject());
    accelerationManager_ = MakeBox<AccelerationStructureManager>(device_, *sync_);

    clearBufferUtils_  = MakeBox<ClearBufferUtils>(this);
    clearTextureUtils_ = MakeBox<ClearTextureUtils>(this);
    copyTextureUtils_  = MakeBox<CopyTextureUtils>(this);

    bindingGroupLayoutCache_ = MakeBox<BindingGroupLayoutCache>(this);
}

RC<Buffer> Device::CreateAndUploadBuffer(
    const RHI::BufferDesc    &_desc,
    const void               *initData,
    size_t                    initDataOffset,
    size_t                    initDataSize)
{
    RHI::BufferDesc desc = _desc;
    desc.usage |= RHI::BufferUsage::TransferDst;
    auto ret = CreateBuffer(desc);
    this->Upload(ret, initData, initDataOffset, initDataSize);
    return ret;
}

RC<Texture> Device::CreateAndUploadTexture2D(
    const RHI::TextureDesc &_desc,
    Span<const void *>      imageData,
    RHI::TextureLayout      afterLayout)
{
    RHI::TextureDesc desc = _desc;
    desc.usage |= RHI::TextureUsage::TransferDst;
    assert(imageData.size() == desc.mipLevels * desc.arraySize);
    auto ret = CreateTexture(desc);
    for(uint32_t a = 0, i = 0; a < desc.arraySize; ++a)
    {
        for(uint32_t m = 0; m < desc.mipLevels; ++m)
        {
            Upload(ret, { m, a }, imageData[i++], 0, afterLayout);
        }
    }
    return ret;
}

RC<Texture> Device::CreateAndUploadTexture2D(
    const RHI::TextureDesc &_desc,
    Span<ImageDynamic>      images,
    RHI::TextureLayout      afterLayout)
{
    RHI::TextureDesc desc = _desc;
    desc.usage |= RHI::TextureUsage::TransferDst;
    assert(images.size() == desc.mipLevels * desc.arraySize);
    auto ret = CreateTexture(desc);
    for(uint32_t a = 0, i = 0; a < desc.arraySize; ++a)
    {
        for(uint32_t m = 0; m < desc.mipLevels; ++m)
        {
            Upload(ret, { m, a }, images[i++], afterLayout);
        }
    }
    return ret;
}

RC<Texture> Device::LoadTexture2D(
    const std::string    &filename,
    RHI::Format           format,
    RHI::TextureUsageFlags usages,
    bool                  generateMipLevels,
    RHI::TextureLayout    postLayout)
{
    std::vector<ImageDynamic> images;
    images.push_back(ImageDynamic::Load(filename));
    uint32_t mipLevels = 1;
    if(generateMipLevels)
    {
        mipLevels = ComputeFullMipmapChainSize(images[0].GetWidth(), images[0].GetHeight());
        images.reserve(mipLevels);
        for(uint32_t i = 1; i < mipLevels; ++i)
        {
            images.push_back(GenerateNextImageMipmapLevel(images.back()));
        }
    }
    const RHI::TextureDesc desc =
    {
        .dim                  = RHI::TextureDimension::Tex2D,
        .format               = format,
        .width                = images[0].GetWidth(),
        .height               = images[0].GetHeight(),
        .arraySize            = 1,
        .mipLevels            = mipLevels,
        .sampleCount          = 1,
        .usage                = usages,
        .initialLayout        = RHI::TextureLayout::Undefined,
        .concurrentAccessMode = RHI::QueueConcurrentAccessMode::Exclusive
    };
    return CreateAndUploadTexture2D(desc, images, postLayout);
}

RTRC_END
