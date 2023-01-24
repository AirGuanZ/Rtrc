#pragma once

#include <Rtrc/Graphics/RHI/Vulkan/Resource/Buffer.h>

RTRC_RHI_VK_BEGIN

RTRC_RHI_IMPLEMENT(VulkanBufferSrv, BufferSrv)
{
public:

    VulkanBufferSrv(const VulkanBuffer * buffer, const BufferSrvDesc & desc, VkBufferView view);

    const BufferSrvDesc &GetDesc() const RTRC_RHI_OVERRIDE;

    const VulkanBuffer *_internalGetBuffer() const;

    VkBufferView _internalGetNativeView() const;

private:

    const VulkanBuffer *buffer_;
    BufferSrvDesc desc_;
    VkBufferView view_;
};

RTRC_RHI_IMPLEMENT(VulkanBufferUav, BufferUav)
{
public:

    VulkanBufferUav(const VulkanBuffer * buffer, const BufferUavDesc & desc, VkBufferView view);

    const BufferUavDesc &GetDesc() const RTRC_RHI_OVERRIDE;

    const VulkanBuffer *GetBuffer() const;

    VkBufferView _internalGetNativeView() const;

private:

    const VulkanBuffer *buffer_;
    BufferUavDesc desc_;
    VkBufferView view_;
};

RTRC_RHI_VK_END
