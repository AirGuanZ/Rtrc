#include <Rtrc/Graphics/Device/Device.h>

RTRC_BEGIN

Box<Device> Device::CreateComputeDevice(RHI::DevicePtr rhiDevice)
{
    Box<Device> ret{ new Device };
    ret->InitializeInternal(std::move(rhiDevice), true);
    return ret;
}

Box<Device> Device::CreateComputeDevice(bool debugMode)
{
    Box<Device> ret{ new Device };
    ret->instance_ = RHI::CreateVulkanInstance({ .debugMode = debugMode });
    const RHI::DeviceDesc deviceDesc =
    {
        .graphicsQueue = false,
        .computeQueue = true,
        .transferQueue = true,
        .supportSwapchain = false
    };
    auto rhiDevice = ret->instance_->CreateDevice(deviceDesc);
    ret->InitializeInternal(rhiDevice, true);
    return ret;
}

Box<Device> Device::CreateGraphicsDevice(
    RHI::DevicePtr rhiDevice,
    Window        &window,
    RHI::Format    swapchainFormat,
    int            swapchainImageCount,
    bool           vsync)
{
    Box<Device> ret{ new Device };
    ret->InitializeInternal(std::move(rhiDevice), false);
    ret->window_ = &window;
    ret->swapchainFormat_ = swapchainFormat;
    ret->swapchainImageCount_ = swapchainImageCount;
    ret->vsync_ = vsync;
    ret->RecreateSwapchain();
    ret->window_->Attach([d = ret.get()](const WindowResizeEvent &e)
    {
        if(e.width > 0 && e.height > 0)
        {
            d->device_->WaitIdle();
            d->RecreateSwapchain();
        }
    });
    return ret;
}

Box<Device> Device::CreateGraphicsDevice(
    Window     &window,
    RHI::Format swapchainFormat,
    int         swapchainImageCount,
    bool        debugMode,
    bool        vsync)
{
    Box<Device> ret{ new Device };
    ret->instance_ = CreateVulkanInstance(RHI::VulkanInstanceDesc
    {
            .extensions = Window::GetRequiredVulkanInstanceExtensions(),
            .debugMode = debugMode
    });
    const RHI::DeviceDesc deviceDesc =
    {
        .graphicsQueue = true,
        .computeQueue = true,
        .transferQueue = true,
        .supportSwapchain = true
    };
    auto rhiDevice = ret->instance_->CreateDevice(deviceDesc);
    ret->InitializeInternal(rhiDevice, false);
    ret->window_ = &window;
    ret->swapchainFormat_ = swapchainFormat;
    ret->swapchainImageCount_ = swapchainImageCount;
    ret->vsync_ = vsync;
    ret->RecreateSwapchain();
    ret->window_->Attach([d = ret.get()](const WindowResizeEvent &e)
    {
        if(e.width > 0 && e.height > 0)
        {
            d->device_->WaitIdle();
            d->RecreateSwapchain();
        }
    });
    return ret;
}

Device::~Device()
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

    sync_->WaitIdle();

    bindingLayoutManager_.reset();
    copyContext_.reset();
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

RC<Texture> Device::CreateColorTexture2D(uint8_t r, uint8_t g, uint8_t b, uint8_t a, const std::string &name)
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
        .concurrentAccessMode = RHI::QueueConcurrentAccessMode::Concurrent
    });
    if(!name.empty())
    {
        tex->SetName(name);
    }
    const Vector4b color(b, g, r, a);
    copyContext_->UploadTexture2D(tex, 0, 0, &color);
    ExecuteBarrier(
        tex,
        RHI::TextureLayout::CopyDst, RHI::PipelineStage::None, RHI::ResourceAccess::None,
        RHI::TextureLayout::ShaderTexture, RHI::PipelineStage::None, RHI::ResourceAccess::None);
    return tex;
}

void Device::InitializeInternal(RHI::DevicePtr device, bool isComputeOnly)
{
    device_ = std::move(device);
    mainQueue_ = Queue(device_->GetQueue(isComputeOnly ? RHI::QueueType::Compute : RHI::QueueType::Graphics));
    sync_ = MakeBox<DeviceSynchronizer>(device_, mainQueue_.GetRHIObject());

    bufferManager_ = MakeBox<BufferManager>(device_, *sync_);
    pooledBufferManager_ = MakeBox<PooledBufferManager>(device_, *sync_);
    dynamicBufferManager_ = MakeBox<DynamicBufferManager>(device_, *sync_);

    textureManager_ = MakeBox<TextureManager>(device_, *sync_);
    pooledTextureManager_ = MakeBox<PooledTextureManager>(device_, *sync_);

    bindingLayoutManager_ = MakeBox<BindingGroupManager>(device_, *sync_, dynamicBufferManager_.get());
    commandBufferManager_ = MakeBox<CommandBufferManager>(device_, *sync_);
    pipelineManager_ = MakeBox<PipelineManager>(device_, *sync_);
    samplerManager_ = MakeBox<SamplerManager>(device_, *sync_);

    copyContext_ = MakeBox<CopyContext>(device_, bufferManager_.get(), textureManager_.get());
}

void Device::RecreateSwapchain()
{
    assert(window_);
    swapchain_.Reset();
    swapchain_ = device_->CreateSwapchain(RHI::SwapchainDesc
    {
        .format     = swapchainFormat_,
        .imageCount = static_cast<uint32_t>(swapchainImageCount_),
        .vsync      = vsync_
    }, *window_);
}

RTRC_END
