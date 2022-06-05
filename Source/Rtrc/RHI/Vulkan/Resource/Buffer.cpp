#include <Rtrc/RHI/Vulkan/Resource/BufferSRV.h>
#include <Rtrc/Utils/ScopeGuard.h>

RTRC_RHI_VK_BEGIN

VulkanBuffer::VulkanBuffer(
    const BufferDesc      &desc,
    VkDevice               device,
    VkBuffer               buffer,
    VulkanMemoryAllocation alloc,
    ResourceOwnership      ownership)
    : desc_(desc), device_(device), buffer_(buffer), alloc_(alloc), ownership_(ownership)
{
    
}

VulkanBuffer::~VulkanBuffer()
{
    for(auto &[_, view] : views_)
    {
        vkDestroyBufferView(device_, view, VK_ALLOC);
    }

    if(ownership_ == ResourceOwnership::Allocation)
    {
        vmaDestroyBuffer(alloc_.allocator, buffer_, alloc_.allocation);
    }
    else if(ownership_ == ResourceOwnership::Image)
    {
        vkDestroyBuffer(device_, buffer_, VK_ALLOC);
    }
}

RC<BufferSRV> VulkanBuffer::CreateSRV(const BufferSRVDesc &desc) const
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
        view = nullptr;
    }
    return MakeRC<VulkanBufferSRV>(this, desc, view);
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
        vkCreateBufferView(device_, &createInfo, VK_ALLOC, &view),
        "failed to create vulkan buffer view");
    RTRC_SCOPE_FAIL{ vkDestroyBufferView(device_, view, VK_ALLOC); };

    views_[key] = view;
    return view;
}

RTRC_RHI_VK_END
