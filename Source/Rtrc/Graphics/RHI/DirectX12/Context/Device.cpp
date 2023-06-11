#include <Rtrc/Graphics/RHI/DirectX12/Context/Device.h>
#include <Rtrc/Graphics/RHI/DirectX12/Context/Swapchain.h>
#include <Rtrc/Graphics/RHI/DirectX12/Pipeline/BindingLayout.h>
#include <Rtrc/Graphics/RHI/DirectX12/Pipeline/BindingGroup.h>
#include <Rtrc/Graphics/RHI/DirectX12/Pipeline/ComputePipeline.h>
#include <Rtrc/Graphics/RHI/DirectX12/Pipeline/GraphicsPipeline.h>
#include <Rtrc/Graphics/RHI/DirectX12/Pipeline/Shader.h>
#include <Rtrc/Graphics/RHI/DirectX12/Queue/CommandPool.h>
#include <Rtrc/Graphics/RHI/DirectX12/Queue/Fence.h>
#include <Rtrc/Graphics/RHI/DirectX12/Queue/Queue.h>
#include <Rtrc/Graphics/RHI/DirectX12/Queue/Semaphore.h>
#include <Rtrc/Graphics/RHI/DirectX12/Resource/Buffer.h>
#include <Rtrc/Graphics/RHI/DirectX12/Resource/Sampler.h>
#include <Rtrc/Graphics/RHI/DirectX12/Resource/Texture.h>
#include <Rtrc/Utility/Enumerate.h>
#include <Rtrc/Utility/Unreachable.h>

RTRC_RHI_D3D12_BEGIN

namespace DirectX12DeviceDetail
{

    D3D12_SHADER_BYTECODE GetByteCode(const Ptr<RawShader> &shader)
    {
        if(!shader)
        {
            return {};
        }
        return static_cast<DirectX12RawShader *>(shader.Get())->_internalGetShaderByteCode();
    }

} // namespace DirectX12DeviceDetail

DirectX12Device::DirectX12Device(
    ComPtr<ID3D12Device>  device,
    const Queues         &queues,
    ComPtr<IDXGIFactory4> factory,
    ComPtr<IDXGIAdapter>  adapter)
    : device_(std::move(device)), factory_(std::move(factory)), adapter_(std::move(adapter))
{
    const D3D12MA::ALLOCATOR_DESC allocatorDesc =
    {
        .Flags                = D3D12MA::ALLOCATOR_FLAG_NONE,
        .pDevice              = device_.Get(),
        .PreferredBlockSize   = 0,
        .pAllocationCallbacks = RTRC_D3D12_ALLOC,
        .pAdapter             = adapter_.Get()
    };
    RTRC_D3D12_FAIL_MSG(
        CreateAllocator(&allocatorDesc, allocator_.GetAddressOf()),
        "Fail to initialize directx12 memory allocator");

    constexpr int CPU_DESC_HEAP_CHUNK_SIZE = 2048;
    cpuDescHeap_CbvSrvUav_ = MakeBox<CPUDescriptorHandleManager>(
        this, CPU_DESC_HEAP_CHUNK_SIZE, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    cpuDescHeap_Sampler_ = MakeBox<CPUDescriptorHandleManager>(
        this, CPU_DESC_HEAP_CHUNK_SIZE, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    cpuDescHeap_Rtv_ = MakeBox<CPUDescriptorHandleManager>(
        this, CPU_DESC_HEAP_CHUNK_SIZE, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    cpuDescHeap_Dsv_ = MakeBox<CPUDescriptorHandleManager>(
        this, CPU_DESC_HEAP_CHUNK_SIZE, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    gpuDescHeap_CbvSrvUav_ = MakeBox<DescriptorHeap>(
        this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, true, 65536);
    gpuDescHeap_Sampler_ = MakeBox<DescriptorHeap>(
        this, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, true, 1024); // d3d12 spec says max limit is 2048

    if(queues.graphicsQueue)
    {
        graphicsQueue_ = MakePtr<DirectX12Queue>(this, queues.graphicsQueue, QueueType::Graphics);
    }
    if(queues.computeQueue)
    {
        computeQueue_ = MakePtr<DirectX12Queue>(this, queues.computeQueue, QueueType::Compute);
    }
    if(queues.copyQueue)
    {
        copyQueue_ = MakePtr<DirectX12Queue>(this, queues.copyQueue, QueueType::Transfer);
    }
    // presentQueue_  = MakePtr<DirectX12Queue>(this, queues.presentQueue, QueueType::Graphics);

    const D3D12_INDIRECT_ARGUMENT_DESC indirectDispatchArgDesc =
    {
        .Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH,
    };
    const D3D12_COMMAND_SIGNATURE_DESC indirectDispatchCommandSignatureDesc =
    {
        .ByteStride       = sizeof(D3D12_DISPATCH_ARGUMENTS),
        .NumArgumentDescs = 1,
        .pArgumentDescs   = &indirectDispatchArgDesc
    };
    RTRC_D3D12_FAIL_MSG(
        device_->CreateCommandSignature(
            &indirectDispatchCommandSignatureDesc, nullptr,
            IID_PPV_ARGS(indirectDispatchCommandSignature_.GetAddressOf())),
        "Fail to create directx12 command signature for indirect dispatch");
}

DirectX12Device::~DirectX12Device()
{
    WaitIdle();
}

Ptr<Queue> DirectX12Device::GetQueue(QueueType type)
{
    switch (type)
    {
    case QueueType::Graphics:
        return graphicsQueue_;
    case QueueType::Compute:
        return computeQueue_;
    case QueueType::Transfer:
        return copyQueue_;
    }
    Unreachable();
}

Ptr<CommandPool> DirectX12Device::CreateCommandPool(const Ptr<Queue> &queue)
{
    ComPtr<ID3D12CommandAllocator> commandAllocator;
    RTRC_D3D12_FAIL_MSG(
        device_->CreateCommandAllocator(
            TranslateCommandListType(queue->GetType()),
            IID_PPV_ARGS(commandAllocator.GetAddressOf())),
        "Fail to create directx12 command allocator");
    return MakePtr<DirectX12CommandPool>(this, queue->GetType(), std::move(commandAllocator));
}

Ptr<Swapchain> DirectX12Device::CreateSwapchain(const SwapchainDesc &desc, Window &window)
{
    const HWND windowHandle = reinterpret_cast<HWND>(window.GetWin32WindowHandle());
    DXGI_SWAP_CHAIN_DESC swapChainDesc =
    {
        .BufferDesc = DXGI_MODE_DESC
        {
            .Width            = static_cast<UINT>(window.GetFramebufferSize().x),
            .Height           = static_cast<UINT>(window.GetFramebufferSize().y),
            .RefreshRate      = DXGI_RATIONAL{ 60, 1 },
            .Format           = TranslateFormat(desc.format),
            .ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
            .Scaling          = DXGI_MODE_SCALING_UNSPECIFIED
        },
        .SampleDesc   = DXGI_SAMPLE_DESC{ 1, 0 },
        .BufferUsage  = DXGI_USAGE_RENDER_TARGET_OUTPUT | (desc.allowUav ? DXGI_USAGE_UNORDERED_ACCESS : 0),
        .BufferCount  = static_cast<UINT>(desc.imageCount),
        .OutputWindow = windowHandle,
        .Windowed     = true,
        .SwapEffect   = DXGI_SWAP_EFFECT_FLIP_DISCARD,
        .Flags        = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH
    };

    ComPtr<IDXGISwapChain> swapChain;
    RTRC_D3D12_FAIL_MSG(
        factory_->CreateSwapChain(graphicsQueue_->_internalGetCommandQueue(), &swapChainDesc, swapChain.GetAddressOf()),
        "Fail to create dxgi swapchain");

    ComPtr<IDXGISwapChain3> swapChain3;
    RTRC_D3D12_FAIL_MSG(
        swapChain->QueryInterface(IID_PPV_ARGS(swapChain3.GetAddressOf())),
        "Fail to convert dxgi swapchain to dxgi swapchain 3");

    const TextureDesc imageDesc =
    {
        .dim           = TextureDimension::Tex2D,
        .format        = desc.format,
        .width         = static_cast<uint32_t>(window.GetFramebufferSize().x),
        .height        = static_cast<uint32_t>(window.GetFramebufferSize().y),
        .arraySize     = 1,
        .mipLevels     = 1,
        .sampleCount   = 1,
        .usage         = TextureUsage::RenderTarget |
                         (desc.allowUav ? TextureUsage::UnorderAccess : TextureUsage::None),
        .initialLayout = TextureLayout::Undefined,
        .concurrentAccessMode = QueueConcurrentAccessMode::Exclusive
    };
    return MakePtr<DirectX12Swapchain>(this, imageDesc, std::move(swapChain3), desc.vsync ? 1 : 0);
}

Ptr<Fence> DirectX12Device::CreateFence(bool signaled)
{
    ComPtr<ID3D12Fence> fence;
    RTRC_D3D12_FAIL_MSG(
        device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence.GetAddressOf())),
        "DirectX12Device::CreateFence: fail to create directx12 fence");
    return MakePtr<DirectX12Fence>(std::move(fence), signaled);
}

Ptr<Semaphore> DirectX12Device::CreateTimelineSemaphore(uint64_t initialValue)
{
    ComPtr<ID3D12Fence> fence;
    RTRC_D3D12_FAIL_MSG(
        device_->CreateFence(initialValue, D3D12_FENCE_FLAG_SHARED, IID_PPV_ARGS(fence.GetAddressOf())),
        "DirectX12Device::CreateTimelineSemaphore: fail to create directx12 fence");
    return MakePtr<DirectX12Semaphore>(std::move(fence));
}

Ptr<RawShader> DirectX12Device::CreateShader(const void *data, size_t size, std::vector<RawShaderEntry> entries)
{
    std::vector<std::byte> byteCode(size);
    std::memcpy(byteCode.data(), data, size);
    return MakePtr<DirectX12RawShader>(std::move(byteCode), std::move(entries));
}

Ptr<GraphicsPipeline> DirectX12Device::CreateGraphicsPipeline(const GraphicsPipelineDesc &desc)
{
    auto d3dBindingLayout = static_cast<DirectX12BindingLayout *>(desc.bindingLayout.Get());
    auto rootSignature = d3dBindingLayout->_internalGetRootSignature(!desc.vertexAttributs.empty());

    D3D12_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0] = D3D12_RENDER_TARGET_BLEND_DESC
    {
        .BlendEnable           = desc.enableBlending,
        .LogicOpEnable         = false,
        .SrcBlend              = TranslateBlendFactor(desc.blendingSrcColorFactor),
        .DestBlend             = TranslateBlendFactor(desc.blendingDstColorFactor),
        .BlendOp               = TranslateBlendOp(desc.blendingColorOp),
        .SrcBlendAlpha         = TranslateBlendFactor(desc.blendingSrcAlphaFactor),
        .DestBlendAlpha        = TranslateBlendFactor(desc.blendingDstAlphaFactor),
        .BlendOpAlpha          = TranslateBlendOp(desc.blendingAlphaOp),
        .LogicOp               = {},
        .RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL
    };

    std::vector<D3D12_INPUT_ELEMENT_DESC> inputElements;
    for(auto &attrib : desc.vertexAttributs)
    {
        auto &elem = inputElements.emplace_back();
        elem.SemanticName         = attrib.semanticName.c_str();
        elem.SemanticIndex        = attrib.semanticIndex;
        elem.Format               = TranslateVertexAttributeType(attrib.type);
        elem.InputSlot            = attrib.inputBufferIndex;
        elem.AlignedByteOffset    = attrib.byteOffsetInBuffer;
        if(desc.vertexBuffers[attrib.inputBufferIndex].perInstance)
        {
            elem.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
            elem.InstanceDataStepRate = 1;
        }
        else
        {
            elem.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
            elem.InstanceDataStepRate = 0;
        }
    }

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineDesc =
    {
        .pRootSignature  = rootSignature.Get(),
        .VS              = DirectX12DeviceDetail::GetByteCode(desc.vertexShader),
        .PS              = DirectX12DeviceDetail::GetByteCode(desc.fragmentShader),
        .DS              = {},
        .HS              = {},
        .GS              = {},
        .StreamOutput    = {},
        .BlendState      = blendDesc,
        .SampleMask      = 0xffffffff,
        .RasterizerState = D3D12_RASTERIZER_DESC
        {
            .FillMode              = TranslateFillMode(desc.fillMode),
            .CullMode              = TranslateCullMode(desc.cullMode),
            .FrontCounterClockwise = desc.frontFaceMode == FrontFaceMode::CounterClockwise ? true : false,
            .DepthBias             = static_cast<INT>(desc.depthBiasConstFactor),
            .DepthBiasClamp        = desc.depthBiasClampValue,
            .SlopeScaledDepthBias  = desc.depthBiasSlopeFactor,
            .DepthClipEnable       = false,
            .MultisampleEnable     = false,
            .AntialiasedLineEnable = false,
            .ForcedSampleCount     = 0,
            .ConservativeRaster    = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
        },
        .DepthStencilState = D3D12_DEPTH_STENCIL_DESC
        {
            .DepthEnable      = desc.enableDepthTest || desc.enableDepthWrite,
            .DepthWriteMask   = desc.enableDepthWrite ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO,
            .DepthFunc        = TranslateCompareOp(desc.depthCompareOp),
            .StencilEnable    = desc.enableStencilTest,
            .StencilReadMask  = desc.stencilReadMask,
            .StencilWriteMask = desc.stencilWriteMask,
            .FrontFace = D3D12_DEPTH_STENCILOP_DESC
            {
                .StencilFailOp      = TranslateStencilOp(desc.frontStencilOp.failOp),
                .StencilDepthFailOp = TranslateStencilOp(desc.frontStencilOp.depthFailOp),
                .StencilPassOp      = TranslateStencilOp(desc.frontStencilOp.passOp),
                .StencilFunc        = TranslateCompareOp(desc.frontStencilOp.compareOp)
            },
            .BackFace = D3D12_DEPTH_STENCILOP_DESC
            {
                .StencilFailOp      = TranslateStencilOp(desc.backStencilOp.failOp),
                .StencilDepthFailOp = TranslateStencilOp(desc.backStencilOp.depthFailOp),
                .StencilPassOp      = TranslateStencilOp(desc.backStencilOp.passOp),
                .StencilFunc        = TranslateCompareOp(desc.backStencilOp.compareOp)
            }
        },
        .InputLayout = D3D12_INPUT_LAYOUT_DESC
        {
            .pInputElementDescs = inputElements.data(),
            .NumElements        = static_cast<UINT>(inputElements.size())
        },
        .IBStripCutValue       = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,
        .PrimitiveTopologyType = TranslatePrimitiveTopologyType(desc.primitiveTopology),
        .NumRenderTargets      = static_cast<UINT>(desc.colorAttachments.GetSize()),
        .DSVFormat             = TranslateFormat(desc.depthStencilFormat),
        .SampleDesc            = DXGI_SAMPLE_DESC{ static_cast<UINT>(desc.multisampleCount), 0 }
    };
    for(uint32_t i = 0; i < desc.colorAttachments.GetSize(); ++i)
    {
        pipelineDesc.RTVFormats[i] = TranslateFormat(desc.colorAttachments[i]);
    }

    ComPtr<ID3D12PipelineState> pipelineState;
    RTRC_D3D12_FAIL_MSG(
        device_->CreateGraphicsPipelineState(&pipelineDesc, IID_PPV_ARGS(pipelineState.GetAddressOf())),
        "Fail to create directx12 graphics pipeline state");

    std::optional<Span<Viewport>> staticViewports;
    if(desc.viewports.Is<std::vector<Viewport>>())
    {
        staticViewports = desc.viewports.As<std::vector<Viewport>>();
    }

    std::optional<Span<Scissor>> staticScissors;
    if(desc.scissors.Is<std::vector<Scissor>>())
    {
        staticScissors = desc.scissors.As<std::vector<Scissor>>();
    }

    std::vector<size_t> vertexStrides(desc.vertexBuffers.size());
    for(size_t i = 0; i < desc.vertexBuffers.size(); ++i)
    {
        vertexStrides[i] = desc.vertexBuffers[i].elementSize;
    }

    return MakePtr<DirectX12GraphicsPipeline>(
        desc.bindingLayout, std::move(rootSignature), std::move(pipelineState),
        staticViewports, staticScissors, vertexStrides, TranslatePrimitiveTopology(desc.primitiveTopology));
}

Ptr<ComputePipeline> DirectX12Device::CreateComputePipeline(const ComputePipelineDesc &desc)
{
    auto d3dBindingLayout = static_cast<DirectX12BindingLayout *>(desc.bindingLayout.Get());
    auto rootSignature = d3dBindingLayout->_internalGetRootSignature(false);

    const D3D12_COMPUTE_PIPELINE_STATE_DESC pipelineDesc =
    {
        .pRootSignature = rootSignature.Get(),
        .CS             = DirectX12DeviceDetail::GetByteCode(desc.computeShader),
    };

    ComPtr<ID3D12PipelineState> pipelineState;
    RTRC_D3D12_FAIL_MSG(
        device_->CreateComputePipelineState(&pipelineDesc, IID_PPV_ARGS(pipelineState.GetAddressOf())),
        "Fail to create directx12 compute pipeline state");

    return MakePtr<DirectX12ComputePipeline>(desc.bindingLayout, std::move(rootSignature), std::move(pipelineState));
}

Ptr<RayTracingPipeline> DirectX12Device::CreateRayTracingPipeline(const RayTracingPipelineDesc &desc)
{
    throw Exception("Not implemented");
}

Ptr<RayTracingLibrary> DirectX12Device::CreateRayTracingLibrary(const RayTracingLibraryDesc &desc)
{
    throw Exception("Not implemented");
}

Ptr<BindingGroupLayout> DirectX12Device::CreateBindingGroupLayout(const BindingGroupLayoutDesc &desc)
{
    return MakePtr<DirectX12BindingGroupLayout>(desc);
}

Ptr<BindingGroup> DirectX12Device::CreateBindingGroup(
    const Ptr<BindingGroupLayout> &bindingGroupLayout, uint32_t variableArraySize)
{
    const auto &layoutDesc = bindingGroupLayout->GetDesc();
    assert(layoutDesc.variableArraySize == (variableArraySize > 0));

    const auto d3dBindingGroupLayout = static_cast<DirectX12BindingGroupLayout *>(bindingGroupLayout.Get());
    const auto d3dLayoutDesc = d3dBindingGroupLayout->_internalGetD3D12Desc();

    size_t variableSizedTableIndex = (std::numeric_limits<size_t>::max)();
    if(layoutDesc.variableArraySize)
    {
        variableSizedTableIndex = d3dLayoutDesc.bindings.back().tableIndex;
    }

    std::vector<DescriptorTable> tables(d3dLayoutDesc.tables.size());
    RTRC_SCOPE_FAIL
    {
        for(auto &&[i, t] : Enumerate(tables))
        {
            if(t.count)
            {
                if(d3dLayoutDesc.tables[i].type == Helper::D3D12DescTable::SrvUavCbv)
                {
                    _internalFreeGPUDescriptorHandles_CbvSrvUav(t);
                }
                else
                {
                    _internalFreeGPUDescriptorHandles_Sampler(t);
                }
            }
        }
    };

    for(size_t i = 0; i < d3dLayoutDesc.tables.size(); ++i)
    {
        auto &tableDesc = d3dLayoutDesc.tables[i];

        uint32_t count;
        if(tableDesc.type == Helper::D3D12DescTable::SrvUavCbv)
        {
            count = tableDesc.srvCount + tableDesc.uavCount + tableDesc.cbvCount;
        }
        else
        {
            count = tableDesc.samplerCount;
        }
        
        if(i == variableSizedTableIndex)
        {
            assert(variableArraySize <= layoutDesc.bindings.back().arraySize.value());
            count = d3dLayoutDesc.bindings.back().offsetInTable + variableArraySize;
        }

        if(tableDesc.type == Helper::D3D12DescTable::SrvUavCbv)
        {
            if(!_internalAllocateGPUDescriptorHandles_CbvSrvUav(count, tables[i]))
            {
                throw Exception("Fail to allocate gpu srv/uav/cbv descriptors");
            }
        }
        else
        {
            if(!_internalAllocateGPUDescriptorHandles_Sampler(count, tables[i]))
            {
                throw Exception("Fail to allocate gpu sampler descriptors");
            }
        }
    }

    return MakePtr<DirectX12BindingGroup>(this, Ptr(d3dBindingGroupLayout), std::move(tables), variableArraySize);
}

Ptr<BindingLayout> DirectX12Device::CreateBindingLayout(const BindingLayoutDesc &desc)
{
    return MakePtr<DirectX12BindingLayout>(this, desc);
}

void DirectX12Device::UpdateBindingGroups(const BindingGroupUpdateBatch &batch)
{
    // TODO: optimize
    for(auto &record : batch.GetRecords())
    {
        auto group = static_cast<DirectX12BindingGroup *>(record.group);
        record.data.Match(
            [&](auto &r)
            {
                group->ModifyMember(record.index, record.arrayElem, r);
            });
    }
}

void DirectX12Device::CopyBindingGroup(
    const BindingGroupPtr &dstGroup,
    uint32_t               dstIndex,
    uint32_t               dstArrayOffset,
    const BindingGroupPtr &srcGroup,
    uint32_t               srcIndex,
    uint32_t               srcArrayOffset,
    uint32_t               count)
{
    auto d3dSrc = static_cast<DirectX12BindingGroup *>(srcGroup.Get());
    auto d3dDst = static_cast<DirectX12BindingGroup *>(dstGroup.Get());
    auto d3dSrcLayout = static_cast<const DirectX12BindingGroupLayout *>(srcGroup->GetLayout())->_internalGetD3D12Desc();
    auto d3dDstLayout = static_cast<const DirectX12BindingGroupLayout *>(dstGroup->GetLayout())->_internalGetD3D12Desc();
    auto &srcBinding = d3dSrcLayout.bindings[srcIndex];
    auto &dstBinding = d3dDstLayout.bindings[dstIndex];
    auto srcTable = d3dSrc->_internalGetDescriptorTables()[srcBinding.tableIndex];
    auto dstTable = d3dDst->_internalGetDescriptorTables()[dstBinding.tableIndex];

    const size_t handleSize = d3dSrcLayout.tables[srcBinding.tableIndex].type == Helper::D3D12DescTable::SrvUavCbv ?
        gpuDescHeap_CbvSrvUav_->GetHandleSize() : gpuDescHeap_Sampler_->GetHandleSize();

    D3D12_CPU_DESCRIPTOR_HANDLE srcStart = srcTable.cpuHandle;
    srcStart.ptr += handleSize * (srcBinding.offsetInTable + srcArrayOffset);

    D3D12_CPU_DESCRIPTOR_HANDLE dstStart = dstTable.cpuHandle;
    dstStart.ptr += handleSize * (dstBinding.offsetInTable + dstArrayOffset);

    const D3D12_DESCRIPTOR_HEAP_TYPE type =
        d3dSrcLayout.tables[srcBinding.tableIndex].type == Helper::D3D12DescTable::SrvUavCbv ?
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV : D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    device_->CopyDescriptorsSimple(count, dstStart, srcStart, type);
}

Ptr<Texture> DirectX12Device::CreateTexture(const TextureDesc &desc)
{
    D3D12_RESOURCE_DIMENSION dimension;
    if(desc.dim == TextureDimension::Tex2D)
    {
        dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    }
    else if(desc.dim == TextureDimension::Tex3D)
    {
        dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
    }
    else
    {
        Unreachable();
    }

    D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
    if(desc.usage.Contains(TextureUsage::RenderTarget))
    {
        flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    }
    if(desc.usage.Contains(TextureUsage::DepthStencil))
    {
        flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    }
    if(desc.usage.Contains(TextureUsage::UnorderAccess))
    {
        flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }
    if(desc.concurrentAccessMode == QueueConcurrentAccessMode::Concurrent)
    {
        flags |= D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS;
    }
    if(!desc.usage.Contains(TextureUsage::ShaderResource) &&
       desc.usage.Contains(TextureUsage::DepthStencil))
    {
        flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
    }

    const D3D12_RESOURCE_DESC resourceDesc =
    {
        .Dimension        = dimension,
        .Alignment        = 0,
        .Width            = desc.width,
        .Height           = desc.height,
        .DepthOrArraySize = static_cast<UINT16>(desc.dim == TextureDimension::Tex3D ? desc.depth : desc.arraySize),
        .MipLevels        = static_cast<UINT16>(desc.mipLevels),
        .Format           = TranslateFormat(desc.format),
        .SampleDesc       = DXGI_SAMPLE_DESC{ desc.sampleCount, 0 },
        .Layout           = D3D12_TEXTURE_LAYOUT_UNKNOWN,
        .Flags            = flags
    };
    const D3D12MA::ALLOCATION_DESC allocDesc =
    {
        .HeapType = D3D12_HEAP_TYPE_DEFAULT // Upload/readback are done using staging buffers
    };

    ComPtr<D3D12MA::Allocation> rawAlloc;
    ComPtr<ID3D12Resource> resource;
    RTRC_D3D12_FAIL_MSG(
        allocator_->CreateResource(
            &allocDesc, &resourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr,
            rawAlloc.GetAddressOf(), IID_PPV_ARGS(resource.GetAddressOf())),
        "Fail to create directx12 texture resource");

    DirectX12MemoryAllocation alloc = { allocator_.Get(), std::move(rawAlloc) };
    return MakePtr<DirectX12Texture>(desc, this, std::move(resource), std::move(alloc));
}

Ptr<Buffer> DirectX12Device::CreateBuffer(const BufferDesc &desc)
{
    D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE;
    if(desc.usage & (BufferUsage::ShaderRWBuffer | BufferUsage::ShaderRWStructuredBuffer))
    {
        flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }
    assert(!desc.usage.Contains(BufferUsage::AccelerationStructure) && "Not implemented");

    D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT;
    if(desc.hostAccessType == BufferHostAccessType::Upload)
    {
        heapType = D3D12_HEAP_TYPE_UPLOAD;
    }
    else if(desc.hostAccessType == BufferHostAccessType::Readback)
    {
        heapType = D3D12_HEAP_TYPE_READBACK;
    }

    const D3D12_RESOURCE_DESC resourceDesc =
    {
        .Dimension        = D3D12_RESOURCE_DIMENSION_BUFFER,
        .Alignment        = 0,
        .Width            = desc.size,
        .Height           = 1,
        .DepthOrArraySize = 1,
        .MipLevels        = 1,
        .Format           = DXGI_FORMAT_UNKNOWN,
        .SampleDesc       = { 1, 0 },
        .Layout           = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        .Flags            = flags
    };
    const D3D12MA::ALLOCATION_DESC allocDesc =
    {
        .HeapType = heapType
    };

    ComPtr<D3D12MA::Allocation> rawAlloc;
    ComPtr<ID3D12Resource> resource;
    RTRC_D3D12_FAIL_MSG(
        allocator_->CreateResource(
            &allocDesc, &resourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr,
            rawAlloc.GetAddressOf(), IID_PPV_ARGS(resource.GetAddressOf())),
        "Fail to create directx12 buffer resource");

    DirectX12MemoryAllocation alloc = { allocator_.Get(), std::move(rawAlloc) };
    return MakePtr<DirectX12Buffer>(desc, this, std::move(resource), std::move(alloc));
}

Ptr<Sampler> DirectX12Device::CreateSampler(const SamplerDesc &desc)
{
    return MakePtr<DirectX12Sampler>(this, desc);
}

size_t DirectX12Device::GetConstantBufferAlignment() const
{
    return 256;
}

size_t DirectX12Device::GetConstantBufferSizeAlignment() const
{
    return 256;
}

size_t DirectX12Device::GetAccelerationStructureScratchBufferAlignment() const
{
    throw Exception("Not implemented");
}

size_t DirectX12Device::GetTextureBufferCopyRowPitchAlignment(Format texelFormat) const
{
    return D3D12_TEXTURE_DATA_PITCH_ALIGNMENT;
}

void DirectX12Device::WaitIdle()
{
    if(graphicsQueue_)
    {
        graphicsQueue_->WaitIdle();
    }
    if(computeQueue_)
    {
        computeQueue_->WaitIdle();
    }
    if(copyQueue_)
    {
        copyQueue_->WaitIdle();
    }
    // presentQueue_->WaitIdle();
}

BlasPtr DirectX12Device::CreateBlas(const BufferPtr &buffer, size_t offset, size_t size)
{
    throw Exception("Not implemented");
}

TlasPtr DirectX12Device::CreateTlas(const BufferPtr &buffer, size_t offset, size_t size)
{
    throw Exception("Not implemented");
}

BlasPrebuildInfoPtr DirectX12Device::CreateBlasPrebuildInfo(
    Span<RayTracingGeometryDesc> geometries, RayTracingAccelerationStructureBuildFlags flags)
{
    throw Exception("Not implemented");
}

TlasPrebuildInfoPtr DirectX12Device::CreateTlasPrebuildInfo(
    Span<RayTracingInstanceArrayDesc> instanceArrays, RayTracingAccelerationStructureBuildFlags flags)
{
    throw Exception("Not implemented");
}

const ShaderGroupRecordRequirements &DirectX12Device::GetShaderGroupRecordRequirements()
{
    throw Exception("Not implemented");
}

void DirectX12Device::_internalFreeBindingGroup(DirectX12BindingGroup &bindingGroup)
{
    auto &layout = static_cast<const DirectX12BindingGroupLayout *>(bindingGroup.GetLayout())->_internalGetD3D12Desc();
    for(auto &&[i, t] : Enumerate(bindingGroup._internalGetDescriptorTables()))
    {
        if(layout.tables[i].type == Helper::D3D12DescTable::SrvUavCbv)
        {
            gpuDescHeap_CbvSrvUav_->Free(t);
        }
        else
        {
            gpuDescHeap_Sampler_->Free(t);
        }
    }
}

D3D12_CPU_DESCRIPTOR_HANDLE DirectX12Device::_internalOffsetDescriptorHandle(
    const D3D12_CPU_DESCRIPTOR_HANDLE &start, size_t offset, bool isSampler) const
{
    const size_t step = isSampler ? gpuDescHeap_Sampler_->GetHandleSize() : gpuDescHeap_CbvSrvUav_->GetHandleSize();
    return { start.ptr + step * offset };
}

ID3D12CommandSignature *DirectX12Device::_internalGetIndirectDispatchCommandSignature() const
{
    return indirectDispatchCommandSignature_.Get();
}

RTRC_RHI_D3D12_END
