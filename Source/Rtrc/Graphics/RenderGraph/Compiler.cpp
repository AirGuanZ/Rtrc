#include <queue>

#include <Rtrc/Graphics/RenderGraph/Compiler.h>
#include <Rtrc/Graphics/RenderGraph/Graph.h>

RTRC_RG_BEGIN

namespace CompilerDetail
{

    void RemoveUnnecessaryBufferAccessMask(RHI::ResourceAccessFlag &beforeAccess, RHI::ResourceAccessFlag &afterAccess)
    {
        if(IsReadOnly(beforeAccess))
        {
            beforeAccess = RHI::ResourceAccess::None;
        }
        if(IsWriteOnly(afterAccess))
        {
            afterAccess = RHI::ResourceAccess::None;
        }
    }

    void RemoveUnnecessaryTextureAccessMask(
        RHI::TextureLayout beforeLayout, RHI::TextureLayout afterLayout,
        RHI::ResourceAccessFlag &beforeAccess, RHI::ResourceAccessFlag &afterAccess)
    {
        if(IsReadOnly(beforeAccess))
        {
            beforeAccess = RHI::ResourceAccess::None;
        }
        if(IsWriteOnly(afterAccess) && beforeLayout == afterLayout)
        {
            afterAccess = RHI::ResourceAccess::None;
        }
    }

} // namespace CompilerDetail

Compiler::Compiler(Ref<Device> device, Options options)
    : options_(options)
    , device_(device)
    , graph_(nullptr)
{
    if(!device_->GetRawDevice()->IsGlobalBarrierWellSupported())
    {
        options_.value &= ~std::to_underlying(Options::PreferGlobalMemoryBarrier);
    }
}

void Compiler::SetTransientResourcePool(RHI::TransientResourcePoolOPtr pool)
{
    transientResourcePool_ = pool;
}

void Compiler::Compile(const RenderGraph &graph, ExecutableGraph &result)
{
    graph_ = &graph;

    ProcessSynchronizedResourceStates();

    TopologySortPasses();
    CollectResourceUsers();

    GenerateSections();
    GenerateSemaphores();

    result.resources.indexToBuffer.resize(graph_->buffers_.size());
    result.resources.indexToTexture.resize(graph_->textures_.size());
    aliasedPrevs_.resize(graph.buffers_.size());

    FillExternalResources(result.resources);
    if(transientResourcePool_)
    {
        AllocateInternalResources(result.resources, aliasedPrevs_);
    }
    else
    {
        AllocateInternalResourcesLegacy(result.resources);
    }

    sortedCompilePasses_.resize(sortedPasses_.size());
    for(auto &p : sortedCompilePasses_)
    {
        p = MakeBox<CompilePass>();
    }
    GenerateBarriers(result.resources);

    result.queue = graph.queue_.GetRHIObject();
    FillSections(result);

    result.completeFence = graph.completeFence_;
}

bool Compiler::DontNeedBarrier(const Pass::BufferUsage &a, const Pass::BufferUsage &b)
{
    return IsReadOnly(a.accesses) && IsReadOnly(b.accesses);
}

bool Compiler::DontNeedBarrier(const Pass::SubTexUsage &a, const Pass::SubTexUsage &b)
{
    if(a.layout != b.layout)
    {
        return false;
    }

    // Render target access order are handled by ROP

    constexpr RHI::ResourceAccessFlag RENDER_TARGET_RW_MASK = RHI::ResourceAccess::RenderTargetWrite |
                                                              RHI::ResourceAccess::RenderTargetRead;
    if(RENDER_TARGET_RW_MASK.Contains(a.accesses) && RENDER_TARGET_RW_MASK.Contains(b.accesses))
    {
        return true;
    }

    // Readonly accesses

    return IsReadOnly(a.accesses) && IsReadOnly(b.accesses);
}

void Compiler::GenerateGlobalMemoryBarriers(ExecutablePass &pass)
{
    RHI::GlobalMemoryBarrier globalMemoryBarrier =
    {
        .beforeStages   = RHI::PipelineStage::None,
        .beforeAccesses = RHI::ResourceAccess::None,
        .afterStages    = RHI::PipelineStage::None,
        .afterAccesses  = RHI::ResourceAccess::None
    };

    for(const RHI::BufferTransitionBarrier &b : pass.preBufferBarriers)
    {
        globalMemoryBarrier.beforeStages   |= b.beforeStages;
        globalMemoryBarrier.beforeAccesses |= b.beforeAccesses;
        globalMemoryBarrier.afterStages    |= b.afterStages;
        globalMemoryBarrier.afterAccesses  |= b.afterAccesses;
    }

    std::vector<RHI::TextureTransitionBarrier> newTextureBarriers;
    for(const RHI::TextureTransitionBarrier &b : pass.preTextureBarriers)
    {
        if(b.beforeLayout == b.afterLayout)
        {
            globalMemoryBarrier.beforeStages   |= b.beforeStages;
            globalMemoryBarrier.beforeAccesses |= b.beforeAccesses;
            globalMemoryBarrier.afterStages    |= b.afterStages;
            globalMemoryBarrier.afterAccesses  |= b.afterAccesses;
        }
        else
        {
            newTextureBarriers.push_back(b);
        }
    }

    const size_t reducedBarrierCount = pass.preBufferBarriers.size()
                                     + (pass.preTextureBarriers.size() - newTextureBarriers.size());
    if(reducedBarrierCount >= 2)
    {
        pass.preGlobalBarrier = globalMemoryBarrier;
        pass.preBufferBarriers.clear();
        pass.preTextureBarriers = std::move(newTextureBarriers);
    }
}

void Compiler::GenerateConnectionsByDefinitionOrder(
    bool                          optimizeConnection,
    Span<Box<Pass>>               passes,
    std::vector<std::set<Pass*>> &outPrevs,
    std::vector<std::set<Pass*>> &outSuccs)
{
    struct UserRecord
    {
        std::set<Pass *>   prevUsers;
        std::set<Pass *>   users;
        RHI::TextureLayout layout = RHI::TextureLayout::Undefined;
        bool               isReadOnly = false;
        UAVOverlapGroup    uavOverlapGroup;
    };

    std::map<uint64_t, UserRecord> resourceIndexToUsers;

    for(const Box<Pass> &pass : passes)
    {
        Pass *curr = pass.get();

        for(auto &&[buffer, usage] : pass->bufferUsages_)
        {
            assert(!HasUAVAccess(usage.accesses) || IsUAVOnly(usage.accesses));
            const bool isCurrReadOnly = IsReadOnly(usage.accesses);

            auto &userRecord = resourceIndexToUsers[buffer->GetResourceIndex()];
            if(optimizeConnection && userRecord.isReadOnly && isCurrReadOnly)
            {
                userRecord.users.insert(curr);
            }
            else
            {
                const bool isUAVOnly = IsUAVOnly(usage.accesses);
                if(pass->uavOverlapGroup_.IsValid() && isUAVOnly &&
                   userRecord.uavOverlapGroup == pass->uavOverlapGroup_)
                {
                    userRecord.users.insert(curr);
                }
                else
                {
                    userRecord.prevUsers       = userRecord.users;
                    userRecord.users           = { curr };
                    userRecord.isReadOnly      = isCurrReadOnly;
                    userRecord.uavOverlapGroup = isUAVOnly ? pass->uavOverlapGroup_ : UAVOverlapGroup{};
                }
            }
            for(auto prev : userRecord.prevUsers)
            {
                outSuccs[prev->index_].insert(curr);
                outPrevs[curr->index_].insert(prev);
            }
        }
        
        for(auto &[tex, usage] : pass->textureUsages_)
        {
            uint64_t subrscIndex = 0;
            for(auto &subrscUsage : usage)
            {
                if(subrscUsage.has_value())
                {
                    assert(!HasUAVAccess(subrscUsage->accesses) || IsUAVOnly(subrscUsage->accesses));
                    const bool isCurrReadOnly = IsReadOnly(subrscUsage->accesses);

                    auto& userRecord = resourceIndexToUsers[tex->GetResourceIndex() | (subrscIndex << 32)];
                    if(optimizeConnection && userRecord.layout == subrscUsage->layout &&
                       userRecord.isReadOnly && isCurrReadOnly)
                    {
                        userRecord.users.insert(curr);
                    }
                    else
                    {
                        const bool isUAVOnly = IsUAVOnly(subrscUsage->accesses);
                        if(pass->uavOverlapGroup_.IsValid() &&
                           userRecord.uavOverlapGroup == pass->uavOverlapGroup_ &&
                           isUAVOnly &&  userRecord.layout == subrscUsage->layout)
                        {
                            userRecord.users.insert(curr);
                        }
                        else
                        {
                            userRecord.prevUsers       = userRecord.users;
                            userRecord.users           = { curr };
                            userRecord.layout          = subrscUsage->layout;
                            userRecord.isReadOnly      = isCurrReadOnly;
                            userRecord.uavOverlapGroup = isUAVOnly ? pass->uavOverlapGroup_ : UAVOverlapGroup{};
                        }
                    }
                    for(auto prev : userRecord.prevUsers)
                    {
                        outSuccs[prev->index_].insert(curr);
                        outPrevs[curr->index_].insert(prev);
                    }
                }
                ++subrscIndex;
            }
        }
    }
}

bool Compiler::IsInSection(int passIndex, const CompileSection *section) const
{
    return passToSection_.at(passIndex) == section;
}

void Compiler::ProcessSynchronizedResourceStates()
{
    const RHI::QueueSessionID synchronizedSessionID = graph_->GetQueue().GetRHIObject()->GetSynchronizedSessionID();

    for(auto &buffer : graph_->buffers_)
    {
        if(auto extBuf = TryCastResource<RenderGraph::ExternalBufferResource>(buffer.get());
           extBuf && extBuf->buffer->GetState().queueSessionID <= synchronizedSessionID)
        {
            extBuf->buffer->SetState(BufferState(
                RHI::INITIAL_QUEUE_SESSION_ID, RHI::PipelineStage::None, RHI::ResourceAccess::None));
        }
    }

    for(auto &texture : graph_->textures_)
    {
        if(auto extTex = TryCastResource<RenderGraph::ExternalTextureResource>(texture.get()))
        {
            for(auto &&subrsc : EnumerateSubTextures(extTex->GetDesc()))
            {
                auto &state = extTex->texture->GetState(subrsc);
                if(state.queueSessionID <= synchronizedSessionID)
                {
                    extTex->texture->SetState(subrsc, TexSubrscState(
                        RHI::INITIAL_QUEUE_SESSION_ID, state.layout,
                        RHI::PipelineStage::None, RHI::ResourceAccess::None));
                }
            }
        }
    }
}

void Compiler::TopologySortPasses()
{
    assert(graph_);
    const std::vector<Box<Pass>> &passes = graph_->passes_;

    std::vector<std::set<Pass *>> prevs(passes.size()), succs(passes.size());
    for(size_t i = 0; i < passes.size(); ++i)
    {
        prevs[i] = passes[i]->prevs_;
        succs[i] = passes[i]->succs_;
    }

    if(options_.Contains(Options::ConnectPassesByDefinitionOrder))
    {
        const bool optimize = options_.Contains(CompilerDetail::OptionBit::OptimizePassConnection);
        GenerateConnectionsByDefinitionOrder(optimize, passes, prevs, succs);
    }

    std::queue<int> availablePasses;
    std::vector<int> passToUnprocessedPrevCount(passes.size());
    for(size_t i = 0; i < passes.size(); ++i)
    {
        passToUnprocessedPrevCount[i] = static_cast<int>(prevs[i].size());
        if(!passToUnprocessedPrevCount[i])
        {
            availablePasses.push(static_cast<int>(i));
        }
    }

    std::vector<int> sortedPassIndices;
    sortedPassIndices.reserve(passes.size());
    while(!availablePasses.empty())
    {
        const int passIndex = availablePasses.front();
        availablePasses.pop();
        sortedPassIndices.push_back(passIndex);
        for(Pass *succ : succs[passIndex])
        {
            const int succIndex = succ->index_;
            assert(passToUnprocessedPrevCount[succIndex]);
            if(!--passToUnprocessedPrevCount[succIndex])
            {
                availablePasses.push(succIndex);
            }
        }
    }
    if(sortedPassIndices.size() != passes.size())
    {
        throw Exception("Cycle detected in pass dependency graph");
    }

    sortedPasses_.resize(passes.size());
    for(size_t i = 0; i < sortedPassIndices.size(); ++i)
    {
        Pass *pass = passes[sortedPassIndices[i]].get();
        sortedPasses_[i] = pass;
        passToSortedIndex_[pass] = static_cast<int>(i);
    }
}

void Compiler::CollectResourceUsers()
{
    for(size_t passIndex = 0; passIndex != sortedPasses_.size(); ++passIndex)
    {
        const Pass &pass = *sortedPasses_[passIndex];

        for(auto &[bufferResource, bufferUsage] : pass.bufferUsages_)
        {
            bufferUsers_[bufferResource].push_back(BufferUser
            {
                .passIndex       = static_cast<int>(passIndex),
                .usage           = bufferUsage,
                .uavOverlapGroup = IsUAVOnly(bufferUsage.accesses) ? pass.uavOverlapGroup_ : UAVOverlapGroup{}
            });
        }

        for(auto &[textureResource, textureUsage] : pass.textureUsages_)
        {
            for(auto [mipLevel, arrayLayer] : EnumerateSubTextures(
                textureResource->GetMipLevels(), textureResource->GetArraySize()))
            {
                if(!textureUsage(mipLevel, arrayLayer).has_value())
                {
                    continue;
                }
                const Pass::SubTexUsage &subTexUsage = textureUsage(mipLevel, arrayLayer).value();
                const SubTexKey key = { textureResource, TexSubrsc{ mipLevel, arrayLayer } };
                subTexUsers_[key].push_back(SubTexUser
                {
                    .passIndex       = static_cast<int>(passIndex),
                    .usage           = subTexUsage,
                    .uavOverlapGroup = IsUAVOnly(subTexUsage.accesses) ? pass.uavOverlapGroup_ : UAVOverlapGroup{}
                });
            }

#if RTRC_DEBUG
            if(const auto extRsc = dynamic_cast<const RenderGraph::ExternalTextureResource *>(textureResource);
               extRsc && extRsc->isReadOnlySampledTexture)
            {
                for(auto [mipLevel, arrayLayer] : EnumerateSubTextures(extRsc->GetMipLevels(), extRsc->GetArraySize()))
                {
                    if(!textureUsage(mipLevel, arrayLayer).has_value())
                    {
                        continue;
                    }
                    const Pass::SubTexUsage &subTexUsage = textureUsage(mipLevel, arrayLayer).value();
                    if(subTexUsage.layout != RHI::TextureLayout::ShaderTexture)
                    {
                        throw Exception("External readonly texture is used in non ShaderTexture layout");
                    }
                }
            }
#endif
        }
    }
}

void Compiler::GenerateSections()
{
    // need splitting when:
    //      signaling fence is not nil
    //      be the last user of swapchain texture
    //      next pass is the first user of swapchain texture (disabled)

    const SubTexUsers *swapchainTexUsers = nullptr;
    if(graph_->swapchainTexture_)
    {
        if(auto it = subTexUsers_.find(SubTexKey{ graph_->swapchainTexture_, { 0, 0 } }); it != subTexUsers_.end())
        {
            swapchainTexUsers = &it->second;
        }
    }

    bool needNewSection = true;
    for(int passIndex = 0; passIndex < static_cast<int>(sortedPasses_.size()); ++passIndex)
    {
        const Pass *pass = sortedPasses_[passIndex];
        if(needNewSection)
        {
            sections_.push_back(MakeBox<CompileSection>());
        }

        assert(!sections_.empty());
        auto &section = sections_.back();
        section->passes.push_back(passIndex);
        passToSection_.push_back(section.get());

        assert(!section->signalFence);
        section->signalFence = pass->signalFence_;
        needNewSection = pass->signalFence_ != nullptr;

        if(swapchainTexUsers)
        {
            needNewSection |= passIndex == swapchainTexUsers->back().passIndex;
        }
    }
}

void Compiler::GenerateSemaphores()
{
    const SubTexUsers *swapchainTextureUsers = nullptr;
    for(auto &[subTexKey, users] : subTexUsers_)
    {
        if(subTexKey.first == graph_->swapchainTexture_)
        {
            swapchainTextureUsers = &users;
            break;
        }
    }

    if(swapchainTextureUsers)
    {
        const SubTexUsers &users = *swapchainTextureUsers;

        CompileSection *firstSection = passToSection_.at(users[0].passIndex);
        firstSection->waitBackbufferSemaphore = true;
        firstSection->waitBackbufferSemaphoreStages |= users[0].usage.stages;
        for(int j = 1; j < static_cast<int>(users.size()) && DontNeedBarrier(users[j].usage, users[0].usage); ++j)
        {
            firstSection->waitBackbufferSemaphoreStages |= users[j].usage.stages;
        }

        CompileSection *lastSection = passToSection_.at(users.back().passIndex);
        lastSection->signalBackbufferSemaphore = true;
        lastSection->signalBackbufferSemaphoreStages |= users.back().usage.stages;
        for(int j = static_cast<int>(users.size()) - 1;
            j >= 0 && DontNeedBarrier(users[j].usage, users.back().usage); --j)
        {
            lastSection->signalBackbufferSemaphoreStages |= users[j].usage.stages;
        }
    }
}

void Compiler::FillExternalResources(ExecutableResources &output)
{
    for(auto &buffer : graph_->buffers_)
    {
        if(auto ext = TryCastResource<RenderGraph::ExternalBufferResource>(buffer.get()))
        {
            const int index = ext->GetResourceIndex();
            output.indexToBuffer[index].buffer = ext->buffer;

            auto usersIt = bufferUsers_.find(ext);
            if(usersIt != bufferUsers_.end())
            {
                auto &users = usersIt->second;
                RHI::PipelineStageFlag stages = users.back().usage.stages;
                RHI::ResourceAccessFlag accesses = users.back().usage.accesses;
                for(int j = static_cast<int>(users.size()) - 1;
                    j >= 0 && DontNeedBarrier(users[j].usage, users.back().usage); --j)
                {
                    stages |= users[j].usage.stages;
                    accesses |= users[j].usage.accesses;
                }
                output.indexToBuffer[index].finalState = BufferState(RHI::INITIAL_QUEUE_SESSION_ID, stages, accesses);
            }
            else
            {
                output.indexToBuffer[index].finalState = ext->buffer->GetState();
            }
        }
    }

    for(auto &texture : graph_->textures_)
    {
        if(auto ext = TryCastResource<RenderGraph::ExternalTextureResource>(texture.get()))
        {
            const int index = ext->GetResourceIndex();
            output.indexToTexture[index].texture = ext->texture;

            auto &finalStates = output.indexToTexture[index].finalState;
            finalStates = TextureSubrscMap<std::optional<TexSubrscState>>(ext->texture->GetDesc());
            if(TryCastResource<RenderGraph::SwapchainTexture>(ext))
            {
                finalStates(0, 0) = TexSubrscState(
                    RHI::INITIAL_QUEUE_SESSION_ID, RHI::TextureLayout::Present,
                    RHI::PipelineStage::None, RHI::ResourceAccess::None);
            }
            else
            {
                for(auto subrsc : EnumerateSubTextures(ext->GetMipLevels(), ext->GetArraySize()))
                {
                    auto usersIt = subTexUsers_.find({ ext, subrsc });
                    if(usersIt != subTexUsers_.end())
                    {
                        auto &users = usersIt->second;
                        const RHI::TextureLayout layout = users.back().usage.layout;
                        RHI::PipelineStageFlag stages = users.back().usage.stages;
                        RHI::ResourceAccessFlag accesses = users.back().usage.accesses;
                        for(int j = static_cast<int>(users.size()) - 1;
                            j >= 0 && DontNeedBarrier(users[j].usage, users.back().usage); --j)
                        {
                            stages |= users[j].usage.stages;
                            accesses |= users[j].usage.accesses;
                        }
                        finalStates(subrsc.mipLevel, subrsc.arrayLayer) = TexSubrscState(
                            RHI::INITIAL_QUEUE_SESSION_ID, layout, stages, accesses);
                    }
                    else
                    {
                        finalStates(subrsc.mipLevel, subrsc.arrayLayer) =
                            ext->texture->GetState(subrsc.mipLevel, subrsc.arrayLayer);
                    }
                }
            }
        }
    }
}

void Compiler::AllocateInternalResourcesLegacy(ExecutableResources &output)
{
    for(auto &buf : graph_->buffers_)
    {
        auto intRsc = TryCastResource<RenderGraph::InternalBufferResource>(buf.get());
        if(!intRsc)
        {
            continue;
        }

        const RHI::BufferDesc &desc = intRsc->rhiDesc_;
        const int resourceIndex = intRsc->GetResourceIndex();

        auto usersIt = bufferUsers_.find(intRsc);
        if(usersIt != bufferUsers_.end())
        {
            auto &users = usersIt->second;
            RHI::PipelineStageFlag stages = users.back().usage.stages;
            RHI::ResourceAccessFlag accesses = users.back().usage.accesses;
            for(int j = static_cast<int>(users.size()) - 1;
                j >= 0 && DontNeedBarrier(users[j].usage, users.back().usage); --j)
            {
                stages |= users[j].usage.stages;
                accesses |= users[j].usage.accesses;
            }
            output.indexToBuffer[resourceIndex].finalState = BufferState(
                RHI::INITIAL_QUEUE_SESSION_ID, stages, accesses);
        }
        else
        {
            output.indexToBuffer[resourceIndex].finalState = BufferState{};
        }

        if(desc.hostAccessType == RHI::BufferHostAccessType::None)
        {
            output.indexToBuffer[resourceIndex].buffer = device_->CreateStatefulBuffer(RHI::BufferDesc
            {
                .size = desc.size,
                .usage = desc.usage,
                .hostAccessType = desc.hostAccessType
            });
        }
        else
        {
            auto buffer = device_->CreateBuffer(desc);
            output.indexToBuffer[resourceIndex].buffer = MakeRC<WrappedStatefulBuffer>(std::move(buffer), BufferState{});
        }

        output.indexToBuffer[resourceIndex].buffer->SetDefaultStructStride(intRsc->defaultStructStride_);
        output.indexToBuffer[resourceIndex].buffer->SetDefaultTexelFormat(intRsc->defaultTexelFormat_);
        output.indexToBuffer[resourceIndex].buffer->SetName(std::move(intRsc->name_));
    }

    for(auto &texture : graph_->textures_)
    {
        auto intRsc = TryCastResource<RenderGraph::InternalTextureResource>(texture.get());
        if(!intRsc)
        {
            continue;
        }

        const int resourceIndex = intRsc->GetResourceIndex();
        auto &desc = intRsc->rhiDesc;

        auto &finalStates = output.indexToTexture[resourceIndex].finalState;
        finalStates = TextureSubrscMap<std::optional<TexSubrscState>>(
            intRsc->GetMipLevels(), intRsc->GetArraySize());
        for(auto subrsc : EnumerateSubTextures(intRsc->GetMipLevels(), intRsc->GetArraySize()))
        {
            auto usersIt = subTexUsers_.find({ intRsc, subrsc });
            if(usersIt != subTexUsers_.end())
            {
                auto &users = usersIt->second;
                const RHI::TextureLayout layout = users.back().usage.layout;
                RHI::PipelineStageFlag stages = users.back().usage.stages;
                RHI::ResourceAccessFlag accesses = users.back().usage.accesses;
                for(int j = static_cast<int>(users.size()) - 1;
                    j >= 0 && DontNeedBarrier(users[j].usage, users.back().usage); --j)
                {
                    stages |= users[j].usage.stages;
                    accesses |= users[j].usage.accesses;
                }
                finalStates(subrsc.mipLevel, subrsc.arrayLayer) = TexSubrscState(
                    RHI::INITIAL_QUEUE_SESSION_ID, layout, stages, accesses);
            }
            else
            {
                finalStates(subrsc.mipLevel, subrsc.arrayLayer) = TexSubrscState{};
            }
        }

        output.indexToTexture[resourceIndex].texture = device_->CreateStatefulTexture(desc);
        output.indexToTexture[resourceIndex].texture->SetLayoutToUndefined();
        output.indexToTexture[resourceIndex].texture->SetName(std::move(intRsc->name));
    }
}

void Compiler::AllocateInternalResources(ExecutableResources &output, std::vector<std::vector<int>> &aliasedPrevs)
{
    std::vector<int> tranasientResourceIndices;
    std::vector<RHI::TransientResourceDeclaration> transientResourceDecls;

    auto ComputeFinalState = [&]<typename T>(std::vector<ResourceUser<T>> &users)
    {
        assert(!users.empty());
        T state = users.back().usage;
        for(int j = static_cast<int>(users.size()) - 1;
            j >= 0 && DontNeedBarrier(users[j].usage, users.back().usage); --j)
        {
            state.stages |= users[j].usage.stages;
            state.accesses |= users[j].usage.accesses;
        }
        return state;
    };

    for(auto &buf : graph_->buffers_)
    {
        auto intRsc = TryCastResource<RenderGraph::InternalBufferResource>(buf.get());
        if(!intRsc)
        {
            continue;
        }

        const int rscIdx = intRsc->GetResourceIndex();
        auto userIt = bufferUsers_.find(intRsc);
        if(userIt == bufferUsers_.end()) // skip this resource since no one use it
        {
            output.indexToBuffer[rscIdx].buffer = {};
            output.indexToBuffer[rscIdx].finalState = BufferState{};
            continue;
        }

        // Compute final state

        auto &users = userIt->second;
        output.indexToBuffer[rscIdx].finalState = ComputeFinalState(users);

        // Usage through device address cannot be tracked by debug layer easily. Allocate it separately.

        if(intRsc->rhiDesc_.usage.Contains(RHI::BufferUsage::DeviceAddress))
        {
            auto &desc = intRsc->rhiDesc_;
            output.indexToBuffer[rscIdx].buffer = device_->CreateStatefulBuffer(RHI::BufferDesc
                {
                    .size = desc.size,
                    .usage = desc.usage,
                    .hostAccessType = desc.hostAccessType
                });
            output.indexToBuffer[rscIdx].buffer->SetName(std::move(intRsc->name_));
            continue;
        }

        // Prepare transient resource declaration

        RHI::TransientBufferDeclaration decl;
        decl.desc      = intRsc->rhiDesc_;
        decl.beginPass = users.front().passIndex;
        decl.endPass   = users.back().passIndex;
        tranasientResourceIndices.push_back(rscIdx);
        transientResourceDecls.emplace_back(std::move(decl));
    }

    for(auto& tex : graph_->textures_)
    {
        auto intRsc = TryCastResource<RenderGraph::InternalTextureResource>(tex.get());
        if(!intRsc)
        {
            continue;
        }

        const int rscIdx = intRsc->GetResourceIndex();
        int begPass = std::numeric_limits<int>::max();
        int endPass = std::numeric_limits<int>::lowest();

        auto &finalStates = output.indexToTexture[rscIdx].finalState;
        finalStates = TextureSubrscMap<std::optional<TexSubrscState>>(
            intRsc->GetMipLevels(), intRsc->GetArraySize());

        bool used = false;
        for(auto subrsc : EnumerateSubTextures(intRsc->GetMipLevels(), intRsc->GetArraySize()))
        {
            auto userIt = subTexUsers_.find({ intRsc, subrsc });
            if(userIt != subTexUsers_.end())
            {
                used = true;
                auto &users = userIt->second;
                begPass = (std::min)(begPass, users.front().passIndex);
                endPass = (std::max)(endPass, users.back().passIndex);
                finalStates(subrsc.mipLevel, subrsc.arrayLayer) = ComputeFinalState(users);
            }
            else
            {
                finalStates(subrsc.mipLevel, subrsc.arrayLayer) = TexSubrscState{};
            }
        }

        if(!used)
        {
            continue;
        }

        RHI::TransientTextureDeclaration decl;
        decl.desc      = intRsc->rhiDesc;
        decl.beginPass = begPass;
        decl.endPass   = endPass;
        tranasientResourceIndices.push_back(rscIdx);
        transientResourceDecls.emplace_back(std::move(decl));
    }

    std::vector<RHI::AliasedTransientResourcePair> aliasedPairs;
    transientResourcePool_->Allocate(transientResourceDecls, aliasedPairs);

    class TransientBuffer : public Buffer
    {
    public:

        TransientBuffer(RHI::BufferRPtr rhiBuffer, RHI::Format defaultTexelFormat, uint32_t defaultStride)
        {
            size_                    = rhiBuffer->GetDesc().size;
            rhiBuffer_               = std::move(rhiBuffer);
            defaultViewTexelFormat_  = defaultTexelFormat;
            defaultViewStructStride_ = defaultStride;
            manager_                 = nullptr;
        }
    };

    class TransientTexture : public Texture
    {
    public:

        explicit TransientTexture(RHI::TextureRPtr rhiTexture)
        {
            desc_       = rhiTexture->GetDesc();
            rhiTexture_ = std::move(rhiTexture);
            manager_    = nullptr;
        }
    };

    for(auto &pair : aliasedPairs)
    {
        const int a = tranasientResourceIndices[pair.first];
        const int b = tranasientResourceIndices[pair.second];
        aliasedPrevs[b].push_back(a);
    }

    for(size_t i = 0; i < tranasientResourceIndices.size(); ++i)
    {
        const int rscIdx = tranasientResourceIndices[i];
        transientResourceDecls[i].Match(
            [&](RHI::TransientBufferDeclaration &decl)
            {
                auto rsc = TryCastResource<RenderGraph::InternalBufferResource>(graph_->buffers_[rscIdx].get());
                output.indexToBuffer[rscIdx].buffer = StatefulBuffer::FromBuffer(MakeRC<TransientBuffer>(
                    std::move(decl.buffer), rsc->defaultTexelFormat_, static_cast<uint32_t>(rsc->defaultStructStride_)));
                output.indexToBuffer[rscIdx].buffer->SetName(rsc->name_);
            },
            [&](RHI::TransientTextureDeclaration &decl)
            {
                auto rsc = TryCastResource<RenderGraph::InternalTextureResource>(graph_->textures_[rscIdx].get());
                output.indexToTexture[rscIdx].texture = StatefulTexture::FromTexture(
                    MakeRC<TransientTexture>(std::move(decl.texture)));
                output.indexToTexture[rscIdx].texture->SetName(rsc->name);
            });

    }
}

void Compiler::GenerateBarriers(const ExecutableResources &resources)
{
    for(auto &[buffer, users] : bufferUsers_)
    {
        const int rscIdx = buffer->GetResourceIndex();
        if(!resources.indexToBuffer[rscIdx].buffer)
        {
            continue;
        }

        BufferState lastState = resources.indexToBuffer[rscIdx].buffer->GetState();
        if(!aliasedPrevs_[rscIdx].empty())
        {
            assert(lastState.stages == RHI::PipelineStage::None);
            assert(lastState.accesses == RHI::ResourceAccess::None);
            for(int prev : aliasedPrevs_[rscIdx])
            {
                if(graph_->buffers_[prev])
                {
                    auto &prevFinalState = resources.indexToBuffer[prev].finalState;
                    lastState.stages |= prevFinalState.stages;
                    lastState.accesses |= prevFinalState.accesses;
                }
                else
                {
                    for(auto &prevFinalState : resources.indexToTexture[prev].finalState)
                    {
                        if(prevFinalState)
                        {
                            lastState.stages |= prevFinalState->stages;
                            lastState.accesses |= prevFinalState->accesses;
                        }
                    }
                }
            }
        }

        uint32_t userIndex = 0;
        while(userIndex < users.size())
        {
            auto CanMerge = [&](uint32_t ai, uint32_t bi)
            {
                auto &ua = users[ai], &ub = users[bi];
                if(DontNeedBarrier(ua.usage, ub.usage))
                {
                    return true;
                }
                if(ua.uavOverlapGroup.IsValid() && ua.uavOverlapGroup == ub.uavOverlapGroup)
                {
                    return true;
                }
                return false;
            };

            uint32_t nextUserIndex = userIndex + 1;
            while(nextUserIndex < users.size() && CanMerge(userIndex, nextUserIndex))
            {
                ++nextUserIndex;
            }
            RTRC_SCOPE_EXIT{ userIndex = nextUserIndex; };

            BufferState currState = BufferState(
                RHI::INITIAL_QUEUE_SESSION_ID, RHI::PipelineStage::None, RHI::ResourceAccess::None);
            RTRC_SCOPE_EXIT{ lastState = currState; };
            for(size_t i = userIndex; i < nextUserIndex; ++i)
            {
                currState.stages |= users[i].usage.stages;
                currState.accesses |= users[i].usage.accesses;
            }

            if(DontNeedBarrier(lastState, currState))
            {
                currState.stages |= lastState.stages;
                currState.accesses |= lastState.accesses;
                continue;
            }

            const int minBarrierPass = userIndex > 0 ? (users[userIndex - 1].passIndex + 1) : 0;
            const int maxBarrierPass = users[userIndex].passIndex;
            int barrierPass = users[userIndex].passIndex;
            for(int i = maxBarrierPass - 1; i >= minBarrierPass; --i)
            {
                const bool hasBarrier = !sortedCompilePasses_[i]->preBufferTransitions.empty()
                                     || !sortedCompilePasses_[i]->preTextureTransitions.empty();
                if(hasBarrier)
                {
                    barrierPass = i;
                    break;
                }
            }

            sortedCompilePasses_[barrierPass]->preBufferTransitions.push_back(RHI::BufferTransitionBarrier
            {
                .buffer         = resources.indexToBuffer[buffer->GetResourceIndex()].buffer->GetRHIObject(),
                .beforeStages   = lastState.stages,
                .beforeAccesses = lastState.accesses,
                .afterStages    = currState.stages,
                .afterAccesses  = currState.accesses
            });
        }
    }

    for(auto &[subTexKey, users] : subTexUsers_)
    {
        auto [tex, subrsc] = subTexKey;
        const int rscIdx = tex->GetResourceIndex();
        const bool isSwapchainTex = TryCastResource<RenderGraph::SwapchainTexture>(tex);
        TexSubrscState lastState = resources.indexToTexture[rscIdx].texture
                                        ->GetState(subrsc.mipLevel, subrsc.arrayLayer);

        if(!aliasedPrevs_[rscIdx].empty())
        {
            assert(lastState.layout == RHI::TextureLayout::Undefined);
            assert(lastState.stages == RHI::PipelineStage::None);
            assert(lastState.accesses == RHI::ResourceAccess::None);
            for(int prev : aliasedPrevs_[rscIdx])
            {
                if(graph_->buffers_[prev])
                {
                    auto &prevFinalState = resources.indexToBuffer[prev].finalState;
                    lastState.stages |= prevFinalState.stages;
                    lastState.accesses |= prevFinalState.accesses;
                }
                else
                {
                    for(auto &prevFinalState : resources.indexToTexture[prev].finalState)
                    {
                        if(prevFinalState)
                        {
                            lastState.stages |= prevFinalState->stages;
                            lastState.accesses |= prevFinalState->accesses;
                        }
                    }
                }
            }
        }

        uint32_t userIndex = 0;
        while(userIndex < users.size())
        {
            auto CanMerge = [&](uint32_t ai, uint32_t bi)
            {
                auto &ua = users[ai], &ub = users[bi];
                if(DontNeedBarrier(ua.usage, ub.usage))
                {
                    return true;
                }
                if(ua.uavOverlapGroup.IsValid() &&
                   ua.usage.layout == ub.usage.layout &&
                   ua.uavOverlapGroup == ub.uavOverlapGroup)
                {
                    return true;
                }
                return false;
            };

            uint32_t nextUserIndex = userIndex + 1;
            while(nextUserIndex < users.size() && CanMerge(userIndex, nextUserIndex))
            {
                ++nextUserIndex;
            }
            RTRC_SCOPE_EXIT{ userIndex = nextUserIndex; };

            TexSubrscState currState;
            RTRC_SCOPE_EXIT{ lastState = currState; };
            currState.layout = users[userIndex].usage.layout;
            for(size_t i = userIndex; i < nextUserIndex; ++i)
            {
                currState.stages |= users[i].usage.stages;
                currState.accesses |= users[i].usage.accesses;
            }

            if(DontNeedBarrier(lastState, currState))
            {
                currState.stages |= lastState.stages;
                currState.accesses |= lastState.accesses;
                continue;
            }

            int minBarrierPass = userIndex > 0 ? (users[userIndex - 1].passIndex + 1) : 0;
            if(userIndex == 0 && TryCastResource<RenderGraph::SwapchainTexture>(tex))
            {
                while(passToSection_[minBarrierPass] != passToSection_[userIndex])
                {
                    ++minBarrierPass;
                }
            }

            const int maxBarrierPass = users[userIndex].passIndex;
            int barrierPass = users[userIndex].passIndex;
            for(int i = maxBarrierPass - 1; i >= minBarrierPass; --i)
            {
                const bool hasBarrier = !sortedCompilePasses_[i]->preBufferTransitions.empty()
                                     || !sortedCompilePasses_[i]->preTextureTransitions.empty();
                if(hasBarrier)
                {
                    barrierPass = i;
                    break;
                }
            }

            if(isSwapchainTex && userIndex == 0)
            {
                assert(lastState.accesses == RHI::ResourceAccess::None);
                assert(lastState.layout == RHI::TextureLayout::Undefined);
                sortedCompilePasses_[barrierPass]->preTextureTransitions.push_back(RHI::TextureTransitionBarrier
                {
                    .texture      = resources.indexToTexture[tex->GetResourceIndex()].texture->GetRHIObject(),
                    .subresources = TexSubrscs
                    {
                        .mipLevel = subrsc.mipLevel,
                        .levelCount = 1,
                        .arrayLayer = subrsc.arrayLayer,
                        .layerCount = 1
                    },
                    .beforeStages   = currState.stages,
                    .beforeAccesses = lastState.accesses,
                    .beforeLayout   = RHI::TextureLayout::Present,
                    .afterStages    = currState.stages,
                    .afterAccesses  = currState.accesses,
                    .afterLayout    = currState.layout
                });
            }
            else
            {
                sortedCompilePasses_[barrierPass]->preTextureTransitions.push_back(RHI::TextureTransitionBarrier
                {
                    .texture      = resources.indexToTexture[tex->GetResourceIndex()].texture->GetRHIObject(),
                    .subresources = TexSubrscs
                    {
                        .mipLevel   = subrsc.mipLevel,
                        .levelCount = 1,
                        .arrayLayer = subrsc.arrayLayer,
                        .layerCount = 1
                    },
                    .beforeStages   = lastState.stages,
                    .beforeAccesses = lastState.accesses,
                    .beforeLayout   = lastState.layout,
                    .afterStages    = currState.stages,
                    .afterAccesses  = currState.accesses,
                    .afterLayout    = currState.layout
                });
            }
        }

        if(isSwapchainTex)
        {
            passToSection_[users.back().passIndex]->swapchainPresentBarrier = RHI::TextureTransitionBarrier
            {
                .texture        = resources.indexToTexture[tex->GetResourceIndex()].texture->GetRHIObject(),
                .beforeStages   = lastState.stages,
                .beforeAccesses = IsReadOnly(lastState.accesses) ? RHI::ResourceAccess::None : lastState.accesses,
                .beforeLayout   = lastState.layout,
                .afterStages    = RHI::PipelineStage::None,
                .afterAccesses  = RHI::ResourceAccess::None,
                .afterLayout    = RHI::TextureLayout::Present
            };
        }
    }
}

void Compiler::FillSections(ExecutableGraph &output)
{
    bool initialGlobalBarrierEmitted = false;
    const bool hasGoodBarrierMemoryModel =
        device_->GetRawDevice()->GetBarrierMemoryModel() == RHI::BarrierMemoryModel::AvailableAndVisible;

    output.sections.clear();
    output.sections.reserve(sections_.size());
    for(const auto &compileSection : sections_)
    {
        ExecutableSection &section = output.sections.emplace_back();

        if(compileSection->waitBackbufferSemaphore)
        {
            section.waitAcquireSemaphore = graph_->swapchainTexture_->acquireSemaphore;
            section.waitAcquireSemaphoreStages = compileSection->waitBackbufferSemaphoreStages;
        }
        else
        {
            section.waitAcquireSemaphore = nullptr;
            section.waitAcquireSemaphoreStages = RHI::PipelineStage::None;
        }

        if(compileSection->signalBackbufferSemaphore)
        {
            section.signalPresentSemaphore = graph_->swapchainTexture_->presentSemaphore;
            section.signalPresentSemaphoreStages = compileSection->signalBackbufferSemaphoreStages;
        }
        else
        {
            section.signalPresentSemaphore = nullptr;
            section.signalPresentSemaphoreStages = RHI::PipelineStage::None;
        }

        if(compileSection->swapchainPresentBarrier)
        {
            section.postTextureBarriers.PushBack(compileSection->swapchainPresentBarrier.value());
        }

        section.signalFence = compileSection->signalFence;

        section.passes.reserve(compileSection->passes.size());
        for(const int compilePassIndex : compileSection->passes)
        {
            Pass          *&rawPass     = sortedPasses_[compilePassIndex];
            CompilePass    &compilePass = *sortedCompilePasses_[compilePassIndex];
            ExecutablePass &pass        = section.passes.emplace_back();

            pass.preBufferBarriers = std::move(compilePass.preBufferTransitions);
            if(hasGoodBarrierMemoryModel)
            {
                for(RHI::BufferTransitionBarrier &b : pass.preBufferBarriers)
                {
                    CompilerDetail::RemoveUnnecessaryBufferAccessMask(b.beforeAccesses, b.afterAccesses);
                }
            }

            pass.preTextureBarriers = std::move(compilePass.preTextureTransitions);
            if(hasGoodBarrierMemoryModel)
            {
                for(RHI::TextureTransitionBarrier &b : pass.preTextureBarriers)
                {
                    CompilerDetail::RemoveUnnecessaryTextureAccessMask(
                        b.beforeLayout, b.afterLayout, b.beforeAccesses, b.afterAccesses);
                }
            }

            if(options_.Contains(Options::PreferGlobalMemoryBarrier))
            {
                GenerateGlobalMemoryBarriers(pass);
            }

            if(!initialGlobalBarrierEmitted)
            {
                initialGlobalBarrierEmitted = true;
                pass.preGlobalBarrier = RHI::GlobalMemoryBarrier
                {
                    .beforeStages   = RHI::PipelineStage::All,
                    .beforeAccesses = RHI::ResourceAccess::All,
                    .afterStages    = RHI::PipelineStage::All,
                    .afterAccesses  = RHI::ResourceAccess::All
                };
            }

            pass.nameNode = rawPass->nameNode_;

            if(sortedPasses_[compilePassIndex]->callback_)
            {
                pass.callback = &rawPass->callback_;
            }
            else
            {
                pass.callback = nullptr;
            }

#if RTRC_RG_DEBUG
            for(auto &r : rawPass->bufferUsages_)
            {
                pass.declaredResources.insert(r.first);
            }
            for(auto &t : rawPass->textureUsages_)
            {
                pass.declaredResources.insert(t.first);
            }
#endif
        }
    }
}

RTRC_RG_END
