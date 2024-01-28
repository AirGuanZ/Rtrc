#pragma once

#include <Rtrc/Graphics/Device/CommandBuffer.h>
#include <Rtrc/Graphics/RenderGraph/Label.h>
#include <Rtrc/Graphics/RenderGraph/Resource.h>

RTRC_RG_BEGIN

struct ExecutableResources;
class Compiler;
class Pass;

class PassContext : public Uncopyable
{
public:

    CommandBuffer &GetCommandBuffer();

    RC<Buffer>      Get(const BufferResource *resource);
    const RC<Tlas> &Get(const TlasResource *resource);
    RC<Texture>     Get(const TextureResource *resource);

private:

    friend class Executer;

    PassContext(const ExecutableResources &resources, CommandBuffer &commandBuffer);
    ~PassContext();

    Ref<const ExecutableResources> resources_;
    Ref<CommandBuffer> commandBuffer_;

#if RTRC_RG_DEBUG
    const std::set<const Resource *> *declaredResources_ = nullptr;
#endif
};

PassContext   &GetCurrentPassContext();
CommandBuffer &GetCurrentCommandBuffer();

void Connect(Pass *head, Pass *tail);

class UAVOverlapGroup
{
public:

    auto operator<=>(const UAVOverlapGroup &) const = default;

    bool IsValid() const { return groupIndex != InvalidGroupIndex; }

    bool DontNeedUAVBarrier(const UAVOverlapGroup &other) const { return IsValid() && *this == other; }

private:

    friend class RenderGraph;

    static constexpr uint32_t InvalidGroupIndex = (std::numeric_limits<uint32_t>::max)();

    uint32_t groupIndex = InvalidGroupIndex;
};

class Pass
{
public:

    using Callback = std::move_only_function<void()>;
    using LegacyCallback = std::move_only_function<void(PassContext &)>;

    friend void Connect(Pass *head, Pass *tail);

    Pass *Use(const BufferResource  *buffer,  const UseInfo &info);
    Pass *Use(const TextureResource *texture, const UseInfo &info);
    Pass *Use(const TextureResource *texture, const TexSubrsc &subrsc, const UseInfo &info);
    Pass *Use(const TlasResource    *tlas,    const UseInfo &info);

    Pass *SetCallback(Callback callback);
    Pass *SetCallback(LegacyCallback callback);

    Pass *SetSignalFence(RHI::FenceRPtr fence);

private:

    using SubTexUsage = TexSubrscState;
    using BufferUsage = BufferState;

    using TextureUsage = TextureSubrscMap<std::optional<SubTexUsage>>;

    friend class RenderGraph;
    friend class Compiler;

    Pass(int index, const LabelStack::Node *node);

    int            index_;
    Callback       callback_;
    RHI::FenceRPtr signalFence_;

    const LabelStack::Node *nameNode_;

    std::map<const BufferResource *, BufferUsage, std::less<>> bufferUsages_;
    std::map<const TextureResource *, TextureUsage, std::less<>> textureUsages_;

    std::set<Pass *> prevs_;
    std::set<Pass *> succs_;

    UAVOverlapGroup uavOverlapGroup_;

    bool isExecuted_;
};

RTRC_RG_END
