#include <Rtrc/Core/Enumerate.h>
#include <Rtrc/RHI/DirectX12/Context/Device.h>
#include <Rtrc/RHI/DirectX12/Resource/Buffer.h>
#include <Rtrc/RHI/DirectX12/Resource/Texture.h>
#include <Rtrc/RHI/DirectX12/Resource/TransientResourcePool/FreeMemorySegmentSet.h>
#include <Rtrc/RHI/DirectX12/Resource/TransientResourcePool/TransientResourcePool.h>
#include <Rtrc/RHI/Helper/MemorySegmentUsageTracker.h>

RTRC_RHI_D3D12_BEGIN

DirectX12TransientResourcePool::DirectX12TransientResourcePool(
    DirectX12Device *device, const TransientResourcePoolDesc &desc)
    : device_(device), memoryBlocks_(device, desc.chunkSizeHint)
{
    resourceHeapTier_ = device->_internalGetFeatureOptions().ResourceHeapTier;
}

void DirectX12TransientResourcePool::Recycle()
{
    auto [first, last] = std::ranges::remove_if(resources_, [&](const ResourceSlot &slot)
    {
        return slot.queueSync->IsSynchronized();
    });
    resources_.erase(first, last);
    memoryBlocks_.FreeUnusedMemoryBlocks();
}

RC<QueueSyncQuery> DirectX12TransientResourcePool::Allocate(
    QueueOPtr                                  queue,
    MutSpan<TransientResourceDeclaration>      resources,
    std::vector<AliasedTransientResourcePair> &aliasRelation)
{
    using namespace TransientResourcePoolDetail;

    auto queueSync = MakeRC<QueueSyncQuery>();
    auto d3dDevice = device_->_internalGetNativeDevice();
    memoryBlocks_.RecycleAvailableMemoryBlocks();

    // Translate resource requests to allocate/release events

    struct AllocateEvent
    {
        int           resource;
        Category      category;
        HeapAlignment heapAlignment;
        size_t        size;
        size_t        alignment;
        int           sortKey;
    };

    struct ReleaseEvent
    {
        int resource;
        int sortKey;
    };

    using Event = Variant<AllocateEvent, ReleaseEvent>;

    std::vector<Event> events;
    events.reserve(resources.size() << 1);

    for(auto &&[resourceIndex, resource] : Enumerate(resources))
    {
        resource.Match(
            [&](const TransientBufferDeclaration &bufferDecl)
            {
                const auto d3d12BufferDesc = TranslateBufferDesc(bufferDecl.desc);
                const auto allocInfo = d3dDevice->GetResourceAllocationInfo(0, 1, &d3d12BufferDesc);

                AllocateEvent allocate;
                allocate.resource = static_cast<int>(resourceIndex);
                allocate.category = resourceHeapTier_ == D3D12_RESOURCE_HEAP_TIER_2 ? Category::General
                                                                                    : Category::Buffer;
                allocate.heapAlignment = HeapAlignment::Regular;
                allocate.size          = allocInfo.SizeInBytes;
                allocate.alignment     = allocInfo.Alignment;
                allocate.sortKey       = 2 * bufferDecl.beginPass;

                ReleaseEvent release;
                release.resource = allocate.resource;
                release.sortKey  = 2 * bufferDecl.endPass + 1;

                events.emplace_back(allocate);
                events.emplace_back(release);
            },
            [&](const TransientTextureDeclaration &textureDecl)
            {
                const auto d3d12TexDesc = TranslateTextureDesc(textureDecl.desc);
                const auto allocInfo = d3dDevice->GetResourceAllocationInfo(0, 1, &d3d12TexDesc);

                AllocateEvent allocate;
                allocate.resource      = static_cast<int>(resourceIndex);
                allocate.category      = GetTextureCategory(textureDecl.desc);
                allocate.heapAlignment = GetTextureHeapAlignment(textureDecl.desc);
                allocate.size          = allocInfo.SizeInBytes;
                allocate.alignment     = allocInfo.Alignment;
                allocate.sortKey       = 2 * textureDecl.beginPass;

                ReleaseEvent release;
                release.resource = allocate.resource;
                release.sortKey  = 2 * textureDecl.endPass + 1;

                events.emplace_back(allocate);
                events.emplace_back(release);
            });
    }

    std::ranges::sort(events, [](const Event &lhs, const Event &rhs)
    {
        const int lhsKey = lhs.Match([](auto &e) { return e.sortKey; });
        const int rhsKey = rhs.Match([](auto &e) { return e.sortKey; });
        return lhsKey < rhsKey;
    });

    // Execute all resource events

    MemorySegmentUsageTracker memorySegmentUsageTracker;
    FreeMemorySegmentSet freeMemorySegmentSet[4/*category*/][2/*alignment*/];

    struct ResourceAllocation
    {
        Category             category      = Category::General;
        HeapAlignment            heapAlignment = HeapAlignment::Regular;
        D3D12MA::Allocation *allocation    = nullptr;
        size_t               offset        = 0;
        size_t               size          = 0;
    };
    std::vector<ResourceAllocation> resourceAllocations(resources.size());

    auto HandleAllocateEvent = [&](const AllocateEvent &allocate)
    {
        auto &output = resourceAllocations[allocate.resource];
        output.category = allocate.category;

        RTRC_SCOPE_EXIT
        {
            const std::vector<int> dependencies = memorySegmentUsageTracker.AddNewUser(
                output.allocation, allocate.resource, output.offset, output.size);

            for(const int d : dependencies)
            {
                aliasRelation.push_back({ d, allocate.resource });
            }
        };

        const int categoryIndex = std::to_underlying(allocate.category);
        const int heapAlignmentIndex = std::to_underlying(allocate.heapAlignment);
        auto &freeBlocks = freeMemorySegmentSet[categoryIndex][heapAlignmentIndex];
        auto mb = freeBlocks.Allocate(allocate.size, allocate.alignment);
        if(mb)
        {
            output.heapAlignment = allocate.heapAlignment;
            output.allocation    = mb->allocation;
            output.offset        = mb->offset;
            output.size          = mb->size;
            return;
        }

        if(allocate.heapAlignment == HeapAlignment::Regular)
        {
            const int fallbackHeapAlignmentIndex = std::to_underlying(HeapAlignment::MSAA);
            auto &fallbackFreeBlocks = freeMemorySegmentSet[categoryIndex][fallbackHeapAlignmentIndex];
            mb = fallbackFreeBlocks.Allocate(allocate.size, allocate.alignment);
            if(mb)
            {
                output.heapAlignment = HeapAlignment::MSAA;
                output.allocation    = mb->allocation;
                output.offset        = mb->offset;
                output.size          = mb->size;
                return;
            }
        }

        auto memoryBlock = memoryBlocks_.GetMemoryBlock(
            queue, queueSync, allocate.category, allocate.heapAlignment, allocate.size);
        assert(memoryBlock.size >= allocate.size);

        output.heapAlignment = memoryBlock.alignment;
        output.allocation    = memoryBlock.allocation.Get();
        output.offset        = 0;
        output.size          = allocate.size;

        if(memoryBlock.size > allocate.size)
        {
            freeMemorySegmentSet[categoryIndex][std::to_underlying(memoryBlock.alignment)].Free(
            {
                .allocation = output.allocation,
                .offset     = allocate.size,
                .size       = memoryBlock.size - allocate.size
            });
        }
    };

    auto HandleReleaseEvent = [&](const ReleaseEvent &release)
    {
        const ResourceAllocation &allocation = resourceAllocations[release.resource];
        assert(allocation.allocation);
        const int categoryIndex = std::to_underlying(allocation.category);
        const int alignmentIndex = std::to_underlying(allocation.heapAlignment);
        freeMemorySegmentSet[categoryIndex][alignmentIndex].Free(
        {
            .allocation = allocation.allocation,
            .offset     = allocation.offset,
            .size       = allocation.size
        });
    };

    for(const Event &resourceEvent : events)
    {
        resourceEvent.Match(
            [&](const AllocateEvent &allocate) { HandleAllocateEvent(allocate); },
            [&](const ReleaseEvent &release) { HandleReleaseEvent(release); });
    }

    // Create placement resources

    for(size_t i = 0; i < resources.size(); ++i)
    {
        const auto &allocation = resourceAllocations[i];
        const size_t offsetInHeap = allocation.allocation->GetOffset() + allocation.offset;
        auto memoryHeap = allocation.allocation->GetHeap();

        resources[i].Match(
            [&](TransientBufferDeclaration &bufferDecl)
            {
                const auto d3dDesc = TranslateBufferDesc(bufferDecl.desc);
                ComPtr<ID3D12Resource> d3d12Resource;
                RTRC_D3D12_FAIL_MSG(
                    d3dDevice->CreatePlacedResource(
                        memoryHeap, offsetInHeap, &d3dDesc, D3D12_RESOURCE_STATE_COMMON,
                        nullptr, IID_PPV_ARGS(d3d12Resource.GetAddressOf())),
                    "DirectX12TransientResourcePool: fail to create placement buffer resource");

                bufferDecl.buffer = MakeRPtr<DirectX12Buffer>(
                    bufferDecl.desc, device_, std::move(d3d12Resource),
                    DirectX12MemoryAllocation{ nullptr });
                resources_.push_back({ bufferDecl.buffer, queueSync });
            },
            [&](TransientTextureDeclaration &textureDecl)
            {
                const auto d3dDesc = TranslateTextureDesc(textureDecl.desc);
                D3D12_CLEAR_VALUE clearValue = {}, *pClearValue = nullptr;
                if(!textureDecl.desc.clearValue.Is<std::monostate>())
                {
                    clearValue.Format = d3dDesc.Format;
                    pClearValue = &clearValue;
                    if(auto c = textureDecl.desc.clearValue.AsIf<ColorClearValue>())
                    {
                        clearValue.Color[0] = c->r;
                        clearValue.Color[1] = c->g;
                        clearValue.Color[2] = c->b;
                        clearValue.Color[3] = c->a;
                    }
                    else
                    {
                        auto d = textureDecl.desc.clearValue.AsIf<DepthStencilClearValue>();
                        assert(d);
                        clearValue.DepthStencil.Depth = d->depth;
                        clearValue.DepthStencil.Stencil = d->stencil;
                    }
                }

                ComPtr<ID3D12Resource> d3d12Resource;
                RTRC_D3D12_FAIL_MSG(
                    d3dDevice->CreatePlacedResource(
                        memoryHeap, offsetInHeap, &d3dDesc, D3D12_RESOURCE_STATE_COMMON,
                        pClearValue, IID_PPV_ARGS(d3d12Resource.GetAddressOf())),
                    "DirectX12TransientResourcePool: fail to create placement texture resource");

                textureDecl.texture = MakeRPtr<DirectX12Texture>(
                    textureDecl.desc, device_, std::move(d3d12Resource), DirectX12MemoryAllocation{ nullptr });
                resources_.push_back({ textureDecl.texture, queueSync });
            });
    }

    return queueSync;
}

DirectX12TransientResourcePool::Category DirectX12TransientResourcePool::GetTextureCategory(
    const TextureDesc &desc) const
{
    if(resourceHeapTier_ == D3D12_RESOURCE_HEAP_TIER_2)
    {
        return Category::General;
    }
    const bool isRTDS =
        desc.usage & (TextureUsage::RenderTarget | TextureUsage::ClearColor | TextureUsage::DepthStencil);
    return isRTDS ? Category::RTDSTexture : Category::NonRTDSTexture;
}

DirectX12TransientResourcePool::HeapAlignment DirectX12TransientResourcePool::GetTextureHeapAlignment(
    const TextureDesc &desc) const
{
    return desc.sampleCount != 1 ? HeapAlignment::MSAA : HeapAlignment::Regular;
}

RTRC_RHI_D3D12_END
