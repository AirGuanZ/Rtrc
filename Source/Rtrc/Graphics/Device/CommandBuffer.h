#pragma once

#include <tbb/concurrent_queue.h>
#include <tbb/spin_rw_mutex.h>

#include <Rtrc/Graphics/Device/BindingGroup.h>
#include <Rtrc/Graphics/Device/Buffer.h>
#include <Rtrc/Graphics/Device/DeviceSynchronizer.h>
#include <Rtrc/Graphics/Device/Pipeline.h>
#include <Rtrc/Graphics/Device/Texture.h>

RTRC_BEGIN

class Buffer;
class CommandBuffer;
class CommandBufferManager;
class Device;
class Mesh;
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

private:

    friend class CommandBuffer;

    std::vector<RHI::BufferTransitionBarrier> BT_;
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

    void CopyBuffer(Buffer &dst, size_t dstOffset, const Buffer &src, size_t srcOffset, size_t size);
    void CopyColorTexture2DToBuffer(
        Buffer &dst, size_t dstOffset, Texture &src, uint32_t arrayLayer, uint32_t mipLevel);

    void BeginRenderPass(Span<ColorAttachment> colorAttachments);
    void BeginRenderPass(const DepthStencilAttachment &depthStencilAttachment);
    void BeginRenderPass(Span<ColorAttachment> colorAttachments, const DepthStencilAttachment &depthStencilAttachment);
    void EndRenderPass();

    void BindGraphicsPipeline(const RC<GraphicsPipeline> &graphicsPipeline);
    void BindComputePipeline(const RC<ComputePipeline> &computePipeline);

    const GraphicsPipeline *GetCurrentGraphicsPipeline() const;
    const ComputePipeline *GetCurrentComputePipeline() const;

    void BindGraphicsGroup(int index, const RC<BindingGroup> &group);
    void BindComputeGroup(int index, const RC<BindingGroup> &group);
    
    void SetViewports(Span<Viewport> viewports);
    void SetScissors(Span<Scissor> scissors);

    void SetVertexBuffers(int slot, Span<RC<Buffer>> buffers, Span<size_t> byteOffsets = {});
    void SetIndexBuffer(const RC<Buffer> &buffer, RHI::IndexBufferFormat format, size_t byteOffset = 0);
    void BindMesh(const Mesh &mesh);

    void SetStencilReferenceValue(uint8_t value);

    void SetGraphicsPushConstantRange(RHI::ShaderStageFlag stages, uint32_t offset, uint32_t size, const void *data);
    void SetComputePushConstantRange(RHI::ShaderStageFlag stages, uint32_t offset, uint32_t size, const void *data);

    void SetGraphicsPushConstantRange(uint32_t rangeIndex, const void *data);
    void SetComputePushConstantRange(uint32_t rangeIndex, const void *data);

    void SetGraphicsPushConstants(const void *data, uint32_t size);
    void SetComputePushConstants(const void *data, uint32_t size);

    void SetGraphicsPushConstants(Span<unsigned char> data);
    void SetComputePushConstants(Span<unsigned char> data);

    template<typename T> requires std::is_trivially_copyable_v<T>
    void SetGraphicsPushConstants(const T &data);
    template<typename T> requires std::is_trivially_copyable_v<T>
    void SetComputePushConstants(const T &data);

    void ClearColorTexture2D(const RC<Texture> &tex, const Vector4f &color);

    void Draw(int vertexCount, int instanceCount, int firstVertex, int firstInstance);
    void DrawIndexed(int indexCount, int instanceCount, int firstIndex, int firstVertex, int firstInstance);
    void Dispatch(int groupCountX, int groupCountY, int groupCountZ);
    void Dispatch(const Vector3i &groupCount);

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
        const RC<Blas>                               &blas,
        Span<RHI::RayTracingGeometryDesc>             geometries,
        RHI::RayTracingAccelerationStructureBuildFlag flags,
        const RC<SubBuffer>                          &scratchBuffer);
    void BuildBlas(
        const RC<Blas>                   &blas,
        Span<RHI::RayTracingGeometryDesc> geometries,
        const BlasPrebuildInfo           &prebuildInfo,
        const RC<SubBuffer>              &scratchBuffer);

    void BuildTlas(
        const RC<Tlas>                               &tlas,
        Span<RHI::RayTracingInstanceArrayDesc>        instanceArrays,
        RHI::RayTracingAccelerationStructureBuildFlag flags,
        const RC<SubBuffer>                          &scratchBuffer);
    void BuildTlas(
        const RC<Tlas>                        &tlas,
        Span<RHI::RayTracingInstanceArrayDesc> instanceArrays,
        const TlasPrebuildInfo                &prebuildInfo,
        const RC<SubBuffer>                   &scratchBuffer);

    void ExecuteBarriers(const BarrierBatch &barriers);
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
        RHI::CommandPoolPtr rhiPool;
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

    RHI::CommandPoolPtr GetFreeCommandPool();

    Device *device_;
    DeviceSynchronizer &sync_;

    std::map<std::thread::id, PerThreadPoolData> threadToActivePoolData_;
    tbb::spin_rw_mutex threadToActivePoolDataMutex_;

    tbb::concurrent_queue<RHI::CommandPoolPtr> freePools_;
};

template<typename T> requires std::is_trivially_copyable_v<T>
void CommandBuffer::SetGraphicsPushConstants(const T &data)
{
    this->SetGraphicsPushConstants(&data, sizeof(data));
}

template<typename T> requires std::is_trivially_copyable_v<T>
void CommandBuffer::SetComputePushConstants(const T &data)
{
    this->SetComputePushConstants(&data, sizeof(data));
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
