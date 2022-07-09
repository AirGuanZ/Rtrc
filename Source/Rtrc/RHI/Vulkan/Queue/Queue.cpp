#include <Rtrc/RHI/Vulkan/Context/BackBufferSemaphore.h>
#include <Rtrc/RHI/Vulkan/Queue/CommandPool.h>
#include <Rtrc/RHI/Vulkan/Queue/Fence.h>
#include <Rtrc/RHI/Vulkan/Queue/Queue.h>
#include <Rtrc/Utils/Enumerate.h>
#include <Rtrc/Utils/ScopeGuard.h>

RTRC_RHI_VK_BEGIN

VulkanQueue::VulkanQueue(VkDevice device, VkQueue queue, QueueType type, uint32_t queueFamilyIndex)
    : device_(device), queue_(queue), type_(type), queueFamilyIndex_(queueFamilyIndex)
{

}

QueueType VulkanQueue::GetType() const
{
    return type_;
}

Ptr<CommandPool> VulkanQueue::CreateCommandPool()
{
    const VkCommandPoolCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = queueFamilyIndex_
    };
    VkCommandPool pool;
    VK_FAIL_MSG(
        vkCreateCommandPool(device_, &createInfo, VK_ALLOC, &pool),
        "failed to create vulkan command pool");
    RTRC_SCOPE_FAIL{ vkDestroyCommandPool(device_, pool, VK_ALLOC); };
    return MakePtr<VulkanCommandPool>(device_, pool);
}

void VulkanQueue::WaitIdle()
{
    VK_FAIL_MSG(vkQueueWaitIdle(queue_), "failed to wait queue idle");
}

void VulkanQueue::Submit(
    const Ptr<BackBufferSemaphore> &waitBackBufferSemaphore,
    PipelineStage                   waitBackBufferStages,
    Span<Ptr<CommandBuffer>>        commandBuffers,
    const Ptr<BackBufferSemaphore> &signalBackBufferSemaphore,
    PipelineStage                   signalBackBufferStages,
    const Ptr<Fence>               &signalFence)
{
    std::vector<VkCommandBufferSubmitInfo> vkCommandBuffers(commandBuffers.size());
    for(auto &&[i, cb] : Enumerate(commandBuffers))
    {
        vkCommandBuffers[i] = VkCommandBufferSubmitInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
            .commandBuffer = static_cast<VulkanCommandBuffer *>(cb.Get())->GetNativeCommandBuffer()
        };
    }

    VkSemaphore vkWaitBackBufferSemaphore = waitBackBufferSemaphore ?
        static_cast<VulkanBackBufferSemaphore *>(waitBackBufferSemaphore.Get())->GetBinarySemaphore() : nullptr;

    const VkSemaphoreSubmitInfo waitBackBufferSemaphoreSubmitInfo = {
        .sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .semaphore = vkWaitBackBufferSemaphore,
        .stageMask = TranslatePipelineStageFlag(waitBackBufferStages),
    };

    VkSemaphore vkSignalBackBufferSemaphore = signalBackBufferSemaphore ?
        static_cast<VulkanBackBufferSemaphore *>(signalBackBufferSemaphore.Get())->GetBinarySemaphore() : nullptr;

    const VkSemaphoreSubmitInfo signalBackBufferSemaphoreSubmitInfo = {
        .sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .semaphore = vkSignalBackBufferSemaphore,
        .stageMask = TranslatePipelineStageFlag(signalBackBufferStages)
    };

    VkSubmitInfo2 submitInfo = {
        .sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .waitSemaphoreInfoCount   = vkWaitBackBufferSemaphore ? 1u : 0u,
        .pWaitSemaphoreInfos      = vkWaitBackBufferSemaphore ? &waitBackBufferSemaphoreSubmitInfo : nullptr,
        .commandBufferInfoCount   = static_cast<uint32_t>(vkCommandBuffers.size()),
        .pCommandBufferInfos      = vkCommandBuffers.data(),
        .signalSemaphoreInfoCount = vkSignalBackBufferSemaphore ? 1u : 0u,
        .pSignalSemaphoreInfos    = vkSignalBackBufferSemaphore ? &signalBackBufferSemaphoreSubmitInfo : nullptr
    };

    auto fence = signalFence ? static_cast<VulkanFence *>(signalFence.Get())->GetNativeFence() : nullptr;
    VK_FAIL_MSG(
        vkQueueSubmit2(queue_, 1, &submitInfo, fence),
        "failed to submit to vulkan queue");
}

VkQueue VulkanQueue::GetNativeQueue() const
{
    return queue_;
}

uint32_t VulkanQueue::GetNativeFamilyIndex() const
{
    return queueFamilyIndex_;
}

RTRC_RHI_VK_END
