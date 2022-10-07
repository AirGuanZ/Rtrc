#pragma once

#include <thread>

#include <Rtrc/Graphics/Object/BufferManager.h>
#include <Rtrc/Graphics/Object/ConstantBuffer.h>
#include <Rtrc/Graphics/Object/TextureManager.h>

RTRC_BEGIN

class RenderContext : public Uncopyable
{
public:

    explicit RenderContext(RHI::DevicePtr device, std::chrono::milliseconds GCInterval = std::chrono::milliseconds(50));
    ~RenderContext();
    
    void BeginRenderLoop(int maxFlightFrameCount);
    void EndRenderLoop();

    void BeginFrame();
    void WaitIdle();

    RC<Buffer> CreateBuffer(
        size_t                    size,
        RHI::BufferUsageFlag      usages,
        RHI::BufferHostAccessType hostAccess,
        bool                      allowReuse);

    CommandBuffer CreateGraphicsCommandBuffer();

    RC<Texture2D> CreateTexture2D(
        uint32_t                       width,
        uint32_t                       height,
        uint32_t                       arraySize,
        uint32_t                       mipLevelCount, // '<= 0' means full mipmap chain
        RHI::Format                    format,
        RHI::TextureUsageFlag          usages,
        RHI::QueueConcurrentAccessMode concurrentMode,
        uint32_t                       sampleCount,
        bool                           allowReuse);

    // Using of constant buffers must be synchronized with BeginRenderLoop/BeginFrame/...
    RC<ConstantBuffer> CreateConstantBuffer();

private:

    void GCThreadFunc();

    HostSynchronizer      hostSync_;
    BufferManager         bufferManager_;
    CommandBufferManager  commandBufferManager_;
    ConstantBufferManager constantBufferManager_;
    TextureManager        textureManager_;

    std::chrono::milliseconds GCInterval_;
    std::jthread GCThread_;
    std::stop_source stop_source_;
    std::stop_token stop_token_;
};

RTRC_END
