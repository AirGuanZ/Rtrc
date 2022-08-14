#include <Rtrc/RenderGraph/ResourceManager/TransientResourceManagerWithoutReuse.h>

RTRC_RG_BEGIN

TransientResourceManagerWithoutReuse::TransientResourceManagerWithoutReuse(RHI::DevicePtr device, int frameCount)
    : device_(std::move(device)), frameCount_(frameCount)
{
    
}

void TransientResourceManagerWithoutReuse::BeginFrame()
{
    // free resources unused for frameCount_ frames

    std::multimap<RHI::BufferDesc, BufferRecord> newAvailableBuffers;
    for(auto &[desc, record] : availableBuffers_)
    {
        if(record.freeFrameCount < frameCount_)
        {
            ++record.freeFrameCount;
            newAvailableBuffers.insert({ desc, record });
        }
    }
    availableBuffers_ = std::move(newAvailableBuffers);

    std::multimap<RHI::Texture2DDesc, Texture2DRecord> newAvailableTexture2Ds;
    for(auto &[desc, record] : availableTexture2Ds_)
    {
        if(record.freeFrameCount < frameCount_)
        {
            ++record.freeFrameCount;
            newAvailableTexture2Ds.insert({ desc, record });
        }
    }
    availableTexture2Ds_ = std::move(newAvailableTexture2Ds);

    // mark allocated resources available

    for(BufferRecord &record : allocatedBuffers_)
    {
        assert(record.freeFrameCount == 0);
        availableBuffers_.insert({ record.buffer->GetRHIBuffer()->GetDesc(), record });
    }
    allocatedBuffers_.clear();

    for(Texture2DRecord &record : allocatedTextures_)
    {
        assert(record.freeFrameCount == 0);
        availableTexture2Ds_.insert({ record.texture->GetRHITexture()->GetDesc(), record });
    }
    allocatedTextures_.clear();
}

RC<RHI::StatefulBuffer> TransientResourceManagerWithoutReuse::AllocateBuffer(const RHI::BufferDesc &desc)
{
    if(auto it = availableBuffers_.find(desc); it != availableBuffers_.end())
    {
        it->second.freeFrameCount = 0;
        allocatedBuffers_.push_back(it->second);
        availableBuffers_.erase(it);
    }
    else
    {
        auto newRHIBuffer = device_->CreateBuffer(desc);
        auto newStatefulBuffer = MakeRC<RHI::StatefulBuffer>(std::move(newRHIBuffer));
        allocatedBuffers_.push_back(BufferRecord{ std::move(newStatefulBuffer), 0 });
    }
    return allocatedBuffers_.back().buffer;
}

RC<RHI::StatefulTexture> TransientResourceManagerWithoutReuse::AllocateTexture2D(const RHI::Texture2DDesc &desc)
{
    if(auto it = availableTexture2Ds_.find(desc); it != availableTexture2Ds_.end())
    {
        it->second.freeFrameCount = 0;
        allocatedTextures_.push_back(it->second);
        availableTexture2Ds_.erase(it);
    }
    else
    {
        auto newRHITexture = device_->CreateTexture2D(desc);
        auto newStatefulTexture = MakeRC<RHI::StatefulTexture>(std::move(newRHITexture));
        allocatedTextures_.push_back(Texture2DRecord{ std::move(newStatefulTexture), 0 });
    }
    return allocatedTextures_.back().texture;
}

RTRC_RG_END
