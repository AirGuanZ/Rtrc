#pragma once

#include <Rtrc/RHI/DirectX12/Common.h>

RTRC_RHI_DIRECTX12_BEGIN

class DirectX12Device : public Device
{
public:

    DirectX12Device(const DeviceDesc &desc, ComPtr<ID3D12Device> device);

    Ptr<Queue> GetQueue(QueueType type) override;

    Ptr<Fence> CreateFence(bool signaled) override;

    Ptr<Swapchain> CreateSwapchain(const SwapchainDesc &desc, Window &window) override;

    Ptr<Semaphore> CreateSemaphoreA(uint64_t initialValue) override;

    Ptr<RawShader> CreateShader(const void *data, size_t size, std::string entryPoint, ShaderStage type) override;

    Ptr<GraphicsPipelineBuilder> CreateGraphicsPipelineBuilder() override;

    Ptr<ComputePipelineBuilder> CreateComputePipelineBuilder() override;

    Ptr<BindingGroupLayout> CreateBindingGroupLayout(const BindingGroupLayoutDesc &desc) override;

    Ptr<BindingLayout> CreateBindingLayout(const BindingLayoutDesc &desc) override;

    Ptr<Texture> CreateTexture2D(const Texture2DDesc &desc) override;

    Ptr<Buffer> CreateBuffer(const BufferDesc &desc) override;

    Ptr<Sampler> CreateSampler(const SamplerDesc &desc) override;

    Ptr<MemoryPropertyRequirements> GetMemoryRequirements(
        const Texture2DDesc &desc, size_t *size, size_t *alignment) const override;

    Ptr<MemoryPropertyRequirements> GetMemoryRequirements(
        const BufferDesc &desc, size_t *size, size_t *alignment) const override;

    Ptr<MemoryBlock> CreateMemoryBlock(const MemoryBlockDesc &desc) override;

    Ptr<Texture> CreatePlacedTexture2D(
        const Texture2DDesc &desc, const Ptr<MemoryBlock> &memoryBlock, size_t offsetInMemoryBlock) override;

    Ptr<Buffer> CreatePlacedBuffer(
        const BufferDesc &desc, const Ptr<MemoryBlock> &memoryBlock, size_t offsetInMemoryBlock) override;

    void WaitIdle() override;

private:

    ComPtr<ID3D12Device> device_;

    ComPtr<ID3D12CommandQueue> graphicsQueue_;
    ComPtr<ID3D12CommandQueue> computeQueue_;
    ComPtr<ID3D12CommandQueue> copyQueue_;
};

RTRC_RHI_DIRECTX12_END
