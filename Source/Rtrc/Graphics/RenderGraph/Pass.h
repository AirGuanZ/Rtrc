#pragma once

#include <Rtrc/Graphics/Device/CommandBuffer.h>
#include <Rtrc/Graphics/RenderGraph/Label.h>
#include <Rtrc/Graphics/RenderGraph/Resource.h>

RTRC_BEGIN

struct RGExecutableResources;
class RGCompiler;
class RGPassImpl;

using RGPass = RGPassImpl *;

class RGPassContext : public Uncopyable
{
public:

    CommandBuffer &GetCommandBuffer();

    RC<Buffer>      Get(const RGBufImpl *resource);
    const RC<Tlas> &Get(const RGTlasImpl *resource);
    RC<Texture>     Get(const RGTexImpl *resource);

private:

    friend class RGExecuter;

    RGPassContext(const RGExecutableResources &resources, CommandBuffer &commandBuffer);
    ~RGPassContext();

    Ref<const RGExecutableResources> resources_;
    Ref<CommandBuffer> commandBuffer_;

#if RTRC_RG_DEBUG
    const std::set<const RGResource *> *declaredResources_ = nullptr;
#endif
};

RGPassContext &RGGetPassContext();
CommandBuffer &RGGetCommandBuffer();

void Connect(RGPassImpl *head, RGPassImpl *tail);

class RGUavOverlapGroup
{
public:

    auto operator<=>(const RGUavOverlapGroup &) const = default;

    bool IsValid() const { return groupIndex != InvalidGroupIndex; }

    bool DontNeedUAVBarrier(const RGUavOverlapGroup &other) const { return IsValid() && *this == other; }

private:

    friend class RenderGraph;

    static constexpr uint32_t InvalidGroupIndex = (std::numeric_limits<uint32_t>::max)();

    uint32_t groupIndex = InvalidGroupIndex;
};

class RGPassImpl
{
public:

    using Callback = std::move_only_function<void()>;
    using LegacyCallback = std::move_only_function<void(RGPassContext &)>;

    friend void Connect(RGPassImpl *head, RGPassImpl *tail);

    RGPass Use(RGBuffer buffer,  const RGUseInfo &info);
    RGPass Use(RGTexture texture, const RGUseInfo &info);
    RGPass Use(RGTexture texture, const TexSubrsc &subrsc, const RGUseInfo &info);
    RGPass Use(RGTlas tlas,   const RGUseInfo &info);

    RGPass SetCallback(Callback callback);
    RGPass SetCallback(LegacyCallback callback);

    // Wait graph queue to be idle before executing this pass.
    // RenderGraph compiler will not take this into consideration when optimizing device synchronizations.
    // Use this only in editor and tools.
    RGPass SyncQueueBeforeExecution();

    RGPass SetSignalFence(RHI::FenceRPtr fence);

    RGPass ClearUAVOverlapGroup();

private:

    using SubTexUsage = TexSubrscState;
    using BufferUsage = BufferState;

    using TextureUsage = TextureSubrscMap<std::optional<SubTexUsage>>;

    friend class RenderGraph;
    friend class RGCompiler;

    RGPassImpl(int index, const RGLabelStack::Node *node);

    int            index_;
    Callback       callback_;
    RHI::FenceRPtr signalFence_;

    const RGLabelStack::Node *nameNode_;

    std::map<RGBuffer, BufferUsage, std::less<>> bufferUsages_;
    std::map<RGTexture, TextureUsage, std::less<>> textureUsages_;

    std::set<RGPass> prevs_;
    std::set<RGPass> succs_;

    RGUavOverlapGroup uavOverlapGroup_;

    bool syncBeforeExec_;
    bool isExecuted_;
};

RTRC_END
