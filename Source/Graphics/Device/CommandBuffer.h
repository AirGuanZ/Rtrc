#pragma once

#include <tbb/concurrent_queue.h>
#include <tbb/spin_rw_mutex.h>

#include <Graphics/Device/BindingGroup.h>
#include <Graphics/Device/Buffer.h>
#include <Graphics/Device/DeviceSynchronizer.h>
#include <Graphics/Device/Pipeline.h>
#include <Graphics/Device/Texture.h>
#include <Graphics/RenderGraph/Resource.h>

RTRC_BEGIN

class Buffer;
class CommandBuffer;
class CommandBufferManager;
class Device;
class Texture;

class Blas;
class Tlas;
class BlasPrebuildInfo;
class TlasPrebuildInfo;

using AttachmentLoadOp = RHI::AttachmentLoadOp;
using AttachmentStoreOp = RHI::AttachmentStoreOp;
using ColorClearValue = RHI::ColorClearValue;
using DepthStencilClearValue = RHI::DepthStencilClearValue;

using Viewport = RHI::Viewport;
using Scissor = RHI::Scissor;

using ColorAttachment = RHI::RenderPassColorAttachment;
using DepthStencilAttachment = RHI::RenderPassDepthStencilAttachment;

class BarrierBatch
{
public:

    BarrierBatch() = default;

    template<typename A, typename B, typename...Args>
    BarrierBatch(const A &a, const B &b, const Args...args)
    {
        operator()(a, b, args...);
    }

    BarrierBatch &operator()(
        RHI::PipelineStageFlag  prevStages,
        RHI::ResourceAccessFlag prevAccesses,
        RHI::PipelineStageFlag  succStages,
        RHI::ResourceAccessFlag succAccesses);

    BarrierBatch &operator()(
        const RC<StatefulBuffer> &buffer,
        RHI::PipelineStageFlag    stages,
        RHI::ResourceAccessFlag   accesses);

    BarrierBatch &operator()(
        const RC<StatefulTexture> &texture,
        RHI::TextureLayout         layout,
        RHI::PipelineStageFlag     stages,
        RHI::ResourceAccessFlag    accesses);

    BarrierBatch &operator()(
        const RC<StatefulTexture> &texture,
        uint32_t                   arrayLayer,
        uint32_t                   mipLevel,
        RHI::TextureLayout         layout,
        RHI::PipelineStageFlag     stages,
        RHI::ResourceAccessFlag    accesses);
    
    BarrierBatch &operator()(
        const RC<Buffer>       &buffer,
        RHI::PipelineStageFlag  prevStages,
        RHI::ResourceAccessFlag prevAccesses,
        RHI::PipelineStageFlag  succStages,
        RHI::ResourceAccessFlag succAccesses);

    BarrierBatch &operator()(
        const RC<Texture>      &texture,
        RHI::TextureLayout      prevLayout,
        RHI::PipelineStageFlag  prevStages,
        RHI::ResourceAccessFlag prevAccesses,
        RHI::TextureLayout      succLayout,
        RHI::PipelineStageFlag  succStages,
        RHI::ResourceAccessFlag succAccesses);
    
    BarrierBatch &operator()(
        const RC<Texture>      &texture,
        uint32_t                arrayLayer,
        uint32_t                mipLevel,
        RHI::TextureLayout      prevLayout,
        RHI::PipelineStageFlag  prevStages,
        RHI::ResourceAccessFlag prevAccesses,
        RHI::TextureLayout      succLayout,
        RHI::PipelineStageFlag  succStages,
        RHI::ResourceAccessFlag succAccesses);

    BarrierBatch &operator()(
        const RC<Texture> &texture,
        RHI::TextureLayout prevLayout,
        RHI::TextureLayout succLayout);

    BarrierBatch &Add(std::vector<RHI::BufferTransitionBarrier> &&bufferBarriers);
    BarrierBatch &Add(std::vector<RHI::TextureTransitionBarrier> &&textureBarriers);

    BarrierBatch &Add(const std::vector<RHI::BufferTransitionBarrier> &bufferBarriers);
    BarrierBatch &Add(const std::vector<RHI::TextureTransitionBarrier> &textureBarriers);

private:

    friend class CommandBuffer;

    std::optional<RHI::GlobalMemoryBarrier>    G_;
    std::vector<RHI::BufferTransitionBarrier>  BT_;
    std::vector<RHI::TextureTransitionBarrier> TT_;
};

class CommandBuffer : public Uncopyable
{
public:

    CommandBuffer();
    ~CommandBuffer();

    CommandBuffer(CommandBuffer &&other) noexcept;
    CommandBuffer &operator=(CommandBuffer &&other) noexcept;

    void Swap(CommandBuffer &other) noexcept;

    RHI::QueueType GetQueueType() const;

    const RHI::CommandBufferPtr &GetRHIObject() const;

    // Once begin, the command buffer object is bound with current thread, and cannot be used in any other thread.
    void Begin();
    void End();

    void BeginDebugEvent(std::string name, const std::optional<Vector4f> &color = std::nullopt);
    void EndDebugEvent();

    void CopyBuffer(const Buffer &dst, size_t dstOffset, const Buffer &src, size_t srcOffset, size_t size);
    void CopyColorTexture2DToBuffer(
        Buffer &dst, size_t dstOffset, size_t dstRowBytes, Texture &src, uint32_t arrayLayer, uint32_t mipLevel);

    void CopyBuffer(
        const RG::BufferResource *dst, size_t dstOffset,
        const RG::BufferResource *src, size_t srcOffset, size_t size);
    void CopyColorTexture2DToBuffer(
        RG::BufferResource *dst, size_t dstOffset, size_t dstRowBytes,
        RG::TextureResource *src, uint32_t arrayLayer, uint32_t mipLevel);

    void BeginRenderPass(Span<ColorAttachment> colorAttachments);
    void BeginRenderPass(const DepthStencilAttachment &depthStencilAttachment);
    void BeginRenderPass(Span<ColorAttachment> colorAttachments, const DepthStencilAttachment &depthStencilAttachment);
    void EndRenderPass();

    void BindGraphicsPipeline(const RC<GraphicsPipeline> &graphicsPipeline);
    void BindComputePipeline(const RC<ComputePipeline> &computePipeline);
    void BindRayTracingPipeline(const RC<RayTracingPipeline> &rayTracingPipeline);

    const GraphicsPipeline   *GetCurrentGraphicsPipeline() const;
    const ComputePipeline    *GetCurrentComputePipeline() const;
    const RayTracingPipeline *GetCurrentRayTracingPipeline() const;

    void BindGraphicsGroup(int index, const RC<BindingGroup> &group);
    void BindComputeGroup(int index, const RC<BindingGroup> &group);
    void BindRayTracingGroup(int index, const RC<BindingGroup> &group);
    
    void BindGraphicsGroups(Span<RC<BindingGroup>> groups);
    void BindComputeGroups(Span<RC<BindingGroup>> groups);
    void BindRayTracingGroups(Span<RC<BindingGroup>> groups);
    
    void SetViewports(Span<Viewport> viewports);
    void SetScissors(Span<Scissor> scissors);

    void SetVertexBuffer(int slot, const RC<SubBuffer> &buffer, size_t stride);
    void SetVertexBuffers(int slot, Span<RC<SubBuffer>> buffers, Span<size_t> strides);
    void SetIndexBuffer(const RC<SubBuffer> &buffer, RHI::IndexFormat format);
    
    void SetStencilReferenceValue(uint8_t value);

    void SetGraphicsPushConstants(uint32_t rangeIndex, uint32_t offset, uint32_t size, const void *data);
    void SetComputePushConstants(uint32_t rangeIndex, uint32_t offset, uint32_t size, const void *data);

    void SetGraphicsPushConstants(uint32_t rangeIndex, const void *data);
    void SetComputePushConstants(uint32_t rangeIndex, const void *data);
    
    template<typename T> requires std::is_trivially_copyable_v<T>
    void SetGraphicsPushConstants(uint32_t rangeIndex, const T &data);
    template<typename T> requires std::is_trivially_copyable_v<T>
    void SetComputePushConstants(uint32_t rangeIndex, const T &data);

    void ClearColorTexture2D(const RC<Texture> &tex, const Vector4f &color);
    
    void Draw(int vertexCount, int instanceCount, int firstVertex, int firstInstance);
    void DrawIndexed(int indexCount, int instanceCount, int firstIndex, int firstVertex, int firstInstance);
    void Dispatch(int groupCountX, int groupCountY, int groupCountZ);
    void Dispatch(const Vector3i &groupCount);
    void DispatchWithThreadCount(unsigned int countX, unsigned int countY, unsigned int countZ);
    void DispatchWithThreadCount(unsigned int countX, unsigned int countY);
    void DispatchWithThreadCount(unsigned int countX);
    void DispatchWithThreadCount(const Vector3i &count);
    void DispatchWithThreadCount(const Vector3u &count);
    void DispatchWithThreadCount(const Vector2i &count);
    void DispatchWithThreadCount(const Vector2u &count);
    void Trace(
        int                                  rayCountX,
        int                                  rayCountY,
        int                                  rayCountZ,
        const RHI::ShaderBindingTableRegion &raygenSbt,
        const RHI::ShaderBindingTableRegion &missSbt,
        const RHI::ShaderBindingTableRegion &hitSbt,
        const RHI::ShaderBindingTableRegion &callableSbt);
    
    void DispatchIndirect(const RC<SubBuffer> &buffer, size_t byteOffset);

    void DrawIndexedIndirect(
        const RC<SubBuffer> &buffer,
        uint32_t             drawCount,
        size_t               byteOffset,
        size_t               byteStride = sizeof(RHI::IndirectDrawIndexedArgument));

    // if prebuildInfo is not presented
    //   generate prebuildInfo
    // else
    //   assert prebuildInfo's compatibility
    // if as.buffer is not presented
    //   allocate buffer
    // else
    //   assert buffer is large enough
    // if scratchBuffer is not presented
    //   allocate scratchBuffer
    // else
    //   use given scratchBuffer

    void BuildBlas(
        const RC<Blas>                                &blas,
        Span<RHI::RayTracingGeometryDesc>              geometries,
        RHI::RayTracingAccelerationStructureBuildFlags flags,
        const RC<SubBuffer>                           &scratchBuffer);
    void BuildBlas(
        const RC<Blas>                   &blas,
        Span<RHI::RayTracingGeometryDesc> geometries,
        const BlasPrebuildInfo           &prebuildInfo,
        const RC<SubBuffer>              &scratchBuffer);

    void BuildTlas(
        const RC<Tlas>                                &tlas,
        const RHI::RayTracingInstanceArrayDesc        &instances,
        RHI::RayTracingAccelerationStructureBuildFlags flags,
        const RC<SubBuffer>                           &scratchBuffer);
    void BuildTlas(
        const RC<Tlas>                         &tlas,
        const RHI::RayTracingInstanceArrayDesc &instances,
        const TlasPrebuildInfo                 &prebuildInfo,
        const RC<SubBuffer>                    &scratchBuffer);

    void ExecuteBarriers(const BarrierBatch &barriers);

    void ExecuteBarrier(
        RHI::PipelineStageFlag  prevStages,
        RHI::ResourceAccessFlag prevAccesses,
        RHI::PipelineStageFlag  succStages,
        RHI::ResourceAccessFlag succAccesses);
    void ExecuteBarrier(
        const RC<StatefulBuffer> &buffer,
        RHI::PipelineStageFlag    stages,
        RHI::ResourceAccessFlag   accesses);
    void ExecuteBarrier(
        const RC<StatefulTexture> &texture,
        RHI::TextureLayout         layout,
        RHI::PipelineStageFlag     stages,
        RHI::ResourceAccessFlag    accesses);
    void ExecuteBarrier(
        const RC<StatefulTexture> &texture,
        uint32_t                   arrayLayer,
        uint32_t                   mipLevel,
        RHI::TextureLayout         layout,
        RHI::PipelineStageFlag     stages,
        RHI::ResourceAccessFlag    accesses);
    void ExecuteBarrier(
        const RC<Buffer>       &buffer,
        RHI::PipelineStageFlag  prevStages,
        RHI::ResourceAccessFlag prevAccesses,
        RHI::PipelineStageFlag  succStages,
        RHI::ResourceAccessFlag succAccesses);
    void ExecuteBarrier(
        const RC<Texture>      &texture,
        RHI::TextureLayout      prevLayout,
        RHI::PipelineStageFlag  prevStages,
        RHI::ResourceAccessFlag prevAccesses,
        RHI::TextureLayout      succLayout,
        RHI::PipelineStageFlag  succStages,
        RHI::ResourceAccessFlag succAccesses);
    void ExecuteBarrier(
        const RC<Texture>      &texture,
        uint32_t                arrayLayer,
        uint32_t                mipLevel,
        RHI::TextureLayout      prevLayout,
        RHI::PipelineStageFlag  prevStages,
        RHI::ResourceAccessFlag prevAccesses,
        RHI::TextureLayout      succLayout,
        RHI::PipelineStageFlag  succStages,
        RHI::ResourceAccessFlag succAccesses);
    void ExecuteBarrier(
        const RC<Texture> &texture,
        RHI::TextureLayout prevLayout,
        RHI::TextureLayout succLayout);
    void ExecuteBarrier(
        const RC<StatefulTexture> &texture,
        RHI::TextureLayout         prevLayout,
        RHI::TextureLayout         succLayout);

private:

    friend class CommandBufferManager;

    struct Pool
    {
        RHI::CommandPoolUPtr rhiPool;
        std::atomic<uint32_t> historyUserCount = 0;
        std::atomic<uint32_t> activeUserCount = 0;
    };

    void CheckThreadID() const;

    Device *device_;
    CommandBufferManager *manager_;
    RHI::QueueType queueType_;
    RHI::CommandBufferPtr rhiCommandBuffer_;
    Pool *pool_;
#if RTRC_DEBUG
    std::thread::id threadID_;
#endif

    // Temporary data

    RC<GraphicsPipeline> currentGraphicsPipeline_;
    RC<ComputePipeline> currentComputePipeline_;
    RC<RayTracingPipeline> currentRayTracingPipeline_;
};

class CommandBufferManager : public Uncopyable
{
public:

    CommandBufferManager(Device *device, DeviceSynchronizer &sync);
    ~CommandBufferManager();

    CommandBuffer Create();

    void _internalEndFrame();
    void _internalAllocate(CommandBuffer &commandBuffer);
    void _internalRelease(CommandBuffer &commandBuffer);

private:
    
    static constexpr int MAX_COMMAND_BUFFERS_IN_SINGLE_POOL = 8;

    struct PerThreadPoolData
    {
        // Ownership of full pool is transferred to CommandBuffer object
        CommandBuffer::Pool *activePool;
    };

    PerThreadPoolData &GetPerThreadPoolData();

    RHI::CommandPoolUPtr GetFreeCommandPool();

    Device *device_;
    DeviceSynchronizer &sync_;

    std::map<std::thread::id, PerThreadPoolData> threadToActivePoolData_;
    tbb::spin_rw_mutex threadToActivePoolDataMutex_;

    tbb::concurrent_queue<RHI::CommandPoolUPtr> freePools_;
};

template<typename T> requires std::is_trivially_copyable_v<T>
void CommandBuffer::SetGraphicsPushConstants(uint32_t rangeIndex, const T &data)
{
    this->SetGraphicsPushConstants(rangeIndex, 0, sizeof(data), &data);
}

template<typename T> requires std::is_trivially_copyable_v<T>
void CommandBuffer::SetComputePushConstants(uint32_t rangeIndex, const T &data)
{
    this->SetComputePushConstants(rangeIndex, 0, sizeof(data), &data);
}

inline void CommandBuffer::ExecuteBarrier(
    RHI::PipelineStageFlag  prevStages,
    RHI::ResourceAccessFlag prevAccesses,
    RHI::PipelineStageFlag  succStages,
    RHI::ResourceAccessFlag succAccesses)
{
    this->ExecuteBarriers(BarrierBatch(prevStages, prevAccesses, succStages, succAccesses));
}

inline void CommandBuffer::ExecuteBarrier(
    const RC<StatefulBuffer> &buffer,
    RHI::PipelineStageFlag    stages,
    RHI::ResourceAccessFlag   accesses)
{
    this->ExecuteBarriers(BarrierBatch{}(buffer, stages, accesses));
}

inline void CommandBuffer::ExecuteBarrier(
    const RC<StatefulTexture> &texture,
    RHI::TextureLayout         layout,
    RHI::PipelineStageFlag     stages,
    RHI::ResourceAccessFlag    accesses)
{
    this->ExecuteBarriers(BarrierBatch{}(texture, layout, stages, accesses));
}

inline void CommandBuffer::ExecuteBarrier(
    const RC<StatefulTexture> &texture,
    uint32_t                   arrayLayer,
    uint32_t                   mipLevel,
    RHI::TextureLayout         layout,
    RHI::PipelineStageFlag     stages,
    RHI::ResourceAccessFlag    accesses)
{
    this->ExecuteBarriers(BarrierBatch{}(texture, arrayLayer, mipLevel, layout, stages, accesses));
}

inline void CommandBuffer::ExecuteBarrier(
    const RC<Buffer>       &buffer,
    RHI::PipelineStageFlag  prevStages,
    RHI::ResourceAccessFlag prevAccesses,
    RHI::PipelineStageFlag  succStages,
    RHI::ResourceAccessFlag succAccesses)
{
    this->ExecuteBarriers(BarrierBatch{}(buffer, prevStages, prevAccesses, succStages, succAccesses));
}

inline void CommandBuffer::ExecuteBarrier(
    const RC<Texture>      &texture,
    RHI::TextureLayout      prevLayout,
    RHI::PipelineStageFlag  prevStages,
    RHI::ResourceAccessFlag prevAccesses,
    RHI::TextureLayout      succLayout,
    RHI::PipelineStageFlag  succStages,
    RHI::ResourceAccessFlag succAccesses)
{
    this->ExecuteBarriers(BarrierBatch{}(
        texture, prevLayout, prevStages, prevAccesses, succLayout, succStages, succAccesses));
}

inline void CommandBuffer::ExecuteBarrier(
    const RC<Texture>      &texture,
    uint32_t                arrayLayer,
    uint32_t                mipLevel,
    RHI::TextureLayout      prevLayout,
    RHI::PipelineStageFlag  prevStages,
    RHI::ResourceAccessFlag prevAccesses,
    RHI::TextureLayout      succLayout,
    RHI::PipelineStageFlag  succStages,
    RHI::ResourceAccessFlag succAccesses)
{
    this->ExecuteBarriers(BarrierBatch{}(
        texture, arrayLayer, mipLevel,
        prevLayout, prevStages, prevAccesses, succLayout, succStages, succAccesses));
}

inline void CommandBuffer::ExecuteBarrier(
    const RC<Texture> &texture,
    RHI::TextureLayout prevLayout,
    RHI::TextureLayout succLayout)
{
    this->ExecuteBarriers(BarrierBatch{}(texture, prevLayout, succLayout));
}

inline void CommandBuffer::ExecuteBarrier(
    const RC<StatefulTexture> &texture,
    RHI::TextureLayout         prevLayout,
    RHI::TextureLayout         succLayout)
{
    this->ExecuteBarriers(BarrierBatch{}(texture, prevLayout, succLayout));
}

RTRC_END
