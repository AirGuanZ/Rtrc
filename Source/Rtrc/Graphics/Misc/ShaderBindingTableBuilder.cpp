#include <Rtrc/Graphics/Device/Device.h>
#include <Rtrc/Graphics/Misc/ShaderBindingTableBuilder.h>

RTRC_BEGIN

ShaderBindingTableBuilder::ShaderBindingTableBuilder()
    : device_(nullptr), recordSize_(0), dataOffsetInRecord_(0), handleSize_(0), entryCount_(0)
{
    
}

ShaderBindingTableBuilder::ShaderBindingTableBuilder(
    Device *device, uint32_t maxShaderDataSizeInBytes, uint32_t entryCount)
    : device_(device), entryCount_(0)
{
    auto &rhiDevice = device_->GetRawDevice();
    const RHI::ShaderGroupRecordRequirements &shaderGroupReqs = rhiDevice->GetShaderGroupRecordRequirements();

    assert(shaderGroupReqs.shaderGroupHandleSize % shaderGroupReqs.shaderDataUnit == 0);
    dataOffsetInRecord_ = shaderGroupReqs.shaderGroupHandleSize;
    recordSize_ = dataOffsetInRecord_ + UpAlignTo(maxShaderDataSizeInBytes, shaderGroupReqs.shaderDataUnit);
    recordSize_ = UpAlignTo(recordSize_, shaderGroupReqs.shaderGroupHandleAlignment);
    recordSize_ = UpAlignTo(recordSize_, shaderGroupReqs.shaderGroupBaseAlignment);
    handleSize_ = shaderGroupReqs.shaderGroupHandleSize;

    if(entryCount)
    {
        Resize(entryCount);
    }
}

void ShaderBindingTableBuilder::Resize(uint32_t entryCount)
{
    entryCount_ = entryCount;
    data_.resize(recordSize_ * entryCount, 0);
}

void ShaderBindingTableBuilder::SetEntry(
    uint32_t index, ShaderGroupHandle shaderGroup, const void *shaderData, uint32_t shaderDataSizeInBytes)
{
    assert(index < entryCount_);
    assert(dataOffsetInRecord_ + shaderDataSizeInBytes <= recordSize_);
    unsigned char *record = data_.data() + recordSize_ * index;
    if(shaderGroup)
    {
        std::memcpy(record, shaderGroup.storage_, handleSize_);
        std::memcpy(record + dataOffsetInRecord_, shaderData, shaderDataSizeInBytes);
    }
    else
    {
        std::memset(record, 0, recordSize_);
    }
}

size_t ShaderBindingTableBuilder::GetRecordSizeInBytes() const
{
    return recordSize_;
}

uint32_t ShaderBindingTableBuilder::GetEntryCount() const
{
    return entryCount_;
}

Span<unsigned char> ShaderBindingTableBuilder::GetTableStorage() const
{
    return data_;
}

RC<Buffer> ShaderBindingTableBuilder::CreateTableBuffer(bool useCopyCommand) const
{
    return device_->CreateAndUploadBuffer(RHI::BufferDesc
    {
        .size           = data_.size(),
        .usage          = RHI::BufferUsage::ShaderBindingTable | RHI::BufferUsage::DeviceAddress,
        .hostAccessType = useCopyCommand ? RHI::BufferHostAccessType::None : RHI::BufferHostAccessType::SequentialWrite
    }, data_.data());
}

RTRC_END
