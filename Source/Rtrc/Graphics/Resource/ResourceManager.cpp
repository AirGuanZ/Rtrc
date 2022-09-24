#include <Rtrc/Graphics/Resource/ResourceManager.h>

RTRC_BEGIN

namespace
{

    using namespace RHI;

    class DeviceWithFrameResourceProtection final : public Device
    {
    public:

        // We don't need to protect resource views(RTV, DSV, SRV, UAV)
        //  since they are internally referenced by resource objects.

        DeviceWithFrameResourceProtection(DevicePtr underlyingDevice, ResourceManager *frameResourceManager)
            : underlyingDevice_(std::move(underlyingDevice)), frameResourceManager_(frameResourceManager)
        {

        }

        Ptr<Queue> GetQueue(QueueType type) override
        {
            return underlyingDevice_->GetQueue(type);
        }

        Ptr<CommandPool> CreateCommandPool(const Ptr<Queue> &queue) override
        {
            auto ret = underlyingDevice_->CreateCommandPool(queue);
            frameResourceManager_->RegisterFrameResourceProtection(ret);
            return ret;
        }

        Ptr<Swapchain> CreateSwapchain(const SwapchainDesc &desc, Window &window) override
        {
            auto ret = underlyingDevice_->CreateSwapchain(desc, window);
            frameResourceManager_->RegisterFrameResourceProtection(ret);
            return ret;
        }

        Ptr<Fence> CreateFence(bool signaled) override
        {
            auto ret = underlyingDevice_->CreateFence(signaled);
            frameResourceManager_->RegisterFrameResourceProtection(ret);
            return ret;
        }

        Ptr<Semaphore> CreateSemaphore(uint64_t initialValue) override
        {
            auto ret = underlyingDevice_->CreateSemaphore(initialValue);
            frameResourceManager_->RegisterFrameResourceProtection(ret);
            return ret;
        }

        Ptr<RawShader> CreateShader(
            const void *data, size_t size, std::string entryPoint, ShaderStage type) override
        {
            // Raw shader objects are not directly used by GPU
            return underlyingDevice_->CreateShader(data, size, std::move(entryPoint), type);
        }

        Ptr<GraphicsPipeline> CreateGraphicsPipeline(const GraphicsPipelineDesc &desc) override
        {
            auto ret = underlyingDevice_->CreateGraphicsPipeline(desc);
            frameResourceManager_->RegisterFrameResourceProtection(ret);
            return ret;
        }

        Ptr<ComputePipeline> CreateComputePipeline(const ComputePipelineDesc &desc) override
        {
            auto ret = underlyingDevice_->CreateComputePipeline(desc);
            frameResourceManager_->RegisterFrameResourceProtection(ret);
            return ret;
        }

        Ptr<BindingGroupLayout> CreateBindingGroupLayout(const BindingGroupLayoutDesc &desc) override
        {
            auto ret = underlyingDevice_->CreateBindingGroupLayout(desc);
            frameResourceManager_->RegisterFrameResourceProtection(ret);
            return ret;
        }

        Ptr<BindingGroup> CreateBindingGroup(
            const Ptr<BindingGroupLayout> &bindingGroupLayout) override
        {
            auto ret = underlyingDevice_->CreateBindingGroup(bindingGroupLayout);
            frameResourceManager_->RegisterFrameResourceProtection(ret);
            return ret;
        }

        Ptr<BindingLayout> CreateBindingLayout(const BindingLayoutDesc &desc) override
        {
            auto ret = underlyingDevice_->CreateBindingLayout(desc);
            frameResourceManager_->RegisterFrameResourceProtection(ret);
            return ret;
        }

        Ptr<Texture> CreateTexture(const TextureDesc &desc) override
        {
            auto ret = underlyingDevice_->CreateTexture(desc);
            frameResourceManager_->RegisterFrameResourceProtection(ret);
            return ret;
        }

        Ptr<Buffer> CreateBuffer(const BufferDesc &desc) override
        {
            auto ret = underlyingDevice_->CreateBuffer(desc);
            frameResourceManager_->RegisterFrameResourceProtection(ret);
            return ret;
        }

        Ptr<Sampler> CreateSampler(const SamplerDesc &desc) override
        {
            auto ret = underlyingDevice_->CreateSampler(desc);
            frameResourceManager_->RegisterFrameResourceProtection(ret);
            return ret;
        }

        Ptr<MemoryPropertyRequirements> GetMemoryRequirements(
            const BufferDesc &desc, size_t *size, size_t *alignment) const override
        {
            return underlyingDevice_->GetMemoryRequirements(desc, size, alignment);
        }

        Ptr<MemoryPropertyRequirements> GetMemoryRequirements(
            const TextureDesc &desc, size_t *size, size_t *alignment) const override
        {
            return underlyingDevice_->GetMemoryRequirements(desc, size, alignment);
        }

        Ptr<MemoryBlock> CreateMemoryBlock(const MemoryBlockDesc &desc) override
        {
            auto ret = underlyingDevice_->CreateMemoryBlock(desc);
            frameResourceManager_->RegisterFrameResourceProtection(ret);
            return ret;
        }

        Ptr<Texture> CreatePlacedTexture(
            const TextureDesc &desc, const Ptr<MemoryBlock> &memoryBlock, size_t offsetInMemoryBlock) override
        {
            auto ret = underlyingDevice_->CreatePlacedTexture(desc, memoryBlock, offsetInMemoryBlock);
            frameResourceManager_->RegisterFrameResourceProtection(ret);
            return ret;
        }

        Ptr<Buffer> CreatePlacedBuffer(
            const BufferDesc &desc, const Ptr<MemoryBlock> &memoryBlock, size_t offsetInMemoryBlock) override
        {
            auto ret = underlyingDevice_->CreatePlacedBuffer(desc, memoryBlock, offsetInMemoryBlock);
            frameResourceManager_->RegisterFrameResourceProtection(ret);
            return ret;
        }

        size_t GetConstantBufferAlignment() const override
        {
            return underlyingDevice_->GetConstantBufferAlignment();
        }

        void WaitIdle() override
        {
            underlyingDevice_->WaitIdle();
        }

    private:

        DevicePtr underlyingDevice_;
        ResourceManager *frameResourceManager_;
    };

} // namespace anonymous

ResourceManager::ResourceManager(DevicePtr device, int frameCount)
    : device_(device),
      synchronizer_(device, frameCount),
      commandBufferManager_(device, frameCount),
      frameConstantBufferManager_(device, frameCount),
      persistentConstantBufferManager_(device, frameCount)
{
    transientResourceManager_ = MakeRC<RG::TransientResourceManager>(
        device, RG::TransientResourceManager::Strategy::ReuseAcrossFrame, frameCount);

    deviceWithFrameResourceProtection_ = MakePtr<DeviceWithFrameResourceProtection>(device_, this);
}

ResourceManager::~ResourceManager()
{
    device_->WaitIdle();
}

const DevicePtr &ResourceManager::GetDeviceWithFrameResourceProtection() const
{
    return deviceWithFrameResourceProtection_;
}

void ResourceManager::BeginFrame()
{
    std::vector<RHIObjectPtr> removedPtrs;
    {
        // IMPROVE: optimize
        std::lock_guard lock(resourcesMutex_);
        for (auto it = resources_.begin(); it != resources_.end();)
        {
            if (it->GetCounter() == 1)
            {
                // (*it) is 'released' by user in last frame. Delay its release to the end of the frame
                removedPtrs.push_back(*it);
                it = resources_.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
    if(!removedPtrs.empty())
    {
        synchronizer_.OnGPUFrameEnd([ptrs = std::move(removedPtrs)]() mutable { ptrs.clear(); });
    }

    synchronizer_.BeginFrame();
    commandBufferManager_.BeginFrame(synchronizer_.GetFrameIndex());
    frameConstantBufferManager_.BeginFrame();
    transientResourceManager_->BeginFrame();
}

int ResourceManager::GetFrameIndex() const
{
    return synchronizer_.GetFrameIndex();
}

FencePtr ResourceManager::GetFrameFence() const
{
    return synchronizer_.GetFrameFence();
}

CommandBufferPtr ResourceManager::AllocateCommandBuffer(QueueType type)
{
    return commandBufferManager_.AllocateCommandBuffer(type);
}

FrameConstantBuffer ResourceManager::AllocateFrameConstantBuffer(size_t size)
{
    return frameConstantBufferManager_.AllocateConstantBuffer(size);
}

PersistentConstantBuffer ResourceManager::AllocatePersistentConstantBuffer(size_t size)
{
    return persistentConstantBufferManager_.AllocateConstantBuffer(size);
}

void ResourceManager::OnGPUFrameEnd(std::function<void()> func)
{
    synchronizer_.OnGPUFrameEnd(std::move(func));
}

RG::TransientResourceManager *ResourceManager::GetTransicentResourceManager() const
{
    return transientResourceManager_.get();
}

void ResourceManager::RegisterFrameResourceProtection(RHIObjectPtr object)
{
    std::lock_guard lock(resourcesMutex_);
    resources_.insert(std::move(object));
}

RTRC_END
