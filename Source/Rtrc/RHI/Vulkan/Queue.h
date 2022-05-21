#pragma once

#include <Rtrc/RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

class VulkanQueue : public Queue
{
public:

    VulkanQueue(VkQueue queue, uint32_t queueFamilyIndex);

    bool IsInSameFamily(const Queue &other) const override;

private:

    VkQueue queue_;
    uint32_t queueFamilyIndex_;
};

RTRC_RHI_VK_END
