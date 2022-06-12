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

    RC<Queue> GetQueue(QueueType type) override;

    RC<Fence> CreateFence(bool signaled) override;

    RC<Swapchain> CreateSwapchain(const SwapchainDesc &desc, Window &window) override;

    RC<RawShader> CreateShader(const void *data, size_t size, std::string entryPoint, ShaderStage type) override;

    RC<PipelineBuilder> CreatePipelineBuilder() override;

    RC<BindingGroupLayout> CreateBindingGroupLayout(const BindingGroupLayoutDesc *desc) override;

    RC<BindingLayout> CreateBindingLayout(const BindingLayoutDesc &desc) override;

    RC<Texture> CreateTexture2D(const Texture2DDesc &desc) override;

    RC<Buffer> CreateBuffer(const BufferDesc &desc) override;

    RC<Sampler> CreateSampler(const SamplerDesc &desc) override;

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
