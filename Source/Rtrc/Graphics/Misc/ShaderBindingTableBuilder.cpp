#include <Rtrc/Graphics/Device/Device.h>
#include <Rtrc/Graphics/Misc/ShaderBindingTableBuilder.h>
#include <Rtrc/Utility/Enumerate.h>

RTRC_BEGIN

ShaderBindingTable::operator bool() const
{
    return buffer_ != nullptr;
}

RHI::ShaderBindingTableRegion ShaderBindingTable::GetSubtable(uint32_t subtableIndex) const
{
    return subtables_[subtableIndex];
}

ShaderBindingTableBuilder::SubtableBuilder::SubtableBuilder()
    : recordSize_(0), shaderGroupHandleSize_(0), entryCount_(0)
{
    
}

void ShaderBindingTableBuilder::SubtableBuilder::Resize(uint32_t entryCount)
{
    entryCount_ = entryCount;
    storage_.resize(entryCount * recordSize_);
}

void ShaderBindingTableBuilder::SubtableBuilder::SetEntry(
    uint32_t entryIndex, ShaderGroupHandle shaderGroupHandle, const void *shaderData, uint32_t shaderDataSizeInBytes)
{
    assert(entryIndex < entryCount_);
    assert(shaderGroupHandleSize_ + shaderDataSizeInBytes <= recordSize_);
    unsigned char *data = storage_.data() + entryIndex * recordSize_;
    std::memset(data, 0, recordSize_);
    if(shaderGroupHandle)
    {
        std::memcpy(data, shaderGroupHandle.storage_, shaderGroupHandleSize_);
        assert((shaderData != nullptr) == (shaderDataSizeInBytes != 0));
        if(shaderData)
        {
            std::memcpy(data + shaderGroupHandleSize_, shaderData, shaderDataSizeInBytes);
        }
    }
}

ShaderBindingTableBuilder::ShaderBindingTableBuilder(Device *device)
    : device_(device)
{
    const RHI::ShaderGroupRecordRequirements &reqs = device->GetRawDevice()->GetShaderGroupRecordRequirements();
    assert(reqs.shaderGroupHandleSize % reqs.shaderDataUnit == 0);
    handleSize_      = reqs.shaderGroupHandleSize;
    recordAlignment_ = reqs.shaderGroupHandleAlignment;
    dataUnit_        = reqs.shaderDataUnit;
}

ShaderBindingTableBuilder::SubtableBuilder *ShaderBindingTableBuilder::AddSubtable(uint32_t shaderDataSizeInBytes)
{
    auto builder = MakeBox<SubtableBuilder>();
    builder->recordSize_ = UpAlignTo(handleSize_ + UpAlignTo(shaderDataSizeInBytes, dataUnit_), recordAlignment_);
    builder->shaderGroupHandleSize_ = handleSize_;
    subtableBuilders_.push_back(std::move(builder));
    return subtableBuilders_.back().get();
}

ShaderBindingTable ShaderBindingTableBuilder::CreateShaderBindingTable(bool useCopyCommand) const
{
    const RHI::DevicePtr &device = device_->GetRawDevice();
    const uint32_t subtableAlignment = device->GetShaderGroupRecordRequirements().shaderGroupBaseAlignment;

    size_t bufferSize = 0;
    for(auto &builder : subtableBuilders_)
    {
        assert(builder->entryCount_ > 0 && "Empty subtable in shader binding table builder");
        assert(builder->recordSize_ * builder->entryCount_ == builder->storage_.size());
        bufferSize = UpAlignTo<size_t>(bufferSize, subtableAlignment);
        bufferSize += builder->storage_.size();
    }

    std::vector<unsigned char> storage(bufferSize);
    size_t bufferOffset = 0;
    for(auto &builder : subtableBuilders_)
    {
        bufferOffset = UpAlignTo<size_t>(bufferOffset, subtableAlignment);
        std::memcpy(storage.data() + bufferOffset, builder->storage_.data(), builder->storage_.size());
        bufferOffset += builder->storage_.size();
    }

    auto buffer = device_->CreateAndUploadBuffer(RHI::BufferDesc
    {
        .size           = bufferSize,
        .usage          = RHI::BufferUsage::ShaderBindingTable | RHI::BufferUsage::DeviceAddress,
        .hostAccessType = useCopyCommand ? RHI::BufferHostAccessType::None : RHI::BufferHostAccessType::Upload
    }, storage.data());

    ShaderBindingTable ret;
    ret.buffer_ = std::move(buffer);
    ret.subtables_.resize(subtableBuilders_.size());

    uint32_t deviceAddressOffset = 0;
    for(auto &&[i, builder] : Enumerate(subtableBuilders_))
    {
        deviceAddressOffset = UpAlignTo(deviceAddressOffset, subtableAlignment);
        ret.subtables_[i] = RHI::ShaderBindingTableRegion
        {
            .deviceAddress = ret.buffer_->GetDeviceAddress() + deviceAddressOffset,
            .stride        = builder->recordSize_,
            .size          = static_cast<uint32_t>(builder->storage_.size())
        };
        deviceAddressOffset += static_cast<uint32_t>(builder->storage_.size());
    }

    return ret;
}

RTRC_END
