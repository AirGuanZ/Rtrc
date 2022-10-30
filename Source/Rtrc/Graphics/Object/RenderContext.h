#pragma once

#include <thread>

#include <Rtrc/Graphics/Object/BindingGroup.h>
#include <Rtrc/Graphics/Object/BindingGroupDSL.h>
#include <Rtrc/Graphics/Object/Buffer.h>
#include <Rtrc/Graphics/Object/ConstantBuffer.h>
#include <Rtrc/Graphics/Object/CopyContext.h>
#include <Rtrc/Graphics/Object/Queue.h>
#include <Rtrc/Graphics/Object/Pipeline.h>
#include <Rtrc/Graphics/Object/Sampler.h>
#include <Rtrc/Graphics/Object/Texture.h>
#include <Rtrc/Graphics/RenderGraph/Graph.h>

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

    Queue &GetMainQueue();
    const RHI::SwapchainPtr &GetSwapchain() const;
    const RHI::TextureDesc &GetRenderTargetDesc() const;

    void ExecuteAndWait(CommandBuffer commandBuffer);
    template<typename F>
    void ExecuteAndWaitImmediate(F &&f);

    void BeginRenderLoop();
    void EndRenderLoop();
    void Present();

    bool BeginFrame();
    void WaitIdle();

    Box<RG::RenderGraph> CreateRenderGraph();

    CopyContext &GetCopyContext();

    const RHI::FencePtr &GetFrameFence();

    template<BindingGroupDSL::RtrcGroupStruct T>
    RC<BindingGroupLayout> CreateBindingGroupLayout();

    template<BindingGroupDSL::RtrcGroupStruct T>
    RC<BindingGroup> CreateBindingGroup();

    template<BindingGroupDSL::RtrcGroupStruct T>
    RC<BindingGroup> CreateBindingGroup(const RC<BindingGroupLayout> &layoutHint);

    template<BindingGroupDSL::RtrcGroupStruct T>
    RC<BindingGroup> CreateBindingGroup(const T &value);
    
    template<BindingGroupDSL::RtrcGroupStruct T>
    RC<BindingGroup> CreateBindingGroup(const T &value, const RC<BindingGroupLayout> &layoutHint);

    RC<BindingGroupLayout> CreateBindingGroupLayout(const RHI::BindingGroupLayoutDesc &desc);

    RC<BindingLayout> CreateBindingLayout(const BindingLayout::Desc &desc);

    RC<Buffer> CreateBuffer(
        size_t                    size,
        RHI::BufferUsageFlag      usages,
        RHI::BufferHostAccessType hostAccess,
        bool                      allowReuse);

    RC<Texture> CreateTexture2D(
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
    RC<ComputePipeline> CreateComputePipeline(const RC<Shader> &shader);

    RC<Sampler> CreateSampler(const RHI::SamplerDesc &desc);

    const RHI::DevicePtr &GetRawDevice() const;

private:

    void RecreateSwapchain();

    void GCThreadFunc();

    RHI::DevicePtr device_;
    Queue mainQueue_;

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

template<typename F>
void RenderContext::ExecuteAndWaitImmediate(F &&f)
{
    auto cmd = CreateCommandBuffer();
    cmd.Begin();
    std::invoke(std::forward<F>(f), cmd);
    cmd.End();
    ExecuteAndWait(std::move(cmd));
}

template<BindingGroupDSL::RtrcGroupStruct T>
RC<BindingGroupLayout> RenderContext::CreateBindingGroupLayout()
{
    // TODO: fast cache for these group layouts
    return this->CreateBindingGroupLayout(GetRHIBindingGroupLayoutDesc<T>());
}

template<BindingGroupDSL::RtrcGroupStruct T>
RC<BindingGroup> RenderContext::CreateBindingGroup()
{
    return this->CreateBindingGroupLayout<T>()->CreateBindingGroup();
}

template<BindingGroupDSL::RtrcGroupStruct T>
RC<BindingGroup> RenderContext::CreateBindingGroup(const RC<BindingGroupLayout> &layoutHint)
{
    assert(layoutHint == CreateBindingGroupLayout<T>());
    return layoutHint->CreateBindingGroup();
}

template<BindingGroupDSL::RtrcGroupStruct T>
RC<BindingGroup> RenderContext::CreateBindingGroup(const T &value)
{
    auto group = this->CreateBindingGroup<T>();
    Rtrc::ApplyBindingGroup(constantBufferManager_.get(), group, value);
    return group;
}

template<BindingGroupDSL::RtrcGroupStruct T>
RC<BindingGroup> RenderContext::CreateBindingGroup(const T &value, const RC<BindingGroupLayout> &layoutHint)
{
    auto group = this->CreateBindingGroup<T>(layoutHint);
    Rtrc::ApplyBindingGroup(constantBufferManager_.get(), group, value);
    return group;
}

RTRC_END
