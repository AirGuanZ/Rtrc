#pragma once

#include <Rtrc/RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

RTRC_RHI_IMPLEMENT(VulkanSurface, Surface)
{
public:

    VulkanSurface(VkInstance instance, VkSurfaceKHR surface);

    ~VulkanSurface() override;

    VkSurfaceKHR _internalGetSurface() const;

private:

    VkInstance   instance_;
    VkSurfaceKHR surface_;
};

RTRC_RHI_VK_END
