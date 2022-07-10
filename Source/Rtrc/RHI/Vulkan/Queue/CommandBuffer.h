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

    void BindPipeline(const Ptr<GraphicsPipeline> &pipeline) override;

    void BindPipeline(const Ptr<ComputePipeline> &pipeline) override;
    
    void BindGroupsToGraphicsPipeline(int startIndex, Span<RC<BindingGroup>> groups) override;

    void BindGroupsToComputePipeline(int startIndex, Span<RC<BindingGroup>> groups) override;

    void BindGroupToGraphicsPipeline(int index, const Ptr<BindingGroup> &group) override;

    void BindGroupToComputePipeline(int index, const Ptr<BindingGroup> &group) override;

    void SetViewports(Span<Viewport> viewports) override;

    void SetScissors(Span<Scissor> scissors) override;

    void SetViewportsWithCount(Span<Viewport> viewports) override;

    void SetScissorsWithCount(Span<Scissor> scissors) override;

    void Draw(int vertexCount, int instanceCount, int firstVertex, int firstInstance) override;

    void Dispatch(int groupCountX, int groupCountY, int groupCountZ) override;

    void CopyBuffer(
        Buffer *dst, size_t dstOffset,
        Buffer *src, size_t srcOffset, size_t range) override;

    void CopyBufferToTexture(
        Texture *dst, AspectTypeFlag aspect, uint32_t mipLevel, uint32_t arrayLayer,
        Buffer *src, size_t srcOffset) override;

    void CopyTextureToBuffer(
        Buffer *dst, size_t dstOffset,
        Texture *src, AspectTypeFlag aspect, uint32_t mipLevel, uint32_t arrayLayer) override;

    VkCommandBuffer GetNativeCommandBuffer() const;

protected:

    const Ptr<GraphicsPipeline> &GetCurrentPipeline() const override;

private:

    VkDevice        device_;
    VkCommandPool   pool_;
    VkCommandBuffer commandBuffer_;

    Ptr<GraphicsPipeline> currentGraphicsPipeline_;
    Ptr<ComputePipeline>  currentComputePipeline_;
};

RTRC_RHI_VK_END
