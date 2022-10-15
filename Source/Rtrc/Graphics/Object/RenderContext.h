#pragma once

#include <thread>

#include <Rtrc/Graphics/Object/BindingGroup.h>
#include <Rtrc/Graphics/Object/Buffer.h>
#include <Rtrc/Graphics/Object/ConstantBuffer.h>
#include <Rtrc/Graphics/Object/CopyContext.h>
#include <Rtrc/Graphics/Object/Pipeline.h>
#include <Rtrc/Graphics/Object/Sampler.h>
#include <Rtrc/Graphics/Object/Texture.h>

RTRC_BEGIN

class RenderContext : public Uncopyable
{
public:

    explicit RenderContext(RHI::DevicePtr device, bool isComputeOnly = true);

    RenderContext(
        RHI::DevicePtr device,
        Window        *window,
        RHI::Format    swapchainFormat     = RHI::Format::B8G8R8A8_UNorm,
        int            swapchainImageCount = 3);

    ~RenderContext();

    void SetGCInterval(std::chrono::milliseconds ms);

    const RHI::QueuePtr &GetMainQueue() const;
    const RHI::SwapchainPtr &GetSwapchain() const;
    const RHI::TextureDesc &GetRenderTargetDesc() const;
    
    void BeginRenderLoop();
    void EndRenderLoop();
    void Present();

    bool BeginFrame();
    void WaitIdle();

    CopyContext &GetCopyContext();

    const RHI::FencePtr &GetFrameFence();

    RC<BindingGroupLayout> CreateBindingGroupLayout(const RHI::BindingGroupLayoutDesc &desc);

    RC<BindingLayout> CreateBindingLayout(const BindingLayout::Desc &desc);

    RC<Buffer> CreateBuffer(
        size_t                    size,
        RHI::BufferUsageFlag      usages,
        RHI::BufferHostAccessType hostAccess,
        bool                      allowReuse);

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

    CommandBuffer CreateCommandBuffer();

    RC<GraphicsPipeline> CreateGraphicsPipeline(const GraphicsPipeline::Desc &desc);

    RC<Sampler> CreateSampler(const RHI::SamplerDesc &desc);

    const RHI::DevicePtr &GetRawDevice() const;

private:

    void RecreateSwapchain();

    void GCThreadFunc();

    RHI::DevicePtr device_;

    Window           *window_              = nullptr;
    RHI::Format       swapchainFormat_     = RHI::Format::Unknown;
    int               swapchainImageCount_ = 0;
    RHI::SwapchainPtr swapchain_;

    HostSynchronizer             hostSync_;
    Box<GeneralGPUObjectManager> generalGPUObjectManager_;

    Box<BindingLayoutManager>  bindingLayoutManager_;
    Box<BufferManager>         bufferManager_;
    Box<CommandBufferManager>  commandBufferManager_;
    Box<ConstantBufferManager> constantBufferManager_;
    Box<PipelineManager>       pipelineManager_;
    Box<SamplerManager>        samplerManager_;
    Box<TextureManager>        textureManager_;
    Box<CopyContext>           copyContext_;
    
    std::chrono::milliseconds GCInterval_ = std::chrono::milliseconds(50);
    std::jthread              GCThread_;
    std::stop_source          stop_source_;
    std::stop_token           stop_token_;
};

RTRC_END
