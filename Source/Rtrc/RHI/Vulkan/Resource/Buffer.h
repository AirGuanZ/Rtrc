#pragma once

#include <Rtrc/RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

class VulkanBuffer : public Buffer
{
public:

    VulkanBuffer(
        const BufferDesc      &desc,
        VkDevice               device,
        VkBuffer               buffer,
        VulkanMemoryAllocation alloc,
        ResourceOwnership      ownership);

    ~VulkanBuffer() override;

    VkBuffer GetNativeBuffer() const;

private:

    BufferDesc             desc_;
    VkDevice               device_;
    VkBuffer               buffer_;
    VulkanMemoryAllocation alloc_;
    ResourceOwnership      ownership_;
};

RTRC_RHI_VK_END
