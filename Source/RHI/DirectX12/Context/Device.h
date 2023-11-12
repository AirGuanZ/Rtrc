#pragma once

#include <RHI/DirectX12/Context/CPUDescriptorHandleManager.h>
#include <RHI/DirectX12/Context/DescriptorHeap.h>

RTRC_RHI_D3D12_BEGIN

class DirectX12BindingGroup;
class DirectX12Queue;

RTRC_RHI_IMPLEMENT(DirectX12Device, Device)
{
public:

    struct Queues
    {
        ComPtr<ID3D12CommandQueue> graphicsQueue;
        ComPtr<ID3D12CommandQueue> computeQueue;
        ComPtr<ID3D12CommandQueue> copyQueue;
        // ComPtr<ID3D12CommandQueue> presentQueue;
    };

    RTRC_D3D12_IMPL_SET_NAME(device_)

    DirectX12Device(
        ComPtr<ID3D12Device5> device, const Queues &queues, ComPtr<IDXGIFactory4> factory, ComPtr<IDXGIAdapter> adapter);
    ~DirectX12Device() override;

    BackendType GetBackendType() const RTRC_RHI_OVERRIDE { return BackendType::DirectX12; }
    bool IsGlobalBarrierWellSupported() const RTRC_RHI_OVERRIDE { return false; }
    BarrierMemoryModel GetBarrierMemoryModel() const RTRC_RHI_OVERRIDE { return BarrierMemoryModel::Undefined; }

    RPtr<Queue> GetQueue(QueueType type) RTRC_RHI_OVERRIDE;

    UPtr<CommandPool> CreateCommandPool(const RPtr<Queue> &queue) RTRC_RHI_OVERRIDE;

    UPtr<Swapchain> CreateSwapchain(const SwapchainDesc &desc, Window &window) RTRC_RHI_OVERRIDE;

    UPtr<Fence>     CreateFence(bool signaled) RTRC_RHI_OVERRIDE;
    UPtr<Semaphore> CreateTimelineSemaphore(uint64_t initialValue) RTRC_RHI_OVERRIDE;

    UPtr<RawShader> CreateShader(const void *data, size_t size, std::vector<RawShaderEntry> entries) RTRC_RHI_OVERRIDE;

    UPtr<GraphicsPipeline>   CreateGraphicsPipeline(const GraphicsPipelineDesc &desc) RTRC_RHI_OVERRIDE;
    UPtr<ComputePipeline>    CreateComputePipeline(const ComputePipelineDesc &desc) RTRC_RHI_OVERRIDE;
    UPtr<RayTracingPipeline> CreateRayTracingPipeline(const RayTracingPipelineDesc &desc) RTRC_RHI_OVERRIDE;

    UPtr<RayTracingLibrary> CreateRayTracingLibrary(const RayTracingLibraryDesc &desc) RTRC_RHI_OVERRIDE;

    UPtr<BindingGroupLayout> CreateBindingGroupLayout(const BindingGroupLayoutDesc &desc) RTRC_RHI_OVERRIDE;
    UPtr<BindingGroup> CreateBindingGroup(
        const OPtr<BindingGroupLayout> &bindingGroupLayout, uint32_t variableArraySize) RTRC_RHI_OVERRIDE;
    UPtr<BindingLayout> CreateBindingLayout(const BindingLayoutDesc &desc) RTRC_RHI_OVERRIDE;

    void UpdateBindingGroups(const BindingGroupUpdateBatch &batch) RTRC_RHI_OVERRIDE;

    void CopyBindingGroup(
        const BindingGroupOPtr &dstGroup, uint32_t dstIndex, uint32_t dstArrayOffset,
        const BindingGroupOPtr &srcGroup, uint32_t srcIndex, uint32_t srcArrayOffset,
        uint32_t count) RTRC_RHI_OVERRIDE;

    UPtr<Texture> CreateTexture(const TextureDesc &desc) RTRC_RHI_OVERRIDE;
    UPtr<Buffer>  CreateBuffer(const BufferDesc &desc) RTRC_RHI_OVERRIDE;
    UPtr<Sampler> CreateSampler(const SamplerDesc &desc) RTRC_RHI_OVERRIDE;
    
    size_t GetConstantBufferAlignment() const RTRC_RHI_OVERRIDE;
    size_t GetConstantBufferSizeAlignment() const RTRC_RHI_OVERRIDE;
    size_t GetAccelerationStructureScratchBufferAlignment() const RTRC_RHI_OVERRIDE;
    size_t GetTextureBufferCopyRowPitchAlignment(Format texelFormat) const RTRC_RHI_OVERRIDE;

    void WaitIdle() RTRC_RHI_OVERRIDE;

    BlasUPtr CreateBlas(const BufferRPtr &buffer, size_t offset, size_t size) RTRC_RHI_OVERRIDE;
    TlasUPtr CreateTlas(const BufferRPtr &buffer, size_t offset, size_t size) RTRC_RHI_OVERRIDE;

    BlasPrebuildInfoUPtr CreateBlasPrebuildInfo(
        Span<RayTracingGeometryDesc>              geometries,
        RayTracingAccelerationStructureBuildFlags flags) RTRC_RHI_OVERRIDE;
    TlasPrebuildInfoUPtr CreateTlasPrebuildInfo(
        const RayTracingInstanceArrayDesc        &instances,
        RayTracingAccelerationStructureBuildFlags flags) RTRC_RHI_OVERRIDE;

    const ShaderGroupRecordRequirements &GetShaderGroupRecordRequirements() const RTRC_RHI_OVERRIDE;
    const WarpSizeInfo &GetWarpSizeInfo() const RTRC_RHI_OVERRIDE;

    UPtr<TransientResourcePool> CreateTransientResourcePool() RTRC_RHI_OVERRIDE { return nullptr; }

    ID3D12Device5 *_internalGetNativeDevice() const { return device_.Get(); }

    D3D12_CPU_DESCRIPTOR_HANDLE _internalAllocateCPUDescriptorHandle_CbvSrvUav();
    D3D12_CPU_DESCRIPTOR_HANDLE _internalAllocateCPUDescriptorHandle_Sampler();
    D3D12_CPU_DESCRIPTOR_HANDLE _internalAllocateCPUDescriptorHandle_Rtv();
    D3D12_CPU_DESCRIPTOR_HANDLE _internalAllocateCPUDescriptorHandle_Dsv();

    void _internalFreeCPUDescriptorHandle_CbvSrvUav(D3D12_CPU_DESCRIPTOR_HANDLE handle);
    void _internalFreeCPUDescriptorHandle_Sampler(D3D12_CPU_DESCRIPTOR_HANDLE handle);
    void _internalFreeCPUDescriptorHandle_Rtv(D3D12_CPU_DESCRIPTOR_HANDLE handle);
    void _internalFreeCPUDescriptorHandle_Dsv(D3D12_CPU_DESCRIPTOR_HANDLE handle);

    bool _internalAllocateGPUDescriptorHandles_CbvSrvUav(uint32_t count, DescriptorTable &result);
    bool _internalAllocateGPUDescriptorHandles_Sampler(uint32_t count, DescriptorTable &result);

    void _internalFreeGPUDescriptorHandles_CbvSrvUav(const DescriptorTable &table);
    void _internalFreeGPUDescriptorHandles_Sampler(const DescriptorTable &table);

    ID3D12DescriptorHeap *_internalGetGPUDescriptorHeap_CbvSrvUav() const;
    ID3D12DescriptorHeap *_internalGetGPUDescriptorHeap_Sampler() const;

    uint32_t _internalGetDescriptorHandleIncrementSize_CbvSrvUav() const;
    uint32_t _internalGetDescriptorHandleIncrementSize_Sampler() const;

    void _internalFreeBindingGroup(DirectX12BindingGroup &bindingGroup);

    D3D12_CPU_DESCRIPTOR_HANDLE _internalOffsetDescriptorHandle(
        const D3D12_CPU_DESCRIPTOR_HANDLE &start, size_t offset, bool isSampler) const;
    
    ID3D12CommandSignature *_internalGetIndirectDispatchCommandSignature() const;
    ID3D12CommandSignature *_internalGetIndirectDrawIndexedCommandSignature() const;

    const D3D12_FEATURE_DATA_D3D12_OPTIONS &_internalGetFeatureOptions() const;

    const ComPtr<D3D12MA::Allocator> &_internalGetAllocator() const { return allocator_; }

private:

    ComPtr<ID3D12Device5>  device_;
    ComPtr<IDXGIFactory4>  factory_;
    ComPtr<IDXGIAdapter>   adapter_;

    ComPtr<D3D12MA::Allocator> allocator_;

    Box<CPUDescriptorHandleManager> cpuDescHeap_CbvSrvUav_;
    Box<CPUDescriptorHandleManager> cpuDescHeap_Sampler_;
    Box<CPUDescriptorHandleManager> cpuDescHeap_Rtv_;
    Box<CPUDescriptorHandleManager> cpuDescHeap_Dsv_;

    Box<DescriptorHeap> gpuDescHeap_CbvSrvUav_;
    Box<DescriptorHeap> gpuDescHeap_Sampler_;

    RPtr<DirectX12Queue> graphicsQueue_;
    RPtr<DirectX12Queue> computeQueue_;
    RPtr<DirectX12Queue> copyQueue_;
    
    ComPtr<ID3D12CommandSignature> indirectDispatchCommandSignature_;
    ComPtr<ID3D12CommandSignature> indirectDrawIndexedCommandSignature_;

    ShaderGroupRecordRequirements shaderGroupRecordRequirements_;
    WarpSizeInfo warpSizeInfo_;

    uint32_t srvUavCbvDescriptorSize_;
    uint32_t samplerDescriptorSize_;

    D3D12_FEATURE_DATA_D3D12_OPTIONS featureOptions_;
};

inline D3D12_CPU_DESCRIPTOR_HANDLE DirectX12Device::_internalAllocateCPUDescriptorHandle_CbvSrvUav()
{
    return cpuDescHeap_CbvSrvUav_->Allocate();
}

inline D3D12_CPU_DESCRIPTOR_HANDLE DirectX12Device::_internalAllocateCPUDescriptorHandle_Sampler()
{
    return cpuDescHeap_Sampler_->Allocate();
}

inline D3D12_CPU_DESCRIPTOR_HANDLE DirectX12Device::_internalAllocateCPUDescriptorHandle_Rtv()
{
    return cpuDescHeap_Rtv_->Allocate();
}

inline D3D12_CPU_DESCRIPTOR_HANDLE DirectX12Device::_internalAllocateCPUDescriptorHandle_Dsv()
{
    return cpuDescHeap_Dsv_->Allocate();
}

inline void DirectX12Device::_internalFreeCPUDescriptorHandle_CbvSrvUav(D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
    cpuDescHeap_CbvSrvUav_->Free(handle);
}

inline void DirectX12Device::_internalFreeCPUDescriptorHandle_Sampler(D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
    cpuDescHeap_Sampler_->Free(handle);
}

inline void DirectX12Device::_internalFreeCPUDescriptorHandle_Rtv(D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
    cpuDescHeap_Rtv_->Free(handle);
}

inline void DirectX12Device::_internalFreeCPUDescriptorHandle_Dsv(D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
    cpuDescHeap_Dsv_->Free(handle);
}

inline bool DirectX12Device::_internalAllocateGPUDescriptorHandles_CbvSrvUav(uint32_t count, DescriptorTable &result)
{
    return gpuDescHeap_CbvSrvUav_->Allocate(count, result);
}

inline bool DirectX12Device::_internalAllocateGPUDescriptorHandles_Sampler(uint32_t count, DescriptorTable &result)
{
    return gpuDescHeap_Sampler_->Allocate(count, result);
}

inline void DirectX12Device::_internalFreeGPUDescriptorHandles_CbvSrvUav(const DescriptorTable &table)
{
    gpuDescHeap_CbvSrvUav_->Free(table);
}

inline void DirectX12Device::_internalFreeGPUDescriptorHandles_Sampler(const DescriptorTable &table)
{
    gpuDescHeap_Sampler_->Free(table);
}

inline ID3D12DescriptorHeap *DirectX12Device::_internalGetGPUDescriptorHeap_CbvSrvUav() const
{
    return gpuDescHeap_CbvSrvUav_->GetNativeHeap();
}

inline ID3D12DescriptorHeap *DirectX12Device::_internalGetGPUDescriptorHeap_Sampler() const
{
    return gpuDescHeap_Sampler_->GetNativeHeap();
}

inline uint32_t DirectX12Device::_internalGetDescriptorHandleIncrementSize_CbvSrvUav() const
{
    return srvUavCbvDescriptorSize_;
}

inline uint32_t DirectX12Device::_internalGetDescriptorHandleIncrementSize_Sampler() const
{
    return samplerDescriptorSize_;
}

RTRC_RHI_D3D12_END
