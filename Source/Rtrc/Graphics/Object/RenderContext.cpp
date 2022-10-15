#include <Rtrc/Graphics/Object/RenderContext.h>

RTRC_BEGIN

RenderContext::RenderContext(RHI::DevicePtr device, bool isComputeOnly)
    : device_(device),
      hostSync_(device, device->GetQueue(isComputeOnly ? RHI::QueueType::Compute : RHI::QueueType::Graphics))
{
    generalGPUObjectManager_ = MakeBox<GeneralGPUObjectManager>(hostSync_, device);

    bindingLayoutManager_ = MakeBox<BindingLayoutManager>(hostSync_, device);
    bufferManager_ = MakeBox<BufferManager>(hostSync_, device);
    commandBufferManager_ = MakeBox<CommandBufferManager>(hostSync_, device);
    constantBufferManager_ = MakeBox<ConstantBufferManager>(hostSync_, device);
    pipelineManager_ = MakeBox<PipelineManager>(*generalGPUObjectManager_);
    samplerManager_ = MakeBox<SamplerManager>(hostSync_, device);
    textureManager_ = MakeBox<TextureManager>(hostSync_, device);

    stop_token_ = stop_source_.get_token();
    GCThread_ = std::jthread(&RenderContext::GCThreadFunc, this);
}

RenderContext::RenderContext(
    RHI::DevicePtr device, Window *window, RHI::Format swapchainFormat, int swapchainImageCount)
    : RenderContext(std::move(device), false)
{
    window_ = window;
    swapchainFormat_ = swapchainFormat;
    swapchainImageCount_ = swapchainImageCount;
    RecreateSwapchain();
    window_->Attach([&](const WindowResizeEvent &e)
    {
        if(e.width > 0 && e.height > 0)
        {
            device_->WaitIdle();
            RecreateSwapchain();
        }
    });
}

RenderContext::~RenderContext()
{
    hostSync_.End();

    stop_source_.request_stop();
    GCThread_.join();

    bindingLayoutManager_.reset();
    bufferManager_.reset();
    commandBufferManager_.reset();
    constantBufferManager_.reset();
    pipelineManager_.reset();
    samplerManager_.reset();
    textureManager_.reset();

    generalGPUObjectManager_.reset();
}

void RenderContext::SetGCInterval(std::chrono::milliseconds ms)
{
    GCInterval_ = ms;
}

const RHI::QueuePtr &RenderContext::GetMainQueue() const
{
    return hostSync_.GetQueue();
}

const RHI::SwapchainPtr &RenderContext::GetSwapchain() const
{
    return swapchain_;
}

const RHI::TextureDesc &RenderContext::GetRenderTargetDesc() const
{
    return swapchain_->GetRenderTargetDesc();
}

void RenderContext::BeginRenderLoop()
{
    assert(swapchainImageCount_ > 0);
    hostSync_.BeginRenderLoop(swapchainImageCount_);
}

void RenderContext::EndRenderLoop()
{
    hostSync_.EndRenderLoop();
}

void RenderContext::Present()
{
    swapchain_->Present();
}

bool RenderContext::BeginFrame()
{
    if(swapchain_)
    {
        window_->DoEvents();
        if(!window_->HasFocus() || window_->GetFramebufferSize().x <= 0 || window_->GetFramebufferSize().y <= 0)
        {
            return false;
        }
    }
    hostSync_.WaitForOldFrame();
    if(swapchain_ && !swapchain_->Acquire())
    {
        return false;
    }
    hostSync_.BeginNewFrame();
    return true;
}

void RenderContext::WaitIdle()
{
    hostSync_.WaitIdle();
}

const RHI::FencePtr &RenderContext::GetFrameFence()
{
    return hostSync_.GetFrameFence();
}

RC<BindingGroupLayout> RenderContext::CreateBindingGroupLayout(const RHI::BindingGroupLayoutDesc &desc)
{
    return bindingLayoutManager_->CreateBindingGroupLayout(desc);
}

RC<BindingLayout> RenderContext::CreateBindingLayout(const BindingLayout::Desc &desc)
{
    return bindingLayoutManager_->CreateBindingLayout(desc);
}

RC<Buffer> RenderContext::CreateBuffer(
    size_t size, RHI::BufferUsageFlag usages, RHI::BufferHostAccessType hostAccess, bool allowReuse)
{
    return bufferManager_->CreateBuffer(size, usages, hostAccess, allowReuse);
}

RC<Texture2D> RenderContext::CreateTexture2D(
    uint32_t width, uint32_t height, uint32_t arraySize, uint32_t mipLevelCount, RHI::Format format,
    RHI::TextureUsageFlag usages, RHI::QueueConcurrentAccessMode concurrentMode, uint32_t sampleCount, bool allowReuse)
{
    return textureManager_->CreateTexture2D(
        width, height, arraySize, mipLevelCount, format, usages, concurrentMode, sampleCount, allowReuse);
}

RC<ConstantBuffer> RenderContext::CreateConstantBuffer()
{
    return constantBufferManager_->Create();
}

CommandBuffer RenderContext::CreateCommandBuffer()
{
    return commandBufferManager_->Create();
}

RC<GraphicsPipeline> RenderContext::CreateGraphicsPipeline(const GraphicsPipeline::Desc &desc)
{
    return pipelineManager_->CreateGraphicsPipeline(desc);
}

RC<Sampler> RenderContext::CreateSampler(const RHI::SamplerDesc &desc)
{
    return samplerManager_->CreateSampler(desc);
}

const RHI::DevicePtr &RenderContext::GetRawDevice() const
{
    return device_;
}

void RenderContext::RecreateSwapchain()
{
    assert(window_);
    swapchain_.Reset();
    swapchain_ = device_->CreateSwapchain(RHI::SwapchainDesc
    {
        .format = swapchainFormat_,
        .imageCount = static_cast<uint32_t>(swapchainImageCount_)
    }, *window_);
}

void RenderContext::GCThreadFunc()
{
    while(!stop_token_.stop_requested())
    {
        bufferManager_->GC();
        textureManager_->GC();
        std::this_thread::sleep_for(GCInterval_);
    }
}

RTRC_END
