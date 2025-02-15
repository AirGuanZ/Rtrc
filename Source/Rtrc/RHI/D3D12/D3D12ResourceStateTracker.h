#pragma once

#include <ankerl/unordered_dense.h>

#include <Rtrc/RHI/D3D12/D3D12Common.h>

RTRC_RHI_D3D12_BEGIN

class D3D12Buffer;
class D3D12CommandBuffer;
class D3D12Texture;

enum class D3D12BufferAccessBit : uint32_t
{
    None     = 0,
    SRV      = 1u << 0,
    UAVRead  = 1u << 1,
    UAVWrite = 1u << 2,
    CopySrc  = 1u << 3,
    CopyDst  = 1u << 4,
    Vertex   = 1u << 5,
    Index    = 1u << 6,
    Indirect = 1u << 7,
};
RTRC_DEFINE_ENUM_FLAGS(D3D12BufferAccessBit)
using D3D12BufferAccess = EnumFlagsD3D12BufferAccessBit;

struct D3D12BufferAccessProperty
{
    bool read = false;
    bool write = false;
};

const D3D12BufferAccessProperty &GetAccessProperty(D3D12BufferAccessBit access);
      D3D12BufferAccessProperty  GetAccessProperty(D3D12BufferAccess accesses);

bool IsReadOnly(D3D12BufferAccessBit access);
bool IsReadOnly(D3D12BufferAccess accesses);

enum class D3D12BufferAccessFlagBit : uint32_t
{
    None        = 0,
    SkipBarrier = 1u << 0,
};
RTRC_DEFINE_ENUM_FLAGS(D3D12BufferAccessFlagBit)
using D3D12BufferAccessFlags = EnumFlagsD3D12BufferAccessFlagBit;

enum class D3D12AccessQueue : uint8_t
{
    Graphics,
    Compute,
    Copy
};

class D3D12BufferAccessTracker : public Uncopyable
{
public:

    explicit D3D12BufferAccessTracker(QueueType queueType);
    ~D3D12BufferAccessTracker();

    void EnableConcurrentAccess(D3D12Buffer *buffer);

    void SetInitialAccess(D3D12Buffer *buffer, D3D12BufferAccess access);
    void Access(D3D12Buffer *buffer, D3D12BufferAccess access, bool pre);

    void AddFlag(D3D12Buffer *buffer, D3D12BufferAccessFlags flags);
    void RemoveFlag(D3D12Buffer *buffer, D3D12BufferAccessFlags flags);

    void Flush(D3D12CommandBuffer &commandBuffer);

private:

    struct Record
    {
        bool concurrentAccess = false;
        bool writed = false;
        D3D12BufferAccessFlags flags = D3D12BufferAccessFlagBit::None;
        
        D3D12BufferAccess beforeAccess = D3D12BufferAccessBit::None;
        D3D12BufferAccess afterAccess  = D3D12BufferAccessBit::None;
    };

    QueueType queueType_;
    ankerl::unordered_dense::map<D3D12Buffer *, Record> recordMap_;
};

RTRC_RHI_D3D12_END
