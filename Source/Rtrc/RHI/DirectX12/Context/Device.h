#pragma once

#include <Rtrc/RHI/DirectX12/Common.h>

RTRC_RHI_DIRECTX12_BEGIN

class DirectX12Device : public Device
{
public:

    DirectX12Device(const DeviceDesc &desc, ComPtr<ID3D12Device> device);

    RC<Queue> GetQueue(QueueType type) override;

    RC<Fence> CreateFence(bool signaled) override;

    RC<Swapchain> CreateSwapchain(const SwapchainDesc &desc, Window &window) override;

    RC<RawShader> CreateShader(const void *data, size_t size, std::string entryPoint, ShaderStage type) override;

    RC<GraphicsPipelineBuilder> CreateGraphicsPipelineBuilder() override;

    RC<ComputePipelineBuilder> CreateComputePipelineBuilder() override;

    RC<BindingGroupLayout> CreateBindingGroupLayout(const BindingGroupLayoutDesc *desc) override;

    RC<BindingLayout> CreateBindingLayout(const BindingLayoutDesc &desc) override;

    RC<Texture> CreateTexture2D(const Texture2DDesc &desc) override;

    RC<Buffer> CreateBuffer(const BufferDesc &desc) override;

    RC<Sampler> CreateSampler(const SamplerDesc &desc) override;

    void WaitIdle() override;

private:

    ComPtr<ID3D12Device> device_;

    ComPtr<ID3D12CommandQueue> graphicsQueue_;
    ComPtr<ID3D12CommandQueue> computeQueue_;
    ComPtr<ID3D12CommandQueue> copyQueue_;
};

RTRC_RHI_DIRECTX12_END
