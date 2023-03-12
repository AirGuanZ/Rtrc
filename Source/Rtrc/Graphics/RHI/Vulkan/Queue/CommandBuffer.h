#pragma once

#include <Rtrc/Graphics/RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

class VulkanDevice;

RTRC_RHI_IMPLEMENT(VulkanCommandBuffer, CommandBuffer)
{
public:

#ifdef RTRC_STATIC_RHI
    RTRC_RHI_COMMAND_BUFFER_COMMON_METHODS
#endif

    VulkanCommandBuffer(VulkanDevice *device, VkCommandPool pool, VkCommandBuffer commandBuffer);

    ~VulkanCommandBuffer() override;

    void Begin() RTRC_RHI_OVERRIDE;
    void End()   RTRC_RHI_OVERRIDE;

    void BeginRenderPass(
        Span<RenderPassColorAttachment>         colorAttachments,
        const RenderPassDepthStencilAttachment &depthStencilAttachment) RTRC_RHI_OVERRIDE;
    void EndRenderPass() RTRC_RHI_OVERRIDE;

    void BindPipeline(const Ptr<GraphicsPipeline>   &pipeline) RTRC_RHI_OVERRIDE;
    void BindPipeline(const Ptr<ComputePipeline>    &pipeline) RTRC_RHI_OVERRIDE;
    void BindPipeline(const Ptr<RayTracingPipeline> &pipeline) RTRC_RHI_OVERRIDE;
    
    void BindGroupsToGraphicsPipeline  (int startIndex, Span<RC<BindingGroup>> groups) RTRC_RHI_OVERRIDE;
    void BindGroupsToComputePipeline   (int startIndex, Span<RC<BindingGroup>> groups) RTRC_RHI_OVERRIDE;
    void BindGroupsToRayTracingPipeline(int startIndex, Span<RC<BindingGroup>> groups) RTRC_RHI_OVERRIDE;

    void BindGroupToGraphicsPipeline  (int index, const Ptr<BindingGroup> &group) RTRC_RHI_OVERRIDE;
    void BindGroupToComputePipeline   (int index, const Ptr<BindingGroup> &group) RTRC_RHI_OVERRIDE;
    void BindGroupToRayTracingPipeline(int index, const Ptr<BindingGroup> &group) RTRC_RHI_OVERRIDE;

    void SetViewports(Span<Viewport> viewports) RTRC_RHI_OVERRIDE;
    void SetScissors(Span<Scissor> scissors) RTRC_RHI_OVERRIDE;

    void SetViewportsWithCount(Span<Viewport> viewports) RTRC_RHI_OVERRIDE;
    void SetScissorsWithCount(Span<Scissor> scissors) RTRC_RHI_OVERRIDE;

    void SetVertexBuffer(int slot, Span<BufferPtr> buffers, Span<size_t> byteOffsets) RTRC_RHI_OVERRIDE;
    void SetIndexBuffer(const BufferPtr &buffer, size_t byteOffset, IndexFormat format) RTRC_RHI_OVERRIDE;

    void SetStencilReferenceValue(uint8_t value) RTRC_RHI_OVERRIDE;

    void SetPushConstants(
        const BindingLayoutPtr &bindingLayout,
        ShaderStageFlags         stages,
        uint32_t                offset,
        uint32_t                size,
        const void             *values) RTRC_RHI_OVERRIDE;

    void Draw(int vertexCount, int instanceCount, int firstVertex, int firstInstance) RTRC_RHI_OVERRIDE;
    void DrawIndexed(
        int indexCount,
        int instanceCount,
        int firstIndex,
        int firstVertex,
        int firstInstance) RTRC_RHI_OVERRIDE;

    void Dispatch(int groupCountX, int groupCountY, int groupCountZ) RTRC_RHI_OVERRIDE;

    void TraceRays(
        int                             rayCountX,
        int                             rayCountY,
        int                             rayCountZ,
        const ShaderBindingTableRegion &rayGenSbt,
        const ShaderBindingTableRegion &missSbt,
        const ShaderBindingTableRegion &hitSbt,
        const ShaderBindingTableRegion &callableSbt) RTRC_RHI_OVERRIDE;

    void CopyBuffer(Buffer *dst, size_t dstOffset, Buffer *src, size_t srcOffset, size_t range) RTRC_RHI_OVERRIDE;
    void CopyBufferToColorTexture2D(
        Texture *dst,
        uint32_t mipLevel,
        uint32_t arrayLayer,
        Buffer  *src,
        size_t   srcOffset) RTRC_RHI_OVERRIDE;
    void CopyColorTexture2DToBuffer(
        Buffer  *dst,
        size_t   dstOffset,
        Texture *src,
        uint32_t mipLevel,
        uint32_t arrayLayer) RTRC_RHI_OVERRIDE;

    void ClearColorTexture2D(Texture *dst, const ColorClearValue &clearValue) RTRC_RHI_OVERRIDE;

    void BeginDebugEvent(const DebugLabel &label) RTRC_RHI_OVERRIDE;
    void EndDebugEvent() RTRC_RHI_OVERRIDE;

    void BuildBlas(
        const BlasPrebuildInfoPtr   &buildInfo,
        Span<RayTracingGeometryDesc> geometries,
        const BlasPtr               &blas,
        BufferDeviceAddress          scratchBufferAddress) RTRC_RHI_OVERRIDE;
    void BuildTlas(
        const TlasPrebuildInfoPtr        &buildInfo,
        Span<RayTracingInstanceArrayDesc> instanceArrays,
        const TlasPtr                    &tlas,
        BufferDeviceAddress               scratchBufferAddress) RTRC_RHI_OVERRIDE;

    VkCommandBuffer _internalGetNativeCommandBuffer() const;

protected:

    void ExecuteBarriersInternal(
        Span<TextureTransitionBarrier> textureTransitions,
        Span<BufferTransitionBarrier>  bufferTransitions,
        Span<TextureReleaseBarrier>    textureReleaseBarriers,
        Span<TextureAcquireBarrier>    textureAcquireBarriers) RTRC_RHI_OVERRIDE;

private:

    VulkanDevice   *device_;
    VkCommandPool   pool_;
    VkCommandBuffer commandBuffer_;

    Ptr<GraphicsPipeline>   currentGraphicsPipeline_;
    Ptr<ComputePipeline>    currentComputePipeline_;
    Ptr<RayTracingPipeline> currentRayTracingPipeline_;
};

RTRC_RHI_VK_END
