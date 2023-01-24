#include <Rtrc/Graphics/RHI/Vulkan/Context/Device.h>
#include <Rtrc/Graphics/RHI/Vulkan/Queue/CommandPool.h>
#include <Rtrc/Utility/ScopeGuard.h>

RTRC_RHI_VK_BEGIN

VulkanCommandPool::VulkanCommandPool(VulkanDevice *device, QueueType type, VkCommandPool pool)
    : device_(device), type_(type), pool_(pool), nextFreeBufferIndex_(0)
{
    
}

VulkanCommandPool::~VulkanCommandPool()
{
    commandBuffers_.clear();
    vkDestroyCommandPool(device_->_internalGetNativeDevice(), pool_, RTRC_VK_ALLOC);
}

void VulkanCommandPool::Reset()
{
    vkResetCommandPool(device_->_internalGetNativeDevice(), pool_, 0);
    nextFreeBufferIndex_ = 0;
}

QueueType VulkanCommandPool::GetType() const
{
    return type_;
}

Ptr<CommandBuffer> VulkanCommandPool::NewCommandBuffer()
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
    RTRC_VK_FAIL_MSG(
        vkAllocateCommandBuffers(device_->_internalGetNativeDevice(), &allocateInfo, &newBuffer),
        "failed to allocate new command buffer");
    RTRC_SCOPE_FAIL{ vkFreeCommandBuffers(device_->_internalGetNativeDevice(), pool_, 1, &newBuffer); };
    commandBuffers_.push_back(MakePtr<VulkanCommandBuffer>(device_, pool_, newBuffer));
}

RTRC_RHI_VK_END
