#pragma once

#include <Rtrc/RHI/Vulkan/Pipeline/Pipeline.h>

RTRC_RHI_VK_BEGIN

class VulkanCommandBuffer : public CommandBuffer
{
public:

    VulkanCommandBuffer(VkDevice device, VkCommandPool pool, VkCommandBuffer commandBuffer);

    ~VulkanCommandBuffer() override;

    void Begin() override;

    void End() override;

    void ExecuteBarriers(
        Span<TextureTransitionBarrier> textureTransitions,
        Span<BufferTransitionBarrier>  bufferTransitions,
        Span<TextureReleaseBarrier>    textureReleaseBarriers,
        Span<TextureAcquireBarrier>    textureAcquireBarriers,
        Span<BufferReleaseBarrier>     bufferReleaseBarriers,
        Span<BufferAcquireBarrier>     bufferAcquireBarriers) override;

    void BeginRenderPass(Span<RenderPassColorAttachment> colorAttachments) override;

    void EndRenderPass() override;

    void BindPipeline(const RC<Pipeline> &pipeline) override;
    
    void BindGroups(int startIndex, Span<RC<BindingGroup>> groups) override;

    void SetViewports(Span<Viewport> viewports) override;

    void SetScissors(Span<Scissor> scissors) override;

    void SetViewportsWithCount(Span<Viewport> viewports) override;

    void SetScissorsWithCount(Span<Scissor> scissors) override;

    void Draw(int vertexCount, int instanceCount, int firstVertex, int firstInstance) override;

    VkCommandBuffer GetNativeCommandBuffer() const;

private:

    VkDevice        device_;
    VkCommandPool   pool_;
    VkCommandBuffer commandBuffer_;

    RC<VulkanPipeline> currentPipeline_;
};

RTRC_RHI_VK_END
