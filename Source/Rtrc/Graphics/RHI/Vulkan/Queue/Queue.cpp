#include <Rtrc/Graphics/RHI/Vulkan/Context/BackBufferSemaphore.h>
#include <Rtrc/Graphics/RHI/Vulkan/Context/Device.h>
#include <Rtrc/Graphics/RHI/Vulkan/Queue/CommandPool.h>
#include <Rtrc/Graphics/RHI/Vulkan/Queue/Fence.h>
#include <Rtrc/Graphics/RHI/Vulkan/Queue/Queue.h>
#include <Rtrc/Graphics/RHI/Vulkan/Queue/Semaphore.h>
#include <Rtrc/Utils/Enumerate.h>
#include <Rtrc/Utils/ScopeGuard.h>

RTRC_RHI_VK_BEGIN

VulkanQueue::VulkanQueue(VulkanDevice *device, VkQueue queue, QueueType type, uint32_t queueFamilyIndex)
    : device_(device), queue_(queue), type_(type), queueFamilyIndex_(queueFamilyIndex)
{

}

QueueType VulkanQueue::GetType() const
{
    return type_;
}

void VulkanQueue::WaitIdle()
{
    VK_FAIL_MSG(vkQueueWaitIdle(queue_), "failed to wait queue idle");
}

void VulkanQueue::Submit(
    BackBufferSemaphoreDependency waitBackBufferSemaphore,
    Span<SemaphoreDependency>     waitSemaphores,
    Span<Ptr<CommandBuffer>>      commandBuffers,
    BackBufferSemaphoreDependency signalBackBufferSemaphore,
    Span<SemaphoreDependency>     signalSemaphores,
    const Ptr<Fence>             &signalFence)
{
    std::vector<VkCommandBufferSubmitInfo> vkCommandBuffers(commandBuffers.size());
    for(auto &&[i, cb] : Enumerate(commandBuffers))
    {
        vkCommandBuffers[i] = VkCommandBufferSubmitInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
            .commandBuffer = static_cast<VulkanCommandBuffer *>(cb.Get())->GetNativeCommandBuffer()
        };
    }

    std::vector<VkSemaphoreSubmitInfo> waitSemaphoreSubmitInfo;
    if(waitBackBufferSemaphore.semaphore)
    {
        waitSemaphoreSubmitInfo.push_back({
            .sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = static_cast<VulkanBackBufferSemaphore *>(waitBackBufferSemaphore.semaphore.Get())->GetBinarySemaphore(),
            .stageMask = TranslatePipelineStageFlag(waitBackBufferSemaphore.stages),
        });
    }
    for(auto &s : waitSemaphores)
    {
        waitSemaphoreSubmitInfo.push_back({
            .sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = static_cast<VulkanSemaphore*>(s.semaphore.Get())->GetNativeSemaphore(),
            .value     = s.value,
            .stageMask = TranslatePipelineStageFlag(s.stages)
        });
    }

    std::vector<VkSemaphoreSubmitInfo> signalSemaphoreSubmitInfo;
    if(signalBackBufferSemaphore.semaphore)
    {
        signalSemaphoreSubmitInfo.push_back({
            .sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = static_cast<VulkanBackBufferSemaphore *>(signalBackBufferSemaphore.semaphore.Get())->GetBinarySemaphore(),
            .stageMask = TranslatePipelineStageFlag(signalBackBufferSemaphore.stages)
        });
    }
    for(auto &s : signalSemaphores)
    {
        signalSemaphoreSubmitInfo.push_back({
            .sType     = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
            .semaphore = static_cast<VulkanSemaphore*>(s.semaphore.Get())->GetNativeSemaphore(),
            .value     = s.value,
            .stageMask = TranslatePipelineStageFlag(s.stages)
        });
    }

    const VkSubmitInfo2 submitInfo = {
        .sType                    = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .waitSemaphoreInfoCount   = static_cast<uint32_t>(waitSemaphoreSubmitInfo.size()),
        .pWaitSemaphoreInfos      = waitSemaphoreSubmitInfo.data(),
        .commandBufferInfoCount   = static_cast<uint32_t>(vkCommandBuffers.size()),
        .pCommandBufferInfos      = vkCommandBuffers.data(),
        .signalSemaphoreInfoCount = static_cast<uint32_t>(signalSemaphoreSubmitInfo.size()),
        .pSignalSemaphoreInfos    = signalSemaphoreSubmitInfo.data()
    };
    auto fence = signalFence ? static_cast<VulkanFence *>(signalFence.Get())->GetNativeFence() : VK_NULL_HANDLE;
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

Ptr<CommandPool> VulkanQueue::CreateCommandPoolImpl() const
{
    const VkCommandPoolCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .queueFamilyIndex = queueFamilyIndex_
    };
    VkCommandPool pool;
    VK_FAIL_MSG(
        vkCreateCommandPool(device_->GetNativeDevice(), &createInfo, VK_ALLOC, &pool),
        "failed to create vulkan command pool");
    RTRC_SCOPE_FAIL{ vkDestroyCommandPool(device_->GetNativeDevice(), pool, VK_ALLOC); };
    return MakePtr<VulkanCommandPool>(device_, GetType(), pool);
}

RTRC_RHI_VK_END