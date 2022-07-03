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
        Span<TextureTransitionBarrier> textureTransitions,
        Span<BufferTransitionBarrier>  bufferTransitions,
        Span<TextureReleaseBarrier>    textureReleaseBarriers,
        Span<TextureAcquireBarrier>    textureAcquireBarriers,
        Span<BufferReleaseBarrier>     bufferReleaseBarriers,
        Span<BufferAcquireBarrier>     bufferAcquireBarriers) override;

    void BeginRenderPass(Span<RenderPassColorAttachment> colorAttachments) override;

    void EndRenderPass() override;

    void BindPipeline(const RC<GraphicsPipeline> &pipeline) override;
    
    void BindGroups(int startIndex, Span<RC<BindingGroup>> groups) override;

    void BindGroup(int index, const RC<BindingGroup> &group) override;

    void SetViewports(Span<Viewport> viewports) override;

    void SetScissors(Span<Scissor> scissors) override;

    void SetViewportsWithCount(Span<Viewport> viewports) override;

    void SetScissorsWithCount(Span<Scissor> scissors) override;

    void Draw(int vertexCount, int instanceCount, int firstVertex, int firstInstance) override;

    void CopyBuffer(
        const RC<Buffer> &dst, size_t dstOffset,
        const RC<Buffer> &src, size_t srcOffset, size_t range) override;

    void CopyBufferToTexture(
        const RC<Texture> &dst, AspectTypeFlag aspect, uint32_t mipLevel, uint32_t arrayLayer,
        const RC<Buffer> &src, size_t srcOffset) override;

    void CopyTextureToBuffer(
        const RC<Buffer> &dst, size_t dstOffset,
        const RC<Texture> &src, AspectTypeFlag aspect, uint32_t mipLevel, uint32_t arrayLayer) override;

    VkCommandBuffer GetNativeCommandBuffer() const;

protected:

    const RC<GraphicsPipeline> &GetCurrentPipeline() const override;

private:

    VkDevice        device_;
    VkCommandPool   pool_;
    VkCommandBuffer commandBuffer_;

    RC<GraphicsPipeline> currentPipeline_;
};

RTRC_RHI_VK_END
