#pragma once

#include <Rtrc/Graphics/RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

class VulkanSurface : public Surface
{
public:

    VulkanSurface(VkInstance instance, VkSurfaceKHR surface);

    ~VulkanSurface() override;

    VkSurfaceKHR GetSurface() const;

private:

    VkInstance   instance_;
    VkSurfaceKHR surface_;
};

RTRC_RHI_VK_END
