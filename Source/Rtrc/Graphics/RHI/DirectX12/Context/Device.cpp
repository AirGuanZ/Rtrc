#include <Rtrc/Graphics/RHI/DirectX12/Context/Device.h>
#include <Rtrc/Graphics/RHI/DirectX12/Queue/Queue.h>
#include <Rtrc/Utils/Unreachable.h>

RTRC_RHI_DIRECTX12_BEGIN

DirectX12Device::DirectX12Device(const DeviceDesc &desc, ComPtr<ID3D12Device> device)
    : device_(std::move(device))
{
    auto createQueue = [&](D3D12_COMMAND_LIST_TYPE type)
    {
        const D3D12_COMMAND_QUEUE_DESC queueDesc = {
            .Type     = type,
            .Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL
        };
        ComPtr<ID3D12CommandQueue> queue;
        D3D12_FAIL_MSG(
            device_->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(queue.GetAddressOf())),
            "failed to create directx12 command queue");
        return queue;
    };

    if(desc.graphicsQueue)
    {
        graphicsQueue_ = createQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
    }

    if(desc.computeQueue)
    {
        computeQueue_ = createQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE);
    }

    if(desc.transferQueue)
    {
        computeQueue_ = createQueue(D3D12_COMMAND_LIST_TYPE_COPY);
    }
}

Ptr<Queue> DirectX12Device::GetQueue(QueueType type)
{
    switch(type)
    {
    case QueueType::Graphics: return MakePtr<DirectX12Queue>(type, graphicsQueue_);
    case QueueType::Compute:  return MakePtr<DirectX12Queue>(type, computeQueue_);
    case QueueType::Transfer: return MakePtr<DirectX12Queue>(type, copyQueue_);
    }
    Unreachable();
}

Ptr<CommandPool> DirectX12Device::CreateCommandPool(const Ptr<Queue> &queue)
{
    // TODO
    return {};
}

Ptr<Fence> DirectX12Device::CreateFence(bool signaled)
{
    // TODO
    return {};
}

Ptr<Swapchain> DirectX12Device::CreateSwapchain(const SwapchainDesc &desc, Window &window)
{
    // TODO
    return {};
}

Ptr<Semaphore> DirectX12Device::CreateSemaphoreA(uint64_t initialValue)
{
    // TODO
    return {};
}

Ptr<RawShader> DirectX12Device::CreateShader(const void *data, size_t size, std::string entryPoint, ShaderStage type)
{
    // TODO
    return {};
}

Ptr<GraphicsPipeline> DirectX12Device::CreateGraphicsPipeline(const GraphicsPipelineDesc &desc)
{
    // TODO
    return {};
}

Ptr<ComputePipeline> DirectX12Device::CreateComputePipeline(const ComputePipelineDesc &desc)
{
    // TODO
    return {};
}

Ptr<BindingGroupLayout> DirectX12Device::CreateBindingGroupLayout(const BindingGroupLayoutDesc &desc)
{
    // TODO
    return {};
}

Ptr<BindingGroup> DirectX12Device::CreateBindingGroup(const Ptr<BindingGroupLayout> &bindingGroupLayout)
{
    // TODO
    return {};
}

Ptr<BindingLayout> DirectX12Device::CreateBindingLayout(const BindingLayoutDesc &desc)
{
    // TODO
    return {};
}

Ptr<Texture> DirectX12Device::CreateTexture(const TextureDesc &desc)
{
    // TODO
    return {};
}

Ptr<Buffer> DirectX12Device::CreateBuffer(const BufferDesc &desc)
{
    // TODO
    return {};
}

Ptr<Sampler> DirectX12Device::CreateSampler(const SamplerDesc &desc)
{
    // TODO
    return {};
}

Ptr<MemoryPropertyRequirements> DirectX12Device::GetMemoryRequirements(
    const TextureDesc &desc, size_t *size, size_t *alignment) const
{
    // TODO
    return {};
}

Ptr<MemoryPropertyRequirements> DirectX12Device::GetMemoryRequirements(
    const BufferDesc &desc, size_t *size, size_t *alignment) const
{
    // TODO
    return {};
}

Ptr<MemoryBlock> DirectX12Device::CreateMemoryBlock(const MemoryBlockDesc &desc)
{
    // TODO
    return {};
}

Ptr<Texture> DirectX12Device::CreatePlacedTexture(
    const TextureDesc &desc, const Ptr<MemoryBlock> &memoryBlock, size_t offsetInMemoryBlock)
{
    // TODO
    return {};
}

Ptr<Buffer> DirectX12Device::CreatePlacedBuffer(
    const BufferDesc &desc, const Ptr<MemoryBlock> &memoryBlock, size_t offsetInMemoryBlock)
{
    // TODO
    return {};
}

size_t DirectX12Device::GetConstantBufferAlignment() const
{
    // TODO
    return 0;
}

void DirectX12Device::WaitIdle()
{
    // TODO
}

RTRC_RHI_DIRECTX12_END
