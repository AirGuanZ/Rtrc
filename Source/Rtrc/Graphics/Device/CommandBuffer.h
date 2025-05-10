#pragma once

#include <queue>

#include <Rtrc/Core/Container/ConcurrentQueue.h>
#include <Rtrc/Graphics/Device/BindingGroup.h>
#include <Rtrc/Graphics/Device/DeviceSynchronizer.h>
#include <Rtrc/Graphics/Device/Pipeline.h>
#include <Rtrc/Graphics/Device/Texture.h>
#include <Rtrc/Graphics/RenderGraph/Resource.h>

RTRC_BEGIN

class Buffer;
class CommandBuffer;
class DeviceCommandBufferManager;
class Device;
class Texture;

class Blas;
class Tlas;
class BlasPrebuildInfo;
class TlasPrebuildInfo;

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

    BarrierBatch& Add(const std::vector<RHI::TextureReleaseBarrier> &textureBarriers);
    BarrierBatch &Add(const std::vector<RHI::TextureAcquireBarrier> &textureBarriers);

private:

    friend class CommandBuffer;

    std::optional<RHI::GlobalMemoryBarrier>    G_;
    std::vector<RHI::BufferTransitionBarrier>  BT_;
    std::vector<RHI::TextureTransitionBarrier> TT_;
    std::vector<RHI::TextureReleaseBarrier>    TR_;
    std::vector<RHI::TextureAcquireBarrier>    TA_;
};

class CommandBufferManagerInterface
{
public:

    virtual ~CommandBufferManagerInterface() = default;

    virtual void _internalAllocate(CommandBuffer &commandBuffer) = 0;
    virtual void _internalRelease(CommandBuffer &commandBuffer) = 0;
};

struct RenderTargetBinding
{
    TextureRtv             RTV;
    RHI::AttachmentLoadOp  loadOp = RHI::AttachmentLoadOp::Load;
    RHI::AttachmentStoreOp storeOp = RHI::AttachmentStoreOp::Store;
    RHI::ColorClearValue   clearValue = { 0, 0, 0, 0 };

    // load & store

    static RenderTargetBinding Create(const RC<Texture> &texture);
    static RenderTargetBinding Create(TextureRtv rtv);
    static RenderTargetBinding Create(RGTexture texture);
    static RenderTargetBinding Create(RGTexRtv rtv);

    // clear & store. Use hinting clear value if not specified.

    static RenderTargetBinding CreateClear(const RC<Texture> &texture, const std::optional<RHI::ColorClearValue> &clearValue = {});
    static RenderTargetBinding CreateClear(TextureRtv rtv,             const std::optional<RHI::ColorClearValue> &clearValue = {});
    static RenderTargetBinding CreateClear(RGTexture texture,          const std::optional<RHI::ColorClearValue> &clearValue = {});
    static RenderTargetBinding CreateClear(RGTexRtv rtv,               const std::optional<RHI::ColorClearValue> &clearValue = {});
};

struct DepthStencilBinding
{
    TextureDsv                  DSV;
    RHI::AttachmentLoadOp       loadOp = RHI::AttachmentLoadOp::Load;
    RHI::AttachmentStoreOp      storeOp = RHI::AttachmentStoreOp::Store;
    RHI::DepthStencilClearValue clearValue;

    // load & store

    static DepthStencilBinding Create(const RC<Texture> &texture);                                             
    static DepthStencilBinding Create(TextureDsv dsv);
    static DepthStencilBinding Create(RGTexture texture);
    static DepthStencilBinding Create(RGTexDsv dsv);

    // clear & store

    static DepthStencilBinding CreateClear(const RC<Texture> &texture, const std::optional<RHI::DepthStencilClearValue> &clearValue = {});
    static DepthStencilBinding CreateClear(TextureDsv dsv,             const std::optional<RHI::DepthStencilClearValue> &clearValue = {});
    static DepthStencilBinding CreateClear(RGTexture texture,          const std::optional<RHI::DepthStencilClearValue> &clearValue = {});
    static DepthStencilBinding CreateClear(RGTexDsv dsv,               const std::optional<RHI::DepthStencilClearValue> &clearValue = {});
};

class CommandBuffer : public Uncopyable
{
public:

    CommandBuffer();
    ~CommandBuffer();

    CommandBuffer(CommandBuffer &&other) noexcept;
    CommandBuffer &operator=(CommandBuffer &&other) noexcept;

    void Swap(CommandBuffer &other) noexcept;

    RHI::QueueType                GetQueueType() const;
    const RHI::CommandBufferRPtr &GetRHIObject() const;
    Ref<Device>                   GetDevice() const;

    // Once begin, the command buffer object is bound with current thread, and cannot be used in any other thread.
    void Begin();
    void End();

    void BeginDebugEvent(std::string name, const std::optional<Vector4f> &color = std::nullopt);
    void EndDebugEvent();

    void CopyBuffer(
        const RC<Buffer> &dst, size_t dstOffset,
        const RC<Buffer> &src, size_t srcOffset, size_t size);
    void CopyColorTexture(
        const RC<Texture> &dst, uint32_t dstMipLevel, uint32_t dstArrayLayer,
        const RC<Texture> &src, uint32_t srcMipLevel, uint32_t srcArrayLayer);
    void CopyColorTexture(
        const RC<Texture> &dst, uint32_t dstMipLevel, uint32_t dstArrayLayer, const Vector3u &dstOffset,
        const RC<Texture> &src, uint32_t srcMipLevel, uint32_t srcArrayLayer, const Vector3u &srcOffset,
        const Vector3u &size);
    void CopyColorTexture2DToBuffer(
        const RC<Buffer> &dst, size_t dstOffset, size_t dstRowBytes,
        const RC<Texture> &src, uint32_t arrayLayer, uint32_t mipLevel);

    void CopyBuffer(
        RGBuffer dst, size_t dstOffset,
        RGBuffer src, size_t srcOffset, size_t size);
    void CopyColorTexture(
        RGTexture dst, uint32_t dstMipLevel, uint32_t dstArrayLayer,
        RGTexture src, uint32_t srcMipLevel, uint32_t srcArrayLayer);
    void CopyColorTexture(
        RGTexture dst, uint32_t dstMipLevel, uint32_t dstArrayLayer, const Vector3u &dstOffset,
        RGTexture src, uint32_t srcMipLevel, uint32_t srcArrayLayer, const Vector3u &srcOffset,
        const Vector3u &size);
    void CopyColorTexture2DToBuffer(
        RGBuffer dst, size_t dstOffset, size_t dstRowBytes,
        RGTexture src, uint32_t arrayLayer, uint32_t mipLevel);

    void BeginRenderPass(
        Span<RenderTargetBinding> colorAttachments,
        bool                   setViewportAndScissor = false);
    void BeginRenderPass(
        const DepthStencilBinding &depthStencilAttachment,
        bool                    setViewportAndScissor = false);
    void BeginRenderPass(
        Span<RenderTargetBinding>  colorAttachments,
        const DepthStencilBinding &depthStencilAttachment,
        bool                    setViewportAndScissor = false);
    void EndRenderPass();

    Span<RHI::Format> GetCurrentRenderPassColorFormats() const { assert(isInRenderPass_); return currentPassColorFormats_; }
    RHI::Format GetCurrentRenderPassDepthStencilFormat() const { assert(isInRenderPass_); return currentPassDepthStencilFormat_; }

    void BindGraphicsPipeline(const RC<GraphicsPipeline> &graphicsPipeline);
    void BindComputePipeline(const RC<ComputePipeline> &computePipeline);
    void BindRayTracingPipeline(const RC<RayTracingPipeline> &rayTracingPipeline);
    void BindWorkGraphPipeline(
        const RC<WorkGraphPipeline> &workGraphPipeline,
        RHI::BufferDeviceAddress     backingBuffer,
        size_t                       backBufferSize,
        bool                         initializeBackingBuffer);

    const GraphicsPipeline   *GetCurrentGraphicsPipeline() const;
    const ComputePipeline    *GetCurrentComputePipeline() const;
    const RayTracingPipeline *GetCurrentRayTracingPipeline() const;
    const WorkGraphPipeline  *GetCurrentWorkGraphPipeline() const;

    void BindGraphicsGroup(int index, const RC<BindingGroup> &group);
    void BindComputeGroup(int index, const RC<BindingGroup> &group);
    void BindRayTracingGroup(int index, const RC<BindingGroup> &group);
    void BindWorkGraphGroup(int index, const RC<BindingGroup> &group);
    
    void BindGraphicsGroups(Span<RC<BindingGroup>> groups);
    void BindComputeGroups(Span<RC<BindingGroup>> groups);
    void BindRayTracingGroups(Span<RC<BindingGroup>> groups);
    void BindWorkGraphGroups(Span<RC<BindingGroup>> groups);

    void SetViewports(Span<RHI::Viewport> viewports);
    void SetScissors(Span<RHI::Scissor> scissors);
    void SetViewportAndScissorAutomatically(); // Automatically set viewport and scissor with current RT size

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
    void ClearDepthStencil(const RC<Texture> &tex, float depth, uint8_t stencil);
    void ClearDepth(const RC<Texture> &tex, float depth);
    void ClearStencil(const RC<Texture> &tex, uint8_t stencil);
    
    void Draw(int vertexCount, int instanceCount, int firstVertex, int firstInstance);
    void DrawIndexed(int indexCount, int instanceCount, int firstIndex, int firstVertex, int firstInstance);
    void DispatchMesh(unsigned groupCountX, unsigned groupCountY, unsigned groupCountZ);
    void Dispatch(unsigned groupCountX, unsigned groupCountY, unsigned groupCountZ);
    void Dispatch(const Vector3u &groupCount);
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
    void Trace(
        unsigned int                         rayCountX,
        unsigned int                         rayCountY,
        unsigned int                         rayCountZ,
        const RHI::ShaderBindingTableRegion &raygenSbt,
        const RHI::ShaderBindingTableRegion &missSbt,
        const RHI::ShaderBindingTableRegion &hitSbt,
        const RHI::ShaderBindingTableRegion &callableSbt);
    
    void DispatchIndirect(const RC<SubBuffer> &buffer, size_t byteOffset);

    void DispatchNode(
        uint32_t    entryIndex,
        uint32_t    recordCount,
        uint32_t    recordStride,
        const void *records);

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

    friend class Queue;
    friend class DeviceCommandBufferManager;
    friend class SingleThreadCommandBufferManager;

    void CheckThreadID() const;

    template<typename T>
    void AddHoldObject(T object) { holdingObjects_.emplace_back(std::move(object)); }

    Device                        *device_;
    CommandBufferManagerInterface *manager_;
    RHI::QueueType                 queueType_;
    RHI::CommandBufferRPtr         rhiCommandBuffer_;

    void *managerCustomData_;
    
#if RTRC_DEBUG
    std::thread::id threadID_;
#endif

    RHI::QueueSessionID submitSessionID_ = RHI::INITIAL_QUEUE_SESSION_ID;

    // Temporary data

    RC<GraphicsPipeline>   currentGraphicsPipeline_;
    RC<ComputePipeline>    currentComputePipeline_;
    RC<RayTracingPipeline> currentRayTracingPipeline_;
    RC<WorkGraphPipeline>  currentWorkGraphPipeline_;

    bool                         isInRenderPass_ = false;
    Vector2u                     currentPassRenderTargetSize_;
    StaticVector<RHI::Format, 8> currentPassColorFormats_;
    RHI::Format                  currentPassDepthStencilFormat_ = RHI::Format::Unknown;

    std::vector<std::any> holdingObjects_;
};

class SingleThreadCommandBufferManager : public CommandBufferManagerInterface, public Uncopyable
{
public:

    SingleThreadCommandBufferManager(Device* device, RHI::QueueRPtr queue);
    ~SingleThreadCommandBufferManager() override;

    CommandBuffer Create();

    void _internalAllocate(CommandBuffer &commandBuffer) override;
    void _internalRelease(CommandBuffer &commandBuffer) override;

private:

    struct CommandPoolRecord
    {
        RHI::CommandPoolUPtr pool;
        RHI::QueueSessionID submitSessionID = RHI::INITIAL_QUEUE_SESSION_ID;

        auto operator<=>(const CommandPoolRecord &record) const
        {
            return std::make_tuple(submitSessionID, pool.Get()) <=>
                   std::make_tuple(record.submitSessionID, record.pool.Get());
        }
    };

    Device                     *device_;
    RHI::QueueRPtr              queue_;
    std::set<CommandPoolRecord> records_;
};

class DeviceCommandBufferManager : public CommandBufferManagerInterface, public Uncopyable
{
public:

    DeviceCommandBufferManager(Device *device, DeviceSynchronizer &sync);
    ~DeviceCommandBufferManager() override;

    CommandBuffer Create();

    void _internalEndFrame();
    void _internalAllocate(CommandBuffer &commandBuffer) override;
    void _internalRelease(CommandBuffer &commandBuffer) override;

private:
    
    static constexpr int MAX_COMMAND_BUFFERS_IN_SINGLE_POOL = 8;

    struct Pool
    {
        RHI::CommandPoolUPtr  rhiPool;
        std::atomic<uint32_t> historyUserCount = 0;
        std::atomic<uint32_t> activeUserCount = 0;
    };

    struct PerThreadPoolData
    {
        // Ownership of full pool is transferred to CommandBuffer object
        Pool *activePool;
    };

    PerThreadPoolData &GetPerThreadPoolData();
    RHI::CommandPoolUPtr GetFreeCommandPool();

    Device *device_;
    DeviceSynchronizer &sync_;

    std::map<std::thread::id, PerThreadPoolData> threadToActivePoolData_;
    std::shared_mutex threadToActivePoolDataMutex_;

    ConcurrentQueue<RHI::CommandPoolUPtr> freePools_;
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

inline RenderTargetBinding RenderTargetBinding::Create(const RC<Texture> &texture)
{
    return Create(texture->GetRtv());
}

inline RenderTargetBinding RenderTargetBinding::Create(TextureRtv rtv)
{
    RenderTargetBinding result;
    result.RTV = std::move(rtv);
    return result;
}

inline RenderTargetBinding RenderTargetBinding::Create(RGTexture texture)
{
    return Create(texture->Get());
}

inline RenderTargetBinding RenderTargetBinding::Create(RGTexRtv rtv)
{
    return Create(rtv.GetRtv());
}

inline RenderTargetBinding RenderTargetBinding::CreateClear(const RC<Texture> &texture, const std::optional<RHI::ColorClearValue> &clearValue)
{
    return CreateClear(texture->GetRtv(), clearValue);
}

inline RenderTargetBinding RenderTargetBinding::CreateClear(TextureRtv rtv, const std::optional<RHI::ColorClearValue> &clearValue)
{
    RenderTargetBinding result;
    result.loadOp = RHI::AttachmentLoadOp::Clear;
    if(clearValue)
    {
        result.clearValue = *clearValue;
    }
    else if(auto *hintColorClearValue = rtv.GetTexture()->GetDesc().clearValue.AsIf<RHI::ColorClearValue>())
    {
        result.clearValue = *hintColorClearValue;
    }
    result.RTV = std::move(rtv);
    return result;
}

inline RenderTargetBinding RenderTargetBinding::CreateClear(RGTexture texture, const std::optional<RHI::ColorClearValue> &clearValue)
{
    return CreateClear(texture->Get(), clearValue);
}

inline RenderTargetBinding RenderTargetBinding::CreateClear(RGTexRtv rtv, const std::optional<RHI::ColorClearValue> &clearValue)
{
    return CreateClear(rtv.GetRtv(), clearValue);
}

inline DepthStencilBinding DepthStencilBinding::Create(const RC<Texture> &texture)
{
    return Create(texture->GetDsv());
}

inline DepthStencilBinding DepthStencilBinding::Create(TextureDsv dsv)
{
    DepthStencilBinding result;
    result.DSV = std::move(dsv);
    return result;
}

inline DepthStencilBinding DepthStencilBinding::Create(RGTexture texture)
{
    return Create(texture->Get());
}

inline DepthStencilBinding DepthStencilBinding::Create(RGTexDsv dsv)
{
    return Create(dsv.GetDsv());
}

inline DepthStencilBinding DepthStencilBinding::CreateClear(const RC<Texture> &texture, const std::optional<RHI::DepthStencilClearValue> &clearValue)
{
    return CreateClear(texture->GetDsv(), clearValue);
}

inline DepthStencilBinding DepthStencilBinding::CreateClear(TextureDsv dsv, const std::optional<RHI::DepthStencilClearValue> &clearValue)
{
    DepthStencilBinding result;
    result.loadOp = RHI::AttachmentLoadOp::Clear;
    if(clearValue)
    {
        result.clearValue = *clearValue;
    }
    else if(auto *hintDepthStencilClearValue = dsv.GetTexture()->GetDesc().clearValue.AsIf<RHI::DepthStencilClearValue>())
    {
        result.clearValue = *hintDepthStencilClearValue;
    }
    result.DSV = std::move(dsv);
    return result;
}

inline DepthStencilBinding DepthStencilBinding::CreateClear(RGTexture texture, const std::optional<RHI::DepthStencilClearValue> &clearValue)
{
    return CreateClear(texture->Get(), clearValue);
}

inline DepthStencilBinding DepthStencilBinding::CreateClear(RGTexDsv dsv, const std::optional<RHI::DepthStencilClearValue> &clearValue)
{
    return CreateClear(dsv.GetDsv(), clearValue);
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
