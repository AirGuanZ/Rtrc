#include <Rtrc/RHI/Vulkan/Context/Surface.h>

RTRC_RHI_VK_BEGIN

VulkanSurface::VulkanSurface(VkInstance instance, VkSurfaceKHR surface)
    : instance_(instance), surface_(surface)
{
    
}

VulkanSurface::~VulkanSurface()
{
    assert(instance_ && surface_);
    vkDestroySurfaceKHR(instance_, surface_, RTRC_VK_ALLOC);
}

VkSurfaceKHR VulkanSurface::_internalGetSurface() const
{
    return surface_;
}

RTRC_RHI_VK_END
