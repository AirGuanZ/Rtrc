#include <queue>
#include <ranges>

#include <Rtrc/Graphics/RenderGraph/Compiler.h>
#include <Rtrc/Graphics/RenderGraph/Graph.h>

RTRC_RG_BEGIN

namespace CompilerDetail
{

    void RemoveUnnecessaryAccessMask(RHI::ResourceAccessFlag &beforeAccess, RHI::ResourceAccessFlag &afterAccess)
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

} // namespace CompilerDetail

Compiler::Compiler(Device &device)
    : device_(device)
    , graph_(nullptr)
{
    
}

void Compiler::Compile(const RenderGraph &graph, ExecutableGraph &result)
{
    graph_ = &graph;

    TopologySortPasses();
    CollectResourceUsers();

    GenerateSections();
    GenerateSemaphores();

    result.resources.indexToBuffer.resize(graph_->buffers_.size());
    result.resources.indexToTexture.resize(graph_->textures_.size());
    FillExternalResources(result.resources);
    AllocateInternalResources(result.resources);

    sortedCompilePasses_.resize(sortedPasses_.size());
    for(auto &p : sortedCompilePasses_)
    {
        p = MakeBox<CompilePass>();
    }
    GenerateBarriers(result.resources);

    result.queue = graph.queue_.GetRHIObject();
    FillSections(result);
}

bool Compiler::DontNeedBarrier(const Pass::BufferUsage &a, const Pass::BufferUsage &b)
{
    return IsReadOnly(a.accesses) && IsReadOnly(b.accesses);
}

bool Compiler::DontNeedBarrier(const Pass::SubTexUsage &a, const Pass::SubTexUsage &b)
{
    return a.layout == b.layout && IsReadOnly(a.accesses) && IsReadOnly(b.accesses);
}

bool Compiler::IsInSection(int passIndex, const CompileSection *section) const
{
    return passToSection_.at(passIndex) == section;
}

void Compiler::TopologySortPasses()
{
    // TODO: optimize for section generation

    assert(graph_);
    const std::vector<Box<Pass>> &passes = graph_->passes_;

    std::queue<int> availablePasses;
    std::vector<int> passToUnprocessedPrevCount(passes.size());
    for(size_t i = 0; i < passes.size(); ++i)
    {
        passToUnprocessedPrevCount[i] = static_cast<int>(passes[i]->prevs_.size());
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
        for(Pass *succ : passes[passIndex]->succs_)
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
        throw Exception("cycle detected in pass dependency graph");
    }

    sortedPasses_.resize(passes.size());
    for(size_t i = 0; i < sortedPassIndices.size(); ++i)
    {
        const Pass *pass = passes[sortedPassIndices[i]].get();
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
                .passIndex = static_cast<int>(passIndex),
                .usage = bufferUsage
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
                const SubTexKey key = { textureResource, RHI::TextureSubresource{ mipLevel, arrayLayer } };
                subTexUsers_[key].push_back(SubTexUser
                    {
                        .passIndex = static_cast<int>(passIndex),
                        .usage = subTexUsage
                    });
            }
        }
    }
}

void Compiler::GenerateSections()
{
    // need splitting when:
    //      signaling fence is not nil
    //      be the last user of swapchain texture
    //      next pass is the first user of swapchain texture

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
            needNewSection |= passIndex + 1 == swapchainTexUsers->front().passIndex;
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

            auto &users = bufferUsers_.at(ext);
            RHI::PipelineStageFlag stages = users.back().usage.stages;
            RHI::ResourceAccessFlag accesses = users.back().usage.accesses;
            for(int j = static_cast<int>(users.size()) - 1;
                j >= 0 && DontNeedBarrier(users[j].usage, users.back().usage); --j)
            {
                stages |= users[j].usage.stages;
                accesses |= users[j].usage.accesses;
            }
            output.indexToBuffer[index].finalState = BufferState(stages, accesses);
        }
    }

    for(auto &texture : graph_->textures_)
    {
        if(auto ext = TryCastResource<RenderGraph::ExternalTextureResource>(texture.get()))
        {
            const int index = ext->GetResourceIndex();
            output.indexToTexture[index].texture = ext->texture;

            auto &finalStates = output.indexToTexture[index].finalState;
            finalStates = TextureSubrscMap<std::optional<TextureSubrscState>>(ext->texture->GetDesc());
            if(TryCastResource<RenderGraph::SwapchainTexture>(ext))
            {
                finalStates(0, 0) = TextureSubrscState(
                    RHI::TextureLayout::Present, RHI::PipelineStage::None, RHI::ResourceAccess::None);
            }
            else
            {
                for(auto subrsc : EnumerateSubTextures(ext->GetMipLevels(), ext->GetArraySize()))
                {
                    auto &users = subTexUsers_.at({ ext, subrsc });
                    const RHI::TextureLayout layout = users.back().usage.layout;
                    RHI::PipelineStageFlag stages = users.back().usage.stages;
                    RHI::ResourceAccessFlag accesses = users.back().usage.accesses;
                    for(int j = static_cast<int>(users.size()) - 1;
                        j >= 0 && DontNeedBarrier(users[j].usage, users.back().usage); --j)
                    {
                        stages |= users[j].usage.stages;
                        accesses |= users[j].usage.accesses;
                    }
                    finalStates(subrsc.mipLevel, subrsc.arrayLayer) =
                        TextureSubrscState(layout, stages, accesses);
                }
            }
        }
    }
}

void Compiler::AllocateInternalResources(ExecutableResources &output)
{
    for(auto &buf : graph_->buffers_)
    {
        auto intRsc = TryCastResource<RenderGraph::InternalBufferResource>(buf.get());
        if(!intRsc)
        {
            continue;
        }

        const RHI::BufferDesc &desc = intRsc->rhiDesc;
        const int resourceIndex = intRsc->GetResourceIndex();

        auto &users = bufferUsers_.at(intRsc);
        RHI::PipelineStageFlag stages = users.back().usage.stages;
        RHI::ResourceAccessFlag accesses = users.back().usage.accesses;
        for(int j = static_cast<int>(users.size()) - 1;
            j >= 0 && DontNeedBarrier(users[j].usage, users.back().usage); --j)
        {
            stages |= users[j].usage.stages;
            accesses |= users[j].usage.accesses;
        }
        output.indexToBuffer[resourceIndex].finalState = BufferState(stages, accesses);

        if(desc.hostAccessType == RHI::BufferHostAccessType::None)
        {
            output.indexToBuffer[resourceIndex].buffer = device_.CreatePooledBuffer(RHI::BufferDesc
                {
                    .size = desc.size,
                    .usage = desc.usage,
                    .hostAccessType = desc.hostAccessType
                });
        }
        else
        {
            auto buffer = device_.CreateBuffer(desc);
            output.indexToBuffer[resourceIndex].buffer = MakeRC<WrappedStatefulBuffer>(std::move(buffer), BufferState{});
        }
    }

    for(auto &texture : graph_->textures_)
    {
        auto intRsc = TryCastResource<RenderGraph::InternalTextureResource>(texture.get());
        if(!intRsc)
        {
            continue;
        }

        const int resourceIndex = intRsc->GetResourceIndex();
        auto intTex = TryCastResource<RenderGraph::InternalTextureResource>(intRsc);
        assert(intTex);

        auto &desc = intTex->rhiDesc;

        auto &finalStates = output.indexToTexture[resourceIndex].finalState;
        finalStates = TextureSubrscMap<std::optional<TextureSubrscState>>(
            intTex->GetMipLevels(), intTex->GetArraySize());
        for(auto subrsc : EnumerateSubTextures(intRsc->GetMipLevels(), intRsc->GetArraySize()))
        {
            auto &users = subTexUsers_.at({ intRsc, subrsc });
            const RHI::TextureLayout layout = users.back().usage.layout;
            RHI::PipelineStageFlag stages = users.back().usage.stages;
            RHI::ResourceAccessFlag accesses = users.back().usage.accesses;
            for(int j = static_cast<int>(users.size()) - 1;
                j >= 0 && DontNeedBarrier(users[j].usage, users.back().usage); --j)
            {
                stages |= users[j].usage.stages;
                accesses |= users[j].usage.accesses;
            }
            finalStates(subrsc.mipLevel, subrsc.arrayLayer) = TextureSubrscState(layout, stages, accesses);
        }

        output.indexToTexture[resourceIndex].texture = device_.CreatePooledTexture(desc);
    }
}

void Compiler::GenerateBarriers(const ExecutableResources &resources)
{
    for(auto &[buffer, users] : bufferUsers_)
    {
        auto lastState = resources.indexToBuffer[buffer->GetResourceIndex()].buffer->GetState();

        size_t userIndex = 0;
        while(userIndex < users.size())
        {
            size_t nextUserIndex = userIndex + 1;
            while(nextUserIndex < users.size() && DontNeedBarrier(users[userIndex].usage, users[nextUserIndex].usage))
            {
                ++nextUserIndex;
            }
            RTRC_SCOPE_EXIT{ userIndex = nextUserIndex; };

            BufferState currState = BufferState(RHI::PipelineStage::None, RHI::ResourceAccess::None);
            RTRC_SCOPE_EXIT{ lastState = currState; };
            for(size_t i = userIndex; i < nextUserIndex; ++i)
            {
                currState.stages |= users[i].usage.stages;
                currState.accesses |= users[i].usage.accesses;
            }

            if(DontNeedBarrier(lastState, currState))
            {
                continue;
            }

            const int minBarrierPass = userIndex > 0 ? (users[userIndex - 1].passIndex + 1) : 0;
            const int maxBarrierPass = users[userIndex].passIndex;
            int barrierPass = users[userIndex].passIndex;
            for(int i = maxBarrierPass - 1; i >= minBarrierPass; --i)
            {
                const bool hasBarrier = !sortedCompilePasses_[i]->beforeBufferTransitions.empty()
                                     || !sortedCompilePasses_[i]->beforeTextureTransitions.empty();
                if(hasBarrier)
                {
                    barrierPass = i;
                    break;
                }
            }

            sortedCompilePasses_[barrierPass]->beforeBufferTransitions.push_back(RHI::BufferTransitionBarrier
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
        const bool isSwapchainTex = TryCastResource<RenderGraph::SwapchainTexture>(tex);
        auto lastState = resources.indexToTexture[tex->GetResourceIndex()].texture
                                    ->GetState(subrsc.mipLevel, subrsc.arrayLayer);

        size_t userIndex = 0;
        while(userIndex < users.size())
        {
            size_t nextUserIndex = userIndex + 1;
            while(nextUserIndex < users.size() && DontNeedBarrier(users[userIndex].usage, users[nextUserIndex].usage))
            {
                ++nextUserIndex;
            }
            RTRC_SCOPE_EXIT{ userIndex = nextUserIndex; };

            TextureSubrscState currState;
            RTRC_SCOPE_EXIT{ lastState = currState; };
            currState.layout = users[userIndex].usage.layout;
            for(size_t i = userIndex; i < nextUserIndex; ++i)
            {
                currState.stages |= users[i].usage.stages;
                currState.accesses |= users[i].usage.accesses;
            }

            if(DontNeedBarrier(lastState, currState))
            {
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
                const bool hasBarrier = !sortedCompilePasses_[i]->beforeBufferTransitions.empty()
                                     || !sortedCompilePasses_[i]->beforeTextureTransitions.empty();
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
                sortedCompilePasses_[barrierPass]->beforeTextureTransitions.push_back(RHI::TextureTransitionBarrier
                {
                    .texture      = resources.indexToTexture[tex->GetResourceIndex()].texture->GetRHIObject(),
                    .subresources = RHI::TextureSubresources
                    {
                        .mipLevel = subrsc.mipLevel,
                        .levelCount = 1,
                        .arrayLayer = subrsc.arrayLayer,
                        .layerCount = 1
                    },
                    .beforeStages   = currState.stages,
                    .beforeAccesses = lastState.accesses,
                    .beforeLayout   = lastState.layout,
                    .afterStages    = currState.stages,
                    .afterAccesses  = currState.accesses,
                    .afterLayout    = currState.layout
                });
            }
            else
            {
                sortedCompilePasses_[barrierPass]->beforeTextureTransitions.push_back(RHI::TextureTransitionBarrier
                {
                    .texture      = resources.indexToTexture[tex->GetResourceIndex()].texture->GetRHIObject(),
                    .subresources = RHI::TextureSubresources
                    {
                        .mipLevel = subrsc.mipLevel,
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
    output.sections.clear();
    output.sections.reserve(sections_.size());
    for(auto &compileSection : sections_)
    {
        auto &section = output.sections.emplace_back();

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
            section.afterTextureBarriers.PushBack(compileSection->swapchainPresentBarrier.value());
        }

        section.signalFence = compileSection->signalFence;

        section.passes.reserve(compileSection->passes.size());
        for(int compilePassIndex : compileSection->passes)
        {
            auto &rawPass = sortedPasses_[compilePassIndex];
            auto &compilePass = sortedCompilePasses_[compilePassIndex];
            auto &pass = section.passes.emplace_back();

            pass.beforeBufferBarriers = std::move(compilePass->beforeBufferTransitions);
            for(auto &b : pass.beforeBufferBarriers)
            {
                CompilerDetail::RemoveUnnecessaryAccessMask(b.beforeAccesses, b.afterAccesses);
            }

            pass.beforeTextureBarriers = std::move(compilePass->beforeTextureTransitions);
            for(auto &b : pass.beforeTextureBarriers)
            {
                CompilerDetail::RemoveUnnecessaryAccessMask(b.beforeAccesses, b.afterAccesses);
            }

            if(rawPass->name_.empty())
            {
                pass.name = nullptr;
            }
            else
            {
                pass.name = &rawPass->name_;
            }
            if(sortedPasses_[compilePassIndex]->callback_)
            {
                pass.callback = &rawPass->callback_;
            }
            else
            {
                pass.callback = nullptr;
            }
        }
    }
}

RTRC_RG_END
