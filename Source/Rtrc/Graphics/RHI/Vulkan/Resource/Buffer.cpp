#include <ranges>

#include <Rtrc/Graphics/RHI/Vulkan/Context/Device.h>
#include <Rtrc/Graphics/RHI/Vulkan/Resource/BufferSRV.h>
#include <Rtrc/Graphics/RHI/Vulkan/Resource/BufferUAV.h>
#include <Rtrc/Utils/ScopeGuard.h>

RTRC_RHI_VK_BEGIN

VulkanBuffer::VulkanBuffer(
    const BufferDesc      &desc,
    VulkanDevice          *device,
    VkBuffer               buffer,
    VulkanMemoryAllocation alloc,
    ResourceOwnership      ownership)
    : desc_(desc), device_(device), buffer_(buffer), alloc_(alloc), ownership_(ownership)
{
    
}

VulkanBuffer::~VulkanBuffer()
{
    for(VkBufferView view : std::ranges::views::values(views_))
    {
        vkDestroyBufferView(device_->GetNativeDevice(), view, VK_ALLOC);
    }

    if(ownership_ == ResourceOwnership::Allocation)
    {
        vmaDestroyBuffer(alloc_.allocator, buffer_, alloc_.allocation);
    }
    else if(ownership_ == ResourceOwnership::Resource)
    {
        vkDestroyBuffer(device_->GetNativeDevice(), buffer_, VK_ALLOC);
    }
}

const BufferDesc &VulkanBuffer::GetDesc() const
{
    return desc_;
}

Ptr<BufferSRV> VulkanBuffer::CreateSRV(const BufferSRVDesc &desc) const
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
    return MakePtr<VulkanBufferSRV>(this, desc, view);
}

Ptr<BufferUAV> VulkanBuffer::CreateUAV(const BufferUAVDesc &desc) const
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
    return MakePtr<VulkanBufferUAV>(this, desc, view);
}

void *VulkanBuffer::Map(size_t offset, size_t size, bool invalidate)
{
    assert(ownership_ == ResourceOwnership::Allocation);
    assert(alloc_.allocator && alloc_.allocation);
    void *result;
    VK_FAIL_MSG(
        vmaMapMemory(alloc_.allocator, alloc_.allocation, &result),
        "failed to map vulkan buffer memory");
    if(invalidate)
    {
        InvalidateBeforeRead(offset, size);
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
    VK_FAIL_MSG(
        vmaInvalidateAllocation(alloc_.allocator, alloc_.allocation, offset, size),
        "failed to invalidate mapped buffer memory");
}

void VulkanBuffer::FlushAfterWrite(size_t offset, size_t size)
{
    VK_FAIL_MSG(
        vmaFlushAllocation(alloc_.allocator, alloc_.allocation, offset, size),
        "failed to flush buffer memory");
}

VkBuffer VulkanBuffer::GetNativeBuffer() const
{
    return buffer_;
}

VkBufferView VulkanBuffer::CreateBufferView(const ViewKey &key) const
{
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
    VK_FAIL_MSG(
        vkCreateBufferView(device_->GetNativeDevice(), &createInfo, VK_ALLOC, &view),
        "failed to create vulkan buffer view");
    RTRC_SCOPE_FAIL{ vkDestroyBufferView(device_->GetNativeDevice(), view, VK_ALLOC); };

    views_[key] = view;
    return view;
}

RTRC_RHI_VK_END
