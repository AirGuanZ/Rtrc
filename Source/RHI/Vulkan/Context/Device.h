#pragma once

#include <RHI/Vulkan/Context/PhysicalDevice.h>
#include <RHI/Vulkan/Queue/Queue.h>

RTRC_RHI_VK_BEGIN

RTRC_RHI_IMPLEMENT(VulkanDevice, Device)
{
public:

    struct QueueFamilyInfo
    {
        std::optional<uint32_t> graphicsFamilyIndex;
        std::optional<uint32_t> computeFamilyIndex;
        std::optional<uint32_t> transferFamilyIndex;
    };

    VulkanDevice(
        VkInstance             instance,
        VulkanPhysicalDevice   physicalDevice,
        VkDevice               device,
        const QueueFamilyInfo &queueFamilyInfo,
        bool                   enableDebug);

    ~VulkanDevice() override;

    BackendType GetBackendType() const RTRC_RHI_OVERRIDE { return BackendType::Vulkan; }
    bool IsGlobalBarrierWellSupported() const RTRC_RHI_OVERRIDE { return true; }
    BarrierMemoryModel GetBarrierMemoryModel() const RTRC_RHI_OVERRIDE { return BarrierMemoryModel::AvailableAndVisible; }

    RPtr<Queue> GetQueue(QueueType type) RTRC_RHI_OVERRIDE;

    RPtr<CommandPool> CreateCommandPool(const RPtr<Queue> &queue) RTRC_RHI_OVERRIDE;

    RPtr<Fence> CreateFence(bool signaled) RTRC_RHI_OVERRIDE;

    RPtr<Swapchain> CreateSwapchain(const SwapchainDesc &desc, Window &window) RTRC_RHI_OVERRIDE;

    RPtr<Semaphore> CreateTimelineSemaphore(uint64_t initialValue) RTRC_RHI_OVERRIDE;

    RPtr<RawShader> CreateShader(const void *data, size_t size, std::vector<RawShaderEntry> entries) RTRC_RHI_OVERRIDE;

    RPtr<GraphicsPipeline>   CreateGraphicsPipeline  (const GraphicsPipelineDesc &desc) RTRC_RHI_OVERRIDE;
    RPtr<ComputePipeline>    CreateComputePipeline   (const ComputePipelineDesc &desc) RTRC_RHI_OVERRIDE;
    RPtr<RayTracingPipeline> CreateRayTracingPipeline(const RayTracingPipelineDesc &desc) RTRC_RHI_OVERRIDE;

    RPtr<RayTracingLibrary> CreateRayTracingLibrary(const RayTracingLibraryDesc &desc) RTRC_RHI_OVERRIDE;

    RPtr<BindingGroupLayout> CreateBindingGroupLayout(const BindingGroupLayoutDesc &desc) RTRC_RHI_OVERRIDE;
    RPtr<BindingGroup> CreateBindingGroup(
        const RPtr<BindingGroupLayout> &bindingGroupLayout,
        uint32_t                       variableArraySize = 0) RTRC_RHI_OVERRIDE;
    RPtr<BindingLayout> CreateBindingLayout(const BindingLayoutDesc &desc) RTRC_RHI_OVERRIDE;

    void UpdateBindingGroups(const BindingGroupUpdateBatch &batch) RTRC_RHI_OVERRIDE;
    void CopyBindingGroup(
        const BindingGroupPtr &dstGroup, uint32_t dstIndex, uint32_t dstArrayOffset,
        const BindingGroupPtr &srcGroup, uint32_t srcIndex, uint32_t srcArrayOffset,
        uint32_t count) RTRC_RHI_OVERRIDE;

    RPtr<Texture> CreateTexture(const TextureDesc &desc) RTRC_RHI_OVERRIDE;

    RPtr<Buffer> CreateBuffer(const BufferDesc &desc) RTRC_RHI_OVERRIDE;

    RPtr<Sampler> CreateSampler(const SamplerDesc &desc) RTRC_RHI_OVERRIDE;

    size_t GetConstantBufferAlignment() const RTRC_RHI_OVERRIDE;
    size_t GetConstantBufferSizeAlignment() const RTRC_RHI_OVERRIDE;
    size_t GetAccelerationStructureScratchBufferAlignment() const RTRC_RHI_OVERRIDE;
    size_t GetTextureBufferCopyRowPitchAlignment(Format texelFormat) const RTRC_RHI_OVERRIDE;

    void WaitIdle() RTRC_RHI_OVERRIDE;

    BlasPtr CreateBlas(const BufferPtr &buffer, size_t offset, size_t size) RTRC_RHI_OVERRIDE;
    TlasPtr CreateTlas(const BufferPtr &buffer, size_t offset, size_t size) RTRC_RHI_OVERRIDE;

    BlasPrebuildInfoPtr CreateBlasPrebuildInfo(
        Span<RayTracingGeometryDesc>              geometries,
        RayTracingAccelerationStructureBuildFlags flags) RTRC_RHI_OVERRIDE;
    TlasPrebuildInfoPtr CreateTlasPrebuildInfo(
        const RayTracingInstanceArrayDesc        &instances,
        RayTracingAccelerationStructureBuildFlags flags) RTRC_RHI_OVERRIDE;

    const ShaderGroupRecordRequirements &GetShaderGroupRecordRequirements() const RTRC_RHI_OVERRIDE;
    const WarpSizeInfo &GetWarpSizeInfo() const RTRC_RHI_OVERRIDE;

    void _internalSetObjectName(VkObjectType objectType, uint64_t objectHandle, const char *name);

    uint32_t _internalGetQueueFamilyIndex(QueueType type) const;

    VkDevice _internalGetNativeDevice() const;

private:

    bool enableDebug_;

    VkInstance           instance_;
    VulkanPhysicalDevice physicalDevice_;
    VkDevice             device_;

    RPtr<VulkanQueue> graphicsQueue_;
    RPtr<VulkanQueue> computeQueue_;
    RPtr<VulkanQueue> transferQueue_;
    RPtr<VulkanQueue> presentQueue_;
    QueueFamilyInfo queueFamilies_;

    VmaAllocator allocator_;

    std::optional<ShaderGroupRecordRequirements> shaderGroupRecordRequirements_;
    WarpSizeInfo warpSizeInfo_;
};

RTRC_RHI_VK_END
