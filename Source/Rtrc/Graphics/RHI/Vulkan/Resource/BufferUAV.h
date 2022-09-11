#pragma once

#include <Rtrc/Graphics/RHI/Vulkan/Resource/Buffer.h>

RTRC_RHI_VK_BEGIN

class VulkanBufferUAV : public BufferUAV
{
public:

    VulkanBufferUAV(const VulkanBuffer *buffer, const BufferUAVDesc &desc, VkBufferView view);

    const BufferUAVDesc &GetDesc() const override;

    const VulkanBuffer *GetBuffer() const;

    VkBufferView GetNativeView() const;

private:

    const VulkanBuffer *buffer_;
    BufferUAVDesc desc_;
    VkBufferView view_;
};

RTRC_RHI_VK_END
