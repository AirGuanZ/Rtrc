#pragma once

#include <Rtrc/Graphics/RHI/Vulkan/Common.h>

RTRC_RHI_VK_BEGIN

class VulkanMemoryPropertyRequirements : public MemoryPropertyRequirements
{
public:

    explicit VulkanMemoryPropertyRequirements(uint32_t memoryTypeBits);

    bool IsValid() const override;

    bool Merge(const MemoryPropertyRequirements& other) override;

    Ptr<MemoryPropertyRequirements> Clone() const override;

    uint32_t GetMemoryTypeBits() const;

private:

    uint32_t memoryTypeBits_;
};

class VulkanMemoryBlock : public MemoryBlock
{
public:

    VulkanMemoryBlock(const MemoryBlockDesc &desc, VmaAllocator allocator, VmaAllocation allocation);

    ~VulkanMemoryBlock() override;

    const MemoryBlockDesc &GetDesc() const override;

    VmaAllocator GetAllocator() const;

    VmaAllocation GetAllocation() const;

private:

    MemoryBlockDesc desc_;
    VmaAllocator allocator_;
    VmaAllocation allocation_;
};

RTRC_RHI_VK_END
