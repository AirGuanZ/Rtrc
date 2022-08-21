#pragma once

#include <Rtrc/RenderGraph/Executable.h>
#include <Rtrc/RenderGraph/TransientResourceManager.h>

RTRC_RG_BEGIN

class Compiler : public Uncopyable
{
public:

    Compiler(RHI::DevicePtr device, TransientResourceManager &transientResourceManager);

    void Compile(const RenderGraph &graph, ExecutableGraph &result);

private:

    template<typename T>
    struct ResourceUser
    {
        int passIndex;
        T   usage;
    };

    using BufferUser    = ResourceUser<Pass::BufferUsage>;
    using BufferUsers   = std::vector<BufferUser>;
    using BufferUserMap = std::map<BufferResource *, BufferUsers>;
    using SubTexUser    = ResourceUser<Pass::SubTexUsage>;
    using SubTexUsers   = std::vector<SubTexUser>;
    using SubTexKey     = std::pair<TextureResource*, RHI::TextureSubresource>;
    using SubTexUserMap = std::map<SubTexKey, SubTexUsers>;

    struct CompileSection
    {
        std::vector<int> passes;

        bool waitBackbufferSemaphore = false;
        RHI::PipelineStageFlag waitBackbufferSemaphoreStages = RHI::PipelineStage::None;

        bool signalBackbufferSemaphore = false;
        RHI::PipelineStageFlag signalBackbufferSemaphoreStages = RHI::PipelineStage::None;

        RHI::FencePtr signalFence;
        std::optional<RHI::TextureTransitionBarrier> swapchainPresentBarrier;
    };

    struct CompilePass
    {
        std::vector<RHI::TextureTransitionBarrier> beforeTextureTransitions;
        std::vector<RHI::BufferTransitionBarrier> beforeBufferTransitions;
    };

    static bool DontNeedBarrier(const Pass::BufferUsage &a, const Pass::BufferUsage &b);
    static bool DontNeedBarrier(const Pass::SubTexUsage &a, const Pass::SubTexUsage &b);

    bool IsInSection(int passIndex, const CompileSection *section) const;

    void TopologySortPasses();

    void CollectResourceUsers();

    void GenerateSections();

    void GenerateSemaphores();

    void FillExternalResources(ExecutableResources &output);

    void AllocateInternalResources(ExecutableResources &output);

    void GenerateBarriers(const ExecutableResources &resources);

    void FillSections(ExecutableGraph &output);

    RHI::DevicePtr            device_;
    TransientResourceManager &transientResourceManager_;
    const RenderGraph        *graph_;

    std::vector<const Pass*>      sortedPasses_;
    std::vector<Box<CompilePass>> sortedCompilePasses_;
    std::map<const Pass *, int>   passToSortedIndex_;

    BufferUserMap bufferUsers_;
    SubTexUserMap subTexUsers_;

    std::vector<Box<CompileSection>> sections_;
    std::vector<CompileSection *>    passToSection_;
};

RTRC_RG_END
