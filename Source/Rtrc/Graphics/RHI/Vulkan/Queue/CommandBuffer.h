#pragma once

#include <Rtrc/Graphics/RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

class VulkanDevice;

class VulkanCommandBuffer : public CommandBuffer
{
public:

    VulkanCommandBuffer(VulkanDevice *device, VkCommandPool pool, VkCommandBuffer commandBuffer);

    ~VulkanCommandBuffer() override;

    void Begin() override;
    void End() override;

    void BeginRenderPass(
        Span<RenderPassColorAttachment>         colorAttachments,
        const RenderPassDepthStencilAttachment &depthStencilAttachment) override;
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

    void SetVertexBuffer(int slot, Span<BufferPtr> buffers, Span<size_t> byteOffsets) override;
    void SetIndexBuffer(const BufferPtr &buffer, size_t byteOffset, IndexBufferFormat format) override;

    void Draw(int vertexCount, int instanceCount, int firstVertex, int firstInstance) override;
    void DrawIndexed(int indexCount, int instanceCount, int firstIndex, int firstVertex, int firstInstance) override;

    void Dispatch(int groupCountX, int groupCountY, int groupCountZ) override;

    void CopyBuffer(Buffer *dst, size_t dstOffset, Buffer *src, size_t srcOffset, size_t range) override;

    void CopyBufferToColorTexture2D(
        Texture *dst, uint32_t mipLevel, uint32_t arrayLayer, Buffer *src, size_t srcOffset) override;
    void CopyColorTexture2DToBuffer(
        Buffer *dst, size_t dstOffset, Texture *src, uint32_t mipLevel, uint32_t arrayLayer) override;

    void ClearColorTexture2D(Texture *dst, const ColorClearValue &clearValue) override;

    void BeginDebugEvent(const DebugLabel &label) override;
    void EndDebugEvent() override;

    VkCommandBuffer GetNativeCommandBuffer() const;

protected:

    void ExecuteBarriersInternal(
        Span<TextureTransitionBarrier> textureTransitions,
        Span<BufferTransitionBarrier>  bufferTransitions,
        Span<TextureReleaseBarrier>    textureReleaseBarriers,
        Span<TextureAcquireBarrier>    textureAcquireBarriers) override;

private:

    VulkanDevice   *device_;
    VkCommandPool   pool_;
    VkCommandBuffer commandBuffer_;

    Ptr<GraphicsPipeline> currentGraphicsPipeline_;
    Ptr<ComputePipeline>  currentComputePipeline_;
};

RTRC_RHI_VK_END
