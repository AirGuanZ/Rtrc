#pragma once

#include <Rtrc/RHI/Vulkan/Resource/Buffer.h>

RTRC_RHI_VK_BEGIN

class VulkanBufferSRV : public BufferSRV
{
public:

    VulkanBufferSRV(const VulkanBuffer *buffer, const BufferSRVDesc &desc, VkBufferView view);

    const BufferSRVDesc &GetDesc() const override;

    const VulkanBuffer *GetBuffer() const;

    VkBufferView GetNativeView() const;

private:

    const VulkanBuffer *buffer_;
    BufferSRVDesc desc_;
    VkBufferView view_;
};

RTRC_RHI_VK_END
