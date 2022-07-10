#include <Rtrc/RHI/Vulkan/Resource/MemoryBlock.h>

RTRC_RHI_VK_BEGIN

VulkanMemoryPropertyRequirements::VulkanMemoryPropertyRequirements(uint32_t memoryTypeBits)
    : memoryTypeBits_(memoryTypeBits)
{
    
}

bool VulkanMemoryPropertyRequirements::IsValid() const
{
    return memoryTypeBits_ != 0;
}

bool VulkanMemoryPropertyRequirements::Merge(const MemoryPropertyRequirements &other)
{
    memoryTypeBits_ &= static_cast<const VulkanMemoryPropertyRequirements &>(other).GetMemoryTypeBits();
    return IsValid();
}

Ptr<MemoryPropertyRequirements> VulkanMemoryPropertyRequirements::Clone() const
{
    return MakePtr<VulkanMemoryPropertyRequirements>(memoryTypeBits_);
}

uint32_t VulkanMemoryPropertyRequirements::GetMemoryTypeBits() const
{
    return memoryTypeBits_;
}

VulkanMemoryBlock::VulkanMemoryBlock(const MemoryBlockDesc &desc, VmaAllocator allocator, VmaAllocation allocation)
    : desc_(desc), allocator_(allocator), allocation_(allocation)
{
    
}

VulkanMemoryBlock::~VulkanMemoryBlock()
{
    assert(allocator_ && allocation_);
    vmaFreeMemory(allocator_, allocation_);
}

const MemoryBlockDesc &VulkanMemoryBlock::GetDesc() const
{
    return desc_;
}

VmaAllocation VulkanMemoryBlock::GetAllocation() const
{
    return allocation_;
}

VmaAllocator VulkanMemoryBlock::GetAllocator() const
{
    return allocator_;
}

RTRC_RHI_VK_END
