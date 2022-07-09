#pragma once

#include <map>

#include <Rtrc/RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

class VulkanBuffer : public Buffer
{
public:

    VulkanBuffer(
        const BufferDesc      &desc,
        VkDevice               device,
        VkBuffer               buffer,
        VulkanMemoryAllocation alloc,
        ResourceOwnership      ownership);

    ~VulkanBuffer() override;

    const BufferDesc &GetDesc() const override;

    Ptr<BufferSRV> CreateSRV(const BufferSRVDesc &desc) const override;

    Ptr<BufferUAV> CreateUAV(const BufferUAVDesc &desc) const override;

    void *Map(size_t offset, size_t size) const override;

    void Unmap(size_t offset, size_t size) override;

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
    VkDevice                                device_;
    VkBuffer                                buffer_;
    VulkanMemoryAllocation                  alloc_;
    ResourceOwnership                       ownership_;
    mutable std::map<ViewKey, VkBufferView> views_;
};

RTRC_RHI_VK_END
