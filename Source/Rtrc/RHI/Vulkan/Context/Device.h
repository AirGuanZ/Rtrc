#pragma once

#include <Rtrc/RHI/Vulkan/Context/PhysicalDevice.h>

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
        const QueueFamilyInfo &queueFamilyInfo);

    ~VulkanDevice() override;

    Ptr<Queue> GetQueue(QueueType type) override;

    Ptr<Fence> CreateFence(bool signaled) override;

    Ptr<Swapchain> CreateSwapchain(const SwapchainDesc &desc, Window &window) override;

    Ptr<RawShader> CreateShader(const void *data, size_t size, std::string entryPoint, ShaderStage type) override;

    Ptr<GraphicsPipelineBuilder> CreateGraphicsPipelineBuilder() override;

    Ptr<ComputePipelineBuilder> CreateComputePipelineBuilder() override;

    Ptr<BindingGroupLayout> CreateBindingGroupLayout(const BindingGroupLayoutDesc *desc) override;

    Ptr<BindingLayout> CreateBindingLayout(const BindingLayoutDesc &desc) override;

    Ptr<Texture> CreateTexture2D(const Texture2DDesc &desc) override;

    Ptr<Buffer> CreateBuffer(const BufferDesc &desc) override;

    Ptr<Sampler> CreateSampler(const SamplerDesc &desc) override;

    void WaitIdle() override;

private:

    VkInstance           instance_;
    VulkanPhysicalDevice physicalDevice_;
    VkDevice             device_;

    VkQueue graphicsQueue_;
    VkQueue computeQueue_;
    VkQueue transferQueue_;
    VkQueue presentQueue_;

    QueueFamilyInfo queueFamilies_;

    VmaAllocator allocator_;
};

RTRC_RHI_VK_END
