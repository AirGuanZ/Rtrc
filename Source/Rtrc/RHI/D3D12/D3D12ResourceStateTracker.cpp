#include <Rtrc/RHI/D3D12/D3D12ResourceStateTracker.h>

#include <Rtrc/Core/Bit.h>

RTRC_RHI_D3D12_BEGIN

constexpr auto &GetAccessPropertyArray()
{
    static constexpr D3D12BufferAccessProperty properties[] =
    {
        // SRV
        {
            .read = true,
            .write = false
        },
        // UAVRead
        {
            .read = true,
            .write = false
        },
        // UAVWrite
        {
            .read = false,
            .write = true
        },
        // CopySrc
        {
            .read = true,
            .write = false
        },
        // CopyDst
        {
            .read = false,
            .write = true
        },
        // Vertex
        {
            .read = true,
            .write = false
        },
        // Index
        {
            .read = true,
            .write = false
        },
        // Indirect
        {
            .read = true,
            .write = false
        }
    };
    return properties;
}

const D3D12BufferAccessProperty &GetAccessProperty(D3D12BufferAccessBit access)
{
    auto &properties = GetAccessPropertyArray();
    assert(IsPowerOfTwo(std::to_underlying(access)));
    const uint32_t index = std::countr_zero(std::to_underlying(access));
    assert(index < GetArraySize(properties));
    return properties[index];
}

D3D12BufferAccessProperty GetAccessProperty(D3D12BufferAccess accesses)
{
    auto &properties = GetAccessPropertyArray();
    D3D12BufferAccessProperty property =
    {
        .read = false,
        .write = false
    };
    auto bits = accesses.GetInteger();
    while(bits)
    {
        const uint32_t bitIndex = std::countr_zero(bits);
        auto &bitProperty = properties[bitIndex];
        property.read |= bitProperty.read;
        property.write |= bitProperty.write;
        bits &= ~(1u << bitIndex);
    }
    return property;
}

bool IsReadOnly(D3D12BufferAccessBit access)
{
    const auto property = GetAccessProperty(access);
    return property.read && !property.write;
}

bool IsReadOnly(D3D12BufferAccess accesses)
{
    const auto property = GetAccessProperty(accesses);
    return property.read && !property.write;
}

D3D12BufferAccessTracker::D3D12BufferAccessTracker(QueueType queueType)
    : queueType_(queueType)
{

}

void D3D12BufferAccessTracker::EnableConcurrentAccess(D3D12Buffer *buffer)
{
    assert(buffer);
    auto &record = recordMap_[buffer];
    assert(!record.concurrentAccess && !record.writed);
    recordMap_[buffer].concurrentAccess = true;
}

void D3D12BufferAccessTracker::SetInitialAccess(D3D12Buffer *buffer, D3D12BufferAccess accesses)
{
    assert(buffer);
    auto &record = recordMap_[buffer];
    assert(record.beforeAccess == D3D12BufferAccess::None && record.afterAccess == D3D12BufferAccess::None);
    assert(!record.concurrentAccess || IsReadOnly(accesses));
    record.beforeAccess = accesses;
}

void D3D12BufferAccessTracker::Access(D3D12Buffer *buffer, D3D12BufferAccess access, bool pre)
{
    // TODO
}

RTRC_RHI_D3D12_END
