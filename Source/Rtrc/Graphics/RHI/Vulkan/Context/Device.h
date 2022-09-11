#pragma once

#include <Rtrc/Graphics/RHI/Vulkan/Context/PhysicalDevice.h>
#include <Rtrc/Graphics/RHI/Vulkan/Queue/Queue.h>

RTRC_RHI_VK_BEGIN

class VulkanDevice : public Device
{
public:

    struct QueueFamilyInfo
    {
        std::optional<uint32_t> graphicsFamilyIndex;
        std::optional<uint32_t> computeFamilyIndex;
        std::optional<uint32_t> transferFamilyIndex;
    };

    VulkanDevice(
        VkInstance             instance,
        VulkanPhysicalDevice   physicalDevice,
        VkDevice               device,
        const QueueFamilyInfo &queueFamilyInfo,
        bool                   enableDebug);

    ~VulkanDevice() override;

    Ptr<Queue> GetQueue(QueueType type) override;

    Ptr<CommandPool> CreateCommandPool(const Ptr<Queue> &queue) override;

    Ptr<Fence> CreateFence(bool signaled) override;

    Ptr<Swapchain> CreateSwapchain(const SwapchainDesc &desc, Window &window) override;

    Ptr<Semaphore> CreateSemaphore(uint64_t initialValue) override;

    Ptr<RawShader> CreateShader(const void *data, size_t size, std::string entryPoint, ShaderStage type) override;

    Ptr<GraphicsPipeline> CreateGraphicsPipeline(const GraphicsPipelineDesc &desc) override;

    Ptr<ComputePipeline> CreateComputePipeline(const ComputePipelineDesc &desc) override;

    Ptr<BindingGroupLayout> CreateBindingGroupLayout(const BindingGroupLayoutDesc &desc) override;

    Ptr<BindingGroup> CreateBindingGroup(const Ptr<BindingGroupLayout> &bindingGroupLayout) override;

    Ptr<BindingLayout> CreateBindingLayout(const BindingLayoutDesc &desc) override;

    Ptr<Texture> CreateTexture(const TextureDesc &desc) override;

    Ptr<Buffer> CreateBuffer(const BufferDesc &desc) override;

    Ptr<Sampler> CreateSampler(const SamplerDesc &desc) override;

    Ptr<MemoryPropertyRequirements> GetMemoryRequirements(
        const BufferDesc &desc, size_t *size, size_t *alignment) const override;

    Ptr<MemoryPropertyRequirements> GetMemoryRequirements(
        const TextureDesc &desc, size_t *size, size_t *alignment) const override;

    Ptr<MemoryBlock> CreateMemoryBlock(const MemoryBlockDesc &desc) override;

    Ptr<Texture> CreatePlacedTexture(
        const TextureDesc &desc, const Ptr<MemoryBlock> &memoryBlock, size_t offsetInMemoryBlock) override;

    Ptr<Buffer> CreatePlacedBuffer(
        const BufferDesc &desc, const Ptr<MemoryBlock> &memoryBlock, size_t offsetInMemoryBlock) override;

    void WaitIdle() override;

    VkDevice GetNativeDevice();

    void SetObjectName(VkObjectType objectType, uint64_t objectHandle, const char *name);

private:

    bool enableDebug_;

    VkInstance           instance_;
    VulkanPhysicalDevice physicalDevice_;
    VkDevice             device_;

    Ptr<VulkanQueue> graphicsQueue_;
    Ptr<VulkanQueue> computeQueue_;
    Ptr<VulkanQueue> transferQueue_;
    Ptr<VulkanQueue> presentQueue_;

    QueueFamilyInfo queueFamilies_;

    VmaAllocator allocator_;
};

RTRC_RHI_VK_END
