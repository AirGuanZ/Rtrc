#include <queue>
#include <ranges>

#include <Rtrc/RenderGraph/Compiler.h>
#include <Rtrc/RenderGraph/Graph.h>

RTRC_RG_BEGIN

Compiler::Compiler(RHI::DevicePtr device)
    : device_(std::move(device)), graph_(nullptr), semaphoreCount_(0)
{
    
}

void Compiler::Compile(const RenderGraph &graph, ExecutableGraph &result)
{
    graph_ = &graph;

    TopologySortPasses();
    CollectResourceUsers();

    GenerateSections();
    SortPassesInSections();
    GenerateSemaphores();
    result.semaphoreCount = semaphoreCount_;

    result.resources.indexToRHIBuffers.resize(graph_->buffers_.size());
    result.resources.indexToRHITextures.resize(graph_->textures_.size());
    result.resources.indexToFinalBufferStates.resize(graph_->buffers_.size());
    result.resources.indexToFinalTextureStates.resize(graph_->textures_.size());
    FillExternalResources(result.resources);
    AllocateInternalResources(result.resources);

    // TODO: generate barriers
    // TODO: move/merge/reduce barriers
    // TODO: generate final states for persistent resources
}

bool Compiler::DontNeedBarrier(const Pass::BufferUsage &a, const Pass::BufferUsage &b)
{
    return IsReadOnly(a.accesses) && IsReadOnly(b.accesses);
}

bool Compiler::DontNeedBarrier(const Pass::SubTexUsage &a, const Pass::SubTexUsage &b)
{
    return a.layout == b.layout && IsReadOnly(a.accesses) && IsReadOnly(b.accesses);
}

bool Compiler::IsInSection(int passIndex, const Section *section) const
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

    sortedPasses_.reserve(passes.size());
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
            for(auto [mipLevel, arrayLayer] : RHI::EnumerateSubTextures(
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
    //      next pass is in another queue
    //      signaling fence is not nil
    //      be the last user of swapchain texture

    const SubTexUsers *swapchainTexUsers = nullptr;
    if(graph_->swapchainTexture_)
    {
        if(auto it = subTexUsers_.find(SubTexKey{ graph_->swapchainTexture_, { 0, 0 } }); it != subTexUsers_.end())
        {
            swapchainTexUsers = &it->second;
        }
    }

    bool needNewSection = false;
    for(int passIndex = 0; passIndex < static_cast<int>(sortedPasses_.size()); ++passIndex)
    {
        const Pass *pass = sortedPasses_[passIndex];
        if(sections_.empty() || sections_.back()->queue != pass->queue_)
        {
            needNewSection = true;
        }
        if(needNewSection)
        {
            sections_.push_back(MakeBox<Section>());
            sections_.back()->queue = pass->queue_;
        }
        assert(!sections_.empty());
        sections_.back()->passes.push_back(passIndex);
        passToSection_.push_back(sections_.back().get());
        needNewSection = pass->signalFence_ || (swapchainTexUsers && passIndex == swapchainTexUsers->back().passIndex);
    }

    for(auto &section : sections_)
    {
        for(int passIndex : section->passes)
        {
            const Pass *pass = sortedPasses_[passIndex];
            for(const Pass *succ : pass->succs_)
            {
                const int succIndex = passToSortedIndex_.at(succ);
                Section *succSection = passToSection_[succIndex];
                if(succSection != section.get())
                {
                    succSection->directPrevs.insert(section.get());
                }
            }
        }
    }

    // sections are naturally topology-sorted

    for(auto &section : sections_)
    {
        for(auto directPrev : section->directPrevs)
        {
            for(auto indirectPrev : directPrev->directPrevs)
            {
                section->indirectPrevs.insert(indirectPrev);
            }
            for(auto indirectPrev : directPrev->indirectPrevs)
            {
                section->indirectPrevs.insert(indirectPrev);
            }
        }
    }

    // remove indirect section dependencies

    for(auto &section : sections_)
    {
        std::set<Section *, std::less<>> newDirectPrevs;
        for(auto directPrev : section->directPrevs)
        {
            if(!section->indirectPrevs.contains(directPrev))
            {
                newDirectPrevs.insert(directPrev);
            }
        }
        section->directPrevs = std::move(newDirectPrevs);
    }

    for(auto &section : sections_)
    {
        for(Section *prev : section->directPrevs)
        {
            prev->directSuccs.insert(section.get());
        }
    }
}

void Compiler::SortPassesInSections()
{
    // TODO
}

void Compiler::GenerateSemaphores()
{
    // fill signalSemaphoreIndex

    auto FillSignalSemaphoreForUsers = [&](auto &users)
    {
        for(int i = 0; i + 1 < static_cast<int>(users.size()); ++i)
        {
            Section *thisSection = passToSection_.at(users[i].passIndex);
            Section *nextSection = passToSection_.at(users[i + 1].passIndex);
            if(thisSection->queue != nextSection->queue)
            {
                if(thisSection->signalSemaphoreIndex < 0)
                {
                    thisSection->signalSemaphoreIndex = semaphoreCount_++;
                }
                thisSection->signalStages |= users[i].usage.stages;
                auto ShouldContinue = [&](int j)
                {
                    return j >= 0 && IsInSection(users[j].passIndex, thisSection)
                                  && DontNeedBarrier(users[j].usage, users[i].usage);
                };
                for(int j = i - 1; ShouldContinue(j); --j)
                {
                    thisSection->signalStages |= users[j].usage.stages;
                }
            }
        }
    };

    for(auto &users : std::ranges::views::values(bufferUsers_))
    {
        FillSignalSemaphoreForUsers(users);
    }

    for(auto &users : std::ranges::views::values(subTexUsers_))
    {
        FillSignalSemaphoreForUsers(users);
    }

    // fill waitSemaphoreIndex

    auto IsReachable = [](const Section *from, const Section *to)
    {
        return from == to || to->directPrevs.contains(from) || to->indirectPrevs.contains(from);
    };

    auto FillWaitSemaphoreForUsers = [&](auto &users)
    {
        for(int i = 1; i < static_cast<int>(users.size()); ++i)
        {
            Section *lastSection = passToSection_.at(users[i - 1].passIndex);
            Section *thisSection = passToSection_.at(users[i].passIndex);
            if(thisSection->queue != lastSection->queue)
            {
                assert(lastSection->signalSemaphoreIndex >= 0);
                bool hasValidPrev = false;
                for(Section *directPrev : thisSection->directPrevs)
                {
                    if(IsReachable(lastSection, directPrev))
                    {
                        hasValidPrev = true;
                        const int waitSemaphoreIndex = lastSection->signalSemaphoreIndex;
                        RHI::PipelineStageFlag &stages = thisSection->waitSemaphoreIndexToStages[waitSemaphoreIndex];
                        stages |= users[i].usage.stages;
                        auto ShouldContinue = [&](int j)
                        {
                            return j < static_cast<int>(users.size()) && IsInSection(users[j].passIndex, thisSection)
                                                                      && DontNeedBarrier(users[j].usage, users[i].usage);
                        };
                        for(int j = i + 1; ShouldContinue(j); ++j)
                        {
                            stages |= users[j].usage.stages;
                        }
                        break;
                    }
                }
                if(!hasValidPrev)
                {
                    throw Exception("resource usages across sections don't have a proper dependency");
                }
            }
        }
    };

    for(auto &users : std::ranges::views::values(bufferUsers_))
    {
        FillWaitSemaphoreForUsers(users);
    }

    for(auto &users : std::ranges::views::values(subTexUsers_))
    {
        FillWaitSemaphoreForUsers(users);
    }

    // swapchain texture

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

        Section *firstSection = passToSection_.at(users[0].passIndex);
        firstSection->waitBackbufferSemaphore = true;
        firstSection->waitBackbufferSemaphoreStages |= users[0].usage.stages;
        auto ShouldContinue = [&](int j)
        {
            return j < static_cast<int>(users.size()) &&
                   IsInSection(users[j].passIndex, firstSection) &&
                   DontNeedBarrier(users[j].usage, users[0].usage);
        };
        for(int j = 1; ShouldContinue(j); ++j)
        {
            firstSection->waitBackbufferSemaphoreStages |= users[j].usage.stages;
        }

        Section *lastSection = passToSection_.at(users.back().passIndex);
        lastSection->signalBackbufferSemaphore = true;
        lastSection->signalBackbufferSemaphoreStages |= users.back().usage.stages;
        auto ShouldContinue2 = [&](int j)
        {
            return j >= 0 && IsInSection(users[j].passIndex, lastSection)
                          && DontNeedBarrier(users[j].usage, users.back().usage);
        };
        for(int j = static_cast<int>(users.size()) - 1; ShouldContinue2(j); --j)
        {
            lastSection->signalBackbufferSemaphoreStages |= users[j].usage.stages;
        }
    }
}

void Compiler::FillExternalResources(ExecutableResources &output)
{
    for(auto &buffer : graph_->buffers_)
    {
        if(auto ext = dynamic_cast<RenderGraph::ExternalBufferResource*>(buffer.get()))
        {
            const int index = ext->GetResourceIndex();
            output.indexToRHIBuffers[index] = ext->buffer;
            output.indexToFinalBufferStates[index] = bufferUsers_.at(ext).back().usage;
        }
    }

    for(auto &texture : graph_->textures_)
    {
        if(auto ext = dynamic_cast<RenderGraph::ExternalTextureResource*>(texture.get()))
        {
            const int index = ext->GetResourceIndex();
            output.indexToRHITextures[index] = ext->texture;
            output.indexToFinalTextureStates[index] =
                RHI::TextureSubresourceMap<RHI::StatefulTexture::State>(ext->GetMipLevels(), ext->GetArraySize());
            for(auto subrsc : RHI::EnumerateSubTextures(ext->GetMipLevels(), ext->GetArraySize()))
            {
                auto &finalUsage = subTexUsers_.at({ ext, subrsc }).back().usage;
                output.indexToFinalTextureStates[index](subrsc.mipLevel, subrsc.arrayLayer) = finalUsage;
            }
        }
    }
}

void Compiler::AllocateInternalResources(ExecutableResources &output)
{
    // TODO
}

RTRC_RG_END
