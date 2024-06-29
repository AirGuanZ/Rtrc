#include <ranges>
#include <shared_mutex>

#include <Rtrc/Core/ScopeGuard.h>
#include <Rtrc/RHI/Vulkan/Context/Device.h>
#include <Rtrc/RHI/Vulkan/Resource/BufferView.h>

RTRC_RHI_VK_BEGIN

VulkanBuffer::VulkanBuffer(
    const BufferDesc      &desc,
    VulkanDevice          *device,
    VkBuffer               buffer,
    VulkanMemoryAllocation alloc,
    ResourceOwnership      ownership)
    : desc_(desc), device_(device), buffer_(buffer), alloc_(alloc), ownership_(ownership), deviceAddress_()
{
    if(desc_.usage.Contains(BufferUsage::DeviceAddress) ||
       desc_.usage.Contains(BufferUsage::AccelerationStructureBuildInput))
    {
        const VkBufferDeviceAddressInfo deviceAddressInfo =
        {
            .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
            .buffer = buffer_
        };
        deviceAddress_ = vkGetBufferDeviceAddress(device_->_internalGetNativeDevice(), &deviceAddressInfo);
    }
}

VulkanBuffer::~VulkanBuffer()
{
    for(VkBufferView view : std::views::values(views_))
    {
        vkDestroyBufferView(device_->_internalGetNativeDevice(), view, RTRC_VK_ALLOC);
    }

    if(ownership_ == ResourceOwnership::Allocation)
    {
        vmaDestroyBuffer(alloc_.allocator, buffer_, alloc_.allocation);
    }
    else if(ownership_ == ResourceOwnership::Resource)
    {
        vkDestroyBuffer(device_->_internalGetNativeDevice(), buffer_, RTRC_VK_ALLOC);
    }
}

const BufferDesc &VulkanBuffer::GetDesc() const
{
    return desc_;
}

RPtr<BufferSrv> VulkanBuffer::CreateSrv(const BufferSrvDesc &desc) const
{
    VkBufferView view;
    if(desc.format != Format::Unknown)
    {
        view = CreateBufferView(ViewKey{
            .format = TranslateTexelFormat(desc.format),
            .offset = desc.offset,
            .count  = desc.range
        });
    }
    else
    {
        view = VK_NULL_HANDLE;
    }
    return MakeRPtr<VulkanBufferSrv>(this, desc, view);
}

RPtr<BufferUav> VulkanBuffer::CreateUav(const BufferUavDesc &desc) const
{
    VkBufferView view;
    if(desc.format != Format::Unknown)
    {
        view = CreateBufferView(ViewKey{
            .format = TranslateTexelFormat(desc.format),
            .offset = desc.offset,
            .count  = desc.range
        });
    }
    else
    {
        view = VK_NULL_HANDLE;
    }
    return MakeRPtr<VulkanBufferUav>(this, desc, view);
}

void *VulkanBuffer::Map(size_t offset, size_t size, const BufferReadRange &readRange, bool invalidate)
{
    assert(ownership_ == ResourceOwnership::Allocation);
    assert(alloc_.allocator && alloc_.allocation);
    void *result;
    RTRC_VK_FAIL_MSG(
        vmaMapMemory(alloc_.allocator, alloc_.allocation, &result),
        "failed to map vulkan buffer memory");
    if(invalidate)
    {
        size_t invalidateOffset = offset, invalidateEnd = offset + size;
        if(readRange != READ_WHOLE_BUFFER && readRange.size > 0)
        {
            invalidateOffset = std::max(invalidateOffset, readRange.offset);
            invalidateEnd = std::min(invalidateEnd, readRange.offset + readRange.size);
        }

        if(invalidateEnd > invalidateOffset)
        {
            InvalidateBeforeRead(invalidateOffset, invalidateEnd - invalidateOffset);
        }
    }
    return result;
}

void VulkanBuffer::Unmap(size_t offset, size_t size, bool flush)
{
    assert(ownership_ == ResourceOwnership::Allocation);
    vmaUnmapMemory(alloc_.allocator, alloc_.allocation);
    if(flush)
    {
        FlushAfterWrite(offset, size);
    }
}

void VulkanBuffer::InvalidateBeforeRead(size_t offset, size_t size)
{
    RTRC_VK_FAIL_MSG(
        vmaInvalidateAllocation(alloc_.allocator, alloc_.allocation, offset, size),
        "failed to invalidate mapped buffer memory");
}

void VulkanBuffer::FlushAfterWrite(size_t offset, size_t size)
{
    RTRC_VK_FAIL_MSG(
        vmaFlushAllocation(alloc_.allocator, alloc_.allocation, offset, size),
        "failed to flush buffer memory");
}

BufferDeviceAddress VulkanBuffer::GetDeviceAddress() const
{
    assert(deviceAddress_);
    return { deviceAddress_ };
}

VkBuffer VulkanBuffer::_internalGetNativeBuffer() const
{
    return buffer_;
}

VkBufferView VulkanBuffer::CreateBufferView(const ViewKey &key) const
{
    {
        std::shared_lock lock(viewsMutex_);
        if(auto it = views_.find(key); it != views_.end())
        {
            return it->second;
        }
    }

    std::unique_lock lock(viewsMutex_);

    if(auto it = views_.find(key); it != views_.end())
    {
        return it->second;
    }

    const VkBufferViewCreateInfo createInfo = {
        .sType  = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO,
        .buffer = buffer_,
        .format = key.format,
        .offset = key.offset,
        .range  = key.count
    };

    VkBufferView view;
    RTRC_VK_FAIL_MSG(
        vkCreateBufferView(device_->_internalGetNativeDevice(), &createInfo, RTRC_VK_ALLOC, &view),
        "failed to create vulkan buffer view");
    RTRC_SCOPE_FAIL{ vkDestroyBufferView(device_->_internalGetNativeDevice(), view, RTRC_VK_ALLOC); };

    views_[key] = view;
    return view;
}

RTRC_RHI_VK_END
