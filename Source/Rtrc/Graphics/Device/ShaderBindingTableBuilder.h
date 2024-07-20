#pragma once

#include <Rtrc/Graphics/Device/Pipeline.h>

RTRC_BEGIN

class Buffer;
class Device;
class ShaderBindingTableBuilder2;

class ShaderGroupHandle
{
public:

    static ShaderGroupHandle FromPipeline(const RC<RayTracingPipeline> &pipeline)
    {
        const uint32_t stride = pipeline->GetShaderGroupHandleSize();
        const uint32_t count = pipeline->GetShaderGroupCount();
        auto storageBuffer = MakeRC<std::vector<unsigned char>>(stride * count);
        pipeline->GetShaderGroupHandles(0, count, *storageBuffer);

        ShaderGroupHandle ret;
        ret.storage_       = storageBuffer->data();
        ret.stride_        = stride;
        ret.storageBuffer_ = std::move(storageBuffer);
        return ret;
    }

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

    ShaderGroupHandle Offset(size_t index) const
    {
        ShaderGroupHandle ret;
        ret.storage_       = AddToPointer(storage_, stride_ * index);
        ret.stride_        = stride_;
        ret.storageBuffer_ = storageBuffer_;
        return ret;
    }

private:

    friend class ShaderBindingTableBuilder;

    const void *storage_;
    size_t      stride_;

    RC<std::vector<unsigned char>> storageBuffer_;
};

class ShaderBindingTable
{
public:

    ShaderBindingTable() = default;

    operator bool() const;

    RHI::ShaderBindingTableRegion GetSubtable(uint32_t subtableIndex) const;

private:

    friend class ShaderBindingTableBuilder;
    
    RC<Buffer> buffer_;
    std::vector<RHI::ShaderBindingTableRegion> subtables_;
};

class ShaderBindingTableBuilder
{
public:

    class SubtableBuilder
    {
    public:

        SubtableBuilder();

        void Resize(uint32_t entryCount);

        void SetEntry(
            uint32_t          entryIndex,
            ShaderGroupHandle shaderGroupHandle,
            const void       *shaderData,
            uint32_t          shaderDataSizeInBytes);

    private:

        friend class ShaderBindingTableBuilder;

        uint32_t recordSize_;
        uint32_t shaderGroupHandleSize_;
        uint32_t entryCount_;
        std::vector<unsigned char> storage_;
    };

    explicit ShaderBindingTableBuilder(Device *device = nullptr);

    SubtableBuilder *AddSubtable();

    ShaderBindingTable CreateShaderBindingTable(bool useCopyCommand = false) const;

private:

    Device  *device_;
    uint32_t handleSize_;
    uint32_t recordAlignment_;

    std::vector<Box<SubtableBuilder>> subtableBuilders_;
};

RTRC_END
