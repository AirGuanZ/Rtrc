#pragma once

#include <Rtrc/RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

class VulkanCommandBuffer : public CommandBuffer
{
public:

    VulkanCommandBuffer(VkDevice device, VkCommandPool pool, VkCommandBuffer commandBuffer);

    ~VulkanCommandBuffer() override;

    void Begin() override;

    void End() override;

    void ExecuteBarriers(
        const std::vector<TextureTransitionBarrier> &textureTransitions,
        const std::vector<BufferTransitionBarrier> &bufferTransitions) override;

    void BeginRenderPass(const std::vector<RenderPassColorAttachment> &colorAttachments) override;

    void EndRenderPass() override;

    VkCommandBuffer GetNativeCommandBuffer() const;

private:

    VkDevice device_;
    VkCommandPool pool_;
    VkCommandBuffer commandBuffer_;
};

RTRC_RHI_VK_END
