#pragma once

#include <Rtrc/Graphics/RHI/Vulkan/Context/PhysicalDevice.h>
#include <Rtrc/Graphics/RHI/Vulkan/Queue/Queue.h>

RTRC_RHI_VK_BEGIN

RTRC_RHI_IMPLEMENT(VulkanDevice, Device)
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

    Ptr<Queue> GetQueue(QueueType type) RTRC_RHI_OVERRIDE;

    Ptr<CommandPool> CreateCommandPool(const Ptr<Queue> &queue) RTRC_RHI_OVERRIDE;

    Ptr<Fence> CreateFence(bool signaled) RTRC_RHI_OVERRIDE;

    Ptr<Swapchain> CreateSwapchain(const SwapchainDesc &desc, Window &window) RTRC_RHI_OVERRIDE;

    Ptr<Semaphore> CreateSemaphore(uint64_t initialValue) RTRC_RHI_OVERRIDE;

    Ptr<RawShader> CreateShader(const void *data, size_t size, std::string entryPoint, ShaderStage type) RTRC_RHI_OVERRIDE;

    Ptr<GraphicsPipeline> CreateGraphicsPipeline(const GraphicsPipelineDesc &desc) RTRC_RHI_OVERRIDE;
    Ptr<ComputePipeline>  CreateComputePipeline(const ComputePipelineDesc &desc) RTRC_RHI_OVERRIDE;

    Ptr<BindingGroupLayout> CreateBindingGroupLayout(const BindingGroupLayoutDesc &desc) RTRC_RHI_OVERRIDE;
    Ptr<BindingGroup> CreateBindingGroup(
        const Ptr<BindingGroupLayout> &bindingGroupLayout, uint32_t variableArraySize = 0) RTRC_RHI_OVERRIDE;
    Ptr<BindingLayout> CreateBindingLayout(const BindingLayoutDesc &desc) RTRC_RHI_OVERRIDE;

    void UpdateBindingGroups(const BindingGroupUpdateBatch &batch) RTRC_RHI_OVERRIDE;

    Ptr<Texture> CreateTexture(const TextureDesc &desc) RTRC_RHI_OVERRIDE;

    Ptr<Buffer> CreateBuffer(const BufferDesc &desc) RTRC_RHI_OVERRIDE;

    Ptr<Sampler> CreateSampler(const SamplerDesc &desc) RTRC_RHI_OVERRIDE;

    Ptr<MemoryPropertyRequirements> GetMemoryRequirements(
        const BufferDesc &desc, size_t *size, size_t *alignment) const RTRC_RHI_OVERRIDE;
    Ptr<MemoryPropertyRequirements> GetMemoryRequirements(
        const TextureDesc &desc, size_t *size, size_t *alignment) const RTRC_RHI_OVERRIDE;

    Ptr<MemoryBlock> CreateMemoryBlock(const MemoryBlockDesc &desc) RTRC_RHI_OVERRIDE;

    Ptr<Texture> CreatePlacedTexture(
        const TextureDesc &desc, const Ptr<MemoryBlock> &memoryBlock, size_t offsetInMemoryBlock) RTRC_RHI_OVERRIDE;
    Ptr<Buffer> CreatePlacedBuffer(
        const BufferDesc &desc, const Ptr<MemoryBlock> &memoryBlock, size_t offsetInMemoryBlock) RTRC_RHI_OVERRIDE;

    size_t GetConstantBufferAlignment() const RTRC_RHI_OVERRIDE;

    void WaitIdle() RTRC_RHI_OVERRIDE;

    void _internalSetObjectName(VkObjectType objectType, uint64_t objectHandle, const char *name);

    uint32_t _internalGetQueueFamilyIndex(QueueType type) const;

    VkDevice _internalGetNativeDevice() const;

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
