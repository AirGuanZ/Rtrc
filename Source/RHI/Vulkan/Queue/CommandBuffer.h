#pragma once

#include <RHI/Vulkan/Common.h>

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

    void BindPipeline(const RPtr<GraphicsPipeline>   &pipeline) RTRC_RHI_OVERRIDE;
    void BindPipeline(const RPtr<ComputePipeline>    &pipeline) RTRC_RHI_OVERRIDE;
    void BindPipeline(const RPtr<RayTracingPipeline> &pipeline) RTRC_RHI_OVERRIDE;
    
    void BindGroupsToGraphicsPipeline  (int startIndex, Span<RPtr<BindingGroup>> groups) RTRC_RHI_OVERRIDE;
    void BindGroupsToComputePipeline   (int startIndex, Span<RPtr<BindingGroup>> groups) RTRC_RHI_OVERRIDE;
    void BindGroupsToRayTracingPipeline(int startIndex, Span<RPtr<BindingGroup>> groups) RTRC_RHI_OVERRIDE;

    void BindGroupToGraphicsPipeline  (int index, const RPtr<BindingGroup> &group) RTRC_RHI_OVERRIDE;
    void BindGroupToComputePipeline   (int index, const RPtr<BindingGroup> &group) RTRC_RHI_OVERRIDE;
    void BindGroupToRayTracingPipeline(int index, const RPtr<BindingGroup> &group) RTRC_RHI_OVERRIDE;

    void SetViewports(Span<Viewport> viewports) RTRC_RHI_OVERRIDE;
    void SetScissors(Span<Scissor> scissors) RTRC_RHI_OVERRIDE;

    void SetViewportsWithCount(Span<Viewport> viewports) RTRC_RHI_OVERRIDE;
    void SetScissorsWithCount(Span<Scissor> scissors) RTRC_RHI_OVERRIDE;

    void SetVertexBuffer(int slot, Span<BufferPtr> buffers, Span<size_t> byteOffsets, Span<size_t> byteStrides) RTRC_RHI_OVERRIDE;
    void SetIndexBuffer(const BufferPtr &buffer, size_t byteOffset, IndexFormat format) RTRC_RHI_OVERRIDE;

    void SetStencilReferenceValue(uint8_t value) RTRC_RHI_OVERRIDE;

    void SetGraphicsPushConstants(
        uint32_t    rangeIndex,
        uint32_t    offsetInRange,
        uint32_t    size,
        const void *data) RTRC_RHI_OVERRIDE;
    void SetComputePushConstants(
        uint32_t    rangeIndex,
        uint32_t    offsetInRange,
        uint32_t    size,
        const void *data) RTRC_RHI_OVERRIDE;

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

    void DispatchIndirect(const BufferPtr &buffer, size_t byteOffset) RTRC_RHI_OVERRIDE;

    void DrawIndexedIndirect(
        const BufferPtr &buffer, uint32_t drawCount, size_t byteOffset, size_t byteStride) RTRC_RHI_OVERRIDE;

    void CopyBuffer(Buffer *dst, size_t dstOffset, Buffer *src, size_t srcOffset, size_t range) RTRC_RHI_OVERRIDE;
    void CopyBufferToColorTexture2D(
        Texture *dst,
        uint32_t mipLevel,
        uint32_t arrayLayer,
        Buffer  *src,
        size_t   srcOffset,
        size_t   srcRowBytes) RTRC_RHI_OVERRIDE;
    void CopyColorTexture2DToBuffer(
        Buffer  *dst,
        size_t   dstOffset,
        size_t   dstRowBytes,
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
        const TlasPrebuildInfoPtr         &buildInfo,
        const RayTracingInstanceArrayDesc &instances,
        const TlasPtr                     &tlas,
        BufferDeviceAddress                scratchBufferAddress) RTRC_RHI_OVERRIDE;

    VkCommandBuffer _internalGetNativeCommandBuffer() const;

protected:

    void ExecuteBarriersInternal(
        Span<GlobalMemoryBarrier>      globalMemoryBarriers,
        Span<TextureTransitionBarrier> textureTransitions,
        Span<BufferTransitionBarrier>  bufferTransitions,
        Span<TextureReleaseBarrier>    textureReleaseBarriers,
        Span<TextureAcquireBarrier>    textureAcquireBarriers) RTRC_RHI_OVERRIDE;

private:

    VulkanDevice   *device_;
    VkCommandPool   pool_;
    VkCommandBuffer commandBuffer_;

    RPtr<GraphicsPipeline>   currentGraphicsPipeline_;
    RPtr<ComputePipeline>    currentComputePipeline_;
    RPtr<RayTracingPipeline> currentRayTracingPipeline_;
};

RTRC_RHI_VK_END
