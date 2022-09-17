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
        frameResources.BeginFrame();
        ...
        // Signal frame fence when submitting last GPU work in this frame
        queue.submit(..., frameResources.GetFrameFence());
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

    Box<ConstantBuffer> AllocateConstantBuffer(size_t size);

    template<CBDetail::ConstantBufferStruct T>
    Box<ConstantBuffer> AllocateConstantBuffer();

    void OnGPUFrameEnd(std::function<void()> func);

    RG::TransientResourceManager *GetTransicentResourceManager() const;

    void RegisterFrameResourceProtection(RHI::RHIObjectPtr object);

private:

    RHI::DevicePtr                   device_;
    RHI::DevicePtr                   deviceWithFrameResourceProtection_;
    FrameSynchronizer                synchronizer_;
    FrameCommandBufferManager        commandBufferManager_;
    FrameConstantBufferAllocator     constantBufferManager_;
    RC<RG::TransientResourceManager> transientResourceManager_;

    std::set<RHI::Ptr<RHI::RHIObject>> resources_;
    std::mutex resourcesMutex_;
};

template <CBDetail::ConstantBufferStruct T>
Box<ConstantBuffer> ResourceManager::AllocateConstantBuffer()
{
    return constantBufferManager_.AllocateConstantBuffer<T>();
}

RTRC_END
