#pragma once

#include <Rtrc/RenderGraph/ResourceManager/TransientResourceManagerWithoutReuse.h>
#include <Rtrc/Utils/Uncopyable.h>

RTRC_RG_BEGIN

struct ExecutableResources;

class ResourceLifetimeManager
{
public:

    using Mask = uint32_t;
    static constexpr int MASK_BITS = 32;

    ResourceLifetimeManager(int resourceCount, int passCount);

    int GetResourceCount() const;
    int GetMaskCountPerResource() const;
    int GetPassCount() const;

    void SetBit(int resourceIndex, int passIndex);
    bool GetBit(int resourceIndex, int passIndex) const;
    Span<Mask> GetBits(int resourceIndex) const;

private:

    int resourceCount_;
    int passCount_;
    int maskCountPerResource_;
    std::vector<Mask> lifeTimeMasks_;
};

class TransientResourceManager : public Uncopyable
{
public:

    enum class Strategy
    {
        // TODO: more strategies
        ReuseAcrossFrame
    };

    using ResourceDesc = Variant<RHI::BufferDesc, RHI::Texture2DDesc>;

    TransientResourceManager(RHI::DevicePtr device, Strategy strategy, int frameCount);

    void BeginFrame();

    void Allocate(
        const ResourceLifetimeManager &lifetimeManager,
        Span<ResourceDesc>             lifeTimeIndexToDesc,
        Span<int>                      lifeTimeIndexToResourceIndex,
        ExecutableResources           &resources);

private:

    Box<TransientResourceManagerWithoutReuse> reuseAcrossFrame_;
};

RTRC_RG_END
