#include <Rtrc/RHI/DirectX12/Context/Device.h>
#include <Rtrc/RHI/DirectX12/Context/Swapchain.h>
#include <Rtrc/RHI/DirectX12/Pipeline/BindingLayout.h>
#include <Rtrc/RHI/DirectX12/Pipeline/BindingGroup.h>
#include <Rtrc/RHI/DirectX12/Pipeline/ComputePipeline.h>
#include <Rtrc/RHI/DirectX12/Pipeline/GraphicsPipeline.h>
#include <Rtrc/RHI/DirectX12/Pipeline/RayTracingLibrary.h>
#include <Rtrc/RHI/DirectX12/Pipeline/RayTracingPipeline.h>
#include <Rtrc/RHI/DirectX12/Pipeline/Shader.h>
#include <Rtrc/RHI/DirectX12/Queue/CommandPool.h>
#include <Rtrc/RHI/DirectX12/Queue/Fence.h>
#include <Rtrc/RHI/DirectX12/Queue/Queue.h>
#include <Rtrc/RHI/DirectX12/Queue/Semaphore.h>
#include <Rtrc/RHI/DirectX12/RayTracing/Blas.h>
#include <Rtrc/RHI/DirectX12/RayTracing/BlasPrebuildInfo.h>
#include <Rtrc/RHI/DirectX12/RayTracing/Tlas.h>
#include <Rtrc/RHI/DirectX12/RayTracing/TlasPrebuildInfo.h>
#include <Rtrc/RHI/DirectX12/Resource/Buffer.h>
#include <Rtrc/RHI/DirectX12/Resource/Sampler.h>
#include <Rtrc/RHI/DirectX12/Resource/Texture.h>
#include <Rtrc/RHI/DirectX12/Resource/TransientResourcePool/TransientResourcePool.h>
#include <Rtrc/Core/Enumerate.h>
#include <Rtrc/Core/Unreachable.h>

RTRC_RHI_D3D12_BEGIN

namespace DirectX12DeviceDetail
{

    D3D12_SHADER_BYTECODE GetByteCode(OPtr<RawShader> shader)
    {
        if(!shader)
        {
            return {};
        }
        return static_cast<DirectX12RawShader *>(shader.Get())->_internalGetShaderByteCode();
    }

} // namespace DirectX12DeviceDetail

DirectX12Device::DirectX12Device(
    ComPtr<ID3D12Device5>  device,
    const Queues          &queues,
    ComPtr<IDXGIFactory4>  factory,
    ComPtr<IDXGIAdapter>   adapter)
    : device_(std::move(device)), factory_(std::move(factory)), adapter_(std::move(adapter))
    , shaderGroupRecordRequirements_{}
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
        graphicsQueue_ = MakeRPtr<DirectX12Queue>(this, queues.graphicsQueue, QueueType::Graphics);
    }
    if(queues.computeQueue)
    {
        computeQueue_ = MakeRPtr<DirectX12Queue>(this, queues.computeQueue, QueueType::Compute);
    }
    if(queues.copyQueue)
    {
        copyQueue_ = MakeRPtr<DirectX12Queue>(this, queues.copyQueue, QueueType::Transfer);
    }
    
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

    const D3D12_INDIRECT_ARGUMENT_DESC indirectDrawIndexedArgDesc =
    {
        .Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED,
    };
    const D3D12_COMMAND_SIGNATURE_DESC indirectDrawIndexedCommandSignatureDesc =
    {
        .ByteStride       = sizeof(D3D12_DRAW_INDEXED_ARGUMENTS),
        .NumArgumentDescs = 1,
        .pArgumentDescs   = &indirectDrawIndexedArgDesc
    };
    RTRC_D3D12_FAIL_MSG(
        device_->CreateCommandSignature(
            &indirectDrawIndexedCommandSignatureDesc, nullptr,
            IID_PPV_ARGS(indirectDrawIndexedCommandSignature_.GetAddressOf())),
        "Fail to create directx12 command signature for indirect indexed draw");

    shaderGroupRecordRequirements_.shaderGroupHandleSize      = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
    shaderGroupRecordRequirements_.shaderGroupHandleAlignment = D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT;
    shaderGroupRecordRequirements_.shaderGroupBaseAlignment   = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
    shaderGroupRecordRequirements_.maxShaderGroupStride       = 4096;

    D3D12_FEATURE_DATA_D3D12_OPTIONS1 dataOptions1;
    RTRC_D3D12_FAIL_MSG(
        device_->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS1, &dataOptions1, sizeof(dataOptions1)),
        "D3D12_FEATURE_DATA_D3D12_OPTIONS1 is not supported");
    warpSizeInfo_.minSize = dataOptions1.WaveLaneCountMin;
    warpSizeInfo_.maxSize = dataOptions1.WaveLaneCountMax;

    srvUavCbvDescriptorSize_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    samplerDescriptorSize_ = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

    RTRC_D3D12_FAIL_MSG(
        device_->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &featureOptions_, sizeof(featureOptions_)),
        "Fail to check d3d12 feature support of D3D12_FEATURE_D3D12_OPTIONS");
}

DirectX12Device::~DirectX12Device()
{
    WaitIdle();
}

RPtr<Queue> DirectX12Device::GetQueue(QueueType type)
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

UPtr<CommandPool> DirectX12Device::CreateCommandPool(const RPtr<Queue> &queue)
{
    ComPtr<ID3D12CommandAllocator> commandAllocator;
    RTRC_D3D12_FAIL_MSG(
        device_->CreateCommandAllocator(
            TranslateCommandListType(queue->GetType()),
            IID_PPV_ARGS(commandAllocator.GetAddressOf())),
        "Fail to create directx12 command allocator");
    return MakeUPtr<DirectX12CommandPool>(this, queue->GetType(), std::move(commandAllocator));
}

UPtr<Swapchain> DirectX12Device::CreateSwapchain(const SwapchainDesc &desc, Window &window)
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
        .usage         = TextureUsage::RenderTarget | TextureUsage::ClearColor |
                         (desc.allowUav ? TextureUsage::UnorderAccess : TextureUsage::None),
        .initialLayout = TextureLayout::Undefined,
        .concurrentAccessMode = QueueConcurrentAccessMode::Exclusive
    };
    return MakeUPtr<DirectX12Swapchain>(this, imageDesc, std::move(swapChain3), desc.vsync ? 1 : 0);
}

UPtr<Fence> DirectX12Device::CreateFence(bool signaled)
{
    ComPtr<ID3D12Fence> fence;
    RTRC_D3D12_FAIL_MSG(
        device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(fence.GetAddressOf())),
        "DirectX12Device::CreateFence: fail to create directx12 fence");
    return MakeUPtr<DirectX12Fence>(std::move(fence), signaled);
}

UPtr<Semaphore> DirectX12Device::CreateTimelineSemaphore(uint64_t initialValue)
{
    ComPtr<ID3D12Fence> fence;
    RTRC_D3D12_FAIL_MSG(
        device_->CreateFence(initialValue, D3D12_FENCE_FLAG_SHARED, IID_PPV_ARGS(fence.GetAddressOf())),
        "DirectX12Device::CreateTimelineSemaphore: fail to create directx12 fence");
    return MakeUPtr<DirectX12Semaphore>(std::move(fence));
}

UPtr<RawShader> DirectX12Device::CreateShader(const void *data, size_t size, std::vector<RawShaderEntry> entries)
{
    std::vector<std::byte> byteCode(size);
    std::memcpy(byteCode.data(), data, size);
    return MakeUPtr<DirectX12RawShader>(std::move(byteCode), std::move(entries));
}

UPtr<GraphicsPipeline> DirectX12Device::CreateGraphicsPipeline(const GraphicsPipelineDesc &desc)
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

    return MakeUPtr<DirectX12GraphicsPipeline>(
        desc.bindingLayout, std::move(rootSignature), std::move(pipelineState),
        staticViewports, staticScissors, vertexStrides, TranslatePrimitiveTopology(desc.primitiveTopology));
}

UPtr<ComputePipeline> DirectX12Device::CreateComputePipeline(const ComputePipelineDesc &desc)
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

    return MakeUPtr<DirectX12ComputePipeline>(desc.bindingLayout, std::move(rootSignature), std::move(pipelineState));
}

UPtr<RayTracingPipeline> DirectX12Device::CreateRayTracingPipeline(const RayTracingPipelineDesc &desc)
{
    CD3DX12_STATE_OBJECT_DESC pipelineDesc(D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE);

    // Collect shader entries

    struct EntryRecord
    {
        std::string name;
        int libraryIndex;
        int shaderIndex;
        int entryIndex;
        std::wstring exportedName;

        const std::wstring &GetExportName()
        {
            if(exportedName.empty())
            {
                exportedName = Utf8ToWin32W(fmt::format(
                    "_{}_{}_{}_{}", name, libraryIndex, shaderIndex, entryIndex));
            }
            return exportedName;
        }
    };
    std::vector<EntryRecord> entryRecords;

    for(auto &&[shaderIndex, shader] : Enumerate(desc.rawShaders))
    {
        auto dxilLibrary = pipelineDesc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();

        auto d3dShader = static_cast<DirectX12RawShader *>(shader.Get());
        dxilLibrary->SetDXILLibrary(&d3dShader->_internalGetShaderByteCode());

        for(auto &&[entryIndex, entry] : Enumerate(d3dShader->_internalGetEntries()))
        {
            entryRecords.push_back({ 
                entry.name, (std::numeric_limits<int>::max)(),
                static_cast<int>(shaderIndex), static_cast<int>(entryIndex), {}
            });
            dxilLibrary->DefineExport(
                entryRecords.back().GetExportName().c_str(),
                Utf8ToWin32W(entry.name).c_str());
        }
    }

    for(auto &&[libraryIndex, library] : Enumerate(desc.libraries))
    {
        auto d3dLibrary = static_cast<DirectX12RayTracingLibrary *>(library.Get());
        auto dxilLibrary = pipelineDesc.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();

        auto d3dShader = static_cast<DirectX12RawShader *>(d3dLibrary->_internalGetDesc().rawShader.Get());
        dxilLibrary->SetDXILLibrary(&d3dShader->_internalGetShaderByteCode());

        for(auto &&[entryIndex, entry] : Enumerate(d3dShader->_internalGetEntries()))
        {
            entryRecords.push_back({ entry.name, static_cast<int>(libraryIndex), 0, static_cast<int>(entryIndex), {} });
            dxilLibrary->DefineExport(
                entryRecords.back().GetExportName().c_str(),
                Utf8ToWin32W(entry.name).c_str());
        }
    }

    // Collect shader groups

    std::vector<std::wstring> groupExportedNames;

    for(auto &&[groupIndex, group] : Enumerate(desc.shaderGroups))
    {
        group.Match(
            [&](const RayTracingHitShaderGroup &hitGroup)
            {
                auto dxilGroup = pipelineDesc.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
                if(hitGroup.closestHitShaderIndex != RAY_TRACING_UNUSED_SHADER)
                {
                    dxilGroup->SetClosestHitShaderImport(
                        entryRecords[hitGroup.closestHitShaderIndex].GetExportName().c_str());
                }
                if(hitGroup.anyHitShaderIndex != RAY_TRACING_UNUSED_SHADER)
                {
                    dxilGroup->SetAnyHitShaderImport(
                        entryRecords[hitGroup.anyHitShaderIndex].GetExportName().c_str());
                }
                if(hitGroup.intersectionShaderIndex != RAY_TRACING_UNUSED_SHADER)
                {
                    dxilGroup->SetIntersectionShaderImport(
                        entryRecords[hitGroup.intersectionShaderIndex].GetExportName().c_str());
                    dxilGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_PROCEDURAL_PRIMITIVE);
                }
                else
                {
                    dxilGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
                }
                // groupName: HitGroup_{libraryIndex}_{groupIndex}
                groupExportedNames.push_back(Utf8ToWin32W(fmt::format(
"HitGroup_{}_{}", (std::numeric_limits<int>::max)(), groupIndex)));
                dxilGroup->SetHitGroupExport(groupExportedNames.back().c_str());
            },
            [&](const RayTracingRayGenShaderGroup &rayGenGroup)
            {
                groupExportedNames.push_back(entryRecords[rayGenGroup.rayGenShaderIndex].GetExportName());
            },
                [&](const RayTracingMissShaderGroup &missGroup)
            {
                groupExportedNames.push_back(entryRecords[missGroup.missShaderIndex].GetExportName());
            },
                [&](const RayTracingCallableShaderGroup &callableGroup)
            {
                groupExportedNames.push_back(entryRecords[callableGroup.callableShaderIndex].GetExportName());
            });
    }

    auto shaderConfig = pipelineDesc.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
    shaderConfig->Config(desc.maxRayPayloadSize, desc.maxRayHitAttributeSize);

    auto d3dBindingLayout = static_cast<DirectX12BindingLayout *>(desc.bindingLayout.Get());
    auto rootSignature = d3dBindingLayout->_internalGetRootSignature(false);
    auto globalRootSignature = pipelineDesc.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
    globalRootSignature->SetRootSignature(rootSignature.Get());

    auto pipelineConfig = pipelineDesc.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
    pipelineConfig->Config(desc.maxRecursiveDepth);

    ComPtr<ID3D12StateObject> stateObject;
    RTRC_D3D12_FAIL_MSG(
        device_->CreateStateObject(pipelineDesc, IID_PPV_ARGS(stateObject.GetAddressOf())),
        "Fail to create directx12 ray tracing pipeline state object");

    return MakeUPtr<DirectX12RayTracingPipeline>(
        std::move(stateObject), desc.bindingLayout, std::move(rootSignature), std::move(groupExportedNames));
}

UPtr<RayTracingLibrary> DirectX12Device::CreateRayTracingLibrary(const RayTracingLibraryDesc &desc)
{
    return MakeUPtr<DirectX12RayTracingLibrary>(desc);
}

UPtr<BindingGroupLayout> DirectX12Device::CreateBindingGroupLayout(const BindingGroupLayoutDesc &desc)
{
    return MakeUPtr<DirectX12BindingGroupLayout>(desc);
}

UPtr<BindingGroup> DirectX12Device::CreateBindingGroup(
    const OPtr<BindingGroupLayout> &bindingGroupLayout, uint32_t variableArraySize)
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

    return MakeUPtr<DirectX12BindingGroup>(this, d3dBindingGroupLayout, std::move(tables), variableArraySize);
}

UPtr<BindingLayout> DirectX12Device::CreateBindingLayout(const BindingLayoutDesc &desc)
{
    return MakeUPtr<DirectX12BindingLayout>(this, desc);
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
    const BindingGroupOPtr &dstGroup,
    uint32_t                dstIndex,
    uint32_t                dstArrayOffset,
    const BindingGroupOPtr &srcGroup,
    uint32_t                srcIndex,
    uint32_t                srcArrayOffset,
    uint32_t                count)
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

UPtr<Texture> DirectX12Device::CreateTexture(const TextureDesc &desc)
{
    const D3D12_RESOURCE_DESC resourceDesc = TranslateTextureDesc(desc);
    const D3D12MA::ALLOCATION_DESC allocDesc =
    {
        .HeapType = D3D12_HEAP_TYPE_DEFAULT // Upload/readback are done using staging buffers
    };

    D3D12_CLEAR_VALUE clearValue = {}, *pClearValue = nullptr;
    if(!desc.clearValue.Is<std::monostate>())
    {
        clearValue.Format = resourceDesc.Format;
        pClearValue = &clearValue;
        if(auto c = desc.clearValue.AsIf<ColorClearValue>())
        {
            clearValue.Color[0] = c->r;
            clearValue.Color[1] = c->g;
            clearValue.Color[2] = c->b;
            clearValue.Color[3] = c->a;
        }
        else
        {
            auto d = desc.clearValue.AsIf<DepthStencilClearValue>();
            assert(d);
            clearValue.DepthStencil.Depth = d->depth;
            clearValue.DepthStencil.Stencil = d->stencil;
        }
    }

    ComPtr<D3D12MA::Allocation> rawAlloc;
    ComPtr<ID3D12Resource> resource;
    RTRC_D3D12_FAIL_MSG(
        allocator_->CreateResource(
            &allocDesc, &resourceDesc, D3D12_RESOURCE_STATE_COMMON, pClearValue,
            rawAlloc.GetAddressOf(), IID_PPV_ARGS(resource.GetAddressOf())),
        "Fail to create directx12 texture resource");
    
    DirectX12MemoryAllocation alloc = { std::move(rawAlloc) };
    return MakeUPtr<DirectX12Texture>(desc, this, std::move(resource), std::move(alloc));
}

UPtr<Buffer> DirectX12Device::CreateBuffer(const BufferDesc &desc)
{
    D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT;
    if(desc.hostAccessType == BufferHostAccessType::Upload)
    {
        heapType = D3D12_HEAP_TYPE_UPLOAD;
    }
    else if(desc.hostAccessType == BufferHostAccessType::Readback)
    {
        heapType = D3D12_HEAP_TYPE_READBACK;
    }
    const D3D12MA::ALLOCATION_DESC allocDesc = { .HeapType = heapType };
    const D3D12_RESOURCE_DESC resourceDesc = TranslateBufferDesc(desc);

    D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON;
    if(desc.usage.Contains(BufferUsage::AccelerationStructure))
    {
        initialState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
    }

    ComPtr<D3D12MA::Allocation> rawAlloc;
    ComPtr<ID3D12Resource> resource;
    RTRC_D3D12_FAIL_MSG(
        allocator_->CreateResource(
            &allocDesc, &resourceDesc, initialState, nullptr,
            rawAlloc.GetAddressOf(), IID_PPV_ARGS(resource.GetAddressOf())),
        "Fail to create directx12 buffer resource");

    DirectX12MemoryAllocation alloc = { std::move(rawAlloc) };
    return MakeUPtr<DirectX12Buffer>(desc, this, std::move(resource), std::move(alloc));
}

UPtr<Sampler> DirectX12Device::CreateSampler(const SamplerDesc &desc)
{
    return MakeUPtr<DirectX12Sampler>(this, desc);
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
    return 256;
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
}

BlasUPtr DirectX12Device::CreateBlas(const BufferRPtr &buffer, size_t offset, size_t size)
{
    auto ret = MakeUPtr<DirectX12Blas>();
    auto d3dBuffer = static_cast<DirectX12Buffer *>(buffer.Get());
    auto address = d3dBuffer->GetDeviceAddress().address + offset;
    ret->_internalSetBuffer(BufferDeviceAddress{ address }, buffer);
    return BlasUPtr(ret.Release());
}

TlasUPtr DirectX12Device::CreateTlas(const BufferRPtr &buffer, size_t offset, size_t size)
{
    auto d3dBuffer = static_cast<DirectX12Buffer *>(buffer.Get());
    auto address = d3dBuffer->GetDeviceAddress().address + offset;

    auto srv = _internalAllocateCPUDescriptorHandle_CbvSrvUav();
    RTRC_SCOPE_FAIL{ _internalFreeCPUDescriptorHandle_CbvSrvUav(srv); };

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
    srvDesc.Format                                   = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension                            = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
    srvDesc.Shader4ComponentMapping                  = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.RaytracingAccelerationStructure.Location = address;
    device_->CreateShaderResourceView(nullptr, &srvDesc, srv);

    return MakeUPtr<DirectX12Tlas>(this, BufferDeviceAddress{ address }, buffer, srv);
}

BlasPrebuildInfoUPtr DirectX12Device::CreateBlasPrebuildInfo(
    Span<RayTracingGeometryDesc> geometries, RayTracingAccelerationStructureBuildFlags flags)
{
    return MakeUPtr<DirectX12BlasPrebuildInfo>(this, geometries, flags);
}

TlasPrebuildInfoUPtr DirectX12Device::CreateTlasPrebuildInfo(
    const RayTracingInstanceArrayDesc &instances, RayTracingAccelerationStructureBuildFlags flags)
{
    return MakeUPtr<DirectX12TlasPrebuildInfo>(this, instances, flags);
}

const ShaderGroupRecordRequirements &DirectX12Device::GetShaderGroupRecordRequirements() const
{
    return shaderGroupRecordRequirements_;
}

const WarpSizeInfo &DirectX12Device::GetWarpSizeInfo() const
{
    return warpSizeInfo_;
}

UPtr<TransientResourcePool> DirectX12Device::CreateTransientResourcePool(const TransientResourcePoolDesc &desc)
{
    return MakeUPtr<DirectX12TransientResourcePool>(this, desc);
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

ID3D12CommandSignature *DirectX12Device::_internalGetIndirectDrawIndexedCommandSignature() const
{
    return indirectDrawIndexedCommandSignature_.Get();
}

const D3D12_FEATURE_DATA_D3D12_OPTIONS &DirectX12Device::_internalGetFeatureOptions() const
{
    return featureOptions_;
}

RTRC_RHI_D3D12_END
