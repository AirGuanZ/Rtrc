#include <mimalloc.h>

#include <Rtrc/RHI/Vulkan/Common.h>
#include <Rtrc/Utils/Unreachable.h>

RTRC_RHI_VK_BEGIN

namespace
{

    void *VkAlloc(void *pUserData, size_t size, size_t alignment, VkSystemAllocationScope scope)
    {
        return mi_aligned_alloc(alignment, (size + alignment - 1) / alignment * alignment);
    }

    void *VkRealloc(void *pUserData, void *orig, size_t size, size_t alignment, VkSystemAllocationScope scope)
    {
        return mi_realloc_aligned(orig, size, alignment);
    }

    void VkFree(void *pUserData, void *ptr)
    {
        mi_free(ptr);
    }

} // namespace anonymous

VkAllocationCallbacks RtrcGlobalVulkanAllocationCallbacks = {
    .pfnAllocation         = VkAlloc,
    .pfnReallocation       = VkRealloc,
    .pfnFree               = VkFree,
    .pfnInternalAllocation = nullptr,
    .pfnInternalFree       = nullptr
};

VkFormat TranslateTexelFormat(TexelFormat format)
{
    switch(format)
    {
    case TexelFormat::B8G8R8A8_UNorm: return VK_FORMAT_B8G8R8A8_UNORM;
    }
    Unreachable();
}

RTRC_RHI_VK_END
