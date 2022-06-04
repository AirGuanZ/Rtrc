#include <Rtrc/RHI/Vulkan/Queue/CommandPool.h>
#include <Rtrc/Utils/ScopeGuard.h>

RTRC_RHI_VK_BEGIN

VulkanCommandPool::VulkanCommandPool(VkDevice device, VkCommandPool pool)
    : device_(device), pool_(pool), nextFreeBufferIndex_(0)
{
    
}

VulkanCommandPool::~VulkanCommandPool()
{
    commandBuffers_.clear();
    vkDestroyCommandPool(device_, pool_, VK_ALLOC);
}

void VulkanCommandPool::Reset()
{
    vkResetCommandPool(device_, pool_, 0);
    nextFreeBufferIndex_ = 0;
}

RC<CommandBuffer> VulkanCommandPool::NewCommandBuffer()
{
    if(nextFreeBufferIndex_ >= commandBuffers_.size())
    {
        CreateCommandBuffer();
    }
    return commandBuffers_[nextFreeBufferIndex_++];
}

void VulkanCommandPool::CreateCommandBuffer()
{
    const VkCommandBufferAllocateInfo allocateInfo = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool        = pool_,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };
    VkCommandBuffer newBuffer;
    VK_FAIL_MSG(
        vkAllocateCommandBuffers(device_, &allocateInfo, &newBuffer),
        "failed to allocate new command buffer");
    RTRC_SCOPE_FAIL{ vkFreeCommandBuffers(device_, pool_, 1, &newBuffer); };
    commandBuffers_.push_back(MakeRC<VulkanCommandBuffer>(device_, pool_, newBuffer));
}

RTRC_RHI_VK_END
