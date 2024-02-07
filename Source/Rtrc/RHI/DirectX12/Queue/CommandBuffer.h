#pragma once

#include <Rtrc/RHI/DirectX12/Pipeline/ComputePipeline.h>
#include <Rtrc/RHI/DirectX12/Pipeline/GraphicsPipeline.h>
#include <Rtrc/RHI/DirectX12/Pipeline/RayTracingPipeline.h>

RTRC_RHI_D3D12_BEGIN

class DirectX12CommandPool;

RTRC_RHI_IMPLEMENT(DirectX12CommandBuffer, CommandBuffer)
{
public:
    
#if RTRC_STATIC_RHI
    RTRC_RHI_COMMAND_BUFFER_COMMON_METHODS
#endif

    DirectX12CommandBuffer(
        DirectX12Device                   *device,
        DirectX12CommandPool              *pool,
        ComPtr<ID3D12GraphicsCommandList7> commandList);
    
    void Begin() RTRC_RHI_OVERRIDE;
    void End()   RTRC_RHI_OVERRIDE;

    void BeginRenderPass(
        Span<ColorAttachment>         colorAttachments,
        const DepthStencilAttachment &depthStencilAttachment) RTRC_RHI_OVERRIDE;
    void EndRenderPass() RTRC_RHI_OVERRIDE;

    void BindPipeline(const OPtr<GraphicsPipeline>   &pipeline) RTRC_RHI_OVERRIDE;
    void BindPipeline(const OPtr<ComputePipeline>    &pipeline) RTRC_RHI_OVERRIDE;
    void BindPipeline(const OPtr<RayTracingPipeline> &pipeline) RTRC_RHI_OVERRIDE;
    
    void BindGroupsToGraphicsPipeline  (int startIndex, Span<OPtr<BindingGroup>> groups) RTRC_RHI_OVERRIDE;
    void BindGroupsToComputePipeline   (int startIndex, Span<OPtr<BindingGroup>> groups) RTRC_RHI_OVERRIDE;
    void BindGroupsToRayTracingPipeline(int startIndex, Span<OPtr<BindingGroup>> groups) RTRC_RHI_OVERRIDE;

    void BindGroupToGraphicsPipeline  (int index, const OPtr<BindingGroup> &group) RTRC_RHI_OVERRIDE;
    void BindGroupToComputePipeline   (int index, const OPtr<BindingGroup> &group) RTRC_RHI_OVERRIDE;
    void BindGroupToRayTracingPipeline(int index, const OPtr<BindingGroup> &group) RTRC_RHI_OVERRIDE;

    void SetViewports(Span<Viewport> viewports) RTRC_RHI_OVERRIDE;
    void SetScissors(Span<Scissor> scissors) RTRC_RHI_OVERRIDE;

    void SetViewportsWithCount(Span<Viewport> viewports) RTRC_RHI_OVERRIDE;
    void SetScissorsWithCount(Span<Scissor> scissors) RTRC_RHI_OVERRIDE;

    void SetVertexBuffer(int slot, Span<BufferRPtr> buffers, Span<size_t> byteOffsets, Span<size_t> byteStrides) RTRC_RHI_OVERRIDE;
    void SetIndexBuffer(const BufferRPtr &buffer, size_t byteOffset, IndexFormat format) RTRC_RHI_OVERRIDE;

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

    void DispatchIndirect(const BufferRPtr &buffer, size_t byteOffset) RTRC_RHI_OVERRIDE;

    void DrawIndexedIndirect(
        const BufferRPtr &buffer, uint32_t drawCount, size_t byteOffset, size_t byteStride) RTRC_RHI_OVERRIDE;

    void CopyBuffer(Buffer *dst, size_t dstOffset, Buffer *src, size_t srcOffset, size_t range) RTRC_RHI_OVERRIDE;
    void CopyColorTexture(
        Texture *dst, uint32_t dstMipLevel, uint32_t dstArrayLayer,
        Texture *src, uint32_t srcMipLevel, uint32_t srcArrayLayer) RTRC_RHI_OVERRIDE;
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
    void ClearDepthStencilTexture(
        Texture* dst, const DepthStencilClearValue& clearValue, bool depth, bool stencil) RTRC_RHI_OVERRIDE;
    
    void BeginDebugEvent(const DebugLabel &label) RTRC_RHI_OVERRIDE;
    void EndDebugEvent() RTRC_RHI_OVERRIDE;

    void BuildBlas(
        const BlasPrebuildInfoOPtr  &buildInfo,
        Span<RayTracingGeometryDesc> geometries,
        const BlasOPtr              &blas,
        BufferDeviceAddress          scratchBufferAddress) RTRC_RHI_OVERRIDE;
    void BuildTlas(
        const TlasPrebuildInfoOPtr        &buildInfo,
        const RayTracingInstanceArrayDesc &instances,
        const TlasOPtr                    &tlas,
        BufferDeviceAddress                scratchBufferAddress) RTRC_RHI_OVERRIDE;

    ID3D12GraphicsCommandList *_internalGetCommandList() const { return commandList_.Get(); }

protected:

    void ExecuteBarriersInternal(
        Span<GlobalMemoryBarrier>      globalMemoryBarriers,
        Span<TextureTransitionBarrier> textureTransitions,
        Span<BufferTransitionBarrier>  bufferTransitions,
        Span<TextureReleaseBarrier>    textureReleaseBarriers,
        Span<TextureAcquireBarrier>    textureAcquireBarriers) RTRC_RHI_OVERRIDE;

private:
    
    DirectX12Device                   *device_;
    DirectX12CommandPool              *pool_;
    ComPtr<ID3D12GraphicsCommandList7> commandList_;

    OPtr<GraphicsPipeline>   currentGraphicsPipeline_;
    OPtr<ComputePipeline>    currentComputePipeline_;
    OPtr<RayTracingPipeline> currentRayTracingPipeline_;

    ID3D12RootSignature *currentGraphicsRootSignature_;
    ID3D12RootSignature *currentComputeRootSignature_; // For both compute & ray tracing
};

RTRC_RHI_D3D12_END
