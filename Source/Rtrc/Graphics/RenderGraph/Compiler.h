#pragma once

#include <Rtrc/Graphics/RenderGraph/Executable.h>

RTRC_BEGIN

namespace RGCompilerDetail
{

    enum class OptionBit : uint32_t
    {
        ConnectPassesByDefinitionOrder = 1 << 0,
        OptimizePassConnection         = 1 << 1,
        PreferGlobalMemoryBarrier      = 1 << 2,
    };
    RTRC_DEFINE_ENUM_FLAGS(OptionBit)
    using Options = EnumFlagsOptionBit;

} // namespace RGCompilerDetail

class RGCompiler : public Uncopyable
{
public:

    using Options = RGCompilerDetail::Options;

    explicit RGCompiler(Ref<Device> device, Options options = Options::ConnectPassesByDefinitionOrder);

    void SetPartialExecution(bool value);

    void SetTransientResourcePool(RHI::TransientResourcePoolOPtr pool);

    void Compile(const RenderGraph &graph, RGExecutableGraph &result);

private:

    template<typename T>
    struct ResourceUser
    {
        int             passIndex;
        T               usage;
        RGUavOverlapGroup uavOverlapGroup;
    };

    using BufferUser    = ResourceUser<RGPassImpl::BufferUsage>;
    using BufferUsers   = std::vector<BufferUser>;
    using BufferUserMap = std::map<const RGBufImpl *, BufferUsers, std::less<>>;
    using SubTexUser    = ResourceUser<RGPassImpl::SubTexUsage>;
    using SubTexUsers   = std::vector<SubTexUser>;
    using SubTexKey     = std::pair<const RGTexImpl*, TexSubrsc>;
    using SubTexUserMap = std::map<SubTexKey, SubTexUsers, std::less<>>;

    struct CompileSection
    {
        std::vector<int> passes;

        bool waitBackbufferSemaphore = false;
        RHI::PipelineStageFlag waitBackbufferSemaphoreStages = RHI::PipelineStage::None;

        bool signalBackbufferSemaphore = false;
        RHI::PipelineStageFlag signalBackbufferSemaphoreStages = RHI::PipelineStage::None;

        RHI::FenceRPtr signalFence;
        std::optional<RHI::TextureTransitionBarrier> swapchainPresentBarrier;

        bool syncBeforeExec = false;
    };

    struct CompilePass
    {
        std::vector<RHI::TextureTransitionBarrier> preTextureTransitions;
        std::vector<RHI::BufferTransitionBarrier>  preBufferTransitions;
    };

    static bool DontNeedBarrier(const RGPassImpl::BufferUsage &a, const RGPassImpl::BufferUsage &b);
    static bool DontNeedBarrier(const RGPassImpl::SubTexUsage &a, const RGPassImpl::SubTexUsage &b);

    static void GenerateGlobalMemoryBarriers(RGExecutablePass &pass);

    static void GenerateConnectionsByDefinitionOrder(
        bool                           optimizeConnection,
        Span<Box<RGPassImpl>>                passes,
        std::vector<std::set<RGPassImpl *>> &outPrevs,
        std::vector<std::set<RGPassImpl *>> &outSuccs);

    bool IsInSection(int passIndex, const CompileSection *section) const;

    void ProcessSynchronizedResourceStates();

    void AllocateInternalResourcesCrossExecutions();

    void TopologySortPasses();

    void CollectResourceUsers();

    void GenerateSections();

    void GenerateSemaphores();

    void FillExternalResources(RGExecutableResources &output);

    void AllocateInternalResourcesLegacy(RGExecutableResources &output);
    RC<RHI::QueueSyncQuery> AllocateInternalResources(
        RGExecutableResources &output, std::vector<std::vector<int>> &aliasedPrevs);

    void GenerateBarriers(const RGExecutableResources &resources);

    void FillSections(RGExecutableGraph &output);

    Options options_;
    bool    forPartialExecution_;

    Ref<Device>        device_;
    const RenderGraph *graph_;

    std::vector<RGPassImpl*>            sortedPasses_;
    std::vector<Box<CompilePass>> sortedCompilePasses_;
    std::map<const RGPassImpl *, int>   passToSortedIndex_;

    BufferUserMap bufferUsers_;
    SubTexUserMap subTexUsers_;
    std::vector<std::vector<int>> aliasedPrevs_;

    std::vector<Box<CompileSection>> sections_;
    std::vector<CompileSection *>    passToSection_;

    RHI::TransientResourcePoolOPtr transientResourcePool_;
};

RTRC_END
