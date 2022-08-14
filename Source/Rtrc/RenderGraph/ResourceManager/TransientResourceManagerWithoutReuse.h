#pragma once

#include <map>

#include <Rtrc/RHI/Helper/StatefulResource.h>

RTRC_RG_BEGIN

// native strategy: will never reuse resources within a frame
class TransientResourceManagerWithoutReuse : public Uncopyable
{
public:

    TransientResourceManagerWithoutReuse(RHI::DevicePtr device, int frameCount);

    void BeginFrame();

    RC<RHI::StatefulBuffer> AllocateBuffer(const RHI::BufferDesc &desc);
    RC<RHI::StatefulTexture> AllocateTexture2D(const RHI::Texture2DDesc &desc);

private:

    struct BufferRecord
    {
        RC<RHI::StatefulBuffer> buffer;
        int freeFrameCount;
    };

    struct Texture2DRecord
    {
        RC<RHI::StatefulTexture> texture;
        int freeFrameCount;
    };

    RHI::DevicePtr device_;
    int frameCount_;

    std::multimap<RHI::BufferDesc, BufferRecord>       availableBuffers_;
    std::multimap<RHI::Texture2DDesc, Texture2DRecord> availableTexture2Ds_;

    std::vector<BufferRecord>    allocatedBuffers_;
    std::vector<Texture2DRecord> allocatedTextures_;
};

RTRC_RG_END
