#pragma once

#include <map>
#include <shared_mutex>

#include <Rtrc/RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

RTRC_RHI_IMPLEMENT(VulkanBuffer, Buffer)
{
public:

    RTRC_VK_SET_OBJECT_NAME(device_, buffer_, VK_OBJECT_TYPE_BUFFER)

    VulkanBuffer(
        const BufferDesc      &desc,
        VulkanDevice          *device,
        VkBuffer               buffer,
        VulkanMemoryAllocation alloc,
        ResourceOwnership      ownership);

    ~VulkanBuffer() override;

    const BufferDesc &GetDesc() const RTRC_RHI_OVERRIDE;

    RPtr<BufferSrv> CreateSrv(const BufferSrvDesc &desc) const RTRC_RHI_OVERRIDE;

    RPtr<BufferUav> CreateUav(const BufferUavDesc &desc) const RTRC_RHI_OVERRIDE;

    void *Map(size_t offset, size_t size, const BufferReadRange &readRange, bool invalidate = false) RTRC_RHI_OVERRIDE;

    void Unmap(size_t offset, size_t size, bool flush = false) RTRC_RHI_OVERRIDE;

    void InvalidateBeforeRead(size_t offset, size_t size) RTRC_RHI_OVERRIDE;

    void FlushAfterWrite(size_t offset, size_t size) RTRC_RHI_OVERRIDE;

    BufferDeviceAddress GetDeviceAddress() const RTRC_RHI_OVERRIDE;

    VkBuffer _internalGetNativeBuffer() const;

private:

    struct ViewKey
    {
        VkFormat format;
        uint32_t offset;
        uint32_t count;

        auto operator<=>(const ViewKey &) const = default;
    };

    VkBufferView CreateBufferView(const ViewKey &key) const;

    BufferDesc                              desc_;
    VulkanDevice                           *device_;
    VkBuffer                                buffer_;
    VulkanMemoryAllocation                  alloc_;
    ResourceOwnership                       ownership_;
    VkDeviceAddress                         deviceAddress_;
    mutable std::map<ViewKey, VkBufferView> views_;
    mutable std::shared_mutex               viewsMutex_;
};

RTRC_RHI_VK_END
