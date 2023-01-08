#pragma once

#include <Rtrc/Graphics/RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

RTRC_RHI_IMPLEMENT(VulkanMemoryPropertyRequirements, MemoryPropertyRequirements)
{
public:

    explicit VulkanMemoryPropertyRequirements(uint32_t memoryTypeBits);

    bool IsValid() const RTRC_RHI_OVERRIDE;

    bool Merge(const MemoryPropertyRequirements& other) RTRC_RHI_OVERRIDE;

    Ptr<MemoryPropertyRequirements> Clone() const RTRC_RHI_OVERRIDE;

    uint32_t _internalGetMemoryTypeBits() const;

private:

    uint32_t memoryTypeBits_;
};

RTRC_RHI_IMPLEMENT(VulkanMemoryBlock, MemoryBlock)
{
public:

    VulkanMemoryBlock(MemoryBlockDesc desc, VmaAllocator allocator, VmaAllocation allocation);

    ~VulkanMemoryBlock() override;

    const MemoryBlockDesc &GetDesc() const RTRC_RHI_OVERRIDE;

    VmaAllocator GetAllocator() const;

    VmaAllocation GetAllocation() const;

private:

    MemoryBlockDesc desc_;
    VmaAllocator allocator_;
    VmaAllocation allocation_;
};

RTRC_RHI_VK_END
