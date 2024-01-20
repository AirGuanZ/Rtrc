#pragma once

#include <Rtrc/Graphics/RenderGraph/Executable.h>

RTRC_RG_BEGIN

namespace CompilerDetail
{

    enum class OptionBit : uint32_t
    {
        ConnectPassesByDefinitionOrder = 1 << 0,
        OptimizePassConnection         = 1 << 1,
        PreferGlobalMemoryBarrier      = 1 << 2,
    };
    RTRC_DEFINE_ENUM_FLAGS(OptionBit)
    using Options = EnumFlagsOptionBit;

} // namespace CompilerDetail

class Compiler : public Uncopyable
{
public:

    using Options = CompilerDetail::Options;

    explicit Compiler(
        Ref<Device> device,
        Options options = Options::ConnectPassesByDefinitionOrder |
                          Options::PreferGlobalMemoryBarrier);

    void SetTransientResourcePool(RHI::TransientResourcePoolOPtr pool);

    void Compile(const RenderGraph &graph, ExecutableGraph &result);

private:

    template<typename T>
    struct ResourceUser
    {
        int             passIndex;
        T               usage;
        UAVOverlapGroup uavOverlapGroup;
    };

    using BufferUser    = ResourceUser<Pass::BufferUsage>;
    using BufferUsers   = std::vector<BufferUser>;
    using BufferUserMap = std::map<const BufferResource *, BufferUsers, std::less<>>;
    using SubTexUser    = ResourceUser<Pass::SubTexUsage>;
    using SubTexUsers   = std::vector<SubTexUser>;
    using SubTexKey     = std::pair<const TextureResource*, RHI::TextureSubresource>;
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
    };

    struct CompilePass
    {
        std::vector<RHI::TextureTransitionBarrier> preTextureTransitions;
        std::vector<RHI::BufferTransitionBarrier>  preBufferTransitions;
    };

    static bool DontNeedBarrier(const Pass::BufferUsage &a, const Pass::BufferUsage &b);
    static bool DontNeedBarrier(const Pass::SubTexUsage &a, const Pass::SubTexUsage &b);

    static void GenerateGlobalMemoryBarriers(ExecutablePass &pass);

    static void GenerateConnectionsByDefinitionOrder(
        bool                           optimizeConnection,
        Span<Box<Pass>>                passes,
        std::vector<std::set<Pass *>> &outPrevs,
        std::vector<std::set<Pass *>> &outSuccs);

    bool IsInSection(int passIndex, const CompileSection *section) const;

    void TopologySortPasses();

    void CollectResourceUsers();

    void GenerateSections();

    void GenerateSemaphores();

    void FillExternalResources(ExecutableResources &output);

    void AllocateInternalResourcesLegacy(ExecutableResources &output);
    void AllocateInternalResources(ExecutableResources &output, std::vector<std::vector<int>> &aliasedPrevs);

    void GenerateBarriers(const ExecutableResources &resources);

    void FillSections(ExecutableGraph &output);

    Options options_;

    Ref<Device> device_;
    const RenderGraph  *graph_;

    std::vector<Pass*>            sortedPasses_;
    std::vector<Box<CompilePass>> sortedCompilePasses_;
    std::map<const Pass *, int>   passToSortedIndex_;

    BufferUserMap bufferUsers_;
    SubTexUserMap subTexUsers_;
    std::vector<std::vector<int>> aliasedPrevs_;

    std::vector<Box<CompileSection>> sections_;
    std::vector<CompileSection *>    passToSection_;

    RHI::TransientResourcePoolOPtr transientResourcePool_;
};

RTRC_RG_END
