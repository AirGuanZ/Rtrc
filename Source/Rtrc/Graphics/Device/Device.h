#pragma once

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

class Device : public Uncopyable
{
public:

    static Box<Device> CreateComputeDevice(RHI::DevicePtr rhiDevice);

    static Box<Device> CreateComputeDevice(bool debugMode = RTRC_DEBUG);

    static Box<Device> CreateGraphicsDevice(
        RHI::DevicePtr rhiDevice,
        Window        &window,
        RHI::Format    swapchainFormat = RHI::Format::B8G8R8A8_UNorm,
        int            swapchainImageCount = 3);

    static Box<Device> CreateGraphicsDevice(
        Window     &window,
        RHI::Format swapchainFormat     = RHI::Format::B8G8R8A8_UNorm,
        int         swapchainImageCount = 3,
        bool        debugMode           = RTRC_DEBUG);

    ~Device();

    Queue &GetQueue();

    const RHI::SwapchainPtr &GetSwapchain() const;
    const RHI::TextureDesc &GetSwapchainImageDesc() const;

    void ExecuteAndWait(CommandBuffer commandBuffer);
    template<typename F> requires !std::is_same_v<std::remove_cvref_t<F>, CommandBuffer>
    void ExecuteAndWait(F &&f);

    CopyContext &GetCopyContext();

    void WaitIdle();

    void BeginRenderLoop();
    void EndRenderLoop();
    void Present();

    bool BeginFrame();
    const RHI::FencePtr &GetFrameFence();

    Box<RG::RenderGraph> CreateRenderGraph();

    template<BindingGroupDSL::RtrcGroupStruct T>
    RC<BindingGroupLayout> CreateBindingGroupLayout();
    RC<BindingGroupLayout> CreateBindingGroupLayout(const RHI::BindingGroupLayoutDesc &desc);

    template<BindingGroupDSL::RtrcGroupStruct T>
    RC<BindingGroup> CreateBindingGroup();
    template<BindingGroupDSL::RtrcGroupStruct T>
    RC<BindingGroup> CreateBindingGroup(const RC<BindingGroupLayout> &layoutHint);
    template<BindingGroupDSL::RtrcGroupStruct T>
    RC<BindingGroup> CreateBindingGroup(const T &value);
    template<BindingGroupDSL::RtrcGroupStruct T>
    RC<BindingGroup> CreateBindingGroup(const T &value, const RC<BindingGroupLayout> &layoutHint);

    RC<BindingLayout> CreateBindingLayout(const BindingLayout::Desc &desc);

    RC<Buffer> CreateBuffer(const RHI::BufferDesc &desc);
    RC<StatefulBuffer> CreatePooledBuffer(const RHI::BufferDesc &desc);

    RC<Texture> CreateTexture(const RHI::TextureDesc &desc);
    RC<StatefulTexture> CreatePooledTexture(const RHI::TextureDesc &desc);

    RC<DynamicBuffer> CreateDynamicBuffer();
    RC<SubBuffer> CreateConstantBuffer(const void *data, size_t bytes);
    template<RtrcStruct T>
    RC<SubBuffer> CreateConstantBuffer(const T &data);

    CommandBuffer CreateCommandBuffer();

    RC<GraphicsPipeline> CreateGraphicsPipeline(const GraphicsPipeline::Desc &desc);
    RC<ComputePipeline> CreateComputePipeline(const RC<Shader> &shader);

    RC<Sampler> CreateSampler(const RHI::SamplerDesc &desc);

    BufferManager &GetBufferManager();
    BindingGroupManager &GetBindingGroupManager();
    const RHI::DevicePtr &GetRawDevice() const;

private:

    Device() = default;
    
    void InitializeInternal(RHI::DevicePtr device, bool isComputeOnly);

    void RecreateSwapchain();

    RHI::InstancePtr instance_;
    RHI::DevicePtr device_;
    Queue mainQueue_;

    Window           *window_              = nullptr;
    RHI::Format       swapchainFormat_     = RHI::Format::Unknown;
    int               swapchainImageCount_ = 0;
    RHI::SwapchainPtr swapchain_;

    Box<DeviceSynchronizer> sync_;

    Box<BindingGroupManager>  bindingLayoutManager_;
    Box<BufferManager>        bufferManager_;
    Box<PooledBufferManager>  pooledBufferManager_;
    Box<CommandBufferManager> commandBufferManager_;
    Box<DynamicBufferManager> dynamicBufferManager_;
    Box<PipelineManager>      pipelineManager_;
    Box<SamplerManager>       samplerManager_;
    Box<TextureManager>       textureManager_;
    Box<PooledTextureManager> pooledTextureManager_;
    Box<CopyContext>          copyContext_;
};

inline Queue &Device::GetQueue()
{
    return mainQueue_;
}

inline const RHI::SwapchainPtr &Device::GetSwapchain() const
{
    return swapchain_;
}

inline const RHI::TextureDesc &Device::GetSwapchainImageDesc() const
{
    return swapchain_->GetRenderTargetDesc();
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

inline CopyContext &Device::GetCopyContext()
{
    return *copyContext_;
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
        window_->DoEvents();
        if(!window_->HasFocus() || window_->GetFramebufferSize().x <= 0 || window_->GetFramebufferSize().y <= 0)
        {
            return false;
        }
    }
    sync_->WaitForOldFrame();
    commandBufferManager_->_internalEndFrame();
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
    return this->CreateBindingGroupLayout(GetRHIBindingGroupLayoutDesc<T>());
}

inline RC<BindingGroupLayout> Device::CreateBindingGroupLayout(const RHI::BindingGroupLayoutDesc &desc)
{
    return bindingLayoutManager_->CreateBindingGroupLayout(desc);
}

inline RC<BindingLayout> Device::CreateBindingLayout(const BindingLayout::Desc &desc)
{
    return bindingLayoutManager_->CreateBindingLayout(desc);
}

template<BindingGroupDSL::RtrcGroupStruct T>
RC<BindingGroup> Device::CreateBindingGroup()
{
    return this->CreateBindingGroupLayout<T>()->CreateBindingGroup();
}

template<BindingGroupDSL::RtrcGroupStruct T>
RC<BindingGroup> Device::CreateBindingGroup(const RC<BindingGroupLayout> &layoutHint)
{
    assert(layoutHint == this->CreateBindingGroupLayout<T>());
    return layoutHint->CreateBindingGroup();
}

template<BindingGroupDSL::RtrcGroupStruct T>
RC<BindingGroup> Device::CreateBindingGroup(const T &value)
{
    auto group = this->CreateBindingGroup<T>();
    Rtrc::ApplyBindingGroup(dynamicBufferManager_.get(), group, value);
    return group;
}

template<BindingGroupDSL::RtrcGroupStruct T>
RC<BindingGroup> Device::CreateBindingGroup(const T &value, const RC<BindingGroupLayout> &layoutHint)
{
    auto group = this->CreateBindingGroup<T>(layoutHint);
    Rtrc::ApplyBindingGroup(dynamicBufferManager_.get(), group, value);
    return group;
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

inline RC<Sampler> Device::CreateSampler(const RHI::SamplerDesc &desc)
{
    return samplerManager_->CreateSampler(desc);
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

RTRC_END