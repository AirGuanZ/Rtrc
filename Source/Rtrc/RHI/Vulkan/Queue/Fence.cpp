#include <Rtrc/RHI/Vulkan/Queue/Fence.h>

RTRC_RHI_VK_BEGIN

VulkanFence::VulkanFence(VkDevice device, VkFence fence)
    : device_(device), fence_(fence), syncSessionID_(0), syncSessionIDRecevier_(nullptr)
{
    
}

VulkanFence::~VulkanFence()
{
    vkDestroyFence(device_, fence_, RTRC_VK_ALLOC);
}

void VulkanFence::Reset()
{
    RTRC_VK_FAIL_MSG(
        vkResetFences(device_, 1, &fence_),
        "Failed to reset vulkan fence");
    syncSessionID_ = 0;
    syncSessionIDRecevier_ = nullptr;
}

void VulkanFence::Wait()
{
    vkWaitForFences(device_, 1, &fence_, true, UINT64_MAX);
    if(syncSessionIDRecevier_)
    {
        auto &maxv = *syncSessionIDRecevier_;
        uint64_t prevValue = maxv;
        while(prevValue < syncSessionID_ && !maxv.compare_exchange_weak(prevValue, syncSessionID_))
        {

        }
        syncSessionID_ = 0;
        syncSessionIDRecevier_ = nullptr;
    }
}

VkFence VulkanFence::_internalGetNativeFence() const
{
    return fence_;
}

RTRC_RHI_VK_END
