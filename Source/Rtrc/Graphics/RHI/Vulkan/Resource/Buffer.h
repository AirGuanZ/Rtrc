#pragma once

#include <map>

#include <tbb/spin_rw_mutex.h>

#include <Rtrc/Graphics/RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

class VulkanBuffer : public Buffer
{
public:

    VK_SET_OBJECT_NAME(device_, buffer_, VK_OBJECT_TYPE_BUFFER)

    VulkanBuffer(
        const BufferDesc      &desc,
        VulkanDevice          *device,
        VkBuffer               buffer,
        VulkanMemoryAllocation alloc,
        ResourceOwnership      ownership);

    ~VulkanBuffer() override;

    const BufferDesc &GetDesc() const override;

    Ptr<BufferSRV> CreateSRV(const BufferSRVDesc &desc) const override;

    Ptr<BufferUAV> CreateUAV(const BufferUAVDesc &desc) const override;

    void *Map(size_t offset, size_t size, bool invalidate) override;

    void Unmap(size_t offset, size_t size, bool flush) override;

    void InvalidateBeforeRead(size_t offset, size_t size) override;

    void FlushAfterWrite(size_t offset, size_t size) override;

    VkBuffer GetNativeBuffer() const;

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
    mutable std::map<ViewKey, VkBufferView> views_;
    mutable tbb::spin_rw_mutex              viewsMutex_;
};

RTRC_RHI_VK_END
