#pragma once

#include <mutex>

#include <Rtrc/Graphics/RenderGraph/TransientResourceManager.h>
#include <Rtrc/Graphics/Resource/ConstantBuffer.h>
#include <Rtrc/Graphics/Resource/FrameCommandBufferManager.h>
#include <Rtrc/Graphics/Resource/FrameSynchronizer.h>

RTRC_BEGIN

/* Usage:
    while(!done)
    {
        // Call this before anything related to GPU
        resourceManager.BeginFrame();
        ...
        // Signal frame fence when submitting last GPU work in this frame
        queue.submit(..., resourceManager.GetFrameFence());
    }
*/
class ResourceManager : public Uncopyable, public CommandBufferAllocator
{
public:

    ResourceManager(RHI::DevicePtr device, int frameCount);
    ~ResourceManager() override;

    const RHI::DevicePtr &GetDeviceWithFrameResourceProtection() const;

    void BeginFrame();

    RHI::FencePtr GetFrameFence() const;

    int GetFrameIndex() const;

    RHI::CommandBufferPtr AllocateCommandBuffer(RHI::QueueType type) override;

    FrameConstantBuffer AllocateFrameConstantBuffer(size_t size);

    template<CBDetail::ConstantBufferStruct T>
    FrameConstantBuffer AllocateFrameConstantBuffer();

    PersistentConstantBuffer AllocatePersistentConstantBuffer(size_t size);

    template<CBDetail::ConstantBufferStruct T>
    PersistentConstantBuffer AllocatePersistentConstantBuffer();

    void OnGPUFrameEnd(std::function<void()> func);

    RG::TransientResourceManager *GetTransicentResourceManager() const;

    void RegisterFrameResourceProtection(RHI::RHIObjectPtr object);

private:

    RHI::DevicePtr                   device_;
    RHI::DevicePtr                   deviceWithFrameResourceProtection_;
    FrameSynchronizer                synchronizer_;
    FrameCommandBufferManager        commandBufferManager_;
    FrameConstantBufferAllocator     frameConstantBufferManager_;
    PersistentConstantBufferManager  persistentConstantBufferManager_;
    RC<RG::TransientResourceManager> transientResourceManager_;

    std::set<RHI::Ptr<RHI::RHIObject>> resources_;
    std::mutex resourcesMutex_;
};

template <CBDetail::ConstantBufferStruct T>
FrameConstantBuffer ResourceManager::AllocateFrameConstantBuffer()
{
    return frameConstantBufferManager_.AllocateConstantBuffer<T>();
}

template <CBDetail::ConstantBufferStruct T>
PersistentConstantBuffer ResourceManager::AllocatePersistentConstantBuffer()
{
    return persistentConstantBufferManager_.AllocateConstantBuffer<T>();
}

RTRC_END
