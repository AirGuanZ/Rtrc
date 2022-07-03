#include <Rtrc/RHI/DirectX12/Context/Device.h>
#include <Rtrc/RHI/DirectX12/Queue/Queue.h>
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

RC<Queue> DirectX12Device::GetQueue(QueueType type)
{
    switch(type)
    {
    case QueueType::Graphics: return MakeRC<DirectX12Queue>(type, graphicsQueue_);
    case QueueType::Compute:  return MakeRC<DirectX12Queue>(type, computeQueue_);
    case QueueType::Transfer: return MakeRC<DirectX12Queue>(type, copyQueue_);
    }
    Unreachable();
}

RC<Fence> DirectX12Device::CreateFence(bool signaled)
{
    // TODO
    return {};
}

RC<Swapchain> DirectX12Device::CreateSwapchain(const SwapchainDesc &desc, Window &window)
{
    // TODO
    return {};
}

RC<RawShader> DirectX12Device::CreateShader(const void *data, size_t size, std::string entryPoint, ShaderStage type)
{
    // TODO
    return {};
}

RC<GraphicsPipelineBuilder> DirectX12Device::CreateGraphicsPipelineBuilder()
{
    // TODO
    return {};
}

RC<ComputePipelineBuilder> DirectX12Device::CreateComputePipelineBuilder()
{
    // TODO
    return {};
}

RC<BindingGroupLayout> DirectX12Device::CreateBindingGroupLayout(const BindingGroupLayoutDesc *desc)
{
    // TODO
    return {};
}

RC<BindingLayout> DirectX12Device::CreateBindingLayout(const BindingLayoutDesc &desc)
{
    // TODO
    return {};
}

RC<Texture> DirectX12Device::CreateTexture2D(const Texture2DDesc &desc)
{
    // TODO
    return {};
}

RC<Buffer> DirectX12Device::CreateBuffer(const BufferDesc &desc)
{
    // TODO
    return {};
}

RC<Sampler> DirectX12Device::CreateSampler(const SamplerDesc &desc)
{
    // TODO
    return {};
}

void DirectX12Device::WaitIdle()
{
    // TODO
}

RTRC_RHI_DIRECTX12_END
