#include <Rtrc/Graphics/Object/RenderContext.h>

RTRC_BEGIN

RenderContext::RenderContext(RHI::DevicePtr device, std::chrono::milliseconds GCInterval)
    : hostSync_(device, device->GetQueue(RHI::QueueType::Graphics)),
      bufferManager_(hostSync_, device),
      commandBufferManager_(hostSync_, device),
      constantBufferManager_(hostSync_, device),
      textureManager_(hostSync_, device),
      GCInterval_(GCInterval)
{
    stop_token_ = stop_source_.get_token();
    GCThread_ = std::jthread(&RenderContext::GCThreadFunc, this);
}

RenderContext::~RenderContext()
{
    stop_source_.request_stop();
    GCThread_.join();
}

void RenderContext::BeginRenderLoop(int maxFlightFrameCount)
{
    hostSync_.BeginRenderLoop(maxFlightFrameCount);
}

void RenderContext::EndRenderLoop()
{
    hostSync_.EndRenderLoop();
}

void RenderContext::BeginFrame()
{
    hostSync_.BeginFrame();
}

void RenderContext::WaitIdle()
{
    hostSync_.WaitIdle();
}

RC<Buffer> RenderContext::CreateBuffer(
    size_t size, RHI::BufferUsageFlag usages, RHI::BufferHostAccessType hostAccess, bool allowReuse)
{
    return bufferManager_.CreateBuffer(size, usages, hostAccess, allowReuse);
}

CommandBuffer RenderContext::CreateGraphicsCommandBuffer()
{
    return commandBufferManager_.Create();
}

RC<Texture2D> RenderContext::CreateTexture2D(
    uint32_t width, uint32_t height, uint32_t arraySize, uint32_t mipLevelCount, RHI::Format format,
    RHI::TextureUsageFlag usages, RHI::QueueConcurrentAccessMode concurrentMode, uint32_t sampleCount, bool allowReuse)
{
    return textureManager_.CreateTexture2D(
        width, height, arraySize, mipLevelCount, format, usages, concurrentMode, sampleCount, allowReuse);
}

RC<ConstantBuffer> RenderContext::CreateConstantBuffer()
{
    return constantBufferManager_.Create();
}

void RenderContext::GCThreadFunc()
{
    while(!stop_token_.stop_requested())
    {
        bufferManager_.GC();
        textureManager_.GC();
        std::this_thread::sleep_for(GCInterval_);
    }
}

RTRC_END
