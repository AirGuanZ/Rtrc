#pragma once

#include <map>

#include <Rtrc/Graphics/RenderGraph/StatefulResource.h>

RTRC_RG_BEGIN

// native strategy: will never reuse resources within a frame
class TransientResourceManagerWithoutReuse : public Uncopyable
{
public:

    TransientResourceManagerWithoutReuse(RHI::DevicePtr device, int frameCount);

    void BeginFrame();

    RC<StatefulBuffer> AllocateBuffer(const RHI::BufferDesc &desc);
    RC<StatefulTexture> AllocateTexture2D(const RHI::TextureDesc &desc);

private:

    struct BufferRecord
    {
        RC<StatefulBuffer> buffer;
        int freeFrameCount;
    };

    struct TextureRecord
    {
        RC<StatefulTexture> texture;
        int freeFrameCount;
    };

    RHI::DevicePtr device_;
    int frameCount_;

    std::multimap<RHI::BufferDesc, BufferRecord>   availableBuffers_;
    std::multimap<RHI::TextureDesc, TextureRecord> availableTexture2Ds_;

    std::vector<BufferRecord>  allocatedBuffers_;
    std::vector<TextureRecord> allocatedTextures_;
};

RTRC_RG_END
