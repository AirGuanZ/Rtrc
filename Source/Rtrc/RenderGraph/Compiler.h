#pragma once

#include <Rtrc/RenderGraph/Executable.h>

RTRC_RG_BEGIN

class Compiler : public Uncopyable
{
public:

    explicit Compiler(RHI::DevicePtr device);

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

    struct Section
    {
        RHI::QueuePtr queue;
        std::vector<int> passes;
        std::set<Section *, std::less<>> directPrevs;
        std::set<Section *, std::less<>> directSuccs;
        std::set<Section *, std::less<>> indirectPrevs; // can reach this section from which sections

        int signalSemaphoreIndex = -1;
        RHI::PipelineStageFlag signalStages = RHI::PipelineStage::None;
        std::map<int, RHI::PipelineStageFlag> waitSemaphoreIndexToStages;

        bool waitBackbufferSemaphore = false;
        RHI::PipelineStageFlag waitBackbufferSemaphoreStages = RHI::PipelineStage::None;

        bool signalBackbufferSemaphore = false;
        RHI::PipelineStageFlag signalBackbufferSemaphoreStages = RHI::PipelineStage::None;

        bool CanReachFrom(const Section *prev) const
        {
            return prev == this || directPrevs.contains(prev) || indirectPrevs.contains(prev);
        }
    };

    static bool DontNeedBarrier(const Pass::BufferUsage &a, const Pass::BufferUsage &b);
    static bool DontNeedBarrier(const Pass::SubTexUsage &a, const Pass::SubTexUsage &b);

    bool IsInSection(int passIndex, const Section *section) const;

    void TopologySortPasses();

    void CollectResourceUsers();

    void GenerateSections();

    void SortPassesInSections();

    void GenerateSemaphores();

    void FillExternalResources(ExecutableResources &output);

    void AllocateInternalResources(ExecutableResources &output);

    RHI::DevicePtr device_;
    const RenderGraph *graph_;

    std::vector<const Pass*>    sortedPasses_;
    std::map<const Pass *, int> passToSortedIndex_;

    BufferUserMap bufferUsers_;
    SubTexUserMap subTexUsers_;

    std::vector<Box<Section>> sections_;
    std::vector<Section *> passToSection_;

    int semaphoreCount_;
};

RTRC_RG_END
