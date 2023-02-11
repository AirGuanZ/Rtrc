#pragma once

#include <Rtrc/Graphics/RHI/RHI.h>

RTRC_BEGIN

class Buffer;
class Device;
class ShaderBindingTableBuilder;

class ShaderGroupHandle
{
public:

    ShaderGroupHandle()
        : storage_(nullptr), stride_(0)
    {

    }

    ShaderGroupHandle(const void *storage, size_t stride)
        : storage_(storage), stride_(stride)
    {

    }

    operator bool() const
    {
        return storage_ != nullptr;
    }

    ShaderGroupHandle operator+(size_t index) const
    {
        return { AddToPointer(storage_, stride_ * index), stride_ };
    }

private:

    friend class ShaderBindingTableBuilder;

    const void *storage_;
    size_t      stride_;
};

class ShaderBindingTableBuilder
{
public:

    ShaderBindingTableBuilder();

    ShaderBindingTableBuilder(Device *device, uint32_t maxShaderDataSizeInBytes, uint32_t entryCount = 0);

    void Resize(uint32_t entryCount);

    void SetEntry(uint32_t index, ShaderGroupHandle shaderGroup, const void *shaderData, uint32_t shaderDataSizeInBytes);

    // Byte size for single entry
    size_t GetRecordSizeInBytes() const;
    uint32_t GetEntryCount() const;
    Span<unsigned char> GetTableStorage() const;

    RC<Buffer> CreateTableBuffer(bool useCopyCommand = false) const;

    RHI::ShaderBindingTableRegion GetRegion(uint32_t entryOffset, uint32_t entryCount = 1) const;

private:

    Device *device_;

    uint32_t recordSize_;
    uint32_t dataOffsetInRecord_;
    uint32_t handleSize_;

    uint32_t                   entryCount_;
    std::vector<unsigned char> data_;
};

RTRC_END
